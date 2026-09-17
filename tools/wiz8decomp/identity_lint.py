"""Reject conflicting or placeholder C++ identities for recovered functions.

The canonical model is one address, one function identity per binary: one
name, one normalized prototype and one calling convention. A duplicate appears
when an address-qualified declaration and a FUNCTION marker (or two
declarations) name the same address differently.

A recovered function body is also an identity claim. Once a body exists it
must no longer use the address-derived ``Function123ABC`` placeholder form.
Declaration-only placeholders remain valid for unresolved callees.

Each reccmp target links its own image, so claims are grouped by
(target, address): `0x10001000` in srEXT_JPEGImporter.dll and the same RVA in
srEXT_Unzip.dll are unrelated functions, not a collision.

Names are compared by their last ``::`` component so a class-qualified method
matches its marker. Prototypes are compared by the Clang semantic id (the
VC6-mangled name), which folds calling convention, return type and parameter
types together;
the declarations carry it in the source index. Overloads of a method are exempt from the cross -
    declaration comparison because their qualified name does not include the parameter list; only free functions are compared by
qualified name. Function-template primaries and specializations that share a free-function name
with an address-owned ordinary overload are distinct overloads, not consumer redeclarations.
A declaration explicitly marked ``identity-alias:`` is a
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

_ADDRESS = re.compile(r"/\*\s*(0x[0-9a-fA-F]{6,8})\b")
_IDENTITY_ALIAS = re.compile(r"identity-alias\s*:")
_FUNCTION_MARKER = re.compile(r"^\s*//\s*FUNCTION\b", re.IGNORECASE)
_UNNAMED_FUNCTION = re.compile(r"^Function[0-9a-f]{6,8}$", re.IGNORECASE)


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


def _is_function_template(semantic_id: str) -> bool:
    """True for a function-template primary or an emitted specialization.

    Address-owned free functions are ordinary (non-template) entities. A
    same-named ``template <class T> … operator+`` primary, or an MSVC-mangled
    specialization of one, is a distinct overload and must not be treated as a
    consumer redeclaration of the address-owned symbol.
    """

    if not semantic_id:
        return False
    # Clang records dependent/primary template signatures this way in the index.
    if semantic_id.startswith(("FunctionDecl:", "CXXMethodDecl:")):
        return True
    if "type-parameter-" in semantic_id:
        return True
    # MSVC mangled function-template specialization/instantiation: ??$?… / ??$…
    return bool(semantic_id.startswith(("??$?", "??$")))


def _signature_end(lines: list[str], start: int, end: int) -> int:
    """Last 1-based line of the declarator, not the function body.

    Definitions report ``end_line`` at the closing brace; address comments inside
    the body must not bind the declaration.
    """

    last = start
    for position in range(start - 1, min(end, len(lines))):
        last = position + 1
        if "{" in lines[position] or lines[position].rstrip().endswith(";"):
            break
    return last


def _declaration_address(lines: list[str], start: int, end: int) -> str | None:
    """Address attached to a declaration: same-line/signature comment, or a
    preceding ``/* 0x... */`` / ``/* 0x...:`` comment block.

    Walk-back stops at ``// FUNCTION:`` markers and other non-comment code so a
    stale address comment above a prior entity cannot leak onto the next.
    """

    signature_end = _signature_end(lines, start, end)
    own = []
    for position in range(start - 1, signature_end):
        if 0 <= position < len(lines):
            own.extend(_ADDRESS.findall(lines[position]))
    if own:
        return own[-1].lower()[2:].rjust(8, "0")
    position = start - 2
    while 0 <= position < len(lines):
        text = lines[position].strip()
        if not text:
            position -= 1
            continue
        if _FUNCTION_MARKER.match(lines[position]):
            break
        if text.startswith(("//", "/*", "*")):
            match = _ADDRESS.search(lines[position])
            if match:
                return match.group(1).lower()[2:].rjust(8, "0")
            position -= 1
            continue
        break
    return None


def _declaration_lines(repo_dir: Path, entry: dict[str, Any]) -> list[str] | None:
    path = repo_dir / entry["source_file"]
    if not path.is_file():
        return None
    return path.read_text(encoding="utf-8", errors="ignore").splitlines()


def _unnamed_definition_violations(index: dict[str, Any]) -> list[dict[str, Any]]:
    violations: list[dict[str, Any]] = []
    for declaration in index["declarations"]:
        if not declaration.get("is_definition"):
            continue
        qualified_name = str(declaration.get("qualified_name") or "")
        name = _last_component(qualified_name)
        if not _UNNAMED_FUNCTION.fullmatch(name):
            continue
        source = str(declaration.get("source_file") or "")
        line = int(declaration.get("line") or 0)
        violations.append(
            {
                "kind": "unnamed-function-definition",
                "reason": "recovered body keeps address-derived name",
                "names": [qualified_name],
                "source": source,
                "line": line,
                "detail": (
                    f"{source}:{line}: {qualified_name} has a recovered body but still uses "
                    "an address-derived Function... name"
                ),
            }
        )
    return violations


class IdentityGateError(RuntimeError):
    """A recovered function identity is ambiguous or still unnamed."""


def validate_identity(repo_dir: Path) -> dict[str, Any]:
    violations = identity_violations(repo_dir)
    if violations:
        rendered = [item["detail"] for item in violations]
        raise IdentityGateError("function identity gate failed:\n  " + "\n  ".join(rendered))
    return {
        "ok": True,
        "gate": "address-identity",
    }


def identity_violations(repo_dir: Path) -> list[dict[str, Any]]:
    from .source_index import project_targets

    index = json.loads((repo_dir / "build/source-index.json").read_text(encoding="utf-8"))
    targets = project_targets(repo_dir)
    declarations_by_key = {
        (str(entry.get("target") or ""), str(entry.get("semantic_id") or "")): entry
        for entry in index["declarations"]
        if entry.get("semantic_id")
    }

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

    def marker_declaration(marker: dict[str, Any]) -> dict[str, Any]:
        """Resolve a v3 ``declaration_key`` or a legacy embedded declaration."""

        embedded = marker.get("declaration")
        if isinstance(embedded, dict) and embedded:
            return embedded
        key = marker.get("declaration_key")
        if isinstance(key, (list, tuple)) and len(key) >= 2:
            return declarations_by_key.get((str(key[0]), str(key[1]))) or {}
        return {}

    claims: dict[tuple[str, str], list[dict[str, Any]]] = defaultdict(list)
    for marker in index["markers"]:
        if marker["marker_kind"] != "FUNCTION":
            continue
        declaration = marker_declaration(marker)
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
                "alias": bool(marker.get("folded")),
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
                "alias": any(
                    _IDENTITY_ALIAS.search(line)
                    for line in lines[max(0, entry["line"] - 6) : entry["end_line"]]
                ),
            }
        )
        address_declaration_keys.add((entry["source_file"], entry["line"], entry["end_line"]))

    violations = _unnamed_definition_violations(index)
    for (ns, address), entries in sorted(claims.items()):
        owning_entries = [entry for entry in entries if not entry["alias"]]
        names = {entry["name"] for entry in owning_entries}
        prototypes = {entry["prototype"] for entry in owning_entries if entry["prototype"]}
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
            # Template primaries/specializations are distinct overloads, not
            # alternate spellings of an address-owned ordinary free function.
            if _is_function_template(entry["semantic_id"]):
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
        semantic_id = entry.get("semantic_id") or ""
        if _is_function_template(semantic_id):
            continue
        key = (
            entry["qualified_name"],
            _linkage_prefix(semantic_id),
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
