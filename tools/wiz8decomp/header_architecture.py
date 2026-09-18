"""Header ownership classification against the compiler-backed source index.

Every header under ``include/wiz8`` gets an inferred role:

- ``shared-layout``: ``include/wiz8/layouts/**`` and headers that declare no
  functions or globals — packed records, enums and shared-state ``extern``s.
- ``bridge``: ``include/wiz8/sgp_bridge.h``.
- ``header-implementation``: the header emits the code itself — inline or
  template definitions the index records inside a header.
- ``tu-interface``: resolved declarations belong to one original TU.
- ``multi-tu``: declarations resolve to several original TUs; allowed only
  when ``allowed-multi-tu-headers`` covers the resolved set.
- ``provisional-interface``: every resolved declaration lives in an
  ``unresolved-fragment`` source file; inferred, never persisted.
- ``unresolved``: no declaration resolves to an implementation or a retail
  placement.

Ownership comes from the compiler-backed source index: the
``header_declarations`` projection names each entity a header declares by
``semantic_id``; merged declaration/variable records provide the defining
file; markers bind ``semantic_id`` to a retail address the assertion layout
places. No textual C++ parsing participates — ``#include`` directives are the
only line-level fact read.

``src/wiz8/header_architecture.json`` (``wiz8.header-architecture-v2``)
holds only human decisions: ``proven-original-headers`` records
assertion-evidenced original filenames and ``allowed-multi-tu-headers``
permits named headers to combine named original TUs.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

from .ghidra.unit_intervals import (
    TranslationUnitLayout,
    assertion_anchors,
    read_assertions,
)
from .paths import atomic_json
from .source_units import (
    COMPILER_EMISSION,
    ORIGINAL_TU,
    UNRESOLVED_FRAGMENT,
    SourceUnitError,
    source_unit_records,
)

BRIDGE = "bridge"
SHARED_LAYOUT = "shared-layout"
TU_INTERFACE = "tu-interface"
MULTI_TU = "multi-tu"
PROVISIONAL_INTERFACE = "provisional-interface"
HEADER_IMPLEMENTATION = "header-implementation"
UNRESOLVED = "unresolved"

PLACED_ATTRIBUTIONS = frozenset({"direct", "bounded", "cross-build"})

ARCHITECTURE_PATH = Path("src/wiz8/header_architecture.json")
HEADER_ROOT = Path("include/wiz8")
BRIDGE_HEADER = "include/wiz8/sgp_bridge.h"
SKIP_DIRECTORIES = frozenset({"sgp-compat"})
_LAYOUT_PREFIX = "include/wiz8/layouts/"
_LOCAL_INCLUDE_PREFIX = "wiz8/"

# Compatibility umbrella headers deleted during the recovery transition. A
# re-created file under one of these names is a violation; the list is fixed
# policy, not architecture data.
_REMOVED_AGGREGATES = (
    "include/wiz8/character.h",
    "include/wiz8/combat_state.h",
    "include/wiz8/engine_code/anim.h",
    "include/wiz8/engine_code/anim_defs.h",
    "include/wiz8/engine_code/anim_loader.h",
    "include/wiz8/engine_code/palette_effects.h",
    "include/wiz8/magic.h",
    "include/wiz8/render_state.h",
    "include/wiz8/spell_effect.h",
    "include/wiz8/stats.h",
)

_INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
_V2_KEYS = frozenset({"schema", "proven-original-headers", "allowed-multi-tu-headers"})


class HeaderArchitectureError(RuntimeError):
    """Raised when the header architecture evidence cannot be loaded."""


def _load_index(repo_dir: Path) -> dict[str, Any]:
    try:
        from .source_index import load_source_index
    except ImportError as error:
        raise HeaderArchitectureError(
            f"header architecture needs the reccmp-backed source index: {error}"
        ) from error
    try:
        return load_source_index(repo_dir)
    except Exception as error:
        raise HeaderArchitectureError(
            "header architecture needs a generated build/source-index.json; "
            f"run `uv run wiz8 check` first ({error})"
        ) from error


def load_header_architecture_document(repo_dir: Path) -> dict[str, Any]:
    """Return the small v2 metadata document from the source checkout."""
    path = repo_dir / ARCHITECTURE_PATH
    if not path.is_file():
        return {"schema": "wiz8.header-architecture-v2"}
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        raise HeaderArchitectureError(f"{ARCHITECTURE_PATH} is not valid JSON: {error}") from error
    if not isinstance(document, dict) or document.get("schema") != "wiz8.header-architecture-v2":
        raise HeaderArchitectureError(
            f'{ARCHITECTURE_PATH} must declare "wiz8.header-architecture-v2"'
        )
    unknown = set(document) - _V2_KEYS
    if unknown:
        raise HeaderArchitectureError(f"{ARCHITECTURE_PATH} has unknown v2 keys: {sorted(unknown)}")
    return document


def _assertion_layout(repo_dir: Path) -> TranslationUnitLayout:
    units, headers = assertion_anchors(read_assertions(repo_dir))
    return TranslationUnitLayout(units, header_anchors=headers)


def _header_files(repo_dir: Path) -> list[Path]:
    header_root = repo_dir / HEADER_ROOT
    if not header_root.is_dir():
        return []
    return sorted(
        path
        for path in header_root.rglob("*")
        if path.suffix.lower() in {".h", ".hpp"}
        and not set(path.relative_to(header_root).parts) & SKIP_DIRECTORIES
    )


def _source_includes(path: Path) -> list[str]:
    try:
        return [
            match.group(1)
            for line in path.read_text(encoding="utf-8").splitlines()
            if (match := _INCLUDE.match(line))
        ]
    except OSError:
        return []


def _index_maps(
    index: dict[str, Any],
) -> tuple[dict[str, set[str]], dict[str, set[str]], dict[str, int]]:
    """semantic_id -> defining files, plus semantic_id -> retail address."""
    function_defs: dict[str, set[str]] = {}
    for record in index["declarations"]:
        if record.get("is_definition"):
            function_defs.setdefault(str(record.get("semantic_id")), set()).add(
                str(record.get("source_file"))
            )
    global_defs: dict[str, set[str]] = {}
    for record in index["variables"]:
        if record.get("definition_kind") != "declaration":
            global_defs.setdefault(str(record.get("semantic_id")), set()).add(
                str(record.get("source_file"))
            )
    addresses: dict[str, int] = {}
    for marker in index["markers"]:
        address = marker.get("address")
        key = marker.get("declaration_key")
        if address is None or not isinstance(key, list) or len(key) != 2:
            continue
        addresses.setdefault(str(key[1]), int(address))
    return function_defs, global_defs, addresses


def _classify_source(
    source_file: str,
    units: set[str],
    fragments: set[str],
    emissions: set[str],
    recovered: set[str],
    unit_records: dict[str, dict[str, Any]],
) -> None:
    record = unit_records.get(source_file)
    if record is not None and record["class"] == ORIGINAL_TU:
        units.add(record["original_path"])
    elif record is not None and record["class"] == UNRESOLVED_FRAGMENT:
        fragments.add(source_file)
    elif record is not None and record["class"] == COMPILER_EMISSION:
        emissions.add(source_file)
    else:
        recovered.add(source_file)


def analyze_header_architecture(
    repo_dir: Path, layout: TranslationUnitLayout | None = None
) -> dict[str, Any]:
    """Classify every project header from compiler-index evidence."""
    document = load_header_architecture_document(repo_dir)
    index = _load_index(repo_dir)
    layout = layout if layout is not None else _assertion_layout(repo_dir)
    proven = document.get("proven-original-headers") or {}
    allowed_multi = document.get("allowed-multi-tu-headers") or {}

    try:
        unit_records = source_unit_records(repo_dir)
    except (SourceUnitError, OSError):
        unit_records = {}
    function_defs, global_defs, marker_addresses = _index_maps(index)

    includes: dict[str, list[str]] = {}
    header_sources = [
        path
        for path in _header_files(repo_dir)
        + sorted(
            p
            for p in (repo_dir / "src" / "wiz8").rglob("*")
            if p.suffix.lower() in {".h", ".hpp", ".c", ".cpp"}
        )
        if path.is_file()
    ]
    for path in header_sources:
        includes[str(path.relative_to(repo_dir))] = _source_includes(path)

    decls_by_header: dict[str, list[dict[str, Any]]] = {}
    for record in index.get("header_declarations") or []:
        decls_by_header.setdefault(str(record["source_file"]), []).append(record)
    interface_headers = {
        header
        for header, decls in decls_by_header.items()
        if any(d["kind"] == "function" and not d["member"] and not d["defined"] for d in decls)
    }

    violations: list[dict[str, Any]] = []
    headers: list[dict[str, Any]] = []
    provisional: list[str] = []
    unresolved_headers: list[dict[str, Any]] = []
    multi_tu: list[dict[str, Any]] = []

    for path in _header_files(repo_dir):
        relative = str(path.relative_to(repo_dir))
        decls = decls_by_header.get(relative, [])
        units: set[str] = set()
        fragments: set[str] = set()
        emissions: set[str] = set()
        recovered: set[str] = set()
        header_defined = 0
        unresolved: list[str] = []
        declared: list[dict[str, Any]] = []
        is_layout = relative.startswith(_LAYOUT_PREFIX)

        for decl in decls:
            # A member declaration whose qualified name names a template
            # instantiation belongs to a class template in this header: the
            # member's implementation is the template body, which the index
            # only marks defined where the member was actually instantiated.
            if decl["defined"] or (decl["member"] and "<" in decl["qualified_name"]):
                header_defined += 1
                continue
            definitions = (global_defs if decl["kind"] == "global" else function_defs).get(
                decl["semantic_id"], set()
            )
            owners: list[str] = []
            resolved = False
            for source_file in sorted(definitions):
                if source_file.startswith("include/"):
                    continue
                _classify_source(
                    source_file,
                    units,
                    fragments,
                    emissions,
                    recovered,
                    unit_records,
                )
                owners.append(source_file)
                resolved = True
            if not resolved:
                if any(f.startswith("include/") for f in definitions):
                    header_defined += 1
                    continue
                address = marker_addresses.get(decl["semantic_id"])
                owner = layout.owner(address) if address is not None else None
                if owner and owner["attribution"] in PLACED_ATTRIBUTIONS:
                    units.add(owner["source_path"])
                    owners.append(owner["source_path"])
                else:
                    unresolved.append(str(decl["qualified_name"]))
            declared.append(
                {
                    "kind": decl["kind"],
                    "name": decl["qualified_name"],
                    "line": decl["line"],
                    "member": decl["member"],
                    "owners": owners,
                    "semantic_id": decl["semantic_id"],
                }
            )

        if relative == BRIDGE_HEADER:
            role = BRIDGE
        elif is_layout or not decls:
            role = SHARED_LAYOUT
        elif not units and not fragments and not emissions and not recovered:
            if unresolved:
                role = UNRESOLVED
            else:
                role = HEADER_IMPLEMENTATION
        elif len(units) == 1:
            role = TU_INTERFACE
        elif len(units) > 1:
            role = MULTI_TU
        else:
            role = PROVISIONAL_INTERFACE

        if is_layout:
            functions = [d for d in decls if d["kind"] == "function"]
            if functions:
                violations.append(
                    {
                        "rule": "layout-declares-functions",
                        "header": relative,
                        "functions": [d["qualified_name"] for d in functions],
                    }
                )
            for include in includes.get(relative, []):
                target = f"include/{include}"
                if include.startswith(_LOCAL_INCLUDE_PREFIX) and target in interface_headers:
                    violations.append(
                        {
                            "rule": "layout-includes-interface",
                            "header": relative,
                            "include": target,
                        }
                    )
        if role == MULTI_TU:
            allowed = set(allowed_multi.get(relative) or ())
            if not units <= allowed:
                violations.append(
                    {
                        "rule": "multi-tu-header",
                        "header": relative,
                        "original_units": sorted(units),
                        "allowed": sorted(allowed),
                    }
                )
            multi_tu.append({"file": relative, "original_units": sorted(units)})
        if role == PROVISIONAL_INTERFACE:
            provisional.append(relative)
        if unresolved:
            unresolved_headers.append({"file": relative, "unresolved": sorted(unresolved)})

        headers.append(
            {
                "file": relative,
                "role": role,
                "original_units": sorted(units),
                "original_path": proven.get(relative),
                "fragment_units": sorted(fragments),
                "emission_units": sorted(emissions),
                "recovered_files": sorted(recovered),
                "functions": sum(1 for d in decls if d["kind"] == "function"),
                "globals": sum(1 for d in decls if d["kind"] == "global"),
                "defined_in_header": header_defined,
                "unresolved": sorted(unresolved),
                "includes": includes.get(relative, []),
                "declared": declared,
            }
        )

    for relative, targets in sorted(includes.items()):
        for include in targets:
            if (
                include.startswith(_LOCAL_INCLUDE_PREFIX)
                and not (repo_dir / "include" / include).is_file()
            ):
                violations.append(
                    {
                        "rule": "missing-include",
                        "file": relative,
                        "include": include,
                    }
                )
    for removed in _REMOVED_AGGREGATES:
        if (repo_dir / removed).is_file():
            violations.append({"rule": "removed-aggregate", "header": removed})
    for header, original_path in sorted(proven.items()):
        if Path(header).name != Path(original_path.replace("\\", "/")).name:
            violations.append(
                {
                    "rule": "proven-header-spelling",
                    "header": header,
                    "original_path": original_path,
                }
            )

    return {
        "schema": "wiz8.header-architecture-report-v2",
        "headers": headers,
        "provisional": sorted(provisional),
        "multi_tu": multi_tu,
        "unresolved": unresolved_headers,
        "unresolved_fragments": _fragment_ownership(repo_dir, layout, index),
        "violations": violations,
    }


def _fragment_ownership(
    repo_dir: Path, layout: TranslationUnitLayout, index: dict[str, Any]
) -> list[dict[str, Any]]:
    """Report current fragment evidence without a separate ownership layer."""
    try:
        units = source_unit_records(repo_dir)
    except (SourceUnitError, OSError):
        return []
    fragment_files = {
        path for path, record in units.items() if record["class"] == UNRESOLVED_FRAGMENT
    }
    fragments: list[dict[str, Any]] = []
    for fragment_file in sorted(fragment_files):
        functions = []
        for marker in index["markers"]:
            if (
                marker.get("marker_kind") != "function"
                or marker.get("address") is None
                or marker.get("source_file") != fragment_file
            ):
                continue
            owner = layout.owner(int(marker["address"]))
            if owner["attribution"] == "direct":
                suggestion = "merge"
            elif owner["attribution"] in {
                "bounded",
                "inlined-or-conflicting",
                "cross-build",
            }:
                suggestion = "split"
            else:
                suggestion = "keep-fragment"
            functions.append(
                {
                    "marker": marker["marker_name"],
                    "address": marker["address"],
                    "suggestion": suggestion,
                    "attribution": owner["attribution"],
                    "owner": owner["source_path"],
                }
            )
        fragments.append({"file": fragment_file, "functions": functions, "path": fragment_file})
    return fragments


def header_architecture_violations(report: dict[str, Any]) -> list[str]:
    """Flatten violations into human-readable gate errors."""
    messages: list[str] = []
    for violation in report["violations"]:
        rule = violation["rule"]
        if rule == "layout-declares-functions":
            messages.append(
                f"{violation['header']}: shared-layout header declares functions "
                f"({', '.join(violation['functions'])})"
            )
        elif rule == "layout-includes-interface":
            messages.append(
                f"{violation['header']}: shared-layout header includes {violation['include']}"
            )
        elif rule == "multi-tu-header":
            messages.append(
                f"{violation['header']}: declarations resolve to several "
                f"original TUs {violation['original_units']}; allow them in "
                "allowed-multi-tu-headers or move the declarations"
            )
        elif rule == "missing-include":
            messages.append(
                f"{violation['file']}: includes {violation['include']} "
                "which is not an existing project header"
            )
        elif rule == "removed-aggregate":
            messages.append(
                f"{violation['header']}: deleted compatibility umbrella must not be recreated"
            )
        elif rule == "proven-header-spelling":
            messages.append(
                f"{violation['header']}: proven-header-spelling — original "
                f"filename {violation['original_path']} keeps a different basename"
            )
    return messages


def validate_header_architecture(repo_dir: Path) -> list[str]:
    """Fail the fast lane when a header violates the ownership model."""
    return header_architecture_violations(analyze_header_architecture(repo_dir))


def write_header_architecture_report(repo_dir: Path) -> dict[str, Any]:
    """Persist the classification report and return its compact summary."""
    report = analyze_header_architecture(repo_dir)
    path = repo_dir / "build/reports/header-architecture/report.json"
    atomic_json(path, report)
    summary = {
        "schema": report["schema"],
        "headers": len(report["headers"]),
        "roles": {
            role: sum(1 for header in report["headers"] if header["role"] == role)
            for role in {header["role"] for header in report["headers"]}
        },
        "unresolved_fragments": len(report["unresolved_fragments"]),
        "violations": len(report["violations"]),
        "path": str(path.relative_to(repo_dir)),
    }
    return summary
