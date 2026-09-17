"""Live-Ghidra class-binding checks.

Marked ``integration`` so they can be selected explicitly. They skip when the
checkout-owned wiz8 program is unavailable (plain unit CI without restore).
"""

from __future__ import annotations

from typing import Any

import pytest

pytest.importorskip("pyghidra")

from wiz8decomp.class_binding import resolve_class_binding
from wiz8decomp.class_this_typing import apply_this_typing
from wiz8decomp.config import load_settings
from wiz8decomp.ghidra.env import open_program

pytestmark = pytest.mark.integration


@pytest.fixture(scope="module")
def wiz8_program():
    settings = load_settings()
    try:
        with open_program(settings, "wiz8") as program:
            yield program
    except Exception as exc:  # noqa: BLE001
        pytest.skip(f"live wiz8 Ghidra program unavailable: {exc}")


def test_mov_ecx_jmp_helper_removed() -> None:
    import wiz8decomp.function_attributes as fa

    assert not hasattr(fa, "_thiscall_mov_jmp_thunk_target")


def test_w8monster_binding_uses_native_class_structure(wiz8_program: Any) -> None:
    binding = resolve_class_binding(wiz8_program, "W8Monster")
    if binding["status"] == "missing-structure":
        pytest.skip("no bound W8Monster Structure in live program")
    path = str(binding["structure_path"] or "")
    assert not path.startswith("/wiz8/classes/"), path
    assert binding["ghidra_class"].endswith("W8Monster")


def test_bind_class_this_does_not_enable_custom_storage(wiz8_program: Any) -> None:
    import pyghidra

    program = wiz8_program
    binding = resolve_class_binding(program, "W8Monster")
    if binding["status"] != "bound":
        pytest.skip("W8Monster Structure not bound")

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
                "owning_class": "W8Monster",
                "ghidra_this": str(candidate.getParameters()[0].getDataType()),
                "action": "bind-class-this",
            }
        ]
    }
    with pyghidra.transaction(program, "test class binding"):
        result = apply_this_typing(program, plan, allow_custom_storage=False)
        assert candidate.hasCustomVariableStorage() is False
        # Applied, skipped (auto-this-unbound), or errored — never custom storage.
        assert result["applied"] + len(result["skipped"]) + len(result["errors"]) == 1
        raise RuntimeError("abort-test-transaction")
