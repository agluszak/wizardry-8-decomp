"""Rank non-gating recovery debt from the existing source model."""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any, cast

from ..ghidra.unit_intervals import TranslationUnitLayout, assertion_anchors, read_assertions
from ..source_index import address_bound_identities, load_source_index, source_functions
from ..source_units import UNRESOLVED_FRAGMENT, source_unit_records

_PLACEHOLDER = re.compile(r"^Function[0-9A-Fa-f]{6,8}$")
_ADDRESS_SUFFIX = re.compile(r"[A-Za-z_][A-Za-z0-9_:<>]*[0-9A-Fa-f]{6,8}$")
_SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx"})

_STALE_CLAIM = re.compile(
    r"unrecovered|unported|not\s+yet\s+identified|not\s+yet\s+ported|"
    r"not\s+yet\s+recovered|not\s+identified|body\s+is\s+not\s+ported|"
    r"unresolved\s+at\s+link|stays\s+unresolved|named\s+by\s+address|"
    r"identity\s+ceiling",
    re.IGNORECASE,
)
_EXPLICIT_ADDRESS = re.compile(r"\b0x([0-9A-Fa-f]{6,8})\b")
_ADDRESS_SUFFIXED_TOKEN = re.compile(r"\b[A-Za-z_][A-Za-z0-9_:<>]*[0-9A-Fa-f]{6,8}\b")
_TRAILING_HEX = re.compile(r"([0-9A-Fa-f]{6,8})$")
_FIELD_FLOW_CATEGORY_LIMIT = 64

_SOURCE_SHAPING_PATTERNS = (
    ("forceinline", re.compile(r"\b__forceinline\b")),
    ("noinline", re.compile(r"\b__declspec\s*\(\s*noinline\s*\)")),
    (
        "optimizer_pragma",
        re.compile(
            r"^\s*#\s*pragma\s+(?:optimize|inline_depth|inline_recursion|auto_inline)\b",
            re.IGNORECASE,
        ),
    ),
)
_SOURCE_SHAPING_ROOTS = {
    "WIZ8": ("src/wiz8", "include/wiz8"),
    "SURRENDER": ("src/surrender", "include/surrender"),
}


def _source_shaping_directives(repository: Path, target: str) -> list[dict[str, Any]]:
    """Return compiler controls that may encode codegen instead of authored design."""

    rows: list[dict[str, Any]] = []
    for root_name in _SOURCE_SHAPING_ROOTS.get(target.upper(), ()):
        root = repository / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix.lower() not in _SOURCE_SUFFIXES:
                continue
            relative = str(path.relative_to(repository))
            for line_number, line in enumerate(
                path.read_text(encoding="utf-8", errors="replace").splitlines(), 1
            ):
                for kind, pattern in _SOURCE_SHAPING_PATTERNS:
                    if not pattern.search(line):
                        continue
                    rows.append(
                        {
                            "source_file": relative,
                            "line": line_number,
                            "kind": kind,
                            "spelling": line.strip(),
                            "status": "investigation_candidate",
                            "reason": (
                                "compiler source-shaping must be justified by authored-source "
                                "evidence, not comparison score"
                            ),
                        }
                    )
                    break
    return rows


def _comment_regions(lines: list[str]) -> list[tuple[int, int, str]]:
    """Return (start_line, end_line, text) for ``/* */`` blocks and ``//`` runs.

    Lines are 1-based and the end line is exclusive. ``//``-only consecutive
    lines merge into one region; a trailing ``//`` after code forms its own
    single-line region. Only the first block comment on a line is captured.
    """

    regions: list[tuple[int, int, str]] = []
    index = 0
    total = len(lines)
    while index < total:
        line = lines[index]
        if line.lstrip().startswith("//"):
            start = index
            parts = []
            while index < total and lines[index].lstrip().startswith("//"):
                parts.append(lines[index].lstrip()[2:])
                index += 1
            regions.append((start + 1, index, "\n".join(parts)))
            continue
        block = line.find("/*")
        slash = line.find("//")
        if slash != -1 and (block == -1 or slash < block):
            regions.append((index + 1, index + 1, line[slash:]))
            index += 1
            continue
        if block == -1:
            index += 1
            continue
        start = index
        parts = [line[block + 2 :]]
        closing = parts[0].find("*/")
        if closing != -1:
            parts[0] = parts[0][:closing]
        else:
            index += 1
            while index < total:
                closing = lines[index].find("*/")
                if closing != -1:
                    parts.append(lines[index][:closing])
                    break
                parts.append(lines[index])
                index += 1
        regions.append((start + 1, index + 1, "\n".join(parts)))
        index += 1
    return regions


