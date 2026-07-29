from pathlib import Path

from wiz8decomp.reccmp_data import render_wiz8_data_source

REPOSITORY = Path(__file__).resolve().parents[2]


def test_wiz8_data_source_is_a_deterministic_projection_of_reviewed_evidence() -> None:
    rendered = render_wiz8_data_source(REPOSITORY)

    assert rendered == render_wiz8_data_source(REPOSITORY)
    assert rendered.startswith("address|symbol|size|type\n")
    assert "00401000|__WinMainCRTStartup||library\n" in rendered
    assert "004011e0|WindowProc4011E0|844|function\n" in rendered
    assert "004addf0|" in rendered
