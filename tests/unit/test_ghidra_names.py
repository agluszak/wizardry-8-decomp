from __future__ import annotations

import pytest
from wiz8decomp.ghidra import query as query_module
from wiz8decomp.ghidra.project import program_name
from wiz8decomp.ghidra.query import validate_query_arguments


def test_program_name_is_stable_and_hash_qualified() -> None:
    module = {"variant": "gog-base", "relative_path": "Dll/Something.dll", "sha256": "a" * 64}
    assert program_name(module) == "wiz8--gog-base--something--aaaaaaaaaaaa"


def test_query_argument_validation() -> None:
    validate_query_arguments("read-data", ["0x401000", "16"])
    with pytest.raises(ValueError, match="expects 2"):
        validate_query_arguments("read-data", ["0x401000"])
    with pytest.raises(ValueError, match="unknown command"):
        validate_query_arguments("rename", [])
    with pytest.raises(ValueError, match="unknown command"):
        validate_query_arguments("function-slice", ["0x401000"])


def test_listing_and_decompile_do_not_repeat_function_metadata(monkeypatch) -> None:
    class Function:
        def getBody(self):
            return object()

    class Listing:
        def getInstructions(self, _body, _forward):
            return []

    class Program:
        def getListing(self):
            return Listing()

    function = Function()
    monkeypatch.setattr(query_module, "_function", lambda *_args: function)
    monkeypatch.setattr(query_module, "function_metadata", lambda *_args: {"large": "metadata"})
    from wiz8decomp.ghidra import semantic

    monkeypatch.setattr(semantic, "decompile_c", lambda *_args: {"decompiled": "body"})

    assert query_module.execute_query(Program(), "listing", ["name"]) == {"listing": ""}
    assert query_module.execute_query(Program(), "decompile", ["name"]) == {"decompiled": "body"}