def _candidate_addresses(text: str) -> set[int]:
    """Collect retail VAs cited by a comment: ``0x`` literals and the hex
    suffixes positional names embed, accepting dropped leading zeros."""

    addresses = {int(match.group(1), 16) for match in _EXPLICIT_ADDRESS.finditer(text)}
    for token in _ADDRESS_SUFFIXED_TOKEN.finditer(text):
        digits_match = _TRAILING_HEX.search(token.group(0))
        if not digits_match:
            continue
        digits = digits_match.group(1)
        addresses.add(int(digits, 16))
        if len(digits) > 6:
            addresses.add(int(digits[-6:], 16))
    return addresses


def _stale_recovery_claims(repository: Path, functions: dict[int, Any]) -> list[dict[str, Any]]:
    """Flag prose debt claims whose cited address is a defined FUNCTION.

    A claim phrase alone is not flagged: plenty of comments legitimately call
    out still-unrecovered fields or routines. A claim becomes stale when the
    retail address it cites (or an address-suffixed name it mentions) already
    carries a FUNCTION marker in the source index.
    """

    rows: list[dict[str, Any]] = []
    for root_name in ("include/wiz8", "src/wiz8"):
        root = repository / root_name
        for path in sorted(root.rglob("*")):
            if not path.is_file() or path.suffix.casefold() not in _SOURCE_SUFFIXES:
                continue
            relative = path.relative_to(repository).as_posix()
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
            for start, end, comment in _comment_regions(lines):
                phrase = _STALE_CLAIM.search(comment)
                if not phrase:
                    continue
                context = [comment]
                for line in lines[end : min(end + 8, len(lines))]:
                    if not line.strip():
                        break
                    context.append(line)
                for address in sorted(_candidate_addresses("\n".join(context))):
                    function = functions.get(address)
                    if function is None:
                        continue
                    rows.append(
                        {
                            "source_file": relative,
                            "line": start,
                            "phrase": phrase.group(0),
                            "address": f"0x{address:08x}",
                            "resolved_name": function.name,
                            "resolved_source_file": function.source_file,
                        }
                    )
    rows.sort(key=lambda row: (row["source_file"], row["line"], row["address"]))
    return rows


def _unresolved_declarations(index: dict[str, Any]) -> list[dict[str, Any]]:
    definitions = {
        str(item.get("semantic_id") or "")
        for item in index.get("declarations", [])
        if item.get("is_definition")
    }
    rows: dict[str, dict[str, Any]] = {}
    for item in index.get("declarations", []):
        name = str(item.get("qualified_name") or "")
        semantic_id = str(item.get("semantic_id") or "")
        if not _PLACEHOLDER.fullmatch(name) or semantic_id in definitions:
            continue
        rows.setdefault(
            semantic_id,
            {
                "name": name,
                "semantic_id": semantic_id,
                "source_file": str(item.get("source_file") or ""),
                "line": int(item.get("line") or 0),
            },
        )
    return sorted(rows.values(), key=lambda row: (row["name"], row["source_file"], row["line"]))


def semantic_name_opportunity_report(repository: Path) -> dict[str, Any]:
    """Rank unresolved placeholder names by their checked-in call-site footprint."""

    index = load_source_index(repository)
    candidates = _unresolved_declarations(index)
    sources = []
    for root_name in ("include/wiz8", "src/wiz8"):
        root = repository / root_name
        sources.extend(
            path.read_text(encoding="utf-8", errors="replace")
            for path in root.rglob("*")
            if path.is_file() and path.suffix.casefold() in _SOURCE_SUFFIXES
        )
    corpus = "\n".join(sources)
    rows = []
    for candidate in candidates:
        references = len(re.findall(rf"\b{re.escape(candidate['name'])}\s*\(", corpus))
        if references < 2:
            continue
        rows.append({**candidate, "references": references})
    rows.sort(key=lambda row: (-row["references"], row["name"]))
    return {
        "schema": "wiz8.semantic-name-opportunities-v1",
        "non_gating": True,
        "count": len(rows),
        "functions": rows,
    }


