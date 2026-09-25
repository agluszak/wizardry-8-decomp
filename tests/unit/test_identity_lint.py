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
        "declaration_key": None,
        "marker_name": name,
        "folded": False,
        "target": target,
    }


def _declaration(name: str, *, is_definition: bool) -> dict:
    return {
        "target": None,
        "unit_id": None,
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


def test_linker_fold_comment_does_not_alias_two_address_identities(tmp_path: Path) -> None:
    declarations = [
        {**_declaration("CanonicalName", is_definition=False), "line": 1, "end_line": 1},
        {**_declaration("FoldedName", is_definition=False), "line": 3, "end_line": 3},
    ]
    repository = _repository(tmp_path, [], declarations)
    source = repository / "src/srext_unzip/test.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "void CanonicalName(); /* 0x10001000 */\n"
        "// Retail ICF folded this onto CanonicalName.\n"
        "void FoldedName(); /* 0x10001000 */\n",
        encoding="utf-8",
    )

    (violation,) = identity_violations(repository)
    assert violation["reason"] == "multiple names"
    assert violation["names"] == ["CanonicalName", "FoldedName"]


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


def test_v3_declaration_key_binds_marker_to_definition(tmp_path: Path) -> None:
    """reccmp-source-index-v3 stores declaration_key, not an embedded declaration."""

    definition = {
        **_declaration("FindNpcScriptQuoteByKeyword", is_definition=True),
        "semantic_id": "?FindNpcScriptQuoteByKeyword@@YAHPAGPAF1@Z",
        "source_file": "src/srext_unzip/npc.cpp",
        "line": 2,
        "end_line": 2,
        "target": "SREXT_UNZIP",
    }
    stale = {
        **_declaration("FindNpcKeywordQuote", is_definition=False),
        "semantic_id": "?FindNpcKeywordQuote@@YAHPAGPAF1@Z",
        "source_file": "src/srext_unzip/npc.h",
        "line": 2,
        "end_line": 2,
        "target": "SREXT_UNZIP",
    }
    marker = {
        "address": 0x00525E80,
        "marker_kind": "FUNCTION",
        "source_file": "src/srext_unzip/npc.cpp",
        "line": 1,
        "marker_name": None,
        "folded": False,
        "target": "SREXT_UNZIP",
        "declaration_key": ["SREXT_UNZIP", "?FindNpcScriptQuoteByKeyword@@YAHPAGPAF1@Z", None],
    }
    repository = _repository(tmp_path, [marker], [definition, stale])
    (tmp_path / "src/srext_unzip").mkdir(parents=True)
    (tmp_path / "src/srext_unzip/npc.cpp").write_text(
        "// FUNCTION: SREXT_UNZIP 0x00525E80\n"
        "int FindNpcScriptQuoteByKeyword(wchar_t* keyword, short* a, short* b) { return 0; }\n",
        encoding="utf-8",
    )
    (tmp_path / "src/srext_unzip/npc.h").write_text(
        "/* 0x00525E80: stale spelling */\n"
        "int FindNpcKeywordQuote(wchar_t* keyword, short* a, short* b);\n",
        encoding="utf-8",
    )

    (violation,) = identity_violations(repository)
    assert violation["reason"] == "multiple names"
    assert set(violation["names"]) == {"FindNpcKeywordQuote", "FindNpcScriptQuoteByKeyword"}


