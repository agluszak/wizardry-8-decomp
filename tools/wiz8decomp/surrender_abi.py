"""Decode the SurRender export tables into an original-ABI class surface.

`sr.dll` exports far more than Wizardry imports, and every exported name is a
decorated MSVC symbol: it states the class, the member, whether the member is
virtual, static or an adjustor thunk, and - for an exported vftable - which base
subobject that vftable belongs to. That is declaration-side evidence about a
library the game links, which the evidence policy explicitly prefers over
recovering the library's own bodies, so nothing here disassembles SurRender.

Signatures come from `llvm-undname`, a maintained implementation of the MSVC
grammar, rather than from anything written here. The local parser only extracts
the structural facts a signature string does not expose as fields - kind, access,
virtuality, adjustor-thunk status and calling convention - and the two are
cross-checked: `wiz8 surrender-abi` reports any row whose structural class name
is absent from the demangled text. Anything the local parser cannot decode is
recorded with ``parse_status`` set rather than guessed at.

Where the two overlap the demangler wins. A vftable's base subobject is read from
the demangled signature because the decorated form uses back-references
(``??_7X@@6B0@@`` names ``X`` itself, not a base called ``0``) and unexpanded
template arguments.
"""

from __future__ import annotations

import csv
import io
import re
from dataclasses import dataclass
from typing import Any

from .binary.code import relocation_sites
from .binary.demangle import demangle, tool_version
from .binary.image import PeImage
from .binary.inventory import load_inventory
from .config import Settings
from .ghidra.project import program_name
from .paths import atomic_write

_SNAPSHOT_NAME = "surrender-abi"
_REPORT_FILES = ("exports.csv", "vftable-slots.csv", "vbtable-entries.csv")
_MODULE_PREFIX = "sr"
# A vbtable entry is a displacement inside one object, so a value this large is
# not one and the run has ended.
_MAXIMUM_SUBOBJECT_OFFSET = 0x100000

# Leading codes for MSVC special names. Only the ones a C++ library actually
# exports are listed; anything else is reported as an unhandled special name.
_SPECIAL_NAMES = {
    "0": ("constructor", True),
    "1": ("destructor", True),
    "2": ("operator-new", False),
    "3": ("operator-delete", False),
    "4": ("operator-assign", False),
    "5": ("operator-rshift", False),
    "6": ("operator-lshift", False),
    "7": ("operator-not", False),
    "8": ("operator-equals", False),
    "9": ("operator-not-equals", False),
    "A": ("operator-index", False),
    "B": ("operator-cast", False),
    "C": ("operator-arrow", False),
    "D": ("operator-star", False),
    "E": ("operator-increment", False),
    "F": ("operator-decrement", False),
    "G": ("operator-minus", False),
    "H": ("operator-plus", False),
    "I": ("operator-address-of", False),
    "K": ("operator-divide", False),
    "M": ("operator-less", False),
    "N": ("operator-less-equal", False),
    "O": ("operator-greater", False),
    "P": ("operator-greater-equal", False),
    "U": ("operator-call", False),
    "_0": ("operator-divide-assign", False),
    "_4": ("operator-and-assign", False),
    "_5": ("operator-or-assign", False),
    "_7": ("vftable", True),
    "_8": ("vbtable", True),
    "_9": ("vcall", True),
    "_A": ("typeof", True),
    "_B": ("local-static-guard", False),
    "_C": ("string-literal", False),
    "_D": ("vbase-destructor", True),
    "_E": ("vector-deleting-destructor", True),
    "_F": ("default-constructor-closure", True),
    "_G": ("scalar-deleting-destructor", True),
    "_H": ("vector-constructor-iterator", True),
    "_I": ("vector-destructor-iterator", True),
    "_R0": ("rtti-type-descriptor", True),
    "_R1": ("rtti-base-class-descriptor", True),
    "_R2": ("rtti-base-class-array", True),
    "_R3": ("rtti-class-hierarchy-descriptor", True),
    "_R4": ("rtti-complete-object-locator", True),
}

