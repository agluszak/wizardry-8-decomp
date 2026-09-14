from __future__ import annotations

from pathlib import Path

from wiz8decomp.ghidra.unit_intervals import TranslationUnitLayout, UnitAnchor
from wiz8decomp.header_architecture import (
    HeaderArchitectureError,
    analyze_header_architecture,
    header_architecture_violations,
    validate_header_architecture,
)

UNIT_A = r"Local Code\Foo.cpp"
UNIT_B = r"Local Code\Bar.cpp"


def _write_arch(repo: Path, document: str) -> None:
    path = repo / "src/wiz8/header_architecture.json"
    path.parent.mkdir(parents=True)
    path.write_text(document, encoding="utf-8")
    (repo / "evidence/observations/wiz8").mkdir(parents=True)
    (repo / "evidence/observations/wiz8/assertions.csv").write_text(
        "call_site,kind,containing_function,source_path,line,expression,message\n",
        encoding="utf-8",
    )


def test_shared_layout_rejects_behavioral_functions(tmp_path: Path) -> None:
    _write_arch(
        tmp_path,
        '{"schema":"wiz8.header-architecture-v1","headers":{},"proven-original-headers":{}}\n',
    )
    header = tmp_path / "include/wiz8/layouts/state.h"
    header.parent.mkdir(parents=True)
    header.write_text("void DoThing(void); /* 0x00401080 */\n", encoding="utf-8")
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text(
        "// FUNCTION: WIZ8 0x00401080\nvoid DoThing(void) {}\n",
        encoding="utf-8",
    )
    layout = TranslationUnitLayout(
        [UnitAnchor(0x401000, UNIT_A, "assertion"), UnitAnchor(0x401100, UNIT_A, "assertion")]
    )

    violations = header_architecture_violations(tmp_path, layout=layout)

    assert violations[0]["kind"] == "layout-declares-functions"
    assert violations[0]["file"] == "include/wiz8/layouts/state.h"


def test_tu_interface_rejects_two_original_units(tmp_path: Path) -> None:
    _write_arch(
        tmp_path,
        (
            '{"schema":"wiz8.header-architecture-v1","headers":'
            '{"include/wiz8/local_code/Mixed.h":{"role":"tu-interface"}},'
            '"proven-original-headers":{}}\n'
        ),
    )
    header = tmp_path / "include/wiz8/local_code/Mixed.h"
    header.parent.mkdir(parents=True)
    header.write_text(
        "void First(void); /* 0x00401080 */\nvoid Second(void); /* 0x00402080 */\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text(
        "// FUNCTION: WIZ8 0x00401080\nvoid First(void) {}\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/local_code/Bar.cpp").write_text(
        "// FUNCTION: WIZ8 0x00402080\nvoid Second(void) {}\n",
        encoding="utf-8",
    )
    layout = TranslationUnitLayout(
        [
            UnitAnchor(0x401000, UNIT_A, "assertion"),
            UnitAnchor(0x401100, UNIT_A, "assertion"),
            UnitAnchor(0x402000, UNIT_B, "assertion"),
            UnitAnchor(0x402100, UNIT_B, "assertion"),
        ]
    )

    violations = header_architecture_violations(tmp_path, layout=layout)

    assert violations[0]["kind"] == "tu-interface-mixed-units"


def test_reconstructed_header_may_name_its_implementation_tu(tmp_path: Path) -> None:
    _write_arch(
        tmp_path,
        (
            '{"schema":"wiz8.header-architecture-v1","headers":'
            '{"include/wiz8/local_code/Widget.h":{"role":"reconstructed-declarations",'
            '"implementation-tus":["Local Code\\\\Foo.cpp"]}},'
            '"proven-original-headers":{}}\n'
        ),
    )
    header = tmp_path / "include/wiz8/local_code/Widget.h"
    header.parent.mkdir(parents=True)
    header.write_text("void First(void); /* 0x00401080 */\n", encoding="utf-8")
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text(
        "// FUNCTION: WIZ8 0x00401080\nvoid First(void) {}\n",
        encoding="utf-8",
    )
    layout = TranslationUnitLayout(
        [UnitAnchor(0x401000, UNIT_A, "assertion"), UnitAnchor(0x401100, UNIT_A, "assertion")]
    )

    assert header_architecture_violations(tmp_path, layout=layout) == []
    result = validate_header_architecture(tmp_path)
    assert result["ok"] is True


