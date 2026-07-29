"""Audit source-owned layouts against the VC6 PDB and live Ghidra index."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from .paths import atomic_json
from .source_model import load_source_index


def _field_name(name: str) -> str:
    return name.removeprefix("m_")


def _pointer_depth(spelling: str) -> int:
    depth = 0
    value = spelling.rstrip()
    while value.endswith("*"):
        depth += 1
        value = value[:-1].rstrip()
    return depth


def _is_base_component(component: dict[str, Any]) -> bool:
    field = str(component.get("field") or "")
    return field == "base" or field.startswith("base_")


def compare_source_layouts(
    source_classes: list[dict[str, Any]],
    ghidra_types: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    """Check every class made layout-owned by a compiler-enforced size assertion."""

    failures: list[dict[str, Any]] = []
    checks = {"classes": 0, "source_fields": 0, "fields": 0, "bases": 0}
    layout_owned = [item for item in source_classes if item.get("asserted_size") is not None]
    for source_class in layout_owned:
        name = source_class["qualified_name"]
        expected_size = int(source_class["asserted_size"])
        # The reccmp importer materializes the rebuilt PDB class at the root
        # category.  Original reviewed Ghidra layouts remain under
        # /wiz8/classes.  Comparing those two projections keeps PDB decoding in
        # reccmp instead of growing another Wizardry ABI frontend here.
        rebuilt = ghidra_types.get(f"/{name}")
        if rebuilt is None:
            failures.append({"kind": "missing-pdb-class", "class": name})
            continue
        checks["classes"] += 1
        if int(rebuilt["length"]) != expected_size:
            failures.append(
                {
                    "kind": "size",
                    "class": name,
                    "expected": expected_size,
                    "actual": int(rebuilt["length"]),
                }
            )

        actual_bases = {
            str(component["type"])
            for component in rebuilt.get("components", [])
            if _is_base_component(component)
        }
        for expected_base in source_class.get("bases", []):
            checks["bases"] += 1
            if expected_base not in actual_bases:
                failures.append(
                    {
                        "kind": "base",
                        "class": name,
                        "expected": expected_base,
                        "actual": sorted(actual_bases),
                    }
                )

        rebuilt_fields = {
            str(component.get("field")): component
            for component in rebuilt.get("components", [])
            if component.get("field") not in {None, "vftable"} and not _is_base_component(component)
        }
        for source_field in source_class.get("fields", []):
            checks["source_fields"] += 1
            field_name = str(source_field["name"])
            actual_field = rebuilt_fields.get(field_name)
            if actual_field is None:
                failures.append({"kind": "missing-pdb-field", "class": name, "field": field_name})
                continue
            expected_depth = _pointer_depth(str(source_field["type"]))
            actual_depth = _pointer_depth(str(actual_field["type"]))
            if expected_depth and expected_depth != actual_depth:
                failures.append(
                    {
                        "kind": "source-field-pointer-depth",
                        "class": name,
                        "field": field_name,
                        "expected": expected_depth,
                        "actual": actual_depth,
                    }
                )

        original = ghidra_types.get(f"/wiz8/classes/{name}")
        if original is None:
            failures.append({"kind": "missing-ghidra-class", "class": name})
            continue

        def flattened(data_type: dict[str, Any], origin: int = 0) -> list[dict[str, Any]]:
            values: list[dict[str, Any]] = []
            for component in data_type.get("components", []):
                offset = origin + int(component["offset"])
                base = ghidra_types.get(f"/{component.get('type', '')}")
                if _is_base_component(component) and base is not None:
                    values.extend(flattened(base, offset))
                else:
                    values.append({**component, "offset": offset})
            return values

        source_fields = flattened(rebuilt)
        for component in original.get("components", []):
            field = component.get("field")
            if not field:
                continue
            component_offset = int(component["offset"])
            component_end = component_offset + int(component["length"])
            covering = [
                item
                for item in source_fields
                if int(item["offset"]) < component_end
                and int(item["offset"]) + int(item["length"]) > component_offset
            ]
            covered = {
                byte
                for item in covering
                for byte in range(
                    max(component_offset, int(item["offset"])),
                    min(component_end, int(item["offset"]) + int(item["length"])),
                )
            }
            if len(covered) != int(component["length"]):
                failures.append({"kind": "missing-field", "class": name, "field": field})
                continue
            checks["fields"] += 1
            if "vptr" in field:
                continue
            exact = [
                item
                for item in covering
                if int(item["offset"]) == component_offset
                and int(item["length"]) == int(component["length"])
            ]
            expected_depth = _pointer_depth(str(component["type"]))
            actual_depth = _pointer_depth(str(exact[0]["type"])) if exact else 0
            if expected_depth and actual_depth and actual_depth != expected_depth:
                failures.append(
                    {
                        "kind": "field",
                        "class": name,
                        "field": field,
                        "expected_pointer_depth": expected_depth,
                        "actual_types": [str(item["type"]) for item in exact],
                    }
                )
    return {
        "schema": "wiz8.source-layout-audit",
        "ok": not failures,
        "layout_owned_classes": len(layout_owned),
        "checks": checks,
        "failure_count": len(failures),
        "failures": failures,
    }


def verify_source_layouts(settings: Any, pdb: Path | None = None) -> dict[str, Any]:
    path = pdb or (settings.repo_dir / "build/decomp/Wiz8.pdb")
    if not path.is_file():
        raise ValueError(f"compiled VC6 PDB does not exist: {path}; build WIZ8 first")
    types_path = settings.repo_dir / "build/ghidra-index/types.json"
    if not types_path.is_file():
        raise ValueError(f"Ghidra type index does not exist: {types_path}; run wiz8 ghidra index")
    ghidra_document = json.loads(types_path.read_text(encoding="utf-8"))
    ghidra_types = {item["path"]: item for item in ghidra_document["types"]}
    source_classes = [
        item
        for item in load_source_index(settings.repo_dir)["classes"]
        if item["source_file"].startswith(("src/wiz8/", "include/wiz8/"))
    ]
    report = compare_source_layouts(source_classes, ghidra_types)
    report["pdb"] = str(path)
    destination = settings.build_dir / "reports/source-layouts/report.json"
    atomic_json(destination, report)
    report["report"] = str(destination)
    return report


def require_source_layouts(report: dict[str, Any]) -> dict[str, Any]:
    if not report["ok"]:
        raise ValueError(
            f"compiled source layout differs at {report['failure_count']} checks; "
            f"see {report['report']}"
        )
    return report
