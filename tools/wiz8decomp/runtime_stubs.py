"""Generate the runtime-only trap thunks that stand in for unrecovered calls.

The comparison product keeps `/FORCE:UNRESOLVED`: it must link while recovery
is incomplete, and reccmp only needs an inspectable PE. The runnable products
must not, because the linker's fallback leaves the call targeting the PE image
base, where the CPU executes the ``MZ`` header and destroys EBP/ESP before the
fault. Every unresolved callable external instead receives one generated trap
thunk here: an ordinary uniquely named cdecl function that prints the retail
identity and breaks. The exact decorated symbols are defined by a generated
COFF object as five-byte ``jmp`` thunks into that trap, so no prototype is
guessed and the caller's stack is still intact when the debugger stops.

Only objects that the completed comparison link actually used are scanned, so
objects left behind by earlier source layouts cannot invent stubs. The MAP
supplies the library/import definitions, so SGP, SurRender, zlib and the CRT
are never mistaken for missing first-party functions.

The generated C++ and manifest are disposable build artifacts. Only functions
are aliased: anything that cannot be classified as a callable function fails
with the exact symbol and requesting objects, and an unresolved spelling that
resolves to an already recovered retail address fails as an identity bug
rather than hiding behind a second body. Symbols with no retail-address
evidence are still trapped, listed explicitly in the manifest as unmapped.
"""

from __future__ import annotations

import hashlib
import json
import re
from collections import defaultdict
from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import yaml

from .binary.demangle import DemanglerMissing, demangle
from .config import Settings
from .identity_lint import _declaration_address, _declaration_lines
from .paths import atomic_write
from .unresolved import unresolved_report

COMPARISON_RESPONSE = Path("src/wiz8/CMakeFiles/WIZ8.dir/objects1.rsp")
COMPARISON_LINK_DIR = Path("src/wiz8")
GENERATED_ROOT = Path("generated/runtime-stubs")
SOURCE_INDEX = Path("build/source-index.json")

# Compiler/CRT support the runtime support object supplies directly. They are
# data, not retail entities, and the VC6 CRT startup objects are not part of
# the link.
CRT_SUPPORT = {"__except_list", "__fltused"}

MARKER = re.compile(
    r"^\s*//\s*(?P<kind>FUNCTION|LIBRARY|SYNTHETIC|TEMPLATE|STUB):\s+"
    r"WIZ8\s+(?P<address>0x[0-9a-fA-F]{6,8})\b"
)
ADDRESS_NAME = re.compile(r"^(?P<prefix>[^\s]+?)(?P<address>[0-9a-fA-F]{6,8})$")
FUNCTION_SIGNATURE = re.compile(r"([~A-Za-z_][A-Za-z0-9_:<>~]*)\s*\((?P<parameters>[^()]*)\)")


class RuntimeStubError(RuntimeError):
    """Stub generation cannot continue without hiding a source-model defect."""


@dataclass(frozen=True)
class SourceFacts:
    markers_by_address: dict[int, tuple[str, str]]
    declaration_addresses: dict[str, tuple[str, str]]

    @classmethod
    def load(cls, repo_dir: Path) -> SourceFacts:
        return cls(cls._markers(repo_dir), cls._declarations(repo_dir))

    @staticmethod
    def _markers(repo_dir: Path) -> dict[int, tuple[str, str]]:
        markers: dict[int, tuple[str, str]] = {}
        for root_name in ("src", "include"):
            root = repo_dir / root_name
            if not root.is_dir():
                continue
            for path in sorted(root.rglob("*")):
                if path.suffix.lower() not in {".c", ".cpp", ".h", ".hpp"}:
                    continue
                source = path.relative_to(repo_dir).as_posix()
                for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
                    match = MARKER.match(line)
                    if match is None:
                        continue
                    markers.setdefault(
                        int(match.group("address"), 16),
                        (match.group("kind"), source),
                    )
        return markers

    @staticmethod
    def _declarations(repo_dir: Path) -> dict[str, tuple[str, str]]:
        path = repo_dir / SOURCE_INDEX
        if not path.is_file():
            return {}
        document = json.loads(path.read_text(encoding="utf-8"))
        resolved: dict[str, tuple[str, str]] = {}
        for entry in document.get("declarations", []):
            semantic_id = entry.get("semantic_id") or ""
            if not semantic_id:
                continue
            lines = _declaration_lines(repo_dir, entry)
            if lines is None:
                continue
            address = _declaration_address(lines, entry["line"], entry["end_line"])
            if address is None:
                continue
            resolved.setdefault(semantic_id, (address, entry["source_file"]))
        return resolved


