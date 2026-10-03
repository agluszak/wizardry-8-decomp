import csv
from pathlib import Path

from wiz8decomp.evidence.schema import schema_for
from wiz8decomp.reports.retail_bugs import retail_bug_report


def test_report_uses_only_reviewed_bug_claims_without_changing_evidence(tmp_path: Path) -> None:
    evidence = tmp_path / "evidence/reviewed/wiz8/claims.csv"
    evidence.parent.mkdir(parents=True)
    fields = schema_for("claims.csv").columns
    row = dict.fromkeys(fields, "")
    row.update(
        claim_id="retail-bug:wiz8:00400000:guard",
        program="wiz8",
        entity_kind="function",
        entity_key="00400000",
        predicate="retail-bug",
        value="Guard",
        origin="retail-instructions",
        details="Tests x | y before a\nstore.",
        reference="retail:00400010",
    )
    unrelated = {**row, "claim_id": "identity:wiz8:00400000", "predicate": "accepted-identity"}
    foreign = {**row, "claim_id": "retail-bug:sr:10000000", "program": "sr"}
    with evidence.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows([row, unrelated, foreign])
    original = evidence.read_bytes()

    result = retail_bug_report(tmp_path, tmp_path / "build/reports/retail-bugs")

    assert result["claims"] == 1
    report = tmp_path / "build/reports/retail-bugs/report.md"
    assert result["report"] == str(report)
    rendered = report.read_text(encoding="utf-8")
    assert rendered.count("| Guard |") == 1
    assert "0x00400000" in rendered
    assert "Tests x \\| y before a<br>store." in rendered
    assert "retail:00400010" in rendered
    assert evidence.read_bytes() == original
