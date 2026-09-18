import json
import os
from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp import comparison
from wiz8decomp.comparison import (
    addresses_from_files,
    changed_files,
    changed_source_files,
    compare_selected,
    selected_addresses,
)


@pytest.mark.parametrize("accuracy", [1.0, 0.0])
def test_vtable_comparison_keeps_native_slot_diff(tmp_path, monkeypatch, accuracy):
    from reccmp.compare import Compare
    from reccmp.compare.diff import RawDiffOutput
    from reccmp.compare.report import ReccmpComparedEntity
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

    monkeypatch.setattr(comparison, "comparison_target", lambda *_args: object())
    monkeypatch.setattr(comparison, "warn_if_build_may_be_stale", lambda *_args: None)
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
def test_changed_source_files_uses_jj_when_workspace_and_executable_exist(
    tmp_path, monkeypatch, since
):
    (tmp_path / ".jj").mkdir()
    for name in ["One.cpp", "Two Words.h", "README.md"]:
        (tmp_path / name).write_text("")

    monkeypatch.setattr(
        comparison, "resolve_executable", lambda name: "jj" if name == "jj" else None
    )

    def fake_run(command, *, cwd):
        assert cwd == tmp_path
        expected = ["jj", "diff", "--name-only", "--color=never"]
        if since is not None:
            expected.extend(("--from", since))
        assert command == expected
        return SimpleNamespace(stdout="One.cpp\nTwo Words.h\nREADME.md\nremoved.cpp\n")

    monkeypatch.setattr(comparison, "resolve_executable", lambda name: f"/bin/{name}")
    monkeypatch.setattr(comparison, "run", fake_run)
    assert changed_source_files(tmp_path, since) == [
        tmp_path / "One.cpp",
        tmp_path / "Two Words.h",
    ]


def test_changed_files_uses_git_without_jj_workspace(tmp_path, monkeypatch):
    for name in ["One.cpp", "gone.cpp"]:
        (tmp_path / name).write_text("")

    def fake_run(command, *, cwd):
        assert cwd == tmp_path
        assert command == ["git", "diff", "--name-only", "--no-renames", "HEAD^"]
        return SimpleNamespace(stdout="One.cpp\ngone.cpp\n")

    monkeypatch.setattr(comparison, "resolve_executable", lambda name: f"/bin/{name}")
    monkeypatch.setattr(comparison, "run", fake_run)
    monkeypatch.delenv("GITHUB_BASE_REF", raising=False)
    monkeypatch.delenv("GITHUB_EVENT_BEFORE", raising=False)
    assert changed_files(tmp_path) == [tmp_path / "One.cpp", tmp_path / "gone.cpp"]


def test_changed_files_uses_github_base_ref_for_git(tmp_path, monkeypatch):
    (tmp_path / "One.cpp").write_text("")

    def fake_run(command, *, cwd):
        assert command == ["git", "diff", "--name-only", "--no-renames", "origin/develop"]
        return SimpleNamespace(stdout="One.cpp\n")

    monkeypatch.setattr(comparison, "resolve_executable", lambda _name: None)
    monkeypatch.setattr(comparison, "run", fake_run)
    monkeypatch.setenv("GITHUB_BASE_REF", "develop")
    monkeypatch.delenv("GITHUB_EVENT_BEFORE", raising=False)
    assert changed_files(tmp_path) == [tmp_path / "One.cpp"]


def test_changed_files_uses_github_event_before_for_push(tmp_path, monkeypatch):
    (tmp_path / "One.cpp").write_text("")

    before = "abc123def4567890abc123def4567890abc123de"

    def fake_run(command, *, cwd):
        assert command == ["git", "diff", "--name-only", "--no-renames", before]
        return SimpleNamespace(stdout="One.cpp\n")

    monkeypatch.setattr(comparison, "resolve_executable", lambda _name: None)
    monkeypatch.setattr(comparison, "run", fake_run)
    monkeypatch.delenv("GITHUB_BASE_REF", raising=False)
    monkeypatch.setenv("GITHUB_EVENT_BEFORE", before)
    assert changed_files(tmp_path) == [tmp_path / "One.cpp"]


