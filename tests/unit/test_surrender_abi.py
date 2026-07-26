from __future__ import annotations

import csv
from pathlib import Path

from wiz8decomp.surrender_abi import parse_decorated_name, vftable_base_from_signature


def test_constructor_and_destructor_names_are_the_class_not_the_member() -> None:
    constructor = parse_decorated_name("??0srCamera@@QAE@XZ")
    destructor = parse_decorated_name("??1Decompressor@srHuffman@@QAE@XZ")

    assert (constructor.kind, constructor.class_name) == ("constructor", "srCamera")
    assert (destructor.kind, destructor.class_name) == ("destructor", "Decompressor")
    assert destructor.enclosing_scope == "srHuffman"


def test_storage_class_separates_virtual_static_and_free_members() -> None:
    virtual = parse_decorated_name("?verify@srMaterial@@UAEXW4e_verify@srRuntimeClass@@@Z")
    static = parse_decorated_name("?sGetClassName@srTexture@@SAPBDXZ")
    free = parse_decorated_name("?srAssertFail@@YAXPBD0J0ZZ")

    assert (virtual.virtuality, virtual.access) == ("virtual", "public")
    assert virtual.calling_convention == "__thiscall"
    assert static.virtuality == "static"
    assert static.calling_convention == "__cdecl"
    assert free.virtuality == "free-function"
    assert free.class_name == ""


def test_static_data_members_use_digit_storage_classes() -> None:
    parsed = parse_decorated_name("?CPU_Features_Mask@srTimer@@1KB")

    assert parsed.virtuality == "static-data"
    assert parsed.access == "protected"
    assert parsed.class_name == "srTimer"
    assert parsed.parse_status == "ok"


def test_global_operator_has_an_empty_scope_terminated_by_one_at_sign() -> None:
    """Splitting on '@@' here lands inside the template argument list."""
    parsed = parse_decorated_name(
        "??6@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@std@@AAV01@ABVsrShader@@@Z"
    )

    assert parsed.kind == "operator-lshift"
    assert parsed.virtuality == "free-function"
    assert parsed.parse_status == "ok"


def test_encoded_string_literals_carry_no_scope_to_decode() -> None:
    parsed = parse_decorated_name("??_C@_07HOCI@srLight?$AA@")

    assert parsed.kind == "string-literal"
    assert parsed.parse_status == "ok"


def test_vftable_base_comes_from_the_demangled_form_not_the_backreference() -> None:
    """`@@6B0@@` is a back-reference to the class itself, not a base named '0'."""
    assert (
        vftable_base_from_signature("const srBinFStream::`vftable'{for `srBinFStream'}")
        == "srBinFStream"
    )
    assert vftable_base_from_signature("const srBinIMStream::`vbtable'") == ""
    assert (
        vftable_base_from_signature(
            "const srLight::`vftable'{for `srClassSupport<class srIlluminator, class srNode, 0, 4608>'}"
        )
        == "srClassSupport<class srIlluminator, class srNode, 0, 4608>"
    )


def _snapshot() -> list[dict[str, str]]:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/snapshots/surrender-abi/exports.csv").open(
        encoding="utf-8", newline=""
    ) as stream:
        return list(csv.DictReader(stream))


def test_snapshot_is_keyed_by_program_and_decorated_name() -> None:
    rows = _snapshot()

    keys = [(row["program"], row["decorated_name"]) for row in rows]
    assert len(keys) == len(set(keys))


def test_snapshot_decodes_every_decorated_name() -> None:
    rows = _snapshot()

    assert all(row["parse_status"] == "ok" for row in rows)
    undecoded = [row for row in rows if row["decorated_name"].startswith("?") and not row["demangled_signature"]]
    assert not undecoded, undecoded[:5]


def test_snapshot_carries_the_inheritance_edges_the_class_model_needs() -> None:
    rows = _snapshot()

    vftables = [row for row in rows if row["kind"] == "vftable"]
    assert vftables
    # A vftable emitted for a secondary base names that base; a primary does not.
    assert any(row["vftable_base"] for row in vftables)
    assert all("@" not in row["vftable_base"] for row in vftables)


def test_snapshot_structural_class_agrees_with_the_demangler() -> None:
    rows = _snapshot()

    disagreements = [
        row["decorated_name"]
        for row in rows
        if row["class_name"]
        and row["demangled_signature"]
        and row["class_name"] not in row["demangled_signature"]
    ]
    assert not disagreements, disagreements[:5]
