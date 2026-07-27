import csv
import hashlib
from pathlib import Path

import pytest
from wiz8decomp.boundaries import (
    BoundariesDisagree,
    masked_digest,
    resolve_boundary_function,
    symbol_candidates,
    verify_boundaries,
)
from wiz8decomp.sgp_oracle import CoffFunction

REPOSITORY = Path(__file__).resolve().parents[2]
MAPPING = REPOSITORY / "config/reccmp/wiz8-gameplay-boundaries.csv"


def test_a_decorated_method_is_offered_under_the_names_a_review_may_use() -> None:
    # Reviews name the method against the logical class, the implementation
    # class carries the W8 prefix, and a few helpers are reviewed bare.
    assert symbol_candidates("?SetBehaviour@W8GrCycle@@QAEXC@Z") == (
        "W8GrCycle::SetBehaviour",
        "GrCycle::SetBehaviour",
        "SetBehaviour",
    )
    assert symbol_candidates(
        "?method_00446110@?$srVector3T@M@@QAEPAV1@PBV?$srVector3T@N@@@Z"
    )[-1] == "method_00446110"


def test_a_template_member_is_named_by_its_own_instantiation() -> None:
    # The growable-vector template emits one COMDAT set per element type, and an
    # @-delimited regex cannot split an argument list off the owner. The owner
    # is therefore read back out of the demangler's signature, so every
    # instantiation stays distinguishable rather than only the int one.
    assert symbol_candidates(
        "?Grow@?$W8GrowableVector@H@@QAEHH@Z",
        "public: int __thiscall W8GrowableVector<int>::Grow(int)",
    ) == ("W8GrowableVector<int>::Grow", "Grow")
    assert symbol_candidates(
        "?Grow@?$W8GrowableVector@PAUW8WorldItem@@@@QAEHH@Z",
        "public: int __thiscall W8GrowableVector<struct W8WorldItem *>::Grow(int)",
    ) == ("W8GrowableVector<W8WorldItem*>::Grow", "Grow")


def test_a_template_lifetime_body_names_its_class_once() -> None:
    # The demangler repeats the argument list in a constructor or destructor
    # name and quotes the compiler-generated ones; a reviewed row spells the
    # class once and uses the repository's own name for the generated body.
    assert symbol_candidates(
        "??_G?$W8GrowableVector@PAVW8DialogOwned005D14D0@@@@UAEPAXI@Z",
        "public: virtual void * __thiscall "
        "W8GrowableVector<class W8DialogOwned005D14D0 *>::"
        "`scalar deleting dtor'(unsigned int)",
    ) == ("W8GrowableVector<W8DialogOwned005D14D0*>::scalar_deleting_destructor",)
    assert symbol_candidates(
        "??1?$W8GrowableVector@H@@UAE@XZ",
        "public: virtual __thiscall W8GrowableVector<int>::~W8GrowableVector<int>(void)",
    ) == ("W8GrowableVector<int>::~W8GrowableVector",)


def test_a_free_function_taking_a_template_keeps_its_bare_name() -> None:
    # `?$` also appears in a parameter type, where it says nothing about the
    # owner: this function belongs to no class at all.
    assert symbol_candidates(
        "?GenerateItemsFromTable@@YAHPAV?$W8GrowableVector@PAUW8WorldItem@@@@II@Z",
        "int __cdecl GenerateItemsFromTable("
        "class W8GrowableVector<struct W8WorldItem *> *, unsigned int, unsigned int)",
    ) == ("GenerateItemsFromTable",)


def test_msvc_special_members_are_offered_under_their_class_names() -> None:
    assert symbol_candidates("??0W8GrCycleBase004B6900@@QAE@XZ") == (
        "W8GrCycleBase004B6900::W8GrCycleBase004B6900",
        "GrCycleBase004B6900::GrCycleBase004B6900",
    )
    assert symbol_candidates("??1W8GrCycle@@UAE@XZ") == (
        "W8GrCycle::~W8GrCycle",
        "GrCycle::~GrCycle",
    )
    assert symbol_candidates("??_GW8GrCycle@@UAEPAXI@Z") == (
        "W8GrCycle::scalar_deleting_destructor",
        "GrCycle::scalar_deleting_destructor",
    )


def test_a_free_function_in_a_cpp_unit_is_offered_under_its_bare_name() -> None:
    # The game is C++, so first-party units are .cpp and their free functions
    # mangle as ?Name@@YA... with no owner between the two @ signs.
    assert symbol_candidates("?SaveFactState@@YAXH@Z") == ("SaveFactState",)
    assert symbol_candidates("?DestroyFactDatabase@@YAXXZ") == ("DestroyFactDatabase",)


def test_undecorated_names_lose_only_the_c_prefix_and_stdcall_suffix() -> None:
    assert symbol_candidates("_GetFact") == ("GetFact",)
    assert symbol_candidates("_SpawnItem@8") == ("SpawnItem",)


def test_the_digest_stops_at_the_reviewed_size() -> None:
    # A COMDAT carries the switch jump table and alignment that follow the body,
    # while the reviewed size counts only the function. Hashing the whole
    # section reports a false mismatch on every dense switch.
    body = b"\xb8\x03\x00\x00\x00\xc3" + b"\x80\xcb\x4a\x00" * 6
    function = CoffFunction(name="_Switchy", body=body, relocation_offsets=(6, 10))

    assert masked_digest(function, 6) == hashlib.sha256(body[:6]).hexdigest()
    # Relocations past the reviewed size are dropped with the bytes they mask.
    assert masked_digest(function, 6) != masked_digest(function, len(body))


