import json
from pathlib import Path

from wiz8decomp.ghidra.fid import (
    _is_authoritative_fid_name,
    _normalize_seed_provenance,
    _provenance_for_domain,
    _verified_existing_fid_summary,
)
from wiz8decomp.paths import sha256_file


def test_fid_name_filter_rejects_unstable_coff_local_labels() -> None:
    assert not _is_authoritative_fid_name("$L264")
    assert not _is_authoritative_fid_name("$L16506")
    assert not _is_authoritative_fid_name("FUN_00528a80")
    assert not _is_authoritative_fid_name("LAB_00528a80")
    assert not _is_authoritative_fid_name("thunk_FUN_00528a80")
    assert not _is_authoritative_fid_name("thunk__alldiv")
    assert _is_authoritative_fid_name("__alldiv")
    assert _is_authoritative_fid_name("??_M@YGXPAXIHP6EX0@Z@Z")


def test_fid_provenance_resolves_program_from_domain_path() -> None:
    record = {"toolchain": "vc6-sp5", "source_kind": "cmake-object-library"}
    provenance = {"fid--vc6-sp5--jpeg--unit--0123456789ab": record}
    assert _provenance_for_domain(provenance, "/fid--vc6-sp5--jpeg--unit--0123456789ab") == record


def test_fid_database_is_reused_only_for_matching_verified_inputs(tmp_path: Path) -> None:
    database = tmp_path / "static.fidb"
    database.write_bytes(b"packed database")
    summary = {"input_sha256": "inputs", "sha256": sha256_file(database)}
    database.with_suffix(".json").write_text(json.dumps(summary), encoding="utf-8")
    assert _verified_existing_fid_summary(database, "inputs") == summary
    assert _verified_existing_fid_summary(database, "different") is None
    database.write_bytes(b"tampered")
    assert _verified_existing_fid_summary(database, "inputs") is None


def test_fid_provenance_normalizes_shared_group_identity() -> None:
    common = {
        "toolchain": "vc6-sp5",
        "toolchain_commit": "a" * 40,
        "library": "jpeg",
        "variant": "release",
        "source_kind": "cmake-object-library",
        "source": {"archive_sha256": "b" * 64, "source_tree_hash": "c" * 64},
    }
    groups, programs = _normalize_seed_provenance(
        [
            {**common, "program": "one", "object_path": "one.obj", "object_sha256": "1"},
            {**common, "program": "two", "object_path": "two.obj", "object_sha256": "2"},
        ]
    )
    assert list(groups) == ["vc6-sp5/jpeg/release"]
    assert programs["one"] == {
        "seed_group": "vc6-sp5/jpeg/release",
        "object_path": "one.obj",
        "object_sha256": "1",
    }
