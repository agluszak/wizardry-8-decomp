"""Reading the build's own debug information, and joining it to the ledger.

The fixtures are built here rather than checked in: a PDB and a COFF object are
build products, and the repository does not track those. Building the container
byte by byte also pins the format this reader exists for - Ghidra's PDB support
starts at the 7.00 container, and VC6 emits 2.00.
"""

from __future__ import annotations

import struct
from pathlib import Path

import pytest
from wiz8decomp.reconstructed import (
    OVERLAY_TIER,
    REVIEWED_TIER,
    Body,
    Transfer,
    TransferPlan,
    bodies_from_objects,
    bodies_from_pdb,
    build_transfer_plan,
    frame_origin,
    index_bodies,
    parameter_names,
    proposed_signature_rows,
    reviewed_spelling,
)
from wiz8decomp.reconstructed_pdb import (
    LF_CLASS,
    LF_FIELDLIST,
    LF_MEMBER_ST,
    LF_POINTER,
    MAGIC_20,
    S_BPREL32,
    S_GPROC32,
    Signature,
    TypeStream,
    UnsupportedPdb,
    load,
    load_object,
)

PAGE = 1024


def _record(kind: int, payload: bytes) -> bytes:
    body = struct.pack("<H", kind) + payload
    if len(body) % 4:
        body += b"\0" * (4 - len(body) % 4)
    return struct.pack("<H", len(body)) + body


def _name(text: str) -> bytes:
    return bytes([len(text)]) + text.encode("latin-1")


def _field_member(name: str, type_index: int, offset: int) -> bytes:
    value = struct.pack("<HHIH", LF_MEMBER_ST, 0, type_index, offset) + _name(name)
    padding = (-len(value)) % 4
    if padding:
        value += bytes([0xF0 + padding]) + b"\0" * (padding - 1)
    return value


def test_vc6_type_stream_exposes_compiled_class_layout() -> None:
    field_list = _field_member("m_flags_00", 0x0073, 0) + _field_member(
        "m_instance_24", 0x1001, 0x24
    )
    pointer = struct.pack("<I", 0x0003)
    class_record = struct.pack("<HHIIIH", 2, 0, 0x1000, 0, 0, 0x58) + _name("GDProp")
    types = TypeStream(
        {
            0x1000: (LF_FIELDLIST, field_list),
            0x1001: (LF_POINTER, pointer),
            0x1002: (LF_CLASS, class_record),
        }
    )

    layout = types.layouts()["GDProp"]

    assert layout.size == 0x58
    assert [
        (field.name, field.offset, field.width, field.pointer_depth) for field in layout.fields
    ] == [
        ("m_flags_00", 0, 2, 0),
        ("m_instance_24", 0x24, 4, 1),
    ]


def _procedure(name: str, length: int, type_index: int, offset: int = 0x1000) -> bytes:
    return _record(
        S_GPROC32,
        struct.pack(
            "<IIIIIIIIHB",
            0,
            0,
            0,
            length,
            0,
            0,
            type_index,
            offset,
            1,
            0,
        )
        + _name(name),
    )


def _frame(name: str, frame_offset: int, type_index: int) -> bytes:
    return _record(S_BPREL32, struct.pack("<iI", frame_offset, type_index) + _name(name))


def _type_records() -> bytes:
    # T_VOID=0x0003, T_INT4=0x0074, T_32PVOID=0x0403.
    arglist = _record(0x1201, struct.pack("<III", 2, 0x0403, 0x0074))
    procedure = _record(0x1008, struct.pack("<IBBHI", 0x0074, 0x00, 0, 2, 0x1000))
    return arglist + procedure


def _symbols() -> bytes:
    return (
        _procedure("BringUpEngine", 242, 0x1001)
        + _frame("instance", 4, 0x0403)
        + _frame("show_command", 8, 0x0074)
        + _record(0x0006, b"")
    )


def _pdb(path: Path) -> Path:
    """A minimal but real PDB 2.00 container: header, root, DBI, TPI, module."""

    module_symbols = struct.pack("<I", 2) + _symbols()
    type_stream = struct.pack("<IIIIII", 19961031, 24, 0x1000, 0x1002, 0, 0) + _type_records()
    module = (
        struct.pack(
            "<i hhii I hh II hh iii hh III",
            0,
            1,
            0,
            0x1000,
            242,
            0x60501020,
            0,
            0,
            0,
            0,
            0,
            10,
            len(module_symbols),
            0,
            0,
            1,
            0,
            0,
            0,
            0,
        )
        + b"engine.obj\0engine.obj\0\0\0"
    )
    dbi = (
        struct.pack("<IIIIII", 0xFFFFFFFF, 0, 1, 6, 7, 88)
        + struct.pack("<I", len(module))
        + b"\0" * (0x40 - 0x1C)
        + module
    )

    streams: dict[int, bytes] = {2: type_stream, 3: dbi, 10: module_symbols}
    count = 11
    pages: list[bytes] = []
    directory = struct.pack("<HH", count, 0)
    page_lists = b""
    next_page = 3
    for index in range(count):
        data = streams.get(index, b"")
        directory += struct.pack("<II", len(data), 0)
        used = []
        for start in range(0, max(len(data), 1), PAGE):
            if not data:
                break
            used.append(next_page)
            chunk = data[start : start + PAGE]
            pages.append(chunk.ljust(PAGE, b"\0"))
            next_page += 1
        page_lists += struct.pack(f"<{len(used)}H", *used)
    root = directory + page_lists

    header = MAGIC_20 + struct.pack("<IHH", PAGE, 1, next_page)
    header += struct.pack("<II", len(root), 0) + struct.pack("<HH", 1, 2)
    image = bytearray(header.ljust(PAGE, b"\0"))
    image += root.ljust(2 * PAGE, b"\0")
    image += b"".join(pages)
    path.write_bytes(bytes(image))
    return path


