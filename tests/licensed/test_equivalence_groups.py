"""Checks that require the prepared licensed Wizardry 8 corpus."""

from __future__ import annotations

import json
import subprocess
from pathlib import Path


def test_equivalence_groups_reprove_against_original_image() -> None:
    result = subprocess.run(
        ["uv", "run", "wiz8", "equivalence"],
        cwd=Path(__file__).resolve().parents[2],
        check=True,
        capture_output=True,
        text=True,
    )
    payload = json.loads(result.stdout)
    assert payload["count"] > 0
    assert {row["status"] for row in payload["rows"]} == {"equivalent"}
