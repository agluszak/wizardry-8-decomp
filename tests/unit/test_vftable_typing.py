"""Tests for vftable field naming helpers."""

from __future__ import annotations

from wiz8decomp.vftable_typing import (
    _field_name,
    _points_to_function_definition,
    _retarget_class_vfptr,
    _simple_name,
)


def test_simple_and_field_names() -> None:
    assert _simple_name("ns::W8Monster") == "W8Monster"
    assert _field_name(0, None) == "slot_00"
    assert _field_name(1, "W8Monster::CanEnterCycle") == "CanEnterCycle_01"
    assert (
        _field_name(0, "W8Monster::'scalar_deleting_destructor'") == "scalar_deleting_destructor_00"
    )
    assert _field_name(2, "W8OptionsSlider::~W8OptionsSlider") == "dtor_W8OptionsSlider_02"


def test_points_to_function_definition_rejects_void_pointer() -> None:
    class _Void:
        pass

    class _Pointer:
        def getDataType(self) -> _Void:
            return _Void()

    assert _points_to_function_definition(_Pointer()) is False


def test_points_to_function_definition_accepts_fd_pointer() -> None:
    class _FunctionDefinition:
        def getArguments(self) -> list[object]:
            return []

    class _Pointer:
        def getDataType(self) -> _FunctionDefinition:
            return _FunctionDefinition()

    assert _points_to_function_definition(_Pointer()) is True


def test_retarget_class_vfptr_ignores_root_pdb_structures() -> None:
    class _Manager:
        def getDataType(self, path: str):
            if path == "/W8Monster":
                return object()  # would previously have been mutated
            return None

    class _Program:
        def getDataTypeManager(self) -> _Manager:
            return _Manager()

    assert _retarget_class_vfptr(_Program(), "W8Monster", object()) is False
