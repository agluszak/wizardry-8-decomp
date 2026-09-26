#include "wiz8/local_screens/mipe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "input.h"

#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/dialog_code/AssayDialog.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/stCube.h"
#include "wiz8/float_constants.h"
#include "wiz8/item_spawning.h"
#include "wiz8/item_tables.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_code/Search.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/MonsterGenerator.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/monster_generators.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/world_cursor.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/OptionsScreen.h"

#define MIPE_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\mipe.cpp"

/* Local Screens\mipe.cpp. The MIPE state is a diagnostic monster/item
   selection panel used while the debug flag is active. */

// GLOBAL: WIZ8 0x0068f0fe
bool g_mipe_mongen_visible;

// GLOBAL: WIZ8 0x0068f108
int g_mipe_mode;

/* How many monsters/items the next Enter in the create modes spawns. */
// GLOBAL: WIZ8 0x0068f10c
int g_mipe_count;

// GLOBAL: WIZ8 0x0068f110
short g_mipe_item_index;

// GLOBAL: WIZ8 0x0068f0fc
unsigned char g_debug_monster_cycle;

// GLOBAL: WIZ8 0x0068f100
W8MipeState* g_mipe_state;

// GLOBAL: WIZ8 0x0068f112
short g_mipe_monster_index;

// GLOBAL: WIZ8 0x0068f114
unsigned char g_mipe_category;

// GLOBAL: WIZ8 0x0068f118
int g_mipe_table_row;

// GLOBAL: WIZ8 0x0068f120
int g_mipe_table_base;

// GLOBAL: WIZ8 0x0068f124
W8PList* g_mipe_monster_entries;

// GLOBAL: WIZ8 0x0068f12c
W8WorldCursorNode* g_mipe_cube;

/* 0x0064A1CC: 'C' toggles between single-monster ("Choosing: One") and
   whole-group ("Choosing: Group") selection in UpdateMipeSelection. */
// GLOBAL: WIZ8 0x0064a1cc
static unsigned char g_mipe_choose_group = 1;

int FindCategoryItemTable(unsigned int category, int ordinal);

/* The MIPE toggle: leaving the panel restores the level flags and combat
   state and tears the editor state down; entering it clears combat flags,
   prints the mode menu and builds the monster entry list. */
// FUNCTION: WIZ8 0x0057d740
void ToggleMipePanel(void)
{
    if (g_mipe_active != 0) {
        g_level_block->text_box_visible_271 = 1;
        g_level_block->mipe_editing_272 = 0;
        ResetEditorStatusLine(-1);
        if (gXStatus.fCombatMode != 0) {
            SelectTextBox(1);
        }
        ReleaseWorldCursor();
        g_mipe_active = 0;
        g_mipe_menu_active = 1;
        g_debug_monster_cycle = 0;
        if (g_mipe_cube != 0) {
            SetWorldCursorNodeColorComponents(g_mipe_cube, 0.0f, 0.0f, 0.5f);
        }
        for (unsigned int monster_list_index = 0;
             monster_list_index < PLLength(gXStatus.plsMonsterList); ++monster_list_index) {
            W8MonsterInfo* monster =
                static_cast<W8MonsterInfo*>(PLGet(gXStatus.plsMonsterList, monster_list_index));
            if (monster != 0 && monster->fInCombat != 0) {
                SetMonsterHighlight(0, monster->location_id, 0);
            }
        }
        for (unsigned int entry_index = 0; entry_index < gXStatus.uiMonstersInDatabase;
             ++entry_index) {
            free(PLGet(g_mipe_monster_entries, entry_index));
        }
        PLDestroy(g_mipe_monster_entries);
        g_mipe_monster_entries = 0;
        IListFreeData(&g_mipe_state->monster_ids);
        PListFreeData(&g_mipe_state->waypoints);
        free(g_mipe_state);
        g_mipe_state = 0;
        SetWorldCursorNodesVisible(g_mipe_trigger_display);
        return;
    }

    g_level_block->text_box_visible_271 = 0;
    g_level_block->mipe_editing_272 = 1;
    SelectTextBox(0);
    ResetEditorStatusLine(-1);
    g_mipe_active = 1;
    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"What would you like to do?", 0);
    ShowNoticef(15, L"1) Create a monster.", 0);
    ShowNoticef(15, L"2) Create an item.", 0);
    ShowNoticef(15, L"3) Edit object(s).", 0);
    ShowNoticef(15, L"4) Monster Generators.", 0);
    ShowNoticef(15, L"5) Select object(s).", 0);
    ShowNoticef(15, L"6) Handle triggers.", 0);
    g_mipe_mode = 0;
    InitializeWorldCursor();
    g_mipe_active = 1;
    g_mipe_menu_active = 0;
    g_mipe_cube = 0;
    g_mipe_monster_entries = PLCreate();

    W8MonsterRecord* records = static_cast<W8MonsterRecord*>(
        malloc(gXStatus.uiMonstersInDatabase * sizeof(W8MonsterRecord)));
    if (records == 0) {
        srAssertFail("pTempMonsterDB", "C:\\Projects\\Wizardry 8\\Local Screens\\mipe.cpp", 0xea5,
                     0);
    }
    LoadMonsterDatabaseRange(0, gXStatus.uiMonstersInDatabase - 1, 0, records);
    for (unsigned int index = 0; index < gXStatus.uiMonstersInDatabase; ++index) {
        W8MipeMonsterEntry* entry =
            static_cast<W8MipeMonsterEntry*>(malloc(sizeof(W8MipeMonsterEntry)));
        if (entry == 0) {
            srAssertFail("pMonRec", "C:\\Projects\\Wizardry 8\\Local Screens\\mipe.cpp", 0xeab, 0);
        }
        wcscpy(entry->name, records[index].name_00);
        entry->kind = records[index].kind_0cb;
        entry->selectable = records[index].deleted == 0 && records[index].editor_index_1c1 == -1;
        PLAdoptAppend(g_mipe_monster_entries, entry);
    }
    free(records);

    g_mipe_state = static_cast<W8MipeState*>(malloc(sizeof(W8MipeState)));
    memset(g_mipe_state, 0, sizeof(W8MipeState));
    IListInit(&g_mipe_state->monster_ids);
    /* Retail overwrites IList's capacity with this sentinel-sized value here.
       Keep the observed store even though the allocation itself is still the
       ten-entry IListInit buffer; selected_group_id at +0x0c is a separate
       cache used by UpdateMipeSelection. */
    g_mipe_state->monster_ids.capacity = W8_MIPE_NO_GROUP;
    g_mipe_state->selecting = 0;
    g_mipe_state->value_34 = 1.0f;
    g_mipe_state->speed_step = 0.020000000f;
    g_mipe_state->edit_selection = -1;
    PListInit(&g_mipe_state->waypoints);

    for (int cursor_index = 0, count = GetWorldCursorNodeCount(); cursor_index < count;
         ++cursor_index) {
        W8WorldCursorNode* node = GetWorldCursorNode(cursor_index);
        SetWorldCursorNodeColorComponents(node, 0.0f, 0.0f, 0.5f);
        RefreshWorldCursorNodeLabel(node);
    }

    int selection = g_mipe_table_row + g_mipe_table_base;
    unsigned int monster_index = 0;
    int visible = 0;
    while (monster_index < gXStatus.uiMonstersInDatabase) {
        W8MipeMonsterEntry* entry =
            static_cast<W8MipeMonsterEntry*>(PLGet(g_mipe_monster_entries, monster_index));
        if (entry->kind == g_mipe_category && entry->selectable != 0) {
            if (visible == selection) {
                break;
            }
            ++visible;
        }
        ++monster_index;
    }
    if (monster_index == gXStatus.uiMonstersInDatabase) {
        g_mipe_monster_index = 0;
    } else {
        W8MonsterRecord record;
        LoadMonsterDatabaseRecord(monster_index, &record);
        g_mipe_monster_index = record.editor_index_1c1;
    }

    unsigned int item_index = 0;
    visible = 0;
    while (item_index < gXStatus.uiItemsInDatabase) {
        if (g_item_records[item_index].equip_class == g_mipe_category &&
            g_item_records[item_index].editor_excluded_0cb == 0) {
            if (visible == selection) {
                break;
            }
            ++visible;
        }
        ++item_index;
    }
    if (item_index == gXStatus.uiItemsInDatabase) {
        item_index = 0;
    }
    g_mipe_item_index = static_cast<short>(item_index);
}

// GLOBAL: WIZ8 0x0068f11c
W8PList* g_mipe_category_list;

/* 'H' in the item-create mode: spawned items get the hidden flag. */
// GLOBAL: WIZ8 0x0068f128
bool g_mipe_item_hidden;

// GLOBAL: WIZ8 0x0068f130
int g_mipe_cube_param;

// GLOBAL: WIZ8 0x0068f134
int g_mipe_scale_plane;

/* Lock/trap kind names for the selected trigger's lock_type, printed by the
   prop-edit menu and the locks & traps editor. */
// GLOBAL: WIZ8 0x0064a1d0
static const wchar_t* g_lock_type_names[4] = {L"None", L"Pickable Lock", L"Trap", L"Key Lock"};

/* Mode-1 status: monster being placed, count being accumulated and the
   creation method. */
// FUNCTION: WIZ8 0x00577bf0
static void ShowMipeMonsterStatus(void)
{
    wchar_t name[100];
    const wchar_t* method;

    wcscpy(name, static_cast<const wchar_t*>(
                     PLGet(g_mipe_monster_entries, static_cast<int>(g_mipe_monster_index))));
    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Type # to specify how many, then ENTER,");
    ShowNoticef(6, L"OR type C to change what monster to place.  ");
    ShowNoticef(6, L"OR type O to edit the creation method.      ");
    ShowNoticef(3, L"How many: %d", g_mipe_count);
    ShowNoticef(0xf, &g_empty_wide_string);
    if (g_mipe_state->creation_method_30 == 0) {
        method = L"Creation Method: Exact #";
    } else if (g_mipe_state->creation_method_30 == 1) {
        method = L"Creation Method: Placeholder";
    } else {
        if (g_mipe_state->creation_method_30 != 2) {
            ShowNoticef(0xf, L"Monster:   %s        ", name);
            return;
        }
        method = L"Creation Method: Selection";
    }
    ShowNoticef(0xf, method);
    ShowNoticef(0xf, L"Monster:   %s        ", name);
}

/* Mode-2 status: item being placed, count and the hidden/visible toggles. */
// FUNCTION: WIZ8 0x00577cb0
static void ShowMipeItemStatus(void)
{
    wchar_t name[100];
    const wchar_t* line;
    unsigned int palette;

    wcscpy(name, g_item_records[g_mipe_item_index].display_name);
    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Type # to specify how many, then ENTER,");
    ShowNoticef(6, L"OR type C to change what item to place.  ");
    ShowNoticef(3, L"How many: %d", g_mipe_count);
    ShowNoticef(0xf, &g_empty_wide_string);
    if (g_byte_0064a1cd == 0) {
        line = L"A - All invisible items  will be blue.";
        palette = 3;
    } else {
        line = L"A - All invisible items will be hidden.";
        palette = 8;
    }
    ShowNoticef(palette, line);
    if (g_mipe_item_hidden == 0) {
        line = L"H - Item will be visible.";
        palette = 5;
    } else {
        line = L"H - Item will be hidden.";
        palette = 8;
    }
    ShowNoticef(palette, line);
    ShowNoticef(0xf, L"Item:   %s        ", name);
}

/* Six visible rows of `list` starting at the table base; the selected row is
   palette 3, the rest 0xf, and short lists blank the remainder. */
// FUNCTION: WIZ8 0x00577d80
static void ShowMipeTableRows(W8PList* list)
{
    void* entry;
    int row;
    unsigned int palette;

    if (list != 0) {
        row = 0;
        do {
            entry = PLGet(list, g_mipe_table_base + row);
            if (entry == 0) {
                ShowNoticef(0xf, &g_empty_wide_string);
            } else {
                palette = row == g_mipe_table_row ? 3 : 0xf;
                ShowNoticef(palette, L"    %s", entry);
            }
            ++row;
        } while (row < 6);
    }
}

/* Mode-9 menu: pick a monster or item to edit, showing the one/group choice
   the 'C' key toggles. */
// FUNCTION: WIZ8 0x00577de0
static void ShowMipeChooseMenu(void)
{
    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Choose monster or item to edit.");
    ShowNoticef(0xf, L"Type C to change how to choose.");
    ShowNoticef(0xf, &g_empty_wide_string);
    ShowNoticef(0xf, &g_empty_wide_string);
    ShowNoticef(0xf, &g_empty_wide_string);
    ShowNoticef(0xf, &g_empty_wide_string);
    if (g_mipe_choose_group != 0) {
        ShowNoticef(0xf, L"Choosing: Group");
        return;
    }
    ShowNoticef(0xf, L"Choosing: One");
}

/* Mode-5 menu: which kind of object to edit. */
// FUNCTION: WIZ8 0x00577e60
static void ShowMipeEditMenu(void)
{
    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"What do you want to edit?.");
    ShowNoticef(0xf, L"1) Monster");
    ShowNoticef(0xf, L"2) Prop");
    ShowNoticef(0xf, L"3) Item");
    ShowNoticef(0xf, L"4) Move object(s)");
}

/* Mode-0xd menu: prop field-editing choices (locks & traps, treasure table). */
// FUNCTION: WIZ8 0x00577eb0
static void ShowMipePropMenu(void)
{
    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Choose how you want to edit prop.");
    ShowNoticef(0xf, L"1) Locks & Traps");
    ShowNoticef(0xf, L"2) Treasure Table");
    ShowNoticef(0xf, L"3) .");
    ShowNoticef(0xf, L"4) .");
    ShowNoticef(0xf, L"5) .");
    ShowNoticef(0xf, L"6) .");
}

/* Mode-0xc menu: trigger actions plus the selected trigger's kind and the
   armed "selecting" notice. */
// FUNCTION: WIZ8 0x00578000
static void ShowMipeTriggerMenu(void)
{
    const wchar_t* status;
    int kind;

    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Select what you want to do.");
    ShowNoticef(0xf, L"1) Create trigger.");
    ShowNoticef(0xf, L"2) Delete trigger.");
    ShowNoticef(0xf, L"3) ");
    ShowNoticef(0xf, L"4) Select trigger.");
    ShowNoticef(0xf, g_mipe_trigger_display == 0 ? L"5) Toggle trigger display [now off]."
                                                 : L"5) Toggle trigger display [now on].");
    ShowNoticef(0xf, L"6) Volume Triggers.");
    if (g_mipe_state->trigger == 0) {
        if (g_mipe_state->selecting == 0) {
            return;
        }
        status = L"---> SELECTING TRIGGER <---";
    } else {
        kind = g_mipe_state->trigger->trigger_kind_018;
        if (kind == 1) {
            status = L"Trigger is Switch";
            if (g_mipe_state->selecting == 0) {
                ShowNoticef(8, status);
                return;
            }
        } else if (kind == 2) {
            status = L"Trigger is Invisible";
            if (g_mipe_state->selecting == 0) {
                ShowNoticef(8, status);
                return;
            }
        } else {
            status = L"Trigger Type Unknown";
            if (g_mipe_state->selecting == 0) {
                ShowNoticef(0xf, status);
                return;
            }
        }
    }
    ShowNoticef(3, status);
}

