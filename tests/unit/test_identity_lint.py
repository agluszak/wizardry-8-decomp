import json
from pathlib import Path

from wiz8decomp.identity_lint import identity_violations, validate_identity


def _repository(tmp_path: Path, markers: list[dict]) -> Path:
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
                "declarations": [],
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
