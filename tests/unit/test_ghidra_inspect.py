from types import SimpleNamespace

from wiz8decomp.ghidra.inspect import _defects, decompiled_text_with_defects
from wiz8decomp.ghidra.resolve import _component_path, _is_import_cell, _join_access


def _high(parameters: int) -> SimpleNamespace:
    """A decompiler result with no local symbols and a fixed prototype arity."""
    symbols = SimpleNamespace(hasNext=lambda: False)
    return SimpleNamespace(
        getLocalSymbolMap=lambda: SimpleNamespace(getSymbols=lambda: symbols),
        getFunctionPrototype=lambda: SimpleNamespace(getNumParams=lambda: parameters),
    )


def test_candidate_carries_parameter_defects() -> None:
    function = SimpleNamespace(getParameterCount=lambda: 0)
    high = _high(0)
    assert _defects(function, "void fn(void) {}", high) == []

    unused = SimpleNamespace(getParameterCount=lambda: 4)
    unused_high = _high(3)
    defects = _defects(unused, "void fn(void)\n{\n  int in_stack_00000010;\n}\n", unused_high)
    kinds = {row["kind"] for row in defects}
    assert "parameter-count-mismatch" in kinds
    assert "phantom-stack-variable" in kinds
    text = decompiled_text_with_defects("void fn() {}\n", defects)
    assert text is not None
    assert "// defect: parameter-count-mismatch:" in text
    assert "// defect: phantom-stack-variable: in_stack_00000010" in text


def test_source_prototype_mismatch_is_a_defect() -> None:
    function = SimpleNamespace(getParameterCount=lambda: 0)
    high = _high(0)
    identity = SimpleNamespace(kind="definition", parameter_types=("Node *", "int", "int", "int"))
    defects = _defects(function, "void fn(void) {}", high, (identity,))
    assert any(row["kind"] == "programdb-prototype-empty" for row in defects)
    text = decompiled_text_with_defects("void fn(void) {}", defects)
    assert text is not None
    assert "// defect: programdb-prototype-empty:" in text
    assert "Source declaration: 4 explicit arguments" in text
    assert "ProgramDB prototype: 0 explicit arguments" in text
    assert "do not infer argument order from this C" in text


def test_adjusted_this_is_not_an_explicit_source_argument() -> None:
    receiver = SimpleNamespace(
        getName=lambda: "this",
        getVariableStorage=lambda: "ECX:4",
        isAutoParameter=lambda: False,
    )
    row = SimpleNamespace(isAutoParameter=lambda: False)
    identity = SimpleNamespace(
        kind="definition",
        semantic_id="member:OpenCampForSelectedMember",
        has_this=True,
        parameter_types=("int",),
        calling_convention="__thiscall",
    )
    function = SimpleNamespace(
        getParameters=lambda: [receiver, row],
        getCallingConventionName=lambda: "__thiscall",
    )
    assert _defects(function, "", None, (identity,)) == []

    extra = SimpleNamespace(isAutoParameter=lambda: False)
    function.getParameters = lambda: [receiver, row, extra]
    assert any(
        defect["kind"] == "source-parameter-count-mismatch"
        for defect in _defects(function, "", None, (identity,))
    )


