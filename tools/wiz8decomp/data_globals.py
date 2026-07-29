"""Census global variables and their reference sites from the relocation table.

An absolute address only appears in code because the loader has to fix it up, so
the relocation table enumerates every global reference in the image exactly. That
makes the census exhaustive rather than pattern-matched, and it costs one pass.

Each reference carries more than the address. The instruction that contains it
gives the access width, which is the variable's size; whether the operand is read,
written or merely taken as an address, which separates a scalar from an array or
a structure; and the enclosing function, which says who owns the state. The
distance to the next referenced global bounds the extent of this one.

Most of the game's mutable state lives in the uninitialised tail of `.data`,
past the section's raw bytes. Those addresses are still perfectly ordinary
globals - they simply have no initialiser in the file - so they are reported with
`storage` set to `bss` rather than dropped for being unmapped.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from itertools import pairwise
from pathlib import Path
from typing import Any

from .binary.code import (
    covering_index,
    disassembler,
    function_start,
    instruction_covering,
    lookup_covering,
    relocation_sites,
    sweep_text,
)
from .binary.image import PeImage, Section
from .binary.inventory import is_first_party, representative_modules
from .config import Settings
from .eh_metadata import import_slots
from .ghidra.project import program_name
from .paths import atomic_write
from .reports.snapshots import csv_text, publish_report_snapshot

_SNAPSHOT_NAME = "globals"
_REPORT_FILES = ("globals.csv",)
_PREVIEW = 60


@dataclass
class Reference:
    site: int
    function: int | None
    target: int
    mnemonic: str
    access: str
    width: int | None


@dataclass
class Global:
    address: int
    section: str
    storage: str
    references: list[Reference] = field(default_factory=list)
    kind: str = "data"
    preview: str = ""
    extent_upper: int | None = None


def section_containing(image: PeImage, address: int) -> Section | None:
    """The section an address belongs to, including a section's BSS tail.

    ``PeImage.section_at`` deliberately stops at the file-backed bytes because it
    exists to read them. A global with no initialiser lives past that point and
    still needs to be attributed to its section.
    """
    for section in image.sections:
        if section.virtual_address <= address < section.virtual_end:
            return section
    return None


def _printable(image: PeImage, address: int, section: Section) -> str | None:
    """A string literal starting exactly at ``address``.

    Requiring the previous byte to terminate a string is what separates a real
    literal from four printable bytes that happen to sit inside a float constant
    or a jump table - both of which are common in `.rdata` and neither of which
    is a string.
    """
    if address > section.virtual_address and image.read(address - 1, 1) != b"\0":
        return None
    value = image.read_cstring(address, 256)
    if value is None or len(value) < 4:
        return None
    if any(character < " " or character > "~" for character in value):
        return None
    return value


def _classify(
    image: PeImage,
    address: int,
    section: Section,
    initialized: bool,
    import_names: dict[int, str],
) -> tuple[str, str]:
    if not initialized:
        return "data", ""
    name = import_names.get(address)
    if name is not None:
        # An import slot is a global pointer, but it is the loader's, not the
        # game's, and its reference count measures calls to a library function.
        return "import-slot", name
    text = _printable(image, address, section)
    if text is not None:
        return "string", text[:_PREVIEW]
    first = image.read_u32(address)
    if first is not None and image.is_code(first):
        return "code-pointer", ""
    return "data", ""


def _describe(instruction: Any, target: int) -> tuple[str, int | None]:
    """How an instruction uses the address it refers to.

    A direct memory operand at the address is a read or a write of the variable
    and its operand size is the variable's width. Anything else - a pushed
    literal, an immediate load, a `lea` - takes the address rather than the value,
    which is what a pointer to an array or a structure looks like.
    """
    from capstone import CS_AC_WRITE, CS_OP_MEM

    for operand in instruction.operands:
        if operand.type != CS_OP_MEM:
            continue
        if operand.mem.base or operand.mem.index:
            continue
        if (operand.mem.disp & 0xFFFFFFFF) != target:
            continue
        access = "write" if operand.access & CS_AC_WRITE else "read"
        return access, operand.size
    return "address-taken", None


def analyse_image(path: Path) -> list[Global]:
    image = PeImage(path)
    engine = disassembler()
    import_names = import_slots(path)
    text = image.text
    starts, ordered = covering_index(sweep_text(image, engine))

    globals_by_address: dict[int, Global] = {}
    for site in relocation_sites(image):
        if not (text.virtual_address <= site < text.virtual_address + text.raw_size):
            continue
        target = image.read_u32(site)
        if target is None:
            continue
        section = section_containing(image, target)
        if section is None or section.name == ".text" or not section.name.startswith("."):
            continue
        if section.name in {".reloc", ".rsrc"}:
            continue
        instruction = lookup_covering(starts, ordered, site) or instruction_covering(
            image, engine, site
        )
        if instruction is None:
            continue
        access, width = _describe(instruction, target)
        entry = globals_by_address.get(target)
        if entry is None:
            initialized = target < section.virtual_address + section.raw_size
            kind, preview = _classify(image, target, section, initialized, import_names)
            entry = Global(
                address=target,
                section=section.name,
                storage="initialized" if initialized else "bss",
                kind=kind,
                preview=preview,
            )
            globals_by_address[target] = entry
        entry.references.append(
            Reference(
                site=instruction.address,
                function=function_start(image, instruction.address),
                target=target,
                mnemonic=instruction.mnemonic,
                access=access,
                width=width,
            )
        )

    ordered_globals = sorted(globals_by_address.values(), key=lambda item: item.address)
    for current, following in pairwise(ordered_globals):
        if current.section == following.section:
            current.extent_upper = following.address
    return ordered_globals


def _hex(value: Any) -> str:
    return f"{value:08x}" if isinstance(value, int) else ""


def _snapshot_readme() -> str:
    return """# Global-variable snapshot

