"""Compare retail-address marker identities between two revisions.

Ordinary merging can silently turn a recovered FUNCTION back into a bare
declaration, duplicate an address under two names, or rename an identity
without moving its callers. The check reads the matching markers from
``src/`` and ``include/`` at a base and a head revision and reports, per
address: removed markers, changed identities, duplicated addresses and
removed FUNCTION addresses whose name is still referenced by the head
tree (newly unresolved call targets).

The comparison is textual and revision-based so it runs before a build and
without a checkout switch. A loss is acceptable only when named on the
command line with a reason; anything else fails.
"""

from __future__ import annotations

import re
import subprocess
from collections import defaultdict
from pathlib import Path
from typing import Any

MARKER_KINDS = ("FUNCTION", "GLOBAL", "VTABLE", "TEMPLATE", "SYNTHETIC", "LIBRARY", "STUB")
IDENTITY_KINDS = frozenset({"FUNCTION", "GLOBAL", "VTABLE"})
SOURCE_ROOTS = ("src", "include")
SOURCE_SUFFIXES = (".c", ".cpp", ".h", ".hpp")

_MARKER = re.compile(
    r"^\s*//\s*(?P<kind>FUNCTION|GLOBAL|VTABLE|TEMPLATE|SYNTHETIC|LIBRARY|STUB):\s*"
    r"(?P<target>[A-Za-z0-9_]+)\s+(?P<address>0x[0-9A-Fa-f]+)",
)
_DECLARATOR = re.compile(r"([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*(?:\(|=|;|\[)")

Identity = tuple[str, str, int]


def _git(repo_dir: Path, *args: str) -> str:
    return subprocess.run(
        ["git", *args], cwd=repo_dir, capture_output=True, text=True, check=True, errors="replace"
    ).stdout


def _tree_files(repo_dir: Path, revision: str) -> list[str]:
    listing = _git(repo_dir, "ls-tree", "-r", "-z", "--name-only", revision, *SOURCE_ROOTS)
    return [name for name in listing.split("\0") if name.endswith(SOURCE_SUFFIXES)]


def _owned_entity(lines: list[str], start: int, kind: str) -> str:
    for index in range(start, min(start + 6, len(lines))):
        stripped = lines[index].strip()
        if not stripped or stripped.startswith("#"):
            continue
        if stripped.startswith("//"):
            if kind in ("TEMPLATE", "SYNTHETIC"):
                return stripped
            continue
        return re.sub(r"\s+", " ", stripped)
    return ""


def _entity_name(entity: str) -> str:
    cleaned = re.sub(r"/\*.*?\*/", " ", entity)
    match = _DECLARATOR.search(cleaned)
    return match.group(1).split("::")[-1] if match else ""


def collect_identities(repo_dir: Path, revision: str) -> dict[Identity, list[dict[str, str]]]:
    """Markers at ``revision`` keyed by (kind, target, address)."""

    identities: dict[Identity, list[dict[str, str]]] = defaultdict(list)
    for name in _tree_files(repo_dir, revision):
        lines = _git(repo_dir, "show", f"{revision}:{name}").splitlines()
        for index, line in enumerate(lines):
            marker = _MARKER.match(line)
            if marker is None:
                continue
            kind = marker.group("kind")
            key = (kind, marker.group("target"), int(marker.group("address"), 16))
            entity = _owned_entity(lines, index + 1, kind)
            identities[key].append({"file": name, "entity": entity, "name": _entity_name(entity)})
    return identities


def _references(repo_dir: Path, revision: str, names: set[str]) -> dict[str, int]:
    if not names:
        return {}
    counts = dict.fromkeys(names, 0)
    pattern = re.compile(r"\b(" + "|".join(re.escape(name) for name in sorted(names)) + r")\b")
    for file_name in _tree_files(repo_dir, revision):
        for match in pattern.finditer(_git(repo_dir, "show", f"{revision}:{file_name}")):
            counts[match.group(1)] += 1
    return counts


def _format_key(key: Identity) -> str:
    kind, target, address = key
    return f"{kind} {target} 0x{address:08X}"


def merge_preservation_report(
    repo_dir: Path, base: str, head: str, allowed: dict[int, str] | None = None
) -> dict[str, Any]:
    """Compare marker identities between ``base`` and ``head``."""

    allowed = allowed or {}
    before = collect_identities(repo_dir, base)
    after = collect_identities(repo_dir, head)

    removed = [key for key in sorted(before) if key not in after]
    added = [key for key in sorted(after) if key not in before]
    changed = [
        key
        for key in sorted(before.keys() & after.keys())
        if key[0] in IDENTITY_KINDS
        and sorted(item["entity"] for item in before[key])
        != sorted(item["entity"] for item in after[key])
    ]
    duplicates = [key for key in sorted(after) if key[0] in IDENTITY_KINDS and len(after[key]) > 1]

    removed_function_names = {
        item["name"]
        for key in removed
        if key[0] == "FUNCTION"
        for item in before[key]
        if item["name"]
    }
    still_referenced = _references(repo_dir, head, removed_function_names)
    unresolved = sorted(name for name, count in still_referenced.items() if count)

    unexplained_losses = [
        key for key in removed if key[0] in IDENTITY_KINDS and key[2] not in allowed
    ]
    unexplained_duplicates = [key for key in duplicates if key[2] not in allowed]

    def describe(
        keys: list[Identity], source: dict[Identity, list[dict[str, str]]]
    ) -> list[dict[str, Any]]:
        return [{"identity": _format_key(key), "owners": source[key]} for key in keys]

    counts = {
        kind: {
            "base": sum(1 for key in before if key[0] == kind),
            "head": sum(1 for key in after if key[0] == kind),
        }
        for kind in MARKER_KINDS
    }
    return {
        "schema": "wiz8.merge-preservation-v1",
        "base": base,
        "head": head,
        "status": "passed" if not unexplained_losses and not unexplained_duplicates else "failed",
        "counts": counts,
        "removed": describe(removed, before),
        "added": describe(added, after),
        "changed": [
            {
                "identity": _format_key(key),
                "base": [item["entity"] for item in before[key]],
                "head": [item["entity"] for item in after[key]],
            }
            for key in changed
        ],
        "duplicates": describe(duplicates, after),
        "newly_unresolved": unresolved,
        "allowed": {f"0x{address:08X}": reason for address, reason in sorted(allowed.items())},
        "unexplained_losses": [_format_key(key) for key in unexplained_losses],
        "unexplained_duplicates": [_format_key(key) for key in unexplained_duplicates],
    }


def parse_allowed(values: list[str]) -> dict[int, str]:
    """``0xADDRESS=reason`` command-line entries."""

    allowed: dict[int, str] = {}
    for value in values:
        address, separator, reason = value.partition("=")
        if not separator or not reason.strip():
            raise ValueError(f"expected 0xADDRESS=reason, got {value!r}")
        allowed[int(address, 16)] = reason.strip()
    return allowed
