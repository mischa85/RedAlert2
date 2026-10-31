# Contributing

The goal is source that compiles to the original machine code because it *is*
equivalent code, not because a tool was coaxed into emitting the right bytes.
These rules exist to keep every match honest.

## Every claim says how it is known

- A function counts as matched only when the build's objdiff report says so.
  Percentages are never typed in by hand.
- Function bounds, names, module bounds, data bounds and relocations record
  how they were established, in their `evidence` column: what the executable
  states, what a byte-identical match shows (including what the matching
  object says about its relocations and symbols; see `docs/FUNCTIONS.md` for
  the rule and its assumptions), or what they were inferred from.

## No faked matches

- **No byte arrays standing in for data.** Tables, strings and structures are
  written as what they are. Genuine byte tables carry
  `// blob-allowed: <reason>`.
- **`volatile` is flagged for review.** It is a common way to force a match
  without being plausible original code.

These apply to code written for this project. A file taken unmodified from the
Tiberian Sun reconstruction keeps that source as it is, its tables included: it
is the reconstruction's code, not a workaround found here.

## Changes to reconstruction files

A file from the Tiberian Sun reconstruction is first added unmodified. A change
RA2 needs comes in a commit of its own, and every changed line carries a
comment starting `// RA2:` that says what the executable shows. Where the
executable fixes a declaration's place and shape (its position, size, argument
bytes and return type) but not its name, the declaration gets a placeholder:
`Unknown_<offset>` for a data member, `Unknown_Virtual_<offset>` for a virtual
function, the offset in hexadecimal, with parameters only as wide as the
executable shows. A placeholder is never listed in `names.csv`.

## Never commit game data

No executables, disassembly, extracted data or decompiler output dumps. Names,
addresses, sizes, hashes and percentages only.
