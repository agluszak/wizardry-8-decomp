"""Tests for vftable field naming and slot ABI contracts."""

from __future__ import annotations

from types import SimpleNamespace

from wiz8decomp.datatype_contracts import function_definition_contract
from wiz8decomp.vftable_typing import (
    _apply_vftable_typing_row,
    _field_name,
    _function_signature_source_backed,
    _points_to_function_definition,
    _read_slots,
    _retarget_class_vfptr,
    _simple_name,
    _subobject_view_paths,
    _vftable_paths,
    annotate_slot_fd_sources,
    construction_vtable_attachments,
    lifecycle_class_name,
    unique_receiver_offset,
)


def test_simple_and_field_names() -> None:
    assert _simple_name("ns::W8Monster") == "W8Monster"
    assert _field_name(0, None) == "slot_00"
    assert _field_name(1, "W8Monster::CanEnterCycle") == "CanEnterCycle_01"
    assert (
        _field_name(0, "W8Monster::'scalar_deleting_destructor'") == "scalar_deleting_destructor_00"
    )
    assert _field_name(2, "W8OptionsSlider::~W8OptionsSlider") == "dtor_W8OptionsSlider_02"


def test_subobject_view_paths() -> None:
    category, name, path = _subobject_view_paths("W8Monster", "W8Navigator", 0x20)
    assert category == "/wiz8/subobjects/W8Monster"
    assert name == "W8Navigator_at_0x20"
    assert path == "/wiz8/subobjects/W8Monster/W8Navigator_at_0x20"
    category, name, path = _subobject_view_paths("ns::Derived", "Base", 4)
    assert category == "/wiz8/subobjects/ns/Derived"
    assert path == "/wiz8/subobjects/ns/Derived/Base_at_0x4"


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

    category, name, path, sigs = _vftable_paths("W8Monster", base_class="W8Navigator")
    assert name == "W8Monster_vftable_for_W8Navigator"
    assert path == "/wiz8/vftables/W8Monster_vftable_for_W8Navigator"
    assert sigs == "/wiz8/vftables/W8Monster_sigs_for_W8Navigator"

    category, name, path, sigs = _vftable_paths("W8TriggerActionData", phase="construction")
    assert name == "W8TriggerActionData_vftable_ctor"
    assert path == "/wiz8/vftables/W8TriggerActionData_vftable_ctor"
    assert sigs == "/wiz8/vftables/W8TriggerActionData_sigs_ctor"

    category, name, path, sigs = _vftable_paths(
        "W8Monster", base_class="W8Navigator", phase="construction"
    )
    assert name == "W8Monster_vftable_for_W8Navigator_ctor"
    assert path == "/wiz8/vftables/W8Monster_vftable_for_W8Navigator_ctor"

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


def test_function_definition_contract_ignores_parameter_names() -> None:
    class _Arg:
        def __init__(self, name: str, data_type: object) -> None:
            self._name = name
            self._dt = data_type

        def getName(self) -> str:
            return self._name

        def getDataType(self):
            return self._dt

    dt = SimpleNamespace(getPathName=lambda: "/int", getLength=lambda: 4, getName=lambda: "int")
    left = SimpleNamespace(
        getReturnType=lambda: dt,
        getArguments=lambda: [_Arg("param_0", dt)],
        getCallingConvention=lambda: "__thiscall",
        hasVarArgs=lambda: False,
        hasNoReturn=lambda: False,
        getLength=lambda: 0,
    )
    right = SimpleNamespace(
        getReturnType=lambda: dt,
        getArguments=lambda: [_Arg("this", dt)],
        getCallingConvention=lambda: "__thiscall",
        hasVarArgs=lambda: False,
        hasNoReturn=lambda: False,
        getLength=lambda: 0,
    )
    assert function_definition_contract(left) == function_definition_contract(right)


