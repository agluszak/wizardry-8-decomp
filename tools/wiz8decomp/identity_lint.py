"""Reject multiple C++ identities for one original address.

The canonical model is one address, one function identity per binary: one
name, one normalized prototype and one calling convention. A duplicate appears
when an address-qualified declaration and a FUNCTION marker (or two
declarations) name the same address differently.

Each reccmp target links its own image, so claims are grouped by
(target, address): `0x10001000` in srEXT_JPEGImporter.dll and the same RVA in
srEXT_Unzip.dll are unrelated functions, not a collision.

Names are compared by their last ``::`` component so a class-qualified method
matches its marker. Prototypes are compared by the Clang semantic id (the
VC6-mangled name), which folds calling convention, return type and parameter
types together; the declarations carry it in the source index. Overloads of a
method are exempt from the cross-declaration comparison because their qualified
name does not include the parameter list; only free functions are compared by
qualified name. A declaration explicitly marked ``identity-alias:`` is a
documented fold onto another address and is exempt from that comparison.

The scan reads the source index for markers and declarations, then re-reads the
files only to resolve the address comment adjacent to each declaration and to
spot identity-alias markers.
"""

from __future__ import annotations

import json
import re
from collections import defaultdict
from pathlib import Path
from typing import Any

_ADDRESS = re.compile(r"/\*\s*(0x[0-9a-fA-F]{6,8})\s*\*/")
_BARE_ADDRESS = re.compile(r"^\s*/\*\s*0x[0-9a-fA-F]{6,8}\s*\*/\s*$")
_IDENTITY_ALIAS = re.compile(r"identity-alias\s*:")


def _last_component(name: str) -> str:
    return name.split("::")[-1].strip()


def _prototype(declaration: dict[str, Any]) -> str:
    """The ABI-normalized prototype key of one declaration."""

    semantic_id = declaration.get("semantic_id") or ""
    if semantic_id:
        return str(semantic_id)
    parameters = ",".join(declaration.get("parameter_types") or [])
    return "|".join(
        (
            declaration.get("calling_convention") or "",
            declaration.get("return_type") or "",
            parameters,
            "this" if declaration.get("has_this") else "",
        )
    )


def _linkage_prefix(semantic_id: str) -> str:
    """Separate C and C++ linkage so a C library name does not match C++."""

    return semantic_id[:1]


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


def _declaration_lines(repo_dir: Path, entry: dict[str, Any]) -> list[str] | None:
    path = repo_dir / entry["source_file"]
    if not path.is_file():
        return None
    return path.read_text(encoding="utf-8", errors="ignore").splitlines()


class IdentityGateError(RuntimeError):
    """One address carries more than one function identity."""


def validate_identity(repo_dir: Path) -> dict[str, Any]:
    violations = identity_violations(repo_dir)
    if violations:
        rendered = [item["detail"] for item in violations]
        raise IdentityGateError(
            "one address carries multiple function identities:\n  " + "\n  ".join(rendered)
        )
    return {
        "ok": True,
        "gate": "address-identity",
    }


