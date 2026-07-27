"""Aligning one build's functions onto another's, and what refuses to align."""

from __future__ import annotations

from pathlib import Path

import pytest
from wiz8decomp.cross_build import (
    ANCHOR,
    Alignment,
    _call_graph,
    align,
    assertion_fingerprints,
    line_drift,
    normalise_source_path,
    verify,
)

REPOSITORY = Path(__file__).resolve().parents[2]
DEMO = "wiz8--demo--wiz8--8acda50afe39"
RETAIL = "wiz8--gog-base--wiz8--18a74ff61c65"
PATCH = "wiz8--gog-1261--wiz8new--57a3072c2f83"

_ASSERTIONS = (
    "program,call_site,call_kind,function_start,source_path,line,expression,message\n"
    # One expression, one function, both builds: an anchor.
    "left,00001001,direct,00001000,E:\\Wizardry 8\\Engine Code\\GameData.cpp,591,"
    "lCollisions < 100,\n"
    "right,00002001,direct,00002000,C:\\Projects\\Wizardry 8\\Engine Code\\GameData.cpp,595,"
    "lCollisions < 100,\n"
    # The same expression in two functions on the left: no unique anchor.
    "left,00001101,direct,00001100,E:\\Wizardry 8\\Magic.cpp,10,pSpell,\n"
    "left,00001201,direct,00001200,E:\\Wizardry 8\\Magic.cpp,20,pSpell,\n"
    "right,00002101,direct,00002100,C:\\Projects\\Wizardry 8\\Magic.cpp,10,pSpell,\n"
    # A site with no expression falls back to its message.
    "left,00001301,direct,00001300,E:\\Wizardry 8\\Items.cpp,5,,out of items\n"
    "right,00002301,direct,00002300,C:\\Projects\\Wizardry 8\\Items.cpp,5,,out of items\n"
    # A site recording neither is dropped rather than tying its file together.
    "left,00001401,direct,00001400,,,,\n"
)

_CALLS = (
    "program,caller,callee,call_sites\n"
    # The anchored pair has exactly one unaligned callee on each side.
    "left,00001000,00009000,1\n"
    "right,00002000,00009900,1\n"
    # ...which in turn has two, so propagation stops there.
    "left,00009000,0000a000,1\n"
    "left,00009000,0000b000,1\n"
    "right,00009900,0000a900,1\n"
    "right,00009900,0000b900,1\n"
)


def _repo(tmp_path: Path) -> Path:
    snapshots = tmp_path / "evidence" / "snapshots"
    (snapshots / "call-sites").mkdir(parents=True)
    (snapshots / "functions").mkdir(parents=True)
    (snapshots / "call-sites" / "assertions.csv").write_text(_ASSERTIONS, encoding="utf-8")
    (snapshots / "functions" / "calls.csv").write_text(_CALLS, encoding="utf-8")
    return tmp_path


def test_two_build_machines_name_the_same_file() -> None:
    assert (
        normalise_source_path("E:\\Wizardry 8\\Engine Code\\GameData.cpp")
        == "engine code/gamedata.cpp"
    )
    assert (
        normalise_source_path("C:\\Projects\\Wizardry 8\\Engine Code\\GameData.cpp")
        == "engine code/gamedata.cpp"
    )
    assert normalise_source_path("Z:\\elsewhere\\Other.cpp") == "other.cpp"


def test_the_line_number_is_not_part_of_the_fingerprint(tmp_path: Path) -> None:
    # It is exactly what moves between builds; matching through it would make
    # every edited file's functions unmatchable.
    fingerprints = assertion_fingerprints(_repo(tmp_path))

    assert fingerprints["left"]["00001000"] == fingerprints["right"]["00002000"]
    assert fingerprints["left"]["00001300"] == frozenset({("items.cpp", "out of items")})
    assert "00001400" not in fingerprints["left"]


def test_an_anchor_needs_one_candidate_on_each_side(tmp_path: Path) -> None:
    alignment = align(_repo(tmp_path), "left", "right")

    assert alignment.pairs["00001000"] == "00002000"
    assert alignment.reasons["00001000"] == ANCHOR
    # Two left functions assert `pSpell` in Magic.cpp, so neither is anchored.
    assert "00001100" not in alignment.pairs
    assert "00001200" not in alignment.pairs
    assert alignment.ambiguous[0]["left"] == ["00001100", "00001200"]


def test_propagation_stops_where_more_than_one_candidate_fits(tmp_path: Path) -> None:
    alignment = align(_repo(tmp_path), "left", "right")

    assert alignment.pairs["00009000"] == "00009900"
    assert alignment.reasons["00009000"] == "sole unaligned callee of 00001000"
    # 0x9000 calls two unaligned functions and so does 0x9900; which is which
    # is exactly what the call graph cannot say.
    assert "0000a000" not in alignment.pairs
    assert "0000b000" not in alignment.pairs


def test_a_function_is_never_aligned_twice() -> None:
    alignment = Alignment(left="left", right="right")

    assert alignment.add("1", "2", ANCHOR)
    assert not alignment.add("1", "3", ANCHOR)
    assert not alignment.add("4", "2", ANCHOR)
    assert alignment.pairs == {"1": "2"}


def test_drift_reports_which_files_moved(tmp_path: Path) -> None:
    drift = {row["source_path"]: row for row in line_drift(_repo(tmp_path), "left", "right")}

    assert drift["engine code/gamedata.cpp"]["shifts"] == [4]
    assert not drift["engine code/gamedata.cpp"]["unchanged"]
    assert drift["items.cpp"]["unchanged"]


def test_the_patch_alignment_predicts_every_edge_it_did_not_use() -> None:
    # The real corpus: retail against its 1.2.6 patch. Every call between two
    # aligned functions is a prediction, and this pairing keeps all of them -
    # the check that the anchoring rule is sound rather than merely plausible.
    alignment = align(REPOSITORY, RETAIL, PATCH)
    verification = verify(alignment, _call_graph(REPOSITORY, {RETAIL, PATCH}))

    assert verification["predicted_edges"] > 2000
    assert verification["agreement"] == 1.0


def test_the_demo_alignment_is_strong_without_pretending_to_be_perfect() -> None:
    # The demo is a different compilation of a different source state, so some
    # predicted edges genuinely fail; a rate near 1.0 is the honest result and
    # a rate of exactly 1.0 would be the suspicious one.
    alignment = align(REPOSITORY, DEMO, RETAIL)
    verification = verify(alignment, _call_graph(REPOSITORY, {DEMO, RETAIL}))

    assert alignment.summary()["by_reason"][ANCHOR] > 300
    assert 0.9 < verification["agreement"] < 1.0


def test_an_unobserved_program_is_refused_rather_than_aligned_to_nothing() -> None:
    with pytest.raises(ValueError, match="no assertion observations"):
        align(REPOSITORY, RETAIL, "wiz8--not--a--program")
