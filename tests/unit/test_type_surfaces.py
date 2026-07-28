"""Canonical packed-layout ownership and field-level drift checks."""

from __future__ import annotations

import re
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[2]
LAYOUTS = REPOSITORY / "include/wiz8/layouts"


def _text(name: str) -> str:
    return (LAYOUTS / name).read_text(encoding="utf-8")


def _field(text: str, name: str, *, offset: int | None = None) -> tuple[str, int | None, int]:
    pattern = re.compile(
        rf"^\s*(?P<type>[\w ]+?)(?:\s*\*)?\s+{re.escape(name)}"
        rf"(?:\[(?P<count>0x[0-9a-fA-F]+|\d+)\])?;\s*/\*\s*0x(?P<offset>[0-9a-fA-F]+)",
        re.MULTILINE,
    )
    matches = list(pattern.finditer(text))
    if offset is not None:
        matches = [match for match in matches if int(match.group("offset"), 16) == offset]
    assert matches, f"canonical field {name} is missing"
    match = matches[0]
    count = int(match.group("count"), 0) if match.group("count") else None
    return match.group("type").strip(), count, int(match.group("offset"), 16)


def test_catalogue_headers_do_not_redeclare_compiled_records() -> None:
    gameplay = (REPOSITORY / "config/types/wiz8/gameplay_databases.h").read_text(encoding="utf-8")
    encounter = (REPOSITORY / "config/types/wiz8/encounter_tables.h").read_text(encoding="utf-8")
    assert "include/wiz8/layouts/gameplay_databases.h" in gameplay
    assert "include/wiz8/layouts/item_tables.h" in gameplay
    assert "typedef struct W8ItemDatabaseRecord" not in gameplay
    assert "typedef struct W8MonsterDatabaseRecord" not in gameplay
    assert "include/wiz8/layouts/encounter_tables.h" in encounter
    assert "typedef struct W8EncounterByteVector" not in encounter


def test_monster_attribute_array_and_alias_have_one_inventory() -> None:
    text = _text("gameplay_databases.h")
    assert _field(text, "attribute_values_d1") == ("unsigned char", 5, 0xD1)
    assert "typedef W8MonsterRecord W8MonsterDatabaseRecord;" in text
    assert re.search(r"}\s*W8MonsterRecord;\s*/\*\s*0x297\s*\*/", text)


def test_item_fields_keep_offsets_widths_and_array_extents() -> None:
    text = _text("item_tables.h")
    assert _field(text, "display_name") == ("W8WideChar", 30, 0x000)
    assert _field(text, "equip_class") == ("unsigned char", None, 0x03E)
    assert _field(text, "value") == ("unsigned int", None, 0x086)
    assert _field(text, "weight", offset=0x08A) == ("unsigned short", None, 0x08A)
    assert _field(text, "internal_name") == ("char", 0x40, 0x08D)
    assert re.search(r"}\s*W8ItemDatabaseRecord;\s*/\*\s*0x10d\s*\*/", text)


def test_encounter_vector_starts_with_exact_vtable_identity() -> None:
    text = _text("encounter_tables.h")
    assert re.search(r"^\s*void\s*\*vtable;\s*/\*\s*0x00\s*\*/", text, re.MULTILINE)
    assert _field(text, "count") == ("int", None, 0x04)
    assert _field(text, "capacity") == ("int", None, 0x08)
    assert re.search(r"^\s*unsigned char\s*\*values;\s*/\*\s*0x0c\s*\*/", text, re.MULTILINE)
    assert re.search(r"}\s*W8EncounterByteVector;\s*/\*\s*0x10\s*\*/", text)


def test_replay_imports_the_canonical_layout_headers() -> None:
    replay = (REPOSITORY / "tools/wiz8decomp/ghidra/apply_wiz8_format_model.py").read_text(
        encoding="utf-8"
    )
    for name in ("item_tables.h", "encounter_tables.h"):
        assert f'"include/wiz8/layouts/{name}"' in replay
    assert '"disposition_cache_factor"' not in replay
    assert '(0x00, dword, "unknown_00"' not in replay
