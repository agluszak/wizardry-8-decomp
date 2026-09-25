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
    _flow_completeness,
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

    stops: list[dict[str, Any]] = []
    accesses = trace_accesses([this.handle()], "this", incompleteness=stops)

    known_field = _Node("unique", 0x300)
    _Op("INT_ADD", [this, _const(0x20)], known_field, 0x1010)
    _Op("STORE", [_const(0x1A1), known_field, _const(1)], None, 0x1014)
    stops.clear()
    accesses = trace_accesses([this.handle()], "this", incompleteness=stops)

    assert not any(access["kind"] == "load" for access in accesses)
    assert any(access["kind"] == "store" for access in accesses)
    assert [stop["kind"] for stop in stops] == ["ambiguous_join"]


def test_constant_minus_pointer_is_not_a_root_relative_address() -> None:
    this = _Node("register", 4)
    difference = _Node("unique", 0x100)
    _Op("INT_SUB", [_const(0x20), this], difference, 0x1000)

    assert _address_expression(difference.handle()) == {"kind": "unresolved"}


def _affine_load(
    *, stride: int = 16, shifted: bool = False, guarded: bool = False
) -> tuple[list[dict[str, Any]], _Node]:
    this = _Node("register", 4)
    spell_id = _Node("register", 0xC)
    address = _Node("unique", 0x500)
    if shifted:
        base = _Node("unique", 0x100)
        shifted_index = _Node("unique", 0x200)
        _Op("INT_ADD", [this, _const(0x40C2)], base, 0x1100)
        _Op("INT_LEFT", [spell_id, _const(4)], shifted_index, 0x1104)
        _Op("INT_ADD", [base, shifted_index], address, 0x1108)
    else:
        base = _Node("unique", 0x100)
        scaled = _Node("unique", 0x200)
        _Op("PTRADD", [this, spell_id, _const(stride)], scaled, 0x1100)
        _Op("INT_ADD", [scaled, _const(0x40C2)], address, 0x1104)
    loaded = _Node("unique", 0x600)
    _Op("LOAD", [_const(0x1A1), address], loaded, 0x1110)
    if guarded:
        _Op("INT_LESS", [spell_id, _const(32)], _Node("register", 0x10, size=1), 0x1114)
    return trace_accesses([this.handle()], "this"), spell_id


def test_symbolic_index_keeps_ssa_identity_stride_width_and_unbounded_status() -> None:
    accesses, _spell_id = _affine_load()
    load = next(access for access in accesses if access["kind"] == "load")
    address = load["effective_address"]

    assert load["site"] == "00001110"
    assert load["width"] == 4
    assert address["constant"] == 0x40C2
    assert [(term["stride"], term["range_guards"]) for term in address["terms"]] == [(16, [])]


def test_observed_comparison_is_reported_without_claiming_guard_dominance() -> None:
    accesses, _spell_id = _affine_load(guarded=True)
    address = next(access["effective_address"] for access in accesses if access["kind"] == "load")
    guard = address["terms"][0]["range_guards"][0]

    assert guard["site"] == "00001114"
    assert guard["predicate"] == "INT_LESS"
    assert "dominance not established" in guard["control_relation"]


def test_affine_guard_collection_is_bounded_and_marks_truncation() -> None:
    this = _Node("register", 4)
    spell_id = _Node("register", 0xC)
    address = _Node("unique", 0x100)
    loaded = _Node("unique", 0x200)
    _Op("PTRADD", [this, spell_id, _const(16)], address, 0x1130)
    _Op("LOAD", [_const(0x1A1), address], loaded, 0x1134)
    for index in range(17):
        _Op(
            "INT_LESS",
            [spell_id, _const(index)],
            _Node("register", 0x20 + index, size=1),
            0x1140 + index,
        )

    accesses = trace_accesses([this.handle()], "this")
    term = next(
        access["effective_address"]["terms"][0] for access in accesses if access["kind"] == "load"
    )

    assert len(term["range_guards"]) == 16
    assert term["range_guards_truncated"] is True


def test_shifted_base_expression_matches_ptradd_affine_address() -> None:
    ptradd, _ = _affine_load(stride=16)
    shifted, _ = _affine_load(shifted=True)
    left = next(item["effective_address"] for item in ptradd if item["kind"] == "load")
    right = next(item["effective_address"] for item in shifted if item["kind"] == "load")

    assert left == right


