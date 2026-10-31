# Original game files

Place your own copy of the game executable here. Nothing in this directory
except this file is ever committed.

| Path | Version | SHA-256 |
|---|---|---|
| `orig/ra2-1.006/game.exe` | 1.006, built 2001-06-06 | `e6418111c2c36081a1795fb000e0927beea1630170b334c5013d12102602e70f` |

1.006 is produced by Westwood's official 1.006 patch (`RA21006EN.EXE`, an
RTPatch self-applying update) applied to a 1.000 installation. The patcher
reads `InstallPath` (path to `Ra2.exe`) and `Version` from
`HKLM\SOFTWARE\Westwood\Red Alert 2`. Under 64-bit Wine, set them in the
32-bit registry view (`reg add ... /reg:32`), and pass the game directory as
the first argument.

Check your copy with `shasum -a 256` (or `sha256sum`); `build.py` checks it too.
