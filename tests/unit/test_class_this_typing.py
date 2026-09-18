"""Tests for class-binding helpers used by automatic ``this`` typing.

Broader class-binding coverage lives in ``test_class_binding.py``.
"""

from __future__ import annotations

from wiz8decomp.class_this_typing import _storage_matches


def test_storage_matches_compares_parameter_slots() -> None:
    before = {
        "return_storage": "EAX:4",
        "parameters": [{"ordinal": 0, "name": "this", "storage": "ECX:4"}],
    }
    after = {
        "return_storage": "EAX:4",
        "parameters": [{"ordinal": 0, "name": "this", "storage": "ECX:4"}],
    }
    assert _storage_matches(before, after)
    after["parameters"][0]["storage"] = "Stack[0x4]:4"
    assert not _storage_matches(before, after)
