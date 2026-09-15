import json
from pathlib import Path

from wiz8decomp.identity_lint import identity_violations, validate_identity


def _repository(
    tmp_path: Path, markers: list[dict], declarations: list[dict] | None = None
) -> Path:
    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n"
        "  SREXT_JPEGIMPORTER:\n"
        "    filename: srEXT_JPEGImporter.dll\n"
        "    source-root: src/srext_jpegimporter\n"
        "    hash:\n"
        "      sha256: abc\n"
        "  SREXT_UNZIP:\n"
        "    filename: srEXT_Unzip.dll\n"
        "    source-root: src/srext_unzip\n"
        "    hash:\n"
        "      sha256: def\n"
    )
    build = tmp_path / "build"
    build.mkdir(exist_ok=True)
    (build / "source-index.json").write_text(
        json.dumps(
            {
                "schema": "reccmp-source-index-v2",
                "markers": markers,
                "declarations": declarations or [],
                "classes": [],
                "variables": [],
                "conflicts": [],
            }
        ),
        encoding="utf-8",
    )
    return tmp_path


def _marker(target: str, source: str, address: int, name: str) -> dict:
    return {
        "address": address,
        "marker_kind": "FUNCTION",
        "source_file": source,
        "line": 1,
        "declaration": None,
        "marker_name": name,
        "folded": False,
        "target": target,
    }


def _declaration(name: str, *, is_definition: bool) -> dict:
    return {
        "qualified_name": name,
        "semantic_id": "",
        "semantic_kind": "free_function",
        "calling_convention": "__cdecl",
        "return_type": "void",
        "parameter_types": [],
        "has_this": False,
        "source_file": "src/srext_unzip/test.cpp",
        "line": 7,
        "end_line": 7,
        "is_definition": is_definition,
    }


def test_same_rva_in_two_binaries_is_not_a_collision(tmp_path: Path) -> None:
    repository = _repository(
        tmp_path,
        [
            _marker(
                "SREXT_JPEGIMPORTER",
                "src/srext_jpegimporter/codec_adapter.cpp",
                0x10001000,
                "srJPEG_read_header_adapter",
            ),
            _marker("SREXT_UNZIP", "src/srext_unzip/api_subset.c", 0x10001000, "setFileNotFound"),
        ],
    )
    assert identity_violations(repository) == []
    assert validate_identity(repository)["ok"]


def test_same_rva_in_one_binary_still_collides(tmp_path: Path) -> None:
    repository = _repository(
        tmp_path,
        [
            _marker("SREXT_UNZIP", "src/srext_unzip/api_subset.c", 0x10001000, "setFileNotFound"),
            _marker("SREXT_UNZIP", "src/srext_unzip/windll_subset.c", 0x10001000, "otherEntry"),
        ],
    )
    (violations,) = identity_violations(repository)
    assert violations["reason"] == "multiple names"
    assert "SREXT_UNZIP" in violations["detail"]


def test_two_address_qualified_declarations_cannot_claim_one_identity(tmp_path: Path) -> None:
    declarations = [
        {**_declaration("Function536F60", is_definition=False), "line": 1, "end_line": 1},
        {**_declaration("TargetIsInPlay", is_definition=False), "line": 2, "end_line": 2},
    ]
    repository = _repository(tmp_path, [], declarations)
    source = repository / "src/srext_unzip/test.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "void Function536F60(); /* 0x00536F60 */\nvoid TargetIsInPlay(); /* 0x00536F60 */\n",
        encoding="utf-8",
    )

    (violation,) = identity_violations(repository)
    assert violation["kind"] == "address-identity"
    assert violation["reason"] == "multiple names"
    assert violation["names"] == ["Function536F60", "TargetIsInPlay"]


def test_documented_address_alias_does_not_create_a_second_owner(tmp_path: Path) -> None:
    declarations = [
        {**_declaration("CanonicalName", is_definition=False), "line": 1, "end_line": 1},
        {**_declaration("FoldedName", is_definition=False), "line": 3, "end_line": 3},
    ]
    repository = _repository(tmp_path, [], declarations)
    source = repository / "src/srext_unzip/test.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "void CanonicalName(); /* 0x10001000 */\n"
        "// identity-alias: compiler fold onto CanonicalName\n"
        "void FoldedName(); /* 0x10001000 */\n",
        encoding="utf-8",
    )

    assert identity_violations(repository) == []


def test_address_derived_name_is_allowed_for_declaration_only(tmp_path: Path) -> None:
    repository = _repository(tmp_path, [], [_declaration("Function41AAE0", is_definition=False)])

    assert identity_violations(repository) == []
    assert validate_identity(repository)["ok"]


def test_address_derived_name_is_rejected_for_recovered_body(tmp_path: Path) -> None:
    repository = _repository(
        tmp_path,
        [],
        [
            _declaration("Function422B10", is_definition=True),
            _declaration("W8Thing::FUNCTION49FDB0", is_definition=True),
        ],
    )

    violations = identity_violations(repository)
    assert [item["kind"] for item in violations] == [
        "unnamed-function-definition",
        "unnamed-function-definition",
    ]
    assert [item["names"] for item in violations] == [
        ["Function422B10"],
        ["W8Thing::FUNCTION49FDB0"],
    ]


def test_named_recovered_body_is_allowed(tmp_path: Path) -> None:
    repository = _repository(
        tmp_path, [], [_declaration("ClearTransientOverlayFrame00422B10", is_definition=True)]
    )

    assert identity_violations(repository) == []
