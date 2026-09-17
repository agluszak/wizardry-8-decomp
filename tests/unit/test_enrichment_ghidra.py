"""Live-Ghidra class-binding checks.

Marked ``integration`` so they can be selected explicitly. They prefer the
imported lifecycle fixture project (Base/Derived/Secondary) when available, and
otherwise fall back to the checkout-owned wiz8 program. Skip cleanly when
neither is available.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

import pytest

pytest.importorskip("pyghidra")

from wiz8decomp.class_binding import resolve_class_binding
from wiz8decomp.class_this_typing import apply_this_typing
from wiz8decomp.config import load_settings
from wiz8decomp.ghidra.env import open_program

pytestmark = pytest.mark.integration

_LIFECYCLE_CLASSES = ("Base", "Derived", "Secondary")


def _lifecycle_fixture_project(settings) -> Path | None:
    """Locate a previously built lifecycle fixture Ghidra project, if any."""

    candidates = [
        settings.build_dir / "recovery-fixture" / "vc6-sp5" / "project",
        settings.build_dir / "lifecycle-fixture" / "project",
    ]
    for path in candidates:
        if path.is_dir() and any(path.glob("*.gpr")):
            return path
    return None


@pytest.fixture(scope="module")
def binding_program():
    settings = load_settings()
    fixture = _lifecycle_fixture_project(settings)
    if fixture is not None:
        try:
            import pyghidra
            from wiz8decomp.ghidra.env import start_pyghidra

            start_pyghidra(settings)
            # Prefer opening the disposable lifecycle fixture rather than live wiz8.
            for gpr in fixture.glob("*.gpr"):
                name = gpr.stem
                with (
                    pyghidra.open_project(fixture, name) as project,
                    pyghidra.program_context(
                        project,
                        "/"
                        + next(iter(project.getProjectData().getRootFolder().getFiles())).getName(),
                    ) as program,
                ):
                    yield program
                    return
        except Exception as exc:  # noqa: BLE001
            pytest.skip(f"lifecycle fixture project unusable: {exc}")

    try:
        with open_program(settings, "wiz8") as program:
            yield program
    except Exception as exc:  # noqa: BLE001
        pytest.skip(f"live wiz8 Ghidra program unavailable: {exc}")


def test_mov_ecx_jmp_helper_removed() -> None:
    import wiz8decomp.function_attributes as fa

    assert not hasattr(fa, "_thiscall_mov_jmp_thunk_target")


def test_lifecycle_or_w8monster_binding_uses_native_class_structure(binding_program: Any) -> None:
    for name in (*_LIFECYCLE_CLASSES, "W8Monster"):
        binding = resolve_class_binding(binding_program, name)
        if binding["status"] in {"missing-structure", "missing-class"}:
            continue
        path = str(binding["structure_path"] or "")
        assert not path.startswith("/wiz8/classes/"), path
        assert binding["ghidra_class"] and name in str(binding["ghidra_class"])
        return
    pytest.skip("no bound Base/Derived/Secondary/W8Monster Structure available")


def test_bind_class_this_does_not_enable_custom_storage(binding_program: Any) -> None:
    import pyghidra

    program = binding_program
    owning = None
    for name in (*_LIFECYCLE_CLASSES, "W8Monster"):
        binding = resolve_class_binding(program, name)
        if binding["status"] == "bound":
            owning = name
            break
    if owning is None:
        pytest.skip("no bound lifecycle/W8Monster Structure")

    functions = program.getFunctionManager()
    candidate = None
    for function in functions.getFunctions(True):
        if function.getCallingConventionName() != "__thiscall":
            continue
        params = list(function.getParameters())
        if not params or not params[0].isAutoParameter():
            continue
        if function.hasCustomVariableStorage():
            continue
        candidate = function
        break
    if candidate is None:
        pytest.skip("no dynamic thiscall candidate")

    address = f"0x{int(candidate.getEntryPoint().getOffset()):08x}"
    plan = {
        "functions": [
            {
                "address": address,
                "name": candidate.getName(True),
                "owning_class": owning,
                "ghidra_this": str(candidate.getParameters()[0].getDataType()),
                "action": "bind-class-this",
            }
        ]
    }
    with (
        pytest.raises(RuntimeError, match="abort-test-transaction"),
        pyghidra.transaction(program, "test class binding"),
    ):
        result = apply_this_typing(program, plan, allow_custom_storage=False)
        assert candidate.hasCustomVariableStorage() is False
        # Applied, skipped (auto-this-unbound), or errored — never custom storage.
        assert result["applied"] + len(result["skipped"]) + len(result["errors"]) == 1
        raise RuntimeError("abort-test-transaction")
