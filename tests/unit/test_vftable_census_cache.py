"""Tests for census vftable slot-count cache keying."""

from __future__ import annotations

from pathlib import Path

from wiz8decomp import vftable_typing


def test_census_cache_keys_by_content_hash(tmp_path: Path, monkeypatch) -> None:
    vftable_typing._CENSUS_SLOT_CACHE.clear()
    binary = tmp_path / "Wiz8.exe"
    binary.write_bytes(b"binary-a")
    scans: list[Path] = []

    monkeypatch.setattr(
        vftable_typing,
        "_canonical_matching_binary",
        lambda _repo, _work: binary,
    )
    monkeypatch.setattr(
        "wiz8decomp.msvc_tables.scan_msvc_tables",
        lambda path, repo_dir=None: (
            scans.append(path) or {"vftables": [{"address": "0x00500000", "slot_count": 3}]}
        ),
    )

    first = vftable_typing.census_vftable_slot_counts(tmp_path, tmp_path)
    second = vftable_typing.census_vftable_slot_counts(tmp_path, tmp_path)
    assert first == {0x500000: 3}
    assert second == first
    assert len(scans) == 1

    binary.write_bytes(b"binary-b-changed")
    monkeypatch.setattr(
        "wiz8decomp.msvc_tables.scan_msvc_tables",
        lambda path, repo_dir=None: (
            scans.append(path) or {"vftables": [{"address": "0x00500000", "slot_count": 7}]}
        ),
    )
    third = vftable_typing.census_vftable_slot_counts(tmp_path, tmp_path)
    assert third == {0x500000: 7}
    assert len(scans) == 2
