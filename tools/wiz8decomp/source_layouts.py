"""Audit VC6 CodeView class layouts against the reviewed evidence ledger."""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

from .evidence.classes import load_reviewed_class_model, parse_pointee
from .paths import atomic_json
from .reconstructed_pdb import CompiledLayout, load


def _field_name(name: str) -> str:
    return name.removeprefix("m_")


def _reviewed_base_offsets(spec: str) -> set[int]:
    """Offsets explicitly present in the reviewed free-form base relationship."""

    return {int(value, 16) for value in re.findall(r"\+0x([0-9a-fA-F]+)", spec)}


def compare_source_layouts(
    reviewed: Any, compiled: dict[str, CompiledLayout]
) -> dict[str, Any]:
    failures: list[dict[str, Any]] = []
    checks = {"classes": 0, "fields": 0, "bases": 0}
    skipped_fields = 0
    compiled_classes = 0
    fields_by_class: dict[str, list[Any]] = {}
    for field in reviewed.fields:
        fields_by_class.setdefault(field.class_name, []).append(field)
    for source_class in reviewed.classes:
        layout = compiled.get(source_class.name)
        if layout is None:
            continue
        compiled_classes += 1
        checks["classes"] += 1
        if source_class.size is not None and layout.size < source_class.size:
            failures.append(
                {
                    "kind": "minimum-size",
                    "class": source_class.name,
                    "expected": source_class.size,
                    "actual": layout.size,
                }
            )
        ordered = sorted(layout.fields, key=lambda item: item.offset)
        source_fields = {_field_name(item.name): item for item in ordered}
        inferred_widths = {
            item.name: (
                ordered[index + 1].offset - item.offset
                if index + 1 < len(ordered)
                else layout.size - item.offset
            )
            for index, item in enumerate(ordered)
        }
        for field in fields_by_class.get(source_class.name, []):
            # Compiler-owned vptrs are not LF_MEMBER records. Their offsets are
            # covered by reviewed vtable/subobject evidence instead.
            if "vptr" in field.name:
                continue
            actual = source_fields.get(_field_name(field.name))
            # Positional unknown/base-storage names do not prove that a source
            # array and a reviewed subdivision are the same member. They are
            # covered by the class extent, not joined by suggestive spelling.
            if actual is None or field.name.startswith(("unknown_", "base_storage")):
                skipped_fields += 1
                continue
            checks["fields"] += 1
            expected_depth: int | None = None
            if field.data_type == "pointer" and field.pointee:
                _base, extra = parse_pointee(field.pointee)
                expected_depth = 1 + extra
            actual_width = (
                actual.width
                if actual.width not in (None, 0)
                else inferred_widths[actual.name]
            )
            expected = (field.offset, field.size, expected_depth)
            observed = (actual.offset, actual_width, actual.pointer_depth)
            matches = (
                actual.offset == field.offset
                and actual_width == field.size
                and (expected_depth is None or actual.pointer_depth == expected_depth)
            )
            if not matches:
                failures.append(
                    {
                        "kind": "field",
                        "class": source_class.name,
                        "field": field.name,
                        "expected": expected,
                        "actual": observed,
                        "compiled_type": actual.type_name,
                    }
                )
        expected_bases = _reviewed_base_offsets(source_class.base_classes)
        if expected_bases:
            actual_bases = {item.offset for item in layout.bases}
            checks["bases"] += len(expected_bases)
            missing_bases = sorted(expected_bases - actual_bases)
            if missing_bases:
                failures.append(
                    {
                        "kind": "base-offset",
                        "class": source_class.name,
                        "expected": sorted(expected_bases),
                        "actual": sorted(actual_bases),
                        "missing": missing_bases,
                    }
                )
    return {
        "schema": "wiz8.source-layout-audit",
        "ok": not failures,
        "compiled_classes": compiled_classes,
        "checks": checks,
        "skipped_unjoined_fields": skipped_fields,
        "failure_count": len(failures),
        "failures": failures,
    }


def verify_source_layouts(settings: Any, pdb: Path | None = None) -> dict[str, Any]:
    path = pdb or (settings.repo_dir / "build" / "decomp" / "Wiz8.pdb")
    if not path.is_file():
        raise ValueError(f"compiled VC6 PDB does not exist: {path}; build WIZ8 first")
    database = load(path)
    reviewed = load_reviewed_class_model(settings.repo_dir, "wiz8")
    report = compare_source_layouts(reviewed, database.types.layouts())
    report["pdb"] = str(path)
    destination = settings.build_dir / "reports" / "source-layouts" / "report.json"
    atomic_json(destination, report)
    report["report"] = str(destination)
    return report
