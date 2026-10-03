"""Focused tests for project-wide source and pairing statistics.

Every assertion here builds its own minimal reccmp surface, so the tests state
what the derivation does rather than restating current repository counts.
"""

from __future__ import annotations

import logging
from types import SimpleNamespace

import pytest
from reccmp.compare.db import PairBasis
from reccmp.parser.marker import MarkerType
from typer.testing import CliRunner
from wiz8decomp import command_support
from wiz8decomp.cli import app
from wiz8decomp.reports import status


class FakeCodebase:
    def __init__(self, markers):
        self._markers = list(markers)

    def iter_line_functions(self):
        return iter(self._markers)

    def iter_name_functions(self):
        return iter(())


class _FakeMarker:
    def __init__(self, address: int, marker_type: MarkerType, *, nameref: bool = False):
        self.offset = address
        self.type = marker_type
        self._nameref = nameref

    def is_nameref(self) -> bool:
        return self._nameref


def _marker(address: int, marker_type: MarkerType, *, nameref: bool = False):
    return _FakeMarker(address, marker_type, nameref=nameref)


def _match(address: int, name: str = "Function") -> SimpleNamespace:
    return SimpleNamespace(orig_addr=address, best_name=lambda: name)


def _engine(markers, matches, basis=PairBasis.ANNOTATION):
    by_address = {match.orig_addr: match for match in matches}
    return SimpleNamespace(
        codebase=FakeCodebase(markers),
        get_match=by_address.get,
        pair_basis=lambda address: basis if address in by_address else None,
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

    from pathlib import Path

    source, addresses, name_refs = status._source_statistics(
        SimpleNamespace(
            codebase=FakeCodebase(markers),
            data_sources=[
                SimpleNamespace(
                    path=Path("emissions.csv"),
                    text="address|symbol|name|type\n00401030||Destructor|synthetic\n00401040||Grow<int>|template\n",
                )
            ],
        )
    )

    assert source == {
        "functions": 2,
        "stubs": 1,
        "library": 1,
        "synthetic": 1,
        "template": 1,
    }
    assert addresses == {0x401000, 0x401050}
    assert name_refs == set()


def test_pairing_counts_paired_unpaired_and_basis() -> None:
    addresses = {0x401000, 0x401010, 0x401020}
    target = SimpleNamespace(report_config=None)

    pairing = status._pairing_statistics(
        _engine([], [_match(0x401000), _match(0x401010)]), target, addresses, set()
    )

    assert pairing["paired"] == 2
    assert pairing["unpaired"] == 1
    assert pairing["unpaired_line_refs"] == 1
    assert pairing["pair_basis"] == {"annotation": 2}


def test_unpaired_name_refs_are_not_line_ref_diagnostics() -> None:
    # A SYMBOL/name-reference marker may legitimately stay unpaired when the
    # recomp emits no standalone copy; a line-reference marker that stays
    # unpaired is the "Failed to find function symbol" diagnostic.
    addresses = {0x401000, 0x401010, 0x401020}
    target = SimpleNamespace(report_config=None)

    pairing = status._pairing_statistics(
        _engine([], [_match(0x401000)]), target, addresses, {0x401020}
    )

    assert pairing["unpaired"] == 2
    assert pairing["unpaired_line_refs"] == 1


def test_reccmp_diagnostics_are_captured_and_counted() -> None:
    logger = logging.getLogger("reccmp.test")
    with status._capture_reccmp_diagnostics() as capture:
        logger.error("line lookup failed")
        logger.warning("something suspicious")
        logger.info("not a diagnostic")

    diagnostics = status._diagnostics(capture)

    assert diagnostics["error_count"] == 1
    assert diagnostics["warning_count"] == 1
    assert diagnostics["errors"] == ["reccmp.test: line lookup failed"]
    assert diagnostics["warnings"] == ["reccmp.test: something suspicious"]
    assert diagnostics["truncated"] == 0


def test_ignored_source_functions_are_counted_not_dropped() -> None:
    addresses = {0x401000, 0x401010}
    target = SimpleNamespace(report_config=SimpleNamespace(ignore_functions=["Ignored"]))

    pairing = status._pairing_statistics(
        _engine([], [_match(0x401000), _match(0x401010, "Ignored")]), target, addresses, set()
    )

    assert pairing["ignored"] == 1
    assert pairing["paired"] == 1


def _target_row(*, functions: int, paired: int, original: int | None) -> dict:
    return {
        "state": "comparison",
        "source": {"functions": functions},
        "pairing": {
            "paired": paired,
            "unpaired": functions - paired,
            "unpaired_line_refs": 0,
            "ignored": 0,
            "pair_basis": {},
        },
        "diagnostics": {"error_count": 0},
        "original_functions": original,
    }


def test_project_totals_sum_counts_and_coverage() -> None:
    targets = {
        "WIZ8": _target_row(functions=9, paired=8, original=100),
        "SREXT_UNZIP": _target_row(functions=1, paired=1, original=10),
        "SRDD_OPENGL": {"state": "original-only"},
    }

    totals = status._totals(targets)

    assert totals["targets"] == 3
    assert totals["comparison_targets"] == 2
    assert totals["paired"] == 9
    assert totals["unpaired"] == 1
    assert totals["known_original_scope"] == {
        "targets": 2,
        "original_functions": 110,
        "source_functions": 10,
        "source_coverage": pytest.approx(10 / 110),
    }


def test_status_report_uses_hash_matched_original_denominator(tmp_path, monkeypatch) -> None:
    marker = _marker(0x401000, MarkerType.FUNCTION)
    engine = _engine([marker], [_match(0x401000)])

    def partial(filename, sha256, *, recompiled):
        if recompiled:
            product = tmp_path / filename
            pdb = tmp_path / (filename + ".pdb")
            product.write_bytes(b"product")
            pdb.write_bytes(b"pdb")
        else:
            product = None
            pdb = None
        return SimpleNamespace(
            filename=filename,
            sha256=sha256,
            recompiled_path=product,
            recompiled_pdb=pdb,
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
    monkeypatch.setattr(status, "warn_if_build_may_be_stale", lambda *_args: None)
    monkeypatch.setattr(status.RecCmpProject, "from_directory", lambda _path: project)
    monkeypatch.setattr(status.Compare, "from_target", lambda *_args, **_kwargs: engine)
    monkeypatch.setattr(
        status,
        "seed_records",
        lambda _settings: [{"binary_sha256": "a" * 64, "function_count": 7701}],
    )

    report = status.status_report(SimpleNamespace(repo_dir=tmp_path))

    assert set(report["targets"]) == {"WIZ8", "SREXT_UNZIP", "SRDD_OPENGL"}
    assert report["targets"]["SRDD_OPENGL"] == {
        "binary": "srDD_OpenGL.dll",
        "state": "original-only",
    }

    wiz8 = report["targets"]["WIZ8"]
    assert wiz8["state"] == "comparison"
    assert wiz8["original_functions"] == 7701
    assert wiz8["source_coverage"] == pytest.approx(1 / 7701)
    assert wiz8["pairing"]["paired"] == 1
    assert wiz8["last_comparison"] is None

    unzip = report["targets"]["SREXT_UNZIP"]
    assert unzip["state"] == "comparison"
    assert unzip["original_functions"] is None
    assert unzip["source_coverage"] is None
    assert report["totals"]["known_original_scope"] == {
        "targets": 1,
        "original_functions": 7701,
        "source_functions": 1,
        "source_coverage": pytest.approx(1 / 7701),
    }


def test_status_report_marks_configured_missing_product_unbuilt(tmp_path, monkeypatch) -> None:
    partial = SimpleNamespace(
        filename="Wiz8.exe",
        sha256="a" * 64,
        recompiled_path=tmp_path / "Wiz8.exe",
        recompiled_pdb=tmp_path / "Wiz8.pdb",
        report_config=None,
    )
    project = SimpleNamespace(
        targets={"WIZ8": partial},
        get=lambda _target: pytest.fail("unbuilt target must not be opened"),
    )
    monkeypatch.setattr(status.RecCmpProject, "from_directory", lambda _path: project)
    monkeypatch.setattr(
        status,
        "warn_if_build_may_be_stale",
        lambda *_args: pytest.fail("unbuilt target is not stale"),
    )
    monkeypatch.setattr(
        status.Compare, "from_target", lambda *_args, **_kwargs: pytest.fail("must not compare")
    )
    monkeypatch.setattr(status, "seed_records", lambda _settings: [])

    report = status.status_report(SimpleNamespace(repo_dir=tmp_path))

    assert report["targets"]["WIZ8"] == {"binary": "Wiz8.exe", "state": "unbuilt"}
    assert report["totals"]["comparison_targets"] == 0


def test_status_build_is_explicit(monkeypatch) -> None:
    from wiz8decomp import build

    events = []
    monkeypatch.setattr(command_support, "settings", lambda: object())
    monkeypatch.setattr(build, "build_target", lambda _, target: events.append(target))
    monkeypatch.setattr(status, "status_report", lambda _settings: {"ok": True})

    default = CliRunner().invoke(app, ["report", "status"])
    explicit = CliRunner().invoke(app, ["report", "status", "--build"])

    assert default.exit_code == 0, default.output
    assert explicit.exit_code == 0, explicit.output
    assert events == ["reccmp-products"]
