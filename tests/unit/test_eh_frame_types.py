from __future__ import annotations

from pathlib import Path

from wiz8decomp.ghidra.eh_frame_types import (
    ghidra_stack_offset,
    import_destructor_class,
    plan_frame_slots,
    variable_name,
)

PROGRAM = "wiz8--test--wiz8--0000"


def test_import_destructor_class_strips_access_and_convention() -> None:
    assert (
        import_destructor_class("public: __thiscall srStringTable::~srStringTable(void)")
        == "srStringTable"
    )
    assert (
        import_destructor_class("public: virtual __thiscall srBinStream::~srBinStream(void)")
        == "srBinStream"
    )


def test_import_destructor_class_keeps_nested_and_template_names() -> None:
    assert (
        import_destructor_class("public: __thiscall srHuffman::Decompressor::~Decompressor(void)")
        == "srHuffman::Decompressor"
    )
    assert (
        import_destructor_class("__thiscall srVector3T<float>::~srVector3T<float>(void)")
        == "srVector3T<float>"
    )
    assert import_destructor_class("not a destructor at all") is None


def test_variable_name_is_listing_safe() -> None:
    assert variable_name("srHuffman::Decompressor", -1320) == "eh_Decompressor_528"
    assert variable_name("srVector3T<float>", -16) == "eh_srVector3T_float_10"


def test_ghidra_stack_offset_covers_both_vc6_frame_shapes() -> None:
    # Frameless EH shape: the state push is the first instruction, depth 0.
    assert ghidra_stack_offset(0, -56) == -56
    # Classic push-ebp frame: the state push runs at depth -4.
    assert ghidra_stack_offset(-4, -4) == -8


def _write(path: Path, header: str, rows: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join([header, *rows]) + "\n", encoding="utf-8")


def _fixture(tmp_path: Path, unwind_rows: list[str]) -> Path:
    snapshots = tmp_path / "evidence" / "snapshots"
    _write(
        snapshots / "eh-metadata" / "functions.csv",
        "program,funcinfo,magic,handler_thunk,frame_setup,eh_setup_start,"
        "max_state,try_block_count,unwind_signature",
        [f"{PROGRAM},005f0000,19930520,005e0000,00410010,00410000,2,0,abcd"],
    )
    _write(
        snapshots / "eh-metadata" / "unwind.csv",
        "program,funcinfo,state,to_state,kind,frame_offset,target,import_slot,"
        "import_name,import_signature,element_destructor",
        unwind_rows,
    )
    for relative, header in [
        (
            "call-sites/assertions.csv",
            "program,call_site,call_kind,function_start,source_path,line,expression,message",
        ),
        ("call-sites/runtime-class-names.csv", "program,call_site,call_kind,function_start,name"),
        (
            "polymorphism/vtables.csv",
            "program,address,section,kind,slot_count,boundary,vptr_write_count,subobject_offsets,pure_virtual_slots,adjustor_thunk_slots,import_slots",
        ),
        (
            "polymorphism/slots.csv",
            "program,vtable,slot_index,target,kind,import_name,import_signature,adjust,thunk_target",
        ),
        ("polymorphism/vptr-writes.csv", "program,site,function_start,object_offset,vtable"),
        (
            "globals/globals.csv",
            "program,address,section,storage,kind,reference_count,function_count,access_kinds,widths,extent_upper,extent_bytes,preview",
        ),
    ]:
        _write(snapshots / relative, header, [])
    reviewed = tmp_path / "evidence" / "reviewed" / "wiz8"
    _write(
        reviewed / "classes.csv",
        "program,class_name,confidence,primary_vtable_id,constructor,destructor,"
        "scalar_deleting_destructor,minimum_size,base_classes,base_name_origin,"
        "source_path,evidence,layout_proof",
        ["wiz8,Controls,exact,,004f0000,004f3000,004f3100,0x40,,,,ev,proven"],
    )
    _write(
        reviewed / "functions.csv",
        "program,address,size,current_name,provisional_name,owner,confidence,"
        "name_origin,authority,aliases,fid_variants,evidence,source_path,"
        "source_line,relocation_masked_sha256",
        [
            "wiz8,004f8000,32,FUN_004f8000,W8Thing::~W8Thing,wiz8-ui,exact,descriptive,descriptive,,,ev,,,"
        ],
    )
    return tmp_path


def test_plan_joins_imports_and_reviewed_destructors(tmp_path: Path) -> None:
    root = _fixture(
        tmp_path,
        [
            (
                f"{PROGRAM},005f0000,0,-1,object-import,-44,,,x,"
                '"public: __thiscall srStringTable::~srStringTable(void)",'
            ),
            f"{PROGRAM},005f0000,1,-1,pointer,-52,004f3000,,,,",
            f"{PROGRAM},005f0000,2,-1,object,-96,004f8000,,,,",
        ],
    )
    report = plan_frame_slots(PROGRAM, root)

    assert sorted(plan.class_name for plan in report.plans) == [
        "Controls",
        "W8Thing",
        "srStringTable",
    ]
    by_class = {plan.class_name: plan for plan in report.plans}
    assert by_class["srStringTable"].type_source == "library-import"
    assert not by_class["srStringTable"].is_pointer
    assert by_class["Controls"].is_pointer
    assert by_class["Controls"].type_source == "reviewed-class"
    assert by_class["W8Thing"].destructor == 0x4F8000
    assert report.skipped_unresolved_destructor == 0


def test_plan_skips_reused_and_unresolved_slots(tmp_path: Path) -> None:
    root = _fixture(
        tmp_path,
        [
            # One slot, two different resolved classes across states: reused.
            f"{PROGRAM},005f0000,0,-1,pointer,-52,004f3000,,,,",
            f"{PROGRAM},005f0000,1,-1,object,-52,004f8000,,,,",
            # Resolved class sharing a slot with an unknown destructor: reused.
            f"{PROGRAM},005f0000,2,-1,pointer,-60,004f3000,,,,",
            f"{PROGRAM},005f0000,3,-1,pointer,-60,00499999,,,,",
            # Unknown destructor alone: unresolved, never planned.
            f"{PROGRAM},005f0000,4,-1,object,-70,00488888,,,,",
        ],
    )
    report = plan_frame_slots(PROGRAM, root)

    assert report.plans == ()
    assert report.skipped_conflicting_reuse == 3
    assert report.skipped_unresolved_destructor == 2
    assert dict(report.unresolved_destructor_counts) == {
        "0x00499999": 1,
        "0x00488888": 1,
    }


def test_canonical_snapshots_produce_a_nonempty_clean_plan() -> None:
    repo = Path(__file__).resolve().parents[2]
    report = plan_frame_slots("wiz8--gog-base--wiz8--18a74ff61c65", repo)

    assert len(report.plans) >= 20
    for plan in report.plans:
        assert plan.frame_offset < 0
        assert plan.class_name
        assert " " not in plan.class_name.split("::")[-1]
        assert plan.variable_name.startswith("eh_")
    # The single most valuable unreviewed destructor should stay visible to
    # reviewers: it dominates the unresolved counts.
    top = dict(report.unresolved_destructor_counts)
    assert max(top.values()) >= 100
