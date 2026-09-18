"""Tests for curated callback field site identities."""

from __future__ import annotations

from wiz8decomp.callback_typing import _CALLBACK_FAMILIES
from wiz8decomp.datatype_contracts import is_legacy_path


def test_callback_class_fields_use_identities_not_legacy_paths() -> None:
    class_families = {
        "W8RegionCallback",
        "W8DialogDestroyCallback",
        "W8ControlCallback",
        "CycleCallback",
        "ActivationCallback",
    }
    root_only = {"GUI_CALLBACK", "MOUSE_CALLBACK"}
    for family in _CALLBACK_FAMILIES:
        name = family["name"]
        fields = list(family["fields"])
        if name in class_families:
            assert fields, f"{name} should have field sites"
            for owner, _field in fields:
                assert not str(owner).startswith("/"), owner
                assert not is_legacy_path(str(owner))
        elif name in root_only:
            for owner, _field in fields:
                assert owner in {"/_GUI_BUTTON", "/_MOUSE_REGION"}
        elif name == "MOUSEBLT_HOOK":
            assert fields == []


def test_dialog_callback_uses_class_identity() -> None:
    dialog = next(f for f in _CALLBACK_FAMILIES if f["name"] == "W8DialogDestroyCallback")
    assert dialog["fields"] == (("W8DialogBase", "m_destroy_callback"),)
