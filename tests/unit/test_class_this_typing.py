"""Tests for class-binding helpers used by automatic ``this`` typing."""

from __future__ import annotations

from wiz8decomp.class_binding import _sanitize_class_parts, binding_agrees


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
