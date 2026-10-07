"""Focused tests for the PR comparison header checks."""

from __future__ import annotations

from wiz8decomp.reports.comparison_delta import (
    header_regression_candidates,
)


def test_header_candidates_group_regressions_by_directly_included_headers() -> None:
    def row(address: str, outcome: str, path: str) -> dict:
        return {"orig": address, "name": address, "outcome": outcome, "source": {"path": path}}

    base = {
        "functions": [
            row("0x1", "no-differences", "/r/a.cpp"),
            row("0x2", "no-differences", "/r/b.cpp"),
            row("0x3", "differences", "/r/a.cpp"),
            row("0x4", "no-differences", "/r/c.cpp"),
            row("0x5", "no-differences", "/r/a.cpp"),
        ]
    }
    head = {
        "functions": [
            row("0x1", "differences", "/r/a.cpp"),
            row("0x2", "no-differences", "/r/b.cpp"),
            row("0x3", "differences", "/r/a.cpp"),
            row("0x4", "differences", "/r/c.cpp"),
            row("0x5", "differences", "/r/a.cpp"),
        ]
    }

    candidates = header_regression_candidates(
        head,
        base,
        {"include/x.h": {"/r/a.cpp"}, "include/y.h": {"/r/a.cpp", "/r/b.cpp"}},
    )

    assert candidates == {
        "newly_different": 3,
        "without_direct_changed_header": 1,
        "groups": [
            {
                "headers": ["include/x.h", "include/y.h"],
                "newly_different": 2,
                "representatives": [
                    {"orig": "0x1", "name": "0x1"},
                    {"orig": "0x5", "name": "0x5"},
                ],
            }
        ],
    }
