from wiz8decomp.ghidra.function_roles import source_role_tags, source_virtual_slots
from wiz8decomp.source_model import SourceFunction


def source_function(**overrides: object) -> SourceFunction:
    values = {
        "address": 0x401000,
        "kind": "FUNCTION",
        "file": "src/wiz8/example.cpp",
        "line": 10,
        "name": "Example::method",
        "semantic_id": "?method@Example@@QAEXXZ",
        "semantic_kind": "method",
        "calling_convention": "__thiscall",
        "return_type": "void",
        "parameter_types": (),
        "owning_class": "Example",
        "is_virtual": False,
    }
    values.update(overrides)
    return SourceFunction(**values)  # type: ignore[arg-type]


def test_source_roles_distinguish_constructor_and_destructor() -> None:
    assert source_role_tags(source_function(semantic_kind="constructor"), target="WIZ8") == (
        "wiz8:source:constructor",
        "wiz8:role:constructor-body",
        "wiz8:origin:first-party",
    )
    assert "wiz8:role:complete-destructor" in source_role_tags(
        source_function(semantic_kind="destructor"), target="WIZ8"
    )


def test_source_roles_split_scalar_and_vector_deleting_emissions() -> None:
    scalar = source_function(
        kind="SYNTHETIC",
        name="Example::`scalar deleting destructor'",
        semantic_id=None,
        semantic_kind=None,
    )
    vector = source_function(
        kind="SYNTHETIC",
        name="Example::`vector deleting destructor'`adjustor{4}'",
        semantic_id=None,
        semantic_kind=None,
    )
    assert "wiz8:role:scalar-deleting-destructor" in source_role_tags(scalar, target="WIZ8")
    assert "wiz8:role:vector-deleting-destructor" in source_role_tags(vector, target="WIZ8")


def test_source_virtual_slots_preserve_decorated_identity_and_order() -> None:
    document = {
        "classes": [
            {
                "source_file": "include/wiz8/example.h",
                "vtable_address": 0x500000,
                "virtual_declarations": ["??_DExample@@QAEXXZ", "?value@Example@@UAEHXZ"],
            },
            {
                "source_file": "include/surrender/example.h",
                "vtable_address": 0x600000,
                "virtual_declarations": ["?other@Example@@UAEXXZ"],
            },
        ]
    }

    assert source_virtual_slots(document, target="WIZ8") == {
        0x500000: "??_DExample@@QAEXXZ",
        0x500004: "?value@Example@@UAEHXZ",
    }
