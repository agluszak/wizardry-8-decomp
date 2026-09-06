"""Unit tests for the read-only headless recovery host."""

from __future__ import annotations

from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.ghidra import recovery as recovery_host


def _settings(tmp_path: Path) -> Settings:
    install = tmp_path / "ghidra-install"
    (install / "support").mkdir(parents=True)
    (install / "support/analyzeHeadless").write_text("", encoding="utf-8")
    repo = tmp_path / "repo"
    script_dir = repo / "tools/ghidra-scripts"
    script_dir.mkdir(parents=True)
    (script_dir / "Wiz8Recover.java").write_text("class Wiz8Recover {}\n", encoding="utf-8")
    return Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": str(install),
            "WIZ8_INPUT_DIR": str(tmp_path / "inputs"),
            "WIZ8_WORK_DIR": str(tmp_path / "work"),
            "repo_dir": repo,
        }
    )


def test_recover_functions_rejects_empty_selection() -> None:
    with pytest.raises(ValueError, match="at least one"):
        recovery_host.recover_functions(object(), [])


def test_recover_passes_the_selected_program_target(monkeypatch: pytest.MonkeyPatch) -> None:
    import contextlib

    import wiz8decomp.ghidra.env as env_module
    import wiz8decomp.ghidra.query as query_module
    import wiz8decomp.source_index as source_index_module

    settings = SimpleNamespace(repo_dir=Path("/repo"), build_dir=Path("/repo/build"))
    monkeypatch.setattr(env_module, "open_program", lambda _s, _p: contextlib.nullcontext(object()))
    monkeypatch.setattr(source_index_module, "write_source_index", lambda _settings: {})
    monkeypatch.setattr(
        source_index_module,
        "project_targets",
        lambda _repository: {
            "WIZ8": {"filename": "Wiz8.exe", "hash": {"sha256": "a"}},
            "SURRENDER": {"filename": "sr.dll", "hash": {"sha256": "b"}},
        },
    )
    monkeypatch.setattr(query_module, "resolve_function_selectors", lambda _p, _s: [0x10003840])
    seen: list[str] = []
    monkeypatch.setattr(
        recovery_host,
        "_execute_script",
        lambda _settings, _program, _script, arguments: (
            seen.extend(str(value) for value in arguments) or {"program": "SR.dll", "exports": []}
        ),
    )

    recovery_host._recover(
        settings,
        ["0x10003840"],
        program_selector="wiz8--gog-base--sr--cec1caf85861",
        explain=False,
    )

    assert seen[seen.index("--target") + 1] == "SURRENDER"


def test_recover_refreshes_source_index_before_opening_program(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    import contextlib

    import wiz8decomp.ghidra.env as env_module
    import wiz8decomp.ghidra.query as query_module
    import wiz8decomp.source_index as source_index_module

    settings = SimpleNamespace(repo_dir=Path("/repo"), build_dir=Path("/repo/build"))
    events: list[str] = []
    monkeypatch.setattr(
        source_index_module,
        "write_source_index",
        lambda _settings: events.append("source-index") or {},
    )
    monkeypatch.setattr(
        source_index_module,
        "project_targets",
        lambda _repository: {"WIZ8": {"filename": "Wiz8.exe", "hash": {"sha256": "a"}}},
    )

    def open_program(_settings: object, _selector: str):
        events.append("open-program")
        return contextlib.nullcontext(object())

    monkeypatch.setattr(env_module, "open_program", open_program)
    monkeypatch.setattr(query_module, "resolve_function_selectors", lambda _p, _s: [0x401000])
    monkeypatch.setattr(
        recovery_host,
        "_execute_script",
        lambda *_args: {"program": "wiz8", "exports": []},
    )

    recovery_host._recover(settings, ["0x401000"], program_selector="wiz8", explain=False)

    assert events == ["source-index", "open-program"]


def test_explain_resolves_ranges_through_ghidra(monkeypatch: pytest.MonkeyPatch) -> None:
    import contextlib

    import wiz8decomp.ghidra.env as env_module
    import wiz8decomp.ghidra.query as query_module

    settings = SimpleNamespace(repo_dir=Path("/repo"))
    monkeypatch.setattr(env_module, "open_program", lambda _s, _p: contextlib.nullcontext(object()))
    monkeypatch.setattr(
        query_module, "resolve_function_selectors", lambda _p, _s: [0x00401000, 0x00401020]
    )
    seen: list[list[str]] = []
    monkeypatch.setattr(
        recovery_host,
        "_recover",
        lambda _settings, selections, **_kwargs: (
            seen.append(list(selections))
            or {
                "program": "wiz8",
                "selections": selections,
                "exports": [
                    {
                        "entry": "0x00401000",
                        "name": "Function401000",
                        "recovery": {
                            "emission_kind": "function",
                            "source_kind": "unrecovered",
                            "passes": [],
                            "defects": [],
                        },
                    }
                ],
            }
        ),
    )

    result = recovery_host.explain_function(settings, "0x401000:0x401020")

    assert seen == [["0x00401000", "0x00401020"]]
    assert result["selections"] == ["0x00401000", "0x00401020"]
    assert result["functions"][0]["entry"] == "0x00401000"
