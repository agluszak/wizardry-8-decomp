"""Read the reconstructed build's VC6 program database.

The build links with the same toolchain the original used, so its PDB carries
exactly the facts the original image lost: which object file each body came
from, where each frame variable sits and what it is called, and the return
type, calling convention and parameter types of every function. Feeding that
back at the reviewed original addresses is the cheapest type recovery the
project has - it is *our own source*, arriving as debug information.

Ghidra cannot read it. The file begins ``Microsoft C/C++ program database
2.00\\r\\n\\x1aJG`` - the pre-VC7 MSF container with 16-bit page indices and
``_ST`` (length-prefixed name) CodeView records - while Ghidra's universal PDB
reader requires the 7.00 ``DS`` container and its native reader requires DIA on
Windows. Rather than give up the data, this module parses the container itself
and hands Ghidra an ordinary model. Only what the transfer needs is decoded:
the module table, procedure and frame-variable symbols, the public symbols, and
the type leaves a signature walks through. A leaf this reader does not decode
keeps its index rather than becoming a plausible-looking guess.

Nothing here reads the original game image. The PDB describes our build.
"""

# One correction to the obvious expectation, because it changes how callers use
# this: VC6 stores the *source* name in a procedure symbol - `W8Timer::Sample`,
# `W8Dialog::`scalar deleting destructor'` - not the decorated one. Signatures
# therefore come from the type stream rather than from the demangler, and the
# join against a reviewed row is on the qualified source spelling, which is the
# spelling those rows already use.

from __future__ import annotations

import struct
from dataclasses import dataclass, field
from pathlib import Path

MAGIC_20 = b"Microsoft C/C++ program database 2.00\r\n\x1aJG\x00\x00"

# CodeView symbol kinds, `_ST` generation: names are length-prefixed bytes.
S_END = 0x0006
S_OBJNAME = 0x0009
S_BPREL32 = 0x1006
S_LDATA32 = 0x1007
S_GDATA32 = 0x1008
S_PUB32 = 0x1009
S_LPROC32 = 0x100A
S_GPROC32 = 0x100B
S_THUNK32 = 0x100C
S_BLOCK32 = 0x100D

# CodeView type leaves, same generation.
LF_MODIFIER = 0x1001
LF_POINTER = 0x1002
LF_ARRAY = 0x1003
LF_CLASS = 0x1004
LF_STRUCTURE = 0x1005
LF_UNION = 0x1006
LF_ENUM = 0x1007
LF_PROCEDURE = 0x1008
LF_MFUNCTION = 0x1009
LF_ARGLIST = 0x1201

FIRST_TYPE_INDEX = 0x1000

# CV_call_e, reduced to the conventions a VC6 x86 build can emit.
CALLING_CONVENTIONS = {
    0x00: "__cdecl",
    0x02: "__pascal",
    0x04: "__fastcall",
    0x07: "__stdcall",
    0x09: "__syscall",
    0x0B: "__thiscall",
}

# Primitive type indices are structured rather than enumerated: the low nibble
# names a size within a family, and the mode field above it makes it a pointer.
_PRIMITIVE_KINDS = {
    0x00: {0x00: "<no type>", 0x03: "void"},
    0x01: {0x00: "signed char", 0x01: "short", 0x02: "long", 0x03: "__int64"},
    0x02: {
        0x00: "unsigned char",
        0x01: "unsigned short",
        0x02: "unsigned long",
        0x03: "unsigned __int64",
    },
    0x03: {0x00: "bool", 0x01: "bool16", 0x02: "bool32"},
    0x04: {0x00: "float", 0x01: "double", 0x02: "long double"},
    0x07: {
        0x00: "char",
        0x01: "wchar_t",
        0x02: "short",
        0x03: "unsigned short",
        0x04: "int",
        0x05: "unsigned int",
        0x06: "__int64",
        0x07: "unsigned __int64",
    },
}

