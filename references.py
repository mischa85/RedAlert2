"""Check names inferred from the references of matched code (docs/NAMES.md).

Usage: python3 references.py TIBSUN_CODE [VERSION] [--symbol NAME]

TIBSUN_CODE is the code/ directory of the Tiberian Sun reconstruction
(https://github.com/OpenTS-Developers/TibSun). Its source files are compiled
with the project's flags (see compile.py), with it and its subdirectories as
include paths, into build/VERSION/tibsun/; files that fail to compile are
skipped and listed.

Matched code is:
  - each function of the reconstruction whose code section meets the match
    rule of docs/FUNCTIONS.md: its bytes, with its relocated fields masked,
    occur at exactly one address in the code section, it is at least 32 bytes
    long, and at least 60% of its bytes are not relocated;
  - each function compiled from src/ at the address names.csv gives it.
A source file in src/ replaces the reconstruction's file of the same name.

Every relocation of matched code to a symbol outside its own section pairs
that symbol with an address: the target of a REL32 field, or the value of a
DIR32 field less the addend the object gives. Each field counts once.

Without --symbol, every names.csv row whose evidence says it is "referenced
by matched src/..." is checked: the symbol must be paired only with that
address, the address only with that symbol, and the number of references in
the evidence must equal the number of fields. Exits 1 if any check fails.
With --symbol, prints every address the symbol is paired with.
"""
import csv
import glob
import os
import re
import struct
import subprocess
import sys
import tempfile

import compile as compiler

ROOT = os.path.dirname(os.path.abspath(__file__))
REL32, DIR32 = 0x14, 0x06
COMDAT_NO_DUPLICATES = 1
# Symbols the compiler makes for each object on its own (docs/NAMES.md).
PER_OBJECT = re.compile(r"^(_\$E|__real@|_\?|\?CELL_LEPTON_DIAG@@3NB$)")  # CELL_LEPTON_DIAG: every module computes its own copy (geometry.h)
EVIDENCE = re.compile(r"referenced by matched src/(\S+?)\.cpp \((\d+) references?\)")


def read_executable(version):
    with open(os.path.join(ROOT, "orig", version, "game.exe"), "rb") as f:
        image = f.read()
    pe = struct.unpack_from("<I", image, 0x3C)[0]
    count = struct.unpack_from("<H", image, pe + 6)[0]
    opt = struct.unpack_from("<H", image, pe + 20)[0]
    base = struct.unpack_from("<I", image, pe + 24 + 28)[0]
    for i in range(count):
        name, vsize, vaddr, raw_size, raw_ptr = struct.unpack_from("<8sIIII", image, pe + 24 + opt + i * 40)
        if name.rstrip(b"\0") == b".text":
            return image[raw_ptr:raw_ptr + min(vsize, raw_size)], base + vaddr
    sys.exit("no .text section in game.exe")


def read_object(path):
    """Code sections that are their object's own: [(symbol, data, relocs)], relocs (offset, type, symbol, section)."""
    with open(path, "rb") as f:
        b = f.read()
    _, nsections, _, symtab, nsyms = struct.unpack_from("<HHIII", b, 0)
    optional = struct.unpack_from("<H", b, 16)[0]
    strings = symtab + nsyms * 18

    def name_of(raw):
        if raw[:4] == b"\0\0\0\0":
            offset = strings + struct.unpack_from("<I", raw, 4)[0]
            return b[offset:b.index(b"\0", offset)].decode("latin-1")
        return raw.rstrip(b"\0").decode("latin-1")

    symbols, selection, i = {}, {}, 0
    while i < nsyms:
        p = symtab + i * 18
        value, section, typ, storage, aux = struct.unpack_from("<IhHBB", b, p + 8)
        symbols[i] = (name_of(b[p:p + 8]), value, section, typ)
        if storage == 3 and aux:
            selection[section] = b[p + 18 + 14]
        i += 1 + aux
    sections = []
    for k in range(nsections):
        h = 20 + optional + k * 40
        size, raw, rel, _, nrel = struct.unpack_from("<IIIIH", b, h + 16)
        flags = struct.unpack_from("<I", b, h + 36)[0]
        relocs = []
        for j in range(nrel):
            offset, index, typ = struct.unpack_from("<IIH", b, rel + j * 10)
            name, _, section, _ = symbols[index]
            relocs.append((offset, typ, name, section))
        sections.append((flags, b[raw:raw + size], relocs))
    own = []
    for name, value, section, typ in symbols.values():
        if section <= 0 or value or typ != 0x20 or name.startswith((".", "$")):
            continue
        flags, data, relocs = sections[section - 1]
        if flags & 0x20 and selection.get(section) == COMDAT_NO_DUPLICATES:
            own.append((name, section, data, relocs))
    return own


def find(text, base, data, masked):
    """Addresses where data occurs in text with the masked offsets ignored."""
    runs, start = [], None
    for i in range(len(data) + 1):
        if i < len(data) and i not in masked:
            start = i if start is None else start
        elif start is not None:
            runs.append((i - start, start))
            start = None
    if not runs:
        return []
    length, anchor = max(runs)
    needle = data[anchor:anchor + length]
    found, at = [], text.find(needle)
    while at != -1:
        s = at - anchor
        if 0 <= s <= len(text) - len(data) and all(
                i in masked or text[s + i] == data[i] for i in range(len(data))):
            found.append(base + s)
        at = text.find(needle, at + 1)
    return found