/* The status line block the ','/'.' and 'k'/'l' handlers repaint. */
// FUNCTION: WIZ8 0x00577f10
void ShowMonsterSpeedStatus(void)
{
    W8PathAI* path;
    float speed;

    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Type ',' to decrease speed, '.' to increase.");
    ShowNoticef(6, L"Type 'k' to decrease increment, 'l' to increase.");
    ShowNoticef(0xf, &g_empty_wide_string);
    ShowNoticef(0xf, L"Increment: %g", g_mipe_state->speed_step);
    ShowNoticef(0xf, &g_empty_wide_string);
    ShowNoticef(0xf, &g_empty_wide_string);
    if (g_mipe_state->monster == 0) {
        ShowNoticef(8, L"No monster available.");
        return;
    }
    path = static_cast<W8PathAI*>(MonsterGetObject0C(g_mipe_state->monster));
    if (path != 0 && PathAIRecordFlag(path) == 0) {
        speed = PathAIGetScale(path);
    } else {
        if (g_mipe_state->monster == 0) {
            ShowNoticef(8, L"Monster has no path AI.");
            return;
        }
        speed = MonsterGetNavigatorValue120(g_mipe_state->monster);
    }
    ShowNoticef(0xf, L"Current speed: %g", speed);
}

/* The "Parameters" pane for the selected volume cube. */
// FUNCTION: WIZ8 0x005780f0
void ShowCubeParameters(void)
{
    ResetEditorStatusLine(-1);
    if (g_mipe_cube == 0) {
        ShowNoticef(6, L"No cube selected.");
        return;
    }
    ShowNoticef(6, L"Parameters:");
    if (g_mipe_cube_param == 0) {
        ShowNoticef(0, L"Message: %d", GetWorldCursorNodeParameter(g_mipe_cube, 0));
    } else {
        ShowNoticef(0xf, L"Message: %d", GetWorldCursorNodeParameter(g_mipe_cube, 0));
    }
    if (g_mipe_cube_param == 1) {
        ShowNoticef(0, L"Search: %d", GetWorldCursorNodeParameter(g_mipe_cube, 1));
    } else {
        ShowNoticef(0xf, L"Search: %d", GetWorldCursorNodeParameter(g_mipe_cube, 1));
    }
    if (g_mipe_cube_param == 2) {
        ShowNoticef(0, L"Function: %d", GetWorldCursorNodeParameter(g_mipe_cube, 2));
        return;
    }
    ShowNoticef(0xf, L"Function: %d", GetWorldCursorNodeParameter(g_mipe_cube, 2));
}

/* The monster generator top menu plus its selection status line. */
// FUNCTION: WIZ8 0x005781f0
void ShowMonsterGeneratorStatus(void)
{
    const char* state;

    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Monster Generators (%d)", GetMonsterGeneratorCount());
    ShowNoticef(0xf, L"1) Create 2) Delete");
    ShowNoticef(0xf, L"3) Edit   4) Select");
    state = "On";
    if (g_mipe_mongen_visible == 0) {
        state = "Off";
    }
    ShowNoticef(0xf, L"5) Toggle Display [%s]", state);
    state = "Off";
    if (g_generator_save_flag == 0) {
        state = "On";
    }
    ShowNoticef(0xf, L"6) Toggle Active  [%s]", state);
    ShowNoticef(0xf, &g_empty_wide_string);
    if (g_mipe_state->generator != 0) {
        if (g_mipe_state->selecting != 0) {
            ShowNoticef(3, L"<--- MOUSE OVER MONGEN --->");
            return;
        }
        ShowNoticef(8, L"MONGEN selected");
        return;
    }
    if (g_mipe_state->selecting != 0) {
        ShowNoticef(3, L"---> SELECTING MONGEN <---");
        return;
    }
    ShowNoticef(0xf, &g_empty_wide_string);
}

/* The "Edit Monster Generator" pane. */
// FUNCTION: WIZ8 0x005782d0
void ShowMonsterGeneratorEditor(void)
{
    MonGen* generator;
    W8EncounterTableRuntime* table;
    const char* state;
    int chance;
    short interval;
    int color;
    const wchar_t* format;

    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Edit Monster Generator");
    ShowNoticef(0xf, L"1) Name: %hs", g_mipe_state->generator->name);
    if (g_mipe_state->generator->encounter_table_index == -1) {
        ShowNoticef(0xf, L"2) Table: Not Selected");
    } else {
        table = GetEncounterTable(g_mipe_state->generator->encounter_table_index);
        ShowNoticef(0xf, L"2) Table: %hs", table->name);
    }
    state = "On";
    if (g_mipe_state->generator->generation_enabled == 0) {
        state = "Off";
    }
    ShowNoticef(0xf, L"3) Toggle Active [%s]", state);
    generator = g_mipe_state->generator;
    if ((generator->flags >> 3 & 1) == 0) {
        interval = generator->custom_interval_seconds;
        chance = generator->custom_spawn_chance;
        format = L" Custom: %d (4-/5+) chance every %d (6-/7+) s";
        color = 5;
    } else {
        chance = g_generator_interval_min;
        format = L" Default: %d (4-/5+) chance every %d (6-/7+) s";
        color = 8;
        interval = g_generator_default_interval;
    }
    ShowNoticef(color, format, chance, static_cast<int>(interval));
    ShowNoticef(0xf, L"8) to toggle chance default");
}

/* Mode-0x17 menu: current encounter-table category plus its six rows. */
// FUNCTION: WIZ8 0x005783c0
static void ShowMipeEncounterCategory(void)
{
    W8EncounterTableRuntime* entry;
    int row;
    unsigned int palette;

    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Category: %S", *g_encounter_names.GetAt(g_mipe_category & 0xff));
    if (g_mipe_category_list != 0) {
        row = 0;
        do {
            entry = static_cast<W8EncounterTableRuntime*>(
                PLGet(g_mipe_category_list, g_mipe_table_base + row));
            if (entry == 0) {
                ShowNoticef(0xf, &g_empty_wide_string);
            } else {
                palette = row == g_mipe_table_row ? 3 : 0xf;
                ShowNoticef(palette, L"    %S", entry->name);
            }
            ++row;
        } while (row < 6);
    }
}

/* Mode-0x1d menu: current item-table category plus its six rows. */
// FUNCTION: WIZ8 0x00578470
static void ShowMipeItemTableCategory(void)
{
    W8ItemTableRecord* entry;
    int row;
    unsigned int palette;

    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Category: %S", g_item_table_category_names[g_mipe_category & 0xff]);
    if (g_mipe_category_list != 0) {
        row = 0;
        do {
            entry = static_cast<W8ItemTableRecord*>(
                PLGet(g_mipe_category_list, g_mipe_table_base + row));
            if (entry == 0) {
                ShowNoticef(0xf, &g_empty_wide_string);
            } else {
                palette = row == g_mipe_table_row ? 3 : 0xf;
                ShowNoticef(palette, L"    %S", entry->name);
            }
            ++row;
        } while (row < 6);
    }
}

/* Mode-1 key handler: digit keys build the spawn count, Enter creates the
   selected monster group at the world cursor, 'C' jumps to the monster
   category picker and 'O' cycles the creation method. */
// FUNCTION: WIZ8 0x00578500
static unsigned char HandleMipeMonsterCreateKey(unsigned short key)
{
    W8PList* list;
    W8MipeMonsterEntry* entry;
    W8MonsterGroup* monster_group;
    int index;
    int visible;
    unsigned int monster_index;
    srVector3T<float> anchor;
    srVector3T<float> formation;
    W8MonsterRecord record;

    if (g_mipe_category_list == 0) {
        list = PLCreate();
        g_mipe_category_list = list;
        if (list != 0) {
            PListClear(list);
            for (index = 0; index < static_cast<int>(gXStatus.uiMonstersInDatabase); ++index) {
                entry = static_cast<W8MipeMonsterEntry*>(PLGet(g_mipe_monster_entries, index));
                if (entry->kind == g_mipe_category && entry->selectable != 0) {
                    PLAdoptAppend(list, entry);
                }
            }
        }
    }
    list = g_mipe_category_list;
    switch (key) {
    case 8:
        g_mipe_count /= 10;
        ShowMipeMonsterStatus();
        return 1;
    default:
        return 0;
    case 0xd:
        break;
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
        g_mipe_count =
            static_cast<int>(static_cast<char>(static_cast<char>(key) - 0x30)) + g_mipe_count * 10;
        /* fall through */
    case 0x20:
        ShowMipeMonsterStatus();
        return 1;
    case 0x43:
        g_mipe_count = 1;
        g_mipe_mode = 6;
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Category: %s",
                    gppStringList[g_special_category_name_ids[g_mipe_category & 0xff]]);
        ShowMipeTableRows(list);
        return 1;
    case 0x4f:
        ++g_mipe_state->creation_method_30;
        if (2 < g_mipe_state->creation_method_30) {
            g_mipe_state->creation_method_30 = 0;
        }
        ShowMipeMonsterStatus();
        return 1;
    }
    if (g_mipe_count != 0) {
        visible = g_mipe_table_row + g_mipe_table_base;
        index = 0;
        monster_index = 0;
        if (0 < static_cast<int>(gXStatus.uiMonstersInDatabase)) {
            do {
                entry =
                    static_cast<W8MipeMonsterEntry*>(PLGet(g_mipe_monster_entries, monster_index));
                if (entry->kind == g_mipe_category && entry->selectable != 0) {
                    if (index == visible) {
                        break;
                    }
                    ++index;
                }
                ++monster_index;
            } while (static_cast<int>(monster_index) <
                     static_cast<int>(gXStatus.uiMonstersInDatabase));
        }
        if (monster_index == gXStatus.uiMonstersInDatabase) {
            record.record_id_187 = 0;
        } else {
            LoadMonsterDatabaseRecord(monster_index, &record);
        }
        GetWorldCursorAnchor(&anchor);
        monster_group = CreateGroup(
            static_cast<unsigned int>(static_cast<unsigned short>(record.record_id_187)),
            static_cast<unsigned int>(g_mipe_count), &anchor, 1, 1, 1);
        if (monster_group == 0) {
            g_mipe_count = reinterpret_cast<int>(
                monster_group); /* reinterpret-ok: retail stores the null group pointer as the count sentinel */
            return 1;
        }
        formation.x = anchor.x;
        formation.y = anchor.y;
        formation.z = anchor.z;
        SetMonsterGroupFormation(monster_group, &formation);
        monster_group->group_state[0x6d] = g_mipe_state->creation_method_30;
    }
    return 1;
}

/* Mode-7 key handler: the item-category picker. Enter resolves the highlighted
   record back to a database index and drops into item-create mode; the arrows
   page and walk the six-row view and cycle categories. */
// FUNCTION: WIZ8 0x00578850
static void HandleMipeItemCategoryKey(unsigned short key)
{
    W8PList* list;
    W8ItemDatabaseRecord* entry;
    int found;
    unsigned int item_index;

    list = g_mipe_category_list;
    switch (key) {
    case 0xd:
        PLGet(g_mipe_category_list, g_mipe_table_base + g_mipe_table_row);
        found = 0;
        for (item_index = 0;
             static_cast<int>(item_index) < static_cast<int>(gXStatus.uiItemsInDatabase);
             ++item_index) {
            if (g_item_records[item_index].equip_class == g_mipe_category &&
                g_item_records[item_index].editor_excluded_0cb == 0) {
                if (found == g_mipe_table_base + g_mipe_table_row) {
                    break;
                }
                ++found;
            }
        }
        if (item_index == gXStatus.uiItemsInDatabase) {
            item_index = 0;
        }
        g_mipe_item_index = static_cast<short>(item_index);
        ShowMipeItemStatus();
        g_mipe_mode = 2;
        return;
    case 0x25:
        if (g_mipe_category == 0) {
            return;
        }
        --g_mipe_category;
        g_mipe_table_row = 0;
        g_mipe_table_base = 0;
        if (g_mipe_category_list != 0) {
            PListClear(g_mipe_category_list);
            for (found = 0; found < static_cast<int>(gXStatus.uiItemsInDatabase); ++found) {
                entry = &g_item_records[found];
                if (entry->equip_class == g_mipe_category && entry->editor_excluded_0cb == 0) {
                    PLAdoptAppend(list, entry);
                }
            }
        }
        break;
    case 0x27:
        if (0x18 < g_mipe_category) {
            return;
        }
        ++g_mipe_category;
        g_mipe_table_row = 0;
        g_mipe_table_base = 0;
        if (g_mipe_category_list != 0) {
            PListClear(g_mipe_category_list);
            for (found = 0; found < static_cast<int>(gXStatus.uiItemsInDatabase); ++found) {
                entry = &g_item_records[found];
                if (entry->equip_class == g_mipe_category && entry->editor_excluded_0cb == 0) {
                    PLAdoptAppend(list, entry);
                }
            }
        }
        break;
    case 0x26:
        if (g_mipe_table_row == 0) {
            if (g_mipe_table_base != 0) {
                --g_mipe_table_base;
            }
        } else {
            --g_mipe_table_row;
        }
        break;
    case 0x21:
        if (g_mipe_table_base == 0) {
            return;
        }
        g_mipe_table_base -= 6;
        if (g_mipe_table_base < 0) {
            g_mipe_table_base = 0;
        }
        break;
    case 0x28:
        if (g_mipe_table_row < 5 && g_mipe_table_base + g_mipe_table_row <
                                        static_cast<int>(PLLength(g_mipe_category_list) - 1)) {
            ++g_mipe_table_row;
            break;
        }
        if (static_cast<int>(PLLength(g_mipe_category_list) - 1) <=
            g_mipe_table_base + g_mipe_table_row) {
            return;
        }
        ++g_mipe_table_base;
        break;
    case 0x20:
        break;
    case 0x22:
        if (static_cast<int>(PLLength(g_mipe_category_list) - 6) <=
            g_mipe_table_base + g_mipe_table_row) {
            if (static_cast<int>(PLLength(g_mipe_category_list)) <=
                g_mipe_table_base + g_mipe_table_row) {
                return;
            }
            g_mipe_table_base += 6;
            g_mipe_table_row = 0;
            if (static_cast<int>(PLLength(g_mipe_category_list)) <= g_mipe_table_base) {
                g_mipe_table_base = static_cast<int>(PLLength(g_mipe_category_list));
                --g_mipe_table_base;
            }
        } else {
            g_mipe_table_base += 6;
        }
        break;
    default:
        return;
    }
    list = g_mipe_category_list;
    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Category: %s", gppStringList[g_equip_class_name_ids[g_mipe_category & 0xff]]);
    ShowMipeTableRows(list);
}

