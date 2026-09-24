from __future__ import annotations

import json
from pathlib import Path

import pytest
from wiz8decomp.source_model_lint import (
    SourceModelGateError,
    source_model_violations,
    validate_source_model,
)


def _index(
    tmp_path: Path,
    *,
    markers: list[dict] | None = None,
    declarations: list[dict] | None = None,
    variables: list[dict] | None = None,
    compiler_files: list[str] | None = None,
) -> Path:
    build = tmp_path / "build"
    build.mkdir(parents=True, exist_ok=True)
    (build / "source-index.json").write_text(
        json.dumps(
            {
                "schema": "reccmp-source-index-v3",
                "markers": markers or [],
                "declarations": declarations or [],
                "classes": [],
                "variables": variables or [],
                "conflicts": [],
            }
        ),
        encoding="utf-8",
    )
    units = tmp_path / "src/wiz8/source_units.json"
    units.parent.mkdir(parents=True, exist_ok=True)
    units.write_text(
        json.dumps(
            {
                "schema": "wiz8.source-units-v1",
                "compiler-emission": compiler_files or [],
                "unresolved-fragment": [],
                "original-path-map": {},
            }
        ),
        encoding="utf-8",
    )
    return tmp_path


def _definition(
    name: str,
    *,
    source_file: str = "src/wiz8/example.cpp",
    semantic_id: str = "?Function@@YAXXZ",
    owning_class: str | None = None,
) -> dict:
    return {
        "target": "WIZ8",
        "unit_id": None,
        "semantic_id": semantic_id,
        "qualified_name": name,
        "semantic_kind": "instance_method" if owning_class else "free_function",
        "calling_convention": "__thiscall" if owning_class else "__cdecl",
        "return_type": "void",
        "parameter_types": [],
        "owning_class": owning_class,
        "has_this": owning_class is not None,
        "source_file": source_file,
        "line": 2,
        "end_line": 2,
        "is_definition": True,
    }


def _marker(
    kind: str,
    *,
    name: str | None = None,
    source_file: str = "src/wiz8/example.cpp",
    declaration_key: list[str | None] | None = None,
) -> dict:
    return {
        "address": 0x00401000,
        "target": "WIZ8",
        "marker_kind": kind,
        "source_file": source_file,
        "line": 1,
        "marker_name": name,
        "folded": False,
        "declaration_key": declaration_key,
    }


def test_function_marker_cannot_claim_class_template_emission(tmp_path: Path) -> None:
    declaration = _definition(
        "W8GrowableVector<int>::Grow",
        semantic_id="?Grow@?$W8GrowableVector@H@@QAEHH@Z",
        owning_class="W8GrowableVector<int>",
    )
    marker = _marker(
        "FUNCTION",
        declaration_key=["WIZ8", declaration["semantic_id"], None],
    )
    repository = _index(tmp_path, markers=[marker], declarations=[declaration])

    violations = source_model_violations(repository)

    assert [item["kind"] for item in violations] == ["template-emission-as-function"]


@pytest.mark.parametrize(
    "kind,name",
    [
        ("FUNCTION", "W8Thing::`scalar deleting destructor'"),
        ("TEMPLATE", "W8Thing::`vector deleting destructor'"),
        ("FUNCTION", "W8Thing::Method`vtordisp{-4, 0}'"),
        ("FUNCTION", "W8Thing::Method`adjustor{12}'"),
    ],
)
def test_compiler_helpers_must_be_synthetic(tmp_path: Path, kind: str, name: str) -> None:
    repository = _index(tmp_path, markers=[_marker(kind, name=name)])

    with pytest.raises(SourceModelGateError, match="compiler-helper-as-authored"):
        validate_source_model(repository)


@pytest.mark.parametrize("kind", ["SYNTHETIC", "LIBRARY"])
def test_marker_only_emissions_cannot_bind_authored_declarations(tmp_path: Path, kind: str) -> None:
    declaration = _definition("W8Thing::Body", owning_class="W8Thing")
    marker = _marker(
        kind,
        name="W8Thing::Body",
        declaration_key=["WIZ8", declaration["semantic_id"], None],
    )
    repository = _index(tmp_path, markers=[marker], declarations=[declaration])

    with pytest.raises(SourceModelGateError, match="emission-marker-binds-declaration"):
        validate_source_model(repository)