@dataclass(frozen=True)
class GhidraIndex:
    by_name: dict[str, tuple[tuple[str, int, int], ...]]

    @classmethod
    def load(cls, settings: Settings) -> GhidraIndex:
        from .ghidra.env import open_program

        by_name: dict[str, list[tuple[str, int, int]]] = defaultdict(list)
        with open_program(settings, "wiz8") as program:
            for function in program.getFunctionManager().getFunctions(True):
                name = function.getName()
                entry = function.getEntryPoint().getOffset()
                count = len(list(function.getParameters()))
                by_name[name].append((name, entry, count))
                by_name[name.split("::")[-1]].append((name, entry, count))
        return cls({name: tuple(sorted(set(values))) for name, values in by_name.items()})

    def lookup(self, qualified: str, parameter_count: int) -> int | None:
        candidates: tuple[tuple[str, int, int], ...] = self.by_name.get(qualified, ())
        if not candidates:
            candidates = self.by_name.get(qualified.split("::")[-1], ())
        if not candidates:
            return None
        if len(candidates) == 1:
            return candidates[0][1]
        narrowed = [item for item in candidates if item[2] == parameter_count]
        if len(narrowed) == 1:
            return narrowed[0][1]
        return None


@dataclass(frozen=True)
class ResolvedStub:
    symbol: str
    name: str
    address: int | None
    stub: str
    requesters: tuple[str, ...]
    identity: str
    reason: str


def linked_objects(settings: Settings) -> list[Path]:
    """The recovered objects the completed comparison link actually consumed.

    The NMake link runs in the owning component's binary directory, so
    response-file tokens are relative to it, not to the build root.
    """

    build_dir = settings.product_build_dir
    response = build_dir / COMPARISON_RESPONSE
    if not response.is_file():
        raise RuntimeStubError(
            f"the comparison link response is missing: {response}; build WIZ8 first"
        )
    objects = [
        (build_dir / COMPARISON_LINK_DIR / token).resolve()
        for token in response.read_text(encoding="utf-8").split()
        if token.endswith(".obj")
    ]
    if not objects:
        raise RuntimeStubError(f"no object files listed in {response}")
    return objects


def retail_text_range(repo_dir: Path) -> tuple[int, int]:
    from reccmp.formats import detect_image

    document = yaml.safe_load((repo_dir / "reccmp-user.yml").read_text(encoding="utf-8"))
    path = Path(document["targets"]["WIZ8"]["path"])
    if not path.is_absolute():
        path = repo_dir / path
    image = detect_image(path)
    for section in image.sections:
        if section.name == ".text":
            return section.virtual_range.start, section.virtual_range.stop
    raise RuntimeStubError(f"no .text section in the retail image: {path}")


def _function_identity(signature: str) -> tuple[str, str, int] | None:
    """Qualified name, unqualified name and parameter count of a function."""

    match = FUNCTION_SIGNATURE.search(signature)
    if match is None:
        return None
    qualified = match.group(1)
    parameters = match.group("parameters").strip()
    count = 0 if parameters in ("", "void") else parameters.count(",") + 1
    return qualified, qualified.split("::")[-1], count


def _is_constructor_or_destructor(qualified: str) -> bool:
    if "::" not in qualified:
        return False
    owner, _, method = qualified.rpartition("::")
    return method == owner.rsplit("::", 1)[-1] or method.startswith("~")


