"""Joint object-file assignment: what it decides, and what it refuses to."""

from __future__ import annotations

from pathlib import Path

from wiz8decomp.object_map import ANCHOR, CUT, ENCLOSED, assertion_anchors, assign

REPOSITORY = Path(__file__).resolve().parents[2]
CANONICAL = "wiz8--gog-base--wiz8--18a74ff61c65"

_ASSERTIONS = (
    "program,call_site,call_kind,function_start,source_path,line,expression,message\n"
    "p,00001001,direct,00001000,C:\\Projects\\Wizardry 8\\Left.cpp,1,a,\n"
    "p,00001101,direct,00001100,C:\\Projects\\Wizardry 8\\Left.cpp,2,b,\n"
    "p,00001901,direct,00001900,C:\\Projects\\Wizardry 8\\Right.cpp,3,c,\n"
    # Two units name one function: it was inlined into, and anchors neither.
    "p,00002001,direct,00002000,C:\\Projects\\Wizardry 8\\Left.cpp,4,d,\n"
    "p,00002002,direct,00002000,C:\\Projects\\Wizardry 8\\Right.cpp,5,e,\n"
)

# 0x1200 talks only to Left's anchors, 0x1800 only to Right's, and 0x1500 to
# neither - so the boundary is bounded to the stretch around 0x1500.
_CALLS = (
    "program,caller,callee,call_sites\n"
    "p,00001000,00001100,1\n"
    "p,00001000,00001200,1\n"
    "p,00001100,00001200,1\n"
    "p,00001900,00001800,1\n"
    "p,00001800,00001900,1\n"
    "p,00001500,00007000,1\n"
    "p,00001000,00001050,1\n"
)


def _repo(tmp_path: Path) -> Path:
    snapshots = tmp_path / "evidence" / "snapshots"
    (snapshots / "call-sites").mkdir(parents=True)
    (snapshots / "functions").mkdir(parents=True)
    (snapshots / "call-sites" / "assertions.csv").write_text(_ASSERTIONS, encoding="utf-8")
    (snapshots / "functions" / "calls.csv").write_text(_CALLS, encoding="utf-8")
    return tmp_path


def test_a_function_two_units_assert_in_anchors_neither(tmp_path: Path) -> None:
    anchors = assertion_anchors(_repo(tmp_path), "p")

    assert anchors[0x1000] == "left.cpp"
    assert 0x2000 not in anchors


def test_functions_between_one_unit_s_anchors_are_that_unit(tmp_path: Path) -> None:
    mapping = assign(_repo(tmp_path), "p")

    assert mapping.owners["00001000"] == "left.cpp"
    assert mapping.basis["00001000"] == ANCHOR
    assert mapping.owners["00001050"] == "left.cpp"
    assert mapping.basis["00001050"] == ENCLOSED


def test_a_boundary_is_placed_where_calls_stop_crossing_it(tmp_path: Path) -> None:
    mapping = assign(_repo(tmp_path), "p")

    assert mapping.owners["00001200"] == "left.cpp"
    assert mapping.basis["00001200"] == CUT
    assert mapping.owners["00001800"] == "right.cpp"
    assert mapping.basis["00001800"] == CUT


def test_a_function_the_optimum_cannot_place_is_left_unassigned(tmp_path: Path) -> None:
    # 0x1500 exchanges nothing with either unit, so every optimal boundary
    # placement is equally good and they disagree about it.
    mapping = assign(_repo(tmp_path), "p")

    assert "00001500" not in mapping.owners
    boundary = mapping.boundaries[0]
    assert boundary["left_unit"] == "left.cpp"
    assert boundary["right_unit"] == "right.cpp"
    assert boundary["undetermined"] == 1


def test_the_joint_model_decides_more_than_the_intervals_it_replaces() -> None:
    # The interval model speaks only for anchors and what they enclose; the
    # boundary placements are the functions it left without an owner.
    mapping = assign(REPOSITORY, CANONICAL)
    counts = mapping.summary()["by_basis"]

    assert counts[CUT] > counts[ANCHOR]
    assert counts[CUT] + counts[ENCLOSED] + counts[ANCHOR] == mapping.summary()["assigned"]
    # And it still declines to place a function inside a flat optimum.
    assert sum(item["undetermined"] for item in mapping.boundaries) > 0