def _field_observations(index: dict[str, Any], target: str) -> list[dict[str, Any]]:
    """Group compiler-collected member expressions by canonical field identity."""
    class_ids = {
        (str(record.get("target") or "").upper(), str(record.get("qualified_name") or "")): str(
            record.get("semantic_id") or ""
        )
        for record in index.get("classes", [])
        if record.get("semantic_id") and record.get("qualified_name")
    }
    grouped: dict[tuple[str, str, str], dict[str, Any]] = {}
    for use in index["member_uses"]:
        if str(use.get("target") or "").upper() != target.upper():
            continue
        owner_identity = str(use.get("owner_identity") or "")
        field_identity = str(use.get("field_identity") or "")
        if not field_identity:
            continue
        key = (target.upper(), owner_identity, field_identity)
        offset_bits = use.get("offset_bits")
        row = grouped.setdefault(
            key,
            {
                "target": target.upper(),
                "owner_identity": owner_identity or None,
                "owner_semantic_id": class_ids.get((target.upper(), str(use.get("owner") or "")))
                or None,
                "owner_status": str(
                    use.get("owner_status") or ("resolved" if owner_identity else "unknown")
                ),
                "field_identity": field_identity,
                "field_usr": str(use.get("field_usr") or "") or None,
                "owner": str(use.get("owner") or "") or None,
                "name": str(use.get("name") or ""),
                "declaration_file": str(use.get("declaration_file") or ""),
                "declaration_line": int(use.get("declaration_line") or 0),
                "declaration_column": int(use.get("declaration_column") or 0),
                "declaration_offset": use.get("declaration_offset"),
                "offset_bits": offset_bits,
                "extent_bits": use.get("extent_bits"),
                "offset_bytes": use.get("offset_bytes"),
                "extent_bytes": use.get("extent_bytes"),
                "layout_source": use.get("layout_source")
                or ("clang-target-layout" if offset_bits is not None else None),
                "declared_type": str(use.get("declared_type") or ""),
                "uses": [],
            },
        )
        row["uses"].append(
            {
                "field_identity": field_identity,
                "function_identity": str(use.get("function_identity") or ""),
                "function": str(use.get("function") or ""),
                "source_file": str(use.get("use_file") or ""),
                "line": int(use.get("use_line") or 0),
                "column": int(use.get("use_column") or 0),
                "offset": use.get("use_offset"),
                "operations": list(use.get("operations") or ()),
                "array_indices": list(use.get("array_indices") or ()),
                "conversions": list(use.get("conversions") or ()),
            }
        )
    rows = list(grouped.values())
    for row in rows:
        row["uses"].sort(
            key=lambda use: (
                use["source_file"],
                use["line"],
                use["column"],
                use["function_identity"],
            )
        )
        row["references"] = len(row["uses"])
        row["consumers"] = sorted(
            {(use["function_identity"], use["function"], use["source_file"]) for use in row["uses"]}
        )
    return sorted(
        rows,
        key=lambda row: (
            row["target"],
            row["owner_identity"] or "~unresolved-owner",
            row["field_identity"],
        ),
    )


def _source_pointee(type_name: str) -> str | None:
    normalized = re.sub(r"\b(class|struct|enum)\s+", "", type_name.strip())
    normalized = re.sub(r"\b(const|volatile)\b", "", normalized).strip()
    match = re.fullmatch(r"(.+?)\s*\*+", normalized)
    return match.group(1).strip() if match else None


def _class_member_fields(
    owner_class: dict[str, Any] | None,
    classes_by_name: dict[str, dict[str, Any]],
    field_groups: dict[str, list[dict[str, Any]]],
) -> list[dict[str, Any]]:
    """Return observed fields in a class, including trusted base-subobject offsets."""
    if owner_class is None:
        return []

    fields: list[dict[str, Any]] = []

    def visit(record: dict[str, Any], base_offset: int, path: frozenset[str]) -> None:
        semantic_id = str(record.get("semantic_id") or "")
        if not semantic_id or semantic_id in path:
            return
        for field in field_groups.get(semantic_id, []):
            offset = field.get("offset_bytes")
            adjusted = dict(field)
            adjusted["offset_bytes"] = offset + base_offset if isinstance(offset, int) else None
            fields.append(adjusted)

        if record.get("layout_trusted") is not True:
            return
        offsets = {
            str(base.get("name") or ""): base.get("offset")
            for base in record.get("base_offsets", [])
            if isinstance(base, dict)
        }
        for base_name in record.get("bases", []):
            base_name = str(base_name)
            offset = offsets.get(base_name)
            base_record = classes_by_name.get(base_name)
            if isinstance(offset, int) and base_record is not None:
                visit(base_record, base_offset + offset, path | {semantic_id})

    visit(owner_class, 0, frozenset())
    return fields


