"""The whole-program fact store: seeding, provenance, determinism."""

from __future__ import annotations

from pathlib import Path

import pytest
from wiz8decomp.facts import build_store, content_digest, why

REPOSITORY = Path(__file__).resolve().parents[2]
CANONICAL = "wiz8--gog-base--wiz8--18a74ff61c65"


class _Settings:
    repo_dir = REPOSITORY


def _build(tmp_path: Path, name: str) -> tuple[Path, dict]:
    destination = tmp_path / name
    summary = build_store(_Settings(), destination)  # type: ignore[arg-type]
    return destination, summary


def test_every_evidence_channel_lands_and_rebuilds_are_deterministic(tmp_path: Path) -> None:
    first, summary = _build(tmp_path, "a.sqlite")
    second, _ = _build(tmp_path, "b.sqlite")

    # Every canonical reviewed function is addressable as a fact, and each
    # imported channel contributed.
    kinds = summary["kinds"]
    assert kinds["function-identity"] >= 390
    for kind in (
        "calls",
        "installs-vptr",
        "vtable-observed",
        "imported-member",
        "matches-source-body",
        "asserts-in-unit",
        "eh-frame",
        "has-field-at-offset",
    ):
        assert kinds.get(kind), kind

    # The same inputs must produce the same content, or the store cannot be
    # treated as disposable.
    assert content_digest(first) == content_digest(second)


def test_a_derived_candidate_answers_why_with_its_actual_inputs(tmp_path: Path) -> None:
    store, summary = _build(tmp_path, "c.sqlite")
    assert summary["derived"] > 250

    answer = why(_Settings(), f"candidate:{CANONICAL}:005ec078", store=store)  # type: ignore[arg-type]
    candidate = next(f for f in answer["facts"] if f["kind"] == "candidate-class")
    parents = candidate["derived_from"]

    # The chain names the vtable observation and the vptr-write observations
    # this hypothesis rests on - including the census's misattributed writer,
    # which is exactly what a later correction must be able to find.
    assert any(p["kind"] == "vtable-observed" for p in parents)
    writers = {p["subject"] for p in parents if p["kind"] == "installs-vptr"}
    assert f"function:{CANONICAL}:004397d0" in writers
    assert all(p["source"].startswith("evidence/") for p in parents)


def test_unknown_subjects_are_an_error_not_an_empty_answer(tmp_path: Path) -> None:
    store, _ = _build(tmp_path, "d.sqlite")
    with pytest.raises(ValueError, match="no facts"):
        why(_Settings(), "type:Nonexistent", store=store)  # type: ignore[arg-type]
