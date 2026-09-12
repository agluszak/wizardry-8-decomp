import json
import os
from pathlib import Path

import pytest
from reccmp.source import SourceIndexError
from wiz8decomp import source_index
from wiz8decomp.config import Settings
from wiz8decomp.source_index import source_functions, target_for_program


def _settings(tmp_path: Path) -> Settings:
    return Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": tmp_path / "ghidra",
            "WIZ8_INPUT_DIR": tmp_path / "inputs",
            "WIZ8_WORK_DIR": tmp_path / "work",
            "repo_dir": tmp_path / "repo",
        }
    )


def test_compile_db_files_include_local_checkout_paths(tmp_path: Path) -> None:
    repository = tmp_path / "checkout"
    local = repository / "src/wiz8/Combat.cpp"
    local.parent.mkdir(parents=True)
    local.write_text("", encoding="utf-8")
    database = tmp_path / "compile_commands.json"
    database.write_text(
        json.dumps(
            [
                {"file": "/repo/src/wiz8/Combat.cpp"},
                {"file": str(local)},
                {"file": "/zlib/adler32.c"},
                {"file": "src/surrender/srCore.cpp"},
            ]
        ),
        encoding="utf-8",
    )

    assert source_index._compile_db_files(database, repository) == {
        "src/wiz8/Combat.cpp",
        "src/surrender/srCore.cpp",
    }


def test_program_target_resolution_uses_configured_identity() -> None:
    repository = Path(__file__).resolve().parents[2]

    assert target_for_program(repository, "srEXT_Unzip.dll") == "SREXT_UNZIP"
    assert target_for_program(repository, "wiz8--gog-base--sr--cec1caf85861") == "SURRENDER"
    with pytest.raises(SourceIndexError, match="no configured reccmp target"):
        target_for_program(repository, "unregistered.dll")


def test_synthetic_marker_cannot_own_a_declaration(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/item.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "// SYNTHETIC: WIZ8 0x0049F420\n"
        "// W8Item::`scalar deleting destructor'\n"
        "W8Item::~W8Item() {}\n",
        encoding="utf-8",
    )

    with pytest.raises(SourceIndexError, match="SYNTHETIC owns no declaration or body"):
        source_index.validate_synthetic_marker_blocks(tmp_path)

    source.write_text(
        "// SYNTHETIC: WIZ8 0x0049F420\n"
        "// W8Item::`scalar deleting destructor'\n\n"
        "// FUNCTION: WIZ8 0x0049F440\n"
        "W8Item::~W8Item() {}\n",
        encoding="utf-8",
    )
    assert source_index.validate_synthetic_marker_blocks(tmp_path) == 1


@pytest.mark.parametrize("existing_database", [False, True])
def test_source_index_configures_missing_or_stale_compile_database(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, existing_database: bool
) -> None:
    settings = _settings(tmp_path)
    repository = settings.repo_dir
    inventory = repository / "CMakeLists.txt"
    inventory.parent.mkdir(parents=True)
    inventory.write_text("project(wiz8)\n", encoding="utf-8")
    (repository / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    source-root: src/wiz8\n"
        "    hash:\n      sha256: abc\n",
        encoding="utf-8",
    )
    database = repository / "build/clang/compile_commands.json"
    if existing_database:
        database.parent.mkdir(parents=True)
        database.write_text("[]\n", encoding="utf-8")
        os.utime(database, ns=(1_000_000_000, 1_000_000_000))
        os.utime(inventory, ns=(2_000_000_000, 2_000_000_000))

    configured: list[bool] = []

    def configure(_settings: Settings) -> None:
        configured.append(True)
        database.parent.mkdir(parents=True, exist_ok=True)
        database.write_text("[]\n", encoding="utf-8")

    class FakeIndex:
        markers: tuple[()] = ()
        declarations: tuple[()] = ()
        classes: tuple[()] = ()
        variables: tuple[()] = ()
        conflicts: tuple[()] = ()

        def write(self, path: Path) -> None:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('{"schema": "reccmp-source-index-v2"}\n', encoding="utf-8")

    import wiz8decomp.build as build_module

    monkeypatch.setattr(build_module, "configure_clang", configure)
    monkeypatch.setattr(
        source_index.SourceIndex,
        "from_compile_database",
        lambda *_args, **_kwargs: FakeIndex(),
    )

    source_index.write_source_index(settings)

    assert configured == [True]


