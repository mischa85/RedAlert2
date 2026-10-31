# The executable (1.006)

What `game.exe` 1.006 states about itself, read from the file. Labels and
interpretations from outside the file are marked as such.

## Header

| | |
|---|---|
| Time stamp | 2001-06-06 23:25:33 UTC |
| Linker version | 6.0 |
| Image base | `0x00400000` |
| Entry point | `0x00785AA0` |
| Subsystem | Windows GUI, 4.0 |
| Characteristics | `0x030F`: relocations stripped, executable, line numbers and local symbols stripped, 32-bit, debug information stripped |

| Section | Address | Size in memory | Size in file |
|---|---|---|---|
| `.text` | `0x00401000` | `0x003985CD` | `0x00399000` |
| `.rdata` | `0x0079A000` | `0x0002EC36` | `0x0002F000` |
| `.data` | `0x007C9000` | `0x003639C4` | `0x00068000` |
| `.rsrc` | `0x00B2D000` | `0x00015FB8` | `0x00016000` |

`.data` is mostly uninitialized: only its first `0x68000` bytes are in the
file. There is no base relocation table and no symbol table.

## Imports

368 functions from 15 DLLs, each with its import address table slot and its
export name, or ordinal for 32 of them. The import address table is
`0x0079A000`–`0x0079A5FC` (383 slots: 368 imports and a terminator per DLL).

| DLL | Imports |
|---|---|
| KERNEL32.dll | 160 |
| USER32.dll | 96 |
| GDI32.dll | 22 |
| WSOCK32.dll | 19 |
| ole32.dll | 15 |
| binkw32.dll | 13 |
| OLEAUT32.dll | 11 |
| ADVAPI32.dll | 9 |
| COMCTL32.dll | 7 |
| WINMM.dll | 5 |
| IMM32.dll | 5 |
| VERSION.dll | 3 |
| SHELL32.dll | 1 |
| DDRAW.dll | 1 |
| DSOUND.dll | 1 |

## Debug directory

One entry, of type `MISC` (4), with the same time stamp as the header, 272
bytes at the end of the file (offset `0x447000`). It is an executable-name
record (data type 1) whose name field reads `tsun.dbg`, followed in the same
field by `SUN.exe`.

## Rich header

The linker records how many objects it was given per tool and tool version:

| Product ID | Build | Objects | richprint's label for this ID and build |
|---|---|---|---|
| `0x000B` | 8447 | 505 | C++ compiler, VS6 SP3 |
| `0x000B` | 8168 | 12 | C++ compiler, VS98 (6.0) |
| `0x000A` | 8168 | 190 | C compiler, VS98 (6.0) |
| `0x000E` | 7299 | 48 | MASM 6.13 |
| `0x0006` | 1735 | 1 | cvtres (resources) |
| `0x000C` | 7291 | 18 | alias objects (OLDNAMES.LIB) |
| `0x0004` | 8447 | 15 | linker import/export records |
| `0x0013` | 8022 | 13 | Linker 5.12 (libraries) |
| `0x0013` | 8034 | 27 | Linker 5.12 (libraries) |
| `0x0000` | 0 | 21 | unmarked objects |
| `0x0001` | 0 | 335 | unmarked objects |

Product ID, build and count are read from the file. The labels are
[richprint](https://github.com/dishather/richprint)'s community database, not
part of the executable; the totals below rest on them. The counts include
objects the linker took from libraries; the header does not say which objects
are the game's own.

So the game was linked from **517 C++ objects, 190 C objects and 48 assembly
objects**, among others.
