from pathlib import Path

from wiz8decomp.unresolved import parse_map_publics, unresolved_report

REPOSITORY = Path(__file__).resolve().parents[2]


def test_map_publics_survive_the_crlf_the_linker_writes(tmp_path: Path) -> None:
    # The MAP is written with CRLF, so a parser that trusts the line ending
    # silently reports every resolved symbol as missing.
    path = tmp_path / "Wiz8.map"
    path.write_bytes(
        b" 0001:0000b532       _GetTickCount@0            0040c532 f   kernel32:KERNEL32.dll\r\n"
        b" 0002:00000008       __imp__GetTickCount@0      0040d008     kernel32:KERNEL32.dll\r\n"
    )

    assert "_GetTickCount@0" in parse_map_publics(path)


def test_a_symbol_the_map_defines_is_not_reported_missing(tmp_path: Path) -> None:
    objects = tmp_path / "probe.dir"
    objects.mkdir()
    # No objects at all: the report is empty rather than raising, because an
    # unbuilt tree is a reason to build, not a finding.
    report = unresolved_report(objects, None)

    assert report["unresolved_symbols"] == 0
    assert report["by_unit"] == {}


def test_the_object_root_must_exist(tmp_path: Path) -> None:
    try:
        unresolved_report(tmp_path / "absent", None)
    except RuntimeError as error:
        assert "no built objects" in str(error)
    else:  # pragma: no cover - the call above is expected to raise
        raise AssertionError("a missing object root should be refused")


def test_imports_are_not_counted_as_unresolved() -> None:
    # A symbol an import library satisfies is resolved, and the decorated
    # __imp_ spelling exists only because the linker rewrote a resolved call.
    build = REPOSITORY / "build" / "decomp" / "CMakeFiles"
    if not build.is_dir():
        return
    report = unresolved_report(build, REPOSITORY / "build" / "decomp" / "Wiz8.map")
    assert not [name for name in report["by_symbol"] if name.startswith("__imp_")]
