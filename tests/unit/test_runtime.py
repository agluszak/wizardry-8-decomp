from pathlib import Path

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.runtime import stage_runtime


def _settings(tmp_path: Path) -> Settings:
    repo = tmp_path / "repo"
    work = tmp_path / "work"
    for name in ("Data", "Dll", "Levels"):
        (work / "variants" / "gog-base" / name).mkdir(parents=True, exist_ok=True)
    (repo / "build" / "decomp").mkdir(parents=True)
    (repo / "build" / "decomp" / "Wiz8Runtime.exe").write_bytes(b"runtime")
    (repo / "config" / "runtime").mkdir(parents=True)
    (repo / "config" / "runtime" / "3DVideo.CFG").write_text("video")
    (repo / "config" / "runtime" / "Wiz8.CFG.hex").write_text("00ff")
    return Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": tmp_path / "ghidra",
            "WIZ8_INPUT_DIR": tmp_path / "inputs",
            "WIZ8_WORK_DIR": work,
            "repo_dir": repo,
        }
    )


def test_stage_runtime_uses_managed_links_and_materialized_cfg(tmp_path: Path) -> None:
    settings = _settings(tmp_path)

    result = stage_runtime(settings)
    stage = Path(result["stage"])

    assert (stage / "Data").is_symlink()
    assert (stage / "Wiz8.CFG").read_bytes() == b"\x00\xff"
    assert (stage / "Wiz8Runtime.exe").read_bytes() == b"runtime"


def test_stage_runtime_refuses_an_unmanaged_asset_directory(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    unmanaged = settings.repo_dir / "build" / "runtime" / "wiz8" / "Data"
    unmanaged.mkdir(parents=True)

    with pytest.raises(RuntimeError, match="not a managed symlink"):
        stage_runtime(settings)
