import json
from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp import reccmp_workflows
from wiz8decomp.reccmp_workflows import (
    _sr_assert_alias_mismatch,
    addresses_from_files,
    compare_rows,
    compare_vtables,
    run_report,
    selected_addresses,
    translate_rows,
    triage_rows,
    vtable_count,
)


def _entity(address: int, status: str, **comparison):
    return {
        "address": f"0x{address:08x}",
        "recomp": f"0x{address + 0x1000:08x}",
        "name": f"Function{address:08X}",
        "matching": 0.75,
        "comparison": {"status": status, **comparison},
    }


def test_sr_assert_import_alias_requires_the_exact_known_pair() -> None:
    row = _entity(
        0x451160,
        "mismatch",
        difference={
            "kind": "call_target",
            "orig": {"facts": {"target_name": "SR.DLL::?srAssertFail@@YAXPBD0J0ZZ (IMPORT)"}},
            "recomp": {"facts": {"target_name": "SR.dll::?srAssertFail@@YAXPBD0J0@Z (IMPORT)"}},
        },
    )

    assert _sr_assert_alias_mismatch(row)
    row["comparison"]["difference"]["orig"]["facts"]["target_name"] = "someOtherCall"
    assert not _sr_assert_alias_mismatch(row)


def test_source_selection_deduplicates_function_markers(tmp_path: Path) -> None:
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
                "schema": "reccmp-source-index-v1",
                "classes": [],
                "declarations": [],
                "markers": [
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

    assert addresses_from_files(tmp_path, [source]) == [0x00401000, 0x00401010]
    assert selected_addresses(tmp_path, ["0x00401000"], [source]) == [
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

    monkeypatch.setattr(reccmp_workflows, "run", fake_run)
    assert reccmp_workflows.changed_source_files(tmp_path, since) == [
        tmp_path / "One.cpp",
        tmp_path / "Two Words.h",
    ]


def test_compare_treats_semantically_effective_as_complete() -> None:
    result = compare_rows(
        [
            _entity(0x401000, "exact"),
            _entity(0x401010, "effective", effective_reasons=["register_allocation"]),
        ],
        [0x401000, 0x401010],
    )

    assert result["ok"] is True
    assert result["exact"] == 1
    assert result["effective"] == 1
    assert result["functions"][1]["effective_matching"] == 1.0


def test_triage_preserves_structured_difference_and_inconclusive_ceiling() -> None:
    result = triage_rows(
        [
            _entity(
                0x401000,
                "mismatch",
                difference={"kind": "call_argument", "orig": {}, "recomp": {}},
            ),
            _entity(0x401010, "inconclusive", inconclusive_reason="unsupported_control_flow"),
        ],
        [0x401000, 0x401010],
    )

    assert result["functions"][0]["difference"]["kind"] == "call_argument"
    assert "receiver" in result["functions"][0]["guidance"]
    assert result["functions"][1]["conclusion"] == "not evidence of a source defect"


def test_address_translation_checks_both_spaces() -> None:
    entity = _entity(0x401000, "exact")

    result = translate_rows([entity], [0x401000, 0x402000, 0x999999])

    assert result["translations"][0]["direction"] == "original-to-recompiled"
    assert result["translations"][1]["direction"] == "recompiled-to-original"
    assert result["translations"][2]["status"] == "missing"


def test_vtable_summary_count_makes_zero_detectable() -> None:
    assert vtable_count("Vtables found: 3.\n100% match.\n") == 3
    assert vtable_count("Vtables found: 0.\n100% match.\n") == 0
    with pytest.raises(ValueError, match="entity count"):
        vtable_count("100% match.\n")


def test_one_report_process_receives_every_selected_address(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    seen = []

    def fake_run(command, *, cwd):
        seen.append((command, cwd))
        report = Path(command[command.index("--json") + 1])
        report.write_text('{"format": 1, "data": []}', encoding="utf-8")

    monkeypatch.setattr(reccmp_workflows, "run", fake_run)

    assert run_report(tmp_path, "WIZ8", original_addresses=[0x401010, 0x401000]) == []
    assert len(seen) == 1
    assert seen[0][0].count("--orig-address") == 2
    assert seen[0][1] == tmp_path / "build/decomp"


def test_vtable_workflow_refuses_zero_entity_success(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(
        reccmp_workflows,
        "run",
        lambda *args, **kwargs: SimpleNamespace(
            exit_status=0,
            stdout="Vtables found: 0.\n100% match.\n",
            stderr="",
        ),
    )

    with pytest.raises(RuntimeError, match="vacuous success"):
        compare_vtables(tmp_path, "WIZ8", None)


def test_compare_mismatch_acquires_one_report_and_includes_triage(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    calls = 0

    def fake_report(*_args, **_kwargs):
        nonlocal calls
        calls += 1
        return [
            _entity(
                0x401000,
                "mismatch",
                difference={"kind": "branch_target", "orig": {}, "recomp": {}},
            )
        ]

    monkeypatch.setattr(reccmp_workflows, "run_report", fake_report)
    monkeypatch.setattr(
        reccmp_workflows,
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

    result = reccmp_workflows.compare_selected(tmp_path, "WIZ8", [0x401000])

    assert calls == 1
    assert result["functions"][0]["difference"]["kind"] == "branch_target"
    assert result["functions"][0]["instruction_window"]["original"][0]["divergence"]
    assert "first divergence: branch_target" in reccmp_workflows.comparison_human(result)
