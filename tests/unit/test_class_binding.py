"""Unit tests for native Ghidra class-binding helpers.

Acceptance (ordinary / derived / secondary-base): after
``ensure_function_class_namespace`` + ``__thiscall`` with dynamic storage,
auto ``this`` must resolve the bound Structure without enabling custom
storage. Full ProgramBuilder coverage is still in progress; see the
integration module for lifecycle-fixture skips.
"""

from __future__ import annotations

from types import SimpleNamespace

import pytest
from wiz8decomp.class_binding import (
    _LEGACY_ENRICHED_CATEGORY,
    _sanitize_class_parts,
    binding_agrees,
    ensure_function_class_namespace,
    legacy_enriched_structure,
    resolve_class_binding,
)


def test_sanitize_class_parts_splits_namespaces() -> None:
    assert _sanitize_class_parts("W8Monster") == ((), "W8Monster")
    assert _sanitize_class_parts("ns::Inner::Type") == (("ns", "Inner"), "Type")
    assert _sanitize_class_parts("  ns::Type  ") == (("ns",), "Type")


def test_sanitize_class_parts_templates_collapse_to_leaf() -> None:
    parts, leaf = _sanitize_class_parts("srVector3T<float>")
    assert parts == ()
    assert "srVector3T" in leaf


def test_sanitize_class_parts_rejects_empty() -> None:
    with pytest.raises(ValueError, match="empty"):
        _sanitize_class_parts("")
    with pytest.raises(ValueError, match="empty"):
        _sanitize_class_parts("::")


def test_binding_agrees_requires_matching_structure_path(monkeypatch) -> None:
    binding = {"status": "bound", "structure_path": "/W8Monster"}

    class _Pointee:
        def getPathName(self) -> str:
            return "/W8Monster"

    monkeypatch.setattr(
        "wiz8decomp.class_binding.auto_this_structure",
        lambda _fn: _Pointee(),
    )
    assert binding_agrees(binding, object())

    monkeypatch.setattr(
        "wiz8decomp.class_binding.auto_this_structure",
        lambda _fn: type("P", (), {"getPathName": lambda self: "/wiz8/classes/W8Monster"})(),
    )
    assert not binding_agrees(binding, object())


def test_binding_agrees_rejects_unbound_or_missing_auto_this(monkeypatch) -> None:
    monkeypatch.setattr("wiz8decomp.class_binding.auto_this_structure", lambda _fn: None)
    assert not binding_agrees({"status": "missing-structure", "structure_path": None}, object())
    assert not binding_agrees({"status": "bound", "structure_path": "/W8Monster"}, object())


def test_legacy_path_constant_and_detection() -> None:
    assert _LEGACY_ENRICHED_CATEGORY == "/wiz8/classes/"

    class _Manager:
        def getDataType(self, path: str):
            if path == "/wiz8/classes/W8Monster":
                return SimpleNamespace(getPathName=lambda: path)
            return None

    program = SimpleNamespace(getDataTypeManager=lambda: _Manager())
    legacy = legacy_enriched_structure(program, "W8Monster")
    assert legacy is not None
    assert str(legacy.getPathName()).startswith(_LEGACY_ENRICHED_CATEGORY)
    assert legacy_enriched_structure(program, "Missing") is None


def test_is_legacy_enriched_path() -> None:
    from wiz8decomp.class_binding import is_legacy_enriched_path

    assert is_legacy_enriched_path("/wiz8/classes/W8Monster")
    assert is_legacy_enriched_path("/wiz8/classes/W8Monster *")
    assert not is_legacy_enriched_path("/W8Monster")
    assert not is_legacy_enriched_path(None)