/* Mode-2 key handler: digits accumulate the count, Enter spawns that many of
   the selected item at the world cursor, 'A' flips the invisible-item display
   and activates/deactivates the flagged items, 'C' opens the item category
   picker and 'H' toggles the spawned item's hidden flag. */
// FUNCTION: WIZ8 0x00578d00
static unsigned char HandleMipeItemCreateKey(unsigned short key)
{
    W8PList* list;
    W8WorldItem* spawned;
    W8WorldItem* world_item;
    int index;
    int found;
    unsigned int item_index;
    unsigned int spawned_index;
    bool show_invisible;
    srVector3T<float> anchor;

    if (g_mipe_category_list == 0) {
        list = PLCreate();
        g_mipe_category_list = list;
        if (list != 0) {
            PListClear(list);
            for (index = 0; index < static_cast<int>(gXStatus.uiItemsInDatabase); ++index) {
                if (g_item_records[index].equip_class == g_mipe_category &&
                    g_item_records[index].editor_excluded_0cb == 0) {
                    PLAdoptAppend(list, &g_item_records[index]);
                }
            }
        }
    }
    list = g_mipe_category_list;
    switch (key) {
    case 0x20:
        ShowMipeItemStatus();
        break;
    case 0x43:
        g_mipe_count = 0;
        g_mipe_mode = 7;
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Category: %s",
                    gppStringList[g_equip_class_name_ids[g_mipe_category & 0xff]]);
        ShowMipeTableRows(list);
        return 1;
    case 8:
        g_mipe_count /= 10;
        ShowMipeItemStatus();
        break;
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
        g_mipe_count =
            static_cast<int>(static_cast<char>(static_cast<char>(key) - 0x30)) + g_mipe_count * 10;
        ShowMipeItemStatus();
        break;
    case 0x48:
        g_mipe_item_hidden = g_mipe_item_hidden == 0;
        ShowMipeItemStatus();
        break;
    case 0x41:
        show_invisible = g_byte_0064a1cd == 0;
        g_byte_0064a1cd = show_invisible;
        for (item_index = 0; item_index < PLLength(gXStatus.plsItemList); ++item_index) {
            world_item = ItemInfo(item_index);
            if (ItemHasFlags(world_item, 1) != 0) {
                if (show_invisible) {
                    if (world_item->fActive) {
                        DeactivateWorldItem(world_item);
                    }
                } else if (!world_item->fActive) {
                    ActivateItem(world_item);
                }
            }
        }
        ShowMipeItemStatus();
        return 1;
    case 0xd:
        found = 0;
        for (item_index = 0;
             static_cast<int>(item_index) < static_cast<int>(gXStatus.uiItemsInDatabase);
             ++item_index) {
            if (g_item_records[item_index].equip_class == g_mipe_category &&
                g_item_records[item_index].editor_excluded_0cb == 0) {
                if (found == g_mipe_table_base + g_mipe_table_row) {
                    break;
                }
                ++found;
            }
        }
        if (item_index == gXStatus.uiItemsInDatabase) {
            item_index = 0;
        }
        GetWorldCursorAnchor(&anchor);
        spawned_index = 0;
        if (g_mipe_count != 0) {
            do {
                int flags = 3;
                if (g_mipe_item_hidden != 0) {
                    flags = 0x83;
                }
                spawned = SpawnItem(item_index & 0xffff, &anchor, flags, 1);
                if (g_mipe_item_hidden != 0) {
                    SetItemFlags(spawned, 1, 1);
                    RegisterSearchableWorldItem(spawned);
                }
                ++spawned_index;
            } while (spawned_index < static_cast<unsigned int>(g_mipe_count));
        }
        return 1;
    default:
        return 0;
    }
    return 1;
}

/* Mode-6 key handler: the monster-category picker. Enter resolves the
   highlighted entry to a monster record, seeds the spawn count from the
   creation method and drops back to mode 1; the arrows page/walk rows and
   cycle through non-empty categories. */
// FUNCTION: WIZ8 0x00579300
static void HandleMipeMonsterCategoryKey(unsigned short key)
{
    W8PList* list;
    W8MipeMonsterEntry* entry;
    int index;
    int found;
    int wraps;
    unsigned int monster_index;
    W8MonsterRecord record;
    W8MonsterRecord selected;

    list = g_mipe_category_list;
    switch (key) {
    case 0xd:
        PLGet(g_mipe_category_list, g_mipe_table_base + g_mipe_table_row);
        index = g_mipe_table_base + g_mipe_table_row;
        found = 0;
        monster_index = 0;
        if (0 < static_cast<int>(gXStatus.uiMonstersInDatabase)) {
            do {
                entry =
                    static_cast<W8MipeMonsterEntry*>(PLGet(g_mipe_monster_entries, monster_index));
                if (entry->kind == g_mipe_category && entry->selectable != 0) {
                    if (found == index) {
                        break;
                    }
                    ++found;
                }
                ++monster_index;
            } while (static_cast<int>(monster_index) <
                     static_cast<int>(gXStatus.uiMonstersInDatabase));
        }
        if (monster_index == gXStatus.uiMonstersInDatabase) {
            record.record_id_187 = 0;
        } else {
            LoadMonsterDatabaseRecord(monster_index, &record);
        }
        g_mipe_monster_index = record.record_id_187;
        if (g_mipe_state->creation_method_30 != 1) {
            if (g_mipe_state->creation_method_30 == 2) {
                LoadMonsterDatabaseRecord(static_cast<int>(record.record_id_187), &selected);
                g_mipe_count =
                    selected.group_size_dice_0c1.sides * selected.group_size_dice_0c1.count +
                    static_cast<int>(selected.group_size_dice_0c1.base);
            }
            ShowMipeMonsterStatus();
            g_mipe_mode = 1;
            return;
        }
        g_mipe_count = 1;
        ShowMipeMonsterStatus();
        g_mipe_mode = 1;
        return;
    case 0x25:
        wraps = 0;
        g_mipe_table_row = 0;
        g_mipe_table_base = 0;
        do {
            list = g_mipe_category_list;
            if (g_mipe_category == 0) {
                g_mipe_category = 0x20;
                ++wraps;
            } else {
                --g_mipe_category;
            }
            if (g_mipe_category_list != 0) {
                PListClear(g_mipe_category_list);
                for (index = 0; index < static_cast<int>(gXStatus.uiMonstersInDatabase); ++index) {
                    entry = static_cast<W8MipeMonsterEntry*>(PLGet(g_mipe_monster_entries, index));
                    if (entry->kind == g_mipe_category && entry->selectable != 0) {
                        PLAdoptAppend(list, entry);
                    }
                }
            }
        } while (PLLength(g_mipe_category_list) == 0 && wraps < 2);
        break;
    case 0x27:
        wraps = 0;
        g_mipe_table_row = 0;
        g_mipe_table_base = 0;
        do {
            list = g_mipe_category_list;
            if (g_mipe_category < 0x20) {
                ++g_mipe_category;
            } else {
                g_mipe_category = 0;
                ++wraps;
            }
            if (g_mipe_category_list != 0) {
                PListClear(g_mipe_category_list);
                for (index = 0; index < static_cast<int>(gXStatus.uiMonstersInDatabase); ++index) {
                    entry = static_cast<W8MipeMonsterEntry*>(PLGet(g_mipe_monster_entries, index));
                    if (entry->kind == g_mipe_category && entry->selectable != 0) {
                        PLAdoptAppend(list, entry);
                    }
                }
            }
        } while (PLLength(g_mipe_category_list) == 0 && wraps < 2);
        break;
    case 0x26:
        if (g_mipe_table_row == 0) {
            if (g_mipe_table_base != 0) {
                --g_mipe_table_base;
            }
        } else {
            --g_mipe_table_row;
        }
        break;
    case 0x21:
        if (g_mipe_table_base == 0) {
            return;
        }
        g_mipe_table_base -= 6;
        if (g_mipe_table_base < 0) {
            g_mipe_table_base = 0;
        }
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Category: %s",
                    gppStringList[g_special_category_name_ids[g_mipe_category & 0xff]]);
        ShowMipeTableRows(list);
        return;
    case 0x28:
        if (g_mipe_table_row < 5 && g_mipe_table_base + g_mipe_table_row <
                                        static_cast<int>(PLLength(g_mipe_category_list) - 1)) {
            ++g_mipe_table_row;
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Category: %s",
                        gppStringList[g_special_category_name_ids[g_mipe_category & 0xff]]);
            ShowMipeTableRows(list);
            return;
        }
        if (static_cast<int>(PLLength(g_mipe_category_list) - 1) <=
            g_mipe_table_base + g_mipe_table_row) {
            return;
        }
        ++g_mipe_table_base;
        break;
    case 0x22:
        if (g_mipe_table_base + g_mipe_table_row <
            static_cast<int>(PLLength(g_mipe_category_list) - 6)) {
            g_mipe_table_base += 6;
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Category: %s",
                        gppStringList[g_special_category_name_ids[g_mipe_category & 0xff]]);
            ShowMipeTableRows(list);
            return;
        }
        if (static_cast<int>(PLLength(g_mipe_category_list)) <=
            g_mipe_table_base + g_mipe_table_row) {
            return;
        }
        g_mipe_table_base += 6;
        g_mipe_table_row = 0;
        if (static_cast<int>(PLLength(g_mipe_category_list)) <= g_mipe_table_base) {
            g_mipe_table_base = static_cast<int>(PLLength(g_mipe_category_list));
            --g_mipe_table_base;
        }
        break;
    case 0x20:
        break;
    default:
        return;
    }
    list = g_mipe_category_list;
    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Category: %s",
                gppStringList[g_special_category_name_ids[g_mipe_category & 0xff]]);
    ShowMipeTableRows(list);
}

/* '1' halves the selected monster's animation scale, '2' doubles it, '3'
   arms speed editing, '4' sends it to the world cursor, '5' advances it one
   subcycle. */
// FUNCTION: WIZ8 0x00579900
void HandleMonsterDebugKey(unsigned short key)
{
    W8Monster* monster;
    W8MonsterInfo* info;
    W8World* world;
    signed char cycle;
    float scale;
    srVector3T<float> position;
    srVector3T<float> location;

    switch (key) {
    case 0x31:
        monster = GetMonsterByLocationID(IListGetAt(&g_mipe_state->monster_ids, 0));
        if (monster != 0) {
            scale = monster->GetCurrentAnimationScale();
            monster->SetCurrentAnimationScale(scale * g_float_005ebc7c);
            return;
        }
        break;
    case 0x32:
        monster = GetMonsterByLocationID(IListGetAt(&g_mipe_state->monster_ids, 0));
        if (monster != 0) {
            scale = monster->GetCurrentAnimationScale();
            monster->SetCurrentAnimationScale(scale + scale);
            return;
        }
        break;
    case 0x33:
        if (ILLength(&g_mipe_state->monster_ids) == 1) {
            g_mipe_mode = 0xb;
            ShowMonsterSpeedStatus();
            return;
        }
        break;
    case 0x34:
        if (ILLength(&g_mipe_state->monster_ids) == 1) {
            info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x6e3, MIPE_CPP, IListGetAt(&g_mipe_state->monster_ids, 0), 1));
            if (info == 0 || info->p3D == 0) {
                srAssertFail("pMonsterInfo && pMonsterInfo->p3D", MIPE_CPP, 0x6e5, 0);
            }
            monster = info->p3D;
            GetWorldCursorPosition(&position);
            MonsterGetLocation(monster, &location);
            MonsterForward453690(monster, &position);
            MonsterSetAnimating(monster, 1);
            MonsterSetCycle(monster, 4);
            MonsterSetNavigatorObjectFlag38(monster, 1);
            g_mipe_state->monster = monster;
            return;
        }
        break;
    case 0x35:
        if (ILLength(&g_mipe_state->monster_ids) == 1) {
            info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x703, MIPE_CPP, IListGetAt(&g_mipe_state->monster_ids, 0), 1));
            if (info == 0) {
                srAssertFail("pMonsterInfo", MIPE_CPP, 0x705, 0);
            }
            monster = info->p3D;
            if (monster == 0) {
                srAssertFail("pMonster", MIPE_CPP, 0x707, 0);
            }
            cycle = monster->m_pRep->current_subcycle;
            if (static_cast<char>(monster->m_pRep->GetNumSubsPerCycle(-1)) - 1 <= cycle) {
                cycle = monster->m_pRep->current_cycle + 1;
                do {
                    if (0x1a < cycle) {
                        goto scan_wrap;
                    }
                    do {
                        if (monster->m_pRep->GetNumSubsPerCycle(cycle) != 0) {
                            monster->SetCycle(cycle);
                            monster->SetForcedSubcycleA6(0);
                            monster->flags_1dc |= 0x10;
                            goto cycle_done;
                        }
                    scan_wrap:
                        ++cycle;
                    } while (cycle < 0x1b);
                    cycle = 0;
                } while (true);
            }
            monster->SetForcedSubcycleA6(cycle + 1);
        cycle_done:
            monster->SetSubCycle(0);
            monster->m_pRep->pending_subcycle_066 = 0;
            world = GetWorld();
            WorldGetCameraLocation(world, &location);
            position = location;
            monster->SelectLOD(&position);
            monster->ReplacePath(0);
        }
    }
}

/* The ',', '.', 'k' and 'l' keys while speed editing the debug monster. */
// FUNCTION: WIZ8 0x00579bf0
void AdjustMonsterSpeed(unsigned short key)
{
    W8PathAI* path;
    float speed;
    float factor;

    factor = 2.0f;
    if (g_mipe_state->monster == 0) {
        return;
    }
    path = static_cast<W8PathAI*>(MonsterGetObject0C(g_mipe_state->monster));
    if (path == 0) {
        factor = 1.0f;
        speed = MonsterGetNavigatorValue120(g_mipe_state->monster);
    } else {
        if (PathAIRecordFlag(path) != 0) {
            return;
        }
        speed = PathAIGetScale(path);
        speed += speed;
    }
    switch (key) {
    case 0xbc:
        speed = speed - factor * g_mipe_state->speed_step;
        if (path != 0) {
            PathAISetScale(path, speed);
        }
        MonsterSetNavigatorValue120(g_mipe_state->monster, speed);
        break;
    case 0xbe:
        speed = factor * g_mipe_state->speed_step + speed;
        if (path != 0) {
            PathAISetScale(path, speed);
        }
        MonsterSetNavigatorValue120(g_mipe_state->monster, speed);
        break;
    case 0x4c:
        g_mipe_state->speed_step = g_mipe_state->speed_step + g_camera_transition_epsilon;
        break;
    case 0x4b:
        g_mipe_state->speed_step = g_mipe_state->speed_step - g_camera_transition_epsilon;
        if (g_mipe_state->speed_step < static_cast<float>(g_double_005ec8d0)) {
            g_mipe_state->speed_step = 0.001f;
        }
        break;
    default:
        goto speed_done;
    }
    ShowMonsterSpeedStatus();
speed_done:
    if (path != 0 && PathAIGetScale(path) < g_float_005ebc90) {
        PathAISetScale(path, 0.0001f);
    }
}

