import csv
import hashlib
import re
from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp.sgp_oracle import (
    BUILDS,
    PROJECT_FLAGS,
    UNITS,
    BuildText,
    CoffFunction,
    _evaluate,
    classify_body,
    sweep_sgp_units,
)


def _without_c_comments(source: str) -> str:
    source = re.sub(r"/\*.*?\*/", " ", source, flags=re.DOTALL)
    return re.sub(r"//.*", " ", source)


def _declared_function_names(source: str) -> set[str]:
    declaration = re.compile(
        r"(?:^|;)\s*(?:extern\s+)?[^;{}#]*?\b([A-Za-z_]\w*)"
        r"\s*\([^;{}]*\)\s*;",
        re.MULTILINE,
    )
    return {match.group(1) for match in declaration.finditer(_without_c_comments(source))}


def _local_extern_function_names(source: str) -> set[str]:
    declaration = re.compile(
        r'\bextern(?:\s+"C")?\s+[^;{}]*?\b([A-Za-z_]\w*)'
        r"\s*\([^;{}]*\)\s*;",
        re.DOTALL,
    )
    return {match.group(1) for match in declaration.finditer(_without_c_comments(source))}


def _extern_variable_names(source: str) -> set[str]:
    declaration = re.compile(
        r'\bextern(?:\s+"C")?\s+[^;(){}#]*?\b([A-Za-z_]\w*)'
        r"\s*(?:\[[^;]*\])?\s*;",
        re.DOTALL,
    )
    return {match.group(1) for match in declaration.finditer(_without_c_comments(source))}


def _macro_names(source: str) -> set[str]:
    return set(re.findall(r"^\s*#\s*define\s+([A-Za-z_]\w*)", source, re.MULTILINE))


