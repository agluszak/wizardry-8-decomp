from __future__ import annotations

import json
from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp.comparison import header_dependent_files, selected_addresses
from wiz8decomp.config import Settings


def _index(repository: Path, names: list[tuple[int, str]]) -> None:
    (repository / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
    )
    (repository / "build").mkdir()
    (repository / "build/source-index.json").write_text(
        json.dumps(
            {
                "schema": "reccmp-source-index-v2",
                "classes": [],
                "declarations": [],
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
    scan = {
        "translation-units": [
            {
                "commands": [
                    {
                        "input-file": "/repo/src/wiz8/unit.cpp",
                        "file-deps": [
                            "/repo/src/wiz8/unit.cpp",
                            "/repo/include/wiz8/outer.h",
                            "/repo/include/wiz8/shared.h",
                            "/repo/include/wiz8/inline.h",
                        ],
                    }
                ]
            },
            {
                "commands": [
                    {
                        "input-file": str(tmp_path / "src/wiz8/other.cpp"),
                        "file-deps": [str(tmp_path / "include/wiz8/outer.h")],
                    }
                ]
            },
            {
                "commands": [
                    {
                        "input-file": "/repo/src/another_target/unit.cpp",
                        "file-deps": ["/repo/include/wiz8/shared.h", "/repo/src/wiz8/other.cpp"],
                    }
                ]
            },
        ]
    }
    monkeypatch.setattr("wiz8decomp.build.configure_clang", lambda _: (tmp_path / "build", []))
    monkeypatch.setattr(
        "wiz8decomp.comparison.run", lambda *a, **kw: SimpleNamespace(stdout=json.dumps(scan))
    )
    settings = Settings.model_construct(repo_dir=tmp_path)
    dependents = header_dependent_files(settings, "WIZ8", [tmp_path / "include/wiz8/shared.h"])
    assert set(dependents) == {tmp_path / "src/wiz8/unit.cpp", tmp_path / "include/wiz8/inline.h"}
    assert selected_addresses(tmp_path, "WIZ8", [], dependents) == [0x401000, 0x401020]


def test_cpp_only_change_needs_no_header_dependency_scan(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    def unexpected(*args, **kwargs):
        pytest.fail("a cpp-only change must not run a dependency scan")

    monkeypatch.setattr("wiz8decomp.build.configure_clang", unexpected)
    assert (
        header_dependent_files(
            Settings.model_construct(repo_dir=tmp_path), "WIZ8", [tmp_path / "unit.cpp"]
        )
        == []
    )
