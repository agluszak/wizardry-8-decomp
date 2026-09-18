"""Unit tests for type-graph field remapper helpers."""

from __future__ import annotations

import random
from types import SimpleNamespace

from wiz8decomp.datatype_contracts import (
    datatype_shape_key,
    has_legacy_nested_ref,
    structures_field_shape_agree,
)
from wiz8decomp.type_graph_projection import (
    decide_field_action,
    plan_semantic_hash,
)


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
    assert decide_field_action(bound, evidence, nested_legacy=False) == "agree"


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


def test_opaque_size_mismatch_is_conflict() -> None:
    evidence = _FakeStructure(
        "/wiz8/classes/Trigger",
        908,
        [
            _FakeComponent(
                0, 4, "x", SimpleNamespace(getPathName=lambda: "/int", getLength=lambda: 4)
            )
        ],
    )
    bound = _FakeStructure("/Trigger", 1, [])
    assert decide_field_action(bound, evidence) == "conflict"


def test_array_element_legacy_detected() -> None:
    legacy = _FakeStructure("/wiz8/classes/Bar", 16, [])
    array = _FakeArray("/Foo/[4]", legacy, 4)
    owner = _FakeStructure("/Foo", 16, [_FakeComponent(0, 16, "items", array)])
    assert has_legacy_nested_ref(owner)
    bound = _FakeStructure("/Foo", 16, [_FakeComponent(0, 16, "items", array)])
    evidence = bound
    assert decide_field_action(bound, evidence) == "remap-nested"


def test_agreeing_structure_still_walked_for_nested_legacy() -> None:
    legacy_pointee = _FakeStructure("/wiz8/classes/Inner", 8, [])
    ptr = _FakePointer("/wiz8/classes/Inner *", legacy_pointee)
    bound = _FakeStructure(
        "/Outer",
        4,
        [_FakeComponent(0, 4, "inner", ptr)],
    )
    # Same path evidence ⇒ still remap when nested legacy remains.
    assert decide_field_action(bound, bound) == "remap-nested"
    assert decide_field_action(bound, bound, nested_legacy=False) == "agree"


def test_datatype_shape_distinguishes_pointer_pointee() -> None:
    a = _FakePointer("/A *", _FakeStructure("/A", 4, []))
    b = _FakePointer("/B *", _FakeStructure("/B", 4, []))
    assert datatype_shape_key(a) != datatype_shape_key(b)


def test_union_with_nested_legacy_is_conflict() -> None:
    class _FakeUnion:
        def __init__(self, path: str, member: object) -> None:
            self._path = path
            self._member = member

        def getPathName(self) -> str:
            return self._path

        def getLength(self) -> int:
            return 4

        def getDefinedComponents(self):
            return [_FakeComponent(0, 4, "as_ptr", self._member)]

    legacy = _FakePointer("/wiz8/classes/Foo *", _FakeStructure("/wiz8/classes/Foo", 4, []))
    union = _FakeUnion("/MaybeFoo", legacy)
    bound = _FakeStructure("/Owner", 4, [_FakeComponent(0, 4, "u", union)])
    assert decide_field_action(bound, bound) == "conflict"


def test_legacy_pointer_agrees_after_identity_normalization() -> None:
    identity = {"Foo": {"bound_path": "/Foo", "evidence_path": "/wiz8/classes/Foo"}}
    legacy = _FakePointer("/wiz8/classes/Foo *", _FakeStructure("/wiz8/classes/Foo", 4, []))
    bound = _FakePointer("/Foo *", _FakeStructure("/Foo", 4, []))
    left = _FakeStructure("/Owner", 4, [_FakeComponent(0, 4, "p", legacy)])
    right = _FakeStructure("/Owner", 4, [_FakeComponent(0, 4, "p", bound)])
    assert not structures_field_shape_agree(left, right)
    assert structures_field_shape_agree(left, right, identity_map=identity)
    assert decide_field_action(left, right, identity_map=identity) == "remap-nested"


def test_class_key_from_path_does_not_invent_namespaces() -> None:
    from wiz8decomp.type_graph_projection import _class_key_from_path

    identities = {"Foo", "alpha::Bar", "stLight"}
    assert _class_key_from_path("/wiz8/classes/ns/Foo") == "ns::Foo"
    assert _class_key_from_path("/alpha/Bar", source_identities=identities) == "alpha::Bar"
    assert _class_key_from_path("/Foo", source_identities=identities) == "Foo"
    assert _class_key_from_path("/Demangler/Foo", source_identities=identities) == "Foo"
    assert _class_key_from_path("/Demangler/Foo") is None
    assert _class_key_from_path("/foo/bar/Baz", source_identities=identities) is None
    assert _class_key_from_path("/stLight", source_identities=identities) == "stLight"