def test_unclassified_mixed_headers_are_reported_not_gated(tmp_path: Path) -> None:
    _write_arch(
        tmp_path,
        '{"schema":"wiz8.header-architecture-v1","headers":{},"proven-original-headers":{}}\n',
    )
    header = tmp_path / "include/wiz8/bundle.h"
    header.parent.mkdir(parents=True)
    header.write_text(
        "void First(void); /* 0x00401080 */\nvoid Second(void); /* 0x00402080 */\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text(
        "// FUNCTION: WIZ8 0x00401080\nvoid First(void) {}\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/local_code/Bar.cpp").write_text(
        "// FUNCTION: WIZ8 0x00402080\nvoid Second(void) {}\n",
        encoding="utf-8",
    )
    layout = TranslationUnitLayout(
        [
            UnitAnchor(0x401000, UNIT_A, "assertion"),
            UnitAnchor(0x401100, UNIT_A, "assertion"),
            UnitAnchor(0x402000, UNIT_B, "assertion"),
            UnitAnchor(0x402100, UNIT_B, "assertion"),
        ]
    )

    report = analyze_header_architecture(tmp_path, layout=layout)
    assert report["violations"] == []
    assert report["unclassified_mixed"][0]["file"] == "include/wiz8/bundle.h"


def test_proven_hpp_spelling_is_required(tmp_path: Path) -> None:
    _write_arch(
        tmp_path,
        (
            '{"schema":"wiz8.header-architecture-v1","headers":{},'
            '"proven-original-headers":{"include/wiz8/engine_code/AnimRep.hpp":'
            '"Engine Code\\\\Include\\\\AnimRep.hpp"}}\n'
        ),
    )
    header = tmp_path / "include/wiz8/engine_code/AnimRep.h"
    header.parent.mkdir(parents=True)
    header.write_text("struct AnimRep;\n", encoding="utf-8")

    try:
        validate_header_architecture(tmp_path)
    except HeaderArchitectureError as exc:
        assert "proven-header-spelling" in str(exc)
    else:
        raise AssertionError("expected proven-header-spelling")


def test_shared_layout_rejects_member_functions(tmp_path: Path) -> None:
    _write_arch(
        tmp_path,
        '{"schema":"wiz8.header-architecture-v1","headers":{},"proven-original-headers":{}}\n',
    )
    header = tmp_path / "include/wiz8/layouts/state.h"
    header.parent.mkdir(parents=True)
    header.write_text(
        "struct State {\n    State() {}\n    void Set(int value);\n    int value;\n};\n",
        encoding="utf-8",
    )

    violations = header_architecture_violations(tmp_path, layout=None)

    assert violations[0]["kind"] == "layout-declares-functions"
    assert "State::Set" in violations[0]["functions"]
    assert "State::State" in violations[0]["functions"]


def test_tu_interface_member_declarations_map_to_their_unit(tmp_path: Path) -> None:
    _write_arch(
        tmp_path,
        (
            '{"schema":"wiz8.header-architecture-v1","headers":'
            '{"include/wiz8/local_code/Widget.h":{"role":"tu-interface"}},'
            '"proven-original-headers":{}}\n'
        ),
    )
    header = tmp_path / "include/wiz8/local_code/Widget.h"
    header.parent.mkdir(parents=True)
    header.write_text(
        "struct Widget {\n    void Reset(void); /* 0x00401080 */\n};\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text(
        "// FUNCTION: WIZ8 0x00401080\nvoid Widget::Reset(void) {}\n",
        encoding="utf-8",
    )
    layout = TranslationUnitLayout(
        [UnitAnchor(0x401000, UNIT_A, "assertion"), UnitAnchor(0x401100, UNIT_A, "assertion")]
    )

    assert header_architecture_violations(tmp_path, layout=layout) == []