/* 'C' drops a waypoint monster at the cursor, 'X' removes the newest one. */
// FUNCTION: WIZ8 0x00579df0
void HandleWaypointKey(unsigned short key)
{
    W8Monster* monster;
    W8World* world;
    srVector3T<float> position;
    W8GrCycleLoadContext context;

    if (key == 0x43) {
        monster = 0;
        context.directory_08 = "Data\\Monsters";
        context.world_00 = GetWorld();
        LoadMonsterCycle(&context, "waypoint", &monster, -1, 1);
        GetWorldCursorPosition(&position);
        monster->SetPosition004A6DF0(&position);
        world = GetWorld();
        AddMonsterToWorld(world, monster);
        world = GetWorld();
        UpdateCycleRepresentation(monster, world);
        ++g_mipe_state->waypoint_count;
        PLAdoptAppend(&g_mipe_state->waypoints, monster);
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Type 'C' to create a waypoint.");
        ShowNoticef(0xf, &g_empty_wide_string);
        ShowNoticef(3, L"Laying down waypoint %d", g_mipe_state->waypoint_count);
        ShowNoticef(0xf, &g_empty_wide_string);
        ShowNoticef(0xf, &g_empty_wide_string);
        ShowNoticef(0xf, &g_empty_wide_string);
        ShowNoticef(0xf, L"Type X to delete last waypoint.");
    } else if (key == 0x58) {
        if (g_mipe_state != 0) {
            unsigned int count = PLLength(&g_mipe_state->waypoints);
            if (count != 0) {
                monster = static_cast<W8Monster*>(PLGet(&g_mipe_state->waypoints, count - 1));
                if (monster != 0) {
                    PLRemoveAt(&g_mipe_state->waypoints, count - 1);
                    world = GetWorld();
                    RemoveMonsterFromWorldList(world, monster);
                    world = GetWorld();
                    DetachMonsterRepresentation(monster, world);
                    g_mipe_state->waypoint_count = g_mipe_state->waypoint_count - 1;
                }
            }
        }
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Type 'C' to create a waypoint.");
        ShowNoticef(0xf, &g_empty_wide_string);
        ShowNoticef(3, L"Laying down waypoint %d", g_mipe_state->waypoint_count);
        ShowNoticef(0xf, &g_empty_wide_string);
        ShowNoticef(0xf, &g_empty_wide_string);
        ShowNoticef(0xf, &g_empty_wide_string);
        ShowNoticef(0xf, L"Type X to delete last waypoint.");
    }
}

/* Mode-0xd key handler: scans the world's props for the picked one with a
   trigger, stores it as the edit target and opens the locks & traps ('1') or
   treasure-table ('2') editors. */
// FUNCTION: WIZ8 0x00579ff0
static void HandleMipePropEditKey(unsigned short key)
{
    W8PList* list;
    W8World* world;
    W8Prop* prop;
    Trigger* trigger;
    W8ItemTableRecord* table;
    const wchar_t* key_name;
    int index;
    int found;
    int count;
    int table_index;
    int row;
    unsigned int palette;

    g_mipe_count = 0;
    world = GetWorld();
    count = WorldGetPropCount(world);
    index = 0;
    if (0 < count) {
        while (true) {
            world = GetWorld();
            prop = WorldGetPropAt(world, index);
            trigger = prop->GetValue18();
            if (trigger != 0 && prop->IsPickedProp(g_world)) {
                break;
            }
            ++index;
            if (count <= index) {
                return;
            }
        }
        if (prop != 0) {
            g_mipe_state->prop = prop;
            if (key == 0x31) {
                g_mipe_mode = 0x1b;
                trigger = g_mipe_state->prop->GetValue18();
                ResetEditorStatusLine(-1);
                ShowNoticef(6, L"Edit Locks & Traps");
                ShowNoticef(0xf, L"1) Type: %s", g_lock_type_names[trigger->lock_state.lock_type]);
                index = trigger->lock_state.key_id;
                if (index < 0) {
                    key_name = &g_empty_wide_string;
                } else {
                    key_name = g_item_records[index].display_name;
                }
                ShowNoticef(0xf, L"2) Key Id: (%d) %s", index, key_name);
                ShowNoticef(0xf, L" Difficulty (3+/4-): %d", trigger->lock_state.difficulty);
            } else if (key == 0x32) {
                trigger = prop->GetValue18();
                table_index = FindItemTableByName(trigger->inline_action_data_24c);
                if (g_mipe_category_list == 0) {
                    g_mipe_category_list = PLCreate();
                }
                list = g_mipe_category_list;
                if (table_index < 0) {
                    g_mipe_category = 0;
                    g_mipe_table_base = 0;
                    g_mipe_table_row = 0;
                    if (g_mipe_category_list != 0) {
                        PListClear(g_mipe_category_list);
                        for (index = 0; index < static_cast<int>(gXStatus.uiItemTablesInDatabase);
                             ++index) {
                            table = g_item_tables[index];
                            if (table->category_id == 0) {
                                PLAdoptAppend(list, table);
                            }
                        }
                    }
                } else {
                    unsigned char category = g_item_tables[table_index]->category_id;
                    g_mipe_category = category;
                    if (g_mipe_category_list != 0) {
                        PListClear(g_mipe_category_list);
                        for (index = 0; index < static_cast<int>(gXStatus.uiItemTablesInDatabase);
                             ++index) {
                            table = g_item_tables[index];
                            if (table->category_id == static_cast<unsigned int>(category)) {
                                PLAdoptAppend(list, table);
                            }
                        }
                    }
                    index = 0;
                    found = static_cast<int>(PLLength(g_mipe_category_list));
                    if (0 < found) {
                        do {
                            table =
                                static_cast<W8ItemTableRecord*>(PLGet(g_mipe_category_list, index));
                            if (table == g_item_tables[table_index]) {
                                break;
                            }
                            ++index;
                            found = static_cast<int>(PLLength(g_mipe_category_list));
                        } while (index < found);
                    }
                    g_mipe_table_base = (index / 6) * 6;
                    g_mipe_table_row = index % 6;
                }
                g_mipe_menu_active = 0;
                g_mipe_mode = 0x1d;
                ResetEditorStatusLine(-1);
                ShowNoticef(6, L"Category: %S",
                            g_item_table_category_names[g_mipe_category & 0xff]);
                if (g_mipe_category_list != 0) {
                    row = 0;
                    do {
                        table = static_cast<W8ItemTableRecord*>(
                            PLGet(g_mipe_category_list, g_mipe_table_base + row));
                        if (table == 0) {
                            ShowNoticef(0xf, &g_empty_wide_string);
                        } else {
                            palette = row == g_mipe_table_row ? 3 : 0xf;
                            ShowNoticef(palette, L"    %S", table->name);
                        }
                        ++row;
                    } while (row < 6);
                    return;
                }
            }
        }
    }
}

/* The volume cube top menu: create, delete, edit parameters, move, scale and
   select. */
// FUNCTION: WIZ8 0x0057a310
int HandleCubeMenuKey(unsigned int key)
{
    const wchar_t* prompt;
    srVector3T<float> position;
    srVector3T<float> anchor;
    char* name;
    int serial;
    char scale_planes[4];

    switch (key & 0xffff) {
    case 0x20:
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Choose an action:");
        ShowNoticef(0xf, L"1) Create cube.");
        ShowNoticef(0xf, L"2) Delete cube.");
        ShowNoticef(0xf, L"3) Edit cube parameters.");
        ShowNoticef(0xf, L"4) Move cube.");
        ShowNoticef(0xf, L"5) Scale cube.");
        ShowNoticef(0xf, L"6) Select cube.");
        return 1;
    default:
        return 0;
    case 0x31:
        g_mipe_cube = CreateWorldCursorCube();
        GetCameraForwardPoint00421100(2500.0f, &position);
        position.y = SettlePositionToGround00420C30(&position, 0);
        MoveWorldCursorNode(g_mipe_cube, &position);
        AttachWorldCursorNode(g_mipe_cube, 1);
        SetWorldCursorNodeColorComponents(g_mipe_cube, 0.0f, 1.0f, 0.0f);
        serial = gXStatus.mipe_cube_serial;
        ++gXStatus.mipe_cube_serial;
        name = FormatString("Cube%3.3", serial);
        SetWorldCursorNodeName(g_mipe_cube, name);
        return 1;
    case 0x32:
        if (g_mipe_cube != 0) {
            DestroyWorldCursorCube(g_mipe_cube);
            g_mipe_cube = 0;
        }
        return 1;
    case 0x33:
        if (g_mipe_cube != 0) {
            g_mipe_mode = 0x10;
            g_mipe_menu_active = 0;
            ShowCubeParameters();
        }
        return 1;
    case 0x34:
        if (g_mipe_cube == 0) {
            return 1;
        }
        g_mipe_mode = 0x11;
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Move Volume Trigger.");
        if (g_mipe_cube == 0) {
            ShowNoticef(0xf, L"Click on trigger to move.");
        } else {
            ShowNoticef(0xf, L"Move trigger. Hold down SHIFT to");
            ShowNoticef(0xf, L"change elevation.");
        }
        g_mipe_menu_active = 0;
        ShowWorldCursor();
        WarpSystemCursor(0x140, 0xf0);
        GetWorldCursorAnchor(&anchor);
        MoveWorldCursorNode(g_mipe_cube, &anchor);
        g_mipe_state->dragging = true;
        g_mipe_menu_active = 0;
        return 1;
    case 0x35:
        if (g_mipe_cube == 0) {
            return 1;
        }
        g_mipe_mode = 0x12;
        scale_planes[0] = 'X';
        scale_planes[1] = 'Y';
        scale_planes[2] = 'Z';
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Scale Volume Trigger.");
        if (g_mipe_cube != 0) {
            ShowNoticef(0xf, L"Scaling in the %c plane. ",
                        static_cast<int>(scale_planes[g_mipe_scale_plane]));
            ShowNoticef(0xf, L"Press X/Y/Z to change plane.");
            g_mipe_menu_active = 0;
            return 1;
        }
        prompt = L"Click on trigger to scale.";
        break;
    case 0x36:
        g_mipe_mode = 0x13;
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Select cube:");
        if (g_mipe_cube == 0) {
            prompt = L"Click on a cube to select it.";
        } else {
            prompt = L"Click on another cube to select it.";
        }
        break;
    }
    ShowNoticef(0xf, prompt);
    g_mipe_menu_active = 0;
    return 1;
}

/* The digit keys editing the selected cube parameter, plus the arrow keys
   stepping between Message, Search and Function. */
// FUNCTION: WIZ8 0x0057a630
int HandleCubeParameterKey(unsigned int key)
{
    int value;

    switch (key & 0xffff) {
    case 0x20:
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Choose an action:");
        ShowNoticef(0xf, L"1) Create cube.");
        ShowNoticef(0xf, L"2) Delete cube.");
        ShowNoticef(0xf, L"3) Edit cube parameters.");
        ShowNoticef(0xf, L"4) Move cube.");
        ShowNoticef(0xf, L"5) Scale cube.");
        ShowNoticef(0xf, L"6) Select cube.");
        return 1;
    case 0x26:
        if (0 < g_mipe_cube_param) {
            --g_mipe_cube_param;
            ShowCubeParameters();
            return 1;
        }
        break;
    case 0x28:
        if (g_mipe_cube_param < 2) {
            ++g_mipe_cube_param;
            ShowCubeParameters();
            return 1;
        }
        break;
    case 8:
        if (g_mipe_cube != 0) {
            value = GetWorldCursorNodeParameter(g_mipe_cube, g_mipe_cube_param);
            SetWorldCursorNodeParameter(g_mipe_cube, g_mipe_cube_param, value / 10);
            DrawWorldCursorNodeLabel(g_mipe_cube);
            ShowCubeParameters();
            return 1;
        }
        break;
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
        if (g_mipe_cube != 0 &&
            (value = GetWorldCursorNodeParameter(g_mipe_cube, g_mipe_cube_param), value < 99999)) {
            value = GetWorldCursorNodeParameter(g_mipe_cube, g_mipe_cube_param);
            SetWorldCursorNodeParameter(g_mipe_cube, g_mipe_cube_param,
                                        ((key & 0xffff) - 0x30) + value * 10);
            DrawWorldCursorNodeLabel(g_mipe_cube);
            ShowCubeParameters();
        }
        break;
    default:
        return 0;
    }
    return 1;
}

/* The scale editor's keys: arrows resize the current plane and X/Y/Z pick the
   plane. */
// FUNCTION: WIZ8 0x0057a800
int HandleCubeScaleKey(unsigned short key)
{
    char scale_planes[8];

    switch (key) {
    case 0x20:
        scale_planes[0] = 'X';
        scale_planes[1] = 'Y';
        scale_planes[2] = 'Z';
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Scale Volume Trigger.");
        if (g_mipe_cube == 0) {
            ShowNoticef(0xf, L"Click on trigger to scale.");
            return 1;
        }
        ShowNoticef(0xf, L"Scaling in the %c plane. ",
                    static_cast<int>(scale_planes[g_mipe_scale_plane]));
        ShowNoticef(0xf, L"Press X/Y/Z to change plane.");
        return 1;
    default:
        return 0;
    case 0x26:
        if (g_mipe_cube != 0) {
            if (g_mipe_scale_plane == 0) {
                ScaleWorldCursorNodeX(g_mipe_cube, 1.1);
                return 1;
            }
            if (g_mipe_scale_plane == 1) {
                ScaleWorldCursorNodeY(g_mipe_cube, 1.1);
                return 1;
            }
            if (g_mipe_scale_plane == 2) {
                ScaleWorldCursorNodeZ(g_mipe_cube, 1.1);
                return 1;
            }
        }
        break;
    case 0x28:
        if (g_mipe_cube != 0) {
            if (g_mipe_scale_plane == 0) {
                ScaleWorldCursorNodeX(g_mipe_cube, 0.9);
                return 1;
            }
            if (g_mipe_scale_plane == 1) {
                ScaleWorldCursorNodeY(g_mipe_cube, 0.9);
                return 1;
            }
            if (g_mipe_scale_plane == 2) {
                ScaleWorldCursorNodeZ(g_mipe_cube, 0.9);
                return 1;
            }
        }
        break;
    case 0x58:
        g_mipe_scale_plane = 0;
        return 1;
    case 0x59:
        g_mipe_scale_plane = 1;
        return 1;
    case 0x5a:
        g_mipe_scale_plane = 2;
        break;
    }
    return 1;
}

