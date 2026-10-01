"""Focused tests for the PR comparison allocator and header checks."""

from __future__ import annotations

from wiz8decomp.reports.comparison_delta import (
    allocator_call_disagreements,
    header_regression_candidates,
)


def _pair(address: str, retail: list[str], rebuild: list[str]) -> dict:
    return {
        "old": {"address": address, "name": f"f{address}", "called": retail},
        "new": {"address": address, "name": f"f{address}", "called": rebuild},
    }


def test_allocator_substitution_is_reported() -> None:
    """The old mixed-family growth bug allocated on CRT instead of srHeap."""
    ghidriff = {
        "functions": {
            "modified": [
                _pair(
                    "0x480560",
                    ["SR.DLL::srHeap::allocate", "srHeapBuffer::release"],
                    ["operator_new", "SR.DLL::srHeap::free"],
                ),
            ]
        }
    }

    assert allocator_call_disagreements(ghidriff) == {
        (0x480560, "allocate"): {
            "orig": "0x480560",
            "name": "f0x480560",
            "operation": "allocate",
            "retail": ["srHeap"],
            "rebuild": ["crt"],
        }
    }


def test_srheap_free_is_not_read_as_the_crt_free() -> None:
    ghidriff = {
        "functions": {"modified": [_pair("0x1", ["SR.DLL::srHeap::free"], ["srHeap::free"])]}
    }

    assert allocator_call_disagreements(ghidriff) == {}


def test_a_side_without_allocator_calls_is_not_a_disagreement() -> None:
    """One side reaches the allocator through a helper the other inlined."""
    ghidriff = {"functions": {"modified": [_pair("0x1", [], ["operator_new"])]}}

    assert allocator_call_disagreements(ghidriff) == {}


def test_allocator_operations_are_compared_independently() -> None:
    ghidriff = {
        "functions": {
            "modified": [
                _pair(
                    "0x2",
                    ["SR.DLL::srHeap::allocate", "operator_delete"],
                    ["operator_new", "SR.DLL::srHeap::free"],
                )
            ]
        }
    }

    assert allocator_call_disagreements(ghidriff) == {
        (0x2, "allocate"): {
            "orig": "0x2",
            "name": "f0x2",
            "operation": "allocate",
            "retail": ["srHeap"],
            "rebuild": ["crt"],
        },
        (0x2, "free"): {
            "orig": "0x2",
            "name": "f0x2",
            "operation": "free",
            "retail": ["crt"],
            "rebuild": ["srHeap"],
        },
    }


def test_matching_mixed_allocator_families_are_not_reported() -> None:
    calls = [
        "SR.DLL::srHeap::allocate",
        "operator_new",
        "SR.DLL::srHeap::free",
        "operator_delete",
    ]
    ghidriff = {"functions": {"modified": [_pair("0x3", calls, calls)]}}

    assert allocator_call_disagreements(ghidriff) == {}


def test_allocator_reached_through_normalized_inline_callee_is_not_a_disagreement() -> None:
    """Retail inlines the srHeap-freeing helper; the rebuild calls it."""
    ghidriff = {
        "functions": {
            "modified": [
                _pair(
                    "0x10",
                    ["SR.DLL::srHeap::free", "operator_delete"],
                    ["operator_delete", "clearBlocks"],
                )
            ]
        }
    }
    summary = {"functions": [{"orig": "0x10", "inline_callees": ["0x20"]}]}
    direct_calls = {
        "functions": [
            {
                "address": "0x10",
                "orig": {"calls": [{"identity": "pair:0x30", "name": "srHeap::free"}]},
                "recomp": {
                    "calls": [
                        {"identity": "pair:0x20", "name": "clearBlocks"},
                        {"identity": "pair:0x40", "name": "operator_delete"},
                    ]
                },
            },
            {
                "address": "0x20",
                "orig": {"calls": [{"identity": "pair:0x30", "name": "srHeap::free"}]},
                "recomp": {"calls": [{"identity": "pair:0x30", "name": "srHeap::free"}]},
            },
        ]
    }

    assert (0x10, "free") in allocator_call_disagreements(ghidriff)
    assert allocator_call_disagreements(ghidriff, summary, direct_calls) == {}


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
