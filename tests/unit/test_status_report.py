"""Focused tests for project-wide source and matching statistics.

Every assertion here builds its own minimal reccmp surface, so the tests state
what the derivation does rather than restating current repository counts.
"""

from __future__ import annotations

from types import SimpleNamespace

import pytest
from reccmp.compare.diagnosis import (
    ComparisonAnalysis,
    ComparisonDifference,
    DifferenceSide,
)
from reccmp.compare.report import ReccmpComparedEntity
from reccmp.parser.marker import MarkerType
from reccmp.types import EntityType
from wiz8decomp.reports import status


class FakeCodebase:
    def __init__(self, markers):
        self._markers = list(markers)

    def iter_line_functions(self):
        return iter(self._markers)

    def iter_name_functions(self):
        return iter(())


def _marker(address: int, marker_type: MarkerType):
    return SimpleNamespace(offset=address, type=marker_type)


def _entity(
    address: int,
    analysis: ComparisonAnalysis,
    *,
    accuracy: float = 1.0,
    name: str = "Function",
) -> ReccmpComparedEntity:
    return ReccmpComparedEntity(
        orig_addr=address,
        recomp_addr=address + 0x1000,
        name=name,
        type=EntityType.FUNCTION,
        accuracy=accuracy,
        analysis=analysis,
    )


def _engine(markers, entities):
    return SimpleNamespace(
        codebase=FakeCodebase(markers),
        compare_addresses=lambda **_kwargs: entities,
    )


def test_function_markers_are_the_only_recovered_source() -> None:
    markers = [
        _marker(0x401000, MarkerType.FUNCTION),
        _marker(0x401050, MarkerType.FUNCTION),
        _marker(0x401010, MarkerType.STUB),
        _marker(0x401020, MarkerType.LIBRARY),
        _marker(0x401030, MarkerType.SYNTHETIC),
        _marker(0x401040, MarkerType.TEMPLATE),
    ]

    source, addresses = status._source_statistics(SimpleNamespace(codebase=FakeCodebase(markers)))

    assert source == {
        "functions": 2,
        "stubs": 1,
        "library": 1,
        "synthetic": 1,
        "template": 1,
    }
    assert addresses == {0x401000, 0x401050}


def test_comparison_classification_and_effective_score() -> None:
    addresses = {0x401000, 0x401010, 0x401020, 0x401030, 0x401040}
    entities = [
        _entity(0x401000, ComparisonAnalysis.exact(), accuracy=1.0),
        _entity(
            0x401010,
            ComparisonAnalysis.effective(("register_allocation",)),
            accuracy=0.2,
        ),
        _entity(
            0x401020,
            ComparisonAnalysis.mismatch(
                ComparisonDifference(
                    kind="call_argument",
                    orig=DifferenceSide(),
                    recomp=DifferenceSide(),
                )
            ),
            accuracy=0.75,
        ),
        _entity(
            0x401030,
            ComparisonAnalysis.inconclusive("analysis_limit"),
            accuracy=0.25,
        ),
    ]
    target = SimpleNamespace(report_config=None)

    comparison, effective_score = status._comparison_statistics(
        _engine([], entities), target, addresses
    )

    assert comparison["exact"] == 1
    assert comparison["effective"] == 1
    assert comparison["mismatch"] == 1
    assert comparison["inconclusive"] == 1
    assert comparison["unpaired"] == 1
    assert comparison["paired"] == 4
    assert effective_score == pytest.approx(3.0)
    assert comparison["accuracy"] == pytest.approx(0.75)


def test_ignored_source_functions_are_counted_not_dropped() -> None:
    addresses = {0x401000, 0x401010}
    entities = [
        _entity(0x401000, ComparisonAnalysis.exact()),
        _entity(0x401010, ComparisonAnalysis.exact(), name="Ignored"),
    ]
    target = SimpleNamespace(report_config=SimpleNamespace(ignore_functions=["Ignored"]))

    comparison, effective_score = status._comparison_statistics(
        _engine([], entities), target, addresses
    )

    assert comparison["ignored"] == 1
    assert comparison["paired"] == 1
    assert comparison["exact"] == 1
    assert effective_score == pytest.approx(1.0)