def _address_from_name(unqualified: str) -> int | None:
    match = ADDRESS_NAME.match(unqualified)
    if match is None or not match.group("prefix"):
        return None
    return int(match.group("address"), 16)


def _stub_name(symbol: str, address: int | None) -> str:
    if address is None:
        return f"wiz8_runtime_stub_unmapped_{hashlib.sha1(symbol.encode()).hexdigest()[:8]}"
    return f"wiz8_runtime_stub_{address:08x}"


def _identity_error(
    address: int, symbol: str, kind: str, source: str, requesters: Iterable[str]
) -> str:
    requesters = "\n  ".join(sorted(requesters))
    if kind == "FUNCTION":
        return (
            "runtime unresolved symbol maps to already recovered address\n\n"
            f"retail:     {address:08x}\n"
            f"unresolved: {symbol}\n"
            f"recovered:  {source} [FUNCTION]\n"
            f"requesters:\n  {requesters}"
        )
    return (
        "runtime unresolved symbol maps to an existing source-model entity\n\n"
        f"retail:     {address:08x}\n"
        f"unresolved: {symbol}\n"
        f"existing:   {source} [{kind}]\n"
        f"requesters:\n  {requesters}"
    )


def resolve_stubs(
    settings: Settings,
    *,
    object_root: Path,
    map_path: Path,
    objects: list[Path],
    facts: SourceFacts | None = None,
    text_range: tuple[int, int] | None = None,
    ghidra: GhidraIndex | None = None,
) -> list[ResolvedStub]:
    repo_dir = settings.repo_dir
    facts = facts if facts is not None else SourceFacts.load(repo_dir)
    text_range = text_range if text_range is not None else retail_text_range(repo_dir)
    report = unresolved_report(object_root, map_path, objects=objects)
    by_symbol: dict[str, list[str]] = report["by_symbol"]
    decorated = sorted(by_symbol)
    try:
        demangled = demangle(decorated)
    except (DemanglerMissing, RuntimeError) as error:
        raise RuntimeStubError(str(error)) from error

    pending: dict[str, tuple[str, str, int, tuple[str, ...]]] = {}
    for symbol in decorated:
        if symbol in CRT_SUPPORT:
            continue
        requesters = tuple(sorted(by_symbol[symbol]))
        if not symbol.startswith("?"):
            raise RuntimeStubError(
                "unresolved symbol cannot be classified as a callable first-party "
                f"function (data or unmapped C symbol): {symbol}\n"
                f"requested by:\n  {', '.join(requesters)}"
            )
        signature = demangled.get(symbol, "")
        parsed = _function_identity(signature)
        if parsed is None:
            raise RuntimeStubError(
                "unresolved symbol is not a function signature (data, RTTI or "
                f"vtable storage): {symbol}\n"
                f"demangled: {signature or '<none>'}\n"
                f"requested by:\n  {', '.join(requesters)}"
            )
        qualified, unqualified, count = parsed
        if _is_constructor_or_destructor(qualified):
            unqualified = qualified
        pending[symbol] = (qualified, unqualified, count, requesters)

    decisions: dict[str, tuple[int | None, str, str]] = {}
    lookup: list[str] = []
    for symbol, (qualified, unqualified, _count, _requesters) in pending.items():
        declared = facts.declaration_addresses.get(symbol)
        if declared is not None and declared[0]:
            decisions[symbol] = (int(declared[0], 16), "declaration", declared[1])
            continue
        candidate = (
            None if _is_constructor_or_destructor(qualified) else _address_from_name(unqualified)
        )
        if candidate is not None and text_range[0] <= candidate < text_range[1]:
            decisions[symbol] = (candidate, "address-name", "")
            continue
        lookup.append(symbol)

    if lookup and ghidra is None:
        ghidra = GhidraIndex.load(settings)
    for symbol in lookup:
        qualified, _unqualified, count, _requesters = pending[symbol]
        candidate = ghidra.lookup(qualified, count) if ghidra is not None else None
        decisions[symbol] = (
            candidate,
            "ghidra" if candidate is not None else "unmapped",
            "",
        )

    stubs: list[ResolvedStub] = []
    for symbol in sorted(pending):
        qualified, unqualified, _count, requesters = pending[symbol]
        address, identity, _source = decisions[symbol]
        if address is not None:
            marker = facts.markers_by_address.get(address)
            if marker is not None:
                kind, marker_source = marker
                raise RuntimeStubError(
                    _identity_error(address, symbol, kind, marker_source, requesters)
                )
        stubs.append(
            ResolvedStub(
                symbol=symbol,
                name=unqualified,
                address=address,
                stub=_stub_name(symbol, address),
                requesters=requesters,
                identity=identity,
                reason="" if address is not None else "no retail address evidence",
            )
        )
    return _dedupe_names(stubs)


