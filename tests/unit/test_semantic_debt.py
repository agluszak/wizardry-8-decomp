from pathlib import Path

import pytest
from wiz8decomp.reports.semantic_debt import (
    _empty_special_members,
    _external_single_unit_definitions,
    _known_semantics_bad_spelling,
    _storage_debt,
    _target_sources,
    _Usage,
    _void_storage,
)


@pytest.mark.parametrize("target, directory", [("WIZ8", "wiz8"), ("SURRENDER", "surrender")])
def test_selected_product_reports_accessed_storage_and_void_payloads(
    tmp_path: Path, target, directory
):
    classes = []
    variables = []
    for product, subdir in [("WIZ8", "wiz8"), ("SURRENDER", "surrender")]:
        header = tmp_path / f"include/{subdir}/record.h"
        header.parent.mkdir(parents=True)
        header.write_text("struct Record { long value_10; void* payload; };\n")
        source = tmp_path / f"src/{subdir}/record.cpp"
        source.parent.mkdir(parents=True)
        source.write_text("void use() { record.value_10 = 1; consume(record.value_10); }\n")
        path = header.relative_to(tmp_path).as_posix()
        classes.append(
            {
                "qualified_name": f"{product}Record",
                "fields": [
                    {
                        "name": "value_10",
                        "source_file": path,
                        "line": 1,
                        "type": "long",
                        "offset": 0,
                    },
                    {
                        "name": "payload",
                        "source_file": path,
                        "line": 1,
                        "type": "void *",
                        "offset": 4,
                    },
                ],
            }
        )
        variables.append(
            {
                "qualified_name": f"g_{subdir}_field_00400",
                "source_file": path,
                "line": 1,
                "target": product,
                "type": "long",
            }
        )
    index = {"classes": classes, "variables": variables}
    sources = _target_sources(tmp_path, target.lower())
    assert set(sources) == {f"include/{directory}/record.h", f"src/{directory}/record.cpp"}
    debt = _storage_debt(index, _Usage(sources), target)
    assert len(debt["address_named_members"]) == 1
    row = debt["address_named_members"][0]
    assert row["kind"] == "offset"
    assert row["references"] == 2
    assert row["writes"] == 1
    assert row["fields"][0]["record"] == f"{target}Record"
    assert row["receiver_verified"] is False
    assert [row["name"] for row in debt["address_named_globals"]] == [f"g_{directory}_field_00400"]
    assert [row["record"] for row in _void_storage(index, target)] == [f"{target}Record"]


def test_surrender_semantic_comments_special_members_and_linkage_stay_in_scope(tmp_path: Path):
    path = "src/surrender/record.cpp"
    source = "long field_10; // texture count\nvoid local_api() {}\nRecord::Record() {}\n"
    file = tmp_path / path
    file.parent.mkdir(parents=True)
    file.write_text(source)
    unrelated = "src/wiz8/record.cpp"
    index = {
        "variables": [
            {"qualified_name": "field_10", "source_file": path, "line": 1, "target": "SURRENDER"},
            {"qualified_name": "field_20", "source_file": unrelated, "line": 1, "target": "WIZ8"},
        ],
        "declarations": [
            {
                "qualified_name": "local_api",
                "source_file": path,
                "line": 2,
                "semantic_kind": "free_function",
                "is_definition": True,
                "linkage": "external",
            },
            {
                "qualified_name": "Record::Record",
                "source_file": path,
                "line": 3,
                "end_line": 3,
                "semantic_kind": "constructor",
                "is_definition": True,
            },
            {
                "qualified_name": "Other::Other",
                "source_file": unrelated,
                "line": 3,
                "end_line": 3,
                "semantic_kind": "constructor",
                "is_definition": True,
            },
        ],
    }
    sources = _target_sources(tmp_path, "SURRENDER")
    usage = _Usage(sources)
    assert [
        row["name"] for row in _known_semantics_bad_spelling(index, sources, usage, "SURRENDER")
    ] == ["field_10"]
    assert [
        row["name"] for row in _external_single_unit_definitions(index, usage, "SURRENDER")
    ] == ["local_api"]
    assert [row["name"] for row in _empty_special_members(index, sources)] == ["Record::Record"]
    assert _external_single_unit_definitions(index, usage, "WIZ8") == []


def test_report_wires_surrender_scope_through_all_queues(tmp_path: Path, monkeypatch):
    from wiz8decomp.reports import portability, semantic_debt
    from wiz8decomp.source_units import UNMAPPED_SOURCE

    path = "include/surrender/record.h"
    header = tmp_path / path
    header.parent.mkdir(parents=True)
    header.write_text("struct Record { long value_10; void* payload; };\n")
    source = tmp_path / "src/surrender/record.cpp"
    source.parent.mkdir(parents=True)
    source.write_text("void use() { record.value_10 = 1; }\n")
    index = {
        "classes": [
            {
                "qualified_name": "Record",
                "fields": [
                    {
                        "name": "value_10",
                        "source_file": path,
                        "type": "long",
                        "line": 1,
                        "offset": 0,
                    },
                    {
                        "name": "payload",
                        "source_file": path,
                        "type": "void *",
                        "line": 1,
                        "offset": 4,
                    },
                ],
            }
        ]
    }
    monkeypatch.setattr(semantic_debt, "load_source_index", lambda _: index)
    monkeypatch.setattr(semantic_debt, "source_functions", lambda _, target: {})
    monkeypatch.setattr(semantic_debt, "read_assertions", lambda _: [])
    monkeypatch.setattr(
        semantic_debt,
        "source_unit_records",
        lambda _: {
            "src/surrender/record.cpp": {"mapping": UNMAPPED_SOURCE},
            "src/wiz8/record.cpp": {"mapping": UNMAPPED_SOURCE},
        },
    )
    monkeypatch.setattr(portability, "portability_queues", lambda *_: {})
    report = semantic_debt.semantic_debt_report(tmp_path, "SURRENDER")
    assert report["summary"]["unmapped_sources"] == 1
    assert report["summary"]["address_named_members"] == 1
    assert report["address_named_members"][0]["references"] == 1
    assert report["summary"]["void_storage_members"] == 1
    assert report["unmapped_sources"][0]["source_file"] == "src/surrender/record.cpp"