def test_preceding_block_comment_address_binds_header_declaration(tmp_path: Path) -> None:
    first = {
        **_declaration("CanonicalName", is_definition=False),
        "line": 1,
        "end_line": 1,
        "semantic_id": "?CanonicalName@@YAXXZ",
    }
    second = {
        **_declaration("StaleName", is_definition=False),
        "line": 3,
        "end_line": 3,
        "semantic_id": "?StaleName@@YAXXZ",
    }
    repository = _repository(tmp_path, [], [first, second])
    source = repository / "src/srext_unzip/test.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "void CanonicalName(); /* 0x0052A1A0 */\n"
        "/* 0x0052A1A0: stale alias documented in a block comment */\n"
        "void StaleName();\n",
        encoding="utf-8",
    )
    (repository / "build/source-index.json").write_text(
        json.dumps(
            {
                "schema": "reccmp-source-index-v3",
                "markers": [],
                "declarations": [first, second],
                "classes": [],
                "variables": [],
                "conflicts": [],
            }
        ),
        encoding="utf-8",
    )

    (violation,) = identity_violations(repository)
    assert violation["reason"] == "multiple names"
    assert set(violation["names"]) == {"CanonicalName", "StaleName"}


def test_templated_operator_overload_is_not_a_consumer_redeclaration(tmp_path: Path) -> None:
    """Address-owned operator+ must not collide with an unrelated template overload.

    Reproduces the main regression: WIZ8 0x0047D2A0 owns
    ``srInlineString operator+(const srInlineString&, const srInlineString&)``
    while ``template <class T> srVector3T<T> operator+`` is a distinct overload.
    """

    owned = {
        **_declaration("operator+", is_definition=True),
        "semantic_id": "??H@YA?AUsrInlineString@@ABU0@0@Z",
        "return_type": "struct srInlineString",
        "parameter_types": [
            "const struct srInlineString &",
            "const struct srInlineString &",
        ],
        "source_file": "src/srext_unzip/string_ops.cpp",
        "line": 2,
        "end_line": 2,
        "target": "SREXT_UNZIP",
    }
    template_primary = {
        **_declaration("operator+", is_definition=True),
        "semantic_id": (
            "FunctionDecl:operator+:srVector3T<type-parameter-0-0> "
            "(const srVector3T<type-parameter-0-0> &, "
            "const srVector3T<type-parameter-0-0> &)"
        ),
        "return_type": "srVector3T<type-parameter-0-0>",
        "parameter_types": [
            "const srVector3T<type-parameter-0-0> &",
            "const srVector3T<type-parameter-0-0> &",
        ],
        "source_file": "include/surrender/srMath.h",
        "line": 10,
        "end_line": 10,
    }
    template_specialization = {
        **_declaration("operator+", is_definition=True),
        "semantic_id": "??$?HM@@YA?AV?$srVector3T@M@@ABV0@0@Z",
        "return_type": "class srVector3T<float>",
        "parameter_types": [
            "const class srVector3T<float> &",
            "const class srVector3T<float> &",
        ],
        "source_file": "include/surrender/srMath.h",
        "line": 10,
        "end_line": 10,
    }
    marker = {
        "address": 0x0047D2A0,
        "marker_kind": "FUNCTION",
        "source_file": "src/srext_unzip/string_ops.cpp",
        "line": 1,
        "marker_name": None,
        "folded": False,
        "target": "SREXT_UNZIP",
        "declaration_key": ["SREXT_UNZIP", "??H@YA?AUsrInlineString@@ABU0@0@Z", None],
    }
    repository = _repository(tmp_path, [marker], [owned, template_primary, template_specialization])
    (tmp_path / "src/srext_unzip").mkdir(parents=True)
    (tmp_path / "src/srext_unzip/string_ops.cpp").write_text(
        "// FUNCTION: SREXT_UNZIP 0x0047D2A0\n"
        "srInlineString operator+(const srInlineString& left, "
        "const srInlineString& right) { return left; }\n",
        encoding="utf-8",
    )
    (tmp_path / "include/surrender").mkdir(parents=True)
    (tmp_path / "include/surrender/srMath.h").write_text(
        "template <class T>\n"
        "srVector3T<T> operator+(const srVector3T<T>& first, "
        "const srVector3T<T>& second) { return first; }\n",
        encoding="utf-8",
    )

    assert identity_violations(repository) == []


