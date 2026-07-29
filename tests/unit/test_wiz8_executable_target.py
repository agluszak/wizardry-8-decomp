from pathlib import Path

import yaml
from wiz8decomp.source_model import build_source_model


def test_wiz8_reccmp_target_and_platform_header_are_canonical() -> None:
    repository = Path(__file__).resolve().parents[2]
    project = yaml.safe_load((repository / "reccmp-project.yml").read_text(encoding="utf-8"))

    target = project["targets"]["WIZ8"]
    assert target == {
        "filename": "Wiz8.exe",
        "source-root": ["src/wiz8", "include/wiz8"],
        "data-sources": ["build/reccmp/wiz8-symbols.csv"],
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
    assert "SetPendingScreenState(10)" in menu
    assert "Function425B40();" in video
    assert "invalidateTiles" not in video
    assert "setTextureSubImage" in surface
    assert "// FUNCTION: WIZ8 0x00425B40" in dirty


def test_semantic_runtime_driver_is_a_separate_product() -> None:
    repository = Path(__file__).resolve().parents[2]
    product = (repository / "src/wiz8/CMakeLists.txt").read_text(encoding="utf-8")
    driver = (repository / "tests/runtime/wiz8_runtime_test.cpp").read_text(encoding="utf-8")

    assert "TARGET WIZ8_RUNTIME_TEST" in product
    assert "SOURCES tests/runtime/wiz8_runtime_test.cpp" in product
    assert "TARGET WIZ8_BRINGUP" in product
    assert "TARGET WIZ8_RUNTIME\n" in product
    assert "add_library(wiz8_recovered_objects OBJECT" in product
    assert "WIZ8_GAMEPLAY_BOUNDARIES" not in product
    assert "WIZ8_MATCHING" not in product
    assert "QueueEvent(KEY_DOWN, KEY_END, 0)" in driver
    assert "QueueEvent(KEY_DOWN, ENTER, 0)" in driver
    assert "tests/runtime" not in (repository / "src/wiz8/sources.cmake").read_text(
        encoding="utf-8"
    )


def test_intro_video_owner_uses_canonical_bink_import_surface() -> None:
    repository = Path(__file__).resolve().parents[2]
    sdk = (repository / "include/bink.h").read_text(encoding="utf-8")
    header = (repository / "include/wiz8/bink_video.h").read_text(encoding="utf-8")
    source = (repository / "src/wiz8/engine_code/Bink.cpp").read_text(encoding="utf-8")
    imports = (repository / "src/wiz8/imports/binkw32.def").read_text(encoding="utf-8")
    product = (repository / "src/wiz8/CMakeLists.txt").read_text(encoding="utf-8")

    assert "sizeof(W8BinkVideo) == 0x0c" in header
    assert '#include "bink.h"' in header
    assert "typedef BINK* HBINK" in sdk
    assert "BINKRECT FrameRects[BINKMAXDIRTYRECTS]" in sdk
    assert "S32 __stdcall BinkCopyToBuffer" in sdk
    assert "void __stdcall BinkSetVolume(HBINK bink, S32 volume)" in sdk
    assert "__declspec(dllimport)" not in source
    assert "struct W8BinkHandle" not in source
    assert "BinkSetSoundSystem(BinkOpenMiles" in source
    assert "m_target->Lock" in source
    assert "m_target->Restore" in source
    assert "BinkCopyToBuffer@28" in imports
    assert "BinkNextFrame@4" in imports
    assert "wiz8_add_import_library(WIZ8_BINKW32" in product


def test_library_calls_use_their_owned_interface_headers() -> None:
    repository = Path(__file__).resolve().parents[2]
    bringup = (repository / "src/wiz8/bringup_gates.cpp").read_text(encoding="utf-8")
    renderer = (repository / "src/wiz8/renderer_window.cpp").read_text(encoding="utf-8")
    gameplay_database = (repository / "src/wiz8/local_code/GameplayDatabase.cpp").read_text(
        encoding="utf-8"
    )
    directdraw_users = [
        renderer,
        (repository / "src/wiz8/dirty_tiles.cpp").read_text(encoding="utf-8"),
        (repository / "src/wiz8/video2.cpp").read_text(encoding="utf-8"),
        (repository / "src/wiz8/local_code/SurfaceFill.cpp").read_text(encoding="utf-8"),
    ]
    sr_api = (repository / "include/wiz8/sr_api.h").read_text(encoding="utf-8")

    assert '#include "Mss.h"' in bringup
    assert "int __stdcall AIL_" not in bringup
    assert '#include "FileMan.h"' in gameplay_database
    assert "int FileOpen(" not in gameplay_database
    assert all('#include "DirectDraw Calls.h"' in source for source in directdraw_users)
    assert all("void DDLockSurface(" not in source for source in directdraw_users)
    assert '#include "wiz8/sr_api.h"' in renderer
    assert "__declspec(dllimport) int __cdecl srInit(void);" in sr_api
    assert "__declspec(dllimport) void __cdecl srAssertSetFunc" in sr_api
    assert "__declspec(dllimport)" not in renderer


def test_reviewed_vc6_runtime_functions_are_library_annotations() -> None:
    repository = Path(__file__).resolve().parents[2]
    model = build_source_model(repository)
    expected = {address for address, item in model.functions.items() if item.kind == "LIBRARY"}
    source = (repository / "src/wiz8/vc6_runtime.cpp").read_text(encoding="utf-8")
    assert len(expected) == 16
    assert 0x00401000 in expected
    assert "// FUNCTION:" not in source

    # The bring-up entry was a marker-less link stub until WinMain itself was
    # recovered. It now carries its canonical address like any other body, and
    # the stub is gone rather than sitting alongside a real definition.
    assert not (repository / "src/wiz8/bringup/WinMain.cpp").exists()
    gates = (repository / "src/wiz8/bringup_gates.cpp").read_text(encoding="utf-8")
    assert '#include "wiz8/wiz8_windows.h"' in gates
    assert "// FUNCTION: WIZ8 0x00401670" in gates
