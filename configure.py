"""Check config/ and generate the target objects and objdiff.json from it.

Usage: python3 configure.py [VERSION]        (default: ra2-1.006)

config/VERSION/ holds what is known about the executable, every row saying how
it was established:

  functions.csv     known function starts in the code section, with the size
                    where it is known
  modules.csv       code units: address ranges covering the code section; a
                    unit may have several ranges
  data.csv          data units: address ranges in the initialized data; a range
                    may belong to a code unit, which then owns that data too
  relocations.csv   known address fields: the symbol's address they refer to
                    and the addend; the field holds target + addend
  names.csv         symbol names by address; anything unnamed is fn_<address>
                    or dat_<address>

The code is divided into pieces: each known function, from its start to its
end (start + size, plus the padding bytes after it; in a unit with a source
file only 0x90, and the linker's 0xCC fill after it a piece without a symbol),
or, if its size is not known, to the next known start; and chunk_<address> for
code between them that no known function covers.

The config is checked first; any inconsistency stops with a list of problems.
Then this writes:

  build/VERSION/target/*.obj    one object per unit, holding the original
                                bytes: one section per piece and per data
                                range, and the known relocations
  objdiff.json                  one unit per object; a unit with a source file
                                src/<unit>.cpp (or .c) also gets the object
                                compile.py builds from it

Section layout is read from orig/VERSION/game.exe, which must match
config/VERSION/game.sha256.
"""
import bisect
import csv
import hashlib
import json
import os
import re
import shutil
import struct
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
CODE_FLAGS = 0x60501020  # code, COMDAT, 16-byte aligned, execute, read
COMDAT_NO_DUPLICATES = 1
FUNCTION_TYPE = 0x20
EXTERNAL = 2
IMAGE_REL_I386_DIR32 = 0x06
IMAGE_REL_I386_REL32 = 0x14
PADDING = (0x90, 0xCC)
ALIAS = re.compile(r"alias of ([^ ,;:]+)")  # names.csv evidence of a second name for an address
ENTRY = re.compile(r"entry point of ")  # names.csv evidence of a symbol inside a known function


def read_csv(version, name):
    with open(os.path.join(ROOT, "config", version, name), newline="") as f:
        return list(csv.DictReader(f))


def source_file(unit):
    for extension in (".cpp", ".c"):
        if os.path.exists(os.path.join(ROOT, "src", unit + extension)):
            return f"src/{unit}{extension}"
    return None


def load_executable(version):
    path = os.path.join(ROOT, "orig", version, "game.exe")
    with open(path, "rb") as f:
        data = f.read()
    with open(os.path.join(ROOT, "config", version, "game.sha256")) as f:
        if hashlib.sha256(data).hexdigest() != f.read().split()[0].lower():
            sys.exit(f"{path} is not the expected {version} executable; see orig/README.md")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    opt = struct.unpack_from("<H", data, pe + 20)[0]
    base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    sections = []
    for i in range(count):
        name, vsize, vaddr, raw_size, raw_ptr, _, _, _, _, flags = struct.unpack_from(
            "<8sIIIIIIHHI", data, pe + 24 + opt + i * 40)
        sections.append({"name": name.rstrip(b"\0").decode(), "va": base + vaddr,
                         "vsize": vsize, "raw_ptr": raw_ptr, "raw_size": raw_size,
                         "flags": flags})
    return data, sections


def section_of(sections, va):
    for s in sections:
        if s["va"] <= va < s["va"] + max(s["vsize"], s["raw_size"]):
            return s
    return None


def file_offset(sections, va):
    """File offset of va, which must lie in a section's raw bytes."""
    s = section_of(sections, va)
    if s is None:
        raise ValueError(f"0x{va:08X} is outside every section")
    return s["raw_ptr"] + va - s["va"]


