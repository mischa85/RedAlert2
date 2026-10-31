# Names (1.006)

`config/ra2-1.006/names.csv` gives symbol names by address, each with its
evidence. Everything not named there is `fn_<address>`, `dat_<address>` or
`chunk_<address>` in the target objects.

Each name says how it is known: the named bytes are matched themselves, the
executable states what the address is, or it is inferred, with what from.

## Static libraries

A matched library section (see [FUNCTIONS.md](FUNCTIONS.md)) and what it refers
to are named from its object's symbol table:

- the section: the object's symbol at its start. Where the object has two
  symbols there, the first is used and the other given in the evidence;
- entry points inside the section: hand-written code may have several, each a
  symbol the object puts at an offset into the section. Each names its own
  address, with evidence starting `entry point of`; the section stays one
  function (see [FUNCTIONS.md](FUNCTIONS.md));
- data it refers to: where the object's data section that defines the symbol
  matches as well, by the same rule as code, and puts the symbol at that
  address;
- import address table slots it refers to (`__imp_` symbols): where the import
  table names the same function for that slot. The import table states export
  names, not the symbols the code used for them, so it does not name a slot on
  its own.

A symbol that a library defines as an alias of another (a weak external, as
`OLDNAMES.LIB` defines `_stricmp` for `__stricmp`) names the same address as a
second row, whose evidence starts with `alias of <symbol>` and says where the
alias is defined. The address keeps its first name in the target objects.

A section that is the identical code of several objects is named only where all
of them use the same name. Not named: uninitialized data, which has no bytes to
match; data sections too small for the rule; slots the import table gives only
by ordinal; and names that occur at several addresses (a static function each
object that uses it has its own copy of).

## Source

A function or data item compiled from `src/` that matches, by objdiff for code
and by the match rule for data, is named with the symbol it has in that source
(evidence `compiled from src/<file>, matched`). The name is the source's; the
match shows the code, not what RA2's own source called it.

## References of matched code

A symbol that matched code compiled from `src/` refers to through a relocation
is named at the address the field holds, less its addend. It is named only
where, across all matched code (code compiled from `src/` and the matched
functions of the Tiberian Sun reconstruction), the symbol is never at another
address and the address never has another symbol. Symbols the compiler makes
for each object on its own (`_$E…`, `__real@…`, static locals) are not named
this way, since each object may have its own copy. Such a name is inferred,
and its evidence gives the source file and the number of references.
`references.py` checks every such row against the reconstruction's `code/`.