def compile_reconstruction(code_dir, out_dir):
    cl = os.path.join(os.environ.get("MSVC6_DIR", os.path.join(ROOT, "toolchain", "msvc6")), "wine", "x86", "cl")
    env = compiler.environment()
    os.makedirs(out_dir, exist_ok=True)
    failed = []
    for source in sorted(glob.glob(os.path.join(code_dir, "*.cpp"))):
        obj = os.path.join(out_dir, os.path.splitext(os.path.basename(source))[0].lower() + ".obj")
        if os.path.exists(obj) and os.path.getmtime(obj) >= os.path.getmtime(source):
            continue
        includes = [f"/I{d}" for d in [code_dir] + sorted(glob.glob(os.path.join(code_dir, "*", "")))]
        with tempfile.TemporaryFile() as log:
            # The reconstruction is compiled without WIN32, as its headers predate RA2 types that
            # the WIN32-only files would otherwise pair with wrong names.
            flags = [f for f in compiler.FLAGS if f != "/DWIN32"]
            result = subprocess.run([cl, *flags, *includes, f"/Fo{obj}", source],
                                    stdout=log, stderr=subprocess.STDOUT, env=env)
        if result.returncode != 0:
            failed.append(os.path.basename(source))
            if os.path.exists(obj):
                os.remove(obj)
    return failed


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    symbol_query = sys.argv[sys.argv.index("--symbol") + 1] if "--symbol" in sys.argv else None
    if symbol_query in args:
        args.remove(symbol_query)
    if not args:
        sys.exit(__doc__)
    code_dir = os.path.abspath(args[0])
    version = args[1] if len(args) > 1 else "ra2-1.006"

    text, base = read_executable(version)
    with open(os.path.join(ROOT, "config", version, "names.csv"), newline="") as f:
        names = list(csv.DictReader(f))
    address_of = {n["symbol"]: int(n["address"], 16) for n in names}

    tibsun_dir = os.path.join(ROOT, "build", version, "tibsun")
    failed = compile_reconstruction(code_dir, tibsun_dir)
    src_units = {os.path.splitext(os.path.basename(p))[0].lower()
                 for p in glob.glob(os.path.join(ROOT, "src", "*.c*"))}
    objects = [(p, False) for p in sorted(glob.glob(os.path.join(tibsun_dir, "*.obj")))
               if os.path.splitext(os.path.basename(p))[0] not in src_units]
    objects += [(p, True) for p in sorted(glob.glob(os.path.join(ROOT, "build", version, "src", "*.obj")))]

    fields = {}  # field address -> (symbol, address)
    for path, from_src in objects:
        for name, section, data, relocs in read_object(path):
            masked = {o + i for o, typ, _, _ in relocs if typ in (REL32, DIR32) for i in range(4)}
            if from_src:
                if name not in address_of:
                    continue
                at = address_of[name]
                if text[at - base:at - base + len(data)] and any(
                        i not in masked and text[at - base + i] != data[i] for i in range(len(data))):
                    continue  # not matched (yet)
            else:
                if len(data) < 32 or len(data) - len(masked) < 0.6 * len(data):
                    continue
                hits = find(text, base, data, masked)
                if len(hits) != 1:
                    continue
                at = hits[0]
            for offset, typ, target_name, target_section in relocs:
                if typ not in (REL32, DIR32) or target_section == section or target_name.startswith((".", "$")):
                    continue
                field = at + offset
                value = struct.unpack_from("<i", text, field - base)[0]
                if typ == REL32:
                    target = field + 4 + value
                else:
                    target = (value - struct.unpack_from("<i", data, offset)[0]) & 0xFFFFFFFF
                fields[field] = (target_name, target)

    by_symbol, by_address = {}, {}
    for field, (symbol, target) in fields.items():
        by_symbol.setdefault(symbol, {}).setdefault(target, []).append(field)
        by_address.setdefault(target, set()).add(symbol)

    if failed:
        print(f"not compiled ({len(failed)}): {' '.join(failed)}")
    print(f"{len(fields)} fields of matched code, {len(by_symbol)} symbols")

    if symbol_query:
        for target, refs in sorted(by_symbol.get(symbol_query, {}).items()):
            others = sorted(by_address[target] - {symbol_query})
            print(f"0x{target:08X}: {len(refs)} references" + (f"; also {', '.join(others)}" if others else ""))
        return

    problems = 0
    for n in names:
        m = EVIDENCE.search(n["evidence"])
        if not m:
            continue
        symbol, address, stated = n["symbol"], int(n["address"], 16), int(m.group(2))
        seen = by_symbol.get(symbol, {})
        issue = None
        if PER_OBJECT.match(symbol):
            issue = "a per-object symbol"
        elif set(seen) != {address}:
            issue = "paired with " + ", ".join(f"0x{a:08X}" for a in sorted(seen))
        elif by_address[address] != {symbol}:
            issue = "address also has " + ", ".join(sorted(by_address[address] - {symbol}))
        elif len(seen[address]) != stated:
            issue = f"{len(seen[address])} references, evidence says {stated}"
        status = "ok  " if issue is None else "FAIL"
        problems += issue is not None
        print(f"{status} 0x{address:08X} {symbol}" + (f": {issue}" if issue else f" ({stated})"))
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
