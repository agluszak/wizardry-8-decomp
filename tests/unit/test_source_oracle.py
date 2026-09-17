from __future__ import annotations

import json
from pathlib import Path

import pytest
from wiz8decomp.source_oracle import (
    OracleFamily,
    SourceOracleGateError,
    contribution_hulls,
    proven_oracle_symbols,
    source_oracle_violations,
    validate_source_oracle_ownership,
)


def _write_index(repo: Path, markers: list[dict], declarations: list[dict] | None = None) -> None:
    build = repo / "build"
    build.mkdir(exist_ok=True)
    (build / "source-index.json").write_text(
        json.dumps(
            {
                "schema": "reccmp-source-index-v2",
                "markers": markers,
                "declarations": declarations or [],
                "classes": [],
                "variables": [],
                "conflicts": [],
            }
        ),
        encoding="utf-8",
    )


def _write_claims(repo: Path, rows: list[dict[str, str]]) -> None:
    path = repo / "evidence/reviewed/wiz8/claims.csv"
    path.parent.mkdir(parents=True, exist_ok=True)
    header = (
        "claim_id,program,entity_kind,entity_key,predicate,value,origin,"
        "authority,confidence,reference,details\n"
    )
    body = "".join(
        ",".join(
            (
                row["claim_id"],
                "wiz8",
                "function",
                row["entity_key"],
                row["predicate"],
                row["value"],
                row["origin"],
                row.get("authority", "source-backed"),
                row.get("confidence", "strong"),
                row.get("reference", "test"),
                row.get("details", '""'),
            )
        )
        + "\n"
        for row in rows
    )
    path.write_text(header + body, encoding="utf-8")


def _marker(
    address: int,
    source: str,
    *,
    kind: str = "FUNCTION",
    name: str = "OracleFn",
    target: str = "WIZ8",
) -> dict:
    return {
        "address": address,
        "marker_kind": kind,
        "source_file": source,
        "line": 10,
        "declaration": None,
        "marker_name": name,
        "folded": False,
        "target": target,
    }


def _declaration(
    name: str,
    source: str,
    *,
    line: int,
    end_line: int | None = None,
    is_definition: bool = False,
) -> dict:
    return {
        "qualified_name": name,
        "semantic_id": "",
        "semantic_kind": "free_function",
        "calling_convention": "__cdecl",
        "return_type": "void",
        "parameter_types": [],
        "has_this": False,
        "source_file": source,
        "line": line,
        "end_line": end_line or line,
        "is_definition": is_definition,
        "target": "WIZ8",
    }


def test_misplaced_wiz8_function_in_sgp_hull_fails(tmp_path: Path) -> None:
    _write_claims(tmp_path, [])
    _write_index(
        tmp_path,
        [
            _marker(0x405000, "src/sgp/input.c", name="First"),
            _marker(0x405200, "src/sgp/input.c", name="Last"),
            _marker(0x405100, "src/wiz8/local_code/Wrong.cpp", name="Stolen"),
        ],
    )

    violations = source_oracle_violations(tmp_path)

    assert [item["kind"] for item in violations] == ["misplaced-oracle-function"]
    assert violations[0]["address"] == "0x00405100"
    assert violations[0]["family"] == "sgp"
    assert "src/wiz8/local_code/Wrong.cpp" in violations[0]["detail"]


def test_function_placeholder_in_sgp_space_fails(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/local_code/Wrong.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "/* 0x00405100 */\nvoid Function405100(void);\n",
        encoding="utf-8",
    )
    _write_claims(tmp_path, [])
    _write_index(
        tmp_path,
        [
            _marker(0x405000, "src/sgp/input.c", name="First"),
            _marker(0x405200, "src/sgp/input.c", name="Last"),
        ],
        [_declaration("Function405100", "src/wiz8/local_code/Wrong.cpp", line=2)],
    )

    violations = source_oracle_violations(tmp_path)

    assert [item["kind"] for item in violations] == ["oracle-placeholder"]
    assert "recover from the oracle" in violations[0]["detail"]


