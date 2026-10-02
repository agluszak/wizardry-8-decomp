"""Unit tests for type-graph field remapper helpers."""

from __future__ import annotations

import random
from pathlib import Path
from types import SimpleNamespace

from wiz8decomp.datatype_contracts import (
    datatype_shape_key,
    structures_field_shape_agree,
)
from wiz8decomp.type_graph_projection import (
    _asserted_size_classes,
    _selected_identities,
    decide_field_action,
    plan_semantic_hash,
)


def test_asserted_sizes_are_scoped_to_target() -> None:
    source_data = {
        "classes": [
            {"qualified_name": "W8WorldCursorState", "asserted_size": 224, "target": "WIZ8"},
            {"qualified_name": "srMutex", "asserted_size": 24, "target": "SURRENDER"},
        ]
    }
    assert _asserted_size_classes(source_data, "WIZ8") == {"W8WorldCursorState": 224}


def test_template_records_are_excluded_from_ghidra_class_binding(monkeypatch) -> None:
    from wiz8decomp import type_graph_projection as tgp

    monkeypatch.setattr(
        tgp,
        "_thiscall_owning_classes",
        lambda *_args: {"W8WorldCursorState": 1, "srHeapBuffer<unsigned long>": 1},
    )
    source_data = {
        "classes": [
            {"qualified_name": "W8WorldCursorState", "asserted_size": 224, "target": "WIZ8"},
            {"qualified_name": "srHeapBuffer<unsigned long>", "asserted_size": 8, "target": "WIZ8"},
        ]
    }
    assert _selected_identities(source_data, Path(), "WIZ8") == ["W8WorldCursorState"]


class _FakeComponent:
    def __init__(self, offset: int, length: int, name: str, data_type: object) -> None:
        self._offset = offset
        self._length = length
        self._name = name
        self._data_type = data_type

    def getOffset(self) -> int:
        return self._offset

    def getLength(self) -> int:
        return self._length

    def getFieldName(self) -> str:
        return self._name

    def getDataType(self):
        return self._data_type

    def getComment(self):
        return None


class _FakeStructure:
    def __init__(
        self,
        path: str,
        length: int,
        components: list[_FakeComponent] | None = None,
        name: str | None = None,
    ) -> None:
        self._path = path
        self._length = length
        self._components = components or []
        self._name = name or path.rsplit("/", 1)[-1]

    def getPathName(self) -> str:
        return self._path

    def getLength(self) -> int:
        return self._length

    def getName(self) -> str:
        return self._name

    def getDefinedComponents(self):
        return list(self._components)


class _FakePointer:
    def __init__(self, path: str, pointee: object) -> None:
        self._path = path
        self._pointee = pointee

    def getPathName(self) -> str:
        return self._path

    def getLength(self) -> int:
        return 4

    def getDataType(self):
        return self._pointee

    def isPointer(self) -> bool:
        return True


class _FakeArray:
    def __init__(self, path: str, element: object, count: int) -> None:
        self._path = path
        self._element = element
        self._count = count

    def getPathName(self) -> str:
        return self._path

    def getLength(self) -> int:
        return 4 * self._count

    def getDataType(self):
        return self._element

    def getNumElements(self) -> int:
        return self._count


def test_plan_hash_order_independent() -> None:
    plan_a = {
        "identity_map": {
            "B": {
                "status": "agree",
                "bound_path": "/B",
                "evidence_path": "/B",
                "field_action": "agree",
            },
            "A": {
                "status": "create-opaque",
                "bound_path": None,
                "evidence_path": None,
                "field_action": None,
                "asserted_size": 16,
            },
        }
    }
    keys = list(plan_a["identity_map"])
    random.Random(0).shuffle(keys)
    plan_b = {"identity_map": {key: plan_a["identity_map"][key] for key in keys}}
    assert plan_semantic_hash(plan_a) == plan_semantic_hash(plan_b)


def test_idempotent_agree_plan_has_no_field_mutations() -> None:
    bound = _FakeStructure(
        "/Foo",
        16,
        [
            _FakeComponent(
                0, 4, "x", SimpleNamespace(getPathName=lambda: "/int", getLength=lambda: 4)
            )
        ],
    )
    evidence = _FakeStructure(
        "/Foo",
        16,
        [
            _FakeComponent(
                0, 4, "x", SimpleNamespace(getPathName=lambda: "/int", getLength=lambda: 4)
            )
        ],
    )
    assert decide_field_action(bound, evidence) == "agree"
    # Second pass with same shapes stays agree (0 mutations for apply filter).
    assert decide_field_action(bound, evidence) == "agree"


