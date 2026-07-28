import csv
import re
from pathlib import Path

import yaml


def test_wiz8_reccmp_target_and_platform_header_are_canonical() -> None:
    repository = Path(__file__).resolve().parents[2]
    project = yaml.safe_load((repository / "reccmp-project.yml").read_text(encoding="utf-8"))

    target = project["targets"]["WIZ8"]
    assert target == {
        "filename": "Wiz8.exe",
        "source-root": "src/wiz8",
        "hash": {"sha256": "18a74ff61c65b8a2d4cfa11ffce82ad7fef022a94eaf0c2f217e479e981420d2"},
    }

    windows_header = (repository / "include/wiz8/wiz8_windows.h").read_text(encoding="utf-8")
    assert "#include <windows.h>" in windows_header
    assert "#include <ddraw.h>" in windows_header
    assert "#define DIRECTDRAW_VERSION 0x0700" in windows_header


def test_main_menu_runtime_uses_dirty_uploads_and_real_input_dispatch() -> None:
    repository = Path(__file__).resolve().parents[2]
    menu = (repository / "src/wiz8/local_screens/MainMenuScreen.cpp").read_text(encoding="utf-8")
    video = (repository / "src/wiz8/video2.cpp").read_text(encoding="utf-8")
    dirty = (repository / "src/wiz8/dirty_tiles.cpp").read_text(encoding="utf-8")
    surface = (repository / "src/wiz8/surface2d.cpp").read_text(encoding="utf-8")

    assert "DequeueEvent(&input)" in menu
    assert "ScreenToClient(ghWindow, &mouse)" in menu
    assert "RegionContainsPoint(region, x, y)" in menu
    assert "region->callback((const W8RegionEvent*)&input, region)" in menu
    assert "input.usParam == UPARROW" in menu
    assert "input.usParam == ENTER" in menu
    assert "Function55EC50(10)" in menu
    assert "Function425B40();" in video
    assert "invalidateTiles" not in video
    assert "setTextureSubImage" in surface
    assert "// FUNCTION: WIZ8 0x00425B40" in dirty


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

    # The bring-up entry was a marker-less link stub until WinMain itself was
    # recovered. It now carries its canonical address like any other body, and
    # the stub is gone rather than sitting alongside a real definition.
    assert not (repository / "src/wiz8/bringup/WinMain.cpp").exists()
    gates = (repository / "src/wiz8/bringup_gates.cpp").read_text(encoding="utf-8")
    assert '#include "wiz8/wiz8_windows.h"' in gates
    assert "// FUNCTION: WIZ8 0x00401670" in gates
