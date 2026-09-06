"""The field-access trace, driven by fakes of the varnode/P-code op surface.

`trace_accesses` is deliberately duck-typed so this file can execute the whole
traversal without a JVM. The fakes reproduce the one property that already
broke the real thing once: every accessor returns a *fresh* wrapper object, so
Python identity between two references to the same varnode is always false and
only `equals` can be trusted.
"""

from __future__ import annotations

from typing import Any

from wiz8decomp.ghidra.semantic import (
    _address_expression,
    _implicit_receiver_paths,
    trace_accesses,
)


class _Space:
    def __init__(self, name: str) -> None:
        self._name = name

    def getName(self) -> str:
        return self._name


class _Address:
    def __init__(self, space: str, offset: int) -> None:
        self._space = _Space(space)
        self._offset = offset

    def getAddressSpace(self) -> _Space:
        return self._space

    def equals(self, other: Any) -> bool:
        return isinstance(other, _Address) and (self._space.getName(), self._offset) == (
            other._space.getName(),
            other._offset,
        )

    def __str__(self) -> str:
        return f"{self._offset:08x}"


class _Seq:
    def __init__(self, address: int, time: int) -> None:
        self._address = address
        self._time = time

    def getTarget(self) -> _Address:
        return _Address("ram", self._address)

    def getTime(self) -> int:
        return self._time

    def __str__(self) -> str:
        return f"{self._address:x}:{self._time}"


class _Node:
    """One SSA value. `handle()` returns a fresh wrapper sharing this core."""

    def __init__(self, space: str, offset: int, size: int = 4, constant: bool = False) -> None:
        self.space = space
        self.offset = offset
        self.size = size
        self.constant = constant
        self.definition: Any = None
        self.descendants: list[Any] = []

    def handle(self) -> _Handle:
        return _Handle(self)


class _Handle:
    def __init__(self, core: _Node) -> None:
        self._core = core

    def equals(self, other: Any) -> bool:
        return isinstance(other, _Handle) and other._core is self._core

    def getAddress(self) -> _Address:
        return _Address(self._core.space, self._core.offset)

    def getOffset(self) -> int:
        return self._core.offset

    def getSize(self) -> int:
        return self._core.size

    def isConstant(self) -> bool:
        return self._core.constant

    def isAddress(self) -> bool:
        return self._core.space == "ram"

    def getDef(self) -> Any:
        return self._core.definition

    def getHigh(self) -> Any:
        return None

    def getDescendants(self) -> _Iterator:
        return _Iterator([op for op in self._core.descendants])


class _Iterator:
    def __init__(self, items: list[Any]) -> None:
        self._items = list(items)

    def hasNext(self) -> bool:
        return bool(self._items)

    def next(self) -> Any:
        return self._items.pop(0)


class _Op:
    def __init__(
        self,
        mnemonic: str,
        inputs: list[_Node | None],
        output: _Node | None,
        address: int,
        time: int = 0,
    ) -> None:
        self._mnemonic = mnemonic
        self._inputs = inputs
        self._output = output
        self._seq = _Seq(address, time)
        for node in inputs:
            if node is not None:
                node.descendants.append(self)
        if output is not None:
            output.definition = self

    def getMnemonic(self) -> str:
        return self._mnemonic

    def getSeqnum(self) -> _Seq:
        return self._seq

    def getOutput(self) -> Any:
        return self._output.handle() if self._output is not None else None

    def getNumInputs(self) -> int:
        return len(self._inputs)

    def getInput(self, index: int) -> Any:
        node = self._inputs[index]
        return node.handle() if node is not None else None


def _const(value: int) -> _Node:
    return _Node("const", value, constant=True)


def test_offsets_survive_fresh_wrappers_and_pointer_arithmetic() -> None:
    # this -> PTRADD +0x14 -> LOAD; identity between wrappers is always false,
    # so only Java-style equality can attribute the ops to the root.
    this = _Node("register", 4)
    member_pointer = _Node("unique", 0x100)
    loaded = _Node("unique", 0x200)
    _Op("PTRADD", [this, _const(5), _const(4)], member_pointer, 0x1000)
    _Op("LOAD", [_const(0x1A1), member_pointer], loaded, 0x1004)

    accesses = trace_accesses([this.handle()], "this")

    assert {(a["kind"], a["path"], a["offset"]) for a in accesses} >= {
        ("load", "this", "0x14"),
    }


def test_the_virtual_destructor_chain_is_visible_to_depth_three() -> None:
    # load member at +0x14, null-test it, load its vptr, load slot 0, CALLIND:
    # the Prop destructor's shape, which must arrive as data rather than prose.
    this = _Node("register", 4)
    member_ptr = _Node("unique", 0x100)
    member = _Node("unique", 0x200)
    vptr = _Node("unique", 0x300)
    slot = _Node("unique", 0x400)
    compare = _Node("register", 0x206, size=1)
    _Op("PTRADD", [this, _const(5), _const(4)], member_ptr, 0x1000)
    _Op("LOAD", [_const(0x1A1), member_ptr], member, 0x1004)
    _Op("INT_NOTEQUAL", [member, _const(0)], compare, 0x1008)
    _Op("LOAD", [_const(0x1A1), member], vptr, 0x100C)
    _Op("LOAD", [_const(0x1A1), vptr], slot, 0x1010)
    _Op("CALLIND", [slot, _const(1)], None, 0x1014)

    accesses = trace_accesses([this.handle()], "this")
    kinds = {(a["kind"], a["path"], a["offset"]) for a in accesses}

    assert ("load", "this", "0x14") in kinds
    assert ("null-test", "this[0x14]", "0x0") in kinds
    assert ("load", "this[0x14]", "0x0") in kinds
    assert ("load", "this[0x14][0x0]", "0x0") in kinds
    assert ("indirect-call-target", "this[0x14][0x0][0x0]", "0x0") in kinds


