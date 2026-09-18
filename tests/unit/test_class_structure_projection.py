"""Tests for class Structure projection helpers."""

from __future__ import annotations

from wiz8decomp.class_structure_projection import (
    _is_useful,
    _richness,
    _simple_name,
    _structure_path_tier,
)


class _FakeStructure:
    def __init__(self, length: int, components: int) -> None:
        self._length = length
        self._components = components

    def getLength(self) -> int:
        return self._length

    def getDefinedComponents(self):
        return [object()] * self._components


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
    assert _richness(rich) > _richness(shell)


def test_structure_path_tier_prefers_root_then_demangler() -> None:
    assert _structure_path_tier("/W8Monster", "W8Monster", "W8Monster") == 0
    assert _structure_path_tier("/Demangler/W8Monster", "W8Monster", "W8Monster") == 1
    assert _structure_path_tier("/other/W8Monster", "W8Monster", "W8Monster") == 2