def test_changed_files_ignores_all_zero_github_event_before(tmp_path, monkeypatch):
    (tmp_path / "One.cpp").write_text("")

    def fake_run(command, *, cwd):
        assert command == ["git", "diff", "--name-only", "--no-renames", "HEAD^"]
        return SimpleNamespace(stdout="One.cpp\n")

    monkeypatch.setattr(comparison, "resolve_executable", lambda _name: None)
    monkeypatch.setattr(comparison, "run", fake_run)
    monkeypatch.delenv("GITHUB_BASE_REF", raising=False)
    monkeypatch.setenv("GITHUB_EVENT_BEFORE", "0" * 40)
    assert changed_files(tmp_path) == [tmp_path / "One.cpp"]


def test_changed_files_normalizes_main_at_origin_for_git(tmp_path, monkeypatch):
    (tmp_path / "One.cpp").write_text("")

    def fake_run(command, *, cwd):
        assert command == ["git", "diff", "--name-only", "--no-renames", "origin/main"]
        return SimpleNamespace(stdout="One.cpp\n")

    monkeypatch.setattr(comparison, "resolve_executable", lambda name: f"/bin/{name}")
    monkeypatch.setattr(comparison, "run", fake_run)
    assert changed_files(tmp_path, "main@origin") == [tmp_path / "One.cpp"]


def test_changed_files_prefers_git_when_jj_executable_missing(tmp_path, monkeypatch):
    (tmp_path / ".jj").mkdir()
    (tmp_path / "One.cpp").write_text("")

    def fake_run(command, *, cwd):
        assert command == ["git", "diff", "--name-only", "--no-renames", "HEAD^"]
        return SimpleNamespace(stdout="One.cpp\n")

    monkeypatch.setattr(comparison, "resolve_executable", lambda _name: None)
    monkeypatch.setattr(comparison, "run", fake_run)
    monkeypatch.delenv("GITHUB_BASE_REF", raising=False)
    monkeypatch.delenv("GITHUB_EVENT_BEFORE", raising=False)
    assert changed_files(tmp_path) == [tmp_path / "One.cpp"]


def test_changed_files_uses_jj_in_workspace(tmp_path, monkeypatch):
    (tmp_path / ".jj").mkdir()
    (tmp_path / "One.cpp").write_text("")

    def fake_run(command, *, cwd):
        assert cwd == tmp_path
        assert command == ["jj", "diff", "--name-only", "--color=never"]
        return SimpleNamespace(stdout="One.cpp\n")

    monkeypatch.setattr(comparison, "resolve_executable", lambda name: f"/bin/{name}")
    monkeypatch.setattr(comparison, "run", fake_run)
    monkeypatch.delenv("GITHUB_BASE_REF", raising=False)
    monkeypatch.delenv("GITHUB_EVENT_BEFORE", raising=False)
    assert changed_files(tmp_path) == [tmp_path / "One.cpp"]


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
    (tmp_path / "reccmp-project.yml").write_text("targets:\n  WIZ8:\n    filename: Wiz8.exe\n")

    class Engine:
        def compare_addresses(self, **kwargs):
            seen.append(kwargs)
            return iter([entity])

    products = tmp_path / "build/decomp"
    products.mkdir(parents=True)
    target = SimpleNamespace(
        original_path=tmp_path / "orig.exe",
        recompiled_path=products / "Wiz8.exe",
        recompiled_pdb=products / "Wiz8.pdb",
    )
    target.recompiled_path.write_bytes(b"exe")
    target.recompiled_pdb.write_bytes(b"pdb")
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
    (tmp_path / "reccmp-project.yml").write_text("targets:\n  WIZ8:\n    filename: Wiz8.exe\n")

    class Engine:
        def compare_addresses(self, **_kwargs):
            return iter(())

    products = tmp_path / "build/decomp"
    products.mkdir(parents=True)
    target = SimpleNamespace(
        original_path=tmp_path / "orig.exe",
        recompiled_path=products / "Wiz8.exe",
        recompiled_pdb=products / "Wiz8.pdb",
    )
    target.recompiled_path.write_bytes(b"exe")
    target.recompiled_pdb.write_bytes(b"pdb")
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