def _object(path: Path) -> Path:
    """A COFF object shaped like a `/Z7` one: two `.debug$S`, one `.debug$T`."""

    sections = [
        (".debug$S", _procedure("Function402970", 28, 0x1001) + _record(0x0006, b"")),
        (".debug$S", _symbols()),
        (".debug$T", struct.pack("<I", 1) + _type_records()),
    ]
    headers = b""
    contents = b""
    offset = 20 + 40 * len(sections)
    for name, data in sections:
        headers += (
            name.encode("latin-1").ljust(8, b"\0")
            + struct.pack("<IIII", 0, 0, len(data), offset + len(contents))
            + struct.pack("<IIHHI", 0, 0, 0, 0, 0)
        )
        contents += data
    image = struct.pack("<HHIIIHH", 0x14C, len(sections), 0, 0, 0, 0, 0) + headers + contents
    path.write_bytes(image)
    return path


def test_the_container_ghidra_cannot_open_is_read_here(tmp_path: Path) -> None:
    database = load(_pdb(tmp_path / "build.pdb"))

    assert [module.object_name for module in database.modules] == ["engine.obj"]
    assert [procedure.name for procedure in database.procedures] == ["BringUpEngine"]
    assert database.procedures[0].length == 242


def test_a_seven_point_zero_container_is_refused_rather_than_misread(tmp_path: Path) -> None:
    path = tmp_path / "modern.pdb"
    path.write_bytes(b"Microsoft C/C++ MSF 7.00\r\n\x1aDS\0\0\0" + b"\0" * 512)

    with pytest.raises(UnsupportedPdb):
        load(path)


def test_signatures_come_out_of_the_type_stream(tmp_path: Path) -> None:
    bodies = bodies_from_pdb(_pdb(tmp_path / "build.pdb"))

    assert len(bodies) == 1
    assert bodies[0].signature == Signature(
        return_type="int", convention="__cdecl", parameters=("void *", "int"), owner=""
    )
    assert bodies[0].frame_variables == (
        ("instance", 4, "void *"),
        ("show_command", 8, "int"),
    )


def test_every_comdat_debug_section_of_an_object_is_read(tmp_path: Path) -> None:
    # Each COMDAT function carries its own `.debug$S`; keeping one section per
    # name would report an object with a dozen recovered bodies as empty.
    info = load_object(_object(tmp_path / "engine.obj"))

    assert [procedure.name for procedure in info.procedures] == [
        "Function402970",
        "BringUpEngine",
    ]


def test_objects_cover_what_the_link_left_out(tmp_path: Path) -> None:
    build = tmp_path / "build"
    (build / "CMakeFiles").mkdir(parents=True)
    _object(build / "CMakeFiles" / "engine.obj")

    bodies = bodies_from_objects(build)

    assert {body.name for body in bodies} == {"Function402970", "BringUpEngine"}
    assert all(body.object_file == "CMakeFiles/engine.obj" for body in bodies)


def test_generated_members_join_on_the_spelling_the_ledger_uses() -> None:
    assert (
        reviewed_spelling("W8Dialog::`scalar deleting destructor'")
        == "W8Dialog::scalar_deleting_destructor"
    )
    assert reviewed_spelling("W8GrowableVector<W8Element *>::Grow") == (
        "W8GrowableVector<W8Element*>::Grow"
    )


def _body(name: str, length: int, object_file: str, parameters: tuple[str, ...] = ()) -> Body:
    return Body(
        name=name,
        length=length,
        object_file=object_file,
        signature=Signature("void", "__cdecl", parameters, ""),
        frame_variables=(),
    )


def test_one_comdat_in_many_objects_is_one_body_two_different_ones_are_neither() -> None:
    same = [_body("Vector::Grow", 40, "a.obj"), _body("Vector::Grow", 40, "b.obj")]
    differing = [_body("helper", 40, "a.obj"), _body("helper", 64, "b.obj")]

    unique, ambiguous = index_bodies(same + differing)

    assert "Vector::Grow" in unique
    assert ambiguous["helper"] == ["a.obj", "b.obj"]
    assert "helper" not in unique