def _field_flow_triage(
    repository: Path,
    index: dict[str, Any],
    target: str,
    flows: list[dict[str, Any]],
    *,
    identities_by_address: dict[int, tuple[Any, ...]] | None = None,
) -> dict[str, Any]:
    """Join a bounded set of rooted retail flows to compiler-owned source fields."""
    wanted = target.upper()
    fields = _field_observations(index, wanted)
    field_groups: dict[str, list[dict[str, Any]]] = {}
    for field in fields:
        if field["owner_semantic_id"]:
            field_groups.setdefault(field["owner_semantic_id"], []).append(field)

    identities = (
        identities_by_address
        if identities_by_address is not None
        else address_bound_identities(repository, wanted)
    )
    classes_by_name = {
        str(record.get("qualified_name") or ""): record
        for record in index.get("classes", [])
        if str(record.get("target") or "").upper() == wanted
        and record.get("qualified_name")
        and record.get("semantic_id")
    }
    class_ids_by_name = {
        name: str(record["semantic_id"]) for name, record in classes_by_name.items()
    }
    report: dict[str, Any] = {
        "schema": "wiz8.semantic-debt-field-flow-v1",
        "scope": "bounded selected functions and roots; no whole-program absence claims",
        "categories": {
            "known_selector_outside_declared_array": [],
            "actual_pointee_stride": [],
            "byte_access_interpretation": [],
            "typed_project_conversion": [],
            "cross_member_boundary": [],
            "bulk_copy_candidates": [],
            "unresolved_owner_or_root": [],
        },
        "selector_observations": [],
        "queries": [],
    }

    for request in flows:
        flow = request.get("flow") or request.get("result") or request
        entry_text = str(flow.get("entry") or flow.get("function", {}).get("entry") or "")
        try:
            entry = int(entry_text, 16)
        except ValueError:
            entry = -1
        candidates = identities.get(entry, ())
        identity = candidates[0] if len(candidates) == 1 else None
        root = flow.get("root") or {}
        root_role = str(root.get("role") or "")
        owner_name: str | None = None
        witness_kind: str | None = None
        parameter_index = request.get("source_parameter_index")
        if identity is not None and root_role == "receiver" and identity.owning_class:
            owner_name = identity.owning_class
            witness_kind = "source method receiver"
        elif identity is not None and parameter_index is not None:
            parameters = identity.parameter_types
            if 0 <= int(parameter_index) < len(parameters):
                owner_name = _source_pointee(parameters[int(parameter_index)])
                if owner_name:
                    witness_kind = "source declaration parameter"
        owner_semantic_id = class_ids_by_name.get(owner_name or "")
        owner_class = classes_by_name.get(owner_name or "")
        owner_fields = _class_member_fields(owner_class, classes_by_name, field_groups)
        root_identity = str(root.get("identity") or "")
        accesses = list(flow.get("accesses") or ())
        stops = list((flow.get("completeness") or {}).get("stops") or ())
        query = {
            "case": str(request.get("case") or ""),
            "program": flow.get("program"),
            "entry": entry_text,
            "function": flow.get("function"),
            "profile": flow.get("profile"),
            "root": root,
            "source_identity": identity.qualified_name if identity is not None else None,
            "source_owner_witness": (
                {
                    "kind": witness_kind,
                    "owner": owner_name,
                    "owner_semantic_id": owner_semantic_id,
                }
                if owner_name
                else None
            ),
            "completeness": flow.get("completeness"),
        }
        report["queries"].append(query)

        if owner_semantic_id is None or not root_identity:
            report["categories"]["unresolved_owner_or_root"].append(
                {
                    "case": query["case"],
                    "entry": entry_text,
                    "requested_root": root.get("requested"),
                    "source_identity_candidates": [item.qualified_name for item in candidates],
                    "reason": (
                        "no compiler class record for the source owner"
                        if owner_name and owner_semantic_id is None
                        else "no unique source owner/pointer witness for this root"
                    ),
                    "stops": stops,
                }
            )
        elif stops:
            report["categories"]["unresolved_owner_or_root"].append(
                {
                    "case": query["case"],
                    "entry": entry_text,
                    "owner_semantic_id": owner_semantic_id,
                    "reason": "the selected rooted flow is incomplete; positive rows remain scoped",
                    "stops": stops,
                }
            )

        direct_accesses = []
        for access in accesses:
            address = access.get("effective_address") or {}
            if address.get("root") != root_identity or address.get("terms"):
                continue
            constant = address.get("constant")
            if not isinstance(constant, int):
                continue
            direct_accesses.append((access, constant))

        if identity is not None and owner_semantic_id:
            function_uses = [
                use
                for field in owner_fields
                for use in field["uses"]
                if use["function_identity"] == identity.semantic_id
            ]
            field_by_identity: dict[str, list[dict[str, Any]]] = {}
            for field in owner_fields:
                if any(use["function_identity"] == identity.semantic_id for use in field["uses"]):
                    field_by_identity.setdefault(field["field_identity"], []).append(field)
            for use in function_uses:
                matching_fields = field_by_identity.get(use["field_identity"], [])
                if len(matching_fields) != 1 or "array-index" not in use["operations"]:
                    continue
                field = matching_fields[0]
                array_match = re.search(r"\[(\d+)\]$", field["declared_type"])
                extent = field["extent_bytes"]
                if not array_match or not isinstance(extent, int):
                    continue
                count = int(array_match.group(1))
                if count <= 0 or extent % count:
                    continue
                element_bytes = extent // count
                field_offset = field["offset_bytes"]
                if not isinstance(field_offset, int):
                    continue
                for selector in use["array_indices"]:
                    if not selector.get("constant"):
                        continue
                    value = int(selector["value"])
                    destination = field_offset + value * element_bytes
                    matching = [
                        access
                        for access, offset in direct_accesses
                        if offset == destination
                        and access.get("kind") == "store"
                        and access.get("width") == element_bytes
                    ]
                    if not matching:
                        continue
                    row = {
                        "case": query["case"],
                        "owner_semantic_id": owner_semantic_id,
                        "field_identity": field["field_identity"],
                        "field": field["name"],
                        "declared_type": field["declared_type"],
                        "array_count": count,
                        "element_bytes": element_bytes,
                        "selector": value,
                        "in_bounds": 0 <= value < count,
                        "retail_store_sites": [access["site"] for access in matching],
                        "retail_store_offset": f"0x{destination:x}",
                        "source_use": {
                            "file": use["source_file"],
                            "line": use["line"],
                        },
                        "status": "within_declared_extent"
                        if 0 <= value < count
                        else "contradiction",
                    }
                    report["selector_observations"].append(row)
                    if not row["in_bounds"]:
                        report["categories"]["known_selector_outside_declared_array"].append(row)

        parameter_index = request.get("source_parameter_index")
        if identity is not None and parameter_index is not None:
            parameters = identity.parameter_types
            pointee = (
                _source_pointee(parameters[int(parameter_index)])
                if 0 <= int(parameter_index) < len(parameters)
                else None
            )
            pointee_class = classes_by_name.get(pointee or "")
            pointee_semantic_id = class_ids_by_name.get(pointee or "")
            pointee_fields = _class_member_fields(pointee_class, classes_by_name, field_groups)
            observed_fields = {
                (field["name"], field["offset_bytes"], field["extent_bytes"])
                for field in pointee_fields
                if any(use["function_identity"] == identity.semantic_id for use in field["uses"])
            }
            source_extent = None
            if pointee_class is not None and pointee_class.get("layout_trusted") is True:
                size = pointee_class.get("size")
                if isinstance(size, int):
                    source_extent = size
            stride_values: set[int] = set()
            for access in accesses:
                effective_address = access.get("effective_address")
                if (
                    not isinstance(effective_address, dict)
                    or effective_address.get("root") != root_identity
                ):
                    continue
                terms = effective_address.get("terms")
                if not isinstance(terms, list):
                    continue
                for term in cast(list[Any], terms):
                    if isinstance(term, dict) and isinstance(term.get("stride"), int):
                        stride_values.add(int(term["stride"]))
            strides = sorted(stride_values)
            if strides:
                if source_extent is None:
                    status = "source_layout_unavailable"
                elif strides == [source_extent]:
                    status = "consistent"
                else:
                    status = "stride_contradiction"
                report["categories"]["actual_pointee_stride"].append(
                    {
                        "case": query["case"],
                        "source_identity": identity.qualified_name,
                        "source_parameter_type": parameters[int(parameter_index)],
                        "source_pointee": pointee,
                        "source_class_semantic_id": pointee_semantic_id,
                        "source_element_bytes": source_extent,
                        "source_layout_alignment_bytes": (
                            pointee_class.get("alignment") if pointee_class is not None else None
                        ),
                        "source_layout_trusted": (
                            pointee_class.get("layout_trusted")
                            if pointee_class is not None
                            else None
                        ),
                        "observed_source_fields": [
                            {"name": name, "offset_bytes": offset, "extent_bytes": extent}
                            for name, offset, extent in sorted(
                                observed_fields,
                                key=lambda item: (
                                    item[1] if isinstance(item[1], int) else -1,
                                    item[0],
                                    item[2] if isinstance(item[2], int) else -1,
                                ),
                            )
                        ],
                        "retail_strides_bytes": strides,
                        "status": status,
                        "ghidra_root_type": root.get("type"),
                        "ghidra_type_origin": root.get("type_origin"),
                    }
                )

        if owner_semantic_id:
            for access, offset in direct_accesses:
                width = access.get("width")
                if not isinstance(width, int) or width <= 0:
                    continue
                containing = [
                    field
                    for field in owner_fields
                    if isinstance(field["offset_bytes"], int)
                    and isinstance(field["extent_bytes"], int)
                    and field["offset_bytes"]
                    <= offset
                    < field["offset_bytes"] + field["extent_bytes"]
                ]
                if len(containing) != 1:
                    continue
                field = containing[0]
                field_end = field["offset_bytes"] + field["extent_bytes"]
                if width == 1 and field["extent_bytes"] > 1:
                    report["categories"]["byte_access_interpretation"].append(
                        {
                            "case": query["case"],
                            "owner_semantic_id": owner_semantic_id,
                            "field_identity": field["field_identity"],
                            "offset": f"0x{offset:x}",
                            "declared_type": field["declared_type"],
                            "access_site": access["site"],
                            "status": "investigation_candidate",
                            "reason": "a byte access does not by itself establish a byte semantic field",
                        }
                    )
                if offset + width > field_end:
                    bulk_copy_uses = [
                        {
                            "field_identity": candidate["field_identity"],
                            "operations": operation,
                            "source_file": use["source_file"],
                            "line": use["line"],
                        }
                        for candidate in owner_fields
                        for use in candidate["uses"]
                        if identity is not None and use["function_identity"] == identity.semantic_id
                        for operation in use["operations"]
                        if operation.startswith(("memory-source:", "memory-destination:"))
                    ]
                    report["categories"]["cross_member_boundary"].append(
                        {
                            "case": query["case"],
                            "owner_semantic_id": owner_semantic_id,
                            "field_identity": field["field_identity"],
                            "offset": f"0x{offset:x}",
                            "width": width,
                            "field_extent": [field["offset_bytes"], field_end],
                            "access_site": access["site"],
                            "same_function_bulk_copy_uses": bulk_copy_uses,
                            "status": "investigation_candidate",
                            "reason": "an aggregate or bulk copy may explain the crossing",
                        }
                    )

    conversions = []
    for field in fields:
        for use in field["uses"]:
            for conversion in use["conversions"]:
                source = _source_pointee(conversion.get("source_type", "")) or ""
                destination = _source_pointee(conversion.get("destination_type", "")) or ""
                if (
                    source in classes_by_name
                    and destination in classes_by_name
                    and source != destination
                ):
                    conversions.append(
                        {
                            "owner_identity": field["owner_identity"],
                            "owner_semantic_id": field["owner_semantic_id"],
                            "field_identity": field["field_identity"],
                            "function_identity": use["function_identity"],
                            "source_type": conversion["source_type"],
                            "destination_type": conversion["destination_type"],
                            "status": "local_project_type_conversion_observed",
                            "resolved_conversion": False,
                        }
                    )
    report["categories"]["typed_project_conversion"] = conversions

    for field in fields:
        for use in field["uses"]:
            copies = [
                operation
                for operation in use["operations"]
                if operation.startswith(("memory-source:", "memory-destination:"))
            ]
            if copies:
                report["categories"]["bulk_copy_candidates"].append(
                    {
                        "owner_identity": field["owner_identity"],
                        "owner_semantic_id": field["owner_semantic_id"],
                        "field_identity": field["field_identity"],
                        "function_identity": use["function_identity"],
                        "operations": copies,
                        "status": "bulk_copy_candidate",
                    }
                )
    report["category_summary"] = {}
    for name, rows in report["categories"].items():
        count = len(rows)
        if count > _FIELD_FLOW_CATEGORY_LIMIT:
            del rows[_FIELD_FLOW_CATEGORY_LIMIT:]
        report["category_summary"][name] = {
            "observed": count,
            "reported": len(rows),
            "omitted": count - len(rows),
        }
    return report