# The storage class letter that follows a qualified name. It carries the access
# level, whether the member is virtual, and whether it is a compiler-generated
# adjustor thunk for a secondary base.
_STORAGE = {
    "A": ("private", "non-virtual", False),
    "B": ("private", "non-virtual", False),
    "C": ("private", "static", False),
    "D": ("private", "static", False),
    "E": ("private", "virtual", False),
    "F": ("private", "virtual", False),
    "G": ("private", "virtual", True),
    "H": ("private", "virtual", True),
    "I": ("protected", "non-virtual", False),
    "J": ("protected", "non-virtual", False),
    "K": ("protected", "static", False),
    "L": ("protected", "static", False),
    "M": ("protected", "virtual", False),
    "N": ("protected", "virtual", False),
    "O": ("protected", "virtual", True),
    "P": ("protected", "virtual", True),
    "Q": ("public", "non-virtual", False),
    "R": ("public", "non-virtual", False),
    "S": ("public", "static", False),
    "T": ("public", "static", False),
    "U": ("public", "virtual", False),
    "V": ("public", "virtual", False),
    "W": ("public", "virtual", True),
    "X": ("public", "virtual", True),
    "Y": ("", "free-function", False),
    "Z": ("", "free-function", False),
}

# Data members use digits where functions use letters.
_DATA_STORAGE = {
    "0": ("private", "static-data"),
    "1": ("protected", "static-data"),
    "2": ("public", "static-data"),
    "3": ("", "global-data"),
    "4": ("", "static-local-data"),
}

_CALLING_CONVENTIONS = {
    "A": "__cdecl",
    "B": "__cdecl",
    "C": "__pascal",
    "E": "__thiscall",
    "G": "__stdcall",
    "I": "__fastcall",
}

_VFTABLE_BASE_RE = re.compile(r"@@[67]B(.*)@$")
# ``const X::`vftable'{for `Base'}`` - the brace clause is absent on a primary.
_DEMANGLED_BASE_RE = re.compile(r"\{for `(.+)'\}\s*$")


def vftable_base_from_signature(signature: str) -> str:
    """The base subobject a demangled vftable/vbtable signature belongs to.

    Preferred over reading the decorated name directly, which would have to
    resolve MSVC back-references (``@@6B0@@`` names the class itself, not a base
    called ``0``) and re-expand template arguments to say anything useful.
    """
    match = _DEMANGLED_BASE_RE.search(signature)
    return match.group(1) if match else ""


@dataclass
class ParsedName:
    kind: str
    class_name: str = ""
    enclosing_scope: str = ""
    member_name: str = ""
    access: str = ""
    virtuality: str = ""
    adjustor_thunk: bool = False
    calling_convention: str = ""
    vftable_base: str = ""
    template: str = ""
    parse_status: str = "ok"


def _template_name(fragment: str) -> str:
    """Name of a ``?$Template@args`` fragment, without its arguments."""
    body = fragment[2:]
    end = body.find("@")
    return body[:end] if end >= 0 else body


def _strip_scope_terminator(value: str) -> str:
    return value.removesuffix("@@")


