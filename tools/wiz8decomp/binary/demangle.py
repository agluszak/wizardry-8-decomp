"""Demangle MSVC decorated names with LLVM's `llvm-undname`.

Writing a demangler here would be a liability: the MSVC grammar is large, and a
partial implementation quietly produces plausible-looking wrong signatures for
exactly the templated and nested names that matter most. `llvm-undname` is a
maintained implementation of the same grammar, so it is used as the authority
and required rather than approximated.
"""

from __future__ import annotations

import shutil
import subprocess
from functools import lru_cache

_TOOL = "llvm-undname"


class DemanglerMissing(RuntimeError):
    """`llvm-undname` is not on PATH, so decorated names cannot be decoded."""


def tool_path() -> str:
    path = shutil.which(_TOOL)
    if not path:
        raise DemanglerMissing(
            f"{_TOOL} is not on PATH; install LLVM (it ships with the LLVM tools) so decorated "
            "names are decoded by a maintained demangler rather than approximated"
        )
    return path


@lru_cache(maxsize=1)
def tool_version() -> str:
    result = subprocess.run(
        [tool_path(), "--version"], capture_output=True, text=True, check=False
    )
    for line in result.stdout.splitlines():
        if "version" in line.casefold():
            return line.strip()
    return "unknown"


def demangle(names: list[str]) -> dict[str, str]:
    """Map each decorated name to its demangled signature.

    Names that are not decorated, or that the demangler rejects, map to an empty
    string. A non-zero exit only means at least one name failed, so it is not
    treated as an error.
    """
    # Only decorated names are worth sending; an undecorated export would just
    # come back as a failure group and add nothing.
    unique = sorted({name for name in names if name.startswith("?")})
    if not unique:
        return {}
    result = subprocess.run(
        [tool_path()],
        input="\n".join(unique) + "\n",
        capture_output=True,
        text=True,
        check=False,
    )
    # Output echoes each input, then emits the demangled signature when there is
    # one, then a blank separator - so a group is three lines on success and two
    # on failure. A fixed stride desynchronises on the first rejected name.
    lines = result.stdout.split("\n")
    signatures: dict[str, str] = {}
    index = 0
    for name in unique:
        if index >= len(lines):
            break
        if lines[index] != name:
            # Refuse to guess an alignment that would attach a signature to the
            # wrong symbol.
            raise RuntimeError(f"{_TOOL} output did not echo {name!r} where expected")
        signature = lines[index + 1] if index + 1 < len(lines) else ""
        signatures[name] = signature.strip()
        index += 2 if signature.strip() == "" else 3
    return signatures