def test_apply_skips_unresolved_source_type_rows(monkeypatch) -> None:
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
                    {
                        "index": 0,
                        "target": "0x00401000",
                        "declaration": {"return_type": "MissingType"},
                        "contract_error": "unresolved-source-type:MissingType",
                    }
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
    assert result["skipped"][0]["skipped"] == "unresolved-source-type"


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


def test_lifecycle_class_name_accepts_ctor_and_dtor() -> None:
    assert lifecycle_class_name("W8TriggerActionData::W8TriggerActionData") == "W8TriggerActionData"
    assert lifecycle_class_name("ns::Widget::~Widget") == "ns::Widget"
    assert lifecycle_class_name("Trigger::Run") is None
    assert lifecycle_class_name("W8TriggerActionData::Reset") is None


def test_unique_receiver_offset_requires_one_incoming_value() -> None:
    assert (
        unique_receiver_offset(
            [
                {"receiver_provenance": "incoming-ecx", "receiver_offset": 16},
                {"receiver_provenance": "incoming-ecx-plus-dynamic", "receiver_offset": 16},
            ]
        )
        == 16
    )
    assert (
        unique_receiver_offset(
            [
                {"receiver_provenance": "incoming-ecx", "receiver_offset": 0},
                {"receiver_provenance": "incoming-ecx", "receiver_offset": 16},
            ]
        )
        is None
    )
    assert (
        unique_receiver_offset([{"receiver_provenance": "unknown", "receiver_offset": 0}]) is None
    )


def test_construction_attachments_take_earlier_ctor_and_later_dtor_tables() -> None:
    record = SimpleNamespace(
        qualified_name="W8TriggerActionData",
        vtable_address=0x5EC138,
        base_vtables=(),
    )
    families = [
        {
            "function_source_name": "W8TriggerActionData::W8TriggerActionData",
            "receiver_offset": 0,
            "transitions": [
                {"kind": "vftable", "table": "0x005ec148"},
                {"kind": "vftable", "table": "0x005ec138"},
            ],
        },
        {
            "function_source_name": "W8TriggerActionData::~W8TriggerActionData",
            "receiver_offset": 0,
            "transitions": [
                {"kind": "vftable", "table": "0x005ec138"},
                {"kind": "vftable", "table": "0x005ec148"},
            ],
        },
    ]
    attachments = construction_vtable_attachments(families, [record])
    assert attachments == [
        {
            "class": "W8TriggerActionData",
            "address": 0x5EC148,
            "base_class": None,
            "role": "construction",
            "receiver_offset": 0,
        }
    ]


def test_construction_attachments_keep_marked_support_tables_and_for_clause_base() -> None:
    support = SimpleNamespace(
        qualified_name="srClassSupport<stModelInstance>",
        vtable_address=0x5EC814,
        base_vtables=(),
    )
    derived = SimpleNamespace(
        qualified_name="stModelInstance",
        vtable_address=0x5EC7D0,
        base_vtables=(SimpleNamespace(address=0x5EC7C0, base_class="srModel::Client"),),
    )
    families = [
        {
            "function_source_name": "stModelInstance::stModelInstance",
            "receiver_offset": 0,
            "transitions": [
                {"kind": "vftable", "table": "0x005ec814"},
                {"kind": "vftable", "table": "0x005ec7d0"},
            ],
        },
        {
            "function_source_name": "stModelInstance::stModelInstance",
            "receiver_offset": 4,
            "transitions": [
                {"kind": "vftable", "table": "0x005ec804"},
                {"kind": "vftable", "table": "0x005ec7c0"},
            ],
        },
    ]
    attachments = construction_vtable_attachments(families, [support, derived])
    assert attachments == [
        {
            "class": "stModelInstance",
            "address": 0x5EC804,
            "base_class": "srModel::Client",
            "role": "construction",
            "receiver_offset": 4,
        }
    ]


