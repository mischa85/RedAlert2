# Functions (1.006)

`config/ra2-1.006/functions.csv` lists the known functions in the code section:
address, size if known, and how it is known: an address the executable states,
one a byte-identical match shows (see below), or one inferred, with what from.
The executable has no symbol table and no list of its functions, so the code is
divided only as far as functions are known.

## How the code is divided

`configure.py` cuts the code section into pieces, each a section of a target
object:

- a known function with a known size runs from its start to start + size, plus
  the padding bytes (`0x90` or `0xCC`) after it. In a module compiled from
  `src/`, it takes only `0x90`, which the compiler emits; `0xCC` after it is
  the linker's fill, which no compiled object holds, and is a piece without a
  symbol, outside objdiff's code total;
- a known function without a known size runs to the next known function;
- code that no known function covers is a chunk, `chunk_<address>`, until the
  next known function. A chunk is not a function: it holds whatever functions
  are there, and their boundaries are not known yet.

Matching a function inside a chunk establishes its start and size, which then
go into `functions.csv` and split the chunk.

objdiff counts every piece as a function, chunks included: the number of
functions in its report (and on ByteMatch) is the known functions plus the
chunks, and changes as chunks are split. Its code total covers the whole code
section, so the matched percentage is a share of all the game's code. A chunk
without a symbol would drop out of that total, which is why chunks keep one.

## Known functions

| Evidence | Meaning |
|---|---|
| `entry point (PE header)` | The executable's entry point, `0x00785AA0` |
| `code section of VC6 <library> <object>, matched` | A code section of an object in VC6's static libraries (`VC98/Lib`) that matches |
| `function of the Tiberian Sun reconstruction, matched` | A function of the [Tiberian Sun reconstruction](https://github.com/OpenTS-Developers/TibSun), compiled with the flags in [TOOLCHAIN.md](TOOLCHAIN.md), whose code section matches |
| `code of src/<file>, matched` | Compiled from `src/`, a section that meets the match rule |
| `code of src/<file>, matched: ...` | Compiled from `src/`, matched where the match rule cannot apply (see "Source") |

The entry point's size is not known, so it runs to the next known function.

## The match rule

A compiled object's code section is matched when:

- its bytes, with the fields its relocations cover masked, occur at exactly
  one address in the game's code;
- it is at least 32 bytes long;
- at least 60% of its bytes are not covered by relocations, so the match does
  not rest mostly on masked bytes.

A matched section gives, from the object: its start and size, listed as one
known function (compiled C and C++ with `/Gy` has one function per section;
hand-written code may have several entry points in one section, which are not
split up), its relocation fields (see [RELOCATIONS.md](RELOCATIONS.md)), and
names for what is matched itself (see [NAMES.md](NAMES.md)). A section that
is the identical code of several objects gives only what all of them agree on.

## What a match shows, and what it assumes

That the bytes are equal at exactly one address can be checked by anyone with
the executable and the object. What is recorded from a match rests on these
assumptions:

1. **Equal bytes are the same code, not a coincidence.** The rule above makes a
   coincidence unlikely; it does not make it impossible.
2. **A masked field is an address in the game too.** `configure.py` checks that
   it holds an address inside the executable; a constant could still happen to.
3. **The section's extent is the same in the game.** Start and size come from
   the object.
4. **For library code: the code is that object's own.** The name of the
   section's own symbol is taken over. Anything a match reaches only through a
   relocation (the address of data, an offset into it) is recorded only where
   its own bytes match too, or the executable states it (see
   [NAMES.md](NAMES.md) and [RELOCATIONS.md](RELOCATIONS.md)). For code from a
   reconstruction, no names or offsets are taken over.

If one of them turns out wrong for a row, the row is corrected.

## Source

Code compiled from `src/` is compared with objdiff. Where several functions
have identical code (relocated fields masked), the match rule cannot place
them, since the code occurs at more than one address. When it occurs at exactly
as many addresses as there are such functions, and the sections otherwise meet
the rule, each address is a known function of that size. The code does not
show which function is at which address; they are placed by order, as every
module compiled from `src/` is laid out:

- within one source file, functions are in the order the file defines them;
- across source files, the files are in alphabetical order.

The evidence of such a function and its name says that it is placed by order.

Code too short or too plain to be unique, such as an empty destructor, can
occur at more addresses than there are functions with that code. Such
functions are placed only where they close a gap: the functions before and
after them in their file's definition order are placed, those leave a gap of
exactly their sizes (with padding), and the gap holds their bytes in definition
order. A function that is first or last in its file is not placed this way,
since nothing bounds it on one side, except at the end of a file as below. The
evidence of such a function and its name says that it is placed by the gap.

Code the compiler copies into every object that uses it (an inline function, a
scalar deleting destructor) follows that object's own functions. Where such a
copy compiled from the same file matches at exactly one address after the
file's placed functions, it bounds the end of the file as a placed function
would: the functions after the last placed one are placed by the gap up to it.
The copy itself is not placed this way, since another object may hold the same
code.

A function whose code RA2 changed matches nowhere. It is placed only where
its address is known otherwise:

- where it closes a gap: the functions before and after it in its file's
  definition order are placed, and it takes the room between them;
- where it is first in its file, and `names.csv` already names its symbol (see
  [NAMES.md](NAMES.md), "References of matched code"): it starts there and runs
  to the next function of the file.

It gets no row here, since RA2's code is not known to fill that room. The
module's evidence names it as not matched.

## Not known

- How many functions there are, and where any function not listed starts or
  ends. Known functions without a size may extend less far than the next known
  start.