def test_a_member_passed_to_a_direct_call_names_its_target_and_position() -> None:
    this = _Node("register", 4)
    member_ptr = _Node("unique", 0x100)
    member = _Node("unique", 0x200)
    target = _Node("ram", 0x5E1C10)
    _Op("INT_ADD", [this, _const(0x20)], member_ptr, 0x1000)
    _Op("LOAD", [_const(0x1A1), member_ptr], member, 0x1004)
    _Op("CALL", [target, member], None, 0x1008)

    accesses = trace_accesses([this.handle()], "this")
    call = next(a for a in accesses if a["kind"] == "call-arg")

    assert call["path"] == "this[0x20]"
    assert call["argument"] == 0
    assert call["target"] == "005e1c10"


def test_stores_record_width_and_value_and_cycles_terminate() -> None:
    this = _Node("register", 4)
    field_ptr = _Node("unique", 0x100)
    looped = _Node("unique", 0x300)
    _Op("INT_ADD", [this, _const(0x828)], field_ptr, 0x1000)
    _Op("STORE", [_const(0x1A1), field_ptr, _const(10000)], None, 0x1004)
    # A MULTIEQUAL feeding itself must not spin the trace.
    _Op("MULTIEQUAL", [this, looped], looped, 0x1008)
    _Op("MULTIEQUAL", [looped], looped, 0x100C)

    accesses = trace_accesses([this.handle()], "this")
    store = next(a for a in accesses if a["kind"] == "store")

    assert (store["path"], store["offset"], store["width"]) == ("this", "0x828", 4)


def test_merged_distinct_pointer_offsets_are_not_reported_as_one_field() -> None:
    this = _Node("register", 4)
    left = _Node("unique", 0x100)
    right = _Node("unique", 0x104)
    merged = _Node("unique", 0x108)
    loaded = _Node("unique", 0x10C)
    _Op("INT_ADD", [this, _const(4)], left, 0x1000)
    _Op("INT_ADD", [this, _const(8)], right, 0x1004)
    _Op("MULTIEQUAL", [left, right], merged, 0x1008)
    _Op("LOAD", [_const(0x1A1), merged], loaded, 0x100C)

    accesses = trace_accesses([this.handle()], "this")

    assert not any(access["kind"] == "load" for access in accesses)


def test_constant_minus_pointer_is_not_a_root_relative_address() -> None:
    this = _Node("register", 4)
    difference = _Node("unique", 0x100)
    _Op("INT_SUB", [_const(0x20), this], difference, 0x1000)

    assert _address_expression(difference.handle()) == {"kind": "unresolved"}


class _Flow:
    def __init__(self, *, jump: bool = False, terminal: bool = False) -> None:
        self._jump = jump
        self._terminal = terminal

    def isJump(self) -> bool:
        return self._jump

    def isTerminal(self) -> bool:
        return self._terminal


class _Instruction:
    def __init__(
        self,
        address: int,
        mnemonic: str,
        pcode: list[_Op] | None = None,
        *,
        jump: bool = False,
    ) -> None:
        self._address = address
        self._mnemonic = mnemonic
        self._pcode = pcode or []
        self._flow = _Flow(jump=jump)

    def getAddress(self) -> _Address:
        return _Address("ram", self._address)

    def getMnemonicString(self) -> str:
        return self._mnemonic

    def getPcode(self) -> list[_Op]:
        return self._pcode

    def getFlowType(self) -> _Flow:
        return self._flow

    def getFallThrough(self) -> _Address:
        return _Address("ram", self._address + 1)


class _Listing:
    def __init__(self, instructions: list[_Instruction]) -> None:
        self._instructions = instructions

    def getInstructions(self, _body: Any, _forward: bool) -> _Iterator:
        return _Iterator(self._instructions)


class _Program:
    def __init__(self, instructions: list[_Instruction]) -> None:
        self._listing = _Listing(instructions)

    def getListing(self) -> _Listing:
        return self._listing


class _Function:
    def getBody(self) -> object:
        return object()


def test_implicit_receiver_evidence_stops_at_calls_and_control_flow() -> None:
    edi = _Node("register", 0x1C)
    ecx = _Node("register", 4)
    seed = _Op("COPY", [_const(0)], edi, 0x1000)
    copy = _Op("COPY", [edi], ecx, 0x1001)
    instructions = [
        _Instruction(0x1000, "MOV", [seed]),
        _Instruction(0x1001, "MOV", [copy]),
        _Instruction(0x1002, "CALL"),
        _Instruction(0x1003, "CALL"),
        _Instruction(0x1004, "JMP", jump=True),
        _Instruction(0x2000, "CALL"),
    ]
    accesses = [{"kind": "load", "path": "this", "offset": "0x14", "site": "00001000"}]

    receivers = _implicit_receiver_paths(_Program(instructions), _Function(), accesses)

    assert receivers == {"00001002": ("this[0x14]", 0, "00001000")}