/* The monster generator top-menu keys. */
// FUNCTION: WIZ8 0x0057aa00
int HandleMonsterGeneratorKey(unsigned short key)
{
    MonGen* generator;
    MonGen* other;
    srVector3T<float> anchor;
    srVector3T<float> position;
    char name[12];
    int index;
    int count;

    switch (key) {
    case 0x20:
        break;
    default:
        return 0;
    case 0x31:
        GetWorldCursorAnchor(&anchor);
        count = GetMonsterGeneratorCount();
        for (index = 0; index < count; ++index) {
            GetMonsterGenerator(index)->SetActive(1, 0);
        }
        g_mipe_mongen_visible = true;
        ShowMonsterGeneratorStatus();
        generator = new MonGen();
        if (generator == 0) {
            srAssertFail("pMongen", MIPE_CPP, 0xa24,
                         "mipe.cpp: Out of memory creating monster generator");
        }
        position = anchor;
        generator->SetState(&position);
        generator->Reload(1, 0);
        generator->Reset();
        sprintf(name, "MonGen%3.3d", gXStatus.saved_encounter_budget++);
        generator->SetName(name);
        AddMonsterGenerator(generator);
        index = 0;
        count = GetMonsterGeneratorCount();
        if (0 < count) {
            do {
                other = GetMonsterGenerator(index);
                if (other->marker_item != 0) {
                    static_cast<W8ItemRep*>(other->marker_item->m_pRep)->SetFlags(0x10, 0);
                    other->marker_item->SetHighlight(false);
                }
                ++index;
                count = GetMonsterGeneratorCount();
            } while (index < count);
        }
        g_mipe_state->generator = generator;
        if (generator->marker_item != 0) {
            static_cast<W8ItemRep*>(generator->marker_item->m_pRep)->SetFlags(0x10, 1);
            generator->marker_item->SetHighlight(true);
        }
        return 1;
    case 0x32:
        if (g_mipe_state->generator != 0) {
            RemoveMonsterGenerator(g_mipe_state->generator);
            g_mipe_state->generator = 0;
            ShowMonsterGeneratorStatus();
        }
        return 1;
    case 0x33:
        if (g_mipe_state->generator != 0) {
            g_mipe_mode = 0x16;
            ShowMonsterGeneratorEditor();
        }
        return 1;
    case 0x34:
        g_mipe_state->selecting = 1;
        HideWorldCursor();
        ShowMonsterGeneratorStatus();
        return 1;
    case 0x35: {
        bool visible = g_mipe_mongen_visible == 0;
        count = GetMonsterGeneratorCount();
        for (index = 0; index < count; ++index) {
            GetMonsterGenerator(index)->SetActive(visible, 0);
        }
        g_mipe_mongen_visible = visible;
        ShowMonsterGeneratorStatus();
        ShowMonsterGeneratorStatus();
        return 1;
    }
    case 0x36:
        g_generator_save_flag = g_generator_save_flag == 0;
        break;
    }
    ShowMonsterGeneratorStatus();
    return 1;
}

/* The edit-menu keys for the selected monster generator: name entry, table
   picker, active toggle, per-generator or shared chance and interval, and the
   shared-interval flag. */
// FUNCTION: WIZ8 0x0057ad10
int HandleMonsterGeneratorEditKey(unsigned short key)
{
    W8PList* list;
    W8EncounterTableRuntime* table;
    W8EncounterTableRuntime* entry;
    MonGen* generator;
    int current_index;
    int count;
    int index;
    int found;
    int slot;

    if (g_mipe_category_list == 0) {
        list = PLCreate();
        g_mipe_category_list = list;
        if (list != 0) {
            PListClear(list);
            count = g_encounter_tables.count;
            index = 0;
            if (0 < g_encounter_tables.count) {
                do {
                    table = GetEncounterTable(index);
                    if (table->category_150 == static_cast<unsigned int>(g_mipe_category)) {
                        PLAdoptAppend(list, table);
                    }
                    ++index;
                } while (index < count);
            }
        }
    }
    switch (key) {
    case 0x20:
        ShowMonsterGeneratorStatus();
        return 1;
    default:
        return 0;
    case 0x31:
        g_mipe_mode = 0x19;
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Enter the name for this generator:");
        ShowNoticef(0xf, L"%S", g_mipe_state->generator->name);
        return 1;
    case 0x32:
        current_index = g_mipe_state->generator->encounter_table_index;
        if (current_index < 0) {
            g_mipe_table_base = 0;
            g_mipe_table_row = 0;
        } else {
            table = GetEncounterTable(current_index);
            list = g_mipe_category_list;
            g_mipe_category = static_cast<unsigned char>(table->category_150);
            if (g_mipe_category_list != 0) {
                PListClear(g_mipe_category_list);
                count = g_encounter_tables.count;
                index = 0;
                if (0 < g_encounter_tables.count) {
                    do {
                        table = GetEncounterTable(index);
                        if (table->category_150 == static_cast<unsigned int>(g_mipe_category)) {
                            PLAdoptAppend(list, table);
                        }
                        ++index;
                    } while (index < count);
                }
            }
            for (found = 0; found < static_cast<int>(PLLength(g_mipe_category_list)); ++found) {
                table = static_cast<W8EncounterTableRuntime*>(PLGet(g_mipe_category_list, found));
                entry = GetEncounterTable(current_index);
                if (table == entry) {
                    break;
                }
            }
            g_mipe_table_base = (found / 6) * 6;
            g_mipe_table_row = found % 6;
        }
        g_mipe_mode = 0x17;
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Category: %S", *g_encounter_names.GetAt(g_mipe_category & 0xff));
        if (g_mipe_category_list == 0) {
            return 1;
        }
        slot = 0;
        do {
            entry = static_cast<W8EncounterTableRuntime*>(
                PLGet(g_mipe_category_list, g_mipe_table_base + slot));
            if (entry == 0) {
                ShowNoticef(0xf, &g_empty_wide_string);
            } else {
                ShowNoticef(slot == g_mipe_table_row ? 3 : 0xf, L"    %S", entry->name);
            }
            ++slot;
        } while (slot < 6);
        return 1;
    case 0x33:
        g_mipe_state->generator->generation_enabled =
            g_mipe_state->generator->generation_enabled == 0;
        ShowMonsterGeneratorEditor();
        return 1;
    case 0x34:
        generator = g_mipe_state->generator;
        if ((generator->flags >> 3 & 1) == 0) {
            if ('\0' < generator->custom_spawn_chance) {
                generator->custom_spawn_chance -= 10;
                ShowMonsterGeneratorEditor();
                return 1;
            }
        } else if ('\0' < static_cast<char>(g_generator_interval_min)) {
            g_generator_interval_min = static_cast<short>(
                static_cast<char>(static_cast<char>(g_generator_interval_min) - 10));
            ShowMonsterGeneratorEditor();
            return 1;
        }
        break;
    case 0x35:
        generator = g_mipe_state->generator;
        if ((generator->flags >> 3 & 1) == 0) {
            if (generator->custom_spawn_chance < 'd') {
                generator->custom_spawn_chance = generator->custom_spawn_chance + '\n';
                ShowMonsterGeneratorEditor();
                return 1;
            }
        } else if (static_cast<char>(g_generator_interval_min) < 'd') {
            g_generator_interval_min = static_cast<short>(
                static_cast<char>(static_cast<char>(g_generator_interval_min) + '\n'));
            ShowMonsterGeneratorEditor();
            return 1;
        }
        break;
    case 0x36:
        generator = g_mipe_state->generator;
        if ((generator->flags >> 3 & 1) == 0) {
            if (0 < generator->custom_interval_seconds) {
                generator->custom_interval_seconds -= 10;
                g_mipe_state->generator->Reset();
                ShowMonsterGeneratorEditor();
                return 1;
            }
        } else if (0 < g_generator_default_interval) {
            g_generator_default_interval -= 10;
            ShowMonsterGeneratorEditor();
            return 1;
        }
        break;
    case 0x37:
        generator = g_mipe_state->generator;
        if ((generator->flags >> 3 & 1) == 0) {
            generator->custom_interval_seconds += 10;
            g_mipe_state->generator->Reset();
            ShowMonsterGeneratorEditor();
            return 1;
        }
        g_generator_default_interval += 10;
        ShowMonsterGeneratorEditor();
        return 1;
    case 0x38:
        generator = g_mipe_state->generator;
        if ((generator->flags >> 3 & 1) != 0) {
            generator->flags &= 0xfffffff7;
            ShowMonsterGeneratorEditor();
            return 1;
        }
        generator->flags |= 8;
        break;
    }
    ShowMonsterGeneratorEditor();
    return 1;
}

/* Mode-0x17 key handler: the generator's encounter-table picker. Enter binds
   the highlighted table to the selected generator; the arrows page/walk the
   rows and cycle through the categories. */
