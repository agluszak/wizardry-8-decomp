"""Tests for vftable field naming and slot ABI contracts."""

from __future__ import annotations

from types import SimpleNamespace

from wiz8decomp.vftable_typing import (
    _apply_vftable_typing_row,
    _field_name,
    _function_signature_source_backed,
    _points_to_function_definition,
    _read_slots,
    _retarget_class_vfptr,
    _simple_name,
    _slot_definitions_match_callees,
    _vftable_paths,
    annotate_slot_fd_sources,
)


def test_simple_and_field_names() -> None:
    assert _simple_name("ns::W8Monster") == "W8Monster"
    assert _field_name(0, None) == "slot_00"
    assert _field_name(1, "W8Monster::CanEnterCycle") == "CanEnterCycle_01"
    assert (
        _field_name(0, "W8Monster::'scalar_deleting_destructor'") == "scalar_deleting_destructor_00"
    )
    assert _field_name(2, "W8OptionsSlider::~W8OptionsSlider") == "dtor_W8OptionsSlider_02"


def test_vftable_paths_are_namespace_safe() -> None:
    category, name, path, sigs = _vftable_paths("ns::W8Monster")
    assert category == "/wiz8/vftables/ns"
    assert name == "W8Monster_vftable"
    assert path == "/wiz8/vftables/ns/W8Monster_vftable"
    assert sigs == "/wiz8/vftables/ns/W8Monster_sigs"

    category, name, path, sigs = _vftable_paths("W8Monster")
    assert category == "/wiz8/vftables"
    assert path == "/wiz8/vftables/W8Monster_vftable"
    assert sigs == "/wiz8/vftables/W8Monster_sigs"

    left = _vftable_paths("alpha::Shared")
    right = _vftable_paths("beta::Shared")
    assert left[2] != right[2]


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


def test_read_slots_preserves_census_length_with_unresolved_middle() -> None:
    targets = [0x401000, 0x402000, 0x403000]

    class _Memory:
        def contains(self, _entry) -> bool:
            return True

        def getInt(self, entry) -> int:
            index = (int(entry.getOffset()) - 0x500000) // 4
            return targets[index]

    class _Space:
        def getAddress(self, value: int):
            return SimpleNamespace(getOffset=lambda: value, value=value)

    class _Functions:
        def getFunctionAt(self, entry):
            target = entry.value if hasattr(entry, "value") else entry.getOffset()
            if target == 0x402000:
                return None
            return SimpleNamespace(
                getName=lambda _q=True: f"fn_{target:x}",
                getPrototypeString=lambda _a, _b: "void fn(void)",
            )

    program = SimpleNamespace(
        getMemory=lambda: _Memory(),
        getAddressFactory=lambda: SimpleNamespace(getDefaultAddressSpace=lambda: _Space()),
        getFunctionManager=lambda: _Functions(),
    )
    slots = _read_slots(program, 0x500000, max_slots=3, stop_before=None)
    assert len(slots) == 3
    assert slots[0]["target"] == "0x00401000"
    assert slots[1]["unresolved"] is True
    assert slots[1]["target"] == "0x00402000"
    assert slots[1]["name"] is None
    assert slots[2]["target"] == "0x00403000"
    assert "unresolved" not in slots[2]


def test_annotate_slot_prefers_source_declaration() -> None:
    declaration = SimpleNamespace(
        return_type="void",
        parameter_types=("int",),
        calling_convention="__thiscall",
        has_this=True,
        owning_class="W8Monster",
        is_variadic=False,
    )
    marker = SimpleNamespace(marker_kind="FUNCTION", declaration=declaration)

    class _Space:
        def getAddress(self, value: int):
            return SimpleNamespace(value=value)

    live = SimpleNamespace(getSignatureSource=lambda: SimpleNamespace(name="ANALYSIS"))
    program = SimpleNamespace(
        getAddressFactory=lambda: SimpleNamespace(getDefaultAddressSpace=lambda: _Space()),
        getFunctionManager=lambda: SimpleNamespace(getFunctionAt=lambda _a: live),
    )
    slots = [{"index": 0, "target": "0x00401000", "name": "W8Monster::CanEnterCycle"}]
    annotate_slot_fd_sources(program, slots, {0x401000: marker})
    assert slots[0]["fd_source"] == "source-declaration"
    assert slots[0]["declaration"]["owning_class"] == "W8Monster"
    assert slots[0]["declaration"]["parameter_types"] == ["int"]


def test_annotate_slot_falls_back_to_callee_implementation() -> None:
    class _Space:
        def getAddress(self, value: int):
            return SimpleNamespace(value=value)

    live = SimpleNamespace(getSignatureSource=lambda: SimpleNamespace(name="ANALYSIS"))
    program = SimpleNamespace(
        getAddressFactory=lambda: SimpleNamespace(getDefaultAddressSpace=lambda: _Space()),
        getFunctionManager=lambda: SimpleNamespace(getFunctionAt=lambda _a: live),
    )
    slots = [{"index": 0, "target": "0x00401000", "name": "fn"}]
    annotate_slot_fd_sources(program, slots, {})
    assert slots[0]["fd_source"] == "callee-implementation"
    assert "declaration" not in slots[0]