def test_sgp_source_claim_requires_src_sgp_marker(tmp_path: Path) -> None:
    _write_claims(
        tmp_path,
        [
            {
                "claim_id": "function-source:wiz8:00405000:CreateStack",
                "entity_key": "00405000",
                "predicate": "accepted-identity",
                "value": "CreateStack",
                "origin": "sgp-source",
            }
        ],
    )
    _write_index(
        tmp_path, [_marker(0x405000, "src/wiz8/engine_code/Wrong.cpp", name="CreateStack")]
    )

    violations = source_oracle_violations(tmp_path)
    kinds = {item["kind"] for item in violations}
    assert "missing-oracle-owner" in kinds
    assert "misplaced-oracle-function" in kinds


def test_retail_folded_sgp_source_does_not_require_sgp_marker(tmp_path: Path) -> None:
    _write_claims(
        tmp_path,
        [
            {
                "claim_id": "retail-folded:wiz8:004023a0:UnlockMouseBuffer",
                "entity_key": "004023a0",
                "predicate": "retail-folded-noop",
                "value": "UnlockMouseBuffer",
                "origin": "sgp-source",
            }
        ],
    )
    _write_index(tmp_path, [])

    assert source_oracle_violations(tmp_path) == []


def test_contribution_hulls_span_proven_starts(tmp_path: Path) -> None:
    _write_claims(tmp_path, [])
    _write_index(
        tmp_path,
        [
            _marker(0x405000, "src/sgp/input.c", name="First"),
            _marker(0x405200, "src/sgp/input.c", name="Last"),
            _marker(0x406000, "src/sgp/soundman.c", name="Only"),
            # LIBRARY dump file must not form an image-wide hull.
            _marker(
                0x401000, "src/wiz8/vc6_runtime.cpp", kind="LIBRARY", name="__WinMainCRTStartup"
            ),
            _marker(0x5E1C30, "src/wiz8/vc6_runtime.cpp", kind="LIBRARY", name="__aulldiv"),
        ],
    )
    symbols = proven_oracle_symbols(tmp_path)
    hulls = contribution_hulls(symbols)

    assert {(hull.source_file, hull.start, hull.end) for hull in hulls} == {
        ("src/sgp/input.c", 0x405000, 0x405200),
        ("src/sgp/soundman.c", 0x406000, 0x406000),
    }
    assert {item.family for item in symbols} >= {"sgp", "msvc-runtime"}


def test_crt_range_rejects_wiz8_function(tmp_path: Path) -> None:
    _write_claims(tmp_path, [])
    _write_index(
        tmp_path,
        [
            _marker(0x5E1C30, "src/wiz8/vc6_runtime.cpp", kind="LIBRARY", name="__aulldiv"),
            _marker(0x5E1C50, "src/wiz8/local_code/Wrong.cpp", name="FakeDiv"),
        ],
    )

    violations = source_oracle_violations(tmp_path)

    assert len(violations) == 1
    assert violations[0]["kind"] == "misplaced-oracle-function"
    assert violations[0]["family"] == "msvc-runtime"
    assert violations[0]["address"] == "0x005e1c50"


def test_zlib_range_rejects_wiz8_function(tmp_path: Path) -> None:
    _write_claims(tmp_path, [])
    _write_index(
        tmp_path,
        [_marker(0x416000, "src/wiz8/engine_code/Wrong.cpp", name="FakeInflate")],
    )

    violations = source_oracle_violations(tmp_path)

    assert violations[0]["family"] == "zlib"
    assert violations[0]["kind"] == "misplaced-oracle-function"


def test_fid_claim_requires_library_marker(tmp_path: Path) -> None:
    _write_claims(
        tmp_path,
        [
            {
                "claim_id": "function-claim:wiz8:005e1c30:fid-variants",
                "entity_key": "005e1c30",
                "predicate": "fid-variants",
                "value": "rtm|sp3|sp4|sp5|sp6",
                "origin": "fid",
                "authority": "",
                "confidence": "",
            }
        ],
    )
    _write_index(tmp_path, [])

    violations = source_oracle_violations(tmp_path)

    assert [item["kind"] for item in violations] == ["missing-oracle-owner"]
    assert violations[0]["family"] == "msvc-runtime"


def test_fid_claim_satisfied_by_library_marker(tmp_path: Path) -> None:
    _write_claims(
        tmp_path,
        [
            {
                "claim_id": "function-claim:wiz8:005e1c30:fid-variants",
                "entity_key": "005e1c30",
                "predicate": "fid-variants",
                "value": "rtm|sp3|sp4|sp5|sp6",
                "origin": "fid",
                "authority": "",
                "confidence": "",
            }
        ],
    )
    _write_index(
        tmp_path,
        [_marker(0x5E1C30, "src/wiz8/vc6_runtime.cpp", kind="LIBRARY", name="__aulldiv")],
    )

    assert source_oracle_violations(tmp_path) == []


