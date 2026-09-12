"""Header role classification against recovered translation-unit ownership.

Headers mix several concepts today: shared layouts, TU interfaces, reconstructed
declaration splits, and genuine C/SGP bridges. This module records the intended
role, maps each out-of-line header declaration onto the TU that defines it, and
fails classified headers that violate their role.

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
UNCLASSIFIED = "unclassified"
ROLES = frozenset({BRIDGE, SHARED_LAYOUT, TU_INTERFACE, RECONSTRUCTED, UNCLASSIFIED})
PLACED_ATTRIBUTIONS = frozenset({"direct", "bounded", "cross-build"})
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
    r"(?P<name>[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*\((?P<args>[^;]*?)\)\s*(?:const)?\s*;"
)
_SKIP_DECL_PREFIX = re.compile(
    r"^\s*(?:typedef|using|friend|static_assert|enum|struct|class|namespace|#)\b"
)
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


def _brace_delta(text: str) -> int:
    cleaned = _strip_comments(text)
    return cleaned.count("{") - cleaned.count("}")


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


def scan_header_function_declarations(path: Path, repo_dir: Path) -> list[dict[str, Any]]:
    """Out-of-line function declarations at namespace scope in one header."""

    relative = _posix(path, repo_dir)
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    declarations: list[dict[str, Any]] = []
    depth = 0
    pending_address: int | None = None
    buffer = ""
    buffer_line = 0
    in_block = False
    for index, line in enumerate(lines):
        marker = _FUNCTION_MARKER.match(line)
        if marker is not None:
            pending_address = int(marker.group("address"), 16)
        code, in_block = _code_line(line, in_block)
        stripped = code.strip()
        if depth == 0 and buffer:
            buffer = f"{buffer} {stripped}"
        elif depth == 0 and stripped and not stripped.startswith("#"):
            if _SKIP_DECL_PREFIX.match(stripped):
                pending_address = None
            elif "(" in stripped and not stripped.startswith("static_assert"):
                buffer = stripped
                buffer_line = index + 1
        depth = max(0, depth + _brace_delta(code if code else line))
        if not buffer:
            continue
        if "{" in buffer:
            buffer = ""
            pending_address = None
            continue
        if ";" not in buffer:
            continue
        if "=" in buffer.split("(", 1)[0]:
            buffer = ""
            pending_address = None
            continue
        name = _declarator_name(buffer)
        addresses = [int(item, 16) for item in _ADDRESS.findall(buffer)]
        address = pending_address or (addresses[-1] if addresses else None)
        if name is not None:
            declarations.append(
                {
                    "name": name,
                    "address": address,
                    "header": relative,
                    "line": buffer_line or index + 1,
                }
            )
        buffer = ""
        pending_address = None
    return declarations


def _role_for(relative: str, document: dict[str, Any]) -> tuple[str, list[str]]:
    headers = document.get("headers") or {}
    configured = headers.get(relative) or {}
    if configured.get("role"):
        return str(configured["role"]), [
            str(item) for item in configured.get("implementation-tus") or ()
        ]
    proven = document.get("proven-original-headers") or {}
    if relative in proven:
        return TU_INTERFACE, []
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


def analyze_header_architecture(
    repo_dir: Path, layout: TranslationUnitLayout | None = None
) -> dict[str, Any]:
    document = load_header_architecture_document(repo_dir)
    if layout is None:
        layout = _assertion_layout(repo_dir)
    definitions = scan_function_definitions(repo_dir)
    by_name = _definitions_by_name(definitions)
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

    for path in _header_paths(repo_dir):
        relative = _posix(path, repo_dir)
        role, implementation_tus = _role_for(relative, document)
        declarations = scan_header_function_declarations(path, repo_dir)
        units: dict[str, int] = {}
        declared: list[dict[str, Any]] = []
        for declaration in declarations:
            address = declaration.get("address")
            definition = definitions.get(address) if isinstance(address, int) else None
            if definition is None:
                matches = by_name.get(str(declaration["name"]), [])
                if len(matches) == 1:
                    definition = matches[0]
                    address = int(definition["address"])
            owner = _owner_unit(
                layout, repo_dir, address if isinstance(address, int) else None, definition
            )
            unit = str(owner.get("original_unit") or "")
            if unit and owner.get("attribution") in PLACED_ATTRIBUTIONS | {
                "recovered-original-tu",
                "recovered-file",
            }:
                units[unit] = units.get(unit, 0) + 1
            declared.append({**declaration, **owner, "address": address})
        original_placed = sorted(
            {
                item["original_unit"]
                for item in declared
                if item.get("attribution") in PLACED_ATTRIBUTIONS and item.get("original_unit")
            }
        )
        record = {
            "file": relative,
            "role": role,
            "implementation_tus": implementation_tus,
            "functions": len(declared),
            "original_units": original_placed,
            "all_units": sorted(units),
        }
        headers.append(record)

        if role == SHARED_LAYOUT and declared:
            violations.append(
                {
                    "kind": "layout-declares-functions",
                    "file": relative,
                    "detail": (
                        f"{relative} is shared-layout but declares "
                        f"{len(declared)} out-of-line function(s)"
                    ),
                    "functions": [item["name"] for item in declared[:8]],
                }
            )
        if role == TU_INTERFACE and len(original_placed) > 1:
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
        if role == RECONSTRUCTED and implementation_tus:
            allowed = {item.casefold() for item in implementation_tus}
            foreign = [
                item
                for item in declared
                if item.get("attribution") in PLACED_ATTRIBUTIONS
                and item.get("original_unit")
                and str(item["original_unit"]).casefold() not in allowed
            ]
            if foreign:
                units_found = sorted({str(item["original_unit"]) for item in foreign})
                violations.append(
                    {
                        "kind": "reconstructed-foreign-unit",
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
