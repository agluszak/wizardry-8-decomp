"""The status report is a pure projection of tracked evidence.

Every assertion here runs against a synthetic repository so that the test
states what the derivation *does* rather than restating whatever the current
evidence tables happen to contain. Live counts belong in the generated report,
never in a test.
"""

from __future__ import annotations

import json
from pathlib import Path

import pytest
from wiz8decomp.reports.status import derive_status

_UNIT = r"C:\Projects\Wizardry 8\Local Code\A.cpp"


def _ghidra_functions() -> list[dict[str, str]]:
    return [
        {"entry": f"0x{address:08x}", "name": name}
        for address, name in (
            (0x00401000, "Owned"),
            (0x00402000, "Foo"),
            (0x00403000, "Baz"),
            (0x00404000, "Bar"),
        )
    ]


def _write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


@pytest.fixture
def repository(tmp_path: Path) -> Path:
    """A minimal repository whose every status input is known by construction."""

    repo = tmp_path / "repo"
    _write(
        repo / "evidence/reviewed/cfagent-128/functions.csv",
        "program,address,provisional_name,owner,confidence,name_origin,authority,evidence\n"
        "cfagent-128,10004050,scan,,high,patch,fan-patch,seed\n"
        "cfagent-128,10004810,seed,,medium,patch,fan-patch,seed\n",
    )
    _write(
        repo / "evidence/reviewed/srext-unzip/functions.csv",
        "program,address,provisional_name,owner,confidence,name_origin,authority,evidence\n"
        "srext-unzip,10001000,unzip,,high,source,source-oracle,oracle\n",
    )
    _write(
        repo / "evidence/reviewed/wiz8/claims.csv",
        "claim_id,program,entity_kind,entity_key,predicate,value,origin,authority,confidence,"
        "reference,details\n"
        "c1,wiz8,function,00402000,accepted-identity,Foo,review,source-oracle,high,,\n"
        "c2,wiz8,function,00404000,accepted-identity,Bar,review,fan-patch,medium,,\n"
        "c3,wiz8,function,00402000,identity-provenance,Foo,review,source-oracle,high,,\n"
        "c4,wiz8,class,BitArray,accepted-identity,BitArray,review,source-oracle,high,,\n",
    )
    _write(
        repo / "evidence/observations/wiz8/source-tree.csv",
        "relative_path,subsystem,canonical_absolute_path,demo_absolute_path,variants\n"
        "Local Code/A.cpp,local_code,,,\n"
        "Engine Code/B.cpp,engine_code,,,\n",
    )
    _write(
        repo / "evidence/observations/wiz8/assertions.csv",
        "call_site,call_kind,containing_function,source_path,line,expression,message\n"
        f"00402010,direct,00402000,{_UNIT},10,x,m\n"
        f"00403010,direct,00403000,{_UNIT},20,x,m\n",
    )
    _write(
        repo / "build/source-index.json",
        json.dumps(
            {
                "schema": "reccmp-source-index-v1",
                "markers": [
                    {
                        "address": 0x00401000,
                        "marker_kind": "FUNCTION",
                        "source_file": "src/wiz8/local_code/A.cpp",
                        "line": 12,
                        "declaration": None,
                        "marker_name": "Owned",
                        "target": "WIZ8",
                    }
                ],
                "declarations": [],
                "classes": [
                    {
                        "semantic_id": "record:BitArray",
                        "qualified_name": "BitArray",
                        "bases": [],
                        "fields": [],
                        "virtual_declarations": [],
                        "source_file": "include/wiz8/BitArray.h",
                        "line": 1,
                        "end_line": 2,
                    }
                ],
            }
        ),
    )
    return repo


def test_every_reviewed_catalog_becomes_a_program_row(repository: Path) -> None:
    report = derive_status(repository, _ghidra_functions())

    assert report["schema"] == "wiz8.recovery-status"
    assert [item["program"] for item in report["programs"]] == [
        "cfagent-128",
        "srext-unzip",
        "wiz8",
    ]
    catalog = next(item for item in report["programs"] if item["program"] == "cfagent-128")
    assert catalog["identities"] == 2
    assert catalog["authority"] == {"fan-patch": 2}
    assert catalog["confidence"] == {"high": 1, "medium": 1}


def test_canonical_identities_union_source_markers_with_accepted_identity_claims(
    repository: Path,
) -> None:
    """0x401000 is source-owned only, 0x402000 is both, 0x404000 is claim-only."""

    report = derive_status(repository, _ghidra_functions())

    assert report["wiz8"]["source_functions"] == 1
    assert report["wiz8"]["function_identities"] == 3
    assert report["wiz8"]["analysis_only_identities"] == 2
    assert report["wiz8"]["claims"] == 4
    # `identity-provenance` contributes authority, never a new address.
    assert report["wiz8"]["authority"] == {"fan-patch": 1, "source-oracle": 2}


def test_source_inventory_counts_come_from_the_observation_tables(repository: Path) -> None:
    report = derive_status(repository, _ghidra_functions())

    assert report["wiz8"]["classes"] == 1
    assert report["wiz8"]["source_units"] == 2
    assert report["wiz8"]["source_units_by_subsystem"] == {"engine_code": 1, "local_code": 1}


def test_gameplay_attribution_separates_markers_assertions_and_gaps(repository: Path) -> None:
    """Ghidra owns the inventory; markers and assertion anchors attribute it."""

    gameplay = derive_status(repository, _ghidra_functions())["wiz8"]["gameplay"]

    assert gameplay["functions"] == 4
    assert gameplay["owners"] == {"source": 1, "unassigned": 3}
    # 0x402000/0x403000 anchor the interval directly; 0x401000 is marker-direct;
    # 0x404000 lies outside every assertion-bounded interval.
    assert gameplay["translation_unit_attribution"] == {
        "direct": 3,
        "inferred": 0,
        "gap": 1,
        "external": 0,
    }
    assert gameplay["unowned_functions"] == 1
    # A marker attributes its recovered `src/` path while an assertion anchor
    # attributes the original `Local Code\` spelling; both are counted.
    assert gameplay["attributed_source_units"] == 2
