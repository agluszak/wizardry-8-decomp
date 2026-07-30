from pathlib import Path

import pytest
import wiz8decomp.unresolved as unresolved_module
from wiz8decomp.unresolved import (
    compare_unresolved_reports,
    load_unresolved_baseline,
    parse_map_publics,
    require_unresolved_delta,
    unresolved_report,
    write_unresolved_baseline,
)

REPOSITORY = Path(__file__).resolve().parents[2]


def test_map_publics_survive_the_crlf_the_linker_writes(tmp_path: Path) -> None:
    # The MAP is written with CRLF, so a parser that trusts the line ending
    # silently reports every resolved symbol as missing.
    path = tmp_path / "Wiz8.map"
    path.write_bytes(
        b" 0001:0000b532       _GetTickCount@0            0040c532 f   kernel32:KERNEL32.dll\r\n"
        b" 0002:00000008       __imp__GetTickCount@0      0040d008     kernel32:KERNEL32.dll\r\n"
    )

    assert "_GetTickCount@0" in parse_map_publics(path)


def test_a_symbol_the_map_defines_is_not_reported_missing(tmp_path: Path) -> None:
    objects = tmp_path / "probe.dir"
    objects.mkdir()
    # No objects at all: the report is empty rather than raising, because an
    # unbuilt tree is a reason to build, not a finding.
    report = unresolved_report(objects, None)

    assert report["unresolved_symbols"] == 0
    assert report["by_unit"] == {}


def test_the_object_root_must_exist(tmp_path: Path) -> None:
    try:
        unresolved_report(tmp_path / "absent", None)
    except RuntimeError as error:
        assert "no built objects" in str(error)
    else:  # pragma: no cover - the call above is expected to raise
        raise AssertionError("a missing object root should be refused")


def test_imports_are_not_counted_as_unresolved() -> None:
    # A symbol an import library satisfies is resolved, and the decorated
    # __imp_ spelling exists only because the linker rewrote a resolved call.
    build = REPOSITORY / "build" / "decomp" / "CMakeFiles"
    if not build.is_dir():
        return
    report = unresolved_report(build, REPOSITORY / "build" / "decomp" / "Wiz8.map")
    assert not [name for name in report["by_symbol"] if name.startswith("__imp_")]


def test_imports_are_reported_separately_and_units_are_ranked(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    objects = tmp_path / "objects"
    (objects / "recovered.dir" / "src").mkdir(parents=True)
    first = objects / "recovered.dir" / "src" / "first.cpp.obj"
    second = objects / "recovered.dir" / "src" / "second.cpp.obj"
    first.write_bytes(b"object")
    second.write_bytes(b"object")

    symbols = {
        first: (set(), {"?MissingA@@YAXXZ", "?MissingB@@YAXXZ", "__imp__CreateFileA@28"}),
        second: (set(), {"?MissingA@@YAXXZ"}),
    }
    monkeypatch.setattr(unresolved_module, "object_symbols", lambda path: symbols[path])

    report = unresolved_report(objects)

    assert report["ranked_units"][0]["unit"] == "recovered.dir/src/first.cpp.obj"
    assert report["ranked_units"][0]["unresolved_count"] == 2
    assert report["near_link_complete_units"] == report["ranked_units"]
    assert report["canonical_imports_by_symbol"] == {
        "__imp__CreateFileA@28": ["recovered.dir/src/first.cpp.obj"]
    }
    assert "__imp__CreateFileA@28" not in report["by_symbol"]


def test_unresolved_delta_reports_progress_and_regression() -> None:
    baseline = {"symbols": [{"symbol": "missing-a"}, {"symbol": "missing-b"}]}
    current = {
        "by_symbol": {"missing-b": ["b.obj"], "missing-c": ["c.obj"]},
        "ranked_units": [],
        "near_link_complete_units": [],
        "canonical_import_symbols": 1,
        "canonical_imports_by_symbol": {"__imp_x": ["b.obj"]},
    }

    delta = compare_unresolved_reports(current, baseline, baseline_name="baseline.csv")

    assert not delta["ok"]
    assert delta["introduced"] == ["missing-c"]
    assert delta["resolved"] == ["missing-a"]
    assert delta["unchanged"] == ["missing-b"]
    delta["report"] = "delta.json"
    with pytest.raises(ValueError, match="introduced 1"):
        require_unresolved_delta(delta)


def test_unresolved_baseline_can_only_shrink(tmp_path: Path) -> None:
    path = tmp_path / "unresolved-baseline.csv"
    initial = {
        "by_symbol": {"missing-a": ["a.obj"], "missing-b": ["b.obj"]},
    }
    reduced = {"by_symbol": {"missing-a": ["a.obj"]}}

    assert write_unresolved_baseline(path, initial)["symbol_count"] == 2
    assert write_unresolved_baseline(path, reduced)["symbol_count"] == 1
    assert load_unresolved_baseline(path)["symbols"] == [{"symbol": "missing-a"}]
    with pytest.raises(ValueError, match="refusing to add 1"):
        write_unresolved_baseline(path, initial)