def test_identity_report_does_not_confuse_pairing_with_source_ownership(tmp_path, monkeypatch):
    from types import SimpleNamespace

    from reccmp.analysis import crt_startup
    from reccmp.compare import Compare
    from reccmp.types import EntityType
    from wiz8decomp import comparison, emissions, source_index
    from wiz8decomp.reports.semantic_debt import _retail_identity_ownership

    def entity(address, kind, **facts):
        return SimpleNamespace(
            orig=SimpleNamespace(facts=facts),
            orig_addr=address,
            recomp_addr=None,
            entity_type=kind,
        )

    monkeypatch.setattr(crt_startup, "iter_crt_array_ranges", lambda *_: ())
    entities = [
        entity(0x1000, EntityType.FUNCTION, name="known"),
        entity(0x1010, EntityType.FUNCTION, name="unowned"),
        entity(0x1020, EntityType.IMPORT, name="external"),
        entity(0x1030, EntityType.DATA, seh_unwinds_orig=[]),
        entity(0x1040, EntityType.VTABLE, name="Owner"),
        entity(0x1050, EntityType.DATA, name="Owner vbtable"),
        entity(0x1060, EntityType.FUNCTION, name="library", library=True),
        entity(0x1070, EntityType.FUNCTION, name="folded"),
    ]
    engine = SimpleNamespace(
        get_all=lambda: entities,
        db=SimpleNamespace(
            get_aliases=lambda _: [(entities[-1], SimpleNamespace(orig_addr=0x1000))]
        ),
    )
    monkeypatch.setattr(Compare, "from_target", lambda _: engine)
    monkeypatch.setattr(comparison, "comparison_target", lambda *_: None)
    monkeypatch.setattr(
        source_index,
        "address_bound_identities",
        lambda *_: {
            0x1000: (SimpleNamespace(kind="definition", source_file="src/wiz8/owner.cpp"),),
            0x1040: (SimpleNamespace(kind="vtable", source_file="include/wiz8/owner.h"),),
        },
    )
    monkeypatch.setattr(
        emissions,
        "emission_inventory",
        lambda *_: [SimpleNamespace(address=0x1050, type="global", source_files=())],
    )
    report = _retail_identity_ownership(tmp_path, "WIZ8")
    assert report["classified"] == 7
    assert report["without_recomp_pair"] == 8
    assert report["unclassified"] == 1
    assert report["unclassified_entities"][0]["address"] == "0x00001010"
    assert report["buckets"]["recovered_authored_source"] == 1
    assert report["buckets"]["compiler_generated"] == 3
    assert report["buckets"]["icf_folded_sibling"] == 1
    assert report["inventory_complete"] is False


def test_crt_boundary_ownership_requires_pair_alignment_and_zero_bytes(tmp_path, monkeypatch):
    from types import SimpleNamespace

    from reccmp.analysis import crt_startup
    from reccmp.compare import Compare
    from reccmp.types import EntityType
    from wiz8decomp import comparison, emissions, source_index
    from wiz8decomp.reports.semantic_debt import _retail_identity_ownership

    addresses = (0x1000, 0x1010, 0x1020, 0x1030, 0x1041, 0x1050, 0x1060)
    entities = [
        SimpleNamespace(
            orig=SimpleNamespace(facts={"name": "sentinel"}),
            orig_addr=address,
            recomp_addr=None,
            entity_type=EntityType.DATA,
        )
        for address in addresses
    ]
    engine = SimpleNamespace(
        get_all=lambda: entities,
        db=SimpleNamespace(get_aliases=lambda _: ()),
        orig_bin=SimpleNamespace(
            read=lambda address, _: b"\1\0\0\0" if address == 0x1030 else b"\0" * 4
        ),
    )
    monkeypatch.setattr(Compare, "from_target", lambda _: engine)
    monkeypatch.setattr(comparison, "comparison_target", lambda *_: None)
    monkeypatch.setattr(source_index, "address_bound_identities", lambda *_: {})
    monkeypatch.setattr(emissions, "emission_inventory", lambda *_: ())
    monkeypatch.setattr(
        crt_startup,
        "iter_crt_array_ranges",
        lambda *_: (
            (None, range(0x1000, 0x1010)),
            (None, range(0x1020, 0x1030)),
            (None, range(0x1041, 0x1050)),
            (None, range(0x1060, 0x1050)),
        ),
    )
    report = _retail_identity_ownership(tmp_path, "WIZ8")
    assert report["buckets"] == {"library_owned": 3}
    assert {row["address"] for row in report["unclassified_entities"]} == {
        "0x00001030",
        "0x00001041",
        "0x00001050",
        "0x00001060",
    }
    assert report["inventory_complete"] is False
