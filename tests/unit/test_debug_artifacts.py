from __future__ import annotations

import csv
from pathlib import Path

from wiz8decomp.debug_artifacts import (
    _parse_7z_slt,
    _printable_strings,
    extract_debug_records,
    infer_build_configuration,
)


def test_debug_record_extraction_requires_path_shape_for_short_source_extensions() -> None:
    records = extract_debug_records(
        [
            (
                "ascii",
                r"53.h abc.c setnewh.cpp C:\work\Release\JPEG.pdb ..\Engine Code\Monster.cpp",
            )
        ]
    )

    values = {row["value"] for row in records}
    assert "53.h" not in values
    assert "abc.c" not in values
    assert "setnewh.cpp" in values
    assert r"C:\work\Release\JPEG.pdb" in values
    assert r"..\Engine Code\Monster.cpp" in values


def test_configuration_inference_uses_only_embedded_path_and_flag_evidence() -> None:
    records = extract_debug_records(
        [
            ("ascii", r"C:\build\Release\srVP_KNI.pdb"),
            (
                "ascii",
                "Intel(R) C/C++ Compiler Version 4.5 00015 kniICL.cpp : -Zi -O2 -Ob2 -MD",
            ),
        ]
    )

    configuration, evidence = infer_build_configuration(records)

    assert configuration == "optimized-with-debug-info"
    assert "optimization flag" in evidence
    assert "Release component" in evidence


def test_7z_listing_keeps_encrypted_files_and_omits_folders() -> None:
    listing = """archive header
----------
Path = source
Folder = +
Size = 0

Path = source\\Thing.cpp
Folder = -
Size = 123
Encrypted = +

Path = Tool.exe
Folder = -
Size = 456
Encrypted = -
"""

    assert _parse_7z_slt(listing) == [
        {"Path": r"source\Thing.cpp", "Folder": "-", "Size": "123", "Encrypted": "+"},
        {"Path": "Tool.exe", "Folder": "-", "Size": "456", "Encrypted": "-"},
    ]


def test_mapped_pe_scan_excludes_appended_installer_payload(synthetic_pe: Path) -> None:
    with synthetic_pe.open("ab") as stream:
        stream.write(b"C:\\compressed-noise\\53.h\0")

    records = extract_debug_records(_printable_strings(synthetic_pe, mapped_only=True))

    assert not records


def test_tracked_snapshot_accounts_for_every_container_and_binary() -> None:
    snapshot = Path(__file__).resolve().parents[2] / "evidence" / "snapshots" / "debug-artifacts"
    with (snapshot / "containers.csv").open(newline="", encoding="utf-8") as stream:
        containers = list(csv.DictReader(stream))
    with (snapshot / "binaries.csv").open(newline="", encoding="utf-8") as stream:
        binaries = list(csv.DictReader(stream))
    with (snapshot / "container-members.csv").open(newline="", encoding="utf-8") as stream:
        artifact_members = list(csv.DictReader(stream))

    assert len(containers) == 7
    assert {row["artifact_member_count"] for row in containers} == {"0"}
    assert artifact_members == []
    assert len(binaries) == 99
    assert {row["scan_status"] for row in binaries} == {"scanned"}
