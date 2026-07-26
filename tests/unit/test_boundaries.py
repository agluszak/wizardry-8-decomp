import csv
import hashlib
from pathlib import Path

import pytest
from wiz8decomp.boundaries import (
    masked_digest,
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
