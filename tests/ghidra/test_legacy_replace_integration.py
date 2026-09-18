"""Live Ghidra: replaceDataType(..., False) keeps /Foo and retires /wiz8/classes/Foo."""

from __future__ import annotations

import pytest

pytest.importorskip("pyghidra")

from wiz8decomp.class_binding import ensure_ghidra_class, find_class_structure
from wiz8decomp.config import load_settings
from wiz8decomp.ghidra.lifecycle_fixture import (
    find_lifecycle_fixture_executable,
    open_lifecycle_fixture_program,
)
from wiz8decomp.legacy_classes_cleanup import apply_legacy_classes_cleanup

pytestmark = pytest.mark.integration

_NAME = "CleanupFoo"
_BOUND = "/CleanupFoo"
_LEGACY = "/wiz8/classes/CleanupFoo"
_LEGACY_PTR = "/wiz8/classes/CleanupFoo *"
_FNDEF = "/test/usesCleanupFoo"


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


def _add_foo(manager, category: str):
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
        IntegerDataType,
        StructureDataType,
    )

    if category in {"/", ""}:
        structure = StructureDataType(_NAME, 0)
    else:
        structure = StructureDataType(CategoryPath(category), _NAME, 0)
    structure.add(IntegerDataType.dataType, 4, "x", None)
    structure.add(IntegerDataType.dataType, 4, "y", None)
    return manager.addDataType(structure, DataTypeConflictHandler.REPLACE_HANDLER)


def test_replace_legacy_keeps_bound_category(lifecycle_program) -> None:
    import pyghidra
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
        FunctionDefinitionDataType,
        ParameterDefinitionImpl,
        PointerDataType,
        StructureDataType,
        VoidDataType,
    )
    from ghidra.program.model.listing import VariableUtilities  # type: ignore[import-not-found]

    program = lifecycle_program
    manager = program.getDataTypeManager()

    with pyghidra.transaction(program, "setup CleanupFoo dual identity"):
        bound = _add_foo(manager, "/")
        ghidra_class = ensure_ghidra_class(program, _NAME)
        existing = VariableUtilities.findExistingClassStruct(ghidra_class, manager)
        assert existing is not None
        assert str(existing.getPathName()) == _BOUND
        legacy = _add_foo(manager, "/wiz8/classes")
        assert str(legacy.getPathName()) == _LEGACY
        holder = StructureDataType("CleanupFooHolder", 0)
        holder.add(PointerDataType(legacy, manager), 4, "ptr", None)
        holder = manager.addDataType(holder, DataTypeConflictHandler.REPLACE_HANDLER)
        fn_def = FunctionDefinitionDataType(CategoryPath("/test"), "usesCleanupFoo")
        fn_def.setReturnType(VoidDataType.dataType)
        fn_def.setArguments(
            [ParameterDefinitionImpl("obj", PointerDataType(legacy, manager), None)]
        )
        fn_def = manager.addDataType(fn_def, DataTypeConflictHandler.REPLACE_HANDLER)

    listing_addr = None
    listing = program.getListing()
    memory = program.getMemory()
    space = program.getAddressFactory().getDefaultAddressSpace()
    with pyghidra.transaction(program, "listing uses CleanupFoo"):
        for candidate in (0x00F00000, 0x00E00000, 0x00D00000):
            start = space.getAddress(candidate)
            try:
                scratch = memory.createInitializedBlock(
                    "legacy-cleanup-scratch",
                    start,
                    16,
                    0,
                    pyghidra.task_monitor(),
                    False,
                )
            except Exception:  # noqa: BLE001,S112 — occupied image range
                continue
            listing_addr = scratch.getStart()
            break
        if listing_addr is not None:
            listing.createData(listing_addr, manager.getDataType(_LEGACY))

    assert manager.getDataType(_BOUND) is not None
    assert manager.getDataType(_LEGACY) is not None
    plan = {
        "types": [
            {
                "path": _LEGACY_PTR,
                "name": f"{_NAME} *",
                "action": "replace-then-delete",
                "bound_path": _BOUND,
            },
            {
                "path": _LEGACY,
                "name": _NAME,
                "action": "replace-then-delete",
                "bound_path": _BOUND,
            },
        ]
    }
    result = apply_legacy_classes_cleanup(program, plan)
    assert result["errors"] == [], result["errors"]
    assert manager.getDataType(_BOUND) is not None
    assert str(manager.getDataType(_BOUND).getPathName()) == _BOUND
    assert manager.getDataType(_LEGACY) is None
    assert manager.getDataType(_LEGACY_PTR) is None

    holder = manager.getDataType("/CleanupFooHolder")
    field = next(iter(holder.getDefinedComponents()))
    field_type = field.getDataType()
    pointee = field_type.getDataType() if hasattr(field_type, "getDataType") else None
    assert pointee is not None
    assert str(pointee.getPathName()) == _BOUND

    fn_def = manager.getDataType(_FNDEF)
    arg = next(iter(fn_def.getArguments())).getDataType()
    arg_pointee = arg.getDataType() if hasattr(arg, "getDataType") else None
    assert arg_pointee is not None
    assert str(arg_pointee.getPathName()) == _BOUND

    if listing_addr is not None:
        data = listing.getDataAt(listing_addr)
        assert data is not None
        assert str(data.getDataType().getPathName()) == _BOUND

    auto = find_class_structure(program, ghidra_class)
    assert auto is not None
    assert str(auto.getPathName()) == _BOUND
    after = manager.getDataType(_BOUND)
    assert after.getUniversalID() == bound.getUniversalID()
