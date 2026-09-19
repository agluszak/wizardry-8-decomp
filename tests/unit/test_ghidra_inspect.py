from pathlib import Path
from types import SimpleNamespace

from typer.testing import CliRunner
from wiz8decomp.cli import app
from wiz8decomp.source_index import source_index_freshness, try_load_source_index


def test_try_load_source_index_missing(tmp_path: Path) -> None:
    assert try_load_source_index(tmp_path) is None


def test_source_index_freshness_missing(tmp_path: Path) -> None:
    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    source-root: src/wiz8\n"
        "    hash:\n      sha256: abc\n",
        encoding="utf-8",
    )
    freshness = source_index_freshness(tmp_path, "WIZ8")
    assert freshness["state"] == "missing"


def test_ghidra_read_commands_are_registered() -> None:
    runner = CliRunner()
    result = runner.invoke(app, ["ghidra", "--help"])
    assert result.exit_code == 0, result.output
    assert "decompile" in result.output
    assert "asm" in result.output
    assert "sym" in result.output
    assert "sync" in result.output
    assert "class" in result.output
    assert "flow" in result.output


def test_retired_commands_are_gone() -> None:
    runner = CliRunner()
    assert runner.invoke(app, ["recover", "function", "--help"]).exit_code != 0
    assert runner.invoke(app, ["recover", "explain", "--help"]).exit_code != 0
    assert runner.invoke(app, ["report", "context", "--help"]).exit_code != 0
    assert runner.invoke(app, ["report", "instructions", "--help"]).exit_code != 0
    assert runner.invoke(app, ["report", "data", "--help"]).exit_code != 0
    assert runner.invoke(app, ["report", "class", "--help"]).exit_code != 0
    assert runner.invoke(app, ["report", "flow", "--help"]).exit_code != 0
    assert runner.invoke(app, ["analyze", "enrichment-checkpoint", "--help"]).exit_code != 0
    assert runner.invoke(app, ["analyze", "prototype-repair", "--help"]).exit_code != 0
    assert runner.invoke(app, ["analyze", "parameter-id", "--help"]).exit_code == 0


def test_inspect_does_not_write_source_index(monkeypatch) -> None:
    import contextlib

    from wiz8decomp import source_index as source_index_module
    from wiz8decomp.ghidra import env as env_module
    from wiz8decomp.ghidra import inspect
    from wiz8decomp.ghidra import workspace as workspace_module
    from wiz8decomp.ghidra.resolve import ResolveError

    events: list[str] = []
    monkeypatch.setattr(
        source_index_module,
        "write_source_index",
        lambda *_args, **_kwargs: events.append("write") or {},
    )
    monkeypatch.setattr(inspect, "resolve_program_selector", lambda *_args: "wiz8")
    monkeypatch.setattr(
        inspect,
        "source_metadata",
        lambda *_args: {"state": "missing", "detail": "absent", "available": False},
    )
    monkeypatch.setattr(
        workspace_module,
        "seed_record",
        lambda *_args, **_kwargs: {"program": "wiz8", "sha256": "x"},
    )
    monkeypatch.setattr(
        workspace_module,
        "project_seed_freshness",
        lambda *_args: {"status": "current", "detail": None},
    )
    monkeypatch.setattr(
        env_module, "open_program", lambda *_args, **_kwargs: contextlib.nullcontext(object())
    )
    monkeypatch.setattr(
        inspect,
        "resolve_function",
        lambda *_args, **_kwargs: (_ for _ in ()).throw(
            ResolveError("no function contains 0x00529570")
        ),
    )

    result = inspect.decompile_functions(
        SimpleNamespace(repo_dir=Path("/repo"), build_dir=Path("/repo/build")),
        ["0x00529570"],
        include_candidate=False,
    )
    assert "write" not in events
    assert result["failures"]
    assert result["failures"][0]["candidate"] is None


