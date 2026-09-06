from pathlib import Path

import pytest
from wiz8decomp import repository
from wiz8decomp.repository import RepositoryHygieneError, validate_repository_hygiene


def test_repository_hygiene_rejects_tracked_build_artifacts(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    artifact = tmp_path / "build/game/Wiz8.exe"
    artifact.parent.mkdir(parents=True)
    artifact.write_bytes(b"MZ")
    monkeypatch.setattr(repository, "tracked_paths", lambda _: [artifact])

    with pytest.raises(RepositoryHygieneError, match="generated/editor directory"):
        validate_repository_hygiene(tmp_path)


def test_repository_hygiene_allows_the_reviewed_ghidra_checkpoint(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    checkpoint = tmp_path / "vendor/ghidra/exports/wiz8.gzf"
    checkpoint.parent.mkdir(parents=True)
    checkpoint.write_bytes(b"reviewed checkpoint")
    monkeypatch.setattr(repository, "tracked_paths", lambda _: [checkpoint])

    assert validate_repository_hygiene(tmp_path)["ok"] is True
