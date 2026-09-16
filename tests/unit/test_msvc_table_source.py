from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from wiz8decomp.msvc_table_source import annotate_source_identities


@dataclass(frozen=True)
class _Marker:
    name: str


def test_source_identity_overlay_names_slots_writes_and_families(
    monkeypatch, tmp_path: Path
) -> None:
    from wiz8decomp import source_index

    monkeypatch.setattr(
        source_index,
        "source_functions",
        lambda _repository, _target: {
            0x401000: _Marker("Foo::virtualCall"),
            0x402000: _Marker("Foo::Foo"),
        },
    )
    report = {
        "vftables": [
            {
                "address": "0x00500000",
                "slots": [
                    {
                        "target": "0x00401000",
                        "resolution": {"kind": "local-body", "shared_count": 1},
                    }
                ],
                "writes": [
                    {
                        "function": "0x00402000",
                        "instruction": "0x00402010",
                    }
                ],
            }
        ],
        "vbtables": [],
        "analysis": {
            "slot_resolution": {"local-body": 1},
            "construction_families": [
                {
                    "function": "0x00402000",
                    "transitions": [],
                }
            ],
        },
    }

    annotate_source_identities(report, tmp_path)

    resolution = report["vftables"][0]["slots"][0]["resolution"]
    assert resolution["kind"] == "source-function"
    assert resolution["source_name"] == "Foo::virtualCall"
    assert report["vftables"][0]["writes"][0]["function_source_name"] == "Foo::Foo"
    assert report["analysis"]["construction_families"][0]["function_source_name"] == "Foo::Foo"
    assert report["analysis"]["slot_resolution"] == {"source-function": 1}
    assert report["analysis"]["source_identity_overlay"] == {
        "status": "available",
        "source_functions": 2,
        "slot_matches": 1,
        "write_function_matches": 1,
        "construction_family_function_matches": 1,
    }


def test_source_identity_overlay_is_optional(monkeypatch, tmp_path: Path) -> None:
    from reccmp.source import SourceIndexError
    from wiz8decomp import source_index

    def unavailable(_repository, _target):
        raise SourceIndexError("source index missing")

    monkeypatch.setattr(source_index, "source_functions", unavailable)
    report = {"vftables": [], "vbtables": [], "analysis": {}}

    annotate_source_identities(report, tmp_path)

    overlay = report["analysis"]["source_identity_overlay"]
    assert overlay["status"] == "unavailable"
    assert "source index missing" in overlay["reason"]
