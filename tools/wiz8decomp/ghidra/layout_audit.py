"""Compare compiler-owned layouts with reviewed Ghidra structures."""

from __future__ import annotations

from typing import Any


def audit_source_layouts(program: Any, source_index: dict[str, Any]) -> dict[str, Any]:
    """Inspect an already-open derived program without changing its data types."""

    from ghidra.program.model.data import Pointer, Structure, TypeDef

    def unwrap(data_type: Any) -> Any:
        while isinstance(data_type, TypeDef):
            data_type = data_type.getBaseDataType()
        return data_type

    def structure(data_type: Any) -> Any:
        resolved = unwrap(data_type)
        return resolved if isinstance(resolved, Structure) else None

    def pointer_depth(data_type: Any) -> int:
        depth = 0
        data_type = unwrap(data_type)
        while isinstance(data_type, Pointer):
            if data_type.getDataType() is None:
                return 0
            depth += 1
            data_type = unwrap(data_type.getDataType())
        return depth

    def is_base(component: Any) -> bool:
        name = component.getFieldName()
        return name is not None and (str(name) == "base" or str(name).startswith("base_"))

    def flatten(record: Any, origin: int = 0) -> list[tuple[int, int, Any]]:
        spans = []
        for component in record.getDefinedComponents():
            offset = origin + component.getOffset()
            base = structure(component.getDataType())
            if is_base(component) and base is not None:
                spans.extend(flatten(base, offset))
            else:
                spans.append((offset, component.getLength(), component.getDataType()))
        return spans

    data_types = {
        str(data_type.getPathName()): data_type
        for data_type in program.getDataTypeManager().getAllDataTypes()
    }
    failures: list[dict[str, Any]] = []
    checks = {"classes": 0, "source_fields": 0, "fields": 0, "bases": 0}
    layout_owned = 0

    def fail(kind: str, name: str, field: str | None = None, **details: Any) -> None:
        failure = {"kind": kind, "class": name}
        if field is not None:
            failure["field"] = field
        failures.append({**failure, **details})

    for source_class in source_index["classes"]:
        if source_class.get("asserted_size") is None:
            continue
        if not (source_class.get("source_file") or "").startswith(("src/wiz8/", "include/wiz8/")):
            continue
        layout_owned += 1
        name = source_class["qualified_name"]
        rebuilt = structure(data_types.get("/" + name))
        if rebuilt is None:
            fail("missing-pdb-class", name)
            continue
        checks["classes"] += 1
        expected_size = source_class["asserted_size"]
        if rebuilt.getLength() != expected_size:
            fail("size", name, expected=expected_size, actual=rebuilt.getLength())
        components = rebuilt.getDefinedComponents()
        actual_bases = {
            str(unwrap(component.getDataType()).getDisplayName())
            for component in components
            if is_base(component)
        }
        for expected in source_class["bases"]:
            checks["bases"] += 1
            if expected not in actual_bases:
                fail("base", name, expected=expected, actual=sorted(actual_bases))
        rebuilt_fields = {
            str(component.getFieldName()): component
            for component in components
            if component.getFieldName() is not None
            and str(component.getFieldName()) != "vftable"
            and not is_base(component)
        }
        for source_field in source_class["fields"]:
            checks["source_fields"] += 1
            field_name = source_field["name"]
            actual = rebuilt_fields.get(field_name)
            if actual is None:
                fail("missing-pdb-field", name, field_name)
                continue
            expected_depth = source_field.get("pointer_depth")
            if expected_depth is None:
                raise ValueError(
                    f"source index field lacks pointer_depth; rebuild the source index: {source_field}"
                )
            actual_depth = pointer_depth(actual.getDataType())
            if expected_depth > 0 and expected_depth != actual_depth:
                fail(
                    "source-field-pointer-depth",
                    name,
                    field_name,
                    expected=expected_depth,
                    actual=actual_depth,
                )
        original = structure(data_types.get("/wiz8/classes/" + name))
        if original is None:
            fail("missing-ghidra-class", name)
            continue
        source_spans = flatten(rebuilt)
        for component in original.getDefinedComponents():
            field_name = component.getFieldName()
            if field_name is None or not str(field_name).strip():
                continue
            field_name = str(field_name)
            start = component.getOffset()
            end = start + component.getLength()
            covering = [
                span for span in source_spans if span[0] < end and span[0] + span[1] > start
            ]
            covered = {
                offset
                for offset, length, _ in covering
                for offset in range(max(start, offset), min(end, offset + length))
            }
            if len(covered) != component.getLength():
                fail("missing-field", name, field_name)
                continue
            checks["fields"] += 1
            if "vptr" in field_name:
                continue
            exact = [
                span for span in covering if span[0] == start and span[1] == component.getLength()
            ]
            expected_depth = pointer_depth(component.getDataType())
            actual_depth = pointer_depth(exact[0][2]) if exact else 0
            if expected_depth > 0 and actual_depth > 0 and expected_depth != actual_depth:
                fail(
                    "field",
                    name,
                    field_name,
                    expected_pointer_depth=expected_depth,
                    actual_types=[str(span[2].getDisplayName()) for span in exact],
                )
    return {
        "schema": "wiz8.source-layout-audit",
        "ok": not failures,
        "layout_owned_classes": layout_owned,
        "checks": checks,
        "failure_count": len(failures),
        "failures": failures,
    }