_MODULE_ENTRY = struct.Struct("<i hhii I hh II hh iii hh III")


class UnsupportedPdb(RuntimeError):
    """The file is not the VC6-era PDB 2.00 container this reader decodes."""


@dataclass(frozen=True)
class FrameVariable:
    """A ``S_BPREL32`` frame variable: name, frame offset, unresolved type."""

    name: str
    frame_offset: int
    type_index: int


@dataclass
class Procedure:
    """One procedure symbol, still in the PDB's section:offset form."""

    name: str
    section: int
    offset: int
    length: int
    module: str
    static: bool
    type_index: int
    frame_variables: list[FrameVariable] = field(default_factory=list)


@dataclass(frozen=True)
class Module:
    """A linker module - for this build, one compiled object file."""

    name: str
    object_name: str
    symbol_stream: int
    section: int
    section_offset: int
    section_size: int


@dataclass(frozen=True)
class Signature:
    """A function's type as the build's own compiler recorded it."""

    return_type: str
    convention: str
    parameters: tuple[str, ...]
    owner: str

    def spelling(self, name: str) -> str:
        arguments = ", ".join(self.parameters) if self.parameters else "void"
        return f"{self.return_type} {self.convention} {name}({arguments})"


class TypeStream:
    """Type records, resolved lazily into spellings.

    Only the leaves a signature transfer walks are decoded. Anything else keeps
    its index - `T#4242` in a rendered type is an honest "the build knows this
    and this reader does not", never a silent `undefined`.
    """

    def __init__(self, records: dict[int, tuple[int, bytes]]) -> None:
        self.records = records

    def leaf(self, index: int) -> int | None:
        record = self.records.get(index)
        return None if record is None else record[0]

    def name(self, index: int) -> str:
        """A C spelling for one type index."""

        if index < FIRST_TYPE_INDEX:
            mode = (index & 0x700) >> 8
            spelling = _PRIMITIVE_KINDS.get((index & 0x0F0) >> 4, {}).get(index & 0x007)
            if spelling is None:
                return f"T#{index:x}"
            return f"{spelling} *" if mode else spelling
        record = self.records.get(index)
        if record is None:
            return f"T#{index:x}"
        leaf, payload = record
        if leaf == LF_POINTER:
            return f"{self.name(struct.unpack_from('<I', payload, 0)[0])} *"
        if leaf == LF_MODIFIER:
            return f"const {self.name(struct.unpack_from('<I', payload, 0)[0])}"
        if leaf in (LF_CLASS, LF_STRUCTURE):
            return _tag_name(payload, 16)
        if leaf == LF_UNION:
            return _tag_name(payload, 8)
        if leaf == LF_ENUM:
            return _tag_name(payload, 12, numeric=False)
        if leaf == LF_ARRAY:
            return f"{self.name(struct.unpack_from('<I', payload, 0)[0])} []"
        return f"T#{index:x}"

    def arguments(self, index: int) -> tuple[str, ...]:
        record = self.records.get(index)
        if record is None or record[0] != LF_ARGLIST:
            return ()
        payload = record[1]
        count = struct.unpack_from("<I", payload, 0)[0]
        indices = struct.unpack_from(f"<{count}I", payload, 4)
        return tuple(self.name(argument) for argument in indices)

    def signature(self, index: int) -> Signature | None:
        """The signature of a procedure type, or None if it is not one."""

        record = self.records.get(index)
        if record is None:
            return None
        leaf, payload = record
        if leaf == LF_PROCEDURE:
            return_type, convention, _attributes, _count, arglist = struct.unpack_from(
                "<IBBHI", payload, 0
            )
            owner = ""
        elif leaf == LF_MFUNCTION:
            return_type, class_type, _this_type, convention, _attributes, _count, arglist = (
                struct.unpack_from("<IIIBBHI", payload, 0)
            )
            owner = self.name(class_type)
        else:
            return None
        return Signature(
            return_type=self.name(return_type),
            convention=CALLING_CONVENTIONS.get(convention, f"convention#{convention:x}"),
            parameters=self.arguments(arglist),
            owner=owner,
        )