def _dedupe_names(stubs: list[ResolvedStub]) -> list[ResolvedStub]:
    """Give every overloaded symbol a distinct, content-stable stub name."""

    counts: dict[str, int] = defaultdict(int)
    for stub in stubs:
        counts[stub.stub] += 1
    output: list[ResolvedStub] = []
    for stub in stubs:
        if counts[stub.stub] == 1:
            output.append(stub)
            continue
        suffix = hashlib.sha1(stub.symbol.encode()).hexdigest()[:8]
        output.append(
            ResolvedStub(
                symbol=stub.symbol,
                name=stub.name,
                address=stub.address,
                stub=f"{stub.stub}_{suffix}",
                requesters=stub.requesters,
                identity=stub.identity,
                reason=stub.reason,
            )
        )
    return output


def render_source(stubs: list[ResolvedStub]) -> str:
    lines = [
        "// AUTOGENERATED. DO NOT EDIT.",
        "",
        '#include "wiz8/runtime_unrecovered.h"',
        "",
    ]
    for stub in stubs:
        lines.extend(
            (
                f"static const Wiz8UnrecoveredFunction record_{stub.stub} = {{",
                f"    0x{stub.address:08x}," if stub.address is not None else "    0x0,",
                f'    "{stub.symbol}",',
                f'    "{stub.name}"',
                "};",
                "",
            )
        )
        if stub.address is not None:
            lines.append(f"// STUB: WIZ8 0x{stub.address:08X}")
        lines.extend(
            (
                f'extern "C" void __cdecl {stub.stub}(void)',
                "{",
                f"    Wiz8UnrecoveredFunctionTrap(&record_{stub.stub});",
                "    for (;;) {}",
                "}",
                "",
            )
        )
    return "\n".join(lines)