def test_relocated_operands_are_masked_before_hashing() -> None:
    # Two bodies differing only in a relocated call target are the same body.
    left = CoffFunction(name="_A", body=b"\xe8\x11\x22\x33\x44\xc3", relocation_offsets=(1,))
    right = CoffFunction(name="_A", body=b"\xe8\x99\x88\x77\x66\xc3", relocation_offsets=(1,))

    assert masked_digest(left, 6) == masked_digest(right, 6)


def test_an_absent_object_tree_is_an_error_not_a_silent_pass(tmp_path: Path) -> None:
    with pytest.raises(RuntimeError, match="no built objects"):
        verify_boundaries(MAPPING, tmp_path / "never-built")


def _write_object(path: Path, name: bytes, body: bytes) -> None:
    """A minimal i386 COFF object with one external .text function."""
    import struct

    section = struct.pack(
        "<8sIIIIIIHHI",
        b".text$mn", 0, 0, len(body), 60, 0, 0, 0, 0, 0x60500020,
    )
    header = struct.pack("<HHIIIHH", 0x14C, 1, 0, 60 + len(body), 2, 0, 0)
    symbols = struct.pack("<8sIhHBB", name[:8], 0, 1, 0x20, 2, 0)
    symbols += struct.pack("<8sIhHBB", b".text$mn", 0, 1, 0, 3, 0)
    strings = struct.pack("<I", 4)
    path.write_bytes(header + section + body + symbols + strings)


def test_a_failed_verdict_still_carries_the_whole_report(tmp_path: Path) -> None:
    # One regressed row used to discard the states of every other row; the
    # exception now carries the report so the CLI can emit it before failing.
    objects = tmp_path / "T.dir"
    objects.mkdir()
    _write_object(objects / "unit.obj", b"_Claimed", b"\xc3\x90\x90\x90")
    mapping = tmp_path / "map.csv"
    mapping.write_text(
        "address,size,symbol,owner,confidence,relocation_masked_sha256,evidence\n"
        "00400000,4,Claimed,test,exact,"
        "0000000000000000000000000000000000000000000000000000000000000000,row\n"
    )

    with pytest.raises(BoundariesDisagree) as failure:
        verify_boundaries(mapping, tmp_path)

    report = failure.value.report
    assert report["states"] == {"regressed": 1}
    # The claimant object is named, which is what turns a stale-object false
    # regression into a one-look diagnosis.
    assert report["results"][0]["object"] == "T.dir/unit.obj"


def test_a_masked_hash_is_recorded_exactly_on_the_rows_that_reproduce_it() -> None:
    # The hash is a cache of a body we can already rebuild, so a near miss has
    # none. That asymmetry is why hash comparison alone cannot notice a near
    # miss becoming exact, and why the check consults the original image.
    with MAPPING.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))

    assert rows
    for number, row in enumerate(rows, start=2):
        assert int(row["size"]) > 0, f"{MAPPING.name}:{number} has no size"
        digest = row["relocation_masked_sha256"].strip()
        if row["confidence"].strip() == "exact":
            assert len(digest) == 64, f"{MAPPING.name}:{number} is exact with no masked hash"
            assert int(digest, 16) >= 0
        else:
            assert not digest, f"{MAPPING.name}:{number} is a near miss carrying a masked hash"


def test_reccmp_similarity_is_not_the_matching_criterion() -> None:
    """A regression guard for the reasoning, not the code.

    reccmp diffs the linked image, where our globals sit at different addresses
    than the original, so every relocated operand counts as a difference.
    `AddLinesToMessageBox` is byte-identical under relocation masking and reccmp
    scores it 75%. Choosing between two candidate bodies by reccmp percentage
    therefore selects the wrong one, which is why `verify-boundaries` exists.
    """

    with MAPPING.open(newline="", encoding="utf-8") as stream:
        rows = {row["symbol"]: row for row in csv.DictReader(stream)}

    assert rows["method_00421680"]["owner"] == "surrender-template"
    assert rows["method_00446110"]["owner"] == "surrender-template"

    assert rows["AddLinesToMessageBox"]["confidence"] == "exact"


def _claimant(body: bytes) -> CoffFunction:
    return CoffFunction(name="_WinMain@16", body=body, relocation_offsets=())


def test_one_claimant_resolves_without_needing_the_original() -> None:
    only = _claimant(b"\x90\xc3")

    assert resolve_boundary_function((only,), size=2, canonical=None) is only


def test_the_body_matching_the_original_wins_over_a_same_sized_one() -> None:
    # The vendored SGP tree and the recovered tree both define WinMain at the
    # same length; only one of them is what the image contains.
    shipped = _claimant(b"\x33\xc0\xc2\x10\x00")
    upstream = _claimant(b"\x33\xc9\xc2\x10\x00")

    resolved = resolve_boundary_function((upstream, shipped), size=5, canonical=shipped.body)

    assert resolved is shipped


def test_the_reviewed_size_decides_when_neither_body_matches_yet() -> None:
    # The ordinary case for a row that is still a near miss: nothing matches the
    # original, so the extent the row states is what picks the body.
    reviewed_length = _claimant(b"\x90\x90\x90\xc3")
    other_length = _claimant(b"\x90\xc3")

    resolved = resolve_boundary_function(
        (other_length, reviewed_length), size=4, canonical=b"\xcc\xcc\xcc\xcc"
    )

    assert resolved is reviewed_length


def test_claimants_that_cannot_be_told_apart_resolve_to_nothing() -> None:
    # Reporting not-built is the safe answer. Picking either would measure a row
    # against a body that may not be its own, which is how a stale link stub
    # named WinMain once shadowed the recovered body without anything noticing.
    left = _claimant(b"\x90\x90\xc3")
    right = _claimant(b"\x91\x91\xc3")

    assert resolve_boundary_function((left, right), size=3, canonical=None) is None
