"""The command-scoped evidence and type-spec views."""

from pathlib import Path

from wiz8decomp.ghidra.evidence_index import load_evidence_index, reviewed_owner
from wiz8decomp.ghidra.type_specs import parse_type_spec

REPOSITORY = Path(__file__).resolve().parents[2]


def test_evidence_index_joins_reviewed_classes_vtables_and_functions() -> None:
    index = load_evidence_index(REPOSITORY)

    assert index.vtables_by_id["GrCycle.primary"].class_name == "GrCycle"
    assert index.slots_by_vtable["GrCycle.primary"][9].target == 0x005E1D9A
    assert index.class_for_lifecycle_function[0x004A5E50].name == "GrCycle"
    assert reviewed_owner(index, 0x004A8460) == "GrCycle"
    assert "srNode" in index.imported_types


def test_one_type_spec_grammar_covers_pointer_array_and_function_types() -> None:
    pointer = parse_type_spec("const srNode *")
    array = parse_type_spec("unsigned short[4]")
    function = parse_type_spec("function:GrCycle.primary.slot9")

    assert (pointer.kind, pointer.target.name) == ("pointer", "srNode")
    assert (array.kind, array.count, array.target.name) == ("array", 4, "unsigned short")
    assert (function.kind, function.name) == ("function", "GrCycle.primary.slot9")
