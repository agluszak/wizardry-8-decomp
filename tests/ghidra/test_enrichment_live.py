"""Live-Ghidra enrichment checks.

These open the checkout-owned wiz8 program and must run only after
``wiz8 ghidra restore`` (see CI). They are excluded from ``wiz8 check``.
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


def test_w8monster_binding_uses_native_class_structure(wiz8_program: Any) -> None:
    binding = resolve_class_binding(wiz8_program, "W8Monster")
    if binding["status"] in {"missing-structure", "missing-class"}:
        pytest.skip("no bound W8Monster Structure in live program")
    path = str(binding["structure_path"] or "")
    assert not path.startswith("/wiz8/classes/"), path
    assert binding["ghidra_class"].endswith("W8Monster")


def test_callback_plan_does_not_target_legacy_paths(wiz8_program: Any) -> None:
    from wiz8decomp.callback_typing import collect_callback_typing_plan
    from wiz8decomp.datatype_contracts import is_legacy_path

    plan = collect_callback_typing_plan(wiz8_program)
    for row in plan["fields"]:
        path = row.get("structure")
        if path:
            assert not is_legacy_path(str(path)), row
        owner = row.get("owner")
        if owner:
            assert not is_legacy_path(str(owner)), row


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
    with (
        pytest.raises(RuntimeError, match="abort-test-transaction"),
        pyghidra.transaction(program, "test class binding"),
    ):
        result = apply_this_typing(program, plan, allow_custom_storage=False)
        assert candidate.hasCustomVariableStorage() is False
        # Applied, skipped (gated), or errored — never custom storage.
        assert result["applied"] + len(result["skipped"]) + len(result["errors"]) == 1
        raise RuntimeError("abort-test-transaction")


def test_source_sync_does_not_create_independent_retail_signature(wiz8_program: Any) -> None:
    """A deliberately wrong recovered Grow prototype cannot confirm its PDB twin."""
    from types import SimpleNamespace

    import pyghidra
    from reccmp.ghidra.signature_provenance import independently_reviewed_signature
    from wiz8decomp.ghidra.sync import _apply_signature

    program = wiz8_program
    address = program.getAddressFactory().getDefaultAddressSpace().getAddress(0x004ADDF0)
    function = program.getFunctionManager().getFunctionAt(address)
    assert function is not None
    before = str(function.getSignature())
    wrong_source = SimpleNamespace(
        return_type="bool",
        parameter_types=(),
        has_this=True,
        calling_convention="__thiscall",
        is_variadic=False,
    )
    with (
        pytest.raises(RuntimeError, match="abort-provenance-test"),
        pyghidra.transaction(program, "test source signature provenance"),
    ):
        result = _apply_signature(program, function, wrong_source)
        assert result == {"applied": True}
        assert str(function.getReturnType().getName()) == "bool"
        assert not independently_reviewed_signature(program, function)
        raise RuntimeError("abort-provenance-test")
    assert str(program.getFunctionManager().getFunctionAt(address).getSignature()) == before
