"""Tests for curated callback field site resolution."""

from __future__ import annotations

from types import SimpleNamespace

from wiz8decomp.callback_typing import (
    _CALLBACK_FAMILIES,
    _field_already_typed,
    _resolve_field_structure,
    definition_matches_family,
)


def test_callback_class_sites_are_owning_class_identities() -> None:
    class_owned = {
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
        if name in class_owned:
            assert fields, f"{name} should have field sites"
            for site, _field in fields:
                assert not str(site).startswith("/"), (
                    f"{name} class site must be an owning-class identity, not {site}"
                )
                assert not str(site).startswith("wiz8/classes")
        elif name in root_only:
            for site, _field in fields:
                assert site in {"/_GUI_BUTTON", "/_MOUSE_REGION"}
        elif name == "MOUSEBLT_HOOK":
            assert fields == []


def test_resolve_field_structure_prefers_bound_over_legacy(monkeypatch) -> None:
    bound = SimpleNamespace(
        getPathName=lambda: "/W8Monster",
        getDefinedComponents=list,
    )
    legacy = SimpleNamespace(
        getPathName=lambda: "/wiz8/classes/W8Monster",
        getDefinedComponents=list,
    )
    monkeypatch.setattr(
        "wiz8decomp.callback_typing.resolve_class_binding",
        lambda _program, _name: {
            "status": "bound",
            "structure_path": "/W8Monster",
            "legacy_enriched_path": "/wiz8/classes/W8Monster",
        },
    )
    monkeypatch.setattr(
        "wiz8decomp.callback_typing.find_ghidra_class",
        lambda _program, _name: object(),
    )
    monkeypatch.setattr(
        "wiz8decomp.callback_typing.find_class_structure",
        lambda _program, _gc: bound,
    )
    monkeypatch.setattr(
        "wiz8decomp.callback_typing.legacy_enriched_structure",
        lambda _program, _name: legacy,
    )
    structure, path, resolution = _resolve_field_structure(object(), "W8Monster")
    assert structure is bound
    assert path == "/W8Monster"
    assert resolution == "bound"


def test_resolve_field_structure_falls_back_to_legacy(monkeypatch) -> None:
    legacy = SimpleNamespace(
        getPathName=lambda: "/wiz8/classes/W8Region",
        getDefinedComponents=list,
    )
    monkeypatch.setattr(
        "wiz8decomp.callback_typing.resolve_class_binding",
        lambda _program, _name: {
            "status": "missing-class",
            "structure_path": None,
            "legacy_enriched_path": "/wiz8/classes/W8Region",
        },
    )
    monkeypatch.setattr(
        "wiz8decomp.callback_typing.find_ghidra_class",
        lambda _program, _name: None,
    )
    monkeypatch.setattr(
        "wiz8decomp.callback_typing.legacy_enriched_structure",
        lambda _program, _name: legacy,
    )
    structure, path, resolution = _resolve_field_structure(object(), "W8Region")
    assert structure is legacy
    assert path == "/wiz8/classes/W8Region"
    assert resolution == "legacy-enriched"


def test_field_already_typed_requires_definition_contract(monkeypatch) -> None:
    family = {
        "name": "CycleCallback",
        "return": "void",
        "convention": "__cdecl",
        "params": (("monster", "W8Monster *"),),
    }

    class _FD:
        def getPathName(self) -> str:
            return "/wiz8/callbacks/CycleCallback"

        def getName(self) -> str:
            return "CycleCallback"

        def getReturnType(self):
            return SimpleNamespace(getPathName=lambda: "/void", getName=lambda: "void")

        def getArguments(self):
            return [
                SimpleNamespace(
                    getName=lambda: "monster",
                    getDataType=lambda: SimpleNamespace(
                        getPathName=lambda: "/W8Monster *", getName=lambda: "W8Monster *"
                    ),
                )
            ]

        def getCallingConvention(self):
            return "__cdecl"

    class _Pointer:
        def getDataType(self):
            return _FD()

        def isPointer(self) -> bool:
            return True

    component = SimpleNamespace(getDataType=lambda: _Pointer())
    monkeypatch.setattr(
        "wiz8decomp.callback_typing.definition_matches_family",
        lambda _existing, _family, _program: True,
    )
    assert _field_already_typed(component, family, object()) is True

    monkeypatch.setattr(
        "wiz8decomp.callback_typing.definition_matches_family",
        lambda _existing, _family, _program: False,
    )
    assert _field_already_typed(component, family, object()) is False


def test_definition_matches_family_rejects_wrong_convention(monkeypatch) -> None:
    family = {
        "name": "CycleCallback",
        "return": "void",
        "convention": "__cdecl",
        "params": (),
    }
    monkeypatch.setattr(
        "wiz8decomp.callback_typing._resolve_type",
        lambda _program, spelling: SimpleNamespace(
            getPathName=lambda: f"/{spelling}", getName=lambda: spelling
        ),
    )
    existing = SimpleNamespace(
        getReturnType=lambda: SimpleNamespace(getPathName=lambda: "/void", getName=lambda: "void"),
        getArguments=list,
        getCallingConvention=lambda: "__stdcall",
    )
    assert definition_matches_family(existing, family, object()) is False