def test_tu_interface_member_declaration_from_another_unit_fails(tmp_path: Path) -> None:
    _write_arch(
        tmp_path,
        (
            '{"schema":"wiz8.header-architecture-v1","headers":'
            '{"include/wiz8/local_code/Widget.h":{"role":"tu-interface"}},'
            '"proven-original-headers":{}}\n'
        ),
    )
    header = tmp_path / "include/wiz8/local_code/Widget.h"
    header.parent.mkdir(parents=True)
    header.write_text(
        "struct Widget {\n"
        "    void Reset(void); /* 0x00401080 */\n"
        "    void Load(void); /* 0x00402080 */\n"
        "};\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text(
        "// FUNCTION: WIZ8 0x00401080\nvoid Widget::Reset(void) {}\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/local_code/Bar.cpp").write_text(
        "// FUNCTION: WIZ8 0x00402080\nvoid Widget::Load(void) {}\n",
        encoding="utf-8",
    )
    layout = TranslationUnitLayout(
        [
            UnitAnchor(0x401000, UNIT_A, "assertion"),
            UnitAnchor(0x401100, UNIT_A, "assertion"),
            UnitAnchor(0x402000, UNIT_B, "assertion"),
            UnitAnchor(0x402100, UNIT_B, "assertion"),
        ]
    )

    violations = header_architecture_violations(tmp_path, layout=layout)

    assert violations[0]["kind"] == "tu-interface-mixed-units"


def test_unknown_role_is_rejected(tmp_path: Path) -> None:
    _write_arch(
        tmp_path,
        (
            '{"schema":"wiz8.header-architecture-v1","headers":'
            '{"include/wiz8/local_code/Widget.h":{"role":"tu-interfaces"}},'
            '"proven-original-headers":{}}\n'
        ),
    )

    try:
        validate_header_architecture(tmp_path)
    except HeaderArchitectureError as exc:
        assert "tu-interfaces" in str(exc)
    else:
        raise AssertionError("expected unknown role rejection")


def test_proven_header_does_not_derive_a_role(tmp_path: Path) -> None:
    _write_arch(
        tmp_path,
        (
            '{"schema":"wiz8.header-architecture-v1","headers":{},'
            '"proven-original-headers":{"include/wiz8/engine_code/Heap.hpp":'
            '"Engine Code\\\\Include\\\\Heap.hpp"}}\n'
        ),
    )
    header = tmp_path / "include/wiz8/engine_code/Heap.hpp"
    header.parent.mkdir(parents=True)
    header.write_text(
        "template <class T> class Heap {\n"
        "public:\n"
        "    void Insert(const T* entry);\n"
        "};\n"
        "template <class T> void Heap<T>::Insert(const T* entry) {}\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text(
        "// FUNCTION: WIZ8 0x00401080\nvoid Other(void) {}\n",
        encoding="utf-8",
    )

    report = analyze_header_architecture(tmp_path, layout=None)

    record = next(h for h in report["headers"] if h["file"].endswith("Heap.hpp"))
    assert record["role"] == "unclassified"
    assert report["violations"] == []


def test_shared_layout_may_not_include_an_interface_header(tmp_path: Path) -> None:
    _write_arch(
        tmp_path,
        '{"schema":"wiz8.header-architecture-v1","headers":{},"proven-original-headers":{}}\n',
    )
    layout_header = tmp_path / "include/wiz8/layouts/state.h"
    layout_header.parent.mkdir(parents=True)
    layout_header.write_text('#include "wiz8/local_code/Widget.h"\n', encoding="utf-8")
    interface = tmp_path / "include/wiz8/local_code/Widget.h"
    interface.parent.mkdir(parents=True)
    interface.write_text("void DoThing(void);\n", encoding="utf-8")

    violations = header_architecture_violations(tmp_path, layout=None)

    assert violations[0]["kind"] == "layout-includes-interface"
    assert violations[0]["include"] == "include/wiz8/local_code/Widget.h"


def test_block_comments_are_not_scanned_as_declarations(tmp_path: Path) -> None:
    from wiz8decomp.header_architecture import scan_header_function_declarations

    header = tmp_path / "include/wiz8/local_code/MagicEffects.h"
    header.parent.mkdir(parents=True)
    header.write_text(
        "/* IsScreenBusy at 0x00554540 precedes Formation & Facing's\n"
        "   assertion-backed hull (0x005545F0). */\n"
        "unsigned char IsScreenBusy(void);\n",
        encoding="utf-8",
    )

    declarations = scan_header_function_declarations(header, tmp_path)

    assert [item["name"] for item in declarations] == ["IsScreenBusy"]
    assert declarations[0]["address"] is None
