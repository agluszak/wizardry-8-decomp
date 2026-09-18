"""Tests for curated callback field site paths."""

from __future__ import annotations

from wiz8decomp.callback_typing import _CALLBACK_FAMILIES


def test_callback_fields_mutate_wiz8_classes_not_root_pdb() -> None:
    projected = {
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
        if name in projected:
            assert fields, f"{name} should have field sites"
            for structure_path, _field in fields:
                assert structure_path.startswith("/wiz8/classes/"), (
                    f"{name} must not mutate root/PDB site {structure_path}"
                )
            assert not any(path.startswith(("/W8", "/Trigger")) for path, _ in fields)
        elif name in root_only:
            for structure_path, _field in fields:
                assert structure_path in {"/_GUI_BUTTON", "/_MOUSE_REGION"}
        elif name == "MOUSEBLT_HOOK":
            assert fields == []


def test_dialog_callback_prefers_projected_class_only() -> None:
    dialog = next(f for f in _CALLBACK_FAMILIES if f["name"] == "W8DialogDestroyCallback")
    assert dialog["fields"] == (("/wiz8/classes/W8DialogBase", "m_destroy_callback"),)
