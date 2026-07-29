import re
from pathlib import Path

import pytest
from wiz8decomp import repository
from wiz8decomp.repository import RepositoryHygieneError, validate_repository_hygiene

REPOSITORY = Path(__file__).resolve().parents[2]


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


def test_first_party_layout_checks_use_static_assert() -> None:
    legacy_assertion = re.compile(
        r"typedef\s+char\s+\w+\s*\[.*?\?\s*1\s*:\s*-1\s*\]\s*;",
        re.DOTALL,
    )
    offenders: list[str] = []

    for root in (REPOSITORY / "include", REPOSITORY / "src"):
        for source in root.rglob("*"):
            if source.suffix.lower() not in {".h", ".hpp", ".c", ".cpp"}:
                continue
            if legacy_assertion.search(source.read_text(encoding="utf-8")):
                offenders.append(str(source.relative_to(REPOSITORY)))

    assert offenders == []
