"""Unit tests for gated /wiz8/classes cleanup classification."""

from __future__ import annotations

from types import SimpleNamespace

import pytest
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
        self.replace_calls: list[tuple[str, str, bool]] = []

    def getDataType(self, path: str):
        return self._types.get(path)

    def replaceDataType(self, old: object, new: object, update_category_path: bool) -> object:
        old_path = str(old.getPathName())  # type: ignore[union-attr]
        new_path = str(new.getPathName())  # type: ignore[union-attr]
        self.replace_calls.append((old_path, new_path, update_category_path))
        if update_category_path:
            moved = _FakeStructure(
                old_path,
                int(new.getLength()),
                list(new.getDefinedComponents()),  # type: ignore[union-attr]
            )
            self._types.pop(new_path, None)
            self._types[old_path] = moved
            return moved
        self._types.pop(old_path, None)
        return new


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


def test_cleanup_accepts_exact_duplicate_for_replace() -> None:
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


def test_apply_replace_keeps_bound_category(monkeypatch: pytest.MonkeyPatch) -> None:
    from wiz8decomp.legacy_classes_cleanup import apply_legacy_classes_cleanup

    int_dt = SimpleNamespace(getPathName=lambda: "/int", getLength=lambda: 4)
    components = [_FakeComponent(0, 4, "x", int_dt)]
    legacy = _FakeStructure("/wiz8/classes/stLight", 600, components)
    bound = _FakeStructure("/stLight", 600, list(components))
    manager = _FakeManager({legacy.getPathName(): legacy, bound.getPathName(): bound})
    program = SimpleNamespace(getDataTypeManager=lambda: manager)

    def _immediate_apply_rows(program, rows, apply_one, **_kwargs):
        applied = []
        errors = []
        for row in rows:
            result = dict(apply_one(program, row))
            if result.get("error"):
                errors.append(result)
            else:
                applied.append(result)
        return {"applied": len(applied), "errors": errors, "rows": applied}

    monkeypatch.setattr("wiz8decomp.ghidra.mutations.apply_rows", _immediate_apply_rows)
    result = apply_legacy_classes_cleanup(
        program,
        {
            "types": [
                {
                    "path": legacy.getPathName(),
                    "name": "stLight",
                    "action": "replace-then-delete",
                    "bound_path": bound.getPathName(),
                }
            ]
        },
    )
    assert result["errors"] == []
    assert result["applied"] == 1
    assert manager.replace_calls == [("/wiz8/classes/stLight", "/stLight", False)]
    assert manager.getDataType("/stLight") is bound
    assert manager.getDataType("/wiz8/classes/stLight") is None


def test_cleanup_merges_when_ghidra_class_is_missing() -> None:
    int_dt = SimpleNamespace(getPathName=lambda: "/int", getLength=lambda: 4)
    components = [_FakeComponent(0, 4, "x", int_dt)]
    legacy = _FakeStructure("/wiz8/classes/stLight", 600, components)
    bound = _FakeStructure("/stLight", 600, list(components))
    program = SimpleNamespace(
        getDataTypeManager=lambda: _FakeManager(
            {legacy.getPathName(): legacy, bound.getPathName(): bound}
        )
    )
    row = classify_legacy_datatype(
        program,
        legacy,
        identity_map_or_bindings={"stLight": {"status": "missing-class"}},
    )
    assert row["action"] == "replace-then-delete"
    assert row["bound_path"] == "/stLight"
    assert row["reason"] == "exact-duplicate"
