"""Header role classification against recovered translation-unit ownership.

Headers mix several concepts today: shared layouts, TU interfaces, reconstructed
declaration splits, header-owned implementations, and genuine C/SGP bridges.
This module records the intended role, maps each header declaration — free or
member — onto the TU that defines it, and fails classified headers that
violate their role.

Path convention: everything under ``include/wiz8/layouts/`` is shared-layout.
``include/wiz8/sgp_bridge.h`` is the SGP C bridge. Remaining roles come from
``src/wiz8/header_architecture.json``. Unclassified headers are reported, not
gated, so the check can land before every header is re-homed.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

from .ghidra.unit_intervals import TranslationUnitLayout, assertion_anchors, read_assertions
from .paths import atomic_json
from .source_units import (
    CLASSIFICATION_PATH,
    ORIGINAL_TU,
    SourceUnitError,
    cmake_source_units,
    load_source_unit_document,
    mapped_repository_source_file,
    original_source_paths,
    source_unit_records,
)

BRIDGE = "bridge"
SHARED_LAYOUT = "shared-layout"
TU_INTERFACE = "tu-interface"
RECONSTRUCTED = "reconstructed-declarations"
HEADER_IMPLEMENTATION = "header-implementation"
UNCLASSIFIED = "unclassified"
ROLES = frozenset(
    {BRIDGE, SHARED_LAYOUT, TU_INTERFACE, RECONSTRUCTED, HEADER_IMPLEMENTATION, UNCLASSIFIED}
)
PLACED_ATTRIBUTIONS = frozenset({"direct", "bounded", "cross-build"})
OWNED_ATTRIBUTIONS = PLACED_ATTRIBUTIONS | {"recovered-original-tu"}
ARCHITECTURE_PATH = Path("src/wiz8/header_architecture.json")
HEADER_ROOT = Path("include/wiz8")
BRIDGE_HEADER = "include/wiz8/sgp_bridge.h"
SOURCE_SUFFIXES = frozenset({".c", ".cpp", ".h", ".hpp"})
HEADER_SUFFIXES = frozenset({".h", ".hpp"})
SKIP_DIRECTORIES = frozenset({"sgp-compat"})

_FUNCTION_MARKER = re.compile(
    r"^\s*//\s*FUNCTION:\s+(?P<target>[A-Za-z0-9_]+)\s+(?P<address>0x[0-9a-fA-F]+)\s*$",
    re.IGNORECASE,
)
_ADDRESS = re.compile(r"0x[0-9a-fA-F]{6,8}")
_DECL_NAME = re.compile(
    r"(?P<name>operator[^\s(]*|~?[A-Za-z_]\w*(?:::[A-Za-z_~]\w*)*)"
    r"\s*\((?P<args>[^;{}]*?)\)[^;{]*;"
)
_SKIP_DECL_PREFIX = re.compile(
    r"^\s*(?:typedef|using|friend|static_assert|enum|struct|class|namespace|#)\b"
)
_RECORD_SCOPE = re.compile(r"\b(?:struct|class)\s+([A-Za-z_]\w*)?[^;{}]*$")
_TRANSPARENT_SCOPE = re.compile(r'^\s*(?:extern\s+"C"|namespace\b)')
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE_COMMENT = re.compile(r"//.*?$", re.MULTILINE)


class HeaderArchitectureError(RuntimeError):
    """A classified header violates its architectural role."""


def _posix(path: Path, repo_dir: Path) -> str:
    return path.relative_to(repo_dir).as_posix()


def load_header_architecture_document(repo_dir: Path) -> dict[str, Any]:
    path = repo_dir / ARCHITECTURE_PATH
    if not path.is_file():
        return {
            "schema": "wiz8.header-architecture-v1",
            "headers": {},
            "proven-original-headers": {},
        }
    document = json.loads(path.read_text(encoding="utf-8"))
    if document.get("schema") != "wiz8.header-architecture-v1":
        raise HeaderArchitectureError(f"{ARCHITECTURE_PATH} has an unsupported schema")
    for relative, configured in (document.get("headers") or {}).items():
        role = str((configured or {}).get("role") or "")
        if role not in ROLES:
            raise HeaderArchitectureError(
                f"{ARCHITECTURE_PATH}: {relative} has unknown role {role!r}"
            )
    return document


def _assertion_layout(repo_dir: Path) -> TranslationUnitLayout:
    units, headers = assertion_anchors(read_assertions(repo_dir))
    return TranslationUnitLayout(units, header_anchors=headers)


def _strip_comments(text: str) -> str:
    return _LINE_COMMENT.sub("", _BLOCK_COMMENT.sub(" ", text))


def _next_code_line(lines: list[str], start: int) -> str:
    collected: list[str] = []
    for index in range(start, min(start + 8, len(lines))):
        stripped = lines[index].strip()
        if not stripped or stripped.startswith("//"):
            continue
        collected.append(stripped)
        joined = " ".join(collected)
        if ";" in joined or "{" in joined:
            return joined
    return " ".join(collected)


def _declarator_name(text: str) -> str | None:
    cleaned = _strip_comments(text).replace("\n", " ")
    match = _DECL_NAME.search(cleaned)
    if match is None:
        return None
    return match.group("name")


def scan_function_definitions(repo_dir: Path) -> dict[int, dict[str, Any]]:
    """Map FUNCTION marker addresses onto the recovered defining file and name."""

    definitions: dict[int, dict[str, Any]] = {}
    roots = [repo_dir / "src/wiz8", repo_dir / "include/wiz8"]
    for root in roots:
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix.lower() not in SOURCE_SUFFIXES or not path.is_file():
                continue
            relative = _posix(path, repo_dir)
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
            for index, line in enumerate(lines):
                marker = _FUNCTION_MARKER.match(line)
                if marker is None:
                    continue
                address = int(marker.group("address"), 16)
                following = _next_code_line(lines, index + 1)
                name = _declarator_name(
                    following + (";" if "(" in following and ";" not in following else "")
                )
                if name is None:
                    fallback = re.search(r"([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*\(", following)
                    name = fallback.group(1) if fallback else ""
                definitions[address] = {
                    "address": address,
                    "name": name,
                    "source_file": relative,
                    "line": index + 1,
                }
    return definitions


def _header_paths(repo_dir: Path) -> list[Path]:
    root = repo_dir / HEADER_ROOT
    if not root.is_dir():
        return []
    paths: list[Path] = []
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in HEADER_SUFFIXES:
            continue
        relative = path.relative_to(root).parts
        if relative and relative[0] in SKIP_DIRECTORIES:
            continue
        paths.append(path)
    return paths


def _code_line(line: str, in_block: bool) -> tuple[str, bool]:
    """Return the compilable span of one source line and the block-comment state."""

    if in_block:
        end = line.find("*/")
        if end < 0:
            return "", True
        line = line[end + 2 :]
        in_block = False
    pieces: list[str] = []
    index = 0
    while index < len(line):
        block = line.find("/*", index)
        slash = line.find("//", index)
        if block < 0 and slash < 0:
            pieces.append(line[index:])
            break
        if slash >= 0 and (block < 0 or slash < block):
            pieces.append(line[index:slash])
            break
        pieces.append(line[index:block])
        end = line.find("*/", block + 2)
        if end < 0:
            return "".join(pieces), True
        index = end + 2
    return "".join(pieces), in_block


_RECORD_KEYWORDS = re.compile(r"\b(?:struct|class)\s+([A-Za-z_]\w*)")
_TEMPLATE_PREFIX = re.compile(r"\btemplate\s*<[^;{}>]*>")
_INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')

_SCOPE_RECORD = "record"
_SCOPE_TRANSPARENT = "transparent"
_SCOPE_OPAQUE = "opaque"


def scan_header_function_declarations(path: Path, repo_dir: Path) -> list[dict[str, Any]]:
    """Function declarations and in-header definitions in one header.

    Both namespace-scope declarations and member declarations are collected;
    member names are qualified with the enclosing record (``Class::method``).
    ``member`` marks declarations inside a class body and ``defined`` marks
    entries the header itself emits (in-class or out-of-class bodies), so role
    checks can distinguish a header's interface from the code it owns.
    ``extern "C"`` and ``namespace`` blocks are transparent: their contents
    count as namespace-scope declarations. Function bodies, enums, unions and
    brace initializers are opaque and their contents are ignored.
    """

    relative = _posix(path, repo_dir)
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    declarations: list[dict[str, Any]] = []
    scopes: list[dict[str, Any]] = []
    pending_record: str | None = None
    pending_transparent = False
    pending_address: int | None = None
    buffer = ""
    buffer_line = 0
    in_block = False

    def scope_names() -> str:
        names = [s["name"] for s in scopes if s["kind"] == _SCOPE_RECORD and s["name"]]
        return "::".join(names)

    def flush(defined: bool, member: bool, index: int) -> None:
        nonlocal buffer, pending_address
        head = buffer.split("{", 1)[0]
        text = head + ";" if defined else buffer
        name = _declarator_name(text)
        addresses = [int(item, 16) for item in _ADDRESS.findall(buffer)]
        address = pending_address or (addresses[-1] if addresses else None)
        if name is not None:
            member = member or "::" in name or "::" in head
            if member and "::" not in name:
                prefix = scope_names()
                name = f"{prefix}::{name}" if prefix else name
            declarations.append(
                {
                    "name": name,
                    "address": address,
                    "header": relative,
                    "line": buffer_line or index + 1,
                    "member": member,
                    "defined": defined,
                }
            )
        buffer = ""
        pending_address = None

    for index, line in enumerate(lines):
        marker = _FUNCTION_MARKER.match(line)
        if marker is not None:
            pending_address = int(marker.group("address"), 16)
        code, in_block = _code_line(line, in_block)
        stripped = code.strip()

        scope_text = _TEMPLATE_PREFIX.sub(" ", stripped)
        brace_pos = stripped.find("{")
        equals_pos = stripped.find("=")
        record_names = _RECORD_KEYWORDS.findall(scope_text)
        if record_names:
            record_opens = brace_pos >= 0 and (equals_pos < 0 or brace_pos < equals_pos)
            record_continues = brace_pos < 0 and ";" not in stripped
            if record_opens or record_continues:
                pending_record = record_names[-1]
        if _TRANSPARENT_SCOPE.match(stripped):
            pending_transparent = True

        scope = scopes[-1] if scopes else None
        at_member = scope is not None and scope["kind"] == _SCOPE_RECORD
        at_namespace = scope is None or scope["kind"] == _SCOPE_TRANSPARENT
        collectible = (at_member or at_namespace) and pending_record is None

        if collectible:
            if buffer:
                buffer = f"{buffer} {stripped}"
            elif stripped and not stripped.startswith("#"):
                if _SKIP_DECL_PREFIX.match(stripped):
                    pending_address = None
                elif "(" in stripped and not stripped.startswith("static_assert"):
                    buffer = stripped
                    buffer_line = index + 1

        if buffer:
            if "{" in buffer:
                flush(defined=True, member=at_member, index=index)
            elif ";" in buffer:
                head = buffer.split("(", 1)[0]
                if "=" not in head or "operator" in head:
                    flush(defined=False, member=at_member, index=index)
                else:
                    buffer = ""
                    pending_address = None

        for character in stripped:
            if character == "{":
                if pending_record is not None:
                    scopes.append({"kind": _SCOPE_RECORD, "name": pending_record})
                    pending_record = None
                elif pending_transparent:
                    scopes.append({"kind": _SCOPE_TRANSPARENT, "name": None})
                    pending_transparent = False
                else:
                    scopes.append({"kind": _SCOPE_OPAQUE, "name": None})
            elif character == "}":
                if scopes:
                    scopes.pop()
            elif character == ";":
                pending_record = None
                pending_transparent = False
    return declarations


def _role_for(relative: str, document: dict[str, Any]) -> tuple[str, list[str]]:
    headers = document.get("headers") or {}
    configured = headers.get(relative) or {}
    if configured.get("role"):
        return str(configured["role"]), [
            str(item) for item in configured.get("implementation-tus") or ()
        ]
    if relative == BRIDGE_HEADER:
        return BRIDGE, []
    if relative.startswith("include/wiz8/layouts/"):
        return SHARED_LAYOUT, []
    return UNCLASSIFIED, []


def _original_unit_for_file(repo_dir: Path, source_file: str) -> str | None:
    if not source_file.endswith((".c", ".cpp")):
        return None
    try:
        records = source_unit_records(repo_dir)
    except SourceUnitError:
        records = {}
    record = records.get(source_file)
    if record and record.get("class") == ORIGINAL_TU:
        return record.get("original_path")
    originals = original_source_paths(repo_dir)
    return originals.get(source_file)


def _owner_unit(
    layout: TranslationUnitLayout,
    repo_dir: Path,
    address: int | None,
    definition: dict[str, Any] | None,
) -> dict[str, Any]:
    if address is not None:
        owner = layout.owner(address)
        attribution = str(owner.get("attribution") or "")
        unit = str(owner.get("source_path") or "")
        if attribution in PLACED_ATTRIBUTIONS and unit:
            return {
                "original_unit": unit,
                "attribution": attribution,
                "source_file": (definition or {}).get("source_file", ""),
            }
    if definition is not None:
        source_file = str(definition.get("source_file") or "")
        unit = _original_unit_for_file(repo_dir, source_file)
        if unit:
            return {
                "original_unit": unit,
                "attribution": "recovered-original-tu",
                "source_file": source_file,
            }
        if source_file:
            return {
                "original_unit": source_file,
                "attribution": "recovered-file",
                "source_file": source_file,
            }
    return {"original_unit": "", "attribution": "unknown", "source_file": ""}


def _definitions_by_name(definitions: dict[int, dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    by_name: dict[str, list[dict[str, Any]]] = {}
    for item in definitions.values():
        name = str(item.get("name") or "")
        if name:
            by_name.setdefault(name, []).append(item)
    return by_name


def _definitions_by_tail(definitions: dict[int, dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    by_tail: dict[str, list[dict[str, Any]]] = {}
    for item in definitions.values():
        name = str(item.get("name") or "")
        if name:
            by_tail.setdefault(name.rsplit("::", 1)[-1], []).append(item)
    return by_tail


def _header_includes(path: Path) -> list[str]:
    return [
        match.group(1)
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines()
        if (match := _INCLUDE.match(line))
    ]


def _resolve_include(
    relative: str, include: str, scanned: dict[str, list[dict[str, Any]]]
) -> str | None:
    """Resolve a quoted include to a scanned ``include/wiz8`` header."""

    candidates = [
        (Path(relative).parent / include).as_posix(),
        (Path("include") / include).as_posix(),
        (HEADER_ROOT / include).as_posix(),
    ]
    for candidate in candidates:
        if candidate in scanned:
            return candidate
    return None


def analyze_header_architecture(
    repo_dir: Path, layout: TranslationUnitLayout | None = None
) -> dict[str, Any]:
    document = load_header_architecture_document(repo_dir)
    if layout is None:
        layout = _assertion_layout(repo_dir)
    definitions = scan_function_definitions(repo_dir)
    by_name = _definitions_by_name(definitions)
    by_tail = _definitions_by_tail(definitions)
    headers: list[dict[str, Any]] = []
    violations: list[dict[str, Any]] = []

    proven = document.get("proven-original-headers") or {}
    for relative, original in proven.items():
        path = repo_dir / relative
        stem = Path(relative)
        if stem.suffix.lower() == ".hpp":
            legacy = stem.with_suffix(".h")
            if (repo_dir / legacy).is_file() and not path.is_file():
                violations.append(
                    {
                        "kind": "proven-header-spelling",
                        "file": legacy.as_posix(),
                        "detail": (
                            f"{legacy.as_posix()} is the proven original header "
                            f"{original}; rename it to {relative}"
                        ),
                    }
                )
        if path.suffix.lower() != ".hpp" and str(original).lower().endswith(".hpp"):
            violations.append(
                {
                    "kind": "proven-header-spelling",
                    "file": relative,
                    "detail": f"{relative} is proven as {original}",
                }
            )

    scanned: dict[str, list[dict[str, Any]]] = {}
    for path in _header_paths(repo_dir):
        scanned[_posix(path, repo_dir)] = scan_header_function_declarations(path, repo_dir)

    for relative, declarations in scanned.items():
        path = repo_dir / relative
        role, implementation_tus = _role_for(relative, document)
        units: dict[str, int] = {}
        declared: list[dict[str, Any]] = []
        for declaration in declarations:
            if declaration.get("defined"):
                continue
            address = declaration.get("address")
            definition = definitions.get(address) if isinstance(address, int) else None
            if definition is None:
                name = str(declaration["name"])
                matches = by_name.get(name, [])
                if len(matches) != 1 and "::" in name:
                    tail = name.rsplit("::", 1)[-1]
                    candidates = by_tail.get(tail, [])
                    if len(candidates) > 1:
                        prefix = name.rsplit("::", 1)[0]
                        narrowed = [
                            item
                            for item in candidates
                            if str(item["name"]).startswith(f"{prefix}::")
                            or str(item["name"]) == tail
                        ]
                        if narrowed:
                            candidates = narrowed
                    if len(candidates) == 1:
                        matches = candidates
                if len(matches) == 1:
                    definition = matches[0]
                    address = int(definition["address"])
            owner = _owner_unit(
                layout, repo_dir, address if isinstance(address, int) else None, definition
            )
            unit = str(owner.get("original_unit") or "")
            if unit and owner.get("attribution") in OWNED_ATTRIBUTIONS | {"recovered-file"}:
                units[unit] = units.get(unit, 0) + 1
            declared.append({**declaration, **owner, "address": address})
        original_placed = sorted(
            {
                item["original_unit"]
                for item in declared
                if item.get("attribution") in OWNED_ATTRIBUTIONS and item.get("original_unit")
            }
        )
        record = {
            "file": relative,
            "role": role,
            "implementation_tus": implementation_tus,
            "functions": len(declarations),
            "original_units": original_placed,
            "all_units": sorted(units),
        }
        headers.append(record)

        if role == SHARED_LAYOUT and declarations:
            violations.append(
                {
                    "kind": "layout-declares-functions",
                    "file": relative,
                    "detail": (
                        f"{relative} is shared-layout but declares or defines "
                        f"{len(declarations)} function(s)"
                    ),
                    "functions": [item["name"] for item in declarations[:8]],
                }
            )
        if role == SHARED_LAYOUT:
            for include in _header_includes(path):
                target = _resolve_include(relative, include, scanned)
                if target is None:
                    continue
                exposed = [item["name"] for item in scanned[target] if not item["member"]]
                if exposed:
                    violations.append(
                        {
                            "kind": "layout-includes-interface",
                            "file": relative,
                            "detail": (
                                f"{relative} includes {target}, which declares "
                                f"{len(exposed)} namespace-scope function(s)"
                            ),
                            "include": target,
                            "functions": exposed[:8],
                        }
                    )
        if role == TU_INTERFACE and not implementation_tus and len(original_placed) > 1:
            violations.append(
                {
                    "kind": "tu-interface-mixed-units",
                    "file": relative,
                    "detail": (
                        f"{relative} is tu-interface but declares functions from "
                        + ", ".join(original_placed)
                    ),
                    "original_units": original_placed,
                }
            )
        if role in (TU_INTERFACE, RECONSTRUCTED) and implementation_tus:
            allowed = {item.casefold() for item in implementation_tus}
            foreign = [
                item
                for item in declared
                if item.get("attribution") in OWNED_ATTRIBUTIONS
                and item.get("original_unit")
                and str(item["original_unit"]).casefold() not in allowed
            ]
            if foreign:
                units_found = sorted({str(item["original_unit"]) for item in foreign})
                violations.append(
                    {
                        "kind": f"{role}-foreign-unit",
                        "file": relative,
                        "detail": (
                            f"{relative} lists implementation TUs "
                            f"{implementation_tus} but declares {units_found}"
                        ),
                        "original_units": units_found,
                    }
                )
        if role == BRIDGE and relative != BRIDGE_HEADER:
            violations.append(
                {
                    "kind": "unexpected-bridge",
                    "file": relative,
                    "detail": f"{relative} is classified as bridge; only {BRIDGE_HEADER} is",
                }
            )

    fragments = _fragment_ownership(repo_dir, layout, definitions)
    report = {
        "schema": "wiz8.header-architecture-report-v1",
        "headers": headers,
        "violations": violations,
        "unclassified_mixed": [
            item
            for item in headers
            if item["role"] == UNCLASSIFIED and len(item["original_units"]) > 1
        ],
        "unresolved_fragments": fragments,
    }
    return report


def _fragment_ownership(
    repo_dir: Path,
    layout: TranslationUnitLayout,
    definitions: dict[int, dict[str, Any]],
) -> list[dict[str, Any]]:
    if not (repo_dir / CLASSIFICATION_PATH).is_file():
        return []
    try:
        document = load_source_unit_document(repo_dir)
    except SourceUnitError:
        return []
    unresolved = {str(item) for item in document.get("unresolved-fragment") or ()}
    listed = (
        set(cmake_source_units(repo_dir))
        if (repo_dir / "src/wiz8/sources.cmake").is_file()
        else unresolved
    )
    rows: list[dict[str, Any]] = []
    for path in sorted(unresolved & listed):
        owned = [item for item in definitions.values() if item["source_file"] == path]
        units: dict[str, int] = {}
        unknown = 0
        for item in owned:
            owner = layout.owner(int(item["address"]))
            if str(owner.get("attribution") or "") in PLACED_ATTRIBUTIONS and owner.get(
                "source_path"
            ):
                unit = str(owner["source_path"])
                units[unit] = units.get(unit, 0) + 1
            else:
                unknown += 1
        suggestion = "keep-fragment"
        if owned and not unknown and len(units) == 1:
            suggestion = "merge"
        elif owned and len(units) > 1:
            suggestion = "split"
        rows.append(
            {
                "file": path,
                "functions": len(owned),
                "original_units": sorted(units),
                "unplaced": unknown,
                "suggestion": suggestion,
            }
        )
    return rows


def header_architecture_violations(
    repo_dir: Path, layout: TranslationUnitLayout | None = None
) -> list[dict[str, Any]]:
    return list(analyze_header_architecture(repo_dir, layout=layout)["violations"])


def validate_header_architecture(repo_dir: Path) -> dict[str, Any]:
    report = analyze_header_architecture(repo_dir)
    destination = repo_dir / "build/reports/header-architecture/report.json"
    atomic_json(destination, report)
    violations = list(report["violations"])
    if violations:
        rendered = [f"{item['file']}: {item['kind']}: {item['detail']}" for item in violations]
        raise HeaderArchitectureError("header architecture failed:\n  " + "\n  ".join(rendered))
    return {
        "ok": True,
        "gate": "header-architecture",
        "headers": len(report["headers"]),
        "unclassified_mixed": len(report["unclassified_mixed"]),
        "report": str(destination.relative_to(repo_dir)),
    }


def write_header_architecture_report(repo_dir: Path) -> dict[str, Any]:
    """Write the ownership report even when the gate would fail."""

    report = analyze_header_architecture(repo_dir)
    destination = repo_dir / "build/reports/header-architecture/report.json"
    atomic_json(destination, report)
    return {
        "schema": "wiz8.header-architecture-summary",
        "headers": len(report["headers"]),
        "violations": len(report["violations"]),
        "unclassified_mixed": len(report["unclassified_mixed"]),
        "unresolved_fragments": len(report["unresolved_fragments"]),
        "mergeable_fragments": sum(
            1 for item in report["unresolved_fragments"] if item["suggestion"] == "merge"
        ),
        "report": str(destination.relative_to(repo_dir)),
        "violation_kinds": sorted({item["kind"] for item in report["violations"]}),
    }


def expected_recovered_source(repo_dir: Path, unit: str) -> str | None:
    """Expose original-TU mapping for callers that rehome fragments."""

    return mapped_repository_source_file(repo_dir, unit)
