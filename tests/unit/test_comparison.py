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


def test_changed_files_normalizes_origin_main_for_jj(tmp_path, monkeypatch):
    (tmp_path / ".jj").mkdir()
    (tmp_path / "One.cpp").write_text("")

    def fake_run(command, *, cwd):
        assert cwd == tmp_path
        assert command == ["jj", "diff", "--name-only", "--color=never", "--from", "main@origin"]
        return SimpleNamespace(stdout="One.cpp\n")

    monkeypatch.setattr(comparison, "resolve_executable", lambda name: f"/bin/{name}")
    monkeypatch.setattr(comparison, "run", fake_run)
    assert changed_files(tmp_path, "origin/main") == [tmp_path / "One.cpp"]


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
        compare_selected(tmp_path, "WIZ8", [0x401000], Path("/opt/ghidra"))


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


def _products(tmp_path: Path, monkeypatch) -> None:
    (tmp_path / "reccmp-project.yml").write_text("targets:\n  WIZ8:\n    filename: Wiz8.exe\n")
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
        comparison, "_project", lambda _repository: SimpleNamespace(get=lambda _target: target)
    )


def _fake_reccmp(monkeypatch, functions: list[dict], *, seen: list | None = None) -> None:
    """Stand in for `reccmp-reccmp`: write its manifest and summary."""

    def run(argv, *, cwd, env, log_path, check):
        output = Path(argv[argv.index("--output") + 1])
        output.mkdir(parents=True, exist_ok=True)
        if seen is not None:
            seen.append({"argv": [str(arg) for arg in argv], "cwd": cwd, "env": env})
        (output / "manifest.json").write_text(json.dumps({"functions": functions}))
        if functions:
            (output / "summary.json").write_text(
                json.dumps({"requested": len(functions), "functions": functions})
            )
        return SimpleNamespace(exit_status=0 if functions else 1, stderr="")

    monkeypatch.setattr(comparison, "run", run)


def _row(address: int, outcome: str, diff: list[str] | None = None) -> dict:
    return {
        "orig": f"{address:#x}",
        "recomp": f"{address + 0x100000:#x}",
        "name": "Widget::Run",
        "basis": "annotation",
        "source": {"path": "src/wiz8/widget.cpp", "line": 3},
        "outcome": outcome,
        "code_diff": diff or [],
        "data": [],
        "failures": [],
        "unidentified_references": 0,
    }


def test_compare_selected_runs_reccmp_for_the_selected_addresses(tmp_path, monkeypatch):
    _products(tmp_path, monkeypatch)
    seen: list = []
    diff = ["--- orig/Widget::Run\n", "+++ recomp/Widget::Run\n", "-  a <= b\n", "+  a > b\n"]
    _fake_reccmp(monkeypatch, [_row(0x401000, "differences", diff)], seen=seen)

    result = compare_selected(tmp_path, "WIZ8", [0x401000], Path("/opt/ghidra"), side_by_side=True)

    [call] = seen
    assert call["argv"][call["argv"].index("--orig-address") + 1] == "401000"
    assert "--sxs" in call["argv"]
    assert call["cwd"] == tmp_path / "build/decomp"
    assert call["env"]["GHIDRA_INSTALL_DIR"] == "/opt/ghidra"
    # Differences are review material, not failures.
    assert result["ok"] is True
    assert result["counts"]["differences"] == 1
    [row] = result["functions"]
    assert row["outcome"] == "differences"
    assert row["code_diff"]["lines"] == 2
    assert (tmp_path / row["code_diff"]["artifact"]).read_text() == "".join(diff)


@pytest.mark.parametrize("outcome", ["analysis-failed", "unpaired"])
def test_incomplete_comparison_fails_the_selection(tmp_path, monkeypatch, outcome):
    _products(tmp_path, monkeypatch)
    _fake_reccmp(monkeypatch, [_row(0x401000, outcome)])

    result = compare_selected(tmp_path, "WIZ8", [0x401000], Path("/opt/ghidra"))

    assert result["ok"] is False
    assert result["counts"][outcome] == 1


