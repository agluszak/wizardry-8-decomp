"""The provenance anchor collector, over the repository's real ledger."""

from __future__ import annotations

from pathlib import Path

from wiz8decomp.ghidra.apply_provenance import _collect_anchors

REPOSITORY = Path(__file__).resolve().parents[2]


def test_every_reviewed_channel_lands_at_its_natural_anchor() -> None:
    anchors = _collect_anchors(REPOSITORY)

    # A reviewed-exact function carries both its identity row and its boundary
    # row, and the boundary's exactness wins the layer.
    monster = anchors["004e5550"]
    assert monster["layer"] == "exact"
    assert any("functions.csv" in c for c in monster["citations"])
    assert any("wiz8-gameplay-boundaries.csv" in c for c in monster["citations"])

    # A reviewed vtable is anchored at the table itself, and the class fact
    # whose primary vtable it is lands at the same address.
    grcycle = anchors["005ece78"]
    assert any("vtables.csv" in c for c in grcycle["citations"])
    assert any("classes.csv" in c for c in grcycle["citations"])

    # Facts without an address anchor are not forced into one.
    assert all(address.strip() for address in anchors)