Every global the code refers to in the first-party Wizardry executables whose code is readable,
found through the relocation table rather than by pattern. Tracked because reproduction needs the
proprietary binaries.

The producer is `wiz8decomp.data_globals`. Normal runs write the same CSV under
`build/reports/globals/` and fail when it differs from this snapshot:

```sh
uv run wiz8 globals                  # verify against the snapshot
uv run wiz8 globals --update-snapshot
```

An absolute address appears in code only because the loader fixes it up, so the relocation table
enumerates global references exactly. A reference the table does not list is not a global reference.

`storage` separates `initialized` from `bss`. Most of the game's mutable state lives in the
uninitialised tail of `.data`, past the bytes the file actually stores; those are ordinary globals
with no initialiser, not unmapped addresses.

`widths` lists every operand size observed at the address, so a single consistent width is the
variable's size and several widths mean either a union, a structure addressed at its first field, or
an array indexed by different types. `access_kinds` separates `read` and `write` from
`address-taken`; a global that is only ever address-taken is an array or a structure rather than a
scalar.

`extent_upper` is the next referenced global in the same section and `extent_bytes` the distance to
it. That bounds the variable's size from above - nothing more, since an unreferenced neighbour leaves
the bound loose.

`kind` is `import-slot` for an import-table entry, whose reference count measures calls to a library
function rather than use of a game global; `string` for a literal starting exactly at the address,
which requires the preceding byte to terminate a string so that printable bytes inside a float
constant are not mistaken for text; `code-pointer` when the first word points into code; and `data`
otherwise. `preview` carries the first characters of a string, or the imported symbol name.

