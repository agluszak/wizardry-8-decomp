"""Focused tests for the SurRender frontier's pure analysis core.

The Ghidra-facing adapters are thin; the classification, qualified-name,
register-provenance, and provider-dependency logic are tested directly.
"""

from __future__ import annotations

from wiz8decomp.reports.surrender_frontier import (
    FrontierInsn,
    classify_priority,
    indirect_call_sites,
    provider_call_targets,
    qualified_member_name,
)

CELL = 0x005EB7E8


def ins(
    address: int,
    mnemonic: str,
    ops=(),
    results=(),
    branch_target: bool = False,
    memory=(),
) -> FrontierInsn:
    return FrontierInsn(
        address=address,
        mnemonic=mnemonic,
        operands=ops,
        results=results,
        branch_target=branch_target,
        memory_operands=frozenset(memory),
    )


def mov_load(address: int, dest: str, cell: int = CELL) -> FrontierInsn:
    """``mov dest, dword ptr [cell]``."""

    return ins(
        address,
        "MOV",
        ops=((("reg", dest),), (("addr", cell),)),
        results=(dest,),
        memory=(1,),
    )


def mov_deref(address: int, dest: str, src: str) -> FrontierInsn:
    """``mov dest, dword ptr [src]``."""

    return ins(
        address,
        "MOV",
        ops=((("reg", dest),), (("reg", src),)),
        results=(dest,),
        memory=(1,),
    )


def mov_reg(address: int, dest: str, src: str) -> FrontierInsn:
    """``mov dest, src`` register copy."""

    return ins(address, "MOV", ops=((("reg", dest),), (("reg", src),)), results=(dest,))


def call_slot(address: int, base: str, offset: int | None = None) -> FrontierInsn:
    """``call dword ptr [base+offset]`` or ``call base``."""

    objects: tuple[tuple[str, object], ...] = (("reg", base),)
    if offset is not None:
        objects = objects + (("scalar", offset),)
    return ins(address, "CALL", ops=(objects,), results=("ESP",), memory=(0,))


def call_abs(address: int, target: int) -> FrontierInsn:
    """``call 0xADDR``."""

    return ins(address, "CALL", ops=((("addr", target),),), results=("ESP",), memory=(0,))


def jmp_abs(address: int, target: int) -> FrontierInsn:
    return ins(address, "JMP", ops=((("addr", target),),))


class TestClassifyPriority:
    def test_thunk_only_import_is_p3(self) -> None:
        sites = [{"kind": "thunk", "function": "imp_beginFrame"}]
        assert classify_priority(sites, []) == "P3"

    def test_thunk_with_real_caller_is_p0(self) -> None:
        sites = [{"kind": "thunk", "function": "imp_beginFrame"}]
        assert classify_priority(sites, ["RenderFrame"]) == "P0"

    def test_direct_call_site_is_p0(self) -> None:
        sites = [{"kind": "call", "function": "RenderFrame"}]
        assert classify_priority(sites, []) == "P0"

    def test_data_reference_is_p0(self) -> None:
        sites = [{"kind": "data", "function": "W8VirtualFileBinIStream"}]
        assert classify_priority(sites, []) == "P0"

    def test_no_references_is_p3(self) -> None:
        assert classify_priority([], []) == "P3"


class TestQualifiedMemberName:
    def test_constructor(self) -> None:
        sig = "public: __thiscall srConfig::srConfig(void)"
        assert qualified_member_name(sig) == "srConfig::srConfig"

    def test_destructor(self) -> None:
        sig = "public: __thiscall srConfig::~srConfig(void)"
        assert qualified_member_name(sig) == "srConfig::~srConfig"

    def test_method(self) -> None:
        sig = "public: void __thiscall srConfig::append(char const *, char const *)"
        assert qualified_member_name(sig) == "srConfig::append"

    def test_nested_class_member(self) -> None:
        sig = "public: __thiscall srHuffman::BitIStream::BitIStream(class srBinIStream &)"
        assert qualified_member_name(sig) == "srHuffman::BitIStream::BitIStream"

    def test_operator(self) -> None:
        sig = (
            "private: class srHuffman::BitOStream & __thiscall "
            "srHuffman::BitOStream::operator=(class srHuffman::BitOStream const &)"
        )
        assert qualified_member_name(sig) == "srHuffman::BitOStream::operator="

    def test_template_owner(self) -> None:
        sig = "public: void __thiscall srGERD::clear(const class srFlags<enum srGERD::e_buffer> &)"
        assert qualified_member_name(sig) == "srGERD::clear"

    def test_free_function(self) -> None:
        sig = "void __cdecl srAssertFail(char const *, char const *, long)"
        assert qualified_member_name(sig) == "srAssertFail"

    def test_data_signature_has_no_member(self) -> None:
        assert qualified_member_name("class srConfig srConfig") is None

    def test_static_data_member(self) -> None:
        sig = "public: static class srVP * srVectorProcessor::vp"
        assert qualified_member_name(sig) == "srVectorProcessor::vp"


