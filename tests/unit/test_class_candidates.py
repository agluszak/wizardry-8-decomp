from wiz8decomp.ghidra.candidate_model import (
    classify_candidates,
    derive_skeletons,
    derived_families,
    push_before_allocator,
    teardown_writers,
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


def test_derived_families_orders_the_pair_by_construction_and_ranks_by_writer_size() -> None:
    """The later store is the derived class, and a small writer is portable.

    A big writer installing the same two tables is a heap builder that happens
    to construct two objects, which is a different finding from a class.
    """

    writes = [
        # One dedicated constructor: base first, derived second.
        {"site": "0042a26b", "function_start": "0042a260", "object_offset": "0x0", "vtable": "005ebfb8"},
        {"site": "0042a298", "function_start": "0042a260", "object_offset": "0x0", "vtable": "005ebfb4"},
        # A far larger body installing another pair.
        {"site": "00473300", "function_start": "00473260", "object_offset": "0x0", "vtable": "005ec50c"},
        {"site": "00473400", "function_start": "00473260", "object_offset": "0x0", "vtable": "005ec508"},
        # A subobject store is not a derivation.
        {"site": "0042a2a8", "function_start": "0042a260", "object_offset": "0x10", "vtable": "005ec000"},
    ]
    slot_counts = {0x005EBFB8: 1, 0x005EBFB4: 1, 0x005EC50C: 1, 0x005EC508: 1, 0x005EC000: 1}
    families = derived_families(writes, slot_counts, {0x0042A260: 84, 0x00473260: 1130})

    assert [family["writer"] for family in families] == [0x0042A260, 0x00473260]
    assert families[0]["base_vtable"] == 0x005EBFB8
    assert families[0]["derived_vtable"] == 0x005EBFB4
    assert families[0]["writer_size"] == 84

    # A writer whose size the census cannot state sorts last rather than first.
    unsized = derived_families(writes, slot_counts, {0x00473260: 1130})
    assert [family["writer"] for family in unsized] == [0x00473260, 0x0042A260]


def test_derived_families_ignores_a_table_with_more_than_one_slot() -> None:
    writes = [
        {"site": "0042a26b", "function_start": "0042a260", "object_offset": "0x0", "vtable": "005ebfb8"},
        {"site": "0042a298", "function_start": "0042a260", "object_offset": "0x0", "vtable": "005ebfb4"},
    ]
    assert derived_families(writes, {0x005EBFB8: 1, 0x005EBFB4: 6}) == []


def test_derived_families_inverts_the_pair_for_a_destructor_writer() -> None:
    """A destructor stores its own table first and the base's last.

    Reading one as a constructor inverts the hierarchy, so a body known to be a
    teardown body has to be ordered the other way round. 0x00443750 is the real
    case: it stores 0x005EC158, releases a member, then stores 0x005EC138, so
    0x005EC158 is the derived class and not the base.
    """

    writes = [
        {"site": "00443756", "function_start": "00443750", "object_offset": "0x0", "vtable": "005ec158"},
        {"site": "0044376a", "function_start": "00443750", "object_offset": "0x0", "vtable": "005ec138"},
    ]
    slot_counts = {0x005EC158: 1, 0x005EC138: 1}

    as_constructor = derived_families(writes, slot_counts)[0]
    assert as_constructor["writer_role"] == "constructor"
    assert as_constructor["base_vtable"] == 0x005EC158

    as_destructor = derived_families(writes, slot_counts, destructor_writers={0x00443750})[0]
    assert as_destructor["writer_role"] == "destructor"
    assert as_destructor["base_vtable"] == 0x005EC138
    assert as_destructor["derived_vtable"] == 0x005EC158


def test_teardown_writers_is_relative_to_the_table_being_written() -> None:
    """A constructor called from some unrelated teardown path is not a destructor.

    0x004A5C30 constructs its class and is byte-exact as a constructor, but a
    deleting destructor elsewhere calls it. A global "called by any deleting
    destructor" test labels it a teardown body and inverts its hierarchy.
    """

    writes = [
        # The real teardown body: reached from its own table's slot 0.
        {"site": "00443756", "function_start": "00443750", "object_offset": "0x0", "vtable": "005ec158"},
        # A constructor, called by a deleting destructor of some other class.
        {"site": "004a5c36", "function_start": "004a5c30", "object_offset": "0x0", "vtable": "005ece64"},
    ]
    slots = [
        {"vtable": "005ec158", "slot_index": "0", "target": "00443730"},
        {"vtable": "005ece64", "slot_index": "0", "target": "004a5cf0"},
    ]
    calls = [
        {"caller": "00443730", "callee": "00443750"},
        {"caller": "004a2d80", "callee": "004a5c30"},
    ]
    assert teardown_writers(writes, slots, calls) == {0x00443750}