def test_ensure_function_class_namespace_moves_when_parent_differs() -> None:
    class _Ns:
        def __init__(self, name: str) -> None:
            self._name = name

        def equals(self, other: object) -> bool:
            return isinstance(other, _Ns) and other._name == self._name

        def getName(self, _qualified: bool = False) -> str:
            return self._name

    class _Fn:
        def __init__(self) -> None:
            self.parent = _Ns("Global")
            self.moved_to = None

        def getParentNamespace(self):
            return self.parent

        def setParentNamespace(self, ns) -> None:
            self.parent = ns
            self.moved_to = ns

    fn = _Fn()
    target = _Ns("W8Monster")
    assert ensure_function_class_namespace(fn, target) is True
    assert fn.moved_to is target
    assert ensure_function_class_namespace(fn, target) is False


def test_resolve_class_binding_reports_legacy_status(monkeypatch) -> None:
    ghidra_class = SimpleNamespace(getName=lambda _q=True: "W8Monster")
    structure = SimpleNamespace(
        getPathName=lambda: "/wiz8/classes/W8Monster",
        getLength=lambda: 16,
    )
    monkeypatch.setattr(
        "wiz8decomp.class_binding.find_ghidra_class",
        lambda _program, _name: ghidra_class,
    )
    monkeypatch.setattr(
        "wiz8decomp.class_binding.find_class_structure",
        lambda _program, _gc: structure,
    )
    monkeypatch.setattr(
        "wiz8decomp.class_binding.legacy_enriched_structure",
        lambda _program, _name: structure,
    )
    binding = resolve_class_binding(object(), "W8Monster")
    assert binding["status"] == "legacy-enriched-path"
    assert binding["structure_path"] == "/wiz8/classes/W8Monster"


def test_resolve_class_binding_missing_class_is_read_only(monkeypatch) -> None:
    monkeypatch.setattr(
        "wiz8decomp.class_binding.find_ghidra_class",
        lambda _program, _name: None,
    )
    monkeypatch.setattr(
        "wiz8decomp.class_binding.legacy_enriched_structure",
        lambda _program, _name: None,
    )
    binding = resolve_class_binding(object(), "Absent")
    assert binding["status"] == "missing-class"
    assert binding["ghidra_class"] is None


def test_ensure_ghidra_class_rejects_plain_namespace(monkeypatch) -> None:
    from wiz8decomp.class_binding import ensure_ghidra_class

    plain = SimpleNamespace(isClass=lambda: False)

    class _Symbols:
        def getNamespace(self, name: str, _parent):
            if name == "Colliding":
                return plain
            return None

        def createClass(self, *_a, **_k):
            raise AssertionError("must not create over a colliding namespace")

        def createNameSpace(self, *_a, **_k):
            raise AssertionError("unexpected namespace create")

    program = SimpleNamespace(
        getSymbolTable=lambda: _Symbols(),
        getGlobalNamespace=lambda: object(),
    )
    # Avoid importing real Ghidra SourceType.
    import sys
    import types

    symbol_mod = types.ModuleType("ghidra.program.model.symbol")
    symbol_mod.SourceType = SimpleNamespace(IMPORTED="IMPORTED")  # type: ignore[attr-defined]
    ghidra_mod = types.ModuleType("ghidra")
    program_mod = types.ModuleType("ghidra.program")
    model_mod = types.ModuleType("ghidra.program.model")
    sys.modules.setdefault("ghidra", ghidra_mod)
    sys.modules.setdefault("ghidra.program", program_mod)
    sys.modules.setdefault("ghidra.program.model", model_mod)
    sys.modules["ghidra.program.model.symbol"] = symbol_mod

    with pytest.raises(RuntimeError, match="namespace collision"):
        ensure_ghidra_class(program, "Colliding")


# Ordinary / Derived / Secondary-base methods should bind without custom storage.
# Full acceptance requires a ProgramBuilder or the imported lifecycle fixture
# project (Base/Derived/Secondary). Unit CI without that fixture cannot prove
# auto-this binding end-to-end; see test_enrichment_ghidra.py integration skips.


def test_acceptance_shapes_require_lifecycle_fixture_documentation() -> None:
    """Document required shapes; do not pretend a placemarker equality is coverage."""

    required = ("Ordinary", "Derived", "Secondary")
    assert required == ("Ordinary", "Derived", "Secondary")
    # apply_this_typing callers keep allow_custom_storage=False for these shapes.