def test_only_an_exact_row_reaches_the_reviewed_tier(tmp_path: Path) -> None:
    repo = tmp_path
    (repo / "config" / "reccmp").mkdir(parents=True)
    (repo / "config" / "reccmp" / "wiz8-gameplay-boundaries.csv").write_text(
        "address,size,symbol,owner,confidence,relocation_masked_sha256,evidence\n"
        "00401570,242,BringUpEngine,wiz8,exact,abc,proved\n"
        "004011e0,844,WindowProc,wiz8,structurally-strong,,close\n"
        "00402970,28,TooShort,wiz8,exact,def,proved\n"
        "00404000,10,Missing,wiz8,exact,ghi,proved\n",
        encoding="utf-8",
    )
    bodies = [
        _body("BringUpEngine", 242, "engine.obj"),
        _body("WindowProc", 908, "window.obj"),
        _body("TooShort", 12, "engine.obj"),
    ]

    plan = build_transfer_plan(repo, bodies, verified_exact={"00401570"})
    by_symbol = {transfer.symbol: transfer for transfer in plan.transfers}

    assert by_symbol["BringUpEngine"].tier == REVIEWED_TIER
    # A longer compiled body is ordinary - the COMDAT carries the switch table
    # and the padding after it - so length alone does not block a transfer.
    assert by_symbol["WindowProc"].tier == OVERLAY_TIER
    assert not by_symbol["WindowProc"].blocked
    assert "too short" in by_symbol["TooShort"].blocked
    assert by_symbol["TooShort"].tier == OVERLAY_TIER
    assert [item["symbol"] for item in plan.unmatched] == ["Missing"]


def test_an_exact_ledger_row_stays_overlay_only_without_fresh_body_verification(
    tmp_path: Path,
) -> None:
    repo = tmp_path
    (repo / "config" / "reccmp").mkdir(parents=True)
    (repo / "config" / "reccmp" / "wiz8-gameplay-boundaries.csv").write_text(
        "address,size,symbol,owner,confidence,relocation_masked_sha256,evidence\n"
        "00401570,242,BringUpEngine,wiz8,exact,abc,proved\n",
        encoding="utf-8",
    )

    plan = build_transfer_plan(repo, [_body("BringUpEngine", 242, "engine.obj")])

    assert plan.transfers[0].tier == OVERLAY_TIER
    assert plan.transfers[0].blocked == "fresh relocation-masked boundary verification required"


def test_the_frame_says_where_its_own_offsets_are_measured_from() -> None:
    # A body that keeps its frame pointer puts the first parameter at ebp+8;
    # one `/O2` made frameless puts it at 4, which is Ghidra's own origin.
    assert frame_origin((("this", 8, "int"), ("local", -4, "int"))) == 8
    assert frame_origin((("value", 4, "int"),)) == 4
    assert frame_origin((("local", -8, "int"),)) is None


def test_parameter_names_come_from_the_source_or_say_they_did_not() -> None:
    named = Transfer(
        address="00401570",
        symbol="BringUpEngine",
        confidence="exact",
        tier=REVIEWED_TIER,
        object_file="engine.obj",
        signature=Signature("int", "__cdecl", ("void *", "int"), ""),
        frame_variables=(("instance", 4, "void *"), ("show_command", 8, "int")),
    )
    optimised = Transfer(
        address="00402970",
        symbol="Function402970",
        confidence="exact",
        tier=REVIEWED_TIER,
        object_file="engine.obj",
        signature=Signature("int", "__cdecl", ("void *", "int"), ""),
        frame_variables=(("kept", 4, "void *"),),
    )

    assert parameter_names(named) == ("instance", "show_command")
    assert parameter_names(optimised) == ("argument1", "argument2")


def test_proposals_are_ledger_shaped_and_only_ever_proposals() -> None:
    transfer = Transfer(
        address="00401570",
        symbol="BringUpEngine",
        confidence="exact",
        tier=REVIEWED_TIER,
        object_file="engine.obj",
        signature=Signature("unsigned char", "__cdecl", ("void *",), ""),
        frame_variables=(("instance", 4, "void *"),),
    )
    overlay_only = Transfer(
        address="004011e0",
        symbol="WindowProc",
        confidence="structurally-strong",
        tier=OVERLAY_TIER,
        object_file="window.obj",
        signature=Signature("long", "__stdcall", ("void *",), ""),
        frame_variables=(),
    )

    rows = proposed_signature_rows(TransferPlan(transfers=[transfer, overlay_only]))

    assert len(rows) == 1
    assert rows[0]["address"] == "00401570"
    assert rows[0]["evidence_id"] == "signatures:wiz8:00401570"
    assert rows[0]["parameters_json"] == '[["instance", "void *"]]'
    assert rows[0]["confidence"] == "strong"
    assert rows[0]["calling_convention_authority"] == "exact-body"
    assert rows[0]["parameter_name_authority"].startswith("reconstructed-source")
    # Only Ghidra can state what it had before, so the column stays empty here.
    assert rows[0]["previous_auto_signature"] == ""