def pieces_of(image, text, functions, compiled=frozenset()):
    """(start, end, is_function) covering the code section, in order.

    A function in `compiled` (in a unit with a source file) takes only the
    compiler's own NOP padding; the linker's 0xCC fill after it, which no
    compiled object holds, becomes a piece of its own (is_function None)
    without a symbol, so that objdiff has nothing to compare it with."""
    text_end = text["va"] + text["vsize"]
    byte_at = lambda va: image[text["raw_ptr"] + va - text["va"]]
    pieces, cursor = [], text["va"]
    for i, (start, size) in enumerate(functions):
        following = functions[i + 1][0] if i + 1 < len(functions) else text_end
        if start > cursor:
            pieces.append((cursor, start, False))
        if size is None:
            end = following
        else:
            end = start + size
            padding = (0x90,) if start in compiled else PADDING
            while end < following and byte_at(end) in padding:
                end += 1
        pieces.append((start, end, True))
        cursor = end
        if start in compiled:
            fill = end
            while fill < following and byte_at(fill) == 0xCC:
                fill += 1
            if fill > end:
                pieces.append((end, fill, None))
                cursor = fill
    if cursor < text_end:
        pieces.append((cursor, text_end, False))
    return pieces


def check_functions(text, functions):
    """Problems with the function starts and sizes; empty if consistent."""
    problems = []
    text_end = text["va"] + text["vsize"]
    starts = [start for start, _ in functions]
    if starts != sorted(set(starts)):
        problems.append("functions.csv: addresses must be unique and in ascending order")
    for i, (start, size) in enumerate(functions):
        following = starts[i + 1] if i + 1 < len(starts) else text_end
        if not text["va"] <= start < text_end:
            problems.append(f"functions.csv: 0x{start:08X} is outside the code section")
        elif size is not None and (size <= 0 or start + size > following):
            problems.append(f"functions.csv: 0x{start:08X} with size {size} overlaps the next function")
    return problems


