"""Focused tests for the built sr.dll export-table comparison."""

from __future__ import annotations

from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp import surrender_exports
from wiz8decomp.surrender_exports import built_export_disagreements


def _evidence(**ordinals: int) -> dict[str, dict[str, str]]:
    return {name: {"ordinal": str(ordinal)} for name, ordinal in ordinals.items()}


def test_matching_table_has_no_disagreement() -> None:
    assert built_export_disagreements({"a": 1, "b": 2}, _evidence(a=1, b=2)) == []


def test_an_extra_class_member_export_is_named_with_the_renumbering_it_causes() -> None:
    """An operator new on a dllexport class exports without touching sr.def."""
    built = {"??2Quantizer": 1, "a": 2, "b": 3}

    assert built_export_disagreements(built, _evidence(a=1, b=2)) == [
        "not a retail export: @1 ??2Quantizer",
        "@2 a: retail ordinal 1",
        "@3 b: retail ordinal 2",
    ]


def test_a_missing_retail_export_is_reported() -> None:
    assert built_export_disagreements({"a": 1}, _evidence(a=1, b=2)) == [
        "retail export not built: @2 b"
    ]


@pytest.mark.parametrize(
    "built,should_fail",
    [
        ({"retail": 7, "implicit": 8}, False),
        ({"retail": 8, "implicit": 9}, True),
        ({"implicit": 8}, True),
        ({"retail": 7, "implicit": 8, "authored": 9}, True),
    ],
)
def test_implicit_extras_do_not_hide_missing_names_ordinal_drift_or_authored_exports(
    monkeypatch, built, should_fail
):
    import pefile

    pe = SimpleNamespace(
        parse_data_directories=lambda **_: None,
        DIRECTORY_ENTRY_EXPORT=SimpleNamespace(
            symbols=[
                SimpleNamespace(name=name.encode(), ordinal=ordinal)
                for name, ordinal in built.items()
            ]
        ),
    )
    monkeypatch.setattr(pefile, "PE", lambda *_args, **_kwargs: pe)
    monkeypatch.setattr(surrender_exports, "_evidence_exports", lambda _: _evidence(retail=7))
    monkeypatch.setattr(surrender_exports, "_implicit_special_members", lambda *_: {"implicit"})
    if should_fail:
        with pytest.raises(surrender_exports.SurrenderExportsError):
            surrender_exports.validate_built_surrender_exports(Path("repo"), Path("sr.dll"))
    else:
        result = surrender_exports.validate_built_surrender_exports(Path("repo"), Path("sr.dll"))
        assert result["ok"] is True
        assert result["exact"] is False
        assert result["compiler_exports_absent_from_retail"] == ["implicit"]