def test_integer_multiply_form_matches_scaled_pointer_arithmetic() -> None:
    ptradd, _ = _affine_load(stride=16)
    this = _Node("register", 4)
    spell_id = _Node("register", 0xC)
    scaled_index = _Node("unique", 0x300)
    base = _Node("unique", 0x400)
    address = _Node("unique", 0x450)
    loaded = _Node("unique", 0x500)
    _Op("INT_MULT", [spell_id, _const(16)], scaled_index, 0x1120)
    _Op("INT_ADD", [this, _const(0x40C2)], base, 0x1124)
    _Op("INT_ADD", [base, scaled_index], address, 0x1128)
    _Op("LOAD", [_const(0x1A1), address], loaded, 0x112C)
    multiplied = trace_accesses([this.handle()], "this")

    left = next(item["effective_address"] for item in ptradd if item["kind"] == "load")
    right = next(item["effective_address"] for item in multiplied if item["kind"] == "load")
    assert left == right


def test_non_affine_symbolic_index_is_an_explicit_stop() -> None:
    this = _Node("register", 4)
    left_index = _Node("register", 0xC)
    right_index = _Node("register", 0x10)
    nonlinear = _Node("unique", 0x100)
    address = _Node("unique", 0x200)
    loaded = _Node("unique", 0x300)
    _Op("INT_MULT", [left_index, right_index], nonlinear, 0x1180)
    _Op("PTRADD", [this, nonlinear, _const(4)], address, 0x1184)
    _Op("LOAD", [_const(0x1A1), address], loaded, 0x1188)
    stops: list[dict[str, Any]] = []

    accesses = trace_accesses([this.handle()], "this", incompleteness=stops)

    assert not any(access["kind"] == "load" for access in accesses)
    assert "symbolic_index" in {stop["kind"] for stop in stops}
    assert "unsupported_arithmetic" in {stop["kind"] for stop in stops}


def test_affine_term_limit_is_explicit() -> None:
    this = _Node("register", 4)
    address: _Node = this
    for index in range(9):
        updated = _Node("unique", 0x400 + index)
        _Op(
            "INT_ADD",
            [address, _Node("register", 0x20 + index)],
            updated,
            0x1190 + index,
        )
        address = updated
    _Op("LOAD", [_const(0x1A1), address], _Node("unique", 0x600), 0x11A0)
    stops: list[dict[str, Any]] = []

    accesses = trace_accesses([this.handle()], "this", incompleteness=stops)

    assert not any(access["kind"] == "load" for access in accesses)
    stop = next(item for item in stops if item["kind"] == "unsupported_arithmetic")
    assert len(stop["expression"]["terms"]) == 8


def test_twelve_and_sixteen_byte_strides_remain_distinct_constraints() -> None:
    stride_12, _ = _affine_load(stride=12)
    stride_16, _ = _affine_load(stride=16)
    first = next(item["effective_address"] for item in stride_12 if item["kind"] == "load")
    second = next(item["effective_address"] for item in stride_16 if item["kind"] == "load")

    assert first["terms"][0]["stride"] == 12
    assert second["terms"][0]["stride"] == 16
    assert first != second


def test_negative_pointer_adjustment_is_signed_at_pointer_width() -> None:
    this = _Node("register", 4)
    adjusted = _Node("unique", 0x100)
    _Op("INT_ADD", [this, _const(0xFFFFFFF0)], adjusted, 0x1200)
    value = _Node("const", 7, size=1, constant=True)
    _Op("STORE", [_const(0x1A1), adjusted, value], None, 0x1204)

    store = next(
        access for access in trace_accesses([this.handle()], "this") if access["kind"] == "store"
    )

    assert store["offset"] == "-0x10"
    assert store["effective_address"]["constant"] == -16


def test_ptrsub_preserves_negative_pointer_adjustment() -> None:
    this = _Node("register", 4)
    adjusted = _Node("unique", 0x110)
    _Op("PTRSUB", [this, _const(0x10)], adjusted, 0x1210)
    _Op(
        "STORE",
        [_const(0x1A1), adjusted, _Node("const", 1, size=1, constant=True)],
        None,
        0x1214,
    )

    store = next(
        access for access in trace_accesses([this.handle()], "this") if access["kind"] == "store"
    )

    assert store["offset"] == "-0x10"
    assert store["effective_address"]["constant"] == -16