The full per-reference list is a generated report under `build/reports/globals/references.csv`
rather than a tracked artifact: it is an order of magnitude larger, and it is derived from this same
producer run.
"""


def sweep_globals(settings: Settings, *, update_snapshot: bool = False) -> dict[str, Any]:
    modules, aliases = representative_modules(settings, is_first_party)
    rows: list[dict[str, Any]] = []
    reference_rows: list[dict[str, Any]] = []
    per_program: dict[str, int] = {}
    skipped: list[str] = []

    for module in modules:
        program = program_name(module)
        path = settings.work_dir / "variants" / module["variant"] / module["relative_path"]
        if not path.is_file():
            raise RuntimeError(f"module payload is missing: {path}")
        if module.get("packed"):
            # A protected code section decodes to noise, so every access width
            # and kind would be invented.
            skipped.append(program)
            continue
        entries = analyse_image(path)
        per_program[program] = len(entries)
        for entry in entries:
            functions = {reference.function for reference in entry.references if reference.function}
            widths = sorted({reference.width for reference in entry.references if reference.width})
            kinds = sorted({reference.access for reference in entry.references})
            rows.append(
                {
                    "program": program,
                    "address": _hex(entry.address),
                    "section": entry.section,
                    "storage": entry.storage,
                    "kind": entry.kind,
                    "reference_count": len(entry.references),
                    "function_count": len(functions),
                    "access_kinds": " ".join(kinds),
                    "widths": " ".join(str(width) for width in widths),
                    "extent_upper": _hex(entry.extent_upper),
                    "extent_bytes": (
                        entry.extent_upper - entry.address if entry.extent_upper else ""
                    ),
                    "preview": entry.preview,
                }
            )
            for reference in entry.references:
                reference_rows.append(
                    {
                        "program": program,
                        "site": _hex(reference.site),
                        "function_start": _hex(reference.function),
                        "target": _hex(reference.target),
                        "mnemonic": reference.mnemonic,
                        "access": reference.access,
                        "width": reference.width or "",
                    }
                )

    rows.sort(key=lambda row: (row["program"], row["address"]))
    reference_rows.sort(key=lambda row: (row["program"], row["site"]))
    outputs = {
        "globals.csv": csv_text(
            [
                "program",
                "address",
                "section",
                "storage",
                "kind",
                "reference_count",
                "function_count",
                "access_kinds",
                "widths",
                "extent_upper",
                "extent_bytes",
                "preview",
            ],
            rows,
        )
    }

    report_dir = settings.build_dir / "reports" / _SNAPSHOT_NAME
    atomic_write(
        report_dir / "references.csv",
        csv_text(
            ["program", "site", "function_start", "target", "mnemonic", "access", "width"],
            reference_rows,
        ),
    )
    report_dir, snapshot_dir, snapshot_fresh = publish_report_snapshot(
        settings,
        name=_SNAPSHOT_NAME,
        outputs=outputs,
        snapshot_files=_REPORT_FILES,
        snapshot_readme=_snapshot_readme(),
        update_snapshot=update_snapshot,
        stale_error=(
            "global-variable report differs from the tracked snapshot; review "
            f"build/reports/{_SNAPSHOT_NAME} and rerun with --update-snapshot"
        ),
    )

    return {
        "schema": "wiz8.globals",
        "programs": per_program,
        "byte_identical_aliases": aliases,
        "programs_without_readable_code": skipped,
        "globals": len(rows),
        "references": len(reference_rows),
        "by_storage": {
            storage: sum(1 for row in rows if row["storage"] == storage)
            for storage in sorted({row["storage"] for row in rows})
        },
        "by_kind": {
            kind: sum(1 for row in rows if row["kind"] == kind)
            for kind in sorted({row["kind"] for row in rows})
        },
        "with_a_single_width": sum(1 for row in rows if len(row["widths"].split()) == 1),
        "written_somewhere": sum(1 for row in rows if "write" in row["access_kinds"]),
        "only_address_taken": sum(1 for row in rows if row["access_kinds"] == "address-taken"),
        "report": str(report_dir.relative_to(settings.repo_dir)),
        "snapshot": str(snapshot_dir.relative_to(settings.repo_dir)),
        "snapshot_fresh": snapshot_fresh,
        "snapshot_updated": update_snapshot,
    }
