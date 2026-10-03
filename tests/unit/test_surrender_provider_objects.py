from __future__ import annotations

import struct
from pathlib import Path

import pytest
from wiz8decomp.binary.coff import export_directives
from wiz8decomp.surrender_exports import (
    SurrenderExportsError,
    validate_surrender_provider_objects,
)


def _object(directives: bytes) -> bytes:
    code = b"\xc3"
    table = 100 + len(code) + len(directives)
    header = struct.pack("<HHLLLHH", 0x14C, 2, 0, table, 1, 0, 0)
    code_header = struct.pack("<8sLLLLLLHHL", b".text", 0, 0, len(code), 100, 0, 0, 0, 0, 0)
    directive_header = struct.pack(
        "<8sLLLLLLHHL", b".drectve", 0, 0, len(directives), 101, 0, 0, 0, 0, 0
    )
    symbol = struct.pack("<8sLhHBB", b"retail", 0, 1, 0, 2, 0)
    return header + code_header + directive_header + code + directives + symbol + b"\x04\0\0\0"


@pytest.fixture
def repository(tmp_path: Path) -> Path:
    source = tmp_path / "src/surrender"
    source.mkdir(parents=True)
    (source / "sr.def").write_text("LIBRARY sr\nEXPORTS\nretail\n", encoding="utf-8")
    evidence = tmp_path / "evidence/snapshots/surrender-abi"
    evidence.mkdir(parents=True)
    (evidence / "exports.csv").write_text(
        "program,module,decorated_name\nwiz8--gog-base--sr--0123456789ab,sr.dll,retail\n",
        encoding="utf-8",
    )
    return tmp_path


def test_reads_quoted_aliases_and_data_without_other_directives() -> None:
    data = _object(b'-defaultlib:MSVCRT /EXPORT:"retail",DATA -export:"alias"=internal')

    assert export_directives(data) == {"retail", "alias"}


def test_matching_provider_is_not_rewritten(repository: Path) -> None:
    obj = repository / "provider.obj"
    data = _object(b"-export:retail -defaultlib:MSVCRT")
    obj.write_bytes(data)

    validate_surrender_provider_objects(repository, [obj])

    assert obj.read_bytes() == data


def test_reports_missing_and_unexpected_emissions_together_without_rewriting(
    repository: Path,
) -> None:
    (repository / "src/surrender/sr.def").write_text("EXPORTS\nretail\nmissing\n", encoding="utf-8")
    obj = repository / "provider.obj"
    data = _object(b"-export:retail -export:extra")
    obj.write_bytes(data)

    with pytest.raises(SurrenderExportsError) as failure:
        validate_surrender_provider_objects(repository, [obj])

    assert "missing required export definition: missing" in str(failure.value)
    assert "compiler export absent from retail: extra" in str(failure.value)
    assert obj.read_bytes() == data


@pytest.mark.parametrize("data", [b"", b"\0" * 2, struct.pack("<HH", 0x14C, 2) + b"\0" * 16])
def test_rejects_truncated_objects(data: bytes) -> None:
    with pytest.raises(ValueError):
        export_directives(data)


def test_rejects_directive_section_past_end() -> None:
    data = bytearray(_object(b"-export:retail"))
    struct.pack_into("<L", data, 80, len(data))

    with pytest.raises(ValueError, match="directive section runs past"):
        export_directives(bytes(data))
