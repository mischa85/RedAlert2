# RedAlert2

[![Code](https://bytematch.dev/mischa85/RedAlert2.svg?label=Code&measure=matched_code_percent)](https://bytematch.dev/mischa85/RedAlert2)

A matching decompilation of *Command & Conquer: Red Alert 2*, version 1.006:
C++ source that compiles with the original compiler and settings to the same
machine code as the shipped game, verified function by function with
[objdiff](https://github.com/encounter/objdiff).

This is not an original Westwood Studios or Electronic Arts source release, and
the project is not affiliated with or endorsed by Electronic Arts. Matching code
compiles to the same bytes as the game; it is not a claim that the source is
the original.

## State

The build produces the original `game.exe`, byte-identical (checked by
SHA-256), with every byte still coming from your own copy of the game.
Progress from source is measured by objdiff's report (see below).

What is known is in `config/`, each row with its evidence: what the
executable itself states, what a byte-identical match with a compiled object
shows (under the assumptions listed in [docs/FUNCTIONS.md](docs/FUNCTIONS.md)),
or what it was inferred from. How each part was established is in `docs/`:

- **The executable:** its sections, entry point, imports, the linker's record of
  the objects it was built from, and its debug record
  ([docs/EXECUTABLE.md](docs/EXECUTABLE.md)).
- **Functions:** known starts and sizes; code not yet divided into functions is
  `chunk_<address>` ([docs/FUNCTIONS.md](docs/FUNCTIONS.md)).
- **Units:** how the code is divided into target objects
  ([docs/MODULES.md](docs/MODULES.md)).
- **Names:** what is named and why; anything unnamed is `fn_<address>` or
  `dat_<address>` ([docs/NAMES.md](docs/NAMES.md)).
- **Relocations:** known address fields; the executable's own relocation table
  was stripped ([docs/RELOCATIONS.md](docs/RELOCATIONS.md)).
- **Toolchain and build:** Visual C++ 6, as the executable records, with the
  flags measured to reproduce the most code
  ([docs/TOOLCHAIN.md](docs/TOOLCHAIN.md)); source compiled by `compile.py`,
  compared live in objdiff, and rebuilt into `game.exe` by `build.py` once it
  matches.

## What comes next

Each later change adds a discovery, one kind at a time: code shown by
byte-identical matches with compiled objects, and then source. Each step
records how it was established and keeps the build byte-identical.

## Rules

See [CONTRIBUTING.md](CONTRIBUTING.md): every match verified by objdiff,
evidence for every row in `config/`, no inline-assembly or byte-blob fakes, and
no game data in the repository.

## Layout

| Path | Contents |
|---|---|
| `build.py` | Builds `game.exe` and verifies it is byte-identical to the original |
| `hooks/pre-commit` | Runs `configure.py` and `build.py` before every commit, with your copy of the game present |
| `hooks/pre-push` | Uploads a progress report to ByteMatch whenever `main` is pushed, with an upload token set |
| `compile.py` | Compiles `src/` with VC6 under Wine; objdiff runs it on save |
| `compile_flags.txt`, `.clangd` | Include paths, VC6 emulation and diagnostics for editors that use clangd (Zed, VS Code) |
| `configure.py` | Checks `config/` for consistency, then writes the target objects and `objdiff.json` from it |
| `references.py` | Checks the names inferred from the references of matched code, given the Tiberian Sun reconstruction's `code/` |
| `src/`, `include/` | Source, one file per unit (see [src/README.md](src/README.md)), and shared headers |
| `config/ra2-1.006/` | Expected SHA-256 of the original, and the project's knowledge of it, each row with its evidence: function starts and sizes, code units, data units, relocations, and symbol names by address |
| `docs/EXECUTABLE.md` | What the executable states about itself |
| `docs/FUNCTIONS.md` | Known functions and how they are known |
| `docs/MODULES.md` | How the code is divided into units |
| `docs/NAMES.md` | Where the names in `names.csv` come from |
| `docs/RELOCATIONS.md` | Known address fields and how they are known |
| `docs/TOOLCHAIN.md` | Compiler, flags, and how they were established |
| `orig/` | Your own copy of the game executable (never committed) |

## Building

```sh
python3 build.py
```

Checks that `orig/ra2-1.006/game.exe` is the expected executable, assembles
`build/ra2-1.006/game.exe` from it and every function rebuilt from source, and
fails unless the result is byte-identical to the original. A compiled function
is rebuilt from source only when `names.csv` gives its address, all its
relocations resolve, and it reproduces the original bytes; everything else is
listed as pending, so work in progress never breaks the build.

## Workflow

1. Pick a unit from `config/ra2-1.006/modules.csv` and create `src/<unit>.cpp`.
2. Write a function. Give it its address and size in
   `config/ra2-1.006/functions.csv` if they are not known yet, and its address
   and the compiler's symbol in `config/ra2-1.006/names.csv`; the same for data
   it refers to. Rerun `configure.py` so the target objects use them. It
   refuses inconsistent config (overlapping functions, a name inside a
   function, a relocation crossing a function start, a row without evidence)
   and says what is wrong.
3. Open the project in [objdiff](https://github.com/encounter/objdiff). It runs
   `compile.py` whenever a source file changes and shows the difference live.
4. When objdiff shows 100%, `python3 build.py` rebuilds the function into
   `game.exe` and confirms the executable is still byte-identical.
5. Commit the source together with its rows, each with its evidence (see
   [CONTRIBUTING.md](CONTRIBUTING.md)). To run `configure.py` and `build.py`
   on every commit, enable the hook once per clone:
   `git config core.hooksPath hooks`.

Set `MSVC6_DIR` to the VC6 toolchain (default `toolchain/msvc6`) and, if
needed, `WINE_BIN` to Wine's binaries; see [docs/TOOLCHAIN.md](docs/TOOLCHAIN.md).
`toolchain/msvc6` can be a symlink to your VC6; editors read VC6's headers from
there through `compile_flags.txt`.

## Producing the target objects and a progress report

`configure.py` writes the target objects to `build/ra2-1.006/target/`, cut
from your copy of the game. A progress report needs
[objdiff-cli](https://github.com/encounter/objdiff/releases):

```sh
python3 configure.py
objdiff-cli report generate -o build/ra2-1.006/report.json
```

With the hooks enabled and an upload token from the project's manage page set
with `git config bytematch.token <token>`, `hooks/pre-push` generates the
report and uploads it to [ByteMatch](https://bytematch.dev/mischa85/RedAlert2)
whenever `main` is pushed.

## Acknowledgments

The source builds on the Tiberian Sun reconstruction by CCHyper, tomsons26,
ZivDero and the [OpenTS](https://github.com/OpenTS-Developers/OpenTS)
contributors.

Changes RA2 made to that source come in part from the
[ra2-decomp](https://github.com/gibbo101/ra2-decomp) contributors' matching
decompilation of Yuri's Revenge.

## License

GPL-3.0-or-later. The source will build on the Tiberian Sun reconstruction,
which is licensed the same way.
