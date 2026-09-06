from __future__ import annotations

from wiz8decomp.surrender_abi import (
    decode_vbtable,
    decode_vftable,
    parse_decorated_name,
    vftable_base_from_signature,
)


class _FakeSection:
    def __init__(self, executable: bool) -> None:
        self.executable = executable


class _FakeImage:
    """Enough of a PE view to exercise the two table decoders.

    Words are addressed by RVA; `code` is the RVA range an executable section
    covers, so a slot pointing outside it ends a vftable the way a real one does.
    """

    image_base = 0x10000000

    def __init__(self, words: dict[int, int], code: range) -> None:
        self._words = words
        self._code = code

    def read_u32(self, address: int) -> int | None:
        return self._words.get(address - self.image_base)

    def read_i32(self, address: int) -> int | None:
        value = self._words.get(address - self.image_base)
        if value is None:
            return None
        return value - 0x100000000 if value >= 0x80000000 else value

    def section_at(self, address: int) -> _FakeSection | None:
        rva = address - self.image_base
        if rva in self._code:
            return _FakeSection(executable=True)
        return _FakeSection(executable=False) if rva in self._words else None


def test_constructor_and_destructor_names_are_the_class_not_the_member() -> None:
    constructor = parse_decorated_name("??0srCamera@@QAE@XZ")
    destructor = parse_decorated_name("??1Decompressor@srHuffman@@QAE@XZ")

    assert (constructor.kind, constructor.class_name) == ("constructor", "srCamera")
    assert (destructor.kind, destructor.class_name) == ("destructor", "Decompressor")
    assert destructor.enclosing_scope == "srHuffman"


def test_storage_class_separates_virtual_static_and_free_members() -> None:
    virtual = parse_decorated_name("?verify@srMaterial@@UAEXW4e_verify@srRuntimeClass@@@Z")
    static = parse_decorated_name("?sGetClassName@srTexture@@SAPBDXZ")
    free = parse_decorated_name("?srAssertFail@@YAXPBD0J0ZZ")

    assert (virtual.virtuality, virtual.access) == ("virtual", "public")
    assert virtual.calling_convention == "__thiscall"
    assert static.virtuality == "static"
    assert static.calling_convention == "__cdecl"
    assert free.virtuality == "free-function"
    assert free.class_name == ""


def test_static_data_members_use_digit_storage_classes() -> None:
    parsed = parse_decorated_name("?CPU_Features_Mask@srTimer@@1KB")

    assert parsed.virtuality == "static-data"
    assert parsed.access == "protected"
    assert parsed.class_name == "srTimer"
    assert parsed.parse_status == "ok"


def test_global_operator_has_an_empty_scope_terminated_by_one_at_sign() -> None:
    """Splitting on '@@' here lands inside the template argument list."""
    parsed = parse_decorated_name(
        "??6@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@std@@AAV01@ABVsrShader@@@Z"
    )

    assert parsed.kind == "operator-lshift"
    assert parsed.virtuality == "free-function"
    assert parsed.parse_status == "ok"


def test_encoded_string_literals_carry_no_scope_to_decode() -> None:
    parsed = parse_decorated_name("??_C@_07HOCI@srLight?$AA@")

    assert parsed.kind == "string-literal"
    assert parsed.parse_status == "ok"


def test_vftable_base_comes_from_the_demangled_form_not_the_backreference() -> None:
    """`@@6B0@@` is a back-reference to the class itself, not a base named '0'."""
    assert (
        vftable_base_from_signature("const srBinFStream::`vftable'{for `srBinFStream'}")
        == "srBinFStream"
    )
    assert vftable_base_from_signature("const srBinIMStream::`vbtable'") == ""
    assert (
        vftable_base_from_signature(
            "const srLight::`vftable'{for `srClassSupport<class srIlluminator, class srNode, 0, 4608>'}"
        )
        == "srClassSupport<class srIlluminator, class srNode, 0, 4608>"
    )


_BASE = _FakeImage.image_base


def test_a_vftable_run_ends_where_the_relocations_do() -> None:
    # Three slots into code, then a word that is not relocated: whatever follows
    # a vftable in .rdata is not part of it.
    image = _FakeImage(
        words={0x1000: 0x10002000, 0x1004: 0x10002100, 0x1008: 0x10002200, 0x100C: 0x10002300},
        code=range(0x2000, 0x3000),
    )
    relocated = {_BASE + 0x1000, _BASE + 0x1004, _BASE + 0x1008}

    slots = decode_vftable(image, relocated, 0x1000, boundaries=set())

    assert [slot.index for slot in slots] == [0, 1, 2]
    assert [slot.target_rva for slot in slots] == [0x2000, 0x2100, 0x2200]


def test_a_vftable_run_ends_at_a_slot_that_leaves_executable_code() -> None:
    image = _FakeImage(
        words={0x1000: 0x10002000, 0x1004: 0x10009000},
        code=range(0x2000, 0x3000),
    )
    relocated = {_BASE + 0x1000, _BASE + 0x1004}

    slots = decode_vftable(image, relocated, 0x1000, boundaries=set())

    assert [slot.target_rva for slot in slots] == [0x2000]


def test_a_vftable_run_ends_where_another_table_begins() -> None:
    # Two adjacent tables are indistinguishable by relocation alone. A boundary
    # is either an exported symbol or an address code refers to; both arrive in
    # the same set, because a table has to be referred to to be used at all.
    image = _FakeImage(
        words={0x1000: 0x10002000, 0x1004: 0x10002100, 0x1008: 0x10002200},
        code=range(0x2000, 0x3000),
    )
    relocated = {_BASE + 0x1000, _BASE + 0x1004, _BASE + 0x1008}

    slots = decode_vftable(image, relocated, 0x1000, boundaries={0x1000, 0x1008})

    assert [slot.target_rva for slot in slots] == [0x2000, 0x2100]


def test_a_vbtable_run_ends_at_the_first_relocated_word() -> None:
    # A vbtable holds displacements, so nothing in it is relocated; the first
    # relocated word belongs to the data that follows.
    image = _FakeImage(
        words={0x1000: 0xFFFFFFFC, 0x1004: 0x20, 0x1008: 0x10002000},
        code=range(0x2000, 0x3000),
    )

    assert decode_vbtable(image, {_BASE + 0x1008}, 0x1000, boundaries=set()) == [-4, 0x20]


def test_a_vbtable_run_ends_at_padding_rather_than_reading_it_as_a_base() -> None:
    # Zero after entry zero would place a virtual base on top of the vbptr.
    image = _FakeImage(
        words={0x1000: 0xFFFFFFFC, 0x1004: 0x14, 0x1008: 0}, code=range(0x2000, 0x3000)
    )

    assert decode_vbtable(image, set(), 0x1000, boundaries=set()) == [-4, 0x14]