class TestIndirectCallSites:
    def test_vp_load_deref_vtable_call(self) -> None:
        instructions = [
            mov_load(0x100, "ECX"),
            mov_deref(0x103, "ECX", "ECX"),
            mov_deref(0x105, "EDX", "ECX"),
            call_slot(0x107, "EDX", 0x38),
        ]
        sites = indirect_call_sites(instructions, CELL)
        assert len(sites) == 1
        assert sites[0]["slot_offset"] == "0x38"
        assert sites[0]["depth"] == 2
        assert sites[0]["via_register"] == "EDX"

    def test_branch_separated_load_does_not_leak(self) -> None:
        """A load on one CFG arm must not feed a call on another."""

        instructions = [
            mov_load(0x100, "ECX"),
            jmp_abs(0x103, 0x110),
            ins(0x105, "MOV", ops=((("reg", "EAX"),), (("scalar", 1),)), results=("EAX",)),
            mov_deref(0x110, "ECX", "ECX"),  # branch_target
            mov_deref(0x112, "EDX", "ECX"),
            call_slot(0x114, "EDX", 0x38),
        ]
        # The instruction at 0x110 is the jump target.
        instructions[3] = FrontierInsn(
            address=0x110,
            mnemonic="MOV",
            operands=((("reg", "ECX"),), (("reg", "ECX"),)),
            results=("ECX",),
            branch_target=True,
            memory_operands=frozenset({1}),
        )
        assert indirect_call_sites(instructions, CELL) == []

    def test_intervening_call_drops_provenance(self) -> None:
        instructions = [
            mov_load(0x100, "ECX"),
            mov_deref(0x103, "ECX", "ECX"),
            call_abs(0x105, 0x500000),
            mov_deref(0x10A, "EDX", "ECX"),
            call_slot(0x10C, "EDX", 0x38),
        ]
        assert indirect_call_sites(instructions, CELL) == []

    def test_clobbered_register_drops_provenance(self) -> None:
        instructions = [
            mov_load(0x100, "ECX"),
            mov_deref(0x103, "ECX", "ECX"),
            ins(
                0x105,
                "XOR",
                ops=((("reg", "ECX"),), (("reg", "ECX"),)),
                results=("ECX",),
            ),
            call_slot(0x107, "ECX", 0x38),
        ]
        assert indirect_call_sites(instructions, CELL) == []

    def test_register_copy_preserves_chain(self) -> None:
        instructions = [
            mov_load(0x100, "ECX"),
            mov_deref(0x103, "EAX", "ECX"),
            mov_deref(0x105, "EDX", "EAX"),
            call_slot(0x107, "EDX", 0x10),
        ]
        sites = indirect_call_sites(instructions, CELL)
        assert len(sites) == 1
        assert sites[0]["slot_offset"] == "0x10"

    def test_direct_indirect_call_without_deref_not_recorded(self) -> None:
        """``call [reg]`` straight off the cell load (depth 0) is not an object slot."""

        instructions = [mov_load(0x100, "ECX"), call_slot(0x103, "ECX")]
        assert indirect_call_sites(instructions, CELL) == []


class TestProviderCallTargets:
    def test_call_is_dependency(self) -> None:
        instructions = [call_abs(0x1000, 0x2000)]
        assert provider_call_targets(instructions, 0x1000, 0x10FF) == [0x2000]

    def test_intra_function_jump_is_not_dependency(self) -> None:
        instructions = [
            call_abs(0x1000, 0x2000),
            jmp_abs(0x1005, 0x1050),
            ins(0x1050, "RET", branch_target=True),
        ]
        assert provider_call_targets(instructions, 0x1000, 0x10FF) == [0x2000]

    def test_tailcall_jump_is_dependency(self) -> None:
        instructions = [jmp_abs(0x1000, 0x3000)]
        assert provider_call_targets(instructions, 0x1000, 0x10FF) == [0x3000]

    def test_indirect_jump_is_ignored(self) -> None:
        instructions = [ins(0x1000, "JMP", ops=((("reg", "EAX"),),), memory=(0,))]
        assert provider_call_targets(instructions, 0x1000, 0x10FF) == []
