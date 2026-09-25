from __future__ import annotations

import json
from pathlib import Path

import pytest
from wiz8decomp import header_architecture
from wiz8decomp.ghidra.unit_intervals import TranslationUnitLayout, UnitAnchor
from wiz8decomp.header_architecture import (
    HeaderArchitectureError,
    analyze_header_architecture,
    validate_header_architecture,
)
from wiz8decomp.source_units import ORIGINAL_TU, UNRESOLVED_FRAGMENT

UNIT_A = r"Local Code\Foo.cpp"
UNIT_B = r"Local Code\Bar.cpp"


def _write_arch(repo: Path, document: dict | None = None) -> None:
    path = repo / "src/wiz8/header_architecture.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(document or {"schema": "wiz8.header-architecture-v2"}),
        encoding="utf-8",
    )
    assertions = repo / "evidence/observations/wiz8"
    assertions.mkdir(parents=True, exist_ok=True)
    (assertions / "assertions.csv").write_text(
        "call_site,kind,containing_function,source_path,line,expression,message\n",
        encoding="utf-8",
    )


def _write_index(
    repo: Path,
    *,
    functions: list[dict] | None = None,
    variables: list[dict] | None = None,
    markers: list[dict] | None = None,
    header_declarations: list[dict] | None = None,
) -> None:
    build = repo / "build"
    build.mkdir(parents=True, exist_ok=True)
    (build / "source-index.json").write_text(
        json.dumps(
            {
                "markers": markers or [],
                "declarations": functions or [],
                "classes": [],
                "variables": variables or [],
                "conflicts": [],
                "header_declarations": header_declarations or [],
            }
        ),
        encoding="utf-8",
    )


def _header_decl(
    header: str,
    semantic_id: str,
    name: str,
    *,
    kind: str = "function",
    member: bool = False,
    defined: bool = False,
) -> dict:
    return {
        "source_file": header,
        "semantic_id": semantic_id,
        "qualified_name": name,
        "kind": kind,
        "member": member,
        "defined": defined,
        "line": 1,
    }


def _definition(semantic_id: str, source_file: str) -> dict:
    return {
        "semantic_id": semantic_id,
        "source_file": source_file,
        "is_definition": True,
    }


def _patch_units(monkeypatch: pytest.MonkeyPatch, records: dict[str, dict]) -> None:
    monkeypatch.setattr(header_architecture, "source_unit_records", lambda repo: records)


def _original_unit(path: str, original: str) -> dict:
    return {"class": ORIGINAL_TU, "path": path, "original_path": original}


def _report(repo: Path, layout: TranslationUnitLayout | None = None) -> dict:
    return analyze_header_architecture(
        repo, layout=layout if layout is not None else TranslationUnitLayout(())
    )