def test_equal_richness_field_disagreement_is_conflict() -> None:
    left_dt = SimpleNamespace(getPathName=lambda: "/int", getLength=lambda: 4)
    right_dt = SimpleNamespace(getPathName=lambda: "/short", getLength=lambda: 4)
    bound = _FakeStructure(
        "/Foo", 8, [_FakeComponent(0, 4, "a", left_dt), _FakeComponent(4, 4, "b", left_dt)]
    )
    evidence = _FakeStructure(
        "/other/Foo",
        8,
        [_FakeComponent(0, 4, "a", left_dt), _FakeComponent(4, 4, "b", right_dt)],
    )
    assert len(bound.getDefinedComponents()) == len(evidence.getDefinedComponents())
    assert not structures_field_shape_agree(bound, evidence)
    assert decide_field_action(bound, evidence) == "conflict"


def test_opaque_plus_rich_same_size_reconciles_fields() -> None:
    evidence = _FakeStructure(
        "/Demangler/Foo",
        840,
        [
            _FakeComponent(
                0, 4, "x", SimpleNamespace(getPathName=lambda: "/int", getLength=lambda: 4)
            )
        ],
    )
    bound = _FakeStructure("/Foo", 840, [])
    assert decide_field_action(bound, evidence) == "reconcile-fields"


def test_datatype_shape_distinguishes_pointer_pointee() -> None:
    a = _FakePointer("/A *", _FakeStructure("/A", 4, []))
    b = _FakePointer("/B *", _FakeStructure("/B", 4, []))
    assert datatype_shape_key(a) != datatype_shape_key(b)


def test_rebuild_fields_uses_replace_within_extent(monkeypatch) -> None:
    from wiz8decomp.type_graph_projection import TypeGraphConflict, _rebuild_fields_from_evidence

    monkeypatch.setattr(
        "wiz8decomp.type_graph_projection._remap_datatype",
        lambda _program, data_type, _identity: data_type,
    )

    class Component(_FakeComponent):
        pass

    class GrowingStructure(_FakeStructure):
        def __init__(self, path: str, length: int, components=None):
            super().__init__(path, length, components)
            self.replaced: list[tuple[int, int]] = []

        def clearAtOffset(self, offset: int) -> None:
            self._components = [item for item in self._components if item.getOffset() != offset]

        def replaceAtOffset(self, offset, data_type, length, name, comment) -> None:
            self.replaced.append((offset, length))
            self._components.append(_FakeComponent(offset, length, name, data_type))

        def insertAtOffset(self, offset, data_type, length, name, comment) -> None:
            self._length += length
            raise AssertionError("insertAtOffset would grow an opaque shell")

    bound = GrowingStructure("/Foo", 840, [])
    evidence = _FakeStructure(
        "/Demangler/Foo",
        840,
        [
            _FakeComponent(
                0, 4, "x", SimpleNamespace(getPathName=lambda: "/int", getLength=lambda: 4)
            )
        ],
    )
    _rebuild_fields_from_evidence(bound, evidence, object(), {})
    assert bound.getLength() == 840
    assert bound.replaced == [(0, 4)]

    mismatch = GrowingStructure("/Foo", 16, [])
    try:
        _rebuild_fields_from_evidence(mismatch, evidence, object(), {})
    except TypeGraphConflict as exc:
        assert "structure-length-mismatch" in str(exc)
    else:
        raise AssertionError("expected size mismatch")


def test_rebuild_rejects_overlap_and_keeps_padding(monkeypatch) -> None:
    from wiz8decomp.type_graph_projection import TypeGraphConflict, _rebuild_fields_from_evidence

    monkeypatch.setattr(
        "wiz8decomp.type_graph_projection._remap_datatype",
        lambda _program, data_type, _identity: data_type,
    )

    class GrowingStructure(_FakeStructure):
        def __init__(self, path: str, length: int, components=None):
            super().__init__(path, length, components)
            self.replaced: list[tuple[int, int]] = []

        def clearAtOffset(self, offset: int) -> None:
            self._components = [item for item in self._components if item.getOffset() != offset]

        def replaceAtOffset(self, offset, data_type, length, name, comment) -> None:
            self.replaced.append((offset, length))
            self._components.append(_FakeComponent(offset, length, name, data_type))

    int_type = SimpleNamespace(getPathName=lambda: "/int", getLength=lambda: 4)
    padded = GrowingStructure("/Foo", 0x40, [])
    evidence = _FakeStructure(
        "/Demangler/Foo",
        0x40,
        [_FakeComponent(0, 4, "x", int_type)],
    )
    _rebuild_fields_from_evidence(padded, evidence, object(), {})
    assert padded.getLength() == 0x40
    assert padded.replaced == [(0, 4)]

    overlapping = GrowingStructure("/Bar", 0x40, [])
    bad = _FakeStructure(
        "/Demangler/Bar",
        0x40,
        [
            _FakeComponent(0, 8, "a", int_type),
            _FakeComponent(4, 8, "b", int_type),
        ],
    )
    try:
        _rebuild_fields_from_evidence(overlapping, bad, object(), {})
    except TypeGraphConflict as exc:
        assert "field-overlap" in str(exc)
    else:
        raise AssertionError("expected overlap")
    assert overlapping.replaced == []