def identity_violations(repo_dir: Path) -> list[dict[str, Any]]:
    from .source_index import project_targets

    index = json.loads((repo_dir / "build/source-index.json").read_text(encoding="utf-8"))
    targets = project_targets(repo_dir)

    def namespace(source_file: str, target: str | None = None) -> str:
        """The link namespace owning a claim. Markers carry their target;
        declarations resolve it through their source root."""
        if target:
            return target
        for name, config in targets.items():
            roots = config.get("source-root", ())
            roots = (roots,) if isinstance(roots, str) else tuple(roots)
            if any(
                source_file == root or source_file.startswith(root.rstrip("/") + "/")
                for root in roots
            ):
                return name
        return ""

    claims: dict[tuple[str, str], list[dict[str, Any]]] = defaultdict(list)
    for marker in index["markers"]:
        if marker["marker_kind"] != "FUNCTION":
            continue
        declaration = marker.get("declaration") or {}
        name = marker.get("marker_name") or declaration.get("qualified_name") or ""
        if not name:
            continue
        claims[
            (namespace(marker["source_file"], marker.get("target")), f"{marker['address']:08x}")
        ].append(
            {
                "name": _last_component(name),
                "qualified_name": declaration.get("qualified_name") or "",
                "prototype": _prototype(declaration),
                "semantic_id": declaration.get("semantic_id") or "",
                "kind": "marker",
                "source": marker["source_file"],
            }
        )

    address_declaration_keys: set[tuple[str, int, int]] = set()
    for entry in index["declarations"]:
        lines = _declaration_lines(repo_dir, entry)
        if lines is None:
            continue
        address = _declaration_address(lines, entry["line"], entry["end_line"])
        if address is None:
            continue
        claims[(namespace(entry["source_file"]), address)].append(
            {
                "name": _last_component(entry["qualified_name"]),
                "qualified_name": entry["qualified_name"],
                "prototype": _prototype(entry),
                "semantic_id": entry.get("semantic_id") or "",
                "kind": "declaration",
                "source": entry["source_file"],
            }
        )
        address_declaration_keys.add((entry["source_file"], entry["line"], entry["end_line"]))

    violations: list[dict[str, Any]] = []
    for (ns, address), entries in sorted(claims.items()):
        names = {entry["name"] for entry in entries}
        prototypes = {entry["prototype"] for entry in entries if entry["prototype"]}
        if len(names) == 1 and len(prototypes) <= 1:
            continue
        details = sorted(
            f"{entry['kind']}:{entry['name']} [{entry['prototype']}] ({entry['source']})"
            for entry in entries
        )
        if len(names) > 1:
            reason = "multiple names"
        else:
            reason = "multiple prototypes"
        label = f"{ns}:0x{address}" if ns else f"0x{address}"
        violations.append(
            {
                "address": f"0x{address}",
                "kind": "address-identity",
                "reason": reason,
                "names": sorted(names),
                "detail": f"{label}: {reason}: " + ", ".join(details),
            }
        )

    violations.extend(_consumer_violations(repo_dir, index, claims, address_declaration_keys))
    return violations


def _consumer_violations(
    repo_dir: Path,
    index: dict[str, Any],
    claims: dict[tuple[str, str], list[dict[str, Any]]],
    address_declaration_keys: set[tuple[str, int, int]],
) -> list[dict[str, Any]]:
    """Callers must redeclare the canonical free function with its prototype."""

    canonical: dict[tuple[str, str], set[tuple[str, str]]] = defaultdict(set)
    for (_, address), entries in claims.items():
        for entry in entries:
            if not entry["qualified_name"] or not entry["semantic_id"]:
                continue
            key = (
                entry["qualified_name"],
                _linkage_prefix(entry["semantic_id"]),
            )
            canonical[key].add((address, entry["prototype"]))
    unique = {key: next(iter(value)) for key, value in canonical.items() if len(value) == 1}

    violations: list[dict[str, Any]] = []
    for entry in index["declarations"]:
        if entry.get("semantic_kind") != "free_function":
            continue
        key = (
            entry["qualified_name"],
            _linkage_prefix(entry.get("semantic_id") or ""),
        )
        if key not in unique:
            continue
        if (entry["source_file"], entry["line"], entry["end_line"]) in address_declaration_keys:
            continue
        canonical_address, canonical_prototype = unique[key]
        prototype = _prototype(entry)
        if prototype == canonical_prototype:
            continue
        lines = _declaration_lines(repo_dir, entry)
        if lines is None:
            continue
        window = lines[max(0, entry["line"] - 6) : entry["end_line"]]
        if any(_IDENTITY_ALIAS.search(line) for line in window):
            continue
        violations.append(
            {
                "address": f"0x{canonical_address}",
                "kind": "consumer-prototype",
                "reason": "redeclared prototype",
                "names": [entry["qualified_name"]],
                "detail": (
                    f"0x{canonical_address}: {entry['qualified_name']} redeclared "
                    f"as [{prototype}] in {entry['source_file']}:{entry['line']}, "
                    f"canonical [{canonical_prototype}]"
                ),
            }
        )
    return violations