def test_compare_selected_classifies_unlinked_header_body_as_emission(tmp_path, monkeypatch):
    (tmp_path / "reccmp-project.yml").write_text("targets:\n  WIZ8:\n    filename: Wiz8.exe\n")

    class Engine:
        def compare_addresses(self, **_kwargs):
            return iter(())

    products = tmp_path / "build/decomp"
    products.mkdir(parents=True)
    target = SimpleNamespace(
        original_path=tmp_path / "orig.exe",
        recompiled_path=products / "Wiz8.exe",
        recompiled_pdb=products / "Wiz8.pdb",
    )
    target.recompiled_path.write_bytes(b"exe")
    target.recompiled_pdb.write_bytes(b"pdb")
    marker = SimpleNamespace(
        name="Widget::Widget",
        source_file="include/wiz8/Widget.h",
        declaration=SimpleNamespace(is_definition=True),
    )
    monkeypatch.setattr(
        comparison,
        "_project",
        lambda _repository: SimpleNamespace(get=lambda _target: target),
    )
    monkeypatch.setattr(comparison.Compare, "from_target", lambda *_args, **_kwargs: Engine())
    monkeypatch.setattr(
        "wiz8decomp.source_index.source_functions", lambda *_args: {0x401000: marker}
    )

    result = compare_selected(tmp_path, "WIZ8", [0x401000], classify_header_emissions=True)

    assert result["ok"] is True
    assert result["missing"] == 0
    assert result["header_emissions"] == 1
    assert result["functions"][0]["status"] == "header-emission"


def test_numeric_missing_comparison_does_not_load_source_index(tmp_path, monkeypatch):
    (tmp_path / "reccmp-project.yml").write_text("targets:\n  WIZ8:\n    filename: Wiz8.exe\n")

    class Engine:
        def compare_addresses(self, **_kwargs):
            return iter(())

    products = tmp_path / "build/decomp"
    products.mkdir(parents=True)
    target = SimpleNamespace(
        original_path=tmp_path / "orig.exe",
        recompiled_path=products / "Wiz8.exe",
        recompiled_pdb=products / "Wiz8.pdb",
    )
    target.recompiled_path.write_bytes(b"exe")
    target.recompiled_pdb.write_bytes(b"pdb")
    monkeypatch.setattr(
        comparison,
        "_project",
        lambda _repository: SimpleNamespace(get=lambda _target: target),
    )
    monkeypatch.setattr(comparison.Compare, "from_target", lambda *_args, **_kwargs: Engine())
    monkeypatch.setattr(
        "wiz8decomp.source_index.source_functions",
        lambda *_args: pytest.fail("numeric comparison must not load the source index"),
    )

    result = compare_selected(tmp_path, "WIZ8", [0x401000])

    assert result["missing"] == 1


def test_missing_comparison_products_fail_without_creating_a_build(tmp_path, monkeypatch):
    (tmp_path / "reccmp-project.yml").write_text("targets:\n  WIZ8:\n    filename: Wiz8.exe\n")
    target = SimpleNamespace(
        original_path=tmp_path / "orig.exe",
        recompiled_path=tmp_path / "Wiz8.exe",
        recompiled_pdb=tmp_path / "Wiz8.pdb",
    )
    monkeypatch.setattr(
        comparison, "_project", lambda _repository: SimpleNamespace(get=lambda _target: target)
    )

    with pytest.raises(FileNotFoundError, match=r"uv run wiz8 build"):
        compare_selected(tmp_path, "WIZ8", [0x401000])


@pytest.mark.parametrize("input_newer, warns", [(True, True), (False, False)])
def test_build_freshness_warning_uses_input_mtimes(
    tmp_path, caplog, input_newer: bool, warns: bool
):
    source = tmp_path / "src/wiz8/unit.cpp"
    source.parent.mkdir(parents=True)
    source.write_text("void f() {}")
    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    source-root: src/wiz8\n"
    )
    executable = tmp_path / "Wiz8.exe"
    pdb = tmp_path / "Wiz8.pdb"
    executable.write_bytes(b"exe")
    pdb.write_bytes(b"pdb")
    old, new = 1_000_000_000, 2_000_000_000
    artifact_time, source_time = (old, new) if input_newer else (new, old)
    os.utime(executable, ns=(artifact_time, artifact_time))
    os.utime(pdb, ns=(artifact_time, artifact_time))
    os.utime(source, ns=(source_time, source_time))
    os.utime(tmp_path / "reccmp-project.yml", ns=(source_time, source_time))
    target = SimpleNamespace(recompiled_path=executable, recompiled_pdb=pdb)

    comparison.warn_if_build_may_be_stale(tmp_path, "WIZ8", target)

    assert ("comparison build may be stale" in caplog.text) is warns
