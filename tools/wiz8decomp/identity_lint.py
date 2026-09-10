"""Reject multiple C++ identities for one original address.

The canonical model is one address, one function identity. A duplicate appears
when an address-qualified declaration and a FUNCTION marker (or two
declarations) name the same address differently. Names are compared by their
last ``::`` component so a class-qualified method matches its marker.

The scan reads the source index for markers and declaration spans, then
re-reads the files only to resolve the address comment adjacent to each
declaration: the declaration's own lines, or a bare comment on the line after.
"""

from __future__ import annotations

import json
import re
from collections import defaultdict
from pathlib import Path
from typing import Any

_ADDRESS = re.compile(r"/\*\s*(0x[0-9a-fA-F]{6,8})\s*\*/")
_BARE_ADDRESS = re.compile(r"^\s*/\*\s*0x[0-9a-fA-F]{6,8}\s*\*/\s*$")


def _last_component(name: str) -> str:
    return name.split("::")[-1].strip()


def _declaration_address(lines: list[str], start: int, end: int) -> str | None:
    own = []
    for position in range(start - 1, end):
        if 0 <= position < len(lines):
            own.extend(_ADDRESS.findall(lines[position]))
    if own:
        return own[-1].lower()[2:].rjust(8, "0")
    following = end
    if 0 <= following < len(lines) and _BARE_ADDRESS.match(lines[following]):
        match = _ADDRESS.search(lines[following])
        if match:
            return match.group(1).lower()[2:].rjust(8, "0")
    return None


class IdentityGateError(RuntimeError):
    """One address carries more than one function identity."""


def validate_identity(repo_dir: Path) -> dict[str, Any]:
    violations = identity_violations(repo_dir)
    if violations:
        rendered = [f"{item['address']}: " + ", ".join(item["names"]) for item in violations]
        raise IdentityGateError(
            "one address carries multiple function identities:\n  " + "\n  ".join(rendered)
        )
    return {
        "ok": True,
        "gate": "address-identity",
    }


def identity_violations(repo_dir: Path) -> list[dict[str, Any]]:
    index = json.loads((repo_dir / "build/source-index.json").read_text(encoding="utf-8"))

    claims: dict[str, set[tuple[str, str, str]]] = defaultdict(set)
    for marker in index["markers"]:
        if marker["marker_kind"] != "FUNCTION":
            continue
        name = marker.get("marker_name") or ""
        if not name:
            continue
        claims[f"{marker['address']:08x}"].add(
            (_last_component(name), "marker", marker["source_file"])
        )

    for entry in index["declarations"]:
        path = repo_dir / entry["source_file"]
        if not path.is_file():
            continue
        lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
        address = _declaration_address(lines, entry["line"], entry["end_line"])
        if address is None:
            continue
        claims[address].add(
            (_last_component(entry["qualified_name"]), "declaration", entry["source_file"])
        )

    violations = []
    for address, entries in sorted(claims.items()):
        names = {name for name, _, _ in entries}
        if len(names) > 1:
            violations.append(
                {
                    "address": f"0x{address}",
                    "names": sorted(names),
                    "entries": sorted(
                        f"{kind}:{name} ({source})" for name, kind, source in entries
                    ),
                }
            )
    return violations