def test_single_original_unit_infers_tu_interface(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_arch(tmp_path)
    (tmp_path / "include/wiz8").mkdir(parents=True)
    (tmp_path / "include/wiz8/widget.h").write_text("void Reset(void);\n")
    _write_index(
        tmp_path,
        functions=[_definition("Reset", "src/wiz8/local_code/Foo.cpp")],
        header_declarations=[_header_decl("include/wiz8/widget.h", "Reset", "Reset")],
    )
    _patch_units(
        monkeypatch,
        {"src/wiz8/local_code/Foo.cpp": _original_unit("src/wiz8/local_code/Foo.cpp", UNIT_A)},
    )

    report = _report(tmp_path)

    row = report["headers"][0]
    assert row["role"] == "tu-interface"
    assert row["original_units"] == [UNIT_A]
    assert report["violations"] == []


def test_multi_tu_header_requires_allowance(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_arch(tmp_path)
    (tmp_path / "include/wiz8").mkdir(parents=True)
    (tmp_path / "include/wiz8/bundle.h").write_text("void First(void);\nvoid Second(void);\n")
    _write_index(
        tmp_path,
        functions=[
            _definition("First", "src/wiz8/local_code/Foo.cpp"),
            _definition("Second", "src/wiz8/local_code/Bar.cpp"),
        ],
        header_declarations=[
            _header_decl("include/wiz8/bundle.h", "First", "First"),
            _header_decl("include/wiz8/bundle.h", "Second", "Second"),
        ],
    )
    _patch_units(
        monkeypatch,
        {
            "src/wiz8/local_code/Foo.cpp": _original_unit("src/wiz8/local_code/Foo.cpp", UNIT_A),
            "src/wiz8/local_code/Bar.cpp": _original_unit("src/wiz8/local_code/Bar.cpp", UNIT_B),
        },
    )

    report = _report(tmp_path)

    row = report["headers"][0]
    assert row["role"] == "multi-tu"
    assert report["violations"][0]["rule"] == "multi-tu-header"


def test_multi_tu_allowance_covers_resolved_units(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_arch(
        tmp_path,
        {
            "schema": "wiz8.header-architecture-v2",
            "allowed-multi-tu-headers": {"include/wiz8/bundle.h": [UNIT_A, UNIT_B]},
        },
    )
    (tmp_path / "include/wiz8").mkdir(parents=True)
    (tmp_path / "include/wiz8/bundle.h").write_text("void First(void);\nvoid Second(void);\n")
    _write_index(
        tmp_path,
        functions=[
            _definition("First", "src/wiz8/local_code/Foo.cpp"),
            _definition("Second", "src/wiz8/local_code/Bar.cpp"),
        ],
        header_declarations=[
            _header_decl("include/wiz8/bundle.h", "First", "First"),
            _header_decl("include/wiz8/bundle.h", "Second", "Second"),
        ],
    )
    _patch_units(
        monkeypatch,
        {
            "src/wiz8/local_code/Foo.cpp": _original_unit("src/wiz8/local_code/Foo.cpp", UNIT_A),
            "src/wiz8/local_code/Bar.cpp": _original_unit("src/wiz8/local_code/Bar.cpp", UNIT_B),
        },
    )

    report = _report(tmp_path)

    assert report["headers"][0]["role"] == "multi-tu"
    assert report["violations"] == []


def test_fragment_owned_declarations_infer_provisional(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_arch(tmp_path)
    (tmp_path / "include/wiz8").mkdir(parents=True)
    (tmp_path / "include/wiz8/npc_items.h").write_text("void AddNpcItem(int slot);\n")
    _write_index(
        tmp_path,
        functions=[_definition("AddNpcItem", "src/wiz8/npc_items.cpp")],
        header_declarations=[_header_decl("include/wiz8/npc_items.h", "AddNpcItem", "AddNpcItem")],
    )
    _patch_units(
        monkeypatch,
        {
            "src/wiz8/npc_items.cpp": {
                "class": UNRESOLVED_FRAGMENT,
                "path": "src/wiz8/npc_items.cpp",
            }
        },
    )

    report = _report(tmp_path)

    assert report["headers"][0]["role"] == "provisional-interface"
    assert report["provisional"] == ["include/wiz8/npc_items.h"]
    assert report["violations"] == []


def test_header_defined_declarations_infer_implementation(tmp_path: Path) -> None:
    _write_arch(tmp_path)
    (tmp_path / "include/wiz8").mkdir(parents=True)
    (tmp_path / "include/wiz8/vector.h").write_text("struct V;\n")
    _write_index(
        tmp_path,
        header_declarations=[
            _header_decl(
                "include/wiz8/vector.h",
                "Grow",
                "W8GrowableVector<int>::Grow",
                member=True,
                defined=True,
            )
        ],
    )

    report = _report(tmp_path)

    assert report["headers"][0]["role"] == "header-implementation"
    assert report["violations"] == []


def test_template_instantiation_members_are_header_defined(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_arch(tmp_path)
    (tmp_path / "include/wiz8").mkdir(parents=True)
    (tmp_path / "include/wiz8/vector.h").write_text("struct V;\n")
    _write_index(
        tmp_path,
        header_declarations=[
            _header_decl(
                "include/wiz8/vector.h",
                "??4?$W8GrowableVector@H@@...",
                "W8GrowableVector<int>::operator=",
                member=True,
            )
        ],
    )
    _patch_units(monkeypatch, {})

    report = _report(tmp_path)

    assert report["headers"][0]["role"] == "header-implementation"
    assert report["headers"][0]["unresolved"] == []


def test_unresolvable_declaration_is_reported_not_hidden(tmp_path: Path) -> None:
    _write_arch(tmp_path)
    (tmp_path / "include/wiz8").mkdir(parents=True)
    (tmp_path / "include/wiz8/mouth_gap.h").write_text("void Function5E2D10(void);\n")
    _write_index(
        tmp_path,
        header_declarations=[_header_decl("include/wiz8/mouth_gap.h", "Fn", "Function5E2D10")],
    )

    report = _report(tmp_path)

    row = report["headers"][0]
    assert row["role"] == "unresolved"
    assert row["unresolved"] == ["Function5E2D10"]
    assert report["unresolved"][0]["file"] == "include/wiz8/mouth_gap.h"


def test_marker_placement_resolves_original_unit(tmp_path: Path) -> None:
    _write_arch(tmp_path)
    (tmp_path / "include/wiz8").mkdir(parents=True)
    (tmp_path / "include/wiz8/declared.h").write_text("void Placed(void);\n")
    _write_index(
        tmp_path,
        markers=[
            {
                "address": 0x401080,
                "marker_kind": "function",
                "marker_name": "Placed",
                "declaration_key": ["WIZ8", "Placed"],
                "source_file": "src/wiz8/local_code/Foo.cpp",
            }
        ],
        header_declarations=[_header_decl("include/wiz8/declared.h", "Placed", "Placed")],
    )
    layout = TranslationUnitLayout(
        [
            UnitAnchor(0x401000, UNIT_A, "assertion"),
            UnitAnchor(0x401100, UNIT_A, "assertion"),
        ]
    )

    report = _report(tmp_path, layout=layout)

    row = report["headers"][0]
    assert row["role"] == "tu-interface"
    assert row["original_units"] == [UNIT_A]


def test_local_include_must_resolve_to_a_header(tmp_path: Path) -> None:
    _write_arch(tmp_path)
    (tmp_path / "include/wiz8").mkdir(parents=True)
    (tmp_path / "include/wiz8/uses_deleted.h").write_text(
        '#include "wiz8/character.h"\nstruct X;\n'
    )
    _write_index(tmp_path)

    report = _report(tmp_path)

    assert report["violations"] == [
        {
            "rule": "missing-include",
            "file": "include/wiz8/uses_deleted.h",
            "include": "wiz8/character.h",
        }
    ]


def test_removed_umbrella_may_not_be_recreated(tmp_path: Path) -> None:
    _write_arch(tmp_path)
    (tmp_path / "include/wiz8").mkdir(parents=True)
    (tmp_path / "include/wiz8/magic.h").write_text("#pragma once\n")
    _write_index(tmp_path)

    report = _report(tmp_path)

    assert report["violations"] == [{"rule": "removed-aggregate", "header": "include/wiz8/magic.h"}]


def test_layout_header_may_not_declare_functions(tmp_path: Path) -> None:
    _write_arch(tmp_path)
    (tmp_path / "include/wiz8/layouts").mkdir(parents=True)
    (tmp_path / "include/wiz8/layouts/state.h").write_text("void DoThing(void);\n")
    _write_index(
        tmp_path,
        header_declarations=[_header_decl("include/wiz8/layouts/state.h", "DoThing", "DoThing")],
    )

    report = _report(tmp_path)

    assert report["headers"][0]["role"] == "shared-layout"
    assert report["violations"][0]["rule"] == "layout-declares-functions"


def test_layout_header_may_not_include_an_interface(tmp_path: Path) -> None:
    _write_arch(tmp_path)
    (tmp_path / "include/wiz8/layouts").mkdir(parents=True)
    (tmp_path / "include/wiz8/layouts/state.h").write_text('#include "wiz8/widget.h"\nstruct S;\n')
    (tmp_path / "include/wiz8/widget.h").write_text("void DoThing(void);\n")
    _write_index(
        tmp_path,
        header_declarations=[_header_decl("include/wiz8/widget.h", "DoThing", "DoThing")],
    )

    report = _report(tmp_path)

    assert report["violations"] == [
        {
            "rule": "layout-includes-interface",
            "header": "include/wiz8/layouts/state.h",
            "include": "include/wiz8/widget.h",
        }
    ]


def test_proven_header_keeps_its_original_spelling(tmp_path: Path) -> None:
    _write_arch(
        tmp_path,
        {
            "schema": "wiz8.header-architecture-v2",
            "proven-original-headers": {
                "include/wiz8/engine_code/AnimRep.h": r"Engine Code\Include\AnimRep.hpp"
            },
        },
    )
    (tmp_path / "include/wiz8/engine_code").mkdir(parents=True)
    (tmp_path / "include/wiz8/engine_code/AnimRep.h").write_text("struct AnimRep;\n")
    _write_index(tmp_path)

    messages = validate_header_architecture(tmp_path)

    assert any("proven-header-spelling" in message for message in messages)


def test_v1_documents_are_rejected(tmp_path: Path) -> None:
    path = tmp_path / "src/wiz8/header_architecture.json"
    path.parent.mkdir(parents=True)
    path.write_text('{"schema":"wiz8.header-architecture-v1","headers":{}}')

    with pytest.raises(HeaderArchitectureError):
        analyze_header_architecture(tmp_path)


def test_fragment_report_derives_from_source_classification(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_arch(tmp_path)
    _write_index(
        tmp_path,
        markers=[
            {
                "address": 0x401080,
                "marker_kind": "function",
                "marker_name": "Unplaced",
                "declaration_key": ["WIZ8", "Unplaced"],
                "source_file": "src/wiz8/new.cpp",
            }
        ],
    )
    _patch_units(
        monkeypatch,
        {"src/wiz8/new.cpp": {"class": UNRESOLVED_FRAGMENT, "path": "src/wiz8/new.cpp"}},
    )

    report = _report(tmp_path)

    assert [
        (item["file"], item["functions"][0]["suggestion"])
        for item in report["unresolved_fragments"]
    ] == [("src/wiz8/new.cpp", "keep-fragment")]
