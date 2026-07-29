from pathlib import Path

from wiz8decomp.reccmp_lint import validate_reccmp_annotations

REPOSITORY = Path(__file__).resolve().parents[2]


def test_reccmp_consumer_accepts_the_checked_in_annotations() -> None:
    result = validate_reccmp_annotations(REPOSITORY)

    assert result["ok"] is True
    assert result["engine"] == "reccmp-decomplint"
    assert set(result["alerts"]) <= {"function_out_of_order"}
    assert result["waived"] == ["function_out_of_order"]
