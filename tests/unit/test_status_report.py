from pathlib import Path
from types import SimpleNamespace

from wiz8decomp.reports.status import derive_status, status_report


def _data_rows(path: Path) -> int:
    return len(path.read_text(encoding="utf-8").splitlines()) - 1


def test_status_is_derived_from_canonical_evidence() -> None:
    repository = Path(__file__).resolve().parents[2]
    report = derive_status(repository)

    assert report["schema"] == "wiz8.recovery-status"
    assert {item["program"] for item in report["programs"]} == {
        "cfagent-128",
        "srext-jpegimporter",
        "srext-unzip",
        "wiz8",
    }
    assert report["wiz8"]["function_identities"] == _data_rows(
        repository / "evidence/reviewed/wiz8/functions.csv"
    )
    assert report["wiz8"]["function_evidence"] == _data_rows(
        repository / "evidence/reviewed/wiz8/function-evidence.csv"
    )
    assert report["wiz8"]["source_units"] == _data_rows(
        repository / "evidence/observations/wiz8/source-tree.csv"
    )
    assert report["wiz8"]["gameplay"]["functions"] == _data_rows(
        repository / "config/reccmp/wiz8-gameplay-boundaries.csv"
    )
    assert report["wiz8"]["gameplay"]["unresolved_matches"] > 0


def test_status_report_writes_json_and_markdown_under_build(tmp_path: Path) -> None:
    repository = Path(__file__).resolve().parents[2]
    settings = SimpleNamespace(repo_dir=repository, build_dir=tmp_path / "build")

    result = status_report(settings)

    assert result["outputs"] == ["build/reports/status.json", "build/reports/status.md"]
    assert (settings.build_dir / "reports/status.json").is_file()
    markdown = (settings.build_dir / "reports/status.md").read_text(encoding="utf-8")
    assert "# Wizardry recovery status" in markdown
    assert "Unresolved non-exact matches" in markdown
