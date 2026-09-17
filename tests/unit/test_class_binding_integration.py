"""Integration probes for class binding on lifecycle fixture classes.

Skips unless a built lifecycle binary exists under ``build/recovery-fixture/``
and pyghidra can open a program. Full empty-ProgramBuilder fixtures for
Ordinary / Derived / Secondary auto-``this`` remain in progress.
"""

from __future__ import annotations

from pathlib import Path

import pytest

pytest.importorskip("pyghidra")

from wiz8decomp.class_binding import resolve_class_binding
from wiz8decomp.config import load_settings

pytestmark = pytest.mark.integration

_REPO = Path(__file__).resolve().parents[2]
_LIFECYCLE_CLASSES = ("Base", "Derived", "Secondary", "VirtualDerived")


def _lifecycle_executable() -> Path | None:
    candidates = [
        _REPO / "build/recovery-fixture/vc6-sp5/lifecycle_probe.exe",
        _REPO / "build/recovery-fixture/vc6-sp5-recovered/lifecycle_recovered.exe",
    ]
    for path in candidates:
        if path.is_file():
            return path
    return None


@pytest.fixture(scope="module")
def wiz8_program():
    settings = load_settings()
    try:
        from wiz8decomp.ghidra.env import open_program

        with open_program(settings, "wiz8") as program:
            yield program
    except Exception as exc:  # noqa: BLE001
        pytest.skip(f"live wiz8 Ghidra program unavailable: {exc}")


def test_lifecycle_binary_present_or_skip() -> None:
    if _lifecycle_executable() is None:
        pytest.skip("lifecycle fixture binary not built under build/recovery-fixture/")


def test_lifecycle_shaped_classes_bind_without_legacy_path(wiz8_program) -> None:
    """When Structures exist, they must not live under /wiz8/classes."""

    if _lifecycle_executable() is None:
        pytest.skip("lifecycle fixture binary not built")

    seen = 0
    for name in _LIFECYCLE_CLASSES:
        binding = resolve_class_binding(wiz8_program, name)
        if binding["status"] in {"missing-class", "missing-structure"}:
            continue
        seen += 1
        path = str(binding.get("structure_path") or "")
        assert not path.startswith("/wiz8/classes/"), path
        assert binding["status"] in {"bound", "legacy-enriched-path"}
        # Prefer native binding; legacy path is a migration signal only.
        if binding["status"] == "bound":
            assert path
    if seen == 0:
        pytest.skip("no lifecycle-named GhidraClass bindings in live wiz8 program")