def test_inline_overload_definition_is_not_a_consumer_redeclaration(tmp_path: Path) -> None:
    """An address-owned overload must not collide with sibling inline overloads.

    Reproduces the srHashValue case: SURRENDER 0x10027BF0 owns
    ``srHashValue(const TextureSetKey&)`` while ``srHashValue(unsigned long)``
    and ``srHashValue(unsigned short)`` are distinct inline overloads.
    """

    owned = {
        **_declaration("srHashValue", is_definition=True),
        "semantic_id": "?srHashValue@@YAIABUTextureSetKey@Renderer@srGERD@@@Z",
        "return_type": "unsigned int",
        "parameter_types": [
            "const struct srGERD::Renderer::TextureSetKey &",
        ],
        "source_file": "include/surrender/srGERD.h",
        "line": 2,
        "end_line": 2,
        "target": "SURRENDER",
    }
    sibling = {
        **_declaration("srHashValue", is_definition=True),
        "semantic_id": "?srHashValue@@YAIK@Z",
        "return_type": "unsigned int",
        "parameter_types": ["unsigned long"],
        "source_file": "include/surrender/srHash.h",
        "line": 1,
        "end_line": 1,
    }
    marker = {
        "address": 0x10027BF0,
        "marker_kind": "FUNCTION",
        "source_file": "include/surrender/srGERD.h",
        "line": 1,
        "marker_name": None,
        "folded": False,
        "target": "SURRENDER",
        "declaration_key": [
            "SURRENDER",
            "?srHashValue@@YAIABUTextureSetKey@Renderer@srGERD@@@Z",
            None,
        ],
    }
    repository = _repository(tmp_path, [marker], [owned, sibling])
    (tmp_path / "include/surrender").mkdir(parents=True)
    (tmp_path / "include/surrender/srGERD.h").write_text(
        "// FUNCTION: SURRENDER 0x10027BF0\n"
        "inline unsigned int srHashValue(const struct srGERD::Renderer::TextureSetKey& key) "
        "{ return 0; }\n",
        encoding="utf-8",
    )
    (tmp_path / "include/surrender/srHash.h").write_text(
        "inline unsigned int srHashValue(unsigned long key) { return key >> 16; }\n",
        encoding="utf-8",
    )

    assert identity_violations(repository) == []


def test_inconsistent_ordinary_consumer_prototype_is_still_reported(tmp_path: Path) -> None:
    """A non-template free function redeclared with the wrong prototype still fails."""

    owned = {
        **_declaration("TryFinishNpcVoicePlayback", is_definition=True),
        "return_type": "void",
        "parameter_types": ["unsigned char"],
        "semantic_id": "?TryFinishNpcVoicePlayback@@YAXE@Z",
        "source_file": "src/srext_unzip/voice.cpp",
        "line": 2,
        "end_line": 2,
        "target": "SREXT_UNZIP",
    }
    consumer = {
        **_declaration("TryFinishNpcVoicePlayback", is_definition=False),
        "return_type": "void",
        "parameter_types": ["char"],
        "semantic_id": "?TryFinishNpcVoicePlayback@@YAXD@Z",
        "source_file": "src/srext_unzip/voice.h",
        "line": 1,
        "end_line": 1,
    }
    marker = {
        "address": 0x00525D90,
        "marker_kind": "FUNCTION",
        "source_file": "src/srext_unzip/voice.cpp",
        "line": 1,
        "marker_name": None,
        "folded": False,
        "target": "SREXT_UNZIP",
        "declaration_key": ["SREXT_UNZIP", "?TryFinishNpcVoicePlayback@@YAXE@Z", None],
    }
    repository = _repository(tmp_path, [marker], [owned, consumer])
    (tmp_path / "src/srext_unzip").mkdir(parents=True)
    (tmp_path / "src/srext_unzip/voice.cpp").write_text(
        "// FUNCTION: SREXT_UNZIP 0x00525D90\n"
        "void TryFinishNpcVoicePlayback(unsigned char force) {}\n",
        encoding="utf-8",
    )
    (tmp_path / "src/srext_unzip/voice.h").write_text(
        "void TryFinishNpcVoicePlayback(char force);\n",
        encoding="utf-8",
    )

    (violation,) = identity_violations(repository)
    assert violation["kind"] == "consumer-prototype"
    assert violation["reason"] == "redeclared prototype"
    assert violation["names"] == ["TryFinishNpcVoicePlayback"]