def parse_decorated_name(name: str) -> ParsedName:
    """Decode the structural prefix of an MSVC decorated name.

    Returns what the symbol *is* - kind, owning class, access, calling
    convention - and leaves the parameter grammar alone.
    """
    if not name.startswith("?"):
        return ParsedName(kind="undecorated", member_name=name)

    body = name[1:]
    kind = "method"
    name_is_class = False
    if body.startswith("?"):
        body = body[1:]
        code = ""
        for candidate in (body[:3], body[:2], body[:1]):
            if candidate in _SPECIAL_NAMES:
                code = candidate
                break
        if not code:
            return ParsedName(kind="special-unhandled", parse_status="unknown-special-name")
        kind, name_is_class = _SPECIAL_NAMES[code]
        body = body[len(code) :]

    if kind == "string-literal":
        # An encoded literal carries no scope and no type; the kind is the whole
        # fact, so there is nothing further to decode.
        return ParsedName(kind=kind)

    if body.startswith("@"):
        # A global operator has an empty qualified name, terminated by a single
        # '@'. Splitting on '@@' here would land inside a template argument.
        qualified, encoding = "", body[1:]
    else:
        separator = body.find("@@")
        if separator < 0:
            return ParsedName(kind=kind, parse_status="no-scope-terminator")
        qualified, encoding = body[:separator], body[separator + 2 :]

    parsed = ParsedName(kind=kind)
    if "?$" in qualified:
        # A templated owning class puts '@' inside its own argument list, so the
        # first '@@' is not reliably the end of the qualified name.
        parsed.parse_status = "template-scope"
        parsed.template = _template_name(qualified[qualified.find("?$") :])

    fragments = [fragment for fragment in qualified.split("@") if fragment]
    if fragments:
        if name_is_class:
            parsed.class_name = fragments[0]
            parsed.enclosing_scope = "::".join(reversed(fragments[1:]))
        else:
            parsed.member_name = fragments[0]
            parsed.class_name = fragments[1] if len(fragments) > 1 else ""
            parsed.enclosing_scope = "::".join(reversed(fragments[2:]))

    if kind == "vftable" or kind == "vbtable":
        match = _VFTABLE_BASE_RE.search(name)
        if match:
            base = _strip_scope_terminator(match.group(1))
            if base.startswith("?$"):
                parsed.vftable_base = _template_name(base)
                parsed.template = parsed.template or parsed.vftable_base
            else:
                # A base path is innermost-first, same as any qualified name.
                parsed.vftable_base = "::".join(reversed([part for part in base.split("@") if part]))
        return parsed

    if encoding:
        data_storage = _DATA_STORAGE.get(encoding[0])
        if data_storage is not None:
            parsed.access, parsed.virtuality = data_storage
            return parsed
        storage = _STORAGE.get(encoding[0])
        if storage is None:
            parsed.parse_status = "unknown-storage-class"
            return parsed
        parsed.access, parsed.virtuality, parsed.adjustor_thunk = storage
        # Non-static members carry a cv-qualifier before the calling convention;
        # statics and free functions do not.
        offset = 1 if parsed.virtuality in {"static", "free-function"} else 2
        if len(encoding) > offset:
            parsed.calling_convention = _CALLING_CONVENTIONS.get(encoding[offset], "")
    return parsed


@dataclass(frozen=True)
class VftableSlot:
    index: int
    target_rva: int
    resolution: str


def decode_vftable(
    image: PeImage,
    relocated: set[int],
    rva: int,
    boundaries: set[int],
    limit: int = 512,
) -> list[VftableSlot]:
    """The slots of the exported vftable at `rva`, in order.

    A vftable export names a data address, so its slots are read rather than
    disassembled. Two independent facts bound the run: every slot holds an
    absolute address the loader fixes up, so it appears in the relocation
    directory, and every slot points into an executable section. The first
    address that fails either test is past the end. Another exported symbol
    beginning mid-run ends it too, since two symbols cannot share a byte.
    """

    slots: list[VftableSlot] = []
    cursor = rva
    while len(slots) < limit:
        address = image.image_base + cursor
        if cursor != rva and cursor in boundaries:
            break
        if address not in relocated:
            break
        target = image.read_u32(address)
        if target is None:
            break
        section = image.section_at(target)
        if section is None or not section.executable:
            break
        slots.append(
            VftableSlot(
                index=len(slots),
                target_rva=target - image.image_base,
                resolution="",
            )
        )
        cursor += 4
    return slots


def decode_vbtable(
    image: PeImage,
    relocated: set[int],
    rva: int,
    boundaries: set[int],
    limit: int = 32,
) -> list[int]:
    """The displacements of the exported vbtable at `rva`.

    A vbtable holds offsets rather than addresses, so - unlike a vftable - none
    of its entries is relocated, and that is what terminates the run: the first
    relocated slot belongs to whatever data follows. Entry zero is the offset
    from the vbptr back to the vbtable itself and the rest locate each virtual
    base, so all of them are small signed displacements within one object.
    """

    entries: list[int] = []
    cursor = rva
    while len(entries) < limit:
        address = image.image_base + cursor
        if cursor != rva and (cursor in boundaries or address in relocated):
            break
        value = image.read_i32(address)
        if value is None or abs(value) > _MAXIMUM_SUBOBJECT_OFFSET:
            break
        # After entry zero a displacement of zero would put a virtual base on
        # top of the vbptr itself, so it is padding rather than an entry.
        if entries and value == 0:
            break
        entries.append(value)
        cursor += 4
    return entries