def check_rest(image, sections, text, pieces, functions, units, data_units, relocs, names):
    """Problems with the rest of the config, given consistent functions."""
    problems = []
    text_end = text["va"] + text["vsize"]
    piece_starts = [start for start, _, _ in pieces]
    boundaries = set(piece_starts) | {text_end}
    function_starts = {start for start, _ in functions}

    expected = text["va"]
    for u in units:
        start, end = int(u["start"], 16), int(u["end"], 16)
        if start != expected:
            problems.append(f"modules.csv: {u['unit']} range starts at 0x{start:08X}, expected 0x{expected:08X} "
                            "(ranges must be adjacent)")
        for va in (start, end):
            if va not in boundaries:
                problems.append(f"modules.csv: {u['unit']}: 0x{va:08X} is not the start or end of a function or chunk")
        if not u.get("evidence"):
            problems.append(f"modules.csv: {u['unit']} has no evidence")
        expected = end
    if expected != text_end:
        problems.append(f"modules.csv: ranges end at 0x{expected:08X}, not at the code section's end 0x{text_end:08X}")

    data_ranges = []
    for d in data_units:
        start, end = int(d["start"], 16), int(d["end"], 16)
        s = section_of(sections, start)
        if s is None or s is text or end > s["va"] + min(s["vsize"], s["raw_size"]) or end <= start:
            problems.append(f"data.csv: {d['unit']} 0x{start:08X}-0x{end:08X} must lie within one "
                            "section's initialized data")
        if not d.get("evidence"):
            problems.append(f"data.csv: {d['unit']} has no evidence")
        data_ranges.append((start, end, d["unit"]))
    data_ranges.sort()
    for (_, end1, unit1), (start2, _, unit2) in zip(data_ranges, data_ranges[1:]):
        if start2 < end1:
            problems.append(f"data.csv: {unit1} and {unit2} overlap")

    for r in relocs:
        field, target = int(r["field"], 16), int(r["target"], 16)
        if r["type"] not in ("DIR32", "REL32"):
            problems.append(f"relocations.csv: 0x{field:08X} has unknown type {r['type']}")
        if text["va"] <= field < text_end:
            i = bisect.bisect_right(piece_starts, field)
            if field + 4 > (pieces[i][0] if i < len(pieces) else text_end):
                problems.append(f"relocations.csv: field 0x{field:08X} crosses the start of 0x{pieces[i][0]:08X}")
        elif not any(start <= field and field + 4 <= end for start, end, _ in data_ranges):
            problems.append(f"relocations.csv: field 0x{field:08X} is not in the code or a data unit")
        if section_of(sections, target) is None:
            problems.append(f"relocations.csv: 0x{field:08X} points outside the executable (0x{target:08X})")
        elif section_of(sections, field) is not None:
            value = struct.unpack_from("<I", image, file_offset(sections, field))[0]
            if r["type"] == "REL32":
                value = (value + field + 4) & 0xFFFFFFFF
            if value != (target + int(r["addend"])) & 0xFFFFFFFF:
                problems.append(f"relocations.csv: 0x{field:08X} holds 0x{value:08X}, not target + addend")
        if not r.get("evidence"):
            problems.append(f"relocations.csv: 0x{field:08X} has no evidence")

    seen_addresses, seen_symbols = set(), set()
    primary = {int(n["address"], 16): n["symbol"] for n in names if not ALIAS.match(n["evidence"])}
    for n in names:
        address = int(n["address"], 16)
        alias = ALIAS.match(n["evidence"])
        if alias:
            if primary.get(address) != alias.group(1):
                problems.append(f"names.csv: {n['symbol']} is an alias of {alias.group(1)}, "
                                f"which is not the name at 0x{address:08X}")
        elif address in seen_addresses:
            problems.append(f"names.csv: 0x{address:08X} is named twice")
        if n["symbol"] in seen_symbols:
            problems.append(f"names.csv: {n['symbol']} is used twice")
        if not alias:
            seen_addresses.add(address)
        seen_symbols.add(n["symbol"])
        if section_of(sections, address) is None:
            problems.append(f"names.csv: 0x{address:08X} is outside the executable")
        elif text["va"] <= address < text_end and address not in function_starts and not (
                ENTRY.match(n["evidence"]) and any(p[0] < address < p[1] and p[2] for p in pieces)):
            problems.append(f"names.csv: 0x{address:08X} is in the code but not a known function start")
        if not n.get("evidence"):
            problems.append(f"names.csv: {n['symbol']} has no evidence")
    return problems


