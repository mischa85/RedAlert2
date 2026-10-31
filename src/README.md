# Source

One file per unit: `src/<unit>.cpp` (or `.c`), named after the unit in
`config/ra2-1.006/modules.csv`. A unit without a source file counts as 0%.
Shared headers go in `include/`.

Everything here is compiled by `compile.py` with Visual C++ 6 and the
project's flags (`/O2 /Ob2 /Gr /GR /DNDEBUG`, see
[docs/TOOLCHAIN.md](../docs/TOOLCHAIN.md)).

A function in a source file is connected to the game by a row in
`config/ra2-1.006/names.csv`: its address, the symbol the compiler gives it
(the decorated name objdiff shows for the compiled object), and the evidence for
the name (see [docs/NAMES.md](../docs/NAMES.md) for what counts). Data the code
refers to is named the same way. Unnamed things are referred to by their address:
`fn_<address>` for functions, `dat_<address>` for data.
