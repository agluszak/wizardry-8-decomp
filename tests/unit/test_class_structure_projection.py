"""Tests for class Structure projection helpers."""

from __future__ import annotations

from pathlib import Path
from types import SimpleNamespace

from wiz8decomp.class_structure_projection import (
    _classes_by_name,
    _decide_structure_action,
    _is_useful,
    _richness,
    _simple_name,
    _structure_path_tier,
)


class _FakeStructure:
    def __init__(self, length: int, components: int, path: str = "/X") -> None:
        self._length = length
        self._components = components
        self._path = path

    def getLength(self) -> int:
        return self._length

    def getDefinedComponents(self):
        return [object()] * self._components

    def getPathName(self) -> str:
        return self._path


def test_simple_name() -> None:
    assert _simple_name("ns::W8Monster") == "W8Monster"
    assert _simple_name("srFlags<srNode::e_flag>") == "srFlags<srNode::e_flag>"
    assert _simple_name("srGERD::Renderer::Parameters") == "Parameters"


def test_class_lookup_uses_requested_target() -> None:
    class Key:
        def __init__(self, target: str) -> None:
            self.target = target

    wiz8 = SimpleNamespace(qualified_name="Shared", size=8)
    surrender = SimpleNamespace(qualified_name="Shared", size=16)
    left = SimpleNamespace(qualified_name="Left::Listener", size=4)
    right = SimpleNamespace(qualified_name="Right::Listener", size=8)
    index = SimpleNamespace(
        classes={
            Key("WIZ8"): wiz8,
            Key("SURRENDER"): surrender,
            Key("WIZ8"): left,
            Key("WIZ8"): right,
        }
    )
    assert _classes_by_name(index, "WIZ8")["Shared"] is wiz8
    assert "Listener" not in _classes_by_name(index, "WIZ8")


def test_global_template_type_is_selected_from_trusted_layout(tmp_path: Path, monkeypatch) -> None:
    from wiz8decomp import class_structure_projection as csp

    class Key:
        target = "WIZ8"

    source = tmp_path / "src/wiz8/globals.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "// GLOBAL: WIZ8 0x00650000\nsrHeapBuffer<unsigned char> g_blob;\n",
        encoding="utf-8",
    )
    record = SimpleNamespace(
        qualified_name="srHeapBuffer<unsigned char>", layout_trusted=True, size=8
    )
    index = SimpleNamespace(classes={Key(): record})
    monkeypatch.setattr(csp, "source_functions", lambda *_a: {})
    assert csp._referenced_template_classes(tmp_path, "WIZ8", index) == {
        "srHeapBuffer<unsigned char>": 1
    }


def test_richness_and_usefulness() -> None:
    empty = _FakeStructure(1, 0)
    shell = _FakeStructure(840, 0)
    rich = _FakeStructure(840, 12)
    assert _richness(None) == (0, 0)
    assert _richness(rich) == (12, 840)
    assert not _is_useful(empty)
    assert _is_useful(shell)
    assert _is_useful(rich)


def test_structure_path_tier_prefers_identity_path() -> None:
    assert _structure_path_tier("/W8Monster", "W8Monster", "W8Monster") == 0
    assert _structure_path_tier("/Demangler/W8Monster", "W8Monster", "W8Monster") == 3
    assert _structure_path_tier("/other/W8Monster", "W8Monster", "W8Monster") == 4
    assert _structure_path_tier("/ns/Foo", "Foo", "ns::Foo") == 0
    assert _structure_path_tier("/Foo", "Foo", "ns::Foo") == 2


def test_decide_agrees_when_bound_is_useful() -> None:
    bound = _FakeStructure(840, 2, "/W8Monster")
    richer_source = _FakeStructure(840, 40, "/other/W8Monster")
    assert (
        _decide_structure_action(
            bound=bound,
            legacy=None,
            source=richer_source,
            asserted_size=840,
            source_size_ok=True,
            size_mismatched_source=None,
        )
        == "agree"
    )


def test_decide_conflict_when_bound_size_mismatches_asserted() -> None:
    bound = _FakeStructure(100, 2, "/W8Monster")
    assert (
        _decide_structure_action(
            bound=bound,
            legacy=None,
            source=None,
            asserted_size=840,
            source_size_ok=True,
            size_mismatched_source=None,
        )
        == "conflict"
    )


def test_decide_legacy_duplicate_when_paths_differ() -> None:
    bound = _FakeStructure(840, 2, "/W8Monster")
    legacy = _FakeStructure(840, 2, "/wiz8/classes/W8Monster")
    assert (
        _decide_structure_action(
            bound=bound,
            legacy=legacy,
            source=None,
            asserted_size=840,
            source_size_ok=True,
            size_mismatched_source=None,
        )
        == "legacy-duplicate"
    )


