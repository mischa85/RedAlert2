# Toolchain

The compiler the executable records, and the flags this project compiles with.
The executable states the compiler builds; it does not state the flags.

## Compiler

From the Rich headers of the game executables, decoded with
[richprint](https://github.com/dishather/richprint)'s `comp_id.txt` database:

| Version | C++ objects | C objects | Compiler |
|---|---|---|---|
| 1.000 (built 2000-10-02) | 516 × cl 8168 | 190 × cl 8168 | Visual C++ 6.0, original release |
| 1.006 (built 2001-06-06) | 505 × cl 8447, 12 × cl 8168 | 190 × cl 8168 | a Visual C++ 6.0 build stamping 8447 (see below) |

Both executables also contain 15 linker import/export records with build 8447,
48 MASM 6.13 objects, Linker 5.12-era library objects and one resource object.
Base relocations are stripped in both.

### What "8447" is

The database labels C++ build 8447 "VS6 SP3". The Visual Studio 6.0 Service
Pack 3 disc does not bear that out for the compiler: it installs only the
front ends `c1.dll` and `c1xx.dll` (12.0.8472), `link.exe` (6.0.8447) and
`cvtres.exe`, and leaves the code generator `c2.dll` and the driver `cl.exe` at
the original release. Objects compiled with that SP3 setup are still stamped
build 8168. 8447 is SP3's linker version; which compiler stamps C++ objects 8447
is not established.

It matters little for code generation: compiling the Tiberian Sun
reconstruction with the SP3 front ends gives exactly the same matches in 1.000
and 1.006 as the original release (1,497 and 1,467 functions, without
`/DNDEBUG`). The functions that match 1.000 but not 1.006 therefore changed in
source, not compiler. This project uses the original-release code generator
with the SP3 front ends.

## Flags

**`/O2 /Ob2 /Gr /GR /DNDEBUG`**, everything else at its default: the flag set
under which the most code matches. That RA2 was compiled with them is not
stated anywhere; the measurement is:

The Tiberian Sun reconstruction was compiled with 20 flag combinations,
counting how many of its functions compile to bytes found exactly once in the
game (address bytes masked). The
winning set placed 1,497 functions in 1.000; every alternative placed fewer:
`/Ob1` (= `/O2` alone) 1,257, `/Ox` 1,248, `/Op` 1,180, no `/Gr` 957,
`/Oy-` 307, `/G6` 296, `/O1` 23. Additions to the winning set either changed
nothing (`/Gs`) or did worse: `/Zp4`, `/QIfdiv`, `/J`, `/GX`, `/Ow`, `/Zp1`, `/Oa`.

`/DNDEBUG`, which turns `assert` off, was measured on its own on top of that
set, with the match rule of [FUNCTIONS.md](FUNCTIONS.md): 90 more functions of
the reconstruction match 1.006 with it, and none fewer. None of the matched
code calls the reconstruction's `Assert_Failure`.

- `/O2` implies, per the VC6 documentation, `/Og /Oi /Ot /Oy /Ob1 /Gs /Gf /Gy`;
  `/Ob2` is set explicitly on top.
- `/GR` (RTTI) does not change code.
- With `/Gy`, each ordinary function is in its own "no duplicates" COMDAT
  section; inline and template copies use "select any".

## Open questions

- Which compiler build stamps C++ objects 8447, and whether it generates code
  differently from the original release in any case that matters.
- Whether `/GF` (read-only string pooling) was used on top of `/Gf`.

## Running VC6

Visual C++ 6.0 is proprietary and is not part of this repository. On macOS and
Linux it runs under Wine, for example packaged in the
[msvc-wine](https://github.com/mstorsjo/msvc-wine) layout.