def test_candidate_carries_parameter_defects() -> None:
    from wiz8decomp.ghidra.inspect import _defects, candidate_text_with_defects

    class EmptySymbols:
        def hasNext(self):
            return False

        def next(self):
            raise StopIteration

    function = SimpleNamespace(getParameterCount=lambda: 0)
    high = SimpleNamespace(
        getLocalSymbolMap=lambda: SimpleNamespace(getSymbols=lambda: EmptySymbols()),
        getFunctionPrototype=lambda: SimpleNamespace(getNumParams=lambda: 0),
    )
    assert _defects(function, "void fn(void) {}", high) == []

    unused = SimpleNamespace(getParameterCount=lambda: 4)
    unused_high = SimpleNamespace(
        getLocalSymbolMap=lambda: SimpleNamespace(getSymbols=lambda: EmptySymbols()),
        getFunctionPrototype=lambda: SimpleNamespace(getNumParams=lambda: 3),
    )
    defects = _defects(unused, "void fn(void)\n{\n  int in_stack_00000010;\n}\n", unused_high)
    kinds = {row["kind"] for row in defects}
    assert "parameter-count-mismatch" in kinds
    assert "phantom-stack-variable" in kinds
    text = candidate_text_with_defects("void fn() {}\n", defects)
    assert text is not None
    assert "// defect: parameter-count-mismatch:" in text
    assert "// defect: phantom-stack-variable: in_stack_00000010" in text


def test_source_prototype_mismatch_is_a_defect() -> None:
    from types import SimpleNamespace

    from wiz8decomp.ghidra.inspect import _defects, candidate_text_with_defects

    class EmptySymbols:
        def hasNext(self):
            return False

        def next(self):
            raise StopIteration

    function = SimpleNamespace(getParameterCount=lambda: 0)
    high = SimpleNamespace(
        getLocalSymbolMap=lambda: SimpleNamespace(getSymbols=lambda: EmptySymbols()),
        getFunctionPrototype=lambda: SimpleNamespace(getNumParams=lambda: 0),
    )
    identity = SimpleNamespace(kind="definition", parameter_types=("Node *", "int", "int", "int"))
    defects = _defects(function, "void fn(void) {}", high, (identity,))
    assert any(row["kind"] == "programdb-prototype-empty" for row in defects)
    text = candidate_text_with_defects("void fn(void) {}", defects)
    assert text is not None
    assert "// defect: programdb-prototype-empty:" in text
    assert "Source declaration: 4 explicit arguments" in text
    assert "ProgramDB prototype: 0 explicit arguments" in text
    assert "do not infer argument order from this C" in text


def test_decompile_text_shows_code_not_json() -> None:
    from wiz8decomp.ghidra.inspect import format_decompile_text

    payload = {
        "source_index": {"state": "current"},
        "functions": [
            {
                "entry": "0x00500000",
                "status": "ok",
                "native": {
                    "qualified_name": "Node::link",
                    "prototype": "void link(void)",
                    "calling_convention": "__thiscall",
                    "decompiled": "void Node::link(void)\n{\n  return;\n}\n",
                    "parameters": [],
                },
                "source": {
                    "state": "current",
                    "identities": [
                        {
                            "name": "Node::link",
                            "source_file": "src/wiz8/Node.cpp",
                            "line": 12,
                            "signature": "void Node::link(Node * other, int flags)",
                        }
                    ],
                },
                "defects": [
                    {
                        "kind": "programdb-prototype-empty",
                        "detail": (
                            "Source declaration: 2 explicit arguments. "
                            "ProgramDB prototype: 0 explicit arguments. "
                            "Argument recovery is inconsistent; do not infer argument order from this C."
                        ),
                    }
                ],
                "artifacts": {"c": "build/ghidra/decompile/wiz8/00500000.c"},
            }
        ],
    }
    text = format_decompile_text(payload)
    assert "void Node::link(void)" in text
    assert "Source declaration: void Node::link" in text
    assert "ProgramDB prototype: void link(void)  __thiscall" in text
    assert "Source index: current" in text
    assert '"decompiled"' not in text