def _target_row(*, functions: int, paired: int, original: int | None) -> dict:
    return {
        "state": "comparison",
        "source": {"functions": functions},
        "comparison": {
            "paired": paired,
            "exact": 0,
            "effective": 0,
            "mismatch": paired,
            "inconclusive": 0,
            "unpaired": 0,
            "ignored": 0,
            "accuracy": 0.0,
        },
        "original_functions": original,
    }


def test_project_totals_weight_by_function_count() -> None:
    targets = {
        "WIZ8": _target_row(functions=9, paired=9, original=100),
        "SREXT_UNZIP": _target_row(functions=1, paired=1, original=10),
        "SRDD_OPENGL": {"state": "original-only"},
    }
    scores = {"WIZ8": 0.0, "SREXT_UNZIP": 1.0}

    totals = status._totals(targets, scores)

    assert totals["targets"] == 3
    assert totals["comparison_targets"] == 2
    # Mean-of-percentages would be 0.5; summed-score coverage is 1/10.
    assert totals["accuracy"] == pytest.approx(0.1)
    assert totals["known_original_scope"] == {
        "targets": 2,
        "original_functions": 110,
        "source_functions": 10,
        "source_coverage": pytest.approx(10 / 110),
        "progress": pytest.approx(1 / 110),
    }


def test_status_report_uses_hash_matched_original_denominator(tmp_path, monkeypatch) -> None:
    marker = _marker(0x401000, MarkerType.FUNCTION)
    engine = _engine([marker], [_entity(0x401000, ComparisonAnalysis.exact())])

    def partial(filename, sha256, *, recompiled):
        return SimpleNamespace(
            filename=filename,
            sha256=sha256,
            recompiled_path=(tmp_path / filename if recompiled else None),
            recompiled_pdb=(tmp_path / (filename + ".pdb") if recompiled else None),
            report_config=None,
        )

    project = SimpleNamespace(
        targets={
            "WIZ8": partial("Wiz8.exe", "a" * 64, recompiled=True),
            "SREXT_UNZIP": partial("srEXT_Unzip.dll", "b" * 64, recompiled=True),
            "SRDD_OPENGL": partial("srDD_OpenGL.dll", "c" * 64, recompiled=False),
        },
        get=lambda target: SimpleNamespace(target_id=target),
    )
    built = []
    monkeypatch.setattr(status, "build_target", lambda _settings, target: built.append(target))
    monkeypatch.setattr(status.RecCmpProject, "from_directory", lambda _path: project)
    monkeypatch.setattr(status.Compare, "from_target", lambda *_args, **_kwargs: engine)
    monkeypatch.setattr(
        status,
        "seed_records",
        lambda _settings: [{"binary_sha256": "a" * 64, "function_count": 7701}],
    )

    report = status.status_report(SimpleNamespace(repo_dir=tmp_path))

    assert built == ["reccmp-products"]
    assert set(report["targets"]) == {"WIZ8", "SREXT_UNZIP", "SRDD_OPENGL"}
    assert report["targets"]["SRDD_OPENGL"] == {
        "binary": "srDD_OpenGL.dll",
        "state": "original-only",
    }

    wiz8 = report["targets"]["WIZ8"]
    assert wiz8["state"] == "comparison"
    assert wiz8["original_functions"] == 7701
    assert wiz8["source_coverage"] == pytest.approx(1 / 7701)
    assert wiz8["progress"] == pytest.approx(1 / 7701)

    unzip = report["targets"]["SREXT_UNZIP"]
    assert unzip["state"] == "comparison"
    assert unzip["original_functions"] is None
    assert unzip["source_coverage"] is None
    assert unzip["progress"] is None

    assert report["totals"]["known_original_scope"] == {
        "targets": 1,
        "original_functions": 7701,
        "source_functions": 1,
        "source_coverage": pytest.approx(1 / 7701),
        "progress": pytest.approx(1 / 7701),
    }
