"""Semantic checks for the recovered sight threshold producers."""

from __future__ import annotations

import json
import subprocess
from pathlib import Path


def test_sight_functions_compare_as_recovered_functions() -> None:
    result = subprocess.run(
        ["uv", "run", "wiz8", "compare", "0x005058A0", "0x00505A40"],
        cwd=Path(__file__).resolve().parents[2],
        check=True,
        capture_output=True,
        text=True,
    )
    payload = json.loads(result.stdout)
    names = {entry["name"] for entry in payload["functions"]}
    assert names == {"CanMonsterSeeMonster", "ComputeSightThreshold"}
    assert payload["missing"] == 0
