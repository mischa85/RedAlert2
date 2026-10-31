# Modules (1.006)

How many objects the game was linked from is a fact (see the Rich header in
[EXECUTABLE.md](EXECUTABLE.md)): **517 C++ objects, 190 C objects and 48
assembly objects**, among others, including objects the linker took from
libraries. Which code belongs to which object is not stated anywhere in the
executable.

## Modules in this repository

`config/ra2-1.006/modules.csv` divides the code section into units: address
ranges that are adjacent and cover it without gaps or overlaps.
`config/ra2-1.006/data.csv` does the same for the initialized `.rdata` and
`.data`. Each unit becomes one target object.

A named unit is a module: one of the objects the game was linked from, with
the evidence for it in the `evidence` column. A module may have several
ranges, in code and in data, where what lies between them is not known to be
its own. A module compiled from `src/` is named after its source file,
`src/<unit>.cpp`.

An `unk_<address>` unit is code or data not assigned to a module; it may come
from any number of objects.

| Category | Meaning |
|---|---|
| `library` | `<library>/<object>`, e.g. `libcmt/strlen`: a module from a VC6 static library, by its matched sections (see [FUNCTIONS.md](FUNCTIONS.md)) |
| `library` | `unk_<address>`: a matched section that is the identical code of several objects, which are listed in its evidence |
| `source` | `<unit>`: a module compiled from `src/<unit>.cpp`, a file not in the Tiberian Sun reconstruction |
| `tibsun` | `<unit>`: a module compiled from `src/<unit>.cpp`, a file of the Tiberian Sun reconstruction; `unk_<address>`: matched functions of one Tiberian Sun source file (see [FUNCTIONS.md](FUNCTIONS.md)) |
| `unknown` | `unk_<address>`: code not assigned to a module |

A `tibsun` `unk_<address>` unit is a range of consecutive matched functions
whose objects mark them as their own (COMDAT selection "no duplicates") and
that come from one source file. It states which functions match one Tiberian
Sun file, not that they were one object in RA2. Inline or template copies
("select any"), which any object that uses them may hold, and identical code
of several objects stay in `unknown` units.

## Not known

- Which code belongs to which of the objects, outside the modules.
