import csv
import re
from pathlib import Path

import yaml


def test_wiz8_executable_target_uses_real_platform_and_reccmp_surfaces() -> None:
    repository = Path(__file__).resolve().parents[2]
    cmake = (repository / "CMakeLists.txt").read_text(encoding="utf-8")
    project = yaml.safe_load((repository / "reccmp-project.yml").read_text(encoding="utf-8"))

    assert "add_custom_target(WIZ8_MATCHING DEPENDS WIZ8_GAMEPLAY_BOUNDARIES)" in cmake
    assert "add_executable(WIZ8_BRINGUP WIN32" in cmake
    assert "/FORCE:UNRESOLVED" in cmake
    assert "ddraw.lib" in cmake
    assert "gdi32.lib" in cmake
    assert "user32.lib" in cmake

    target = project["targets"]["WIZ8"]
    assert target == {
        "filename": "Wiz8.exe",
        "source-root": "src/wiz8",
        "hash": {"sha256": "18a74ff61c65b8a2d4cfa11ffce82ad7fef022a94eaf0c2f217e479e981420d2"},
    }

    windows_header = (repository / "src/wiz8/wiz8_windows.h").read_text(encoding="utf-8")
    assert "#include <windows.h>" in windows_header
    assert "#include <ddraw.h>" in windows_header
    assert "#define DIRECTDRAW_VERSION 0x0700" in windows_header


def test_reviewed_vc6_runtime_functions_are_library_annotations() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/wiz8/functions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        expected = {
            int(row["address"], 16)
            for row in csv.DictReader(stream)
            if row["owner"] == "msvc6-runtime"
        }

    source = (repository / "src/wiz8/vc6_runtime.cpp").read_text(encoding="utf-8")
    actual = {
        int(address, 16) for address in re.findall(r"// LIBRARY: WIZ8 0x([0-9A-Fa-f]+)", source)
    }
    assert actual == expected
    assert "// FUNCTION:" not in source

    bringup = (repository / "src/wiz8/bringup/WinMain.cpp").read_text(encoding="utf-8")
    assert '#include "wiz8_windows.h"' in bringup
    assert "// FUNCTION:" not in bringup
