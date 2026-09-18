"""Tests for Parameter ID skip policy."""

from __future__ import annotations

from types import SimpleNamespace

from wiz8decomp.parameter_id import collect_parameter_id_plan


def test_parameter_id_skips_recovered_and_protected_signatures(tmp_path, monkeypatch) -> None:
    recovered = SimpleNamespace(
        getEntryPoint=lambda: SimpleNamespace(getOffset=lambda: 0x401000),
        getName=lambda _qualified=True: "RecoveredFn",
        getSignatureSource=lambda: SimpleNamespace(name="DEFAULT"),
        isThunk=lambda: False,
        isExternal=lambda: False,
    )
    protected = SimpleNamespace(
        getEntryPoint=lambda: SimpleNamespace(getOffset=lambda: 0x402000),
        getName=lambda _qualified=True: "ImportedFn",
        getSignatureSource=lambda: SimpleNamespace(name="USER_DEFINED"),
        isThunk=lambda: False,
        isExternal=lambda: False,
    )
    target = SimpleNamespace(
        getEntryPoint=lambda: SimpleNamespace(getOffset=lambda: 0x403000),
        getName=lambda _qualified=True: "UnrecoveredFn",
        getSignatureSource=lambda: SimpleNamespace(name="DEFAULT"),
        isThunk=lambda: False,
        isExternal=lambda: False,
    )

    class _Space:
        def getAddress(self, value: int):
            return value

    functions = {0x401000: recovered, 0x402000: protected, 0x403000: target}
    program = SimpleNamespace(
        getAddressFactory=lambda: SimpleNamespace(getDefaultAddressSpace=lambda: _Space()),
        getFunctionManager=lambda: SimpleNamespace(getFunctionAt=lambda addr: functions.get(addr)),
    )
    monkeypatch.setattr(
        "wiz8decomp.parameter_id.source_functions",
        lambda _repo, _target: {
            0x401000: SimpleNamespace(marker_kind="FUNCTION"),
        },
    )
    plan = collect_parameter_id_plan(
        tmp_path,
        program,
        addresses=[0x401000, 0x402000, 0x403000],
    )
    assert plan["counts"]["skip-recovered"] == 1
    assert plan["counts"]["skip-protected-signature"] == 1
    assert plan["counts"]["commit-params"] == 1
    assert plan["functions"] == [
        {
            "address": "0x00403000",
            "name": "UnrecoveredFn",
            "signature_source": "DEFAULT",
            "action": "commit-params",
        }
    ]
