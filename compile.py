"""Compile src/ with Visual C++ 6 under Wine.

Usage: python3 compile.py [OBJECT ...]

OBJECT is a path like build/ra2-1.006/src/crc.obj, compiled from src/crc.cpp
(or .c); this is how objdiff calls it. Without arguments, compiles every source
file for ra2-1.006.

Environment:
  MSVC6_DIR   VC6 in the msvc-wine layout (<dir>/wine/x86/cl);
              default toolchain/msvc6
  WINE_BIN    directory with the wine binaries, if not on PATH
"""
import glob
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.abspath(__file__))
DEFAULT_VERSION = "ra2-1.006"
FLAGS = ["/nologo", "/c", "/W3", "/O2", "/Ob2", "/Gr", "/GR", "/DNDEBUG", "/DWIN32"]


def environment():
    env = dict(os.environ)
    env["WINEDEBUG"] = "-all"
    env["WINEDLLOVERRIDES"] = "mscoree,mshtml="
    if env.get("WINE_BIN"):
        env["PATH"] = env["WINE_BIN"] + os.pathsep + env.get("PATH", "")
    return env


def source_for(obj_path):
    unit = os.path.splitext(os.path.basename(obj_path))[0]
    for extension in (".cpp", ".c"):
        path = os.path.join(ROOT, "src", unit + extension)
        if os.path.exists(path):
            return path
    sys.exit(f"no source file for {obj_path} (expected src/{unit}.cpp or .c)")


def compile_object(obj_path, cl, env):
    obj_path = os.path.join(ROOT, obj_path)
    source = source_for(obj_path)
    os.makedirs(os.path.dirname(obj_path), exist_ok=True)
    includes = [f"/I{os.path.join(ROOT, d)}" for d in ("src", "include")]
    # Output goes to a file, not a pipe: Wine's background processes inherit
    # a pipe and keep it open until wineserver exits.
    with tempfile.TemporaryFile(mode="w+") as log:
        result = subprocess.run([cl, *FLAGS, *includes, f"/Fo{obj_path}", source],
                                stdout=log, stderr=subprocess.STDOUT, env=env)
        log.seek(0)
        output = log.read()
    lines = [l for l in output.splitlines() if l.strip() and l.strip() != os.path.basename(source)]
    if lines:
        print("\n".join(lines))
    if result.returncode != 0:
        sys.exit(f"compiling {os.path.relpath(source, ROOT)} failed")
    print(f"{os.path.relpath(source, ROOT)} -> {os.path.relpath(obj_path, ROOT)}")


def main():
    msvc = os.environ.get("MSVC6_DIR", os.path.join(ROOT, "toolchain", "msvc6"))
    cl = os.path.join(msvc, "wine", "x86", "cl")
    if not os.path.exists(cl):
        sys.exit(f"VC6 not found at {cl}; set MSVC6_DIR (see docs/TOOLCHAIN.md)")
    objects = sys.argv[1:] or [
        os.path.join("build", DEFAULT_VERSION, "src", os.path.splitext(os.path.basename(p))[0] + ".obj")
        for p in sorted(glob.glob(os.path.join(ROOT, "src", "*.c*")))]
    env = environment()
    for obj in objects:
        compile_object(obj, cl, env)


if __name__ == "__main__":
    main()