def semantic_debt_report(
    repository: Path,
    target: str = "WIZ8",
    *,
    field_flows: list[dict[str, Any]] | None = None,
) -> dict[str, Any]:
    """Return a read-only recovery queue; none of its rows are validation failures."""

    index = load_source_index(repository)
    units = source_unit_records(repository)
    fragment_paths = [
        path for path, record in units.items() if record["class"] == UNRESOLVED_FRAGMENT
    ]
    functions = source_functions(repository, target)
    suffixed = [
        {
            "address": f"0x{address:08x}",
            "name": function.name,
            "source_file": function.source_file,
        }
        for address, function in sorted(functions.items())
        if _ADDRESS_SUFFIX.fullmatch(function.name) and not _PLACEHOLDER.fullmatch(function.name)
    ]

    unit_set = set(fragment_paths)
    anchors, headers = assertion_anchors(read_assertions(repository))
    layout = TranslationUnitLayout(anchors, header_anchors=headers)
    provisional_by_file: dict[str, dict[str, Any]] = {
        path: {"source_file": path, "function_count": 0, "candidate_original_units": set()}
        for path in fragment_paths
    }
    for marker in index.get("markers", []):
        current = str(marker.get("source_file") or "")
        if marker.get("marker_kind") != "FUNCTION" or current not in unit_set:
            continue
        provisional_by_file[current]["function_count"] += 1
        address = int(marker["address"])
        owner = layout.owner(address)
        if owner.get("attribution") not in {"direct", "bounded", "cross-build"}:
            continue
        candidate = str(owner.get("source_path") or "")
        if candidate:
            provisional_by_file[current]["candidate_original_units"].add(candidate)

    provisional = [
        {**row, "candidate_original_units": sorted(row["candidate_original_units"])}
        for row in provisional_by_file.values()
    ]
    provisional.sort(key=lambda row: (-row["function_count"], row["source_file"]))

    fields = _field_observations(index, target)
    high_reference_fields = [row for row in fields if row["references"] >= 3]
    unknown_owners = [row for row in fields if row["owner_status"] in {"unknown", "ambiguous"}]
    source_shaping = _source_shaping_directives(repository, target)
    unresolved = _unresolved_declarations(index)
    stale = _stale_recovery_claims(repository, functions)
    return {
        "schema": "wiz8.semantic-debt-v1",
        "non_gating": True,
        "summary": {
            "unresolved_fragments": len(fragment_paths),
            "unresolved_function_declarations": len(unresolved),
            "address_suffixed_names": len(suffixed),
            "high_reference_provisional_fields": len(high_reference_fields),
            "owner_qualified_field_observations": len(fields),
            "field_observations_without_owner": len(unknown_owners),
            "source_shaping_directives": len(source_shaping),
            "provisional_tu_placements": len(provisional),
            "stale_recovery_claims": len(stale),
        },
        "unresolved_fragments": provisional,
        "unresolved_function_declarations": unresolved,
        "address_suffixed_names": suffixed,
        "field_observations": fields,
        "field_observations_without_owner": unknown_owners,
        "high_reference_provisional_fields": high_reference_fields,
        "source_shaping_directives": source_shaping,
        "field_flow_triage": (
            _field_flow_triage(repository, index, target, field_flows)
            if field_flows is not None
            else None
        ),
        "provisional_tu_placements": provisional,
        "stale_recovery_claims": stale,
    }
