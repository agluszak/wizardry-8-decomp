from pathlib import Path

from wiz8decomp.repository import validate_repository_hygiene
from wiz8decomp.source_policy import validate_source_policy

REPOSITORY = Path(__file__).resolve().parents[2]


def test_checked_in_repository_is_hygienic() -> None:
    assert validate_repository_hygiene(REPOSITORY)["ok"] is True


def test_checked_in_source_uses_compiler_owned_lifecycle_artifacts() -> None:
    assert validate_source_policy(REPOSITORY)["ok"] is True