// FUNCTION: WIZ8 0x0057b1a0
static void HandleMipeGeneratorTableKey(unsigned short key)
{
    W8PList* list;
    W8EncounterTableRuntime* entry;
    int index;
    int found;
    int wraps;
    int count;
    unsigned int table_index;
    int row;
    unsigned int palette;

    count = g_encounter_tables.count;
    switch (key) {
    case 0xd:
        if (g_mipe_state->generator != 0) {
            found = 0;
            table_index = 0;
            index = g_mipe_table_base + g_mipe_table_row;
            if (0 < g_encounter_tables.count) {
                unsigned int category = g_mipe_category & 0xff;
                do {
                    entry = GetEncounterTable(table_index);
                    if (entry->category_150 == category) {
                        if (found == index) {
                            break;
                        }
                        ++found;
                    }
                    ++table_index;
                } while (static_cast<int>(table_index) < count);
            }
            g_mipe_state->generator->SetEncounterTable(table_index & 0xffff);
        }
        ShowMonsterGeneratorEditor();
        g_mipe_mode = 0x16;
        return;
    case 0x20:
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Category: %S", *g_encounter_names.GetAt(g_mipe_category & 0xff));
        if (g_mipe_category_list != 0) {
            row = 0;
            do {
                entry = static_cast<W8EncounterTableRuntime*>(
                    PLGet(g_mipe_category_list, g_mipe_table_base + row));
                if (entry == 0) {
                    ShowNoticef(0xf, &g_empty_wide_string);
                } else {
                    palette = row == g_mipe_table_row ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", entry->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x21:
        if (g_mipe_table_base == 0) {
            return;
        }
        g_mipe_table_base -= 6;
        if (g_mipe_table_base < 0) {
            g_mipe_table_base = 0;
            ShowMipeEncounterCategory();
            return;
        }
        ShowMipeEncounterCategory();
        break;
    case 0x22:
        if (g_mipe_table_base + g_mipe_table_row <
            static_cast<int>(PLLength(g_mipe_category_list) - 6)) {
            g_mipe_table_base += 6;
            ShowMipeEncounterCategory();
            return;
        }
        if (static_cast<int>(PLLength(g_mipe_category_list)) <=
            g_mipe_table_base + g_mipe_table_row) {
            return;
        }
        g_mipe_table_base += 6;
        g_mipe_table_row = 0;
        if (static_cast<int>(PLLength(g_mipe_category_list)) <= g_mipe_table_base) {
            g_mipe_table_base = static_cast<int>(PLLength(g_mipe_category_list)) - 1;
        }
        ShowMipeEncounterCategory();
        break;
    case 0x25:
        g_mipe_table_row = 0;
        g_mipe_table_base = 0;
        wraps = 0;
        do {
            list = g_mipe_category_list;
            unsigned char category;
            if (g_mipe_category == 0) {
                g_mipe_category = static_cast<unsigned char>(g_encounter_names.count);
                ++wraps;
            }
            category = g_mipe_category - 1;
            g_mipe_category = category;
            if (g_mipe_category_list != 0) {
                PListClear(g_mipe_category_list);
                count = g_encounter_tables.count;
                index = 0;
                if (0 < g_encounter_tables.count) {
                    do {
                        entry = GetEncounterTable(index);
                        if (entry->category_150 == static_cast<unsigned int>(category)) {
                            PLAdoptAppend(list, entry);
                        }
                        ++index;
                    } while (index < count);
                }
            }
        } while (PLLength(g_mipe_category_list) == 0 && wraps < 2);
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Category: %S", *g_encounter_names.GetAt(g_mipe_category & 0xff));
        if (g_mipe_category_list != 0) {
            row = 0;
            do {
                entry = static_cast<W8EncounterTableRuntime*>(
                    PLGet(g_mipe_category_list, g_mipe_table_base + row));
                if (entry == 0) {
                    ShowNoticef(0xf, &g_empty_wide_string);
                } else {
                    palette = row == g_mipe_table_row ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", entry->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x26:
        if (g_mipe_table_row == 0) {
            if (g_mipe_table_base != 0) {
                --g_mipe_table_base;
            }
        } else {
            --g_mipe_table_row;
        }
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Category: %S", *g_encounter_names.GetAt(g_mipe_category & 0xff));
        if (g_mipe_category_list != 0) {
            row = 0;
            do {
                entry = static_cast<W8EncounterTableRuntime*>(
                    PLGet(g_mipe_category_list, g_mipe_table_base + row));
                if (entry == 0) {
                    ShowNoticef(0xf, &g_empty_wide_string);
                } else {
                    palette = row == g_mipe_table_row ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", entry->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x27:
        g_mipe_table_row = 0;
        g_mipe_table_base = 0;
        wraps = 0;
        do {
            list = g_mipe_category_list;
            if ((g_mipe_category & 0xff) < g_encounter_names.count - 1) {
                ++g_mipe_category;
            } else {
                ++wraps;
                g_mipe_category = 0;
            }
            if (g_mipe_category_list != 0) {
                PListClear(g_mipe_category_list);
                count = g_encounter_tables.count;
                index = 0;
                if (0 < g_encounter_tables.count) {
                    do {
                        entry = GetEncounterTable(index);
                        if (entry->category_150 == static_cast<unsigned int>(g_mipe_category)) {
                            PLAdoptAppend(list, entry);
                        }
                        ++index;
                    } while (index < count);
                }
            }
        } while (PLLength(g_mipe_category_list) == 0 && wraps < 2);
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Category: %S", *g_encounter_names.GetAt(g_mipe_category & 0xff));
        if (g_mipe_category_list != 0) {
            row = 0;
            do {
                entry = static_cast<W8EncounterTableRuntime*>(
                    PLGet(g_mipe_category_list, g_mipe_table_base + row));
                if (entry == 0) {
                    ShowNoticef(0xf, &g_empty_wide_string);
                } else {
                    palette = row == g_mipe_table_row ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", entry->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x28:
        if (g_mipe_table_row < 5 && g_mipe_table_base + g_mipe_table_row <
                                        static_cast<int>(PLLength(g_mipe_category_list) - 1)) {
            ++g_mipe_table_row;
            ShowMipeEncounterCategory();
            return;
        }
        if (g_mipe_table_base + g_mipe_table_row <
            static_cast<int>(PLLength(g_mipe_category_list) - 1)) {
            ++g_mipe_table_base;
            ShowMipeEncounterCategory();
            return;
        }
    }
}

/* Character entry for the generator name, capped at the 32-byte field. */
// FUNCTION: WIZ8 0x0057b7e0
void EditMonsterGeneratorName(unsigned short key)
{
    char* name;
    int length;

    name = g_mipe_state->generator->name;
    length = strlen(name);
    if (key == 8) {
        if (0 < length) {
            name[length - 1] = '\0';
        }
    } else if ((((0x2f < key) && (key < 0x3a)) || ((0x40 < key) && (key < 0x5b))) &&
               (length < 0x1f)) {
        if (gfKeyState[0x10] == 0) {
            key += 0x20;
        }
        name[length] = static_cast<char>(key);
        name[length + 1] = '\0';
    }
    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Enter the name for this generator:");
    ShowNoticef(0xf, L"%S", g_mipe_state->generator->name);
}

/* Mode-0x1b key handler: the selected prop trigger's locks & traps editor.
   '1' cycles the lock type and re-rolls the pin state, '2' enters key-id
   input, '3'/'4' bump the difficulty, and every path repaints the menu. */
// FUNCTION: WIZ8 0x0057b880
static void HandleMipeLockTrapKey(unsigned short key)
{
    Trigger* trigger;
    Trigger* action_trigger;
    W8TriggerActionData* action;
    const wchar_t* key_name;
    W8LockState* lock_state;
    int key_id;
    bool pending;

    trigger = g_mipe_state->prop->GetValue18();
    lock_state = &trigger->lock_state;
    action_trigger = g_mipe_state->prop->GetValue18();
    action = action_trigger->m_pActionData;
    if (action == 0 || action->type_004 != '\n') {
        action = 0;
    }
    switch (key) {
    case 0x31:
        ++lock_state->lock_type;
        if (3 < lock_state->lock_type) {
            lock_state->lock_type = 0;
        }
        lock_state->Reset();
        if (action == 0) {
            if (lock_state->lock_type == 3) {
                key_id = trigger->lock_state.key_id;
                g_mipe_state->prop->GetValue18()->required_item_id = key_id;
            } else {
                g_mipe_state->prop->GetValue18()->required_item_id = 0xffffffff;
            }
        } else {
            if (lock_state->lock_type == 0 || trigger->lock_state.device_state.completed != 0) {
                pending = 0;
            } else {
                pending = 1;
            }
            static_cast<W8DoorTriggerActionData*>(action)->flags_008 =
                (pending << 2) | (static_cast<W8DoorTriggerActionData*>(action)->flags_008 & 0xfb);
            static_cast<W8DoorTriggerActionData*>(action)->item_00a =
                static_cast<short>(trigger->lock_state.key_id);
        }
        break;
    case 0x32:
        g_mipe_mode = 0x1c;
        trigger = g_mipe_state->prop->GetValue18();
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Enter Key ID:");
        ShowNoticef(0xf, g_format_d_0060aa20, trigger->lock_state.key_id);
        return;
    case 0x33:
        ++trigger->lock_state.difficulty;
        if (10 < trigger->lock_state.difficulty) {
            trigger->lock_state.difficulty = 10;
        }
        break;
    case 0x34:
        --trigger->lock_state.difficulty;
        if (trigger->lock_state.difficulty < 0) {
            trigger->lock_state.difficulty = 0;
        }
    }
    trigger = g_mipe_state->prop->GetValue18();
    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Edit Locks & Traps");
    ShowNoticef(0xf, L"1) Type: %s", g_lock_type_names[trigger->lock_state.lock_type]);
    key_id = trigger->lock_state.key_id;
    if (key_id < 0) {
        key_name = &g_empty_wide_string;
    } else {
        key_name = g_item_records[key_id].display_name;
    }
    ShowNoticef(0xf, L"2) Key Id: (%d) %s", key_id, key_name);
    ShowNoticef(0xf, L" Difficulty (3+/4-): %d", trigger->lock_state.difficulty);
}

/* The digit keys editing the selected prop trigger's key id. */
// FUNCTION: WIZ8 0x0057ba60
void EditTriggerKeyID(unsigned int key)
{
    Trigger* trigger;
    W8TriggerActionData* action;
    int key_id;
    bool pending;

    trigger = g_mipe_state->prop->GetValue18();
    action = g_mipe_state->prop->GetValue18()->m_pActionData;
    if (action == 0 || action->type_004 != '\n') {
        action = 0;
    }
    key_id = trigger->lock_state.key_id;
    switch (key & 0xffff) {
    case 8:
        key_id /= 10;
        break;
    case 0x2d:
        key_id = -1;
        break;
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
        key_id = ((key & 0xffff) - 0x30) + key_id * 10;
        break;
    }
    trigger->lock_state.key_id = key_id;
    if (action == 0) {
        if (trigger->lock_state.lock_type == 3) {
            g_mipe_state->prop->GetValue18()->required_item_id = key_id;
        } else {
            g_mipe_state->prop->GetValue18()->required_item_id = 0xffffffff;
        }
    } else {
        if (trigger->lock_state.lock_type == 0 || trigger->lock_state.device_state.completed != 0) {
            pending = 0;
        } else {
            pending = 1;
        }
        static_cast<W8DoorTriggerActionData*>(action)->flags_008 =
            (pending << 2) | (static_cast<W8DoorTriggerActionData*>(action)->flags_008 & 0xfb);
        static_cast<W8DoorTriggerActionData*>(action)->item_00a =
            static_cast<short>(trigger->lock_state.key_id);
    }
    trigger = g_mipe_state->prop->GetValue18();
    ResetEditorStatusLine(-1);
    ShowNoticef(6, L"Enter Key ID:");
    ShowNoticef(0xf, g_format_d_0060aa20, trigger->lock_state.key_id);
}

/* Mode-0x1d key handler: the prop trigger's treasure-table picker. Enter
   copies the highlighted table's name into the trigger's inline action data
   and returns to the prop menu; the arrows page/walk rows and cycle the item
   table categories. */
// FUNCTION: WIZ8 0x0057bbd0
static void HandleMipeItemTableKey(unsigned short key)
{
    W8PList* list;
    Trigger* trigger;
    W8ItemTableRecord* table;
    unsigned int table_index;
    int wraps;
    int index;
    int row;
    unsigned int palette;

    switch (key) {
    case 0xd:
        if (g_mipe_state->prop != 0) {
            table_index =
                FindCategoryItemTable(g_mipe_category & 0xff, g_mipe_table_base + g_mipe_table_row);
            table = g_item_tables[table_index & 0xffff];
            trigger = g_mipe_state->prop->GetValue18();
            strcpy(trigger->inline_action_data_24c, table->name);
            trigger->items_generated = 0;
        }
        ShowMipePropMenu();
        g_mipe_mode = 0xd;
        return;
    case 0x20:
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Category: %S", g_item_table_category_names[g_mipe_category & 0xff]);
        if (g_mipe_category_list != 0) {
            row = 0;
            do {
                table = static_cast<W8ItemTableRecord*>(
                    PLGet(g_mipe_category_list, g_mipe_table_base + row));
                if (table == 0) {
                    ShowNoticef(0xf, &g_empty_wide_string);
                } else {
                    palette = row == g_mipe_table_row ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", table->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x21:
        if (g_mipe_table_base != 0) {
            g_mipe_table_base -= 6;
            if (g_mipe_table_base < 0) {
                g_mipe_table_base = 0;
            }
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Category: %S", g_item_table_category_names[g_mipe_category & 0xff]);
            if (g_mipe_category_list != 0) {
                row = 0;
                do {
                    table = static_cast<W8ItemTableRecord*>(
                        PLGet(g_mipe_category_list, g_mipe_table_base + row));
                    if (table == 0) {
                        ShowNoticef(0xf, &g_empty_wide_string);
                    } else {
                        palette = row == g_mipe_table_row ? 3 : 0xf;
                        ShowNoticef(palette, L"    %S", table->name);
                    }
                    ++row;
                } while (row < 6);
                return;
            }
        }
        break;
    case 0x22:
        if (g_mipe_table_base + g_mipe_table_row <
            static_cast<int>(PLLength(g_mipe_category_list) - 6)) {
            g_mipe_table_base += 6;
            ShowMipeItemTableCategory();
            return;
        }
        if (g_mipe_table_base + g_mipe_table_row <
            static_cast<int>(PLLength(g_mipe_category_list))) {
            g_mipe_table_base += 6;
            g_mipe_table_row = 0;
            if (static_cast<int>(PLLength(g_mipe_category_list)) <= g_mipe_table_base) {
                g_mipe_table_base = static_cast<int>(PLLength(g_mipe_category_list)) - 1;
            }
            ShowMipeItemTableCategory();
        }
        break;
    case 0x25:
        wraps = 0;
        g_mipe_table_row = 0;
        g_mipe_table_base = 0;
        do {
            unsigned char category;
            list = g_mipe_category_list;
            category = g_mipe_category;
            if (g_mipe_category == 0) {
                ++wraps;
                category = static_cast<unsigned char>(gXStatus.uiItemTableCategories);
            }
            g_mipe_category = category - 1;
            if (g_mipe_category_list != 0) {
                PListClear(g_mipe_category_list);
                for (index = 0; index < static_cast<int>(gXStatus.uiItemTablesInDatabase);
                     ++index) {
                    table = g_item_tables[index];
                    if (table->category_id == static_cast<unsigned int>(category - 1)) {
                        PLAdoptAppend(list, table);
                    }
                }
            }
        } while (PLLength(g_mipe_category_list) == 0 && wraps < 2);
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Category: %S", g_item_table_category_names[g_mipe_category & 0xff]);
        if (g_mipe_category_list != 0) {
            row = 0;
            do {
                table = static_cast<W8ItemTableRecord*>(
                    PLGet(g_mipe_category_list, g_mipe_table_base + row));
                if (table == 0) {
                    ShowNoticef(0xf, &g_empty_wide_string);
                } else {
                    palette = row == g_mipe_table_row ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", table->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x26:
        if (g_mipe_table_row == 0) {
            if (g_mipe_table_base != 0) {
                --g_mipe_table_base;
            }
        } else {
            --g_mipe_table_row;
        }
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Category: %S", g_item_table_category_names[g_mipe_category & 0xff]);
        if (g_mipe_category_list != 0) {
            row = 0;
            do {
                table = static_cast<W8ItemTableRecord*>(
                    PLGet(g_mipe_category_list, g_mipe_table_base + row));
                if (table == 0) {
                    ShowNoticef(0xf, &g_empty_wide_string);
                } else {
                    palette = row == g_mipe_table_row ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", table->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x27:
        wraps = 0;
        g_mipe_table_row = 0;
        g_mipe_table_base = 0;
        do {
            list = g_mipe_category_list;
            if ((g_mipe_category & 0xff) < gXStatus.uiItemTableCategories - 1) {
                ++g_mipe_category;
            } else {
                ++wraps;
                g_mipe_category = 0;
            }
            if (g_mipe_category_list != 0) {
                PListClear(g_mipe_category_list);
                for (index = 0; index < static_cast<int>(gXStatus.uiItemTablesInDatabase);
                     ++index) {
                    table = g_item_tables[index];
                    if (table->category_id == static_cast<unsigned int>(g_mipe_category & 0xff)) {
                        PLAdoptAppend(list, table);
                    }
                }
            }
        } while (PLLength(g_mipe_category_list) == 0 && wraps < 2);
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Category: %S", g_item_table_category_names[g_mipe_category & 0xff]);
        if (g_mipe_category_list != 0) {
            row = 0;
            do {
                table = static_cast<W8ItemTableRecord*>(
                    PLGet(g_mipe_category_list, g_mipe_table_base + row));
                if (table == 0) {
                    ShowNoticef(0xf, &g_empty_wide_string);
                } else {
                    palette = row == g_mipe_table_row ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", table->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x28:
        if (g_mipe_table_row < 5 && g_mipe_table_base + g_mipe_table_row <
                                        static_cast<int>(PLLength(g_mipe_category_list) - 1)) {
            ++g_mipe_table_row;
            ShowMipeItemTableCategory();
            return;
        }
        if (g_mipe_table_base + g_mipe_table_row <
            static_cast<int>(PLLength(g_mipe_category_list) - 1)) {
            ++g_mipe_table_base;
            ShowMipeItemTableCategory();
            return;
        }
    }
}

/* The MIPE master key handler, fed input atoms while the panel is up. The
   early Escape block unwinds the mode stack, the combat-timer shortcut keeps
   only 'V'/'X' live, and the mode switch routes everything else into the
   per-mode handlers above. */
// FUNCTION: WIZ8 0x0057c230
unsigned char HandleMipeKey(const InputAtom* event)
{
    bool handled;
    unsigned short key;
    unsigned short event_type;
    Trigger* trigger;
    W8Item* rep_item;
    const wchar_t* key_name;
    wchar_t name[100];
    int key_id;
    int shown;

    handled = 0;
    if (g_mipe_state->dragging != 0) {
        return 1;
    }
    key = static_cast<unsigned short>(event->usParam);
    if (key == 0x1b) {
        if (event->usEvent == 2 || event->usEvent == 4) {
            g_mipe_count = 0;
            if (g_mipe_mode == 6) {
                g_mipe_mode = 1;
                ShowMipeMonsterStatus();
            } else if (g_mipe_mode == 7) {
                g_mipe_mode = 2;
                ShowMipeItemStatus();
            } else if (g_mipe_mode == 2 || g_mipe_mode == 1) {
                g_mipe_mode = 0;
                if (g_mipe_category_list != 0) {
                    PLDestroy(g_mipe_category_list);
                    g_mipe_category_list = 0;
                }
            } else {
                if (g_mipe_mode == 5) {
                    g_mipe_mode = 0;
                } else {
                    if (g_mipe_mode == 8) {
                        g_mipe_mode = 5;
                        ShowMipeEditMenu();
                        g_debug_monster_cycle = 0;
                        return 1;
                    }
                    if (g_mipe_mode == 0xd) {
                        g_mipe_mode = 5;
                        ShowWorldCursor();
                        ShowMipeEditMenu();
                    } else if (g_mipe_mode == 0xe) {
                        g_mipe_mode = 0xd;
                        HandleMipeEditPropKey(0x1b);
                        ShowMipePropMenu();
                    } else if (g_mipe_mode == 3) {
                        g_mipe_mode = 8;
                        ResetEditorStatusLine(-1);
                        ShowNoticef(6, L"Edit Monster");
                        ShowNoticef(0xf, L"1) Slow <<");
                        ShowNoticef(0xf, L"2) Fast >>");
                        ShowNoticef(0xf, L"3) Set Speed");
                        ShowNoticef(0xf, L"4) Assign Path");
                        ShowNoticef(0xf, L"5) Next Cycle");
                        ShowNoticef(0xf, L"6) Direction");
                    } else if (g_mipe_mode == 9) {
                        ShowWorldCursor();
                        g_mipe_mode = 0;
                        g_mipe_state->selecting = 0;
                    } else {
                        if (g_mipe_mode != 4) {
                            if (g_mipe_mode == 0xb) {
                                g_mipe_mode = 8;
                                g_mipe_state->selecting = 0;
                                ResetEditorStatusLine(-1);
                                ShowNoticef(6, L"Edit Monster");
                                ShowNoticef(0xf, L"1) Slow <<");
                                ShowNoticef(0xf, L"2) Fast >>");
                                ShowNoticef(0xf, L"3) Set Speed");
                                ShowNoticef(0xf, L"4) Assign Path");
                                ShowNoticef(0xf, L"5) Next Cycle");
                                ShowNoticef(0xf, L"6) Direction");
                            } else if (g_mipe_mode == 0xc) {
                                if (g_mipe_state->selecting == 0) {
                                    g_mipe_mode = 0;
                                    trigger = g_mipe_state->trigger;
                                    if (trigger != 0 &&
                                        (rep_item = trigger->rep_item_114, rep_item != 0)) {
                                        rep_item->SetHighlight(false);
                                    }
                                    g_mipe_state->trigger = 0;
                                } else {
                                    ShowWorldCursor();
                                    g_mipe_state->selecting = 0;
                                    ShowMipeTriggerMenu();
                                }
                            } else if (g_mipe_mode == 0x15) {
                                if (g_mipe_state->selecting == 0) {
                                    g_mipe_mode = 0;
                                    if (g_mipe_state->generator != 0 &&
                                        g_mipe_state->generator->marker_item != 0) {
                                        g_mipe_state->generator->marker_item->SetHighlight(false);
                                    }
                                    g_mipe_state->generator = 0;
                                    if (g_mipe_category_list != 0) {
                                        PLDestroy(g_mipe_category_list);
                                        g_mipe_category_list = 0;
                                    }
                                } else {
                                    ShowWorldCursor();
                                    g_mipe_state->selecting = 0;
                                    ShowMonsterGeneratorStatus();
                                }
                            } else if (g_mipe_mode == 0x16) {
                                g_mipe_state->selecting = 0;
                                g_mipe_mode = 0x15;
                                ShowMonsterGeneratorStatus();
                            } else if (g_mipe_mode == 0x17) {
                                g_mipe_state->selecting = 0;
                                g_mipe_mode = 0x16;
                                ShowMonsterGeneratorEditor();
                            } else if (g_mipe_mode == 0xf) {
                                if (g_mipe_state->selecting == 0) {
                                    ShowMipeTriggerMenu();
                                    SetWorldCursorNodesVisible(g_mipe_trigger_display);
                                    g_mipe_mode = 0xc;
                                    g_mipe_menu_active = 0;
                                } else {
                                    ShowWorldCursor();
                                    g_mipe_state->selecting = 0;
                                    ResetEditorStatusLine(-1);
                                    ShowNoticef(6, L"Choose an action:");
                                    ShowNoticef(0xf, L"1) Create cube.");
                                    ShowNoticef(0xf, L"2) Delete cube.");
                                    ShowNoticef(0xf, L"3) Edit cube parameters.");
                                    ShowNoticef(0xf, L"4) Move cube.");
                                    ShowNoticef(0xf, L"5) Scale cube.");
                                    ShowNoticef(0xf, L"6) Select cube.");
                                }
                            } else {
                                if (g_mipe_mode != 0x10) {
                                    if (g_mipe_mode == 0x11) {
                                        g_mipe_mode = 0xf;
                                        g_mipe_state->selecting = 0;
                                        HideWorldCursor();
                                        g_mipe_state->dragging = false;
                                        g_mipe_menu_active = 1;
                                        ResetEditorStatusLine(-1);
                                        ShowNoticef(6, L"Choose an action:");
                                        ShowNoticef(0xf, L"1) Create cube.");
                                        ShowNoticef(0xf, L"2) Delete cube.");
                                        ShowNoticef(0xf, L"3) Edit cube parameters.");
                                        ShowNoticef(0xf, L"4) Move cube.");
                                        ShowNoticef(0xf, L"5) Scale cube.");
                                        ShowNoticef(0xf, L"6) Select cube.");
                                        return 1;
                                    }
                                    if (g_mipe_mode == 0x12 || g_mipe_mode == 0x13) {
                                        g_mipe_mode = 0xf;
                                        ResetEditorStatusLine(-1);
                                        ShowNoticef(6, L"Choose an action:");
                                        ShowNoticef(0xf, L"1) Create cube.");
                                        ShowNoticef(0xf, L"2) Delete cube.");
                                        ShowNoticef(0xf, L"3) Edit cube parameters.");
                                        ShowNoticef(0xf, L"4) Move cube.");
                                        ShowNoticef(0xf, L"5) Scale cube.");
                                        ShowNoticef(0xf, L"6) Select cube.");
                                        g_mipe_state->selecting = 0;
                                    } else if (g_mipe_mode == 0x18) {
                                        g_mipe_mode = 0x15;
                                        ShowWorldCursor();
                                        ShowMonsterGeneratorStatus();
                                    } else if (g_mipe_mode == 0x19) {
                                        g_mipe_mode = 0x16;
                                        ShowWorldCursor();
                                        ShowMonsterGeneratorEditor();
                                    } else if (g_mipe_mode == 0x1a) {
                                        g_mipe_mode = 0x15;
                                        ShowWorldCursor();
                                        ShowMonsterGeneratorStatus();
                                    } else if (g_mipe_mode == 0x1b) {
                                        g_mipe_mode = 0xd;
                                        ShowMipePropMenu();
                                    } else if (g_mipe_mode == 0x1c) {
                                        g_mipe_mode = 0x1b;
                                        trigger = g_mipe_state->prop->GetValue18();
                                        ResetEditorStatusLine(-1);
                                        ShowNoticef(6, L"Edit Locks & Traps");
                                        ShowNoticef(
                                            0xf, L"1) Type: %s",
                                            g_lock_type_names[trigger->lock_state.lock_type]);
                                        key_id = trigger->lock_state.key_id;
                                        if (key_id < 0) {
                                            key_name = &g_empty_wide_string;
                                        } else {
                                            key_name = g_item_records[key_id].display_name;
                                        }
                                        ShowNoticef(0xf, L"2) Key Id: (%d) %s", key_id, key_name);
                                        ShowNoticef(0xf, L" Difficulty (3+/4-): %d",
                                                    trigger->lock_state.difficulty);
                                    } else {
                                        if (g_mipe_mode == 0x1d) {
                                            g_mipe_mode = 0xd;
                                            ShowMipePropMenu();
                                            g_mipe_menu_active = 1;
                                            if (g_mipe_category_list != 0) {
                                                PLDestroy(g_mipe_category_list);
                                                g_mipe_category_list = 0;
                                            }
                                            return 1;
                                        }
                                        if (g_mipe_mode < 1) {
                                            g_mipe_state->monster = 0;
                                            ToggleMipePanel();
                                            return 1;
                                        }
                                        g_mipe_mode = 0;
                                    }
                                } else {
                                    g_mipe_mode = 0xf;
                                    ResetEditorStatusLine(-1);
                                    ShowNoticef(6, L"Choose an action:");
                                    ShowNoticef(0xf, L"1) Create cube.");
                                    ShowNoticef(0xf, L"2) Delete cube.");
                                    ShowNoticef(0xf, L"3) Edit cube parameters.");
                                    ShowNoticef(0xf, L"4) Move cube.");
                                    ShowNoticef(0xf, L"5) Scale cube.");
                                    ShowNoticef(0xf, L"6) Select cube.");
                                    g_mipe_menu_active = 1;
                                }
                            }
                        } else {
                            ShowWorldCursor();
                            g_mipe_mode = 0;
                            g_mipe_state->selecting = 0;
                        }
                    }
                }
            }
            handled = 1;
        }
    }
    event_type = event->usEvent;
    if (event_type == 1) {
        switch (key) {
        case 0x25:
        case 0x26:
        case 0x27:
        case 0x28:
            if (g_mipe_mode != 6 && g_mipe_mode != 7 && g_mipe_mode != 0xf && g_mipe_mode != 0x12) {
                return handled;
            }
            return 1;
        default:
            return handled;
        case 8:
        case 0xd:
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x4f:
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
        case 0x58:
        case 0x59:
        case 0x5a:
        case 0xbc:
        case 0xbe:
            return 1;
        }
    }
    if (event_type != 2 && event_type != 4) {
        return handled;
    }
    if (g_monster_combat_timer_enabled != 0) {
        if (key == 0x56) {
            HideWorldCursor();
            g_mipe_mode = 0xf;
            g_mipe_menu_active = 1;
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Choose an action:");
            ShowNoticef(0xf, L"1) Create cube.");
            ShowNoticef(0xf, L"2) Delete cube.");
            ShowNoticef(0xf, L"3) Edit cube parameters.");
            ShowNoticef(0xf, L"4) Move cube.");
            ShowNoticef(0xf, L"5) Scale cube.");
            ShowNoticef(0xf, L"6) Select cube.");
            SetWorldCursorNodesVisible(1);
            return handled;
        }
        if (key != 0x58) {
            return handled;
        }
        ToggleMipePanel();
        return handled;
    }
    switch (g_mipe_mode) {
    case 0:
        switch (key) {
        case 0x30:
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"What would you like to do?");
            ShowNoticef(0xf, L"1) Create a monster.");
            ShowNoticef(0xf, L"2) Create an item.");
            ShowNoticef(0xf, L"3) Edit object(s).");
            ShowNoticef(0xf, L"4) Monster Generators.");
            ShowNoticef(0xf, L"5) Select object(s).");
            ShowNoticef(0xf, L"6) Handle triggers.");
            g_mipe_mode = 0;
            return handled;
        case 0x31:
            g_mipe_count = 0;
            ShowMipeMonsterStatus();
            g_mipe_mode = 1;
            return handled;
        case 0x32:
            g_mipe_count = 0;
            ShowMipeItemStatus();
            g_mipe_mode = 2;
            return handled;
        case 0x33:
            ShowMipeEditMenu();
            g_mipe_mode = 5;
            return handled;
        case 0x35:
            ShowMipeChooseMenu();
            HideWorldCursor();
            g_mipe_state->selecting = 1;
            g_mipe_mode = 9;
            return handled;
        case 0x36:
            ShowMipeTriggerMenu();
            g_mipe_mode = 0xc;
            return handled;
        case 0x34:
            ShowMonsterGeneratorStatus();
            g_mipe_mode = 0x15;
            return handled;
        default:
            break;
        }
        break;
    case 1:
        return HandleMipeMonsterCreateKey(key);
    case 2:
        return HandleMipeItemCreateKey(key);
    case 3:
        if (key == 0x35) {
            g_mipe_mode = 10;
            g_mipe_state->waypoint_count = 0;
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Type 'C' to create a waypoint.");
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(3, L"Laying down waypoint %d", g_mipe_state->waypoint_count);
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, L"Type X to delete last waypoint.");
            return handled;
        }
        break;
    case 5:
        if (key == 0x31) {
            g_mipe_count = 0;
            g_mipe_mode = 8;
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Edit Monster");
            ShowNoticef(0xf, L"1) Slow <<");
            ShowNoticef(0xf, L"2) Fast >>");
            ShowNoticef(0xf, L"3) Set Speed");
            ShowNoticef(0xf, L"4) Assign Path");
            ShowNoticef(0xf, L"5) Next Cycle");
            ShowNoticef(0xf, L"6) Direction");
            g_debug_monster_cycle = 1;
            return handled;
        }
        if (key == 0x32) {
            g_mipe_count = 0;
            g_mipe_mode = 0xd;
            HideWorldCursor();
            ShowMipePropMenu();
            return handled;
        }
        if (key == 0x34) {
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"MOVE IT!!");
            ShowNoticef(0xf, L"Type C to change how to choose.");
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, g_mipe_choose_group == 0 ? L"Choosing: One" : L"Choosing: Group");
            g_mipe_mode = 4;
            g_mipe_state->selecting = 1;
            HideWorldCursor();
            return handled;
        }
        break;
    case 4:
        if (key == 0x43) {
            g_mipe_choose_group = g_mipe_choose_group == 0;
            if (g_mipe_state != 0) {
                IListClear(&g_mipe_state->monster_ids);
            }
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"MOVE IT!!");
            ShowNoticef(0xf, L"Type C to change how to choose.");
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, g_mipe_choose_group == 0 ? L"Choosing: One" : L"Choosing: Group");
            return handled;
        }
        break;
    case 6:
        HandleMipeMonsterCategoryKey(key);
        return handled;
    case 7:
        HandleMipeItemCategoryKey(key);
        return handled;
    case 8:
        HandleMonsterDebugKey(key);
        return handled;
    case 0xd:
        HandleMipePropEditKey(key);
        return handled;
    case 0xe:
        HandleMipeEditPropKey(key);
        return handled;
    case 9:
        if (key == 0x43) {
            g_mipe_choose_group = g_mipe_choose_group == 0;
            if (g_mipe_state != 0) {
                IListClear(&g_mipe_state->monster_ids);
            }
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Choose monster or item to edit.");
            ShowNoticef(0xf, L"Type C to change how to choose.");
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, &g_empty_wide_string);
            ShowNoticef(0xf, g_mipe_choose_group == 0 ? L"Choosing: One" : L"Choosing: Group");
            return handled;
        }
        break;
    case 10:
        HandleWaypointKey(key);
        return handled;
    case 0xb:
        AdjustMonsterSpeed(key);
        return handled;
    case 0xc:
        switch (key) {
        case 0x20:
            ShowMipeTriggerMenu();
            return handled;
        case 0x35:
            g_mipe_trigger_display = g_mipe_trigger_display == 0;
            SetWorldCursorNodesVisible(g_mipe_trigger_display);
            ShowMipeTriggerMenu();
            return handled;
        case 0x36:
            g_mipe_mode = 0xf;
            HideWorldCursor();
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Choose an action:");
            ShowNoticef(0xf, L"1) Create cube.");
            ShowNoticef(0xf, L"2) Delete cube.");
            ShowNoticef(0xf, L"3) Edit cube parameters.");
            ShowNoticef(0xf, L"4) Move cube.");
            ShowNoticef(0xf, L"5) Scale cube.");
            ShowNoticef(0xf, L"6) Select cube.");
            g_mipe_menu_active = 1;
            SetWorldCursorNodesVisible(1);
            return handled;
        }
        break;
    case 0xf:
        HandleCubeMenuKey(key);
        return handled;
    case 0x10:
        HandleCubeParameterKey(key);
        return handled;
    case 0x12:
        HandleCubeScaleKey(key);
        return handled;
    case 0x14:
        if (key == 0x31) {
            ShowMonsterGeneratorStatus();
            g_mipe_mode = 0x15;
            return handled;
        }
        break;
    case 0x15:
        HandleMonsterGeneratorKey(key);
        return handled;
    case 0x16:
        HandleMonsterGeneratorEditKey(key);
        return handled;
    case 0x17:
        HandleMipeGeneratorTableKey(key);
        return handled;
    case 0x18:
        switch (key) {
        case 0x20:
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Enter the limit for random encounters:");
            ShowNoticef(0xf, g_format_d_0060aa20, g_random_encounter_limit);
            return handled;
        case 8:
            g_random_encounter_limit /= 10;
            break;
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
            g_random_encounter_limit = (key - 0x30) + g_random_encounter_limit * 10;
            break;
        default:
            return handled;
        }
        ResetEditorStatusLine(-1);
        ShowNoticef(6, L"Enter the limit for random encounters:");
        ShowNoticef(0xf, g_format_d_0060aa20, g_random_encounter_limit);
        return handled;
    case 0x19:
        EditMonsterGeneratorName(key);
        return handled;
    case 0x1a:
        switch (key) {
        case 0x20:
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Enter the encounter culling time (sec):");
            shown = g_encounter_culling_time_seconds;
            break;
        case 8:
            g_encounter_culling_time_seconds /= 10;
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Enter the encounter culling time (sec):");
            shown = g_encounter_culling_time_seconds;
            break;
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
            g_encounter_culling_time_seconds = (key - 0x30) + g_encounter_culling_time_seconds * 10;
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Enter the encounter culling time (sec):");
            ShowNoticef(0xf, g_format_d_0060aa20, g_encounter_culling_time_seconds);
            return handled;
        default:
            return handled;
        }
        ShowNoticef(0xf, g_format_d_0060aa20, shown);
        return handled;
    case 0x1b:
        HandleMipeLockTrapKey(key);
        return handled;
    case 0x1c:
        EditTriggerKeyID(key);
        return handled;
    case 0x1d:
        HandleMipeItemTableKey(key);
        return handled;
    }
    return handled;
}

// FUNCTION: WIZ8 0x0057dbb0
unsigned char GetFlag68F105(void)
{
    return g_mipe_active;
}
// FUNCTION: WIZ8 0x0057dbc0
unsigned char GetFlag68F104(void)
{
    return g_mipe_menu_active;
}

/* Index of the `ordinal`-th item table in `category`, or the table count when
   fewer than `ordinal` tables match. */
// FUNCTION: WIZ8 0x0057dbd0
int FindCategoryItemTable(unsigned int category, int ordinal)
{
    int found;
    int index;
    W8ItemTableRecord** tables;

    found = 0;
    index = 0;
    if (0 < static_cast<int>(gXStatus.uiItemTablesInDatabase)) {
        tables = g_item_tables;
        do {
            if (tables[0]->category_id == (category & 0xff)) {
                if (found == ordinal) {
                    return index;
                }
                ++found;
            }
            ++index;
            ++tables;
        } while (index < static_cast<int>(gXStatus.uiItemTablesInDatabase));
    }
    return index;
}

/* Per-tick world-view pick while selecting is armed. In mode 0x15 it re-picks
   the nearest camera-close generator marker whose item is selected, refreshes
   marker highlights and the generator status; otherwise it picks the monster
   under the cursor. The group-choose flag collects every active monster of the
   pick's group into monster_ids; the single-monster path keeps at most one. */
// FUNCTION: WIZ8 0x0057dc20
void UpdateMipeSelection(void)
{
    POINT point;
    MonGen* picked;
    W8MonsterInfo* info;
    W8Item* marker;
    float best_distance;
    int group_id;
    int location_id;
    int index;

    if (g_mipe_state->selecting == 0) {
        return;
    }
    SGPMouseGetPos(&point);
    if (g_mipe_mode == 0x15) {
        picked = 0;
        best_distance = 999999.0f;
        for (index = 0; index < GetMonsterGeneratorCount(); ++index) {
            MonGen* entry = GetMonsterGenerator(index);
            float distance;

            marker = entry->marker_item;
            if (marker == 0 || marker->IsSelected() == 0) {
                continue;
            }
            distance = marker->DistanceToCamera(GetWorld());
            if (distance < best_distance) {
                best_distance = distance;
                picked = entry;
            }
        }
        for (index = 0; index < GetMonsterGeneratorCount(); ++index) {
            marker = GetMonsterGenerator(index)->marker_item;
            if (marker != 0) {
                static_cast<W8ItemRep*>(marker->m_pRep)->SetFlags(0x10, 0);
                marker->SetHighlight(0);
            }
        }
        if (picked != 0) {
            marker = picked->marker_item;
            if (marker != 0) {
                static_cast<W8ItemRep*>(marker->m_pRep)->SetFlags(0x10, 1);
                marker->SetHighlight(1);
            }
        }
        g_mipe_state->generator = picked;
        ShowMonsterGeneratorStatus();
    }
    location_id = PickNearestMonsterUnderCursor(point.x, point.y);
    if (g_mipe_choose_group != 0) {
        if (location_id == -1) {
            group_id = W8_MIPE_NO_GROUP;
        } else {
            group_id = MonsterGetScriptPartByLocationIndex(
                           MonsterGetIndexByLocationID(0x10bb, MIPE_CPP, location_id, 1))
                           ->monster_group_id;
        }
        if (group_id == g_mipe_state->selected_group_id) {
            return;
        }
        for (index = 0; index < static_cast<int>(ILLength(&g_mipe_state->monster_ids)); ++index) {
            int listed = IListGetAt(&g_mipe_state->monster_ids, index);
            info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x10cd, MIPE_CPP, listed, 1));
            if (info->fActive != 0) {
                SetMonsterHighlight(0, listed, 0);
            }
        }
        IListClear(&g_mipe_state->monster_ids);
        for (index = 0; index < static_cast<int>(PLLength(gXStatus.plsMonsterList)); ++index) {
            info = static_cast<W8MonsterInfo*>(PLGet(gXStatus.plsMonsterList, index));
            if (info->fActive != 0 && info->monster_group_id == group_id) {
                SetMonsterHighlight(0, info->location_id, 1);
                IListAdd(&g_mipe_state->monster_ids, info->location_id);
            }
        }
        if (ILLength(&g_mipe_state->monster_ids) == 1) {
            info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x10e8, MIPE_CPP, location_id, 1));
            if (info == 0) {
                srAssertFail("pMonsterInfo", MIPE_CPP, 0x10ea, 0);
            }
            g_mipe_state->monster = info->p3D;
        }
        g_mipe_state->selected_group_id = group_id;
        SetWorldCursorGroupId(group_id);
        return;
    }
    if (location_id != -1 && IListIndexOf(&g_mipe_state->monster_ids, location_id) != -1) {
        return;
    }
    if (ILLength(&g_mipe_state->monster_ids) != 0) {
        int old_id = IListGetAt(&g_mipe_state->monster_ids, 0);
        IListClear(&g_mipe_state->monster_ids);
        SetMonsterHighlight(0, old_id, 0);
    }
    if (location_id == -1) {
        return;
    }
    SetMonsterHighlight(0, location_id, 1);
    IListAdd(&g_mipe_state->monster_ids, location_id);
    info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0x110c, MIPE_CPP, location_id, 1));
    if (info == 0) {
        srAssertFail("pMonsterInfo", MIPE_CPP, 0x110e, 0);
    }
    g_mipe_state->monster = info->p3D;
    SetWorldCursorGroupId(info->monster_group_id);
}

/* Drag the selected monsters, or the selected trigger, by the world cursor's
   delta from the drag anchor. */
// FUNCTION: WIZ8 0x0057df80
void DragSelectionWithCursor(void)
{
    srVector3T<float> cursor;
    srVector3T<float> position;
    srVector3T<float> moved;
    W8MonsterInfo* info;
    int index;

    GetWorldCursorPosition(&cursor);
    if (g_mipe_state->trigger != 0) {
        g_mipe_state->trigger->GetPosition(&position);
        moved.x = cursor.x - g_mipe_state->drag_anchor.x + position.x;
        moved.y = cursor.y - g_mipe_state->drag_anchor.y + position.y;
        moved.z = cursor.z - g_mipe_state->drag_anchor.z + position.z;
        g_mipe_state->trigger->SetPosition004416F0(&moved);
    } else {
        for (index = 0; index < static_cast<int>(ILLength(&g_mipe_state->monster_ids)); ++index) {
            info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x114c, MIPE_CPP, IListGetAt(&g_mipe_state->monster_ids, index), 1));
            MonsterGetLocalLocation(info->p3D, &position);
            moved.x = cursor.x - g_mipe_state->drag_anchor.x + position.x;
            moved.y = cursor.y - g_mipe_state->drag_anchor.y + position.y;
            moved.z = cursor.z - g_mipe_state->drag_anchor.z + position.z;
            info->p3D->SetPosition004A6DF0(&moved);
        }
    }
    g_mipe_state->drag_anchor = cursor;
}

/* The world-view share of MIPE input: left-down starts a cube drag in mode 4
   or picks another cube in mode 0x13, left-up drops the drag and un-highlights
   the trigger's item, right-up leaves cube selection for the action menu, and
   mouse motion drags or tracks the cube. Returns whether it consumed the
   event. */
// FUNCTION: WIZ8 0x0057e0e0
unsigned char MipeWorldViewEvent(int event, const POINT* point)
{
    unsigned char result;

    result = 0;
    if (g_mipe_state == 0) {
        return 0;
    }
    switch (event & 0xffff) {
    case LEFT_BUTTON_DOWN:
        if (g_mipe_mode == 4) {
            result = 1;
            g_mipe_state->dragging = true;
            ShowWorldCursor();
            GetWorldCursorPosition(&g_mipe_state->drag_anchor);
            WarpSystemCursor(0x140, 0xf0);
            g_mipe_state->trigger = 0;
        } else if (g_mipe_mode == 0x13) {
            if (g_mipe_cube != 0) {
                SetWorldCursorNodeColorComponents(g_mipe_cube, 0.0f, 0.0f, 0.5f);
                RefreshWorldCursorNodeLabel(g_mipe_cube);
            }
            g_mipe_cube = PickWorldCursorNodeAtScreenPoint(point->x, point->y);
            if (g_mipe_cube != 0) {
                SetWorldCursorNodeColorComponents(g_mipe_cube, 0.0f, 1.0f, 0.0f);
                RefreshWorldCursorNodeLabel(g_mipe_cube);
            }
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Select cube:");
            if (g_mipe_cube == 0) {
                ShowNoticef(0xf, L"Click on a cube to select it.");
            } else {
                ShowNoticef(0xf, L"Click on another cube to select it.");
            }
        }
        break;
    case LEFT_BUTTON_UP:
        if (g_mipe_mode == 4) {
            DragSelectionWithCursor();
            result = 1;
            g_mipe_state->dragging = false;
            HideWorldCursor();
            if (g_mipe_state->trigger != 0) {
                Trigger* trigger = g_mipe_state->trigger;
                W8Item* item;

                trigger->flags_0a0 &= ~0x20u;
                item = trigger->rep_item_114;
                if ((trigger->flags_0a0 & W8_TRIGGER_ON) != 0 && item != 0) {
                    static_cast<W8ItemRep*>(item->m_pRep)->SetFlags(0x10, 0);
                    item->SetHighlight(0);
                }
                g_mipe_state->trigger = 0;
            }
        }
        break;
    case RIGHT_BUTTON_UP:
        if (g_mipe_mode == 0x11) {
            g_mipe_mode = 0xf;
            g_mipe_state->selecting = 0;
            HideWorldCursor();
            g_mipe_state->dragging = false;
            g_mipe_menu_active = 1;
            ResetEditorStatusLine(-1);
            ShowNoticef(6, L"Choose an action:");
            ShowNoticef(0xf, L"1) Create cube.");
            ShowNoticef(0xf, L"2) Delete cube.");
            ShowNoticef(0xf, L"3) Edit cube parameters.");
            ShowNoticef(0xf, L"4) Move cube.");
            ShowNoticef(0xf, L"5) Scale cube.");
            ShowNoticef(0xf, L"6) Select cube.");
        }
        break;
    case MOUSE_POS:
        if (g_mipe_state->dragging != 0) {
            if (g_mipe_mode == 4) {
                DragSelectionWithCursor();
                result = 1;
            } else if (g_mipe_mode == 0x11) {
                result = 1;
                if (g_mipe_cube != 0) {
                    srVector3T<float> position;

                    GetWorldCursorAnchor(&position);
                    MoveWorldCursorNode(g_mipe_cube, &position);
                }
            } else if (g_mipe_mode == 0x12) {
                result = 1;
            }
        }
        break;
    }
    return result;
}

/* Monster-generator index that last satisfied
   AnyMonsterGeneratorMarkerWithinReach. */
// GLOBAL: WIZ8 0x0064A1E0
static int g_last_reachable_mongen_marker = -1;

/* Any monster-generator marker item within reach of the camera (radius
   250000), resuming the scan at the last match. */
// FUNCTION: WIZ8 0x0057E3C0
bool AnyMonsterGeneratorMarkerWithinReach(void)
{
    srVector3T<float> camera;
    int count;
    int index;
    MonGen* generator;
    W8Item* marker;

    if (g_world == 0 || g_world->camera == 0) {
        return 0;
    }
    GetCameraPosition(&camera);
    count = GetMonsterGeneratorCount();
    if (0 <= g_last_reachable_mongen_marker && g_last_reachable_mongen_marker < count) {
        generator = GetMonsterGenerator(g_last_reachable_mongen_marker);
        if (generator == 0) {
            marker = 0;
        } else {
            marker = generator->marker_item;
        }
        if (marker != 0 && IsWorldItemWithinReach(marker, &camera, 250000.0f)) {
            return 1;
        }
    }
    for (index = 0; index < count; ++index) {
        generator = GetMonsterGenerator(index);
        if (generator == 0) {
            marker = 0;
        } else {
            marker = generator->marker_item;
        }
        if (marker != 0 && IsWorldItemWithinReach(marker, &camera, 250000.0f)) {
            g_last_reachable_mongen_marker = index;
            return 1;
        }
    }
    return 0;
}
