from __future__ import annotations

import pytest

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