def test_compare_selected_marks_addresses_reccmp_does_not_know_missing(tmp_path, monkeypatch):
    _products(tmp_path, monkeypatch)
    _fake_reccmp(monkeypatch, [])
    monkeypatch.setattr(
        "wiz8decomp.source_index.source_functions",
        lambda *_args: pytest.fail("numeric comparison must not load the source index"),
    )

    result = compare_selected(tmp_path, "WIZ8", [0x401000], Path("/opt/ghidra"))

    assert result["ok"] is False
    assert result["counts"]["missing"] == 1
    assert result["functions"] == [{"orig": "0x00401000", "outcome": "missing"}]


def test_compare_selected_classifies_unlinked_header_body_as_emission(tmp_path, monkeypatch):
    _products(tmp_path, monkeypatch)
    _fake_reccmp(monkeypatch, [])
    marker = SimpleNamespace(
        name="Widget::Widget",
        source_file="include/wiz8/Widget.h",
        declaration=SimpleNamespace(is_definition=True),
    )
    monkeypatch.setattr(
        "wiz8decomp.source_index.source_functions", lambda *_args: {0x401000: marker}
    )

    result = compare_selected(
        tmp_path, "WIZ8", [0x401000], Path("/opt/ghidra"), classify_header_emissions=True
    )

    assert result["ok"] is True
    assert result["counts"]["header-emission"] == 1
    assert result["functions"][0]["outcome"] == "header-emission"


def test_compare_selected_classifies_unpaired_inline_header_as_emission(tmp_path, monkeypatch):
    _products(tmp_path, monkeypatch)
    unpaired = _row(0x401000, "unpaired")
    unpaired["recomp"] = None
    _fake_reccmp(monkeypatch, [unpaired])
    marker = SimpleNamespace(
        name="Widget::Widget",
        source_file="include/wiz8/Widget.h",
        declaration=None,
        marker_name="?Widget@@YAXXZ",
    )
    monkeypatch.setattr(
        "wiz8decomp.source_index.source_functions", lambda *_args: {0x401000: marker}
    )
    monkeypatch.setattr(
        "wiz8decomp.source_index.load_source_index",
        lambda *_args: {
            "declarations": [
                {
                    "semantic_id": "?Widget@@YAXXZ",
                    "source_file": "include/wiz8/Widget.h",
                    "target": "WIZ8",
                    "is_definition": True,
                }
            ]
        },
    )

    result = compare_selected(
        tmp_path, "WIZ8", [0x401000], Path("/opt/ghidra"), classify_header_emissions=True
    )

    assert result["ok"] is True
    assert result["counts"]["header-emission"] == 1
    assert result["counts"]["unpaired"] == 0


def test_vtable_comparison_reports_unpaired_and_different_slots(tmp_path, monkeypatch):
    from reccmp.compare import Compare
    from reccmp.compare.vtables import SlotStatus

    tables = [
        SimpleNamespace(name="Widget::vftable", orig_addr=0x401000, recomp_addr=0x501000),
        SimpleNamespace(name="Folded::vftable", orig_addr=0x402000, recomp_addr=0x502000),
    ]
    slots = {
        0x401000: [SimpleNamespace(status=SlotStatus.MATCH)],
        0x402000: [
            SimpleNamespace(
                status=SlotStatus.UNPAIRED,
                offset=0,
                orig=SimpleNamespace(best_name=lambda: "Base::Draw"),
                recomp=SimpleNamespace(best_name=lambda: "Folded::Draw"),
                orig_raw=0x403000,
                recomp_raw=0x503000,
            )
        ],
    }
    catalog = SimpleNamespace(get_vtables=lambda: tables, db=None, orig_bin=None, recomp_bin=None)
    monkeypatch.setattr(comparison, "comparison_target", lambda *_args: object())
    monkeypatch.setattr(comparison, "warn_if_build_may_be_stale", lambda *_args: None)
    monkeypatch.setattr(Compare, "from_target", lambda *_: catalog)
    monkeypatch.setattr(
        comparison,
        "compare_vtable",
        lambda _db, _orig, _recomp, table: SimpleNamespace(slots=slots[table.orig_addr]),
    )

    result = comparison.compare_vtables(tmp_path, "WIZ8", None)

    assert result["ok"] is False
    assert (result["match_count"], result["unpaired_count"], result["different_count"]) == (1, 1, 0)
    [table] = result["vtables"]
    assert table["slots"] == [
        {
            "offset": 0,
            "status": "unpaired",
            "original": "Base::Draw",
            "recompiled": "Folded::Draw",
        }
    ]
