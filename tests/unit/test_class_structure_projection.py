"""Tests for class Structure projection helpers."""

from __future__ import annotations

from wiz8decomp.class_structure_projection import (
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


def test_richness_and_usefulness() -> None:
    empty = _FakeStructure(1, 0)
    shell = _FakeStructure(840, 0)
    rich = _FakeStructure(840, 12)
    assert _richness(None) == (0, 0)
    assert _richness(rich) == (12, 840)
    assert not _is_useful(empty)
    assert _is_useful(shell)
    assert _is_useful(rich)


def test_structure_path_tier_prefers_root_then_demangler() -> None:
    assert _structure_path_tier("/W8Monster", "W8Monster", "W8Monster") == 0
    assert _structure_path_tier("/Demangler/W8Monster", "W8Monster", "W8Monster") == 1
    assert _structure_path_tier("/other/W8Monster", "W8Monster", "W8Monster") == 2


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
