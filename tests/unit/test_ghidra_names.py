from pathlib import Path

from wiz8decomp.ghidra.project import program_name
from wiz8decomp.ghidra.resolve import ResolveError, hex_address, resolve_function
from wiz8decomp.source_index import AddressBoundIdentity, address_bound_identities


def test_program_name_is_stable_and_hash_qualified() -> None:
    module = {"variant": "gog-base", "relative_path": "Dll/Something.dll", "sha256": "a" * 64}
    assert program_name(module) == "wiz8--gog-base--something--aaaaaaaaaaaa"


def test_hex_address_zero_pads() -> None:
    assert hex_address(0x51EB90) == "0x0051eb90"


def test_ambiguous_function_name_lists_candidates() -> None:
    class Function:
        def __init__(self, name: str, entry: int) -> None:
            self._name = name
            self._entry = entry

        def getName(self, qualified: bool = False) -> str:
            return self._name

        def getEntryPoint(self):
            class Address:
                def getOffset(inner_self) -> int:
                    return entry

            entry = self._entry
            return Address()

    class Manager:
        def getFunctions(self, _forward: bool):
            class Iterator:
                def __init__(self) -> None:
                    self.values = [Function("Draw", 0x401000), Function("Draw", 0x402000)]

                def hasNext(self) -> bool:
                    return bool(self.values)

                def next(self) -> Function:
                    return self.values.pop(0)

            return Iterator()

        def getFunctionAt(self, _address):
            return None

        def getFunctionContaining(self, _address):
            return None

    class Program:
        def getFunctionManager(self):
            return Manager()

        def getAddressFactory(self):
            class Factory:
                def getAddress(self, _text):
                    raise RuntimeError("not an address")

                def getDefaultAddressSpace(self):
                    class Space:
                        def getAddress(self, _text):
                            return None

                    return Space()

            return Factory()

    try:
        resolve_function(Program(), "Draw")
    except ResolveError as error:
        assert "ambiguous" in str(error)
        assert "0x00401000" in str(error)
        assert "0x00402000" in str(error)
    else:
        raise AssertionError("expected ResolveError")


def test_declaration_only_address_binding(tmp_path: Path) -> None:
    header = tmp_path / "include/wiz8/local_code/PC_Item.h"
    header.parent.mkdir(parents=True)
    header.write_text(
        "void EquipMatchingPartnerItem(W8Character* character, W8ItemInstance* item, "
        "int item_id, int equip_slot);                               /* 0x0051EB90 */\n",
        encoding="utf-8",
    )
    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    source-root: include/wiz8\n"
        "    hash:\n      sha256: abc\n",
        encoding="utf-8",
    )
    build = tmp_path / "build"
    build.mkdir()
    (build / "source-index.json").write_text(
        __import__("json").dumps(
            {
                "schema": "reccmp-source-index-v3",
                "markers": [],
                "declarations": [
                    {
                        "qualified_name": "EquipMatchingPartnerItem",
                        "semantic_id": "?EquipMatchingPartnerItem@@YAXPAVW8Character@@PAUW8ItemInstance@@HH@Z",
                        "source_signature": (
                            "void EquipMatchingPartnerItem(W8Character *, W8ItemInstance *, int, int)"
                        ),
                        "calling_convention": "__cdecl",
                        "return_type": "void",
                        "parameter_types": [
                            "W8Character *",
                            "W8ItemInstance *",
                            "int",
                            "int",
                        ],
                        "has_this": False,
                        "source_file": "include/wiz8/local_code/PC_Item.h",
                        "line": 1,
                        "end_line": 1,
                        "is_definition": False,
                    }
                ],
                "classes": [],
                "variables": [],
                "conflicts": [],
            }
        ),
        encoding="utf-8",
    )

    bound = address_bound_identities(tmp_path, "WIZ8")
    identities = bound[0x0051EB90]
    assert identities
    identity = identities[0]
    assert isinstance(identity, AddressBoundIdentity)
    assert identity.kind == "declaration"
    assert identity.name == "EquipMatchingPartnerItem"
    assert identity.source_file == "include/wiz8/local_code/PC_Item.h"
    assert identity.is_definition is False


def test_v3_declaration_key_joins_marker_to_clang_declaration(tmp_path: Path) -> None:
    from wiz8decomp.source_index import bind_marker_declarations, declaration_for_marker

    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    source-root: src/wiz8\n"
        "    hash:\n      sha256: abc\n",
        encoding="utf-8",
    )
    semantic = "?Draw@@YAXXZ"
    document = {
        "schema": "reccmp-source-index-v3",
        "markers": [
            {
                "marker_kind": "FUNCTION",
                "target": "WIZ8",
                "address": 0x401000,
                "source_file": "src/wiz8/draw.cpp",
                "line": 10,
                "declaration_key": ["WIZ8", semantic],
            }
        ],
        "declarations": [
            {
                "target": "WIZ8",
                "qualified_name": "Draw",
                "semantic_id": semantic,
                "source_signature": "void Draw(void)",
                "source_file": "src/wiz8/draw.cpp",
                "line": 11,
                "end_line": 20,
                "is_definition": True,
                "owning_class": None,
            }
        ],
    }
    build = tmp_path / "build"
    build.mkdir()
    (build / "source-index.json").write_text(__import__("json").dumps(document), encoding="utf-8")

    bound = address_bound_identities(tmp_path, "WIZ8")
    identity = bound[0x401000][0]
    assert identity.name == "Draw"
    assert identity.kind == "definition"
    assert identity.source_signature == "void Draw(void)"

    markers = bind_marker_declarations(document)
    assert markers[0]["declaration"]["end_line"] == 20
    assert (
        declaration_for_marker(
            document["markers"][0], {("WIZ8", semantic): document["declarations"][0]}
        )["source_signature"]
        == "void Draw(void)"
    )
