# Relocations (1.006)

`config/ra2-1.006/relocations.csv` lists the known address fields in the code
and data: where a 4-byte field holds an address, whether it is absolute
(`DIR32`) or a call / jump displacement (`REL32`), the address of the symbol it
refers to, the addend (for example the offset of an array element from the
array's start), and how it is known. `configure.py` checks that every field
holds target + addend. They make each target object's address fields
symbolic, which objdiff needs to compare code built from source.

The executable's own relocation table was stripped (see
[EXECUTABLE.md](EXECUTABLE.md)), so the file states none. A relocation is known
when an object matching the code at that place has it (see the match rule in
[FUNCTIONS.md](FUNCTIONS.md)): the object gives the field and its type, and the
executable the address it holds.

| Evidence | Meaning |
|---|---|
| `VC6 <library> <object>` | A relocation of a matched library section |
| `Tiberian Sun reconstruction, matched` | A relocation of a matched Tiberian Sun section |
| `compiled from src/<file>, matched` | A relocation of code compiled from `src/` that matches |

For library code, the object gives each field and its type. Where it refers to
a symbol plus an offset, that is kept only where the symbol's address is itself
confirmed: the start of a matched section, data whose own section matches, or
an import slot the import table names. Otherwise the target is the address
the field holds.

For the Tiberian Sun functions, the object gives each field and its type. The
match masks the field, so it does not show that RA2 refers to the same symbol
at the same offset: a structure may have changed. The target is the address
the field holds, without addend.

Relocations that resolve to an absolute value outside the executable (an
offset into the FS segment, where Windows keeps the thread's exception-handler
list) are not addresses and are left out.

Until a function's relocations are known, objdiff compares its address fields
as plain bytes.