def test_component_path_preserves_array_and_union() -> None:
    class Component:
        def __init__(self, name, offset, length, data_type):
            self._name = name
            self._offset = offset
            self._length = length
            self._data_type = data_type

        def getFieldName(self):
            return self._name

        def getOffset(self):
            return self._offset

        def getLength(self):
            return self._length

        def getDataType(self):
            return self._data_type

    class Structure:
        def __init__(self, length, components, name="S"):
            self._length = length
            self._components = components
            self._name = name

        def getLength(self):
            return self._length

        def getDefinedComponents(self):
            return self._components

        def getDisplayName(self):
            return self._name

    class Array:
        def __init__(self, count, element, element_length):
            self._count = count
            self._element = element
            self._element_length = element_length

        def getLength(self):
            return self._count * self._element_length

        def getNumElements(self):
            return self._count

        def getElementLength(self):
            return self._element_length

        def getDataType(self):
            return self._element

        def getDisplayName(self):
            return "Array"

    class Union(Structure):
        pass

    scalar = SimpleNamespace(getLength=lambda: 4, getDisplayName=lambda: "float")
    position = Structure(12, [Component("x", 8, 4, scalar)], "Vec")
    entry = Structure(16, [Component("position", 4, 12, position)], "Entry")
    entries = Array(4, entry, 16)
    root = Structure(64, [Component("entries", 0, 64, entries)])
    hit = _component_path(root, 3 * 16 + 4 + 8)
    assert hit is not None
    assert hit.path == "entries[3].position.x"
    assert hit.leaf == "x"
    assert _join_access("g_monster_record_cache", "[0]") == "g_monster_record_cache[0]"
    assert _join_access("g_party", "characters[2].position.x") == "g_party.characters[2].position.x"
    union = Union(4, [Component("as_int", 0, 4, scalar), Component("as_ptr", 0, 4, scalar)])
    owner = Structure(4, [Component("u", 0, 4, union)])
    ambiguous = _component_path(owner, 0)
    assert ambiguous is not None
    assert ambiguous.union_members == ("u.as_int", "u.as_ptr")
    program = SimpleNamespace(
        getMemory=lambda: SimpleNamespace(
            getBlock=lambda _addr: SimpleNamespace(getName=lambda: ".data")
        ),
        getReferenceManager=lambda: SimpleNamespace(getReferencesFrom=lambda _addr: []),
        getSymbolTable=lambda: SimpleNamespace(
            getPrimarySymbol=lambda _addr: SimpleNamespace(isExternal=lambda: False)
        ),
    )
    assert _is_import_cell(program, object()) is False


def test_named_source_abi_defects() -> None:
    high = _high(1)
    function = SimpleNamespace(
        getParameterCount=lambda: 1,
        getCallingConventionName=lambda: "__cdecl",
        getReturnType=lambda: SimpleNamespace(getDisplayName=lambda: "int"),
    )
    identity = SimpleNamespace(
        kind="definition",
        parameter_types=("int",),
        semantic_id="?nudge@Node@@QAEXH@Z",
        calling_convention="__thiscall",
        has_this=True,
        return_type="void",
        name="Node::nudge",
    )
    kinds = {row["kind"] for row in _defects(function, "void fn(int a) {}", high, (identity,))}
    assert "source-calling-convention-mismatch" in kinds
    assert "source-return-type-mismatch" in kinds
    uchar = SimpleNamespace(
        kind="definition",
        parameter_types=("int",),
        semantic_id="?IsLevelCdMissing@@YAEH@Z",
        calling_convention="__cdecl",
        return_type="unsigned char",
        name="IsLevelCdMissing",
    )
    uchar_fn = SimpleNamespace(
        getParameterCount=lambda: 1,
        getCallingConventionName=lambda: "__cdecl",
        getReturnType=lambda: SimpleNamespace(getDisplayName=lambda: "uchar"),
    )
    uchar_kinds = {row["kind"] for row in _defects(uchar_fn, "uchar fn(int a) {}", high, (uchar,))}
    assert "source-return-type-mismatch" not in uchar_kinds

    unresolved = SimpleNamespace(kind="declaration", name="Mystery", parameter_types=())
    empty_high = _high(0)
    empty_fn = SimpleNamespace(getParameterCount=lambda: 0)
    unresolved_defects = _defects(empty_fn, "void Mystery(void) {}", empty_high, (unresolved,))
    assert any(row["kind"] == "source-declaration-unresolved" for row in unresolved_defects)
    text = decompiled_text_with_defects("void Mystery(void) {}", unresolved_defects)
    assert text is not None
    assert "// defect: source-declaration-unresolved:" in text
