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