def test_surrender_source_functions_use_their_own_marker_target() -> None:
    repository = Path(__file__).resolve().parents[2]
    surrender = source_functions(repository, "SURRENDER")
    wiz8 = source_functions(repository, "WIZ8")

    assert surrender[0x10015010].name == "srCore::getCopyright"
    assert surrender[0x10045780].name == ("srDynamicLibrary::checkCompatibility")
    assert set(surrender).isdisjoint(wiz8)
    assert all(
        item.source_file.startswith(("src/surrender/", "include/surrender/"))
        for item in surrender.values()
    )


def test_source_functions_keep_definition_as_owner_of_folded_alias(tmp_path: Path) -> None:
    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
    )
    build = tmp_path / "build"
    build.mkdir()
    (build / "source-index.json").write_text(
        """{
  "schema": "reccmp-source-index-v2",
  "markers": [
    {
      "address": 4878656,
      "target": "WIZ8",
      "marker_kind": "FUNCTION",
      "source_file": "include/wiz8/engine_code/GrCycle.h",
      "line": 149,
      "declaration": {
        "semantic_id": "?CanEnterCycle@W8GrCycle@@UAEEC@Z",
        "qualified_name": "W8GrCycle::CanEnterCycle",
        "semantic_kind": "instance_method",
        "calling_convention": "__thiscall",
        "return_type": "unsigned char",
        "parameter_types": ["signed char"],
        "owning_class": "W8GrCycle",
        "has_this": true,
        "source_file": "include/wiz8/engine_code/GrCycle.h",
        "line": 149,
        "end_line": 150,
        "is_definition": true,
        "is_virtual": true
      },
      "marker_name": null
    },
    {
      "address": 4878656,
      "marker_kind": "SYNTHETIC",
      "target": "WIZ8",
      "source_file": "include/wiz8/engine_code/GrCycle.h",
      "line": 196,
      "declaration": null,
      "marker_name": "W8Navigator::secondary_vslot3",
      "folded": true
    }
  ],
  "declarations": [],
  "classes": []
}\n""",
        encoding="utf-8",
    )

    function = source_functions(tmp_path)[0x004A7140]
    assert function.name == "W8GrCycle::CanEnterCycle"
    assert function.marker_kind == "FUNCTION"


def test_source_functions_reject_two_non_folded_owners(tmp_path: Path) -> None:
    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
    )
    build = tmp_path / "build"
    build.mkdir()
    (build / "source-index.json").write_text(
        """{
  "schema": "reccmp-source-index-v2",
  "markers": [
    {
      "address": 1,
      "target": "WIZ8",
      "marker_kind": "SYNTHETIC",
      "source_file": "src/wiz8/a.cpp",
      "line": 1,
      "declaration": null,
      "marker_name": "First"
    },
    {
      "address": 1,
      "marker_kind": "SYNTHETIC",
      "source_file": "src/wiz8/b.cpp",
      "line": 1,
      "declaration": null,
      "marker_name": "Second"
      ,"target": "WIZ8"
    }
  ],
  "declarations": [],
  "classes": []
}\n""",
        encoding="utf-8",
    )

    with pytest.raises(SourceIndexError, match="more than one source owner"):
        source_functions(tmp_path)


def _cross_tu_index(
    tmp_path: Path,
    declarations: list[dict],
    variables: list[dict],
    conflicts: list[dict] | None = None,
) -> None:
    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
    )
    build = tmp_path / "build"
    build.mkdir(exist_ok=True)
    import json

    (build / "source-index.json").write_text(
        json.dumps(
            {
                "schema": "reccmp-source-index-v2",
                "markers": [],
                "declarations": declarations,
                "classes": [],
                "variables": variables,
                "conflicts": conflicts or [],
            }
        ),
        encoding="utf-8",
    )


def _declaration(
    semantic_id: str,
    source_file: str,
    line: int,
    *,
    linkage: str = "external",
    return_type: str = "void",
    parameters: list[str] | None = None,
) -> dict:
    return {
        "semantic_id": semantic_id,
        "qualified_name": semantic_id,
        "semantic_kind": "free_function",
        "calling_convention": "__cdecl",
        "return_type": return_type,
        "parameter_types": parameters or [],
        "owning_class": None,
        "has_this": False,
        "is_virtual": False,
        "source_file": source_file,
        "line": line,
        "end_line": line,
        "is_definition": True,
        "linkage": linkage,
        "storage_class": "none",
    }


def _variable(
    semantic_id: str, type: str, source_file: str, line: int, *, linkage: str = "external"
) -> dict:
    return {
        "semantic_id": semantic_id,
        "qualified_name": semantic_id,
        "type": type,
        "linkage": linkage,
        "storage_class": "none",
        "definition_kind": "definition",
        "source_file": source_file,
        "line": line,
        "end_line": line,
    }


