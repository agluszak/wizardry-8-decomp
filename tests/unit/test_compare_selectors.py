from __future__ import annotations

import json
from pathlib import Path

import pytest
from wiz8decomp.reccmp_workflows import selected_addresses


def _index(repository: Path, names: list[tuple[int, str]]) -> None:
    (repository / "build").mkdir()
    (repository / "build/source-index.json").write_text(
        json.dumps(
            {
                "schema": "reccmp-source-index-v1",
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
    assert selected_addresses(tmp_path, ["Thing::Run"], []) == [0x401000]
    assert selected_addresses(tmp_path, ["0x401000:0x401020"], []) == [0x401000, 0x401020]


def test_ambiguous_selector_lists_candidates(tmp_path: Path) -> None:
    _index(tmp_path, [(0x401000, "Run"), (0x402000, "Run")])
    with pytest.raises(ValueError, match=r"ambiguous.*0x00401000.*0x00402000"):
        selected_addresses(tmp_path, ["Run"], [])