def coff_object(image, pe_sections, own, data_symbols, relocs_by_section):
    """A COFF object holding the given sections of the executable: one section per
    function, chunk or data range, a symbol at each of their starts (and at each
    data address in data_symbols), and their relocations, each field holding the
    addend to its symbol as a compiler's object would."""
    names, records = {}, []  # symbol name -> index; 18-byte symbol records
    strings = bytearray()

    def name_field(name):
        encoded = name.encode("latin-1")
        if len(encoded) <= 8:
            return encoded.ljust(8, b"\0")
        offset = 4 + len(strings)
        strings.extend(encoded + b"\0")
        return struct.pack("<II", 0, offset)

    def add(name, value, section, typ, storage, aux=None):
        index = len(records)
        records.append(struct.pack("<8sIhHBB", name_field(name), value, section, typ, storage, 1 if aux else 0))
        if aux:
            records.append(aux)
        return index

    bodies = []
    for number, (_, start, end, section_name, flags, symbol) in enumerate(own, 1):
        data = bytearray(image[file_offset(pe_sections, start):file_offset(pe_sections, end)])
        relocs = relocs_by_section.get(start, [])
        selection = COMDAT_NO_DUPLICATES if flags & 0x1000 else 0
        aux = struct.pack("<IHHIHB3x", len(data), len(relocs), 0, 0, number, selection)
        add(section_name, 0, number, 0, 3, aux)
        if symbol is not None:
            names[symbol] = add(symbol, 0, number, FUNCTION_TYPE, EXTERNAL)
        else:
            for address in sorted(a for a in data_symbols if start <= a < end):
                names[data_symbols[address]] = add(data_symbols[address], address - start, number, 0, EXTERNAL)
        bodies.append((section_name, flags, start, data, relocs))
    for _, _, start, data, relocs in bodies:
        for field, _, symbol, addend in relocs:
            if symbol not in names:
                names[symbol] = add(symbol, 0, 0, 0, EXTERNAL)
            struct.pack_into("<i", data, field - start, addend)

    header_size = 20 + 40 * len(bodies)
    layout, position = [], header_size
    for _, _, _, data, relocs in bodies:
        if len(relocs) > 0xFFFF:
            raise ValueError("more than 65535 relocations in one section")
        layout.append((position, position + len(data)))
        position += len(data) + 10 * len(relocs)
    out = bytearray(struct.pack("<HHIIIHH", 0x14C, len(bodies), 0, position, len(records), 0, 0))
    for (section_name, flags, _, data, relocs), (data_at, relocs_at) in zip(bodies, layout):
        out += struct.pack("<8sIIIIIIHHI", section_name.encode(), 0, 0, len(data), data_at,
                           relocs_at if relocs else 0, 0, len(relocs), 0, flags)
    for _, _, start, data, relocs in bodies:
        out += data
        for field, is_relative, symbol, _ in relocs:
            out += struct.pack("<IIH", field - start, names[symbol],
                               IMAGE_REL_I386_REL32 if is_relative else IMAGE_REL_I386_DIR32)
    out += b"".join(records)
    out += struct.pack("<I", 4 + len(strings)) + strings
    return bytes(out)


