"""Focused tests for the generated runtime stub mechanism.

The parser-level tests cannot prove the linker mechanism: VC6's LINK has no
``/alternatename``, so the generated COFF thunk object is what must actually
satisfy cdecl, stdcall and C++ member symbols. The integration test compiles a
tiny fixture with the real toolchain, links it without ``/FORCE``, and checks
that each missing symbol reaches its trap.
"""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp.runtime_stubs import (
    COMPARISON_LINK_DIR,
    COMPARISON_RESPONSE,
    ResolvedStub,
    linked_objects,
    render_alias_object,
    render_source,
)

VC6_IMAGE = "wizardry8-msvc600:sp5"
HAVE_TOOLCHAIN = shutil.which("docker") is not None and shutil.which("wine") is not None


def _stubs() -> list[ResolvedStub]:
    return [
        ResolvedStub(
            symbol="_missing_cdecl",
            name="missing_cdecl",
            address=0x00401000,
            stub="probe_stub_cdecl",
            requesters=("callers.obj",),
            identity="address-name",
            reason="",
        ),
        ResolvedStub(
            symbol="_missing_stdcall@4",
            name="missing_stdcall",
            address=None,
            stub="probe_stub_stdcall",
            requesters=("callers.obj",),
            identity="unmapped",
            reason="no retail address evidence",
        ),
        ResolvedStub(
            symbol="?method@Thing@@QAEHH@Z",
            name="Thing::method",
            address=0x00401010,
            stub="probe_stub_thiscall",
            requesters=("callers.obj",),
            identity="address-name",
            reason="",
        ),
    ]


def test_linked_objects_resolves_tokens_from_the_component_binary_dir(tmp_path: Path) -> None:
    build = tmp_path / "build" / "decomp"
    response = build / COMPARISON_RESPONSE
    response.parent.mkdir(parents=True)
    obj = build / COMPARISON_LINK_DIR / "CMakeFiles" / "unit.dir" / "one.cpp.obj"
    obj.parent.mkdir(parents=True)
    obj.write_bytes(b"object")
    response.write_text(
        "CMakeFiles/unit.dir/one.cpp.obj CMakeFiles/unit.dir/absent.cpp.obj",
        encoding="utf-8",
    )

    objects = linked_objects(SimpleNamespace(product_build_dir=build))

    assert objects == [
        obj.resolve(),
        (build / COMPARISON_LINK_DIR / "CMakeFiles" / "unit.dir" / "absent.cpp.obj").resolve(),
    ]


def test_generated_source_uses_stub_markers_and_never_function() -> None:
    source = render_source(_stubs())

    assert "// STUB: WIZ8 0x00401000" in source
    assert "// STUB: WIZ8 0x00401010" in source
    assert "// FUNCTION:" not in source
    assert "WIZ8_RUNTIME_STUB" not in source  # the trap prints that, not the table


@pytest.mark.skipif(not HAVE_TOOLCHAIN, reason="docker and wine are required")
def test_generated_coff_thunks_link_and_trap(tmp_path: Path) -> None:
    probe = tmp_path / "probe"
    probe.mkdir()
    (probe / "callers.cpp").write_text(
        'extern "C" int missing_cdecl(int);\n'
        'extern "C" int __stdcall missing_stdcall(int);\n'
        "struct Thing { int method(int); };\n"
        "static int call_one(int which, Thing* thing)\n"
        "{\n"
        "    switch (which) {\n"
        "    case 0: return missing_cdecl(1);\n"
        "    case 1: return missing_stdcall(2);\n"
        "    default: return thing->method(3);\n"
        "    }\n"
        "}\n"
        "int main(int argc, char** argv)\n"
        "{\n"
        "    int which = argc > 1 ? argv[1][0] - '0' : 0;\n"
        "    return call_one(which, 0);\n"
        "}\n",
        encoding="utf-8",
    )
    (probe / "support.cpp").write_text(
        "#include <stdio.h>\n"
        "#include <windows.h>\n"
        'extern "C" void probe_stub_cdecl(void) { printf("TRAP cdecl\\n"); fflush(stdout); ExitProcess(10); }\n'
        'extern "C" void probe_stub_stdcall(void) { printf("TRAP stdcall\\n"); fflush(stdout); ExitProcess(11); }\n'
        'extern "C" void probe_stub_thiscall(void) { printf("TRAP thiscall\\n"); fflush(stdout); ExitProcess(12); }\n',
        encoding="utf-8",
    )
    (probe / "runtime_stubs.obj").write_bytes(
        render_alias_object(_stubs(), include_crt_support=False)
    )

    # The fixture directory is bind-mounted directly; Docker's daemon sees host
    # paths, so a pytest temporary directory works without touching build/.
    mount = f"{probe}:/probe"
    compile_result = subprocess.run(
        [
            "docker",
            "run",
            "--rm",
            "--init",
            "--network",
            "none",
            "--volume",
            mount,
            VC6_IMAGE,
            r"C:\msvc\VC98\Bin\CL.EXE",
            "/nologo",
            r"Z:\probe\callers.cpp",
            r"Z:\probe\support.cpp",
            r"Z:\probe\runtime_stubs.obj",
            "/FeZ:\\probe\\probe.exe",
        ],
        capture_output=True,
        text=True,
        check=False,
        timeout=600,
    )
    assert compile_result.returncode == 0, compile_result.stdout + compile_result.stderr
    assert "/FORCE" not in compile_result.stdout + compile_result.stderr
    executable = probe / "probe.exe"
    assert executable.is_file()

    expected = {"0": (10, "TRAP cdecl"), "1": (11, "TRAP stdcall"), "2": (12, "TRAP thiscall")}
    for argument, (code, line) in expected.items():
        run = subprocess.run(
            ["wine", str(executable), argument],
            capture_output=True,
            text=True,
            check=False,
            timeout=120,
        )
        assert run.returncode == code, run.stdout + run.stderr
        assert line in run.stdout


def test_compiled_probe_has_no_forced_unresolved_reference() -> None:
    # The COFF object defines the three decorated spellings plus, when asked,
    # the CRT absolute. This keeps the mechanism's exact input format pinned.
    import struct

    data = render_alias_object(_stubs(), include_crt_support=False)
    machine, sections, _timestamp, table, symbol_count, optional, _flags = struct.unpack_from(
        "<HHIIIHH", data
    )
    assert (machine, sections, optional) == (0x14C, 1, 0)
    strings_at = table + symbol_count * 18
    length = struct.unpack_from("<I", data, strings_at)[0] if symbol_count else 0
    names = []
    for index in range(symbol_count):
        raw = data[table + index * 18 : table + index * 18 + 8]
        if raw[:4] == b"\0" * 4:
            offset = struct.unpack_from("<I", raw, 4)[0]
            end = data.index(b"\0", strings_at + offset)
            names.append(data[strings_at + offset : end].decode())
        else:
            names.append(raw.rstrip(b"\0").decode())
    assert "_missing_cdecl" in names
    assert "_missing_stdcall@4" in names
    assert "?method@Thing@@QAEHH@Z" in names
    assert "__except_list" not in names
    assert length > 4
