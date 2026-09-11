import json
from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp import comparison
from wiz8decomp.comparison import (
    addresses_from_files,
    changed_source_files,
    compare_selected,
    selected_addresses,
)


@pytest.mark.parametrize("accuracy", [1.0, 0.0])
def test_vtable_comparison_keeps_native_slot_diff(tmp_path, monkeypatch, accuracy):
    from reccmp.compare import Compare
    from reccmp.compare.diff import RawDiffOutput
    from reccmp.compare.report import ReccmpComparedEntity
    from reccmp.project.detect import RecCmpProject
    from reccmp.types import EntityType

    slot = ("vtable0x00", "Widget::Draw")

    def compare_vtables(*, include_diff):
        yield ReccmpComparedEntity(
            orig_addr=0x401000,
            recomp_addr=0x501000,
            name="Widget::vftable",
            type=EntityType.VTABLE,
            accuracy=accuracy,
            rdiff=(
                RawDiffOutput(
                    codes=[("equal", 0, 1, 0, 1)],
                    orig_inst=[slot],
                    recomp_inst=[slot],
                )
                if include_diff
                else None
            ),
        )

    monkeypatch.setattr(
        RecCmpProject, "from_directory", lambda *_: SimpleNamespace(get=lambda _: object())
    )
    monkeypatch.setattr(
        Compare, "from_target", lambda *_: SimpleNamespace(compare_vtables=compare_vtables)
    )
    result = comparison.compare_vtables(tmp_path, "WIZ8", "Widget")

    assert result["ok"] is (accuracy == 1.0)
    table = result["vtables"][0]
    assert table["accuracy"] == accuracy
    assert table["diff"][0][1][0]["both"] == [(slot[0], slot[1], slot[0])]


def test_source_selection_deduplicates_function_markers(tmp_path: Path) -> None:
    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n"
        "  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
        "  SURRENDER:\n    filename: sr.dll\n    hash:\n      sha256: def\n"
    )
    source = tmp_path / "unit.cpp"
    source.write_text(
        "// FUNCTION: WIZ8 0x00401000\nvoid a();\n"
        "// LIBRARY: WIZ8 0x00402000\n"
        "// FUNCTION: WIZ8 00401010\nvoid b();\n",
        encoding="utf-8",
    )
    (tmp_path / "build").mkdir()
    (tmp_path / "build/source-index.json").write_text(
        json.dumps(
            {
                "schema": "reccmp-source-index-v2",
                "classes": [],
                "declarations": [],
                "markers": [
                    {
                        "marker_kind": "FUNCTION",
                        "address": 0x00401000,
                        "source_file": "unit.cpp",
                        "line": 1,
                        "declaration": None,
                        "marker_name": "sr_a",
                        "target": "SURRENDER",
                    },
                    {
                        "marker_kind": "FUNCTION",
                        "address": 0x00401000,
                        "source_file": "unit.cpp",
                        "line": 1,
                        "declaration": None,
                        "marker_name": "a",
                        "target": "WIZ8",
                    },
                    {
                        "marker_kind": "LIBRARY",
                        "marker_name": "library",
                        "target": "WIZ8",
                        "address": 0x00402000,
                        "source_file": "unit.cpp",
                        "line": 3,
                        "declaration": None,
                    },
                    {
                        "marker_kind": "FUNCTION",
                        "address": 0x00401010,
                        "source_file": "unit.cpp",
                        "line": 4,
                        "declaration": None,
                        "marker_name": "b",
                        "target": "WIZ8",
                    },
                ],
            }
        ),
        encoding="utf-8",
    )

    assert addresses_from_files(tmp_path, "WIZ8", [source]) == [0x00401000, 0x00401010]
    assert selected_addresses(tmp_path, "WIZ8", ["0x00401000"], [source]) == [
        0x00401000,
        0x00401010,
    ]


@pytest.mark.parametrize("since", [None, "main"])
def test_changed_source_files_preserves_spaces_and_ignores_removed_files(
    tmp_path, monkeypatch, since
):
    for name in ["One.cpp", "Two Words.h", "README.md"]:
        (tmp_path / name).write_text("")

    def fake_run(command, *, cwd):
        assert cwd == tmp_path
        expected = ["jj", "diff", "--name-only", "--color=never"]
        if since is not None:
            expected.extend(("--from", since))
        assert command == expected
        return SimpleNamespace(stdout="One.cpp\nTwo Words.h\nREADME.md\nremoved.cpp\n")

    monkeypatch.setattr(comparison, "run", fake_run)
    assert changed_source_files(tmp_path, since) == [
        tmp_path / "One.cpp",
        tmp_path / "Two Words.h",
    ]


def test_compare_selected_uses_one_in_process_comparison(tmp_path, monkeypatch):
    from reccmp.compare.diagnosis import (
        ComparisonAnalysis,
        ComparisonDifference,
        DifferenceSide,
    )
    from reccmp.compare.report import ReccmpComparedEntity
    from reccmp.types import EntityType

    entity = ReccmpComparedEntity(
        orig_addr=0x401000,
        recomp_addr=0x501000,
        name="Widget::Run",
        type=EntityType.FUNCTION,
        accuracy=0.9,
        analysis=ComparisonAnalysis.mismatch(
            ComparisonDifference(
                kind="branch_target",
                orig=DifferenceSide(instruction_index=1),
                recomp=DifferenceSide(instruction_index=1),
            )
        ),
    )
    seen = []

    class Engine:
        def compare_addresses(self, **kwargs):
            seen.append(kwargs)
            return iter([entity])

    target = SimpleNamespace(
        original_path=tmp_path / "orig.exe", recompiled_path=tmp_path / "recomp.exe"
    )
    monkeypatch.setattr(
        comparison,
        "_project",
        lambda _repository: SimpleNamespace(get=lambda _target: target),
    )
    monkeypatch.setattr(comparison.Compare, "from_target", lambda *_args, **_kwargs: Engine())
    monkeypatch.setattr(
        comparison,
        "_instruction_windows",
        lambda *_args, **_kwargs: {
            "original": [
                {
                    "address": "0x00401003",
                    "instruction": "jne 0x401020",
                    "divergence": True,
                }
            ]
        },
    )

    result = compare_selected(tmp_path, "WIZ8", [0x401000])

    assert len(seen) == 1
    row = result["functions"][0]
    assert row["status"] == "mismatch"
    assert row["effective_matching"] == 0.9
    assert row["difference"]["kind"] == "alignment_or_structure"
    assert row["reported_difference"]["kind"] == "branch_target"
    assert row["instruction_window"]["original"][0]["divergence"]


def test_compare_selected_marks_unpaired_addresses_missing(tmp_path, monkeypatch):
    class Engine:
        def compare_addresses(self, **_kwargs):
            return iter(())

    target = SimpleNamespace(
        original_path=tmp_path / "orig.exe", recompiled_path=tmp_path / "recomp.exe"
    )
    monkeypatch.setattr(
        comparison,
        "_project",
        lambda _repository: SimpleNamespace(get=lambda _target: target),
    )
    monkeypatch.setattr(comparison.Compare, "from_target", lambda *_args, **_kwargs: Engine())

    result = compare_selected(tmp_path, "WIZ8", [0x401000])

    assert result["ok"] is False
    assert result["missing"] == 1
    assert result["functions"] == [{"address": "0x00401000", "status": "missing"}]