def main():
    version = sys.argv[1] if len(sys.argv) > 1 else "ra2-1.006"
    image, sections = load_executable(version)
    text = next(s for s in sections if s["name"] == ".text")
    text_end = text["va"] + text["vsize"]

    function_rows = read_csv(version, "functions.csv")
    functions = [(int(r["address"], 16), int(r["size"]) if r["size"] else None) for r in function_rows]
    units = read_csv(version, "modules.csv")
    data_units = read_csv(version, "data.csv")
    relocs = read_csv(version, "relocations.csv")
    names = read_csv(version, "names.csv")
    problems = check_functions(text, functions)
    problems += [f"functions.csv: 0x{r['address']} has no evidence" for r in function_rows if not r["evidence"]]
    if not problems:
        source_ranges = [(int(u["start"], 16), int(u["end"], 16)) for u in units if source_file(u["unit"])]
        compiled = frozenset(start for start, _ in functions
                             if any(lo <= start < hi for lo, hi in source_ranges))
        pieces = pieces_of(image, text, functions, compiled)
        problems = check_rest(image, sections, text, pieces, functions, units, data_units, relocs, names)
    if problems:
        print("\n".join(problems[:50]))
        sys.exit(f"{len(problems)} problem(s) in config/{version}/; nothing generated")

    names_by_address = {int(n["address"], 16): n["symbol"] for n in names if not ALIAS.match(n["evidence"])}
    function_name = lambda va: names_by_address.get(va, f"fn_{va:08X}")
    data_name = lambda va: names_by_address.get(va, f"dat_{va:08X}")

    objects = []
    for u in units:
        if u["unit"] not in objects:
            objects.append(u["unit"])
    for d in data_units:
        if d["unit"] not in objects:
            objects.append(d["unit"])

    # Sections: (object, start, end, section name, flags, symbol name or None).
    unit_ends = [int(u["end"], 16) for u in units]
    sections_out = []
    for start, end, is_function in pieces:
        unit = units[bisect.bisect_right(unit_ends, start)]["unit"]
        sections_out.append((unit, start, end, ".text", CODE_FLAGS,
                             function_name(start) if is_function
                             else None if is_function is None else f"chunk_{start:08X}"))
    for d in data_units:
        start, end = int(d["start"], 16), int(d["end"], 16)
        s = section_of(sections, start)
        sections_out.append((d["unit"], start, end, s["name"], s["flags"], None))
    sections_out.sort(key=lambda x: x[1])
    section_starts = [x[1] for x in sections_out]

    def containing(va):
        i = bisect.bisect_right(section_starts, va) - 1
        return sections_out[i] if i >= 0 and va < sections_out[i][2] else None

    # Symbols defined in data: data unit starts, named addresses, relocation targets.
    data_symbols = {}  # address -> name
    for unit, start, end, section_name, _, symbol in sections_out:
        if symbol is None and section_name != ".text":
            data_symbols[start] = data_name(start)
    for address in names_by_address:
        if not text["va"] <= address < text_end:
            data_symbols[address] = data_name(address)
    resolved = []  # (field, is_relative, symbol name, addend)
    for r in relocs:
        field, target, addend = int(r["field"], 16), int(r["target"], 16), int(r["addend"])
        if text["va"] <= target < text_end:
            piece = containing(target)
            symbol, addend = piece[5], target - piece[1] + addend
        else:
            data_symbols.setdefault(target, data_name(target))
            symbol = data_symbols[target]
        resolved.append((field, r["type"] == "REL32", symbol, addend))

    out_dir = os.path.join(ROOT, "build", version, "target")
    shutil.rmtree(out_dir, ignore_errors=True)  # no stale objects of renamed units
    os.makedirs(out_dir)
    relocs_by_section = {}
    for field, is_relative, symbol, addend in resolved:
        relocs_by_section.setdefault(containing(field)[1], []).append((field, is_relative, symbol, addend))
    for obj in objects:
        own = [x for x in sections_out if x[0] == obj]
        path = os.path.join(out_dir, obj + ".obj")
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as f:
            f.write(coff_object(image, sections, own, data_symbols, relocs_by_section))

    categories = {u["unit"]: u["category"] for u in units}
    objdiff_units = []
    for n in objects:
        unit = {"name": n, "target_path": f"build/{version}/target/{n}.obj",
                "metadata": {"progress_categories": [categories.get(n, "data")]}}
        source = source_file(n)
        if source:
            unit["base_path"] = f"build/{version}/src/{n}.obj"
            unit["metadata"]["source_path"] = source
        if categories.get(n) == "library":
            # Matched against VC6's own library objects; no source to write.
            unit["metadata"]["complete"] = True
        objdiff_units.append(unit)
    objdiff = {
        "$schema": "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json",
        "min_version": "3.0.0",
        "custom_make": "python3",
        "custom_args": ["compile.py"],
        "build_target": False,
        "build_base": True,
        "watch_patterns": ["src/**/*.cpp", "src/**/*.c", "src/**/*.h", "include/**/*.h"],
        "units": objdiff_units,
        "progress_categories": [
            {"id": "tibsun", "name": "Tiberian Sun derived"},
            {"id": "source", "name": "Other source"},
            {"id": "library", "name": "Static libraries"},
            {"id": "unknown", "name": "Unassigned code"},
            {"id": "data", "name": "Data"},
        ],
    }
    with open(os.path.join(ROOT, "objdiff.json"), "w") as f:
        json.dump(objdiff, f, indent=2)
        f.write("\n")
    chunks = sum(1 for _, _, is_function in pieces if is_function is False)
    print(f"config OK. {len(objects)} objects ({sum('base_path' in u for u in objdiff_units)} with source), "
          f"{len(functions)} known functions, {chunks} chunks, {len(names_by_address)} named, "
          f"{len(relocs)} relocations -> {os.path.relpath(out_dir, ROOT)}, objdiff.json")


if __name__ == "__main__":
    main()