def render_alias_object(stubs: list[ResolvedStub], *, include_crt_support: bool = True) -> bytes:
    """One COFF object defining every unresolved symbol at a five-byte thunk.

    VC6's LINK has no ``/alternatename``, so the exact decorated symbols cannot
    be aliased with a pragma. A generated COFF object is the equivalent: each
    symbol is defined as ``jmp _wiz8_runtime_stub_...``, which enters the
    generated C++ trap with the caller's stack untouched. The runtime products
    also need the CRT's absolute ``__except_list`` zero; probe fixtures that
    link a full CRT pass ``include_crt_support=False``.
    """

    import struct

    text = bytearray()
    relocations: list[tuple[int, int]] = []
    symbols: list[tuple[str, int, int]] = []
    for stub in stubs:
        thunk_offset = len(text)
        text.extend(b"\xe9\x00\x00\x00\x00")
        target = f"_{stub.stub}"
        target_index = len(symbols) + 1
        symbols.append((stub.symbol, thunk_offset, 1))
        symbols.append((target, 0, 0))
        relocations.append((thunk_offset + 1, target_index))
    # The CRT's dllsupp/exsup object normally provides the SEH chain head as an
    # absolute zero, which the startup's `mov eax, fs:[0]` prologue needs.
    if include_crt_support:
        symbols.append(("__except_list", 0, -1))

    section_name = b".text\x00\x00\x00"
    string_table = bytearray(b"\x00\x00\x00\x00")
    encoded_symbols = bytearray()
    for name, value, section in symbols:
        raw = name.encode("ascii")
        if len(raw) <= 8:
            encoded_symbols.extend(raw.ljust(8, b"\x00"))
        else:
            offset = len(string_table)
            string_table.extend(raw + b"\x00")
            encoded_symbols.extend(struct.pack("<II", 0, offset))
        encoded_symbols.extend(
            struct.pack("<IhHBB", value, section, 0x20 if section > 0 else 0, 2, 0)
        )
    struct.pack_into("<I", string_table, 0, len(string_table))
    header_size = 20 + 40
    raw_pointer = header_size
    relocation_pointer = raw_pointer + len(text)
    symbol_pointer = relocation_pointer + 10 * len(relocations)
    file_header = struct.pack(
        "<HHIIIHH",
        0x014C,
        1,
        0,
        symbol_pointer,
        len(symbols),
        0,
        0,
    )
    section_header = struct.pack(
        "<8sIIIIIIHHI",
        section_name,
        len(text),
        0,
        len(text),
        raw_pointer,
        relocation_pointer,
        0,
        len(relocations),
        0,
        0x60500020,
    )
    body = bytearray()
    for offset, symbol_index in relocations:
        body.extend(struct.pack("<IIH", offset, symbol_index, 0x0014))
    return (
        bytes(file_header)
        + section_header
        + bytes(text)
        + bytes(body)
        + bytes(encoded_symbols)
        + bytes(string_table)
    )


def render_manifest(stubs: list[ResolvedStub]) -> dict[str, Any]:
    ordered = sorted(stubs, key=lambda stub: (stub.address is None, stub.address or 0, stub.symbol))
    entries: list[dict[str, Any]] = []
    for stub in ordered:
        entry: dict[str, Any] = {
            "address": f"{stub.address:08x}" if stub.address is not None else "",
            "symbol": stub.symbol,
            "stub": f"_{stub.stub}",
            "name": stub.name,
            "requesters": list(stub.requesters),
        }
        if stub.address is None:
            entry["unmapped"] = stub.reason
        entries.append(entry)
    return {"schema": "wiz8.runtime-stubs", "stubs": entries}


def write_runtime_stubs(settings: Settings, *, force: bool = False) -> dict[str, Any]:
    repo_dir = settings.repo_dir
    output_dir = settings.product_build_dir / GENERATED_ROOT
    output_dir.mkdir(parents=True, exist_ok=True)
    stubs = resolve_stubs(
        settings,
        object_root=settings.recovered_objects_dir,
        map_path=settings.product_build_dir / "Wiz8.map",
        objects=linked_objects(settings),
    )
    source = render_source(stubs)
    alias_object = render_alias_object(stubs)
    manifest = render_manifest(stubs)
    source_path = output_dir / "runtime_stubs.cpp"
    object_path = output_dir / "runtime_stubs.obj"
    manifest_path = output_dir / "runtime_stubs.json"
    source_changed = force or not source_path.is_file() or source_path.read_text() != source
    object_changed = force or not object_path.is_file() or object_path.read_bytes() != alias_object
    manifest_text = json.dumps(manifest, indent=1) + "\n"
    manifest_changed = (
        force
        or not manifest_path.is_file()
        or manifest_path.read_text(encoding="utf-8") != manifest_text
    )
    if source_changed:
        atomic_write(source_path, source)
    if object_changed:
        atomic_write(object_path, alias_object)
    if manifest_changed:
        atomic_write(manifest_path, manifest_text)
    return {
        "source": str(source_path.relative_to(repo_dir)),
        "object": str(object_path.relative_to(repo_dir)),
        "manifest": str(manifest_path.relative_to(repo_dir)),
        "stubs": len(stubs),
        "annotated": sum(stub.address is not None for stub in stubs),
        "unmapped": sum(stub.address is None for stub in stubs),
        "source_changed": source_changed,
        "object_changed": object_changed,
        "manifest_changed": manifest_changed,
    }
