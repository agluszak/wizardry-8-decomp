from __future__ import annotations

import json
import os
from pathlib import Path

import pytest
from wiz8decomp.comparison import header_dependent_files, selected_addresses
from wiz8decomp.config import Settings
from wiz8decomp.source_index import warn_if_source_index_may_be_stale


def _index(repository: Path, names: list[tuple[int, str]]) -> None:
    (repository / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
    )
    (repository / "build").mkdir()
    (repository / "build/source-index.json").write_text(
        json.dumps(
            {
                "schema": "reccmp-source-index-v6",
                "classes": [],
                "declarations": [],
                "variables": [],
                "member_uses": [],
                "conflicts": [],
                "markers": [
                    {
                        "marker_kind": "FUNCTION",
                        "address": address,
                        "source_file": "src/wiz8/unit.cpp",
                        "line": index + 1,
                        "declaration": None,
                        "marker_name": name,
                        "target": "WIZ8",
                    }
                    for index, (address, name) in enumerate(names)
                ],
            }
        ),
        encoding="utf-8",
    )


def test_name_and_range_selectors_resolve_without_source_search(tmp_path: Path) -> None:
    _index(tmp_path, [(0x401000, "Thing::Run"), (0x401020, "Function401020")])
    assert selected_addresses(tmp_path, "WIZ8", ["Thing::Run"], []) == [0x401000]
    assert selected_addresses(tmp_path, "WIZ8", ["0x401000:0x401020"], []) == [
        0x401000,
        0x401020,
    ]


@pytest.mark.parametrize("selector", ["0x401000", "401000"])
def test_numeric_selector_does_not_load_source_functions(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, selector: str
) -> None:
    monkeypatch.setattr(
        "wiz8decomp.source_index.source_functions",
        lambda *_args: pytest.fail("numeric selection must not load the source model"),
    )
    assert selected_addresses(tmp_path, "WIZ8", [selector], []) == [0x401000]


def test_mixed_numeric_and_named_selectors_use_existing_index(tmp_path: Path) -> None:
    _index(tmp_path, [(0x401020, "Thing::Run")])
    assert selected_addresses(tmp_path, "WIZ8", ["0x401000", "Thing::Run"], []) == [
        0x401000,
        0x401020,
    ]


def test_ambiguous_selector_lists_candidates(tmp_path: Path) -> None:
    _index(tmp_path, [(0x401000, "Run"), (0x402000, "Run")])
    with pytest.raises(ValueError, match=r"ambiguous.*0x00401000.*0x00402000"):
        selected_addresses(tmp_path, "WIZ8", ["Run"], [])


def test_changed_header_selects_transitive_consumers_and_inline_bodies(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _index(tmp_path, [(0x401000, "Caller"), (0x401020, "Inline"), (0x401040, "Unrelated")])
    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    source-root: src/wiz8\n"
    )
    index_path = tmp_path / "build/source-index.json"
    index = json.loads(index_path.read_text())
    for marker, source in zip(
        index["markers"],
        ["src/wiz8/unit.cpp", "include/wiz8/inline.h", "src/wiz8/other.cpp"],
        strict=True,
    ):
        marker["source_file"] = source
    index_path.write_text(json.dumps(index))
    index["translation_unit_dependencies"] = [
        {
            "source_file": "src/wiz8/unit.cpp",
            "file_dependencies": [
                "src/wiz8/unit.cpp",
                "include/wiz8/outer.h",
                "include/wiz8/shared.h",
                "include/wiz8/inline.h",
            ],
        },
        {
            "source_file": "src/wiz8/markerless.cpp",
            "file_dependencies": ["include/wiz8/shared.h"],
        },
        {
            "source_file": "src/wiz8/other.cpp",
            "file_dependencies": ["include/wiz8/outer.h"],
        },
        {
            "source_file": "src/another_target/unit.cpp",
            "file_dependencies": ["include/wiz8/shared.h", "src/wiz8/other.cpp"],
        },
    ]
    index_path.write_text(json.dumps(index))
    settings = Settings.model_construct(repo_dir=tmp_path)
    dependents = header_dependent_files(settings, "WIZ8", [tmp_path / "include/wiz8/shared.h"])
    assert set(dependents) == {
        tmp_path / "src/wiz8/unit.cpp",
        tmp_path / "src/wiz8/markerless.cpp",
        tmp_path / "include/wiz8/inline.h",
    }
    assert selected_addresses(tmp_path, "WIZ8", [], dependents) == [0x401000, 0x401020]


def test_cpp_only_change_needs_no_header_dependency_scan(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    def unexpected(*args, **kwargs):
        pytest.fail("a cpp-only change must not run a dependency scan")

    monkeypatch.setattr("wiz8decomp.source_index.load_source_index", unexpected)
    assert (
        header_dependent_files(
            Settings.model_construct(repo_dir=tmp_path), "WIZ8", [tmp_path / "unit.cpp"]
        )
        == []
    )


@pytest.mark.parametrize("source_newer, warns", [(True, True), (False, False)])
def test_source_index_freshness_uses_input_mtimes(
    tmp_path: Path, caplog, source_newer: bool, warns: bool
) -> None:
    _index(tmp_path, [(0x401000, "Run")])
    project = tmp_path / "reccmp-project.yml"
    project.write_text("targets:\n  WIZ8:\n    filename: Wiz8.exe\n    source-root: src/wiz8\n")
    source = tmp_path / "src/wiz8/unit.cpp"
    source.parent.mkdir(parents=True)
    source.write_text("void f() {}")
    index = tmp_path / "build/source-index.json"
    old, new = 1_000_000_000, 2_000_000_000
    source_time, index_time = (new, old) if source_newer else (old, new)
    os.utime(source, ns=(source_time, source_time))
    os.utime(index, ns=(index_time, index_time))

    assert warn_if_source_index_may_be_stale(tmp_path, "WIZ8") is warns
    assert ("source index may be stale" in caplog.text) is warns
