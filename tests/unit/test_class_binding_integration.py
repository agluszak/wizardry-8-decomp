"""Integration probes for class binding on the lifecycle fixture program.

Imports a prebuilt lifecycle PE into a disposable Ghidra project. Skips when the
fixture binary or pyghidra/Ghidra is unavailable. Does not open live wiz8.
"""

from __future__ import annotations

import pytest

pytest.importorskip("pyghidra")

from wiz8decomp.class_binding import (
    ensure_function_class_namespace,
    find_ghidra_class,
    resolve_class_binding,
)
from wiz8decomp.config import load_settings
from wiz8decomp.ghidra.lifecycle_fixture import (
    find_lifecycle_fixture_executable,
    open_lifecycle_fixture_program,
)

pytestmark = pytest.mark.integration

_REQUIRED_CLASSES = ("Base", "Derived", "Secondary")
_OPTIONAL_CLASSES = ("VirtualDerived",)


@pytest.fixture(scope="module")
def lifecycle_program():
    settings = load_settings()
    if find_lifecycle_fixture_executable(settings) is None:
        pytest.skip("lifecycle fixture binary not built under build/recovery-fixture/")
    try:
        with open_lifecycle_fixture_program(settings) as program:
            yield program
    except Exception as exc:  # noqa: BLE001
        pytest.skip(f"ghidra/pyghidra lifecycle fixture unavailable: {exc}")


def test_lifecycle_classes_bind_without_legacy_path(lifecycle_program) -> None:
    """Fixture classes bind to native Structures, never ``/wiz8/classes``."""

    for name in _REQUIRED_CLASSES:
        binding = resolve_class_binding(lifecycle_program, name)
        assert binding["status"] == "bound", binding
        path = str(binding.get("structure_path") or "")
        assert path
        assert not path.startswith("/wiz8/classes/"), path

    for name in _OPTIONAL_CLASSES:
        binding = resolve_class_binding(lifecycle_program, name)
        if binding["status"] in {"missing-class", "missing-structure"}:
            continue
        path = str(binding.get("structure_path") or "")
        assert binding["status"] == "bound", binding
        assert path
        assert not path.startswith("/wiz8/classes/"), path

    ghidra_class = find_ghidra_class(lifecycle_program, "Base")
    assert ghidra_class is not None
    candidate = None
    for function in lifecycle_program.getFunctionManager().getFunctions(True):
        if function.getCallingConventionName() != "__thiscall":
            continue
        parent = function.getParentNamespace()
        if parent is not None and parent.equals(ghidra_class):
            candidate = function
            break
    if candidate is not None:
        assert ensure_function_class_namespace(candidate, ghidra_class) is False
