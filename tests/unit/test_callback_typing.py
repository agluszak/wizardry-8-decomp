"""Tests for curated callback field site identities."""

from __future__ import annotations

from wiz8decomp.callback_typing import _CALLBACK_FAMILIES


def test_dialog_callback_uses_class_identity() -> None:
    dialog = next(f for f in _CALLBACK_FAMILIES if f["name"] == "W8DialogDestroyCallback")
    assert dialog["fields"] == (("W8DialogBase", "m_destroy_callback"),)