def test_validate_writes_artifact_and_passes_clean_repo(tmp_path: Path) -> None:
    _write_claims(tmp_path, [])
    _write_index(tmp_path, [_marker(0x405000, "src/sgp/input.c")])

    result = validate_source_oracle_ownership(tmp_path)

    assert result["ok"] is True
    assert (tmp_path / "build/reports/source-oracle.json").is_file()


def test_validate_raises_on_violations(tmp_path: Path) -> None:
    _write_claims(tmp_path, [])
    _write_index(
        tmp_path,
        [
            _marker(0x405000, "src/sgp/input.c"),
            _marker(0x405000, "src/wiz8/Wrong.cpp", name="Dup"),
        ],
    )

    with pytest.raises(SourceOracleGateError, match="source-oracle ownership gate failed"):
        validate_source_oracle_ownership(tmp_path)


def test_oracle_family_registry_is_extensible() -> None:
    family = OracleFamily(
        name="jpeg",
        target="SREXT_JPEGIMPORTER",
        source_roots=("vendor/jpeg/",),
        name_origins=frozenset({"original-source"}),
    )
    assert family.owns_source("vendor/jpeg/jdmarker.c")
    assert not family.owns_source("src/sgp/Compression.c")


def test_reccmp_csv_library_rejects_function(tmp_path: Path) -> None:
    csv_path = tmp_path / "config/reccmp/srext-unzip.csv"
    csv_path.parent.mkdir(parents=True)
    csv_path.write_text(
        "address|name|size|type\n"
        "10006e60|inflate_codes|1254|library\n"
        "10001000|setFileNotFound|11|function\n",
        encoding="utf-8",
    )
    _write_claims(tmp_path, [])
    _write_index(
        tmp_path,
        [
            _marker(
                0x10006E60,
                "src/srext_unzip/wrong.c",
                name="inflate_codes",
                target="SREXT_UNZIP",
            )
        ],
    )
    family = OracleFamily(
        name="infozip-unzip",
        target="SREXT_UNZIP",
        reccmp_csv="config/reccmp/srext-unzip.csv",
    )

    violations = source_oracle_violations(tmp_path, families=(family,))

    assert len(violations) == 1
    assert violations[0]["family"] == "infozip-unzip"
    assert violations[0]["kind"] == "misplaced-oracle-function"


def test_iat_thunk_function_is_rejected(tmp_path: Path) -> None:
    import struct

    # Minimal PE32: ImageBase 0x400000, .text at VA 0x5000 containing ff 25 ....
    dos = bytearray(0x80)
    dos[0:2] = b"MZ"
    struct.pack_into("<I", dos, 0x3C, 0x80)
    pe = bytearray()
    pe += b"PE\0\0"
    pe += struct.pack("<HHIIIHH", 0x14C, 1, 0, 0, 0, 0xE0, 0x103)
    opt = bytearray(0xE0)
    struct.pack_into("<H", opt, 0, 0x10B)
    struct.pack_into("<I", opt, 28, 0x400000)
    struct.pack_into("<I", opt, 56, 0x1000)
    struct.pack_into("<I", opt, 60, 0x200)
    pe += opt
    sect = bytearray(40)
    sect[0:5] = b".text"
    struct.pack_into("<IIII", sect, 8, 0x200, 0x5000, 0x200, 0x200)
    pe += sect
    blob = bytearray(0x400)
    blob[0:0x80] = dos
    blob[0x80 : 0x80 + len(pe)] = pe
    blob[0x200:0x206] = b"\xff\x25\x11\x22\x33\x44"
    pe_path = tmp_path / ".wiz8-work/extracted/gog-base/Wiz8.exe"
    pe_path.parent.mkdir(parents=True)
    pe_path.write_bytes(blob)

    _write_claims(tmp_path, [])
    # 0x405000 is outside CRT/zlib ranges so only the IAT check should fire.
    _write_index(tmp_path, [_marker(0x405000, "src/wiz8/Wrong.cpp", name="FakeThunk")])

    violations = source_oracle_violations(tmp_path)

    assert [item["kind"] for item in violations] == ["iat-thunk-function"]
    assert violations[0]["address"] == "0x00405000"