def _numeric(payload: bytes, offset: int) -> int:
    """Skip a CodeView numeric leaf, returning the offset past it."""

    value = struct.unpack_from("<H", payload, offset)[0]
    if value < 0x8000:
        return offset + 2
    widths = {0x8000: 1, 0x8001: 2, 0x8002: 2, 0x8003: 2, 0x8004: 4, 0x8005: 4}
    return offset + 2 + widths.get(value, 0)


def _tag_name(payload: bytes, offset: int, numeric: bool = True) -> str:
    cursor = _numeric(payload, offset) if numeric else offset
    return _name_at(payload, cursor)[0]


def _parse_types(stream: bytes) -> TypeStream:
    """Type records keyed by index, from the 2.00 TPI stream."""

    if len(stream) < 8:
        return TypeStream({})
    header_size, first_index = struct.unpack_from("<II", stream, 4)
    records: dict[int, tuple[int, bytes]] = {}
    index = first_index
    for leaf, payload in _records(stream, header_size):
        records[index] = (leaf, payload)
        index += 1
    return TypeStream(records)


@dataclass
class ProgramDatabase:
    """The decoded parts of one PDB."""

    path: Path
    modules: list[Module]
    procedures: list[Procedure]
    publics: dict[str, tuple[int, int]]
    types: TypeStream

    def by_symbol(self) -> dict[str, Procedure]:
        """Procedures keyed by decorated name, static duplicates dropped.

        A name compiled into two objects is not a transfer candidate: the
        reviewed row could not say which body it means, and picking one would
        invent an attribution.
        """

        counts: dict[str, int] = {}
        for procedure in self.procedures:
            counts[procedure.name] = counts.get(procedure.name, 0) + 1
        return {p.name: p for p in self.procedures if counts[p.name] == 1}


def _read_streams(data: bytes) -> list[bytes]:
    """Every stream of the MSF 2.00 container, in stream-number order."""

    if not data.startswith(MAGIC_20):
        raise UnsupportedPdb(
            "not a PDB 2.00 container; this reader exists because Ghidra "
            "handles the 7.00 container and this toolchain emits 2.00"
        )
    page_size, _start_page, file_pages = struct.unpack_from("<IHH", data, 0x2C)
    root_size, _mystery = struct.unpack_from("<II", data, 0x34)
    expected = file_pages * page_size
    if len(data) < expected:
        raise UnsupportedPdb(f"truncated container: {len(data)} bytes, {expected} declared")

    def pages_for(size: int) -> int:
        return (size + page_size - 1) // page_size

    def gather(pages: tuple[int, ...], size: int) -> bytes:
        body = b"".join(data[page * page_size : (page + 1) * page_size] for page in pages)
        return body[:size]

    root_pages = struct.unpack_from(f"<{pages_for(root_size)}H", data, 0x3C)
    root = gather(root_pages, root_size)

    count = struct.unpack_from("<H", root, 0)[0]
    sizes = [struct.unpack_from("<I", root, 4 + 8 * index)[0] for index in range(count)]
    cursor = 4 + 8 * count
    streams: list[bytes] = []
    for size in sizes:
        if size in (0, 0xFFFFFFFF):
            streams.append(b"")
            continue
        page_count = pages_for(size)
        pages = struct.unpack_from(f"<{page_count}H", root, cursor)
        cursor += 2 * page_count
        streams.append(gather(pages, size))
    return streams


def _name_at(data: bytes, offset: int) -> tuple[str, int]:
    """A length-prefixed ``_ST`` name and the offset just past it."""

    length = data[offset]
    text = data[offset + 1 : offset + 1 + length].decode("latin-1")
    return text, offset + 1 + length