def test_construction_apply_does_not_retarget_complete_object_vfptr(monkeypatch) -> None:
    called: list[object] = []

    monkeypatch.setattr(
        "wiz8decomp.vftable_typing._build_vftable_structure",
        lambda *_args, **_kwargs: SimpleNamespace(
            getPathName=lambda: "/wiz8/vftables/W8TriggerActionData_vftable_ctor",
            getNumComponents=lambda: 1,
        ),
    )
    monkeypatch.setattr("wiz8decomp.vftable_typing._apply_data", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(
        "wiz8decomp.vftable_typing._vftable_has_function_definitions", lambda _s: True
    )
    monkeypatch.setattr(
        "wiz8decomp.vftable_typing._retarget_class_vfptr",
        lambda *_args, **_kwargs: called.append(True) or True,
    )
    row = {
        "action": "create-and-apply",
        "extent_source": "census",
        "class": "W8TriggerActionData",
        "role": "construction",
        "address": "0x005ec148",
        "vftable": "/wiz8/vftables/W8TriggerActionData_vftable_ctor",
        "subobject_offset": 0,
        "slots": [
            {"index": 0, "target": "0x00401000", "name": "a", "fd_source": "callee-implementation"}
        ],
    }
    result = _apply_vftable_typing_row(object(), row)
    assert result["vfptr_retargeted"] is False
    assert called == []


def test_base_apply_does_not_retarget_complete_object_vfptr(monkeypatch) -> None:
    called: list[int] = []
    views: list[int] = []

    monkeypatch.setattr(
        "wiz8decomp.vftable_typing._build_vftable_structure",
        lambda *_args, **_kwargs: SimpleNamespace(
            getPathName=lambda: "/wiz8/vftables/W8Monster_vftable_for_Base",
            getNumComponents=lambda: 1,
        ),
    )
    monkeypatch.setattr("wiz8decomp.vftable_typing._apply_data", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(
        "wiz8decomp.vftable_typing._vftable_has_function_definitions", lambda _s: True
    )
    monkeypatch.setattr(
        "wiz8decomp.vftable_typing._retarget_class_vfptr",
        lambda _program, _name, _table, *, offset: called.append(offset) or True,
    )
    monkeypatch.setattr(
        "wiz8decomp.vftable_typing.install_derived_base_view",
        lambda _program, *, derived, base, offset, vftable: views.append(offset) or True,
    )
    row = {
        "action": "create-and-apply",
        "extent_source": "census",
        "class": "W8Monster",
        "base_class": "W8Navigator",
        "role": "base",
        "address": "0x00500010",
        "vftable": "/wiz8/vftables/W8Monster_vftable_for_W8Navigator",
        "subobject_offset": 0,
        "slots": [
            {"index": 0, "target": "0x00401000", "name": "a", "fd_source": "callee-implementation"}
        ],
    }
    result = _apply_vftable_typing_row(object(), row)
    assert result["vfptr_retargeted"] is False
    assert result["subobject_view"] is False
    assert called == []
    assert views == []

    row["subobject_offset"] = 16
    result = _apply_vftable_typing_row(object(), row)
    assert result["vfptr_retargeted"] is False
    assert result["subobject_view"] is True
    assert called == []
    assert views == [16]


def test_retarget_class_vfptr_requires_named_field_at_offset(monkeypatch) -> None:
    import sys
    import types

    replaced: list[int] = []

    class _Component:
        def __init__(self, name: str, offset: int) -> None:
            self._name = name
            self._offset = offset

        def getFieldName(self) -> str:
            return self._name

        def getOffset(self) -> int:
            return self._offset

        def getComment(self):
            return None

    class _Structure:
        def getDefinedComponents(self):
            return [_Component("vfptr", 0), _Component("vfptr", 16)]

        def replaceAtOffset(self, offset, _pointer, _length, _field, _comment) -> None:
            replaced.append(offset)

    monkeypatch.setattr(
        "wiz8decomp.class_binding.find_ghidra_class",
        lambda _program, _name: object(),
    )
    monkeypatch.setattr(
        "wiz8decomp.class_binding.find_class_structure",
        lambda _program, _gc: _Structure(),
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
    assert _retarget_class_vfptr(program, "W8VirtualFileBinIStream", object(), offset=16) is True
    assert replaced == [16]