def test_component_path_preserves_array_and_union() -> None:
    from types import SimpleNamespace

    from wiz8decomp.ghidra.resolve import _component_path, _is_import_cell, _join_access

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
    from wiz8decomp.ghidra.inspect import _defects, candidate_text_with_defects

    class EmptySymbols:
        def hasNext(self):
            return False

        def next(self):
            raise StopIteration

    high = SimpleNamespace(
        getLocalSymbolMap=lambda: SimpleNamespace(getSymbols=lambda: EmptySymbols()),
        getFunctionPrototype=lambda: SimpleNamespace(getNumParams=lambda: 1),
    )
    function = SimpleNamespace(
        getParameterCount=lambda: 1,
        getCallingConventionName=lambda: "__cdecl",
        getReturnType=lambda: SimpleNamespace(getDisplayName=lambda: "int"),
    )
    identity = SimpleNamespace(
        kind="definition",
        parameter_types=("int",),
        source_signature="void Node::nudge(int flags)",
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
        source_signature="unsigned char IsLevelCdMissing0042B6F0(int level)",
        calling_convention="__cdecl",
        return_type="unsigned char",
        name="IsLevelCdMissing0042B6F0",
    )
    uchar_fn = SimpleNamespace(
        getParameterCount=lambda: 1,
        getCallingConventionName=lambda: "__cdecl",
        getReturnType=lambda: SimpleNamespace(getDisplayName=lambda: "uchar"),
    )
    uchar_kinds = {row["kind"] for row in _defects(uchar_fn, "uchar fn(int a) {}", high, (uchar,))}
    assert "source-return-type-mismatch" not in uchar_kinds

    unresolved = SimpleNamespace(kind="declaration", name="Mystery", parameter_types=())
    empty_high = SimpleNamespace(
        getLocalSymbolMap=lambda: SimpleNamespace(getSymbols=lambda: EmptySymbols()),
        getFunctionPrototype=lambda: SimpleNamespace(getNumParams=lambda: 0),
    )
    empty_fn = SimpleNamespace(getParameterCount=lambda: 0)
    unresolved_defects = _defects(empty_fn, "void Mystery(void) {}", empty_high, (unresolved,))
    assert any(row["kind"] == "source-signature-unresolved" for row in unresolved_defects)
    text = candidate_text_with_defects("void Mystery(void) {}", unresolved_defects)
    assert text is not None
    assert "// defect: source-signature-unresolved:" in text


def test_decompile_text_keeps_success_when_batch_has_failure() -> None:
    from wiz8decomp.ghidra.inspect import format_decompile_text

    payload = {
        "ok": False,
        "source_index": {"state": "missing", "detail": "no reccmp source index"},
        "functions": [
            {
                "entry": "0x00500000",
                "status": "ok",
                "native": {
                    "qualified_name": "ok_fn",
                    "prototype": "void ok_fn(void)",
                    "calling_convention": "__cdecl",
                    "decompiled": "void ok_fn(void)\n{\n  return;\n}\n",
                    "parameters": [],
                },
                "source": {"state": "missing", "detail": "unavailable", "identities": []},
                "defects": [],
                "artifacts": {"c": "build/ghidra/decompile/wiz8/00500000.c"},
            },
            {
                "entry": "0x00500010",
                "status": "decompile-failed",
                "native": {
                    "qualified_name": "bad_fn",
                    "error": "decompiler timeout",
                    "decompiled": None,
                    "parameters": [],
                },
                "source": {"state": "missing", "identities": []},
                "defects": [],
                "listing": ["00500010  ret"],
                "artifacts": {"listing": "build/ghidra/decompile/wiz8/00500010.asm"},
            },
        ],
        "failures": [{"selector": "0x00500010", "error": "decompiler timeout"}],
    }
    text = format_decompile_text(payload)
    assert "void ok_fn(void)" in text
    assert "Source index: missing" in text
    assert "Source metadata: missing" in text
    assert "Decompiler error: decompiler timeout" in text
    assert "00500010  ret" in text


def test_parse_address_span_and_class_text() -> None:
    from wiz8decomp.ghidra.inspect import format_class_text
    from wiz8decomp.ghidra.resolve import parse_address_span

    assert parse_address_span("0xA") == (10, 10)
    assert parse_address_span("0xA:0x20") == (10, 32)
    assert parse_address_span("Node::link") is None
    text = format_class_text(
        {
            "classes": [
                {
                    "name": "Derived",
                    "path": "/Derived",
                    "size": 0x28,
                    "fields": [{"field": "vfptr", "offset": 0, "type": "void *", "length": 4}],
                    "unknown": [{"offset": 0x24, "length": 4}],
                    "bases": [
                        {
                            "field": "base",
                            "offset": 0x20,
                            "base": "Base",
                            "vtable": "0x500010",
                        }
                    ],
                    "vfptrs": [{"field": "vfptr", "offset": 0, "type": "void *"}],
                    "vbptrs": [],
                }
            ],
            "vtables": [
                {
                    "name": "Derived::vftable{for Base}",
                    "address": "0x00500010",
                    "size": 8,
                    "role": "base",
                    "slots": [{"offset": 0, "field": "slot0", "contract": "void f()"}],
                }
            ],
        }
    )
    assert "Derived  size=40" in text
    assert "base Base at +0x20  vtable=0x500010" in text
    assert "vfptr vfptr at +0x0" in text
    assert "base" in text
