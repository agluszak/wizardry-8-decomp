from wiz8decomp.ghidra.candidate_model import (
    classify_candidates,
    derive_skeletons,
    push_before_allocator,
)


def _vtable(address: str, **overrides: str) -> dict[str, str]:
    row = {
        "program": "wiz8",
        "address": address,
        "section": ".rdata",
        "kind": "vftable",
        "slot_count": "4",
        "boundary": "code-reference-boundary",
        "vptr_write_count": "2",
        "subobject_offsets": "0x0",
        "pure_virtual_slots": "0",
        "adjustor_thunk_slots": "0",
        "import_slots": "0",
        "allocation_sizes": "",
    }
    row.update(overrides)
    return row


def test_classify_separates_deleting_destructor_from_constructors() -> None:
    vtables = [
        _vtable("00500000", allocation_sizes="0x250"),
        _vtable("00500100", kind="code-pointer-table"),
    ]
    slots = [
        {"vtable": "00500000", "slot_index": "0", "target": "00400100"},
        {"vtable": "00500000", "slot_index": "1", "target": "00400f00"},
    ]
    writes = [
        # The slot 0 target writes the vtable during destruction.
        {"site": "00400110", "function_start": "00400100", "object_offset": "0x0", "vtable": "00500000"},
        # A second writer is the constructor; it also installs a subobject
        # table at +0x18.
        {"site": "00400210", "function_start": "00400200", "object_offset": "0x0", "vtable": "00500000"},
        {"site": "00400220", "function_start": "00400200", "object_offset": "0x18", "vtable": "00500200"},
    ]
    candidates = classify_candidates(vtables, slots, writes, {0x00500100})

    assert len(candidates) == 1
    candidate = candidates[0]
    assert candidate["vtable"] == 0x00500000
    assert candidate["scalar_deleting_destructor"] == 0x00400100
    assert candidate["constructor_or_destructor"] == [0x00400200]
    assert candidate["co_installed_vtables"] == [(0x00500200, 0x18)]
    assert candidate["subobject_vtables"] == [(0x00500200, 0x18)]
    assert candidate["allocation_sizes"] == [0x250]
    assert candidate["reviewed"] is False


def test_classify_skips_vtables_without_primary_writers() -> None:
    vtables = [_vtable("00500000")]
    writes = [
        {"site": "00400110", "function_start": "00400100", "object_offset": "0x18", "vtable": "00500000"},
    ]
    assert classify_candidates(vtables, [], writes, set()) == []


def test_skeletons_cover_vptrs_and_allocation_hints() -> None:
    candidates = [
        {
            "vtable": 0x00500000,
            "allocation_sizes": [0x250],
            "subobject_vtables": [(0x00500200, 0x138)],
            "reviewed": False,
        },
        # No allocation hint: the size falls back to covering the last vptr.
        {
            "vtable": 0x00500400,
            "allocation_sizes": [],
            "subobject_vtables": [(0x00500500, 0x18)],
            "reviewed": False,
        },
        # An allocation hint smaller than the vptr extent cannot be the
        # object size; the vptr extent wins.
        {
            "vtable": 0x00500600,
            "allocation_sizes": [0x8],
            "subobject_vtables": [(0x00500700, 0x20)],
            "reviewed": False,
        },
        # Reviewed candidates belong to the reviewed class model.
        {
            "vtable": 0x00500800,
            "allocation_sizes": [0x10],
            "subobject_vtables": [],
            "reviewed": True,
        },
    ]
    skeletons = derive_skeletons(candidates)

    assert [item["name"] for item in skeletons] == [
        "Candidate_00500000",
        "Candidate_00500400",
        "Candidate_00500600",
    ]
    first = skeletons[0]
    assert first["size"] == 0x250
    assert first["size_is_allocation_hint"] is True
    assert first["vptr_offsets"] == [(0, 0x00500000), (0x138, 0x00500200)]
    assert skeletons[1]["size"] == 0x1C
    assert skeletons[1]["size_is_allocation_hint"] is False
    assert skeletons[2]["size"] == 0x24


def test_push_before_allocator_reads_the_allocation_size() -> None:
    window_va = 0x00401000
    # push 0x250; call allocator(rel +0x100)
    window = b"\x68\x50\x02\x00\x00" + b"\xe8" + (0x100).to_bytes(4, "little")
    allocator = window_va + 10 + 0x100
    assert push_before_allocator(window, window_va, {allocator}) == 0x250
    assert push_before_allocator(window, window_va, {allocator + 4}) is None
