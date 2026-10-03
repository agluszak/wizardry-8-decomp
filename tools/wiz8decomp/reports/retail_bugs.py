from __future__ import annotations

from pathlib import Path

from ..evidence.claims import load_claims
from ..paths import atomic_write


def _cell(value: str) -> str:
    return value.replace("\\", "\\\\").replace("|", "\\|").replace("\n", "<br>")


def retail_bug_report(repository: Path, output_dir: Path) -> dict[str, object]:
    claims = sorted(
        (row for row in load_claims(repository) if row["predicate"] == "retail-bug"),
        key=lambda row: (int(row["entity_key"], 16), row["claim_id"]),
    )
    lines = [
        "# Reviewed retail bugs",
        "",
        "Generated from evidence/reviewed/wiz8/claims.csv; edit the reviewed claims.",
        "These are binary behavior conclusions, not proof of original source spelling.",
        "",
        "| Function | Address | Behavior | Evidence |",
        "| --- | --- | --- | --- |",
    ]
    for claim in claims:
        cells = (
            claim["value"],
            f"0x{int(claim['entity_key'], 16):08X}",
            claim["details"],
            claim["reference"],
        )
        lines.append("| " + " | ".join(_cell(value) for value in cells) + " |")
    report = output_dir / "report.md"
    atomic_write(report, "\n".join(lines) + "\n")
    return {"claims": len(claims), "report": str(report)}