def test_compiler_emission_tu_cannot_contain_authored_definition(tmp_path: Path) -> None:
    path = "src/wiz8/vector.cpp"
    declaration = _definition("HandWrittenBody", source_file=path)
    repository = _index(
        tmp_path,
        declarations=[declaration],
        compiler_files=[path],
    )

    with pytest.raises(SourceModelGateError, match="authored-definition-in-compiler-unit"):
        validate_source_model(repository)


def test_compiler_emission_tu_allows_marker_only_provenance(tmp_path: Path) -> None:
    path = "src/wiz8/vector.cpp"
    repository = _index(
        tmp_path,
        markers=[
            _marker("TEMPLATE", name="W8GrowableVector<int>::Grow", source_file=path),
            _marker(
                "SYNTHETIC",
                name="W8GrowableVector<int>::`scalar deleting destructor'",
                source_file=path,
            ),
        ],
        compiler_files=[path],
    )

    assert validate_source_model(repository)["ok"] is True


def test_typed_object_literal_raw_offset_is_hard_error(tmp_path: Path) -> None:
    repository = _index(tmp_path)
    source = repository / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True, exist_ok=True)
    source.write_text(
        "int read(W8Record* record) {\n"
        "    return *reinterpret_cast<int*>(reinterpret_cast<char*>(record) + 0x24); "
        "// raw-offset-ok: this must not waive a typed object\n"
        "}\n",
        encoding="utf-8",
    )

    with pytest.raises(SourceModelGateError, match="typed-object-raw-offset"):
        validate_source_model(repository)


def test_this_literal_raw_offset_is_hard_error(tmp_path: Path) -> None:
    repository = _index(tmp_path)
    source = repository / "include/wiz8/example.h"
    source.parent.mkdir(parents=True, exist_ok=True)
    source.write_text(
        "struct W8Thing { int f() { return *(int*)(reinterpret_cast<char*>(this) + 4); } };\n",
        encoding="utf-8",
    )

    with pytest.raises(SourceModelGateError, match="typed-object-raw-offset"):
        validate_source_model(repository)


def test_unresolved_raw_buffer_is_left_to_diff_scoped_cast_gate(tmp_path: Path) -> None:
    repository = _index(tmp_path)
    source = repository / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True, exist_ok=True)
    source.write_text(
        "char* field(void* raw) { return reinterpret_cast<char*>(raw) + 0x24; }\n",
        encoding="utf-8",
    )

    assert validate_source_model(repository)["ok"] is True


def test_variable_offset_is_not_a_layout_claim(tmp_path: Path) -> None:
    repository = _index(tmp_path)
    source = repository / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True, exist_ok=True)
    source.write_text(
        "char* field(W8Record* record, int offset) {\n"
        "    return reinterpret_cast<char*>(record) + offset;\n"
        "}\n",
        encoding="utf-8",
    )

    assert validate_source_model(repository)["ok"] is True


def test_inline_function_pointer_reinterpret_cast_is_hard_error(tmp_path: Path) -> None:
    repository = _index(tmp_path)
    source = repository / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True, exist_ok=True)
    source.write_text(
        "void call(unsigned long address, W8Thing* thing) {\n"
        "    reinterpret_cast<void (__thiscall *)(W8Thing*)>(address)(thing);\n"
        "}\n",
        encoding="utf-8",
    )

    with pytest.raises(SourceModelGateError, match="callable-reinterpret-cast"):
        validate_source_model(repository)


def test_normal_callback_invocation_is_allowed(tmp_path: Path) -> None:
    repository = _index(tmp_path)
    source = repository / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True, exist_ok=True)
    source.write_text(
        "typedef void (__cdecl *Callback)(int);\nvoid call(Callback callback) { callback(1); }\n",
        encoding="utf-8",
    )

    assert validate_source_model(repository)["ok"] is True
