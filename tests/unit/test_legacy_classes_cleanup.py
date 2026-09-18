"""Unit tests for gated /wiz8/classes cleanup classification."""

from __future__ import annotations

from types import SimpleNamespace

from wiz8decomp.legacy_classes_cleanup import classify_legacy_datatype


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


class _FakeStructure:
    def __init__(
        self, path: str, length: int, components: list[_FakeComponent] | None = None
    ) -> None:
        self._path = path
        self._length = length
        self._components = components or []
        self._name = path.rsplit("/", 1)[-1]

    def getPathName(self) -> str:
        return self._path

    def getLength(self) -> int:
        return self._length

    def getName(self) -> str:
        return self._name

    def getDefinedComponents(self):
        return list(self._components)


class _FakeManager:
    def __init__(self, types: dict[str, object]) -> None:
        self._types = types

    def getDataType(self, path: str):
        return self._types.get(path)

    def getDataTypesContaining(self, _dt):
        return []


def test_cleanup_refuses_size_mismatch() -> None:
    int_dt = SimpleNamespace(getPathName=lambda: "/int", getLength=lambda: 4)
    legacy = _FakeStructure(
        "/wiz8/classes/Trigger",
        908,
        [_FakeComponent(0, 4, "x", int_dt)],
    )
    bound = _FakeStructure("/Trigger", 1, [])
    program = SimpleNamespace(
        getDataTypeManager=lambda: _FakeManager({"/Trigger": bound, legacy.getPathName(): legacy})
    )
    identity = {
        "Trigger": {"bound_path": "/Trigger", "status": "agree"},
    }
    row = classify_legacy_datatype(program, legacy, identity_map_or_bindings=identity)
    assert row["action"] == "conflict"
    assert row["reason"] == "size-mismatch"


def test_cleanup_accepts_exact_duplicate_for_replace_or_delete() -> None:
    int_dt = SimpleNamespace(getPathName=lambda: "/int", getLength=lambda: 4)
    components = [_FakeComponent(0, 4, "x", int_dt)]
    legacy = _FakeStructure("/wiz8/classes/W8Monster", 840, components)
    bound = _FakeStructure("/W8Monster", 840, list(components))
    program = SimpleNamespace(
        getDataTypeManager=lambda: _FakeManager({"/W8Monster": bound, legacy.getPathName(): legacy})
    )
    identity = {"W8Monster": {"bound_path": "/W8Monster", "status": "agree"}}
    row = classify_legacy_datatype(program, legacy, identity_map_or_bindings=identity)
    assert row["action"] == "replace-then-delete"
    assert row["bound_path"] == "/W8Monster"


def test_cleanup_refuses_when_bound_still_legacy_path() -> None:
    legacy = _FakeStructure("/wiz8/classes/Foo", 16, [])
    program = SimpleNamespace(getDataTypeManager=lambda: _FakeManager({}))
    identity = {
        "Foo": {"bound_path": "/wiz8/classes/Foo", "status": "legacy-duplicate"},
    }
    row = classify_legacy_datatype(program, legacy, identity_map_or_bindings=identity)
    assert row["action"] == "conflict"
    assert row["reason"] == "bound-still-legacy"
