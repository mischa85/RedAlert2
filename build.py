"""Build game.exe and verify it is byte-identical to the original.

Usage: python3 build.py [VERSION]        (default: ra2-1.006)

The executable is assembled from the original, orig/VERSION/game.exe, with every
function rebuilt from source written over its original bytes. A function is
rebuilt from source when an object compiled from src/ (see compile.py) defines
it, config/VERSION/names.csv gives its address, all its relocations resolve, and
the result reproduces the original bytes exactly. Anything else is left to the
original and reported, so work in progress never breaks the build.

The result must have the same SHA-256 as the original (config/VERSION/game.sha256).
"""
import csv
import glob
import hashlib
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
IMAGE_SCN_CNT_CODE = 0x20
IMAGE_REL_I386_DIR32 = 0x06
IMAGE_REL_I386_REL32 = 0x14
RAW_NAME = re.compile(r"(?:fn|dat)_([0-9A-F]{8})")


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def read_coff(data):
    """Returns (sections, symbols): sections as dicts with data, flags and
    relocations (offset, symbol index, type); symbols by index as
    (name, section index or -1, value)."""
    _, nsections, _, symtab, nsyms, opt_size, _ = struct.unpack_from("<HHIIIHH", data, 0)
    strtab = symtab + nsyms * 18

    def name_of(raw):
        if raw[:4] == b"\0\0\0\0":
            start = strtab + struct.unpack_from("<I", raw, 4)[0]
            return data[start:data.index(b"\0", start)].decode("latin-1")
        return raw.rstrip(b"\0").decode("latin-1")

    sections = []
    for i in range(nsections):
        (_, _, _, size, raw_ptr, reloc_ptr, _, nrelocs, _, flags) = struct.unpack_from(
            "<8sIIIIIIHHI", data, 20 + opt_size + i * 40)
        relocs = [struct.unpack_from("<IIH", data, reloc_ptr + r * 10) for r in range(nrelocs)]
        sections.append({"data": data[raw_ptr:raw_ptr + size], "flags": flags, "relocs": relocs})
    symbols = {}
    i = 0
    while i < nsyms:
        raw, value, section, _, _, naux = struct.unpack_from("<8sIhHBB", data, symtab + i * 18)
        symbols[i] = (name_of(raw), section - 1, value)
        i += 1 + naux
    return sections, symbols


def file_offset_reader(original):
    pe = struct.unpack_from("<I", original, 0x3C)[0]
    count = struct.unpack_from("<H", original, pe + 6)[0]
    opt = struct.unpack_from("<H", original, pe + 20)[0]
    base = struct.unpack_from("<I", original, pe + 24 + 28)[0]
    sections = [struct.unpack_from("<8sIIII", original, pe + 24 + opt + i * 40) for i in range(count)]

    def offset(va, size):
        for _, vsize, vaddr, raw_size, raw_ptr in sections:
            start = base + vaddr
            if start <= va and va + size <= start + min(vsize, raw_size):
                return raw_ptr + va - start
        return None
    return offset


def rebuild_function(code, relocs, symbols, section_index, address, addresses):
    """The function's bytes at `address`, or a reason it cannot be rebuilt."""
    code = bytearray(code)
    for offset, symbol_index, kind in relocs:
        name, target_section, value = symbols[symbol_index]
        if target_section == section_index:
            target = address + value
        elif name in addresses:
            target = addresses[name]
        elif RAW_NAME.search(name):
            target = int(RAW_NAME.search(name).group(1), 16)
        else:
            return None, f"relocation to {name} has no known address"
        addend = struct.unpack_from("<i", code, offset)[0]
        if kind == IMAGE_REL_I386_DIR32:
            struct.pack_into("<I", code, offset, (target + addend) & 0xFFFFFFFF)
        elif kind == IMAGE_REL_I386_REL32:
            struct.pack_into("<I", code, offset, (target + addend - (address + offset + 4)) & 0xFFFFFFFF)
        else:
            return None, f"unsupported relocation type {kind}"
    return bytes(code), None


def main():
    version = sys.argv[1] if len(sys.argv) > 1 else "ra2-1.006"
    with open(os.path.join(ROOT, "config", version, "game.sha256")) as f:
        expected = f.read().split()[0].lower()

    original_path = os.path.join(ROOT, "orig", version, "game.exe")
    if not os.path.exists(original_path):
        sys.exit(f"missing {original_path}; see orig/README.md")
    with open(original_path, "rb") as f:
        original = f.read()
    if sha256(original) != expected:
        sys.exit(f"{original_path} is not the expected {version} executable "
                 f"(SHA-256 {sha256(original)}, expected {expected})")

    with open(os.path.join(ROOT, "config", version, "names.csv"), newline="") as f:
        addresses = {r["symbol"]: int(r["address"], 16) for r in csv.DictReader(f)}
    offset_of = file_offset_reader(original)
    output = bytearray(original)
    rebuilt, pending = [], []

    for obj_path in sorted(glob.glob(os.path.join(ROOT, "build", version, "src", "*.obj"))):
        unit = os.path.splitext(os.path.basename(obj_path))[0]
        if not any(os.path.exists(os.path.join(ROOT, "src", unit + e)) for e in (".cpp", ".c")):
            continue  # left over from a source file that no longer exists
        with open(obj_path, "rb") as f:
            sections, symbols = read_coff(f.read())
        for name, section_index, value in symbols.values():
            if section_index < 0 or value != 0 or name.startswith((".", "$")):
                continue
            section = sections[section_index]
            if not section["flags"] & IMAGE_SCN_CNT_CODE:
                continue
            if name not in addresses:
                pending.append((unit, name, "no address in names.csv"))
                continue
            address = addresses[name]
            code, problem = rebuild_function(section["data"], section["relocs"], symbols,
                                             section_index, address, addresses)
            offset = offset_of(address, len(section["data"]))
            if problem is None and offset is None:
                problem = "address outside the executable's sections"
            if problem is None and original[offset:offset + len(code)] != code:
                problem = "does not match the original yet"
            if problem:
                pending.append((unit, name, problem))
                continue
            output[offset:offset + len(code)] = code
            rebuilt.append((unit, name, len(code)))

    output_path = os.path.join(ROOT, "build", version, "game.exe")
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "wb") as f:
        f.write(output)

    actual = sha256(output)
    if actual != expected:
        sys.exit(f"FAIL: {output_path} differs from the original (SHA-256 {actual})")
    for unit, name, problem in pending:
        print(f"  pending  {unit}: {name}: {problem}")
    print(f"OK: {output_path} is byte-identical to {version} (SHA-256 {expected})")
    print(f"{sum(size for _, _, size in rebuilt)} of {len(output)} bytes built from source "
          f"({len(rebuilt)} functions; {len(pending)} pending)")


if __name__ == "__main__":
    main()
