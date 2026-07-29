from pathlib import Path

from wiz8decomp.repository import validate_repository_hygiene

REPOSITORY = Path(__file__).resolve().parents[2]


def test_checked_in_repository_is_hygienic() -> None:
    assert validate_repository_hygiene(REPOSITORY)["ok"] is True
