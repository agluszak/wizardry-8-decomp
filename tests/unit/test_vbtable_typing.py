"""Tests for vbtable path naming, class attachment, and ComponentOffset gating."""

from __future__ import annotations

from types import SimpleNamespace

from wiz8decomp.vbtable_typing import (
    _vbtable_paths,
    apply_existing_vbaseptr_offsets,
    vbtable_class_attachments,
)


def test_vbtable_paths_are_namespace_safe() -> None:
    category, name, path = _vbtable_paths("ns::W8VirtualFileBinIStream")
    assert category == "/wiz8/vbtables/ns"
    assert name == "W8VirtualFileBinIStream_vbtable"
    assert path == "/wiz8/vbtables/ns/W8VirtualFileBinIStream_vbtable"

    category, name, path = _vbtable_paths("W8VirtualFileBinIStream", base_class="srBinIStream")
    assert name == "W8VirtualFileBinIStream_vbtable_for_srBinIStream"
    assert path == "/wiz8/vbtables/W8VirtualFileBinIStream_vbtable_for_srBinIStream"


def test_vbtable_attachments_require_a_marked_vftable_from_the_same_lifecycle() -> None:
    record = SimpleNamespace(
        qualified_name="W8VirtualFileBinIStream",
        vtable_address=0x5EC6A0,
        base_vtables=(SimpleNamespace(address=0x5EC68C, base_class="srBinStream"),),
    )
    vftables = [
        {
            "address": "0x005ec6a0",
            "writes": [
                {
                    "function": "0x0047cc00",
                    "function_source_name": "W8VirtualFileBinIStream::W8VirtualFileBinIStream",
                    "receiver_provenance": "incoming-ecx",
                    "receiver_offset": 0,
                    "object_offset": 0,
                }
            ],
        }
    ]
    vbtables = [
        {
            "address": "0x005ec6a8",
            "entries": [{"displacement": -4}, {"displacement": 12}],
            "writes": [
                {
                    "function": "0x0047cc00",
                    "function_source_name": "W8VirtualFileBinIStream::W8VirtualFileBinIStream",
                    "receiver_provenance": "incoming-ecx",
                    "receiver_offset": 4,
                    "object_offset": 4,
                }
            ],
        }
    ]
    attachments = vbtable_class_attachments(
        vbtables=vbtables,
        vftables=vftables,
        families=[],
        classes=[record],
    )
    assert attachments == [
        {
            "class": "W8VirtualFileBinIStream",
            "address": 0x5EC6A8,
            "base_class": None,
            "role": "vbtable",
            "vbptr_offset": 4,
            "entries": [-4, 12],
        }
    ]

    orphan = vbtable_class_attachments(
        vbtables=vbtables,
        vftables=[],
        families=[],
        classes=[record],
    )
    assert orphan == []


def test_construction_vbtable_keeps_final_table_as_the_vbptr_target() -> None:
    record = SimpleNamespace(
        qualified_name="Widget",
        vtable_address=0x1000,
        base_vtables=(),
    )
    vftables = [
        {
            "address": "0x00001000",
            "writes": [
                {
                    "function": "0x2000",
                    "function_source_name": "Widget::Widget",
                    "receiver_provenance": "incoming-ecx",
                    "receiver_offset": 0,
                    "object_offset": 0,
                }
            ],
        }
    ]
    vbtables = [
        {
            "address": "0x00003000",
            "entries": [{"displacement": -4}, {"displacement": 8}],
            "writes": [
                {
                    "function": "0x2000",
                    "function_source_name": "Widget::Widget",
                    "receiver_provenance": "incoming-ecx",
                    "receiver_offset": 4,
                    "object_offset": 4,
                }
            ],
        },
        {
            "address": "0x00003008",
            "entries": [{"displacement": -4}, {"displacement": 12}],
            "writes": [
                {
                    "function": "0x2000",
                    "function_source_name": "Widget::Widget",
                    "receiver_provenance": "incoming-ecx",
                    "receiver_offset": 4,
                    "object_offset": 4,
                }
            ],
        },
    ]
    families = [
        {
            "function_source_name": "Widget::Widget",
            "receiver_offset": 4,
            "transitions": [
                {"kind": "vbtable", "table": "0x00003000"},
                {"kind": "vbtable", "table": "0x00003008"},
            ],
        }
    ]
    attachments = vbtable_class_attachments(
        vbtables=vbtables,
        vftables=vftables,
        families=families,
        classes=[record],
    )
    by_address = {row["address"]: row["role"] for row in attachments}
    assert by_address[0x3000] == "construction"
    assert by_address[0x3008] == "vbtable"


def test_component_offset_only_touches_existing_vbaseptr_typedefs() -> None:
    set_offsets: list[int] = []

    class _Typedef:
        def __init__(self, name: str, current: int | None = None) -> None:
            self._name = name
            self._offset = current

        def getName(self) -> str:
            return self._name

        def getComponentOffset(self):
            return self._offset

        def setComponentOffset(self, value: int) -> None:
            self._offset = value
            set_offsets.append(value)

    class _PlainPointer:
        def getName(self) -> str:
            return "undefined *"

    class _Component:
        def __init__(self, name: str, offset: int, data_type: object) -> None:
            self._name = name
            self._offset = offset
            self._dt = data_type

        def getFieldName(self) -> str:
            return self._name

        def getOffset(self) -> int:
            return self._offset

        def getDataType(self):
            return self._dt

    vbase = _Typedef("VBasePtr")
    structure = SimpleNamespace(
        getDefinedComponents=lambda: [
            _Component("vbptr", 4, _PlainPointer()),
            _Component("o_srBinStream", 16, vbase),
            _Component("unknown_014", 20, _PlainPointer()),
        ]
    )
    applied = apply_existing_vbaseptr_offsets(structure, vbptr_offset=4, entries=[-4, 12])
    assert applied == 1
    assert set_offsets == [16]
    assert apply_existing_vbaseptr_offsets(structure, vbptr_offset=4, entries=[-4, 12]) == 0
