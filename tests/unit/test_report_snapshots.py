from pathlib import Path
from types import SimpleNamespace
from typing import cast

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.reports.snapshots import csv_text, publish_report_snapshot


def _settings(root: Path) -> Settings:
    return cast(Settings, SimpleNamespace(repo_dir=root, build_dir=root / "build"))


def test_csv_text_uses_stable_header_order_and_line_endings() -> None:
    assert csv_text(["name", "value"], [{"value": 2, "name": "one"}]) == ("name,value\none,2\n")


def test_publish_report_snapshot_writes_reports_and_enforces_reviewed_copy(
    tmp_path: Path,
) -> None:
    settings = _settings(tmp_path)
    arguments = {
        "name": "sample",
        "outputs": {"rows.csv": "name\none\n"},
        "snapshot_files": ("rows.csv",),
        "snapshot_readme": "# Sample\n",
        "stale_error": "sample is stale",
    }

    with pytest.raises(RuntimeError, match="sample is stale"):
        publish_report_snapshot(settings, update_snapshot=False, **arguments)
    assert (tmp_path / "build/reports/sample/rows.csv").read_text() == "name\none\n"

    _, snapshot_dir, fresh = publish_report_snapshot(settings, update_snapshot=True, **arguments)
    assert fresh
    assert (snapshot_dir / "rows.csv").read_text() == "name\none\n"
    assert (snapshot_dir / "README.md").read_text() == "# Sample\n"

    _, _, fresh = publish_report_snapshot(settings, update_snapshot=False, **arguments)
    assert fresh
