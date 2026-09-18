"""Tests for class-binding helpers used by automatic ``this`` typing."""

from __future__ import annotations

import contextlib
import sys
import types
from typing import Any
from unittest.mock import MagicMock

from wiz8decomp.class_binding import _sanitize_class_parts, binding_agrees
from wiz8decomp.class_this_typing import apply_this_typing


def test_sanitize_class_parts_splits_namespaces() -> None:
    assert _sanitize_class_parts("W8Monster") == ((), "W8Monster")
    assert _sanitize_class_parts("ns::Inner::Type") == (("ns", "Inner"), "Type")


def test_binding_agrees_requires_matching_structure_path(monkeypatch) -> None:
    binding = {"status": "bound", "structure_path": "/W8Monster"}

    class _Pointee:
        def getPathName(self) -> str:
            return "/W8Monster"

    class _Fn:
        pass

    monkeypatch.setattr(
        "wiz8decomp.class_binding.auto_this_structure",
        lambda _fn: _Pointee(),
    )
    assert binding_agrees(binding, _Fn())

    monkeypatch.setattr(
        "wiz8decomp.class_binding.auto_this_structure",
        lambda _fn: type("P", (), {"getPathName": lambda self: "/wiz8/classes/W8Monster"})(),
    )
    assert not binding_agrees(binding, _Fn())


def test_mov_ecx_jmp_helper_removed() -> None:
    import wiz8decomp.function_attributes as fa

    assert not hasattr(fa, "_thiscall_mov_jmp_thunk_target")


def _stub_pyghidra_transaction(monkeypatch) -> None:
    module = sys.modules.get("pyghidra")
    if module is None:
        module = types.ModuleType("pyghidra")
        monkeypatch.setitem(sys.modules, "pyghidra", module)
    monkeypatch.setattr(
        module,
        "transaction",
        lambda *_a, **_k: contextlib.nullcontext(),
        raising=False,
    )


def test_this_typing_without_allow_custom_storage_does_not_enable_custom(
    monkeypatch,
) -> None:
    """Gated custom-storage refusal is a skip, not an apply error."""

    def fake_row(
        _program: Any,
        row: dict[str, Any],
        *,
        allow_custom_storage: bool = False,
    ) -> dict[str, Any]:
        assert allow_custom_storage is False
        return {
            **row,
            "error": "requires-custom-storage",
            "hint": "custom storage required; refused without allow_custom_storage",
        }

    _stub_pyghidra_transaction(monkeypatch)
    monkeypatch.setattr(
        "wiz8decomp.class_this_typing.apply_this_typing_row",
        fake_row,
    )

    plan = {
        "functions": [
            {
                "address": "0x00401000",
                "name": "W8Monster::Tick",
                "owning_class": "W8Monster",
                "ghidra_this": "undefined *",
                "action": "bind-class-this",
            }
        ]
    }
    result = apply_this_typing(MagicMock(), plan, allow_custom_storage=False)
    assert result["applied"] == 0
    assert result["errors"] == []
    assert len(result["skipped"]) == 1
    assert result["skipped"][0]["skipped"] == "requires-custom-storage"
    assert result["skipped"][0]["error"] == "requires-custom-storage"


def test_this_typing_auto_this_unbound_is_skipped(monkeypatch) -> None:
    def fake_row(
        _program: Any,
        row: dict[str, Any],
        *,
        allow_custom_storage: bool = False,
    ) -> dict[str, Any]:
        return {**row, "error": "auto-this-unbound", "structure_path": "/W8Monster"}

    _stub_pyghidra_transaction(monkeypatch)
    monkeypatch.setattr(
        "wiz8decomp.class_this_typing.apply_this_typing_row",
        fake_row,
    )

    plan = {
        "functions": [
            {
                "address": "0x00402000",
                "name": "W8Monster::Draw",
                "owning_class": "W8Monster",
                "action": "bind-class-this",
            }
        ]
    }
    result = apply_this_typing(MagicMock(), plan, allow_custom_storage=False)
    assert result["errors"] == []
    assert len(result["skipped"]) == 1
    assert result["skipped"][0]["skipped"] == "auto-this-unbound"


def test_this_typing_namespace_collision_is_skipped(monkeypatch) -> None:
    def fake_row(
        _program: Any,
        row: dict[str, Any],
        *,
        allow_custom_storage: bool = False,
    ) -> dict[str, Any]:
        return {**row, "error": "namespace-collision", "hint": "plain namespace"}

    _stub_pyghidra_transaction(monkeypatch)
    monkeypatch.setattr(
        "wiz8decomp.class_this_typing.apply_this_typing_row",
        fake_row,
    )

    plan = {
        "functions": [
            {
                "address": "0x00403000",
                "name": "Controls::Draw",
                "owning_class": "Controls",
                "action": "bind-class-this",
            }
        ]
    }
    result = apply_this_typing(MagicMock(), plan, allow_custom_storage=False)
    assert result["errors"] == []
    assert len(result["skipped"]) == 1
    assert result["skipped"][0]["skipped"] == "namespace-collision"
