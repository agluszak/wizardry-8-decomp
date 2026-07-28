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
    assert "add_executable(WIZ8_RUNTIME WIN32" in cmake
    assert "/FORCE:UNRESOLVED" in cmake
    assert "add_custom_target(WIZ8_RUNNABLE DEPENDS WIZ8_RUNTIME)" in cmake
    runtime_block = re.search(
        r"target_link_options\(WIZ8_RUNTIME PRIVATE(.*?)\)", cmake, re.DOTALL
    )
    assert runtime_block is not None
    assert "/OPT:REF" in runtime_block.group(1)
    assert "/OPT:NOREF" not in runtime_block.group(1)
    runtime_sources = re.findall(
        r"target_sources\(WIZ8_RUNTIME PRIVATE(.*?)\)", cmake, re.DOTALL
    )
    assert runtime_sources
    runtime_source_text = "\n".join(runtime_sources)
    assert "WIZ8_SGP_RUNTIME_CORE" in runtime_source_text
    assert "WIZ8_SGP_RUNTIME_SHARED" in runtime_source_text
    assert "WIZ8_SGP_RETAINED" in runtime_source_text
    assert "WinMain=SgpRetainedWinMain" in cmake
    assert "ddraw.lib" in cmake
    assert "gdi32.lib" in cmake
    assert "user32.lib" in cmake
    justfile = (repository / "Justfile").read_text(encoding="utf-8")
    assert "C:\\jom\\jom.exe -j {{jobs}}" in justfile
    build_recipe = justfile.split("# Build and run the recovered", 1)[0]
    assert 'default_build_target := "WIZ8_RUNTIME"' in justfile
    assert "build target=default_build_target jobs=num_cpus(): _check-build-dir" in build_recipe
    assert "if test ! -f" in build_recipe
    assert "fid fetch-sources" not in build_recipe
    assert "reccmp-project detect" not in build_recipe
    assert "run: build" in justfile
    assert '$WIZ8_WORK_DIR/variants/gog-base' in justfile
    assert '$WIZ8_WORK_DIR/wine/wiz8-runtime' in justfile
    assert "config/runtime/Wiz8.CFG.hex" in justfile
    assert "wine explorer /desktop=Wizardry8,640x480 &" in justfile
    assert "wine ./Wiz8Runtime.exe" in justfile
    run_recipe = justfile.split("run: build", 1)[1].split(
        "# Refuse a build directory", 1
    )[0]
    assert "pkill" not in run_recipe
    assert "/proc/[0-9]*" not in run_recipe
    assert 'wineserver -k' in run_recipe
    # The matching target sees the recovered headers and the vendored SGP tree,
    # and nothing else. SGP is on the path because its structures are library
    # layout that the evidence policy says to take from the real header rather
    # than restate - GETFILESTRUCT, which LoadSaveGame.cpp walks the save
    # directory through, is the first that mattered. The overlay supplies the
    # empty builddefines.h that tree expects.
    include_block = re.search(
        r"target_include_directories\(WIZ8_GAMEPLAY_BOUNDARIES PRIVATE(.*?)\)",
        cmake,
        re.DOTALL,
    )
    assert include_block is not None
    includes = include_block.group(1).split()
    assert includes == ["include", "config/sgp-overlays/common", '"${SGP_SOURCE}"']
    assert "target_include_directories(WIZ8_BRINGUP PRIVATE include)" in cmake
    assert not list((repository / "src/wiz8").glob("*.h"))

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
    menu = (repository / "src/wiz8/local_screens/MainMenuScreen.cpp").read_text(
        encoding="utf-8"
    )
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
