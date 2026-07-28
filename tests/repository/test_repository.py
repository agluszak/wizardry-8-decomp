from pathlib import Path

import pytest
from wiz8decomp.evidence.validate import validate_repository

pytestmark = pytest.mark.repository


def test_repository_evidence_is_valid() -> None:
    repository = Path(__file__).resolve().parents[2]
    assert validate_repository(repository)["ok"] is True
