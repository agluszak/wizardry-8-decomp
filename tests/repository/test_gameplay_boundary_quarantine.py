import re
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parents[2]
QUARANTINE = REPOSITORY / "include/wiz8/gameplay_boundaries.h"

# This is intentionally a ceiling, not an expected size: the quarantine may
# shrink without updating the test, but growth requires undoing an extraction.
MAX_QUARANTINE_BYTES = 46_447

ALLOWED_INCLUDES = {
    "random.h",
    "surrender/srMath.h",
    "stddef.h",
    "timer.h",
    "wiz8/3d_code/IList.h",
    "wiz8/3d_code/PList.h",
    "wiz8/character.h",
    "wiz8/combat_state.h",
    "wiz8/game_state.h",
    "wiz8/gameplay_databases.h",
    "wiz8/geometry.h",
    "wiz8/item_tables.h",
    "wiz8/layouts/encounter_tables.h",
    "wiz8/magic.h",
    "wiz8/monster_runtime.h",
    "wiz8/regions.h",
    "wiz8/screen_state.h",
    "wiz8/startup_runtime_state.h",
    "wiz8/ui_state.h",
    "wiz8/utility.h",
    "wiz8/vector.h",
}

EXTRACTED_DEFINITIONS = (
    r"typedef struct W8Position\s*\{",
    r"typedef struct W8TargetSource\s*\{",
    r"typedef struct W8CombatSlot\s*\{",
    r"typedef struct W8PartySlotRow\s*\{",
    r"typedef struct W8MonsterGenerator\s*\{",
    r"typedef struct W8MonsterFormation\s*\{",
    r"typedef struct W8MonsterGroup\s*\{",
    r"struct W8MonsterCycle\s*\{",
    r"struct W8MonsterPolymorphicSubobject18\s*\{",
    r"struct W8Monster\s*\{",
    r"typedef struct W8MonsterCombatState\s*\{",
    r"typedef struct W8MonsterInfo\s*\{",
)


def test_gameplay_boundary_quarantine_only_shrinks() -> None:
    contents = QUARANTINE.read_text()
    assert len(contents.encode()) <= MAX_QUARANTINE_BYTES

    includes = set(re.findall(r'^#include [<"]([^>"]+)[>"]', contents, re.MULTILINE))
    assert includes <= ALLOWED_INCLUDES


def test_extracted_monster_declarations_do_not_return_to_quarantine() -> None:
    contents = QUARANTINE.read_text()
    for definition in EXTRACTED_DEFINITIONS:
        assert re.search(definition, contents) is None


def test_monster_owner_does_not_include_quarantine() -> None:
    monster_source = REPOSITORY / "src/wiz8/engine_code/Monster.cpp"
    assert '"wiz8/gameplay_boundaries.h"' not in monster_source.read_text()