def test_cross_tu_gate_covers_agreeing_functions_and_globals(tmp_path: Path) -> None:
    _cross_tu_index(
        tmp_path,
        [_declaration("_helper", "src/wiz8/a.cpp", 10)],
        [_variable("_gShared", "int", "src/wiz8/a.cpp", 3)],
    )
    assert source_index.validate_cross_tu_declarations(tmp_path) == 2


def test_cross_tu_gate_reports_conflicting_global_spellings(tmp_path: Path) -> None:
    _cross_tu_index(
        tmp_path,
        [],
        [
            _variable("_gThing", "Foo *", "src/wiz8/a.cpp", 3),
            _variable("_gThing", "int", "src/wiz8/b.cpp", 7),
        ],
    )
    with pytest.raises(SourceIndexError, match="_gThing"):
        source_index.validate_cross_tu_declarations(tmp_path)


def test_cross_tu_gate_ignores_tu_local_definitions(tmp_path: Path) -> None:
    _cross_tu_index(
        tmp_path,
        [
            _declaration("_helper", "src/wiz8/a.c", 10, linkage="internal"),
            _declaration("_helper", "src/wiz8/b.c", 4, linkage="internal"),
        ],
        [_variable("_counter", "int", "src/wiz8/a.c", 3, linkage="internal")],
    )
    assert source_index.validate_cross_tu_declarations(tmp_path) == 0


def test_cross_tu_gate_reports_recorded_collector_conflicts(tmp_path: Path) -> None:
    _cross_tu_index(
        tmp_path,
        [_declaration("_helper", "src/wiz8/a.c", 10)],
        [],
        [
            {
                "semantic_id": "_gThing",
                "qualified_name": "gThing",
                "record_kind": "variable",
                "variants": [
                    {
                        "signature": ["Foo *", "external"],
                        "locations": ["src/wiz8/a.cpp:3"],
                    },
                    {"signature": ["int", "external"], "locations": ["src/wiz8/b.cpp:7"]},
                ],
            }
        ],
    )
    with pytest.raises(SourceIndexError, match="_gThing"):
        source_index.validate_cross_tu_declarations(tmp_path)


def test_cross_tu_gate_ignores_conflicts_without_external_spelling(tmp_path: Path) -> None:
    _cross_tu_index(
        tmp_path,
        [],
        [],
        [
            {
                "semantic_id": "_buffer",
                "qualified_name": "buffer",
                "record_kind": "variable",
                "variants": [
                    {"signature": ["int[64]", "internal"], "locations": ["src/wiz8/a.c:3"]},
                    {
                        "signature": ["char[64]", "internal"],
                        "locations": ["src/wiz8/b.c:5"],
                    },
                ],
            }
        ],
    )
    assert source_index.validate_cross_tu_declarations(tmp_path) == 0


def test_cross_tu_gate_allows_extern_array_completion(tmp_path: Path) -> None:
    _cross_tu_index(
        tmp_path,
        [],
        [
            _variable("_gTable", "const unsigned short[]", "src/wiz8/a.cpp", 1),
            _variable("_gTable", "const unsigned short[2]", "src/wiz8/b.cpp", 2),
        ],
    )
    assert source_index.validate_cross_tu_declarations(tmp_path) == 1


def test_cross_tu_gate_allows_nested_array_completion(tmp_path: Path) -> None:
    _cross_tu_index(
        tmp_path,
        [],
        [
            _variable("_gGrid", "unsigned short[][4]", "src/wiz8/a.cpp", 1),
            _variable("_gGrid", "unsigned short[4][4]", "src/wiz8/b.cpp", 2),
        ],
    )
    assert source_index.validate_cross_tu_declarations(tmp_path) == 1


def test_cross_tu_gate_rejects_conflicting_array_extents(tmp_path: Path) -> None:
    _cross_tu_index(
        tmp_path,
        [],
        [
            _variable("_gTable", "char[3]", "src/wiz8/a.cpp", 1),
            _variable("_gTable", "char[4]", "src/wiz8/b.cpp", 2),
        ],
    )
    with pytest.raises(SourceIndexError, match="_gTable"):
        source_index.validate_cross_tu_declarations(tmp_path)


def test_cross_tu_gate_rejects_mismatched_array_element_type(tmp_path: Path) -> None:
    _cross_tu_index(
        tmp_path,
        [],
        [
            _variable("_gTable", "char[]", "src/wiz8/a.cpp", 1),
            _variable("_gTable", "int[4]", "src/wiz8/b.cpp", 2),
        ],
    )
    with pytest.raises(SourceIndexError, match="_gTable"):
        source_index.validate_cross_tu_declarations(tmp_path)