def _csv_text(fields: list[str], rows: list[dict[str, Any]]) -> str:
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return stream.getvalue()


def _snapshot_readme() -> str:
    return """# SurRender export-ABI snapshot

Every exported symbol of every SurRender module in the corpus, with its decorated name decoded
into structural facts. Tracked because reproduction needs the proprietary binaries.

The producer is `wiz8decomp.surrender_abi`. Normal runs write the same CSV under
`build/reports/surrender-abi/` and fail when it differs from this snapshot:

```sh
uv run wiz8 surrender-abi                  # verify against the snapshot
uv run wiz8 surrender-abi --update-snapshot
```

This is declaration evidence about linked library code, not recovery of it: the rows come from
export tables, never from disassembling a SurRender body.

`kind` distinguishes constructors, destructors, vftables, vbtables and ordinary members.
`virtuality` separates `virtual` from `non-virtual` and `static`, and `adjustor_thunk` marks the
compiler-generated entries that exist only to shift `this` onto a secondary base - so the columns
together describe the polymorphism ABI without reading a single instruction.

`vftable_base` is the base subobject an exported vftable belongs to, taken from the demangled
signature rather than the decorated name, which uses back-references and unexpanded templates.
An empty value on a `vftable` row means the primary vftable; a non-empty value names the base, which
makes the inheritance edges of the library explicit.

`parse_status` is `ok` only when the whole structural prefix decoded. `template-scope` marks names
whose owning class is itself a template, where the scope chain is reported best-effort.

`vftable-slots.csv` and `vbtable-entries.csv` read the tables `exports.csv` names. A vftable export
is a data address, so its slots are read out of the module rather than disassembled: each slot holds
an absolute address the loader fixes up, which puts it in the relocation directory, and each points
into an executable section. The first address failing either test, or the start of another exported
symbol, ends the run. `resolution` is `exported` when the slot's target is itself an exported
symbol, whose name and signature then fill `target_name` and `target_signature`, and `internal`
when it is a method the library does not export - recorded as unresolved rather than guessed.

A vbtable holds displacements rather than addresses, so none of its entries is relocated and the
first relocated slot ends that run instead. Entry zero is the offset from the vbptr back to the
vbtable; the rest locate each virtual base within the object, which is what a derived declaration
needs in order to inherit virtually and still match.

`subobject` on either table is the base a secondary table belongs to, empty for a primary.

Modules with byte-identical payloads across variants are recorded once, under the canonical
variant; `wiz8 surrender-abi` reports the aliases it collapsed.
"""


def _representative_modules(settings: Settings) -> tuple[list[dict[str, Any]], dict[str, str]]:
    import yaml

    modules = [
        module
        for module in load_inventory(settings)["modules"]
        if module["module_name"].casefold().startswith(_MODULE_PREFIX) and module["exports"]
    ]
    if not modules:
        raise RuntimeError("no SurRender modules in the inventory; run 'wiz8 inventory' first")
    canonical = yaml.safe_load(
        (settings.repo_dir / "config" / "variants.yml").read_text(encoding="utf-8")
    )["canonical_matching_target"]["variant"]

    groups: dict[str, list[dict[str, Any]]] = {}
    for module in modules:
        groups.setdefault(module["sha256"], []).append(module)
    chosen: list[dict[str, Any]] = []
    aliases: dict[str, str] = {}
    for members in groups.values():
        members.sort(key=lambda item: (item["variant"] != canonical, item["variant"], item["relative_path"]))
        chosen.append(members[0])
        for other in members[1:]:
            aliases[program_name(other)] = program_name(members[0])
    chosen.sort(key=lambda item: (item["variant"], item["relative_path"]))
    return chosen, dict(sorted(aliases.items()))