def test_decide_bind_existing_without_richness_contest() -> None:
    # Bound absent / not useful; any useful sized source binds — richness unused.
    thin_source = _FakeStructure(840, 1, "/W8Monster")
    assert (
        _decide_structure_action(
            bound=None,
            legacy=None,
            source=thin_source,
            asserted_size=840,
            source_size_ok=True,
            size_mismatched_source=None,
        )
        == "bind-existing"
    )


def test_decide_create_opaque_when_no_useful_bound() -> None:
    assert (
        _decide_structure_action(
            bound=None,
            legacy=None,
            source=None,
            asserted_size=840,
            source_size_ok=True,
            size_mismatched_source=None,
        )
        == "create-opaque"
    )


def test_decide_one_byte_asserted_structure() -> None:
    empty = _FakeStructure(1, 0, "/Option")
    assert (
        _decide_structure_action(
            bound=None,
            legacy=None,
            source=None,
            asserted_size=1,
            source_size_ok=True,
            size_mismatched_source=None,
        )
        == "create-opaque"
    )
    assert (
        _decide_structure_action(
            bound=empty,
            legacy=None,
            source=None,
            asserted_size=1,
            source_size_ok=True,
            size_mismatched_source=None,
        )
        == "agree"
    )


def test_collect_plan_does_not_call_ensure(monkeypatch) -> None:
    from wiz8decomp import class_structure_projection as csp

    calls: list[str] = []

    def boom(*_a, **_k):
        calls.append("ensure")
        raise AssertionError("collect must not ensure_ghidra_class")

    monkeypatch.setattr(csp, "ensure_ghidra_class", boom)
    monkeypatch.setattr(csp, "find_ghidra_class", lambda *_a, **_k: None)
    monkeypatch.setattr(csp, "_thiscall_owning_classes", lambda *_a, **_k: {"W8Monster": 1})
    monkeypatch.setattr(
        csp,
        "_classes_by_name",
        lambda _index, _target: {
            "W8Monster": SimpleNamespace(asserted_size=840, qualified_name="W8Monster")
        },
    )
    monkeypatch.setattr(
        "wiz8decomp.class_structure_projection.load_source_index",
        lambda _repo: {"classes": [], "markers": []},
    )
    monkeypatch.setattr(
        "wiz8decomp.class_structure_projection.SourceIndex.from_dict",
        lambda _d: SimpleNamespace(classes={}),
    )
    monkeypatch.setattr(csp, "_find_named_structure", lambda *_a, **_k: None)
    monkeypatch.setattr(csp, "_wiz8_structure", lambda *_a, **_k: None)

    plan = csp.collect_structure_projection_plan(
        object(),  # type: ignore[arg-type]
        object(),
        target="WIZ8",
    )
    assert calls == []
    assert plan["counts"].get("create-class") == 1
    assert plan["classes"][0]["action"] == "create-class"


def test_collect_plan_includes_sized_record_without_methods(monkeypatch) -> None:
    from wiz8decomp import class_structure_projection as csp

    record = SimpleNamespace(
        asserted_size=224,
        qualified_name="W8WorldCursorState",
        target="WIZ8",
    )
    monkeypatch.setattr(csp, "_thiscall_owning_classes", lambda *_a, **_k: {})
    monkeypatch.setattr(csp, "_referenced_template_classes", lambda *_a, **_k: {})
    monkeypatch.setattr(csp, "find_ghidra_class", lambda *_a, **_k: None)
    monkeypatch.setattr(csp, "_find_named_structure", lambda *_a, **_k: None)
    monkeypatch.setattr(csp, "_wiz8_structure", lambda *_a, **_k: None)
    monkeypatch.setattr(
        csp,
        "load_source_index",
        lambda _repo: {
            "classes": [
                {
                    "qualified_name": "W8WorldCursorState",
                    "asserted_size": 224,
                    "target": "WIZ8",
                }
            ]
        },
    )
    monkeypatch.setattr(
        csp.SourceIndex,
        "from_dict",
        lambda _data: SimpleNamespace(classes={type("Key", (), {"target": "WIZ8"})(): record}),
    )

    plan = csp.collect_structure_projection_plan(object(), object(), target="WIZ8")

    assert plan["counts"] == {"create-class": 1}
    assert plan["classes"][0]["class"] == "W8WorldCursorState"
    assert plan["classes"][0]["asserted_size"] == 224