def test_access_widths_and_receiver_identities_stay_separate() -> None:
    first_receiver = _Node("register", 4)
    second_receiver = _Node("register", 8)
    first_field = _Node("unique", 0x100)
    second_field = _Node("unique", 0x200)
    _Op("INT_ADD", [first_receiver, _const(0x18)], first_field, 0x1300)
    _Op("INT_ADD", [second_receiver, _const(0x18)], second_field, 0x1304)
    _Op(
        "STORE",
        [_const(0x1A1), first_field, _Node("const", 1, size=2, constant=True)],
        None,
        0x1308,
    )
    _Op(
        "STORE",
        [_const(0x1A1), second_field, _Node("const", 0, size=4, constant=True)],
        None,
        0x130C,
    )
    left = trace_accesses([first_receiver.handle()], "this", root_identity="fnA:this")
    right = trace_accesses([second_receiver.handle()], "this", root_identity="fnB:this")
    left_store = next(access for access in left if access["kind"] == "store")
    right_store = next(access for access in right if access["kind"] == "store")

    assert left_store["width"] == 2
    assert right_store["width"] == 4
    assert left_store["effective_address"]["constant"] == 0x18
    assert right_store["effective_address"]["constant"] == 0x18
    assert left_store["effective_address"]["root"] != right_store["effective_address"]["root"]


def test_depth_and_step_limits_are_explicit_and_keep_prior_accesses(monkeypatch: Any) -> None:
    this = _Node("register", 4)
    stops: list[dict[str, Any]] = []
    loaded = this
    for depth in range(5):
        address = _Node("unique", 0x100 + depth)
        next_pointer = _Node("unique", 0x200 + depth)
        _Op("INT_ADD", [loaded, _const(4)], address, 0x1400 + depth * 8)
        _Op("LOAD", [_const(0x1A1), address], next_pointer, 0x1404 + depth * 8)
        loaded = next_pointer
    accesses = trace_accesses([this.handle()], "this", incompleteness=stops)
    assert any(stop["kind"] == "depth_limit" for stop in stops)
    assert any(access["kind"] == "load" for access in accesses)

    root = _Node("register", 4)
    field = _Node("unique", 0x300)
    _Op("INT_ADD", [root, _const(8)], field, 0x1500)
    _Op("STORE", [_const(0x1A1), field, _Node("const", 1, size=1, constant=True)], None, 0x1504)
    _Op("STORE", [_const(0x1A1), root, _Node("const", 0, size=1, constant=True)], None, 0x1508)
    monkeypatch.setattr("wiz8decomp.ghidra.semantic._TRACE_LIMIT", 2)
    step_stops: list[dict[str, Any]] = []
    partial = trace_accesses([root.handle()], "this", incompleteness=step_stops)

    assert any(stop["kind"] == "step_limit" for stop in step_stops)
    assert any(access["kind"] == "store" for access in partial)


def test_complete_empty_scope_is_distinct_from_incomplete_empty_scope() -> None:
    complete = _flow_completeness([], [])
    incomplete = _flow_completeness([{"kind": "ambiguous_join"}], [])

    assert complete["status"] == "complete"
    assert complete["observation"].startswith("no accesses observed")
    assert complete["whole_program_absence_claim"] is False
    assert incomplete["status"] == "incomplete"


def test_missing_root_is_reported_as_prototype_dependent(monkeypatch: Any) -> None:
    from wiz8decomp.ghidra import resolve, semantic

    function = _Function()
    high = _HighFunction([_HighSymbol("present", [])])
    monkeypatch.setattr(resolve, "resolve_function", lambda _program, _argument: function)
    monkeypatch.setattr(semantic, "_high_function", lambda *_args, **_kwargs: high)

    result = semantic.field_accesses(_Program([]), "0x1000", "missing")

    assert result["program"]["binary_sha256"] == "retail-sha256"
    assert result["function"]["entry"] == "00001000"
    assert result["profile"]["name"] == "analysis"
    assert result["accesses"] == []
    assert {stop["kind"] for stop in result["completeness"]["stops"]} == {
        "missing_root",
        "prototype_dependent_input_omission",
    }

    global_result = semantic.field_accesses(_Program([]), "0x1000", "global:0x1234")
    assert global_result["root"]["kind"] == "global"
    assert any(
        stop["kind"] == "unsupported_root_kind" for stop in global_result["completeness"]["stops"]
    )
    adjusted_result = semantic.field_accesses(_Program([]), "0x1000", "adjusted-receiver:W8Base")
    assert adjusted_result["root"]["kind"] == "adjusted_receiver"
    assert any(
        stop["kind"] == "unsupported_root_kind" for stop in adjusted_result["completeness"]["stops"]
    )