def _decode_module_tables(
    settings: Settings,
    module: dict[str, Any],
    program: str,
    tables: list[tuple[dict[str, Any], ParsedName]],
    signatures: dict[str, str],
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Read every exported vftable and vbtable of one module out of its data."""

    path = settings.work_dir / "variants" / module["variant"] / module["relative_path"]
    image = PeImage(path)
    relocated = set(relocation_sites(image))
    boundaries = {int(symbol["rva"], 16) for symbol in module["exports"]}
    # Only exported targets can be named. Everything else is an internal method,
    # which is recorded as unresolved rather than guessed at.
    by_rva: dict[int, dict[str, Any]] = {}
    for symbol in module["exports"]:
        if symbol["name"]:
            by_rva.setdefault(int(symbol["rva"], 16), symbol)

    slots: list[dict[str, Any]] = []
    entries: list[dict[str, Any]] = []
    for symbol, parsed in tables:
        rva = int(symbol["rva"], 16)
        signature = signatures.get(symbol["name"], "")
        base = vftable_base_from_signature(signature) if signature else ""
        common = {
            "program": program,
            "module": module["relative_path"],
            "table": symbol["name"],
            "class_name": parsed.class_name,
            "enclosing_scope": parsed.enclosing_scope,
            "subobject": base,
            "table_rva": f"0x{rva:x}",
        }
        if parsed.kind == "vftable":
            for slot in decode_vftable(image, relocated, rva, boundaries):
                target = by_rva.get(slot.target_rva)
                slots.append(
                    {
                        **common,
                        "slot": slot.index,
                        "target_rva": f"0x{slot.target_rva:x}",
                        "target_name": target["name"] if target else "",
                        "target_signature": (
                            signatures.get(target["name"], "") if target else ""
                        ),
                        "resolution": "exported" if target else "internal",
                    }
                )
        else:
            for index, displacement in enumerate(
                decode_vbtable(image, relocated, rva, boundaries)
            ):
                entries.append({**common, "entry": index, "displacement": displacement})
    return slots, entries


def sweep_surrender_abi(settings: Settings, *, update_snapshot: bool = False) -> dict[str, Any]:
    modules, aliases = _representative_modules(settings)
    rows: list[dict[str, Any]] = []
    per_module: dict[str, int] = {}

    signatures = demangle(
        [
            symbol["name"]
            for module in modules
            for symbol in module["exports"]
            if symbol["name"]
        ]
    )

    slot_rows: list[dict[str, Any]] = []
    entry_rows: list[dict[str, Any]] = []

    for module in modules:
        program = program_name(module)
        per_module[program] = len(module["exports"])
        tables = [
            (symbol, parse_decorated_name(symbol["name"] or ""))
            for symbol in module["exports"]
            if symbol["name"]
        ]
        tables = [
            (symbol, parsed)
            for symbol, parsed in tables
            if parsed.kind in {"vftable", "vbtable"}
        ]
        if tables:
            slots, entries = _decode_module_tables(
                settings, module, program, tables, signatures
            )
            slot_rows.extend(slots)
            entry_rows.extend(entries)
        for symbol in module["exports"]:
            name = symbol["name"] or f"#{symbol['ordinal']}"
            parsed = parse_decorated_name(name)
            signature = signatures.get(name, "")
            if signature and parsed.kind in {"vftable", "vbtable"}:
                parsed.vftable_base = vftable_base_from_signature(signature)
            rows.append(
                {
                    "program": program,
                    "module": module["relative_path"],
                    "ordinal": symbol["ordinal"],
                    "rva": symbol["rva"],
                    "decorated_name": name,
                    "demangled_signature": signature,
                    "kind": parsed.kind,
                    "class_name": parsed.class_name,
                    "enclosing_scope": parsed.enclosing_scope,
                    "member_name": parsed.member_name,
                    "access": parsed.access,
                    "virtuality": parsed.virtuality,
                    "adjustor_thunk": "yes" if parsed.adjustor_thunk else "",
                    "calling_convention": parsed.calling_convention,
                    "vftable_base": parsed.vftable_base,
                    "template": parsed.template,
                    "parse_status": parsed.parse_status,
                }
            )

    rows.sort(key=lambda row: (row["program"], row["decorated_name"]))
    outputs = {
        "exports.csv": _csv_text(
            [
                "program",
                "module",
                "ordinal",
                "rva",
                "decorated_name",
                "demangled_signature",
                "kind",
                "class_name",
                "enclosing_scope",
                "member_name",
                "access",
                "virtuality",
                "adjustor_thunk",
                "calling_convention",
                "vftable_base",
                "template",
                "parse_status",
            ],
            rows,
        ),
        "vftable-slots.csv": _csv_text(
            [
                "program",
                "module",
                "table",
                "class_name",
                "enclosing_scope",
                "subobject",
                "table_rva",
                "slot",
                "target_rva",
                "target_name",
                "target_signature",
                "resolution",
            ],
            sorted(slot_rows, key=lambda row: (row["program"], row["table"], row["slot"])),
        ),
        "vbtable-entries.csv": _csv_text(
            [
                "program",
                "module",
                "table",
                "class_name",
                "enclosing_scope",
                "subobject",
                "table_rva",
                "entry",
                "displacement",
            ],
            sorted(entry_rows, key=lambda row: (row["program"], row["table"], row["entry"])),
        ),
    }

    report_dir = settings.build_dir / "reports" / _SNAPSHOT_NAME
    snapshot_dir = settings.repo_dir / "evidence" / "snapshots" / _SNAPSHOT_NAME
    for name, value in outputs.items():
        atomic_write(report_dir / name, value)
    if update_snapshot:
        for name, value in outputs.items():
            atomic_write(snapshot_dir / name, value)
        atomic_write(snapshot_dir / "README.md", _snapshot_readme())
    snapshot_fresh = all(
        (snapshot_dir / name).is_file() and (snapshot_dir / name).read_text(encoding="utf-8") == outputs[name]
        for name in _REPORT_FILES
    )
    if not update_snapshot and not snapshot_fresh:
        raise RuntimeError(
            "SurRender ABI report differs from the tracked snapshot; review "
            f"build/reports/{_SNAPSHOT_NAME} and rerun with --update-snapshot"
        )

    classes = {row["class_name"] for row in rows if row["class_name"]}
    vftables = [row for row in rows if row["kind"] == "vftable"]
    # The structural columns and the demangler decode the same grammar
    # independently, so any row where the demangled text does not contain the
    # class the structural parser claimed is a defect in one of them.
    disagreements = [
        row["decorated_name"]
        for row in rows
        if row["class_name"] and row["demangled_signature"] and row["class_name"] not in row["demangled_signature"]
    ]
    return {
        "schema": "wiz8.surrender-abi",
        "modules": per_module,
        "byte_identical_aliases": aliases,
        "demangler": tool_version(),
        "demangled": sum(1 for row in rows if row["demangled_signature"]),
        "undemangled": sorted(
            {row["decorated_name"] for row in rows if not row["demangled_signature"]}
        )[:10],
        "class_name_disagreements": disagreements[:10],
        "class_name_disagreement_count": len(disagreements),
        "exports": len(rows),
        "classes": len(classes),
        "vftables": len(vftables),
        "vftables_with_named_base": sum(1 for row in vftables if row["vftable_base"]),
        "vftables_decoded": len({row["table"] for row in slot_rows}),
        "vftable_slots": len(slot_rows),
        "vftable_slots_exported": sum(
            1 for row in slot_rows if row["resolution"] == "exported"
        ),
        "vftables_with_no_slots": sorted(
            {row["decorated_name"] for row in vftables}
            - {row["table"] for row in slot_rows}
        )[:10],
        "vbtables_decoded": len({row["table"] for row in entry_rows}),
        "vbtable_entries": len(entry_rows),
        "virtual_members": sum(1 for row in rows if row["virtuality"] == "virtual"),
        "adjustor_thunks": sum(1 for row in rows if row["adjustor_thunk"]),
        "constructors": sum(1 for row in rows if row["kind"] == "constructor"),
        "destructors": sum(1 for row in rows if row["kind"].endswith("destructor")),
        "unparsed": sorted({row["parse_status"] for row in rows if row["parse_status"] != "ok"}),
        "unparsed_rows": sum(1 for row in rows if row["parse_status"] != "ok"),
        "report": str(report_dir.relative_to(settings.repo_dir)),
        "snapshot": str(snapshot_dir.relative_to(settings.repo_dir)),
        "snapshot_fresh": snapshot_fresh,
        "snapshot_updated": update_snapshot,
    }
