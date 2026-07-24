#!/usr/bin/env python3
"""Run one VC6 command in the image's isolated 32-bit Wine prefix."""

from __future__ import annotations

import os
import subprocess
import sys


MSVC_ROOT = r"C:\msvc\VC98"


def configure_environment() -> dict[str, str]:
    env = os.environ.copy()
    env.update(
        {
            "INCLUDE": rf"{MSVC_ROOT}\Include;{MSVC_ROOT}\MFC\Include;{MSVC_ROOT}\ATL\Include",
            "LIB": rf"{MSVC_ROOT}\Lib;{MSVC_ROOT}\MFC\Lib",
            "TMP": r"Z:\out\tmp",
            "TEMP": r"Z:\out\tmp",
            "WINEPATH": rf"C:\msvc\Common\MSDev98\Bin;{MSVC_ROOT}\Bin",
        }
    )
    return env


def main() -> int:
    if not sys.argv[1:]:
        print("usage: image COMMAND [ARG ...]", file=sys.stderr)
        return 2
    os.makedirs("/out/tmp", exist_ok=True)
    command = ["/usr/bin/wine", rf"{MSVC_ROOT}\Bin\{sys.argv[1]}", *sys.argv[2:]]
    print("container command:", subprocess.list2cmdline(command), flush=True)
    return subprocess.run(command, env=configure_environment(), check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