def test_parameter_with_no_accesses_is_complete_only_within_its_scope(monkeypatch: Any) -> None:
    from wiz8decomp.ghidra import resolve, semantic

    function = _Function()
    root = _Node("register", 4)
    high = _HighFunction([_HighSymbol("this", [root.handle()])])
    monkeypatch.setattr(resolve, "resolve_function", lambda _program, _argument: function)
    monkeypatch.setattr(semantic, "_high_function", lambda *_args, **_kwargs: high)

    result = semantic.field_accesses(_Program([]), "0x1000", "this")

    assert result["accesses"] == []
    assert result["completeness"]["status"] == "complete"
    assert result["completeness"]["observation"].startswith("no accesses observed")
    assert result["completeness"]["whole_program_absence_claim"] is False


def test_flow_rows_include_program_root_profile_and_instruction_identity(monkeypatch: Any) -> None:
    from wiz8decomp.ghidra import resolve, semantic

    function = _Function()
    root = _Node("register", 4)
    field = _Node("unique", 0x300)
    _Op("INT_ADD", [root, _const(0x18)], field, 0x1600)
    _Op("STORE", [_const(0x1A1), field, _Node("const", 7, size=4, constant=True)], None, 0x1604)
    high = _HighFunction([_HighSymbol("this", [root.handle()])])
    monkeypatch.setattr(resolve, "resolve_function", lambda _program, _argument: function)
    monkeypatch.setattr(semantic, "_high_function", lambda *_args, **_kwargs: high)

    result = semantic.field_accesses(_Program([]), "0x1000", "this", profile="recovery")
    store = next(access for access in result["accesses"] if access["kind"] == "store")

    assert result["program"]["binary_sha256"] == "retail-sha256"
    assert result["entry"] == "00001000"
    assert result["root"]["kind"] == "parameter"
    assert result["root"]["role"] == "receiver"
    assert result["root"]["storage"] == "ECX:4"
    assert result["root"]["type_origin"] == "model-derived HighFunction prototype"
    assert result["profile"]["name"] == "recovery"
    assert store["site"] == "00001604"
    assert store["width"] == 4
    assert store["effective_address"]["root"] == result["root"]["identity"]


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

    def getName(self) -> str:
        return "wiz8"

    def getOptions(self, _name: str) -> Any:
        return _ProgramOptions()

    def getLanguageID(self) -> str:
        return "x86:LE:32:default"

    def getCompilerSpec(self) -> Any:
        return _CompilerSpec()

    def getDefaultPointerSize(self) -> int:
        return 4


class _ProgramOptions:
    def getString(self, _key: str, default: Any = None) -> Any:
        return "retail-sha256" if default is None else default


class _CompilerSpec:
    def getCompilerSpecID(self) -> str:
        return "windows"


class _Function:
    def getBody(self) -> object:
        return object()

    def getEntryPoint(self) -> _Address:
        return _Address("ram", 0x1000)

    def getName(self) -> str:
        return "TestFunction"


class _DataType:
    def getDisplayName(self) -> str:
        return "int *"


class _HighVariable:
    def __init__(self, instances: list[_Handle]) -> None:
        self._instances = instances

    def getInstances(self) -> list[_Handle]:
        return self._instances


class _HighSymbol:
    def __init__(self, name: str, instances: list[_Handle]) -> None:
        self._name = name
        self._high = _HighVariable(instances)

    def getName(self) -> str:
        return self._name

    def getStorage(self) -> str:
        return "ECX:4"

    def getDataType(self) -> _DataType:
        return _DataType()

    def isParameter(self) -> bool:
        return True

    def getCategoryIndex(self) -> int:
        return 0

    def getPCAddress(self) -> None:
        return None

    def getHighVariable(self) -> _HighVariable:
        return self._high


class _Prototype:
    def __init__(self, parameters: list[_HighSymbol]) -> None:
        self._parameters = parameters

    def getNumParams(self) -> int:
        return len(self._parameters)

    def getParam(self, index: int) -> _HighSymbol:
        return self._parameters[index]


class _HighFunction:
    def __init__(self, parameters: list[_HighSymbol]) -> None:
        self._prototype = _Prototype(parameters)

    def getFunctionPrototype(self) -> _Prototype:
        return self._prototype

    def getPcodeOps(self) -> _Iterator:
        return _Iterator([])


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
