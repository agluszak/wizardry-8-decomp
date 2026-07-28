"""Apply the reconstructed build's debug information to an overlay clone.

The reviewed program is rebuilt from the tracked ledger, so nothing written
directly into it survives - and the build directory is not tracked, because it
needs proprietary inputs. Both facts point the same way: this applies to the
disposable clone, and the durable half of the same transfer is the proposed
`signatures.csv` rows `wiz8 reconstructed-transfer` writes for review.

What is applied is deliberately narrow:

* **Signatures**, when every type in one resolves against the program's own
  type manager. A partly resolvable signature contributes its calling
  convention only - a parameter list half filled with `undefined4` reads as
  recovered knowledge and is not.
* **Frame variable names**, and only where Ghidra already has a variable at
  the matching frame offset. The origin those offsets are measured from
  differs between bodies that keep a frame pointer and bodies `/O2` made
  frameless, so it is read off each body's own first parameter; a body that
  offers no evidence of its origin keeps its decompiler names rather than
  acquiring plausible wrong ones.
* **Provenance**, as the `wiz8.reconstructed` property map: which object file
  the body came from and at which tier it arrived.

No function is renamed. The reviewed model decides names; this decides types.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from ..config import Settings
from ..reconstructed import OVERLAY_TIER, REVIEWED_TIER, TransferPlan, frame_origin
from .project import resolve_program_name

PROPERTY_MAP = "wiz8.reconstructed"

# Ghidra measures a stack frame from the return address, so the first incoming
# parameter sits at +4; the build's own origin is read per body, because `/O2`
# omits the frame pointer for some and keeps it for others.
GHIDRA_FIRST_PARAMETER = 4


def apply_transfers(
    settings: Settings, selector: str, hypothesis: str, plan: TransferPlan
) -> dict[str, Any]:
    """Write one transfer plan into the named overlay clone."""

    from .cache import materialize_program
    from .environment import start_pyghidra
    from .overlay import _overlay_settings, _scratch_dir

    effective, _ = materialize_program(settings, selector)
    overlay = _overlay_settings(effective, hypothesis)
    if not _scratch_dir(effective, hypothesis).exists():
        raise ValueError(f"overlay does not exist; create it first: {hypothesis}")

    start_pyghidra(settings)
    import pyghidra
    from ghidra.app.cmd.function import ApplyFunctionSignatureCmd
    from ghidra.program.model.data import CategoryPath
    from ghidra.program.model.symbol import SourceType

    from .apply_unzip_model import _function_type
    from .type_specs import resolve_type_spec

    program_name = resolve_program_name(overlay, selector)
    project = pyghidra.open_project(overlay.project_dir, overlay.project_name, create=False)
    stats: dict[str, Any] = {
        "considered": 0,
        "no_function": 0,
        "signatures": 0,
        "convention_only": 0,
        "renamed_variables": 0,
        "typed_variables": 0,
        "unmatched_frame_variables": 0,
        "unknown_frame_origin": 0,
        "tiers": {REVIEWED_TIER: 0, OVERLAY_TIER: 0},
    }
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("reconstructed build transfer")
            try:
                functions = program.getFunctionManager()
                space = program.getAddressFactory().getDefaultAddressSpace()
                properties = program.getUsrPropertyManager()
                provenance = properties.getStringPropertyMap(PROPERTY_MAP)
                if provenance is None:
                    provenance = properties.createStringPropertyMap(PROPERTY_MAP)

                for transfer in plan.transfers:
                    if transfer.blocked or transfer.signature is None:
                        continue
                    stats["considered"] += 1
                    address = space.getAddress(transfer.address)
                    function = functions.getFunctionAt(address)
                    if function is None:
                        stats["no_function"] += 1
                        continue
                    stats["tiers"][transfer.tier] += 1
                    provenance.add(
                        address, f"{transfer.tier}|{transfer.object_file}|{transfer.symbol}"
                    )

                    # Resolve every component through the shared exact-path
                    # TypeSpec system. The whole signature applies or none of
                    # it does; a partly typed list would look reviewed later.
                    signature = transfer.signature
                    definition = None
                    try:
                        definition = _function_type(
                            program.getDataTypeManager(),
                            CategoryPath("/wiz8/overlay/reconstructed"),
                            f"reconstructed_signature_{transfer.address}",
                            resolve_type_spec(
                                program.getDataTypeManager(), signature.return_type, "wiz8"
                            ),
                            [
                                (
                                    f"argument{index + 1}",
                                    resolve_type_spec(
                                        program.getDataTypeManager(), type_name, "wiz8"
                                    ),
                                )
                                for index, type_name in enumerate(signature.parameters)
                            ],
                            signature.convention,
                        )
                    except (KeyError, ValueError):
                        definition = None
                    if definition is not None:
                        ApplyFunctionSignatureCmd(address, definition, SourceType.ANALYSIS).applyTo(
                            program
                        )
                        stats["signatures"] += 1
                    else:
                        stats["convention_only"] += 1
                    if function.getCallingConventionName() != signature.convention:
                        function.setCallingConvention(signature.convention)

                    origin = frame_origin(transfer.frame_variables)
                    if origin is None:
                        stats["unknown_frame_origin"] += 1
                        continue
                    bias = GHIDRA_FIRST_PARAMETER - origin
                    by_offset = {
                        variable.getStackOffset(): variable
                        for variable in function.getStackFrame().getStackVariables()
                    }
                    for name, frame_offset, type_name in transfer.frame_variables:
                        variable = by_offset.get(frame_offset + bias)
                        if variable is None:
                            stats["unmatched_frame_variables"] += 1
                            continue
                        variable.setName(name, SourceType.ANALYSIS)
                        stats["renamed_variables"] += 1
                        try:
                            data_type = resolve_type_spec(
                                program.getDataTypeManager(), type_name, "wiz8"
                            )
                        except (KeyError, ValueError):
                            continue
                        variable.setDataType(data_type, SourceType.ANALYSIS)
                        stats["typed_variables"] += 1
                program.endTransaction(transaction, True)
            except Exception:
                program.endTransaction(transaction, False)
                raise
            program.save("reconstructed build transfer", None)
    finally:
        project.close()
    return stats


def transfer_into_overlay(
    settings: Settings,
    selector: str,
    hypothesis: str,
    build_dir: Path | None = None,
    *,
    addresses: set[str] | None = None,
) -> dict[str, Any]:
    """Read the build's debug information and apply it, in one step."""

    from ..reconstructed import (
        bodies_from_objects,
        build_transfer_plan,
        verified_boundary_addresses,
    )

    root = build_dir or settings.build_dir
    verified = verified_boundary_addresses(settings.repo_dir, root)
    plan = build_transfer_plan(
        settings.repo_dir, bodies_from_objects(root), verified_exact=verified
    )
    if addresses is not None:
        wanted = {value.lower().removeprefix("0x").zfill(8) for value in addresses}
        plan.transfers = [
            transfer for transfer in plan.transfers if transfer.address.lower().zfill(8) in wanted
        ]
        plan.unmatched = [
            item for item in plan.unmatched if item["address"].lower().zfill(8) in wanted
        ]
    stats = apply_transfers(settings, selector, hypothesis, plan)
    stats["plan"] = plan.summary()
    return stats
