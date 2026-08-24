from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[2]
DELETED_HEADER = REPOSITORY / "include/wiz8/gameplay_boundaries.h"


def test_gameplay_boundary_quarantine_stays_deleted() -> None:
    assert not DELETED_HEADER.exists()

    offenders = []
    for root in ("include", "src", "tests/runtime"):
        for path in (REPOSITORY / root).rglob("*"):
            if path.suffix not in {".c", ".cpp", ".h"}:
                continue
            if '"wiz8/gameplay_boundaries.h"' in path.read_text(encoding="utf-8", errors="replace"):
                offenders.append(str(path.relative_to(REPOSITORY)))

    assert not offenders, "deleted gameplay boundary included by: " + ", ".join(offenders)