def _harness_rows(repository: Path, unit: str) -> list[dict[str, str]]:
    with (repository / "evidence/snapshots/sgp/harness.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        return [row for row in csv.DictReader(stream) if row["unit"] == unit]


def _reviewed_rows(repository: Path, unit: str) -> list[dict[str, str]]:
    with (repository / "evidence/reviewed/sgp/findings.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        return [row for row in csv.DictReader(stream) if row["unit"] == unit]


def _accepted_functions(repository: Path) -> list[dict[str, str]]:
    with (repository / "evidence/reviewed/sgp/units.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        paths = {row["unit"]: row["source_path"] for row in csv.DictReader(stream)}
    with (repository / "evidence/snapshots/sgp/harness.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        harness = [
            row
            for row in csv.DictReader(stream)
            if row["build"] == "gog_base"
            and row["classification"] in {"exact", "relocation-equivalent"}
        ]
    with (repository / "evidence/reviewed/sgp/findings.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        retained = [row for row in csv.DictReader(stream) if row["finding"] == "retained"]

    # COFF folding gives Container and Timer fewer physical bodies than source
    # identities. Their reviewed unit findings preserve the source surface;
    # every other accepted identity comes directly from the generated sweep.
    harness = [row for row in harness if row["unit"] not in {"container", "timer"}]
    harness.extend(
        {
            "unit": row["unit"],
            "function": row["function"],
            "address": row["canonical_address"],
        }
        for row in retained
        if row["unit"] in {"container", "timer"}
    )
    allowed = {
        "directdraw",
        "container",
        "fileman",
        "librarydatabase",
        "random",
        "debug",
        "sgp",
        "input",
        "timer",
    }
    accepted = {
        (row["unit"], row["address"]): {
            "address": row["address"],
            "claimed_name": row["function"],
            "name_origin": "sgp-source",
            "authority": "source-backed",
            "source_path": paths[row["unit"]],
            "confidence": "exact",
        }
        for row in harness
        if row["unit"] in allowed and row["address"]
    }
    return sorted(accepted.values(), key=lambda row: (row["source_path"], row["address"]))


def test_vendored_sgp_source_exposes_the_wizardry_branch_census() -> None:
    repository = Path(__file__).resolve().parents[2]
    source = repository / "third_party/sfi-sgp/sgp"
    candidates = [
        path
        for path in source.iterdir()
        if path.is_file()
        and "WIZ8_PRECOMPILED_HEADERS" in path.read_text(encoding="latin-1", errors="replace")
    ]
    active = [
        path
        for path in candidates
        if any(
            "WIZ8_PRECOMPILED_HEADERS" in line and not line.lstrip().startswith("//")
            for line in path.read_text(encoding="latin-1", errors="replace").splitlines()
        )
    ]

    assert len(active) == 33
    assert {path.name for path in set(candidates) - set(active)} == {"MemMan.c"}
    project_text = "\n".join(
        path.read_text(encoding="utf-8", errors="replace")
        for root in (repository / "CMakeLists.txt", repository / "cmake")
        for path in ([root] if root.is_file() else root.rglob("*.cmake"))
    )
    assert "WIZ8_PRECOMPILED_HEADERS" not in project_text
    assert not list((repository / "config").rglob("WIZ8 SGP ALL.H"))


def test_wizardry_does_not_redeclare_vendored_sgp_header_symbols() -> None:
    repository = Path(__file__).resolve().parents[2]
    sgp = repository / "third_party/sfi-sgp/sgp"
    sgp_function_names: set[str] = set()
    sgp_variable_names: set[str] = set()
    sgp_macro_names: set[str] = set()
    for header in sgp.iterdir():
        if header.is_file() and header.suffix.lower() == ".h":
            contents = header.read_text(encoding="latin-1")
            sgp_function_names |= _declared_function_names(contents)
            sgp_variable_names |= _extern_variable_names(contents)
            sgp_macro_names |= _macro_names(contents)

    overlaps: list[tuple[str, str]] = []
    for source in (repository / "src/wiz8").rglob("*.cpp"):
        contents = source.read_text(encoding="utf-8")
        names = _local_extern_function_names(contents) & sgp_function_names
        names |= _extern_variable_names(contents) & sgp_variable_names
        names |= _macro_names(contents) & sgp_macro_names
        overlaps.extend((str(source.relative_to(repository)), name) for name in names)
    for header in (repository / "include/wiz8").rglob("*.h"):
        contents = header.read_text(encoding="utf-8")
        names = _declared_function_names(contents) & sgp_function_names
        names |= _extern_variable_names(contents) & sgp_variable_names
        names |= _macro_names(contents) & sgp_macro_names
        overlaps.extend(
            (str(header.relative_to(repository)), name)
            for name in names
            if name not in {"return", "void", "WIN32_LEAN_AND_MEAN"}
        )

    assert sorted(overlaps) == [
        ("include/wiz8/sgp-compat/WizLibs.h", "NUMBER_OF_LIBRARIES"),
        ("include/wiz8/sgp-compat/WizLibs.h", "gGameLibaries"),
        ("include/wiz8/sgp-compat/video2.h", "InvalidateRegion"),
    ]


def test_sgp_maps_keep_exact_and_absent_evidence_distinct() -> None:
    repository = Path(__file__).resolve().parents[2]
    functions = [
        row for row in _accepted_functions(repository) if row["source_path"].startswith("sgp/")
    ]
    with (repository / "evidence/observations/sgp/source-paths.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        paths = list(csv.DictReader(stream))

    assert len(functions) == 62
    assert {row["confidence"] for row in functions} == {"exact"}
    assert {row["source_path"] for row in functions} == {
        "sgp/DirectDraw Calls.c",
        "sgp/Container.c",
        "sgp/FileMan.c",
        "sgp/LibraryDataBase.c",
        "sgp/Random.c",
        "sgp/DEBUG.C",
        "sgp/sgp.c",
        "sgp/input.c",
        "sgp/timer.c",
    }
    assert len(paths) == 7
    assert sum(row["classification"] == "exact-path" for row in paths) == 6
    assert sum(row["classification"] == "not-embedded" for row in paths) == 1


def test_random_unit_is_complete_and_consistent_across_builds() -> None:
    repository = Path(__file__).resolve().parents[2]
    unit = [row for row in _accepted_functions(repository) if row["source_path"] == "sgp/Random.c"]
    harness = _harness_rows(repository, "random")

    # Wizardry does not define JA2, so PRERANDOM_GENERATOR is off and the unit
    # compiles to exactly these three functions. All three are exact.
    assert [row["claimed_name"] for row in unit] == ["InitializeRandom", "Random", "Chance"]
    assert {row["authority"] for row in unit} == {"source-backed"}

    for row in unit:
        mappings = [entry for entry in harness if entry["function"] == row["claimed_name"]]
        by_build = {entry["build"]: entry for entry in mappings}
        assert by_build["gog_base"]["address"] == row["address"]
        # The demo carries the whole unit at the same +0x360 shift as DirectDraw Calls.c.
        assert int(by_build["demo"]["address"], 16) - int(row["address"], 16) == 0x360
        # The packed 1.28 patch executable and the protected retail build are
        # recorded as unavailable, never as absent.
        assert by_build["gog_128_patch"]["classification"] == "unavailable"
        assert by_build["retail_2001_12_23"]["classification"] == "unavailable"
        assert by_build["gog_128_patch"]["address"] == ""
        assert by_build["retail_2001_12_23"]["address"] == ""


def test_the_source_oracle_owns_the_name_at_0x0040efa0() -> None:
    repository = Path(__file__).resolve().parents[2]
    rows = [row for row in _accepted_functions(repository) if row["address"] == "0040efa0"]
    assert len(rows) == 1
    row = rows[0]
    assert row["claimed_name"] == "Random"
    assert row["name_origin"] == "sgp-source"
    assert row["authority"] == "source-backed"


def test_sgp_harness_declares_the_settled_project_profile_and_reviewed_builds() -> None:
    repository = Path(__file__).resolve().parents[2]
    assert PROJECT_FLAGS == ("/O2", "/Ob2", "/G5", "/MD")
    assert {unit["id"] for unit in UNITS} == {
        "compression",
        "container",
        "dbman",
        "directdraw",
        "fileman",
        "librarydatabase",
        "random",
        "debug",
        "exceptionhandling",
        "sgp",
        "timer",
        "input",
    }
    assert {build["id"] for build in BUILDS} >= {
        "demo",
        "gog_base",
        "gog_1261",
        "gog_128_base",
    }
    cmake = (repository / "cmake/Sgp.cmake").read_text(encoding="utf-8")
    assert "WIZ8_SGP_WHOLE_SOURCES" in cmake
    assert "WIZ8_SGP_RUNTIME_PARTIAL_SOURCES" in cmake
    assert "WIZ8_SGP_ANALYSIS_ONLY_SOURCES" in cmake
    exception_unit = next(unit for unit in UNITS if unit["id"] == "exceptionhandling")
    assert exception_unit["expected_empty"] is True
    selected = {unit["id"]: unit.get("functions") for unit in UNITS}
    assert selected["sgp"] == ("GetRuntimeSettings", "ProcessCommandLine")
    assert set(selected["input"]) == {
        "KeyboardHandler",
        "MouseHandler",
        "InitializeInputManager",
        "ShutdownInputManager",
        "QueueEvent",
        "DequeueEvent",
        "FreeMouseCursor",
        "GetMouseWheelDeltaValue",
    }
    assert selected["timer"] is None


def test_every_configured_sgp_unit_has_a_reviewed_retention_class() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/sgp/units.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    by_unit = {row["unit"]: row for row in rows}
    assert len(rows) == len(by_unit) == len(UNITS)
    assert set(by_unit) == {unit["id"] for unit in UNITS}
    assert {unit: row["retention_class"] for unit, row in by_unit.items()} == {
        "directdraw": "partial",
        "random": "whole",
        "compression": "partial",
        "fileman": "partial",
        "librarydatabase": "partial",
        "dbman": "empty",
        "container": "partial",
        "debug": "partial",
        "exceptionhandling": "empty",
        "sgp": "partial",
        "timer": "whole",
        "input": "partial",
    }
    assert {unit for unit, row in by_unit.items() if row["bringup_linkage"] == "direct-object"} == {
        "random",
        "timer",
    }
    assert by_unit["container"]["retained_source_identities"] == "12"
    assert by_unit["container"]["retained_physical_bodies"] == "10"


def test_private_vsurface_adapter_preserves_c_linkage() -> None:
    repository = Path(__file__).resolve().parents[2]
    adapter = (repository / "include/wiz8/sgp_vsurface_private.h").read_text(encoding="utf-8")
    bringup = (repository / "src/wiz8/bringup_gates.cpp").read_text(encoding="utf-8")

    assert 'extern "C" {' in adapter
    assert '#include "vsurface_private.h"' in adapter
    assert '#include "wiz8/sgp_vsurface_private.h"' in bringup
    assert '#include "vsurface_private.h"' not in bringup


def test_sgp_csvs_have_one_surface_per_evidence_role() -> None:
    repository = Path(__file__).resolve().parents[2]
    assert (repository / "evidence/snapshots/sgp/harness.csv").is_file()
    assert (repository / "evidence/reviewed/sgp/findings.csv").is_file()
    assert (repository / "evidence/observations/sgp/source-paths.csv").is_file()
    with (repository / "evidence/reviewed/sgp/findings.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        assert all(None not in row for row in csv.DictReader(stream))


def test_sgp_sweep_rejects_unknown_units() -> None:
    with pytest.raises(RuntimeError, match="unknown SGP unit"):
        sweep_sgp_units(SimpleNamespace(), ["unknown"])


def test_relocation_masked_matcher_uses_the_five_way_vocabulary() -> None:
    function = CoffFunction(
        name="Example",
        body=b"\x55\x8b\xec\xe8\0\0\0\0\xc3",
        relocation_offsets=(4,),
    )
    assert classify_body(function, function.body, near_threshold=0.75)["classification"] == "exact"
    relocated = b"\x55\x8b\xec\xe8\x12\x34\x56\x78\xc3"
    assert (
        classify_body(function, relocated, near_threshold=0.75)["classification"]
        == "relocation-equivalent"
    )
    assert (
        classify_body(function, relocated + b"padding" + relocated, near_threshold=0.75)[
            "classification"
        ]
        == "ambiguous-generic"
    )
    near = b"\x55\x8b\xec\xe8\x12\x34\x56\x78\x90"
    assert (
        classify_body(function, near, near_threshold=0.75)["classification"]
        == "near-source-with-wiz8-modifications"
    )
    assert (
        classify_body(function, b"\xcc" * len(function.body), near_threshold=0.75)["classification"]
        == "absent-or-stripped"
    )


def test_source_functions_with_the_same_masked_body_remain_ambiguous() -> None:
    first = CoffFunction("First", b"\xe8\0\0\0\0\xc3", (1,))
    second = CoffFunction("Second", b"\xe8\x11\x22\x33\x44\xc3", (1,))
    build = BuildText("test", "0" * 64, 0x400000, b"\xe8\xaa\xbb\xcc\xdd\xc3", None)

    rows = _evaluate([(("/O2",), [first, second])], [build], 0.75)

    assert {row["classification"] for row in rows} == {"ambiguous-generic"}
    assert {row["address"] for row in rows} == {""}
    assert {row["hit_count"] for row in rows} == {1}


def test_sgp_evaluation_rejects_a_flag_search_matrix() -> None:
    matching = CoffFunction("Example", b"\xc3", ())
    different = CoffFunction("Example", b"\x90\xc3", ())
    build = BuildText("test", "0" * 64, 0x400000, b"\xc3", None)

    with pytest.raises(RuntimeError, match="exactly one settled project profile"):
        _evaluate(
            [(("/O1",), [matching]), (("/O2",), [different])],
            [build],
            0.75,
            ("/O2",),
        )


def test_compression_unit_classifies_every_emitted_function() -> None:
    repository = Path(__file__).resolve().parents[2]
    harness = _harness_rows(repository, "compression")
    reviewed = _reviewed_rows(repository, "compression")

    assert len(harness) == 9 * 7
    assert {row["flags"] for row in harness} == {"/O2 /Ob2 /G5 /MD"}
    assert sum(row["classification"] == "unavailable" for row in harness) == 18
    ambiguous = {row["function"] for row in harness if row["classification"] == "ambiguous-generic"}
    assert ambiguous == {"CompressFini", "DecompressFini"}

    assert [row["function"] for row in reviewed] == [
        "ZAlloc",
        "ZFree",
        "DecompressInit",
        "Decompress",
        "DecompressFini",
        "CompressedBufferSize",
        "CompressInit",
        "Compress",
        "CompressFini",
    ]
    assert [row["source_line"] for row in reviewed[:5]] == ["14", "19", "24", "55", "80"]
    assert {row["canonical_classification"] for row in reviewed[:5]} == {"relocation-equivalent"}
    assert {row["canonical_classification"] for row in reviewed[5:]} == {"absent-or-stripped"}
    assert reviewed[4]["canonical_address"] == "004158f0"
    assert "inflateEnd" in reviewed[4]["evidence"]
    assert "inflateEnd" in reviewed[-1]["evidence"]


def test_container_unit_separates_retained_stack_list_apis_from_stripped_families() -> None:
    repository = Path(__file__).resolve().parents[2]
    harness = _harness_rows(repository, "container")
    reviewed = _reviewed_rows(repository, "container")

    assert len(harness) == 32 * 7
    assert {row["flags"] for row in harness} == {"/O2 /Ob2 /G5 /MD"}
    assert len(reviewed) == 32
    assert sum(row["finding"] == "retained" for row in reviewed) == 12
    assert sum(row["finding"] == "stripped" for row in reviewed) == 20
    assert {row["function"] for row in reviewed if row["canonical_address"] == "00405b00"} == {
        "DeleteStack",
        "DeleteList",
    }
    assert {row["function"] for row in reviewed if row["canonical_address"] == "00405c00"} == {
        "StackSize",
        "ListSize",
    }

    accepted = [
        row for row in _accepted_functions(repository) if row["source_path"] == "sgp/Container.c"
    ]
    assert len(accepted) == 10
    assert {row["address"] for row in accepted} == {
        "00405970",
        "004059b0",
        "00405a00",
        "00405a70",
        "00405ac0",
        "00405b00",
        "00405b20",
        "00405b90",
        "00405c00",
        "00405c10",
    }


def test_debug_and_exception_support_boundaries_are_explicit() -> None:
    repository = Path(__file__).resolve().parents[2]
    debug = _harness_rows(repository, "debug")
    reviewed_debug = _reviewed_rows(repository, "debug")
    reviewed_exception = _reviewed_rows(repository, "exceptionhandling")
    accepted = [
        row for row in _accepted_functions(repository) if row["source_path"] == "sgp/DEBUG.C"
    ]

    assert len(debug) == 15 * 7
    assert len(reviewed_debug) == 15
    assert {row["flags"] for row in debug} == {"/O2 /Ob2 /G5 /MD"}
    assert [(row["claimed_name"], row["address"]) for row in accepted] == [("String", "00404b50")]
    assert reviewed_debug[-1]["canonical_classification"] == "relocation-equivalent"
    assert reviewed_exception[0]["finding"] == "compiled-empty"
    assert reviewed_exception[0]["canonical_classification"] == "compiled-empty"


def test_startup_input_and_timer_surfaces_are_restricted_to_retained_code() -> None:
    repository = Path(__file__).resolve().parents[2]
    accepted = _accepted_functions(repository)

    startup = _reviewed_rows(repository, "sgp")
    timer = _reviewed_rows(repository, "timer")
    input_rows = _reviewed_rows(repository, "input")

    assert len(_harness_rows(repository, "sgp")) == 2 * 7
    assert len(_harness_rows(repository, "timer")) == 6 * 7
    assert len(_harness_rows(repository, "input")) == 8 * 7
    assert len(startup) == 2
    assert len(timer) == 6
    assert len(input_rows) == 8
    assert {row["claimed_name"] for row in accepted if row["source_path"] == "sgp/sgp.c"} == {
        "GetRuntimeSettings",
        "ProcessCommandLine",
    }
    assert {row["claimed_name"] for row in accepted if row["source_path"] == "sgp/timer.c"} == {
        "Clock",
        "InitializeClockManager",
        "ShutdownClockManager",
        "GetClock",
        "SetCountdownClock",
        "ClockIsTicking",
    }
    assert {row["finding"] for row in startup + timer + input_rows} == {"retained"}


def test_fileman_exact_and_near_results_stay_separate() -> None:
    repository = Path(__file__).resolve().parents[2]
    harness = _harness_rows(repository, "fileman")
    near = _reviewed_rows(repository, "fileman")
    exact = [
        row for row in _accepted_functions(repository) if row["source_path"] == "sgp/FileMan.c"
    ]

    assert len(harness) == 43 * 7
    assert {row["flags"] for row in harness} == {"/O2 /Ob2 /G5 /MD"}
    assert len(exact) == 15
    assert {row["confidence"] for row in exact} == {"exact"}
    assert {row["function"] for row in near} == {"FileCheckEndOfFile", "GetFileFirst"}
    assert "stride 0x20" in near[0]["details"]
    assert "stride 0x28" in near[0]["details"]
    assert near[1]["build_scope"] == "gog_1261_new"


def test_vendored_sgp_source_retains_license_and_pinned_units() -> None:
    repository = Path(__file__).resolve().parents[2]
    source = repository / "third_party/sfi-sgp/sgp"
    expected = {
        "LibraryDataBase.c": "9322504f3557c8b704fb990d84fbe9be5640f5c34dcb1d9b8da632dfc0c2681a",
        "DbMan.c": "0d42da2c0be7e9c0f2935e0b5b619257997228a8f753aa5e0021793bd90b7d20",
        "SFI Source Code license agreement.txt": (
            "f78ace6a6cfd40cb1b49de2e5fd4a113ebc58cab4864e4a4e5fffd428005c7fd"
        ),
    }
    for relative, digest in expected.items():
        assert hashlib.sha256((source / relative).read_bytes()).hexdigest() == digest
    assert not [path for path in source.iterdir() if path.suffix.casefold() == ".lib"]


def test_library_database_units_keep_exact_near_and_interior_results_distinct() -> None:
    repository = Path(__file__).resolve().parents[2]
    library = _harness_rows(repository, "librarydatabase")
    dbman = _harness_rows(repository, "dbman")
    reviewed_library = _reviewed_rows(repository, "librarydatabase")
    reviewed_dbman = _reviewed_rows(repository, "dbman")
    accepted = [
        row
        for row in _accepted_functions(repository)
        if row["source_path"] == "sgp/LibraryDataBase.c"
    ]

    assert len(library) == 23 * 7
    assert len(dbman) == 20 * 7
    assert {row["flags"] for row in library + dbman} == {"/O2 /Ob2 /G5 /MD"}
    assert {row["claimed_name"] for row in accepted} == {
        "ShutDownFileDatabase",
        "CreateRealFileHandle",
        "GetLibraryAndFileIDFromLibraryFileHandle",
        "CompareDirEntryFileNames",
    }
    assert {row["function"] for row in reviewed_library} == {
        "CheckIfFileExistInLibrary",
        "CompareFileNames",
    }
    assert reviewed_dbman[0]["function"] == "DbExists"
    assert reviewed_dbman[0]["canonical_classification"] == "rejected-interior-match"
    assert reviewed_dbman[0]["canonical_address"] == "00519bee"