def _parse_modules(dbi: bytes) -> tuple[list[Module], int, int]:
    """The DBI module table, plus the symbol-record and public stream numbers.

    The 2.00 DBI header is 64 bytes and stores its stream numbers as full
    words; each module entry that follows is 64 bytes of fixed fields - its
    section contribution, its symbol stream - then the module name and the
    object name as NUL-terminated strings, padded out to four bytes.
    """

    if len(dbi) < 0x40 or struct.unpack_from("<I", dbi, 0)[0] != 0xFFFFFFFF:
        raise UnsupportedPdb("DBI stream does not start with the 2.00 header signature")
    public_stream, symbol_stream, module_bytes = struct.unpack_from("<III", dbi, 0x10)
    modules: list[Module] = []
    cursor = 0x40
    end = 0x40 + module_bytes
    while cursor < end:
        (
            _opened,
            section,
            _pad,
            section_offset,
            section_size,
            _characteristics,
            _index,
            _pad2,
            _data_crc,
            _reloc_crc,
            _flags,
            symbols,
            _symbol_bytes,
            _line_bytes,
            _fpo_bytes,
            _file_count,
            _pad3,
            _source_file,
            _pdb_file,
            _reserved,
        ) = _MODULE_ENTRY.unpack_from(dbi, cursor)
        cursor += _MODULE_ENTRY.size
        name_end = dbi.index(b"\0", cursor)
        name = dbi[cursor:name_end].decode("latin-1")
        object_end = dbi.index(b"\0", name_end + 1)
        object_name = dbi[name_end + 1 : object_end].decode("latin-1")
        cursor = (object_end + 1 + 3) & ~3
        modules.append(
            Module(
                name=name,
                object_name=object_name,
                symbol_stream=symbols,
                section=section,
                section_offset=section_offset,
                section_size=section_size,
            )
        )
    return modules, symbol_stream, public_stream


def _records(stream: bytes, start: int) -> list[tuple[int, bytes]]:
    """``(kind, payload)`` for each CodeView record, payload after the kind."""

    out: list[tuple[int, bytes]] = []
    cursor = start
    while cursor + 4 <= len(stream):
        length, kind = struct.unpack_from("<HH", stream, cursor)
        if length < 2:
            break
        end = cursor + 2 + length
        if end > len(stream):
            break
        out.append((kind, stream[cursor + 4 : end]))
        cursor = end
    return out


def _module_procedures(stream: bytes, module: str, start: int = 4) -> list[Procedure]:
    """Procedures and their frame variables from one module symbol stream."""

    procedures: list[Procedure] = []
    open_scopes = 0
    current: Procedure | None = None
    for kind, payload in _records(stream, start):
        if kind in (S_GPROC32, S_LPROC32):
            length, _dbg_start, _dbg_end, type_index, offset = struct.unpack_from(
                "<IIIII", payload, 12
            )
            section = struct.unpack_from("<H", payload, 32)[0]
            name, _ = _name_at(payload, 35)
            current = Procedure(
                name=name,
                section=section,
                offset=offset,
                length=length,
                module=module,
                static=kind == S_LPROC32,
                type_index=type_index,
            )
            procedures.append(current)
            open_scopes = 1
        elif kind in (S_BLOCK32, S_THUNK32) and open_scopes:
            open_scopes += 1
        elif kind == S_END and open_scopes:
            open_scopes -= 1
            if not open_scopes:
                current = None
        elif kind == S_BPREL32 and current is not None:
            frame_offset, type_index = struct.unpack_from("<iI", payload, 0)
            name, _ = _name_at(payload, 8)
            current.frame_variables.append(
                FrameVariable(name=name, frame_offset=frame_offset, type_index=type_index)
            )
    return procedures


def _publics(stream: bytes) -> dict[str, tuple[int, int]]:
    """``name -> (section, offset)`` for every public symbol record."""

    out: dict[str, tuple[int, int]] = {}
    for kind, payload in _records(stream, 0):
        if kind != S_PUB32:
            continue
        offset, section = struct.unpack_from("<IH", payload, 4)
        name, _ = _name_at(payload, 10)
        out[name] = (section, offset)
    return out


