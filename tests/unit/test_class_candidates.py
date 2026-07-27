from wiz8decomp.reports.class_candidates import (
    _push_before_allocator,
    classify_candidates,
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
    }
    row.update(overrides)
    return row


def test_classify_separates_deleting_destructor_from_constructors() -> None:
    vtables = [_vtable("00500000"), _vtable("00500100", kind="code-pointer-table")]
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
    assert candidate["reviewed"] is False


def test_classify_skips_vtables_without_primary_writers() -> None:
    vtables = [_vtable("00500000")]
    writes = [
        {"site": "00400110", "function_start": "00400100", "object_offset": "0x18", "vtable": "00500000"},
    ]
    assert classify_candidates(vtables, [], writes, set()) == []


def test_push_before_allocator_reads_the_allocation_size() -> None:
    window_va = 0x00401000
    # push 0x250; call allocator(rel +0x100)
    window = b"\x68\x50\x02\x00\x00" + b"\xe8" + (0x100).to_bytes(4, "little")
    allocator = window_va + 10 + 0x100
    assert _push_before_allocator(window, window_va, {allocator}) == 0x250
    assert _push_before_allocator(window, window_va, {allocator + 4}) is None