def test_annotate_slot_treats_imported_live_signature_as_source() -> None:
    class _Space:
        def getAddress(self, value: int):
            return SimpleNamespace(value=value)

    live = SimpleNamespace(getSignatureSource=lambda: SimpleNamespace(name="IMPORTED"))
    program = SimpleNamespace(
        getAddressFactory=lambda: SimpleNamespace(getDefaultAddressSpace=lambda: _Space()),
        getFunctionManager=lambda: SimpleNamespace(getFunctionAt=lambda _a: live),
    )
    slots = [{"index": 0, "target": "0x00401000", "name": "fn"}]
    annotate_slot_fd_sources(program, slots, {})
    assert slots[0]["fd_source"] == "source-declaration"
    assert _function_signature_source_backed(live) is True


def test_slot_definitions_match_callees_rejects_stale_fd(monkeypatch) -> None:
    class _FD:
        def getArguments(self):
            return []

        def getReturnType(self):
            return SimpleNamespace(getPathName=lambda: "/void", getName=lambda: "void")

        def getCallingConvention(self):
            return "__thiscall"

    class _Pointer:
        def getDataType(self):
            return _FD()

    component = SimpleNamespace(getDataType=lambda: _Pointer())
    structure = SimpleNamespace(getDefinedComponents=lambda: [component])
    slots = [{"index": 0, "target": "0x00401000"}]
    live = SimpleNamespace()

    class _Space:
        def getAddress(self, value: int):
            return SimpleNamespace(value=value)

    program = SimpleNamespace(
        getAddressFactory=lambda: SimpleNamespace(getDefaultAddressSpace=lambda: _Space()),
        getFunctionManager=lambda: SimpleNamespace(getFunctionAt=lambda _a: live),
    )
    monkeypatch.setattr(
        "wiz8decomp.callback_typing.definition_matches_function",
        lambda _definition, _function: False,
    )
    assert _slot_definitions_match_callees(program, structure, slots) is False

    monkeypatch.setattr(
        "wiz8decomp.callback_typing.definition_matches_function",
        lambda _definition, _function: True,
    )
    assert _slot_definitions_match_callees(program, structure, slots) is True


def test_apply_refuses_census_rows_with_unresolved_slots() -> None:
    row = {
        "action": "create-and-apply",
        "extent_source": "census",
        "class": "W8Monster",
        "address": "0x00500000",
        "vftable": "/wiz8/vftables/W8Monster_vftable",
        "slots": [
            {"index": 0, "target": "0x00401000", "name": "a"},
            {"index": 1, "target": "0x00402000", "name": None, "unresolved": True},
        ],
    }
    result = _apply_vftable_typing_row(object(), row)
    assert result.get("error") == "census-slot-unresolved"


def test_apply_vftable_typing_skips_unresolved_census_rows(monkeypatch) -> None:
    from wiz8decomp.vftable_typing import apply_vftable_typing

    plan = {
        "vftables": [
            {
                "action": "create-and-apply",
                "extent_source": "census",
                "class": "W8Monster",
                "address": "0x00500000",
                "vftable": "/wiz8/vftables/W8Monster_vftable",
                "slots": [
                    {"index": 0, "target": "0x00401000", "name": "a"},
                    {"index": 1, "target": "0x00402000", "name": None, "unresolved": True},
                ],
            }
        ]
    }
    called: list[object] = []

    def _fake_apply_rows(program, rows, apply_one, **_kwargs):
        called.extend(rows)
        return {"applied": 0, "errors": [], "rows": []}

    monkeypatch.setattr("wiz8decomp.ghidra.mutations.apply_rows", _fake_apply_rows)
    result = apply_vftable_typing(object(), plan)
    assert called == []
    assert result["applied"] == 0
    assert result["errors"] == []
    assert len(result["skipped"]) == 1
    assert result["skipped"][0]["skipped"] == "census-slot-unresolved"


def test_retarget_class_vfptr_uses_bound_structure(monkeypatch) -> None:
    import sys
    import types

    replaced: list[tuple] = []

    class _Component:
        def getFieldName(self) -> str:
            return "vfptr"

        def getOffset(self) -> int:
            return 0

        def getComment(self):
            return None

    class _Structure:
        def getDefinedComponents(self):
            return [_Component()]

        def replaceAtOffset(self, offset, pointer, length, field, comment) -> None:
            replaced.append((offset, pointer, length, field, comment))

    bound = _Structure()
    monkeypatch.setattr(
        "wiz8decomp.class_binding.find_ghidra_class",
        lambda _program, _name: object(),
    )
    monkeypatch.setattr(
        "wiz8decomp.class_binding.find_class_structure",
        lambda _program, _gc: bound,
    )

    data = types.ModuleType("ghidra.program.model.data")

    class _PointerDataType:
        def __init__(self, vftable, manager):
            self.vftable = vftable
            self.manager = manager

    data.PointerDataType = _PointerDataType
    monkeypatch.setitem(sys.modules, "ghidra", types.ModuleType("ghidra"))
    monkeypatch.setitem(sys.modules, "ghidra.program", types.ModuleType("ghidra.program"))
    monkeypatch.setitem(
        sys.modules, "ghidra.program.model", types.ModuleType("ghidra.program.model")
    )
    monkeypatch.setitem(sys.modules, "ghidra.program.model.data", data)

    program = SimpleNamespace(getDataTypeManager=lambda: object())
    assert _retarget_class_vfptr(program, "W8Monster", object()) is True
    assert len(replaced) == 1
    assert replaced[0][3] == "vfptr"


def test_retarget_class_vfptr_returns_false_without_binding(monkeypatch) -> None:
    monkeypatch.setattr(
        "wiz8decomp.class_binding.find_ghidra_class",
        lambda _program, _name: None,
    )
    assert _retarget_class_vfptr(object(), "W8Monster", object()) is False