@dataclass
class ObjectDebugInfo:
    """One compiled object's own CodeView records, from a `/Z7` build.

    The linked PDB only describes what the linker took, and this build compiles
    far more than it links - the SGP units are object libraries the executable
    picks from. Reading the objects covers every compiled body instead, and
    each object carries its own type stream, so indices are only ever resolved
    against the object they came from.
    """

    object_file: str
    procedures: list[Procedure]
    types: TypeStream


def _coff_sections(data: bytes) -> list[tuple[str, bytes]]:
    """Every section of a COFF object as `(name, contents)`, in order.

    Names repeat: each COMDAT function carries its own associated `.debug$S`,
    so a name-keyed mapping would keep one function's symbols and drop the
    rest - which is exactly the bug that made a whole object look empty.
    """

    section_count = struct.unpack_from("<H", data, 2)[0]
    symbol_table = struct.unpack_from("<I", data, 8)[0]
    symbol_count = struct.unpack_from("<I", data, 12)[0]
    strings = symbol_table + 18 * symbol_count
    sections: list[tuple[str, bytes]] = []
    for index in range(section_count):
        header = 20 + 40 * index
        raw = data[header : header + 8]
        if raw.startswith(b"/"):
            offset = int(raw[1:].rstrip(b"\0").decode("latin-1"))
            end = data.index(b"\0", strings + offset)
            name = data[strings + offset : end].decode("latin-1")
        else:
            name = raw.rstrip(b"\0").decode("latin-1")
        size, pointer = struct.unpack_from("<II", data, header + 16)
        sections.append((name, data[pointer : pointer + size] if pointer else b""))
    return sections


def _symbol_start(section: bytes) -> int:
    """0 or 4, depending on whether this run of records has a signature word."""

    if len(section) >= 4:
        kind = struct.unpack_from("<H", section, 2)[0]
        if 0x1000 <= kind < 0x1200 or kind in (S_END, S_OBJNAME):
            return 0
    return 4


def load_object(path: Path, name: str = "") -> ObjectDebugInfo:
    """Decode one `/Z7` object's debug sections."""

    sections = _coff_sections(Path(path).read_bytes())
    procedures: list[Procedure] = []
    types = TypeStream({})
    label = name or str(path)
    for section, contents in sections:
        if section == ".debug$S" and contents:
            procedures.extend(_module_procedures(contents, label, start=_symbol_start(contents)))
        elif section == ".debug$T" and contents and not types.records:
            types = _object_types(contents)
    return ObjectDebugInfo(object_file=label, procedures=procedures, types=types)


def _object_types(section: bytes) -> TypeStream:
    """Type records from a `.debug$T` section, whose only header is a version."""

    if len(section) < 4:
        return TypeStream({})
    records: dict[int, tuple[int, bytes]] = {}
    index = FIRST_TYPE_INDEX
    for leaf, payload in _records(section, 4):
        records[index] = (leaf, payload)
        index += 1
    return TypeStream(records)


def load(path: Path) -> ProgramDatabase:
    """Decode one PDB 2.00 file into the transfer model."""

    streams = _read_streams(Path(path).read_bytes())
    if len(streams) <= 3:
        raise UnsupportedPdb("container has no DBI stream")
    modules, symbol_stream, _public_stream = _parse_modules(streams[3])
    procedures: list[Procedure] = []
    for module in modules:
        if 0 <= module.symbol_stream < len(streams) and streams[module.symbol_stream]:
            procedures.extend(_module_procedures(streams[module.symbol_stream], module.object_name))
    publics: dict[str, tuple[int, int]] = {}
    if 0 <= symbol_stream < len(streams):
        publics = _publics(streams[symbol_stream])
    return ProgramDatabase(
        path=Path(path),
        modules=modules,
        procedures=procedures,
        publics=publics,
        types=_parse_types(streams[2]),
    )