def test_abi_prototype_waiver_allows_proven_consumer_spelling(tmp_path: Path) -> None:
    owned = {
        **_declaration("TryFinishNpcVoicePlayback", is_definition=True),
        "return_type": "void",
        "parameter_types": ["unsigned char"],
        "semantic_id": "?TryFinishNpcVoicePlayback@@YAXE@Z",
        "source_file": "src/srext_unzip/voice.cpp",
        "line": 2,
        "end_line": 2,
        "target": "SREXT_UNZIP",
    }
    consumer = {
        **_declaration("TryFinishNpcVoicePlayback", is_definition=False),
        "return_type": "void",
        "parameter_types": ["char"],
        "semantic_id": "?TryFinishNpcVoicePlayback@@YAXD@Z",
        "source_file": "src/srext_unzip/voice.h",
        "line": 2,
        "end_line": 2,
    }
    marker = {
        "address": 0x00525D90,
        "marker_kind": "FUNCTION",
        "source_file": "src/srext_unzip/voice.cpp",
        "line": 1,
        "marker_name": None,
        "folded": False,
        "target": "SREXT_UNZIP",
        "declaration_key": ["SREXT_UNZIP", "?TryFinishNpcVoicePlayback@@YAXE@Z", None],
    }
    repository = _repository(tmp_path, [marker], [owned, consumer])
    (tmp_path / "src/srext_unzip").mkdir(parents=True)
    (tmp_path / "src/srext_unzip/voice.cpp").write_text(
        "// FUNCTION: SREXT_UNZIP 0x00525D90\n"
        "void TryFinishNpcVoicePlayback(unsigned char force) {}\n",
        encoding="utf-8",
    )
    (tmp_path / "src/srext_unzip/voice.h").write_text(
        "// abi-prototype-ok: provider and consumer deliberately differ\n"
        "void TryFinishNpcVoicePlayback(char force);\n",
        encoding="utf-8",
    )

    assert identity_violations(repository) == []


def test_same_address_prototype_disagreement_is_reported(tmp_path: Path) -> None:
    first = {
        **_declaration("TryFinishNpcVoicePlayback", is_definition=False),
        "return_type": "void",
        "parameter_types": ["unsigned char"],
        "semantic_id": "?TryFinishNpcVoicePlayback@@YAXE@Z",
        "line": 1,
        "end_line": 1,
    }
    second = {
        **_declaration("TryFinishNpcVoicePlayback", is_definition=False),
        "return_type": "void",
        "parameter_types": ["char"],
        "semantic_id": "?TryFinishNpcVoicePlayback@@YAXD@Z",
        "source_file": "src/srext_unzip/other.h",
        "line": 1,
        "end_line": 1,
    }
    repository = _repository(tmp_path, [], [first, second])
    (tmp_path / "src/srext_unzip").mkdir(parents=True)
    (tmp_path / "src/srext_unzip/test.cpp").write_text(
        "void TryFinishNpcVoicePlayback(unsigned char force); /* 0x00525D90 */\n",
        encoding="utf-8",
    )
    (tmp_path / "src/srext_unzip/other.h").write_text(
        "void TryFinishNpcVoicePlayback(char force); /* 0x00525D90 */\n",
        encoding="utf-8",
    )

    (violation,) = identity_violations(repository)
    assert violation["reason"] == "multiple prototypes"
    assert violation["names"] == ["TryFinishNpcVoicePlayback"]
