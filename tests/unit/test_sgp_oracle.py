import csv
import hashlib
from pathlib import Path
from types import SimpleNamespace

import pytest
import yaml
from wiz8decomp.sgp_oracle import (
    BuildText,
    CoffFunction,
    _evaluate,
    classify_body,
    sweep_sgp_units,
)


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


def test_vendored_sgp_source_exposes_the_wizardry_branch_census() -> None:
    repository = Path(__file__).resolve().parents[2]
    source = repository / "third_party/sfi-sgp/sgp"
    candidates = [
        path
        for path in source.iterdir()
        if path.is_file() and "WIZ8_PRECOMPILED_HEADERS" in path.read_text(
            encoding="latin-1", errors="replace"
        )
    ]
    active = [
        path
        for path in candidates
        if any(
            "WIZ8_PRECOMPILED_HEADERS" in line
            and not line.lstrip().startswith("//")
            for line in path.read_text(encoding="latin-1", errors="replace").splitlines()
        )
    ]

    assert len(active) == 33
    assert {path.name for path in set(candidates) - set(active)} == {"MemMan.c"}


def test_sgp_maps_keep_exact_and_absent_evidence_distinct() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/wiz8/functions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        functions = [
            row for row in csv.DictReader(stream) if row["source_path"].startswith("sgp/")
        ]
    with (repository / "evidence/observations/sgp/source-paths.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        paths = list(csv.DictReader(stream))

    assert len(functions) == 62
    assert {row["confidence"] for row in functions} == {"exact"}
    assert {row["owner"] for row in functions} == {"sgp-shared"}
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
    with (repository / "evidence/reviewed/wiz8/functions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        unit = [row for row in csv.DictReader(stream) if row["source_path"] == "sgp/Random.c"]
    harness = _harness_rows(repository, "random")

    # Wizardry does not define JA2, so PRERANDOM_GENERATOR is off and the unit
    # compiles to exactly these three functions. All three are exact.
    assert [row["provisional_name"] for row in unit] == ["InitializeRandom", "Random", "Chance"]
    assert {row["authority"] for row in unit} == {"source-backed"}

    for row in unit:
        mappings = [entry for entry in harness if entry["function"] == row["provisional_name"]]
        by_build = {entry["build"]: entry for entry in mappings}
        assert by_build["gog_base"]["address"] == row["address"]
        assert row["size"] == by_build["gog_base"]["size"]
        # The demo carries the whole unit at the same +0x360 shift as DirectDraw Calls.c.
        assert int(by_build["demo"]["address"], 16) - int(row["address"], 16) == 0x360
        # The packed 1.28 patch executable and the protected retail build are
        # recorded as unavailable, never as absent.
        assert by_build["gog_128_patch"]["classification"] == "unavailable"
        assert by_build["retail_2001_12_23"]["classification"] == "unavailable"
        assert by_build["gog_128_patch"]["address"] == ""
        assert by_build["retail_2001_12_23"]["address"] == ""


def test_the_sgp_name_supersedes_the_cfagent_name_at_0x0040efa0() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/wiz8/functions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = [row for row in csv.DictReader(stream) if row["address"] == "0040efa0"]
    assert len(rows) == 1
    row = rows[0]
    assert row["provisional_name"] == "Random"
    assert row["aliases"] == "GetRandomNumber"
    assert set(row["name_origin"].split("|")) == {"sgp-source", "fan-patch-signature"}
    assert row["authority"] == "source-backed"

    with (repository / "evidence/reviewed/wiz8/function-evidence.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        evidence = [row for row in csv.DictReader(stream) if row["address"] == "0040efa0"]
    assert {row["origin"] for row in evidence} == {"cfagent-oracle", "sgp"}


def test_sgp_harness_declares_the_settled_project_profile_and_reviewed_builds() -> None:
    repository = Path(__file__).resolve().parents[2]
    harness = yaml.safe_load(
        (repository / "config/sgp.yml").read_text(encoding="utf-8")
    )
    units = [unit for unit in harness["units"] if unit.get("harness", True)]
    assert harness["schema"] == "wiz8.sgp-harness"
    assert harness["project_flags"] == ["/O2", "/Ob2", "/G5", "/MD"]
    assert {unit["id"] for unit in units} == {
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
    assert "flag_axes" not in harness
    assert {build["id"] for build in harness["builds"]} >= {
        "demo",
        "gog_base",
        "gog_1261",
        "gog_128_base",
    }
    assert harness["report"] == "build/reports/sgp/harness.csv"
    assert harness["snapshot"] == "evidence/snapshots/sgp/harness.csv"
    exception_unit = next(
        unit for unit in units if unit["id"] == "exceptionhandling"
    )
    assert exception_unit["expected_empty"] is True
    selected = {unit["id"]: unit.get("functions") for unit in units}
    assert selected["sgp"] == ["GetRuntimeSettings", "ProcessCommandLine"]
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
    harness = yaml.safe_load((repository / "config/sgp.yml").read_text(encoding="utf-8"))
    harness_units = [unit for unit in harness["units"] if unit.get("harness", True)]
    with (repository / "evidence/reviewed/sgp/units.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    by_unit = {row["unit"]: row for row in rows}
    assert len(rows) == len(by_unit) == len(harness_units)
    assert set(by_unit) == {unit["id"] for unit in harness_units}
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
    assert {
        unit for unit, row in by_unit.items() if row["bringup_linkage"] == "direct-object"
    } == {"random", "timer"}
    assert by_unit["container"]["retained_source_identities"] == "12"
    assert by_unit["container"]["retained_physical_bodies"] == "10"


def test_noref_bringup_links_only_whole_retained_sgp_units() -> None:
    repository = Path(__file__).resolve().parents[2]
    cmake = (repository / "CMakeLists.txt").read_text(encoding="utf-8")
    bringup_sources = cmake.rsplit("target_sources(WIZ8_BRINGUP PRIVATE", 1)[1].split(
        ")", 1
    )[0]

    assert 'file(READ "${CMAKE_CURRENT_SOURCE_DIR}/config/sgp.yml" WIZ8_SGP_MODEL)' in cmake
    assert 'string(JSON group_count LENGTH "${WIZ8_SGP_MODEL}"' in cmake
    assert "$<TARGET_OBJECTS:WIZ8_SGP_RETAINED>" in bringup_sources
    for partial_or_empty in (
        "DIRECTDRAW",
        "COMPRESSION",
        "FILEMAN",
        "LIBRARY_DATABASE",
        "DBMAN",
        "CONTAINER",
        "DEBUG",
        "EXCEPTION_HANDLING",
        "CORE",
        "INPUT",
    ):
        assert f"$<TARGET_OBJECTS:WIZ8_SGP_{partial_or_empty}>" not in bringup_sources

    assert "src/wiz8/random_number.c" not in cmake
    assert not (repository / "src/wiz8/random_number.c").exists()


def test_sgp_csvs_have_one_surface_per_evidence_role() -> None:
    repository = Path(__file__).resolve().parents[2]
    assert (repository / "evidence/snapshots/sgp/harness.csv").is_file()
    assert (repository / "evidence/reviewed/sgp/findings.csv").is_file()
    assert (repository / "evidence/observations/sgp/source-paths.csv").is_file()
    with (repository / "evidence/reviewed/sgp/findings.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        assert all(None not in row for row in csv.DictReader(stream))


def test_partial_sgp_sweep_cannot_replace_the_reviewed_snapshot() -> None:
    with pytest.raises(RuntimeError, match="complete sweep"):
        sweep_sgp_units(SimpleNamespace(), ["random"], update_snapshot=True)


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
    assert {row["canonical_classification"] for row in reviewed[:5]} == {
        "relocation-equivalent"
    }
    assert {row["canonical_classification"] for row in reviewed[5:]} == {
        "absent-or-stripped"
    }
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
    assert {
        row["function"]
        for row in reviewed
        if row["canonical_address"] == "00405b00"
    } == {"DeleteStack", "DeleteList"}
    assert {
        row["function"]
        for row in reviewed
        if row["canonical_address"] == "00405c00"
    } == {"StackSize", "ListSize"}

    with (repository / "evidence/reviewed/wiz8/functions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        accepted = [
            row for row in csv.DictReader(stream) if row["source_path"] == "sgp/Container.c"
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
    with (repository / "evidence/reviewed/wiz8/functions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        accepted = [
            row for row in csv.DictReader(stream) if row["source_path"] == "sgp/DEBUG.C"
        ]

    assert len(debug) == 15 * 7
    assert len(reviewed_debug) == 15
    assert {row["flags"] for row in debug} == {"/O2 /Ob2 /G5 /MD"}
    assert [(row["provisional_name"], row["address"]) for row in accepted] == [
        ("String", "00404b50")
    ]
    assert reviewed_debug[-1]["canonical_classification"] == "relocation-equivalent"
    assert reviewed_exception[0]["finding"] == "compiled-empty"
    assert reviewed_exception[0]["canonical_classification"] == "compiled-empty"


def test_startup_input_and_timer_surfaces_are_restricted_to_retained_code() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/wiz8/functions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        accepted = list(csv.DictReader(stream))

    startup = _reviewed_rows(repository, "sgp")
    timer = _reviewed_rows(repository, "timer")
    input_rows = _reviewed_rows(repository, "input")

    assert len(_harness_rows(repository, "sgp")) == 2 * 7
    assert len(_harness_rows(repository, "timer")) == 6 * 7
    assert len(_harness_rows(repository, "input")) == 8 * 7
    assert len(startup) == 2
    assert len(timer) == 6
    assert len(input_rows) == 8
    assert {row["provisional_name"] for row in accepted if row["source_path"] == "sgp/sgp.c"} == {
        "GetRuntimeSettings",
        "ProcessCommandLine",
    }
    assert {row["provisional_name"] for row in accepted if row["source_path"] == "sgp/timer.c"} == {
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
    with (repository / "evidence/reviewed/wiz8/functions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        exact = [row for row in csv.DictReader(stream) if row["source_path"] == "sgp/FileMan.c"]

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
    with (repository / "evidence/reviewed/wiz8/functions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        accepted = [
            row for row in csv.DictReader(stream) if row["source_path"] == "sgp/LibraryDataBase.c"
        ]

    assert len(library) == 23 * 7
    assert len(dbman) == 20 * 7
    assert {row["flags"] for row in library + dbman} == {"/O2 /Ob2 /G5 /MD"}
    assert {row["provisional_name"] for row in accepted} == {
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
