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
unsigned char g_mipe_mongen_visible_0068f0fe;

// GLOBAL: WIZ8 0x0068f108
int g_mipe_mode_0068f108;

/* How many monsters/items the next Enter in the create modes spawns. */
// GLOBAL: WIZ8 0x0068f10c
int g_mipe_count_0068f10c;

// GLOBAL: WIZ8 0x0068f110
short g_mipe_item_index_0068f110;

// GLOBAL: WIZ8 0x0068f0fc
unsigned char g_debug_monster_cycle_0068f0fc;

// GLOBAL: WIZ8 0x0068f100
W8MipeState* g_mipe_state_0068f100;

// GLOBAL: WIZ8 0x0068f112
short g_mipe_monster_index_0068f112;

// GLOBAL: WIZ8 0x0068f114
unsigned char g_mipe_category_0068f114;

// GLOBAL: WIZ8 0x0068f118
int g_mipe_table_row_0068f118;

// GLOBAL: WIZ8 0x0068f120
int g_mipe_table_base_0068f120;

// GLOBAL: WIZ8 0x0068f124
W8PList* g_mipe_monster_entries_0068f124;

// GLOBAL: WIZ8 0x0068f12c
W8WorldCursorNode* g_mipe_cube_0068f12c;

/* 0x0064A1CC: 'C' toggles between single-monster ("Choosing: One") and
   whole-group ("Choosing: Group") selection in UpdateMipeSelection0057DC20. */
// GLOBAL: WIZ8 0x0064a1cc
static unsigned char g_mipe_choose_group_0064a1cc;

int FindCategoryItemTable0057DBD0(unsigned int category, int ordinal);

/* The MIPE toggle: leaving the panel restores the level flags and combat
   state and tears the editor state down; entering it clears combat flags,
   prints the mode menu and builds the monster entry list. */
// FUNCTION: WIZ8 0x0057d740
void ToggleMipePanel0057D740(void)
{
    if (g_flag_68f105 != 0) {
        g_level_block->flag_271 = 1;
        g_level_block->flag_272 = 0;
        ResetEditorStatusLine0058AA20(-1);
        if (gXStatus.fCombatMode != 0) {
            SelectTextBox(1);
        }
        ReleaseWorldCursor004909C0();
        g_flag_68f105 = 0;
        g_flag_68f104 = 1;
        g_debug_monster_cycle_0068f0fc = 0;
        if (g_mipe_cube_0068f12c != 0) {
            SetWorldCursorNodeColorComponents0048E420(g_mipe_cube_0068f12c, 0.0f, 0.0f, 0.5f);
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
            free(PLGet(g_mipe_monster_entries_0068f124, entry_index));
        }
        PLDestroy(g_mipe_monster_entries_0068f124);
        g_mipe_monster_entries_0068f124 = 0;
        IListFreeData(&g_mipe_state_0068f100->monster_ids);
        PListFreeData(&g_mipe_state_0068f100->waypoints);
        free(g_mipe_state_0068f100);
        g_mipe_state_0068f100 = 0;
        SetWorldCursorNodesVisible0048ED70(g_value_0068f0fd);
        return;
    }

    g_level_block->flag_271 = 0;
    g_level_block->flag_272 = 1;
    SelectTextBox(0);
    ResetEditorStatusLine0058AA20(-1);
    g_flag_68f105 = 1;
    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"What would you like to do?", 0);
    ShowNoticef(15, L"1) Create a monster.", 0);
    ShowNoticef(15, L"2) Create an item.", 0);
    ShowNoticef(15, L"3) Edit object(s).", 0);
    ShowNoticef(15, L"4) Monster Generators.", 0);
    ShowNoticef(15, L"5) Select object(s).", 0);
    ShowNoticef(15, L"6) Handle triggers.", 0);
    g_mipe_mode_0068f108 = 0;
    InitializeWorldCursor00490210();
    g_flag_68f105 = 1;
    g_flag_68f104 = 0;
    g_mipe_cube_0068f12c = 0;
    g_mipe_monster_entries_0068f124 = PLCreate();

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
        entry->selectable = records[index].deleted == 0 && records[index].value_1c1 == -1;
        PLAdoptAppend(g_mipe_monster_entries_0068f124, entry);
    }
    free(records);

    g_mipe_state_0068f100 = static_cast<W8MipeState*>(malloc(sizeof(W8MipeState)));
    memset(g_mipe_state_0068f100, 0, sizeof(W8MipeState));
    IListInit(&g_mipe_state_0068f100->monster_ids);
    /* Retail overwrites IList's capacity with this sentinel-sized value here.
       Keep the observed store even though the allocation itself is still the
       ten-entry IListInit buffer; selected_group_id at +0x0c is a separate
       cache used by UpdateMipeSelection0057DC20. */
    g_mipe_state_0068f100->monster_ids.capacity = W8_MIPE_NO_GROUP;
    g_mipe_state_0068f100->selecting = 0;
    g_mipe_state_0068f100->value_34 = 1.0f;
    g_mipe_state_0068f100->speed_step = 0.020000000f;
    g_mipe_state_0068f100->edit_selection = -1;
    PListInit(&g_mipe_state_0068f100->waypoints);

    for (int cursor_index = 0, count = GetWorldCursorNodeCount0048ED00(); cursor_index < count;
         ++cursor_index) {
        W8WorldCursorNode* node = GetWorldCursorNode0048ED10(cursor_index);
        SetWorldCursorNodeColorComponents0048E420(node, 0.0f, 0.0f, 0.5f);
        RefreshWorldCursorNodeLabel0048DCA0(node);
    }

    int selection = g_mipe_table_row_0068f118 + g_mipe_table_base_0068f120;
    unsigned int monster_index = 0;
    int visible = 0;
    while (monster_index < gXStatus.uiMonstersInDatabase) {
        W8MipeMonsterEntry* entry =
            static_cast<W8MipeMonsterEntry*>(PLGet(g_mipe_monster_entries_0068f124, monster_index));
        if (entry->kind == g_mipe_category_0068f114 && entry->selectable != 0) {
            if (visible == selection) {
                break;
            }
            ++visible;
        }
        ++monster_index;
    }
    if (monster_index == gXStatus.uiMonstersInDatabase) {
        g_mipe_monster_index_0068f112 = 0;
    } else {
        W8MonsterRecord record;
        LoadMonsterDatabaseRecord(monster_index, &record);
        g_mipe_monster_index_0068f112 = record.value_1c1;
    }

    unsigned int item_index = 0;
    visible = 0;
    while (item_index < gXStatus.uiItemsInDatabase) {
        if (g_item_records[item_index].equip_class == g_mipe_category_0068f114 &&
            g_item_records[item_index].unknown_0cb == 0) {
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
    g_mipe_item_index_0068f110 = static_cast<short>(item_index);
}

// GLOBAL: WIZ8 0x0068f11c
W8PList* g_mipe_category_list_0068f11c;

/* 'H' in the item-create mode: spawned items get the hidden flag. */
// GLOBAL: WIZ8 0x0068f128
bool g_mipe_item_hidden_0068f128;

// GLOBAL: WIZ8 0x0068f130
int g_mipe_cube_param_0068f130;

// GLOBAL: WIZ8 0x0068f134
int g_mipe_scale_plane_0068f134;

/* Lock/trap kind names for the selected trigger's value_368, printed by the
   prop-edit menu and the locks & traps editor. */
// GLOBAL: WIZ8 0x0064a1d0
static const wchar_t* g_lock_type_names_0064a1d0[4] = {L"None", L"Pickable Lock", L"Trap",
                                                       L"Key Lock"};

/* Mode-1 status: monster being placed, count being accumulated and the
   creation method. */
// FUNCTION: WIZ8 0x00577bf0
static void ShowMipeMonsterStatus00577BF0(void)
{
    wchar_t name[100];
    const wchar_t* method;

    wcscpy(name,
           static_cast<const wchar_t*>(PLGet(g_mipe_monster_entries_0068f124,
                                             static_cast<int>(g_mipe_monster_index_0068f112))));
    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Type # to specify how many, then ENTER,");
    ShowNoticef(6, L"OR type C to change what monster to place.  ");
    ShowNoticef(6, L"OR type O to edit the creation method.      ");
    ShowNoticef(3, L"How many: %d", g_mipe_count_0068f10c);
    ShowNoticef(0xf, &g_wchar_00689b34);
    if (g_mipe_state_0068f100->unknown_30 == 0) {
        method = L"Creation Method: Exact #";
    } else if (g_mipe_state_0068f100->unknown_30 == 1) {
        method = L"Creation Method: Placeholder";
    } else {
        if (g_mipe_state_0068f100->unknown_30 != 2) {
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
static void ShowMipeItemStatus00577CB0(void)
{
    wchar_t name[100];
    const wchar_t* line;
    unsigned int palette;

    wcscpy(name, g_item_records[g_mipe_item_index_0068f110].display_name);
    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Type # to specify how many, then ENTER,");
    ShowNoticef(6, L"OR type C to change what item to place.  ");
    ShowNoticef(3, L"How many: %d", g_mipe_count_0068f10c);
    ShowNoticef(0xf, &g_wchar_00689b34);
    if (g_byte_0064a1cd == 0) {
        line = L"A - All invisible items  will be blue.";
        palette = 3;
    } else {
        line = L"A - All invisible items will be hidden.";
        palette = 8;
    }
    ShowNoticef(palette, line);
    if (g_mipe_item_hidden_0068f128 == 0) {
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
static void ShowMipeTableRows00577D80(W8PList* list)
{
    void* entry;
    int row;
    unsigned int palette;

    if (list != 0) {
        row = 0;
        do {
            entry = PLGet(list, g_mipe_table_base_0068f120 + row);
            if (entry == 0) {
                ShowNoticef(0xf, &g_wchar_00689b34);
            } else {
                palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
                ShowNoticef(palette, L"    %s", entry);
            }
            ++row;
        } while (row < 6);
    }
}

/* Mode-9 menu: pick a monster or item to edit, showing the one/group choice
   the 'C' key toggles. */
// FUNCTION: WIZ8 0x00577de0
static void ShowMipeChooseMenu00577DE0(void)
{
    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Choose monster or item to edit.");
    ShowNoticef(0xf, L"Type C to change how to choose.");
    ShowNoticef(0xf, &g_wchar_00689b34);
    ShowNoticef(0xf, &g_wchar_00689b34);
    ShowNoticef(0xf, &g_wchar_00689b34);
    ShowNoticef(0xf, &g_wchar_00689b34);
    if (g_mipe_choose_group_0064a1cc != 0) {
        ShowNoticef(0xf, L"Choosing: Group");
        return;
    }
    ShowNoticef(0xf, L"Choosing: One");
}

/* Mode-5 menu: which kind of object to edit. */
// FUNCTION: WIZ8 0x00577e60
static void ShowMipeEditMenu00577E60(void)
{
    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"What do you want to edit?.");
    ShowNoticef(0xf, L"1) Monster");
    ShowNoticef(0xf, L"2) Prop");
    ShowNoticef(0xf, L"3) Item");
    ShowNoticef(0xf, L"4) Move object(s)");
}

/* Mode-0xd menu: prop field-editing choices (locks & traps, treasure table). */
// FUNCTION: WIZ8 0x00577eb0
static void ShowMipePropMenu00577EB0(void)
{
    ResetEditorStatusLine0058AA20(-1);
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
static void ShowMipeTriggerMenu00578000(void)
{
    const wchar_t* status;
    int kind;

    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Select what you want to do.");
    ShowNoticef(0xf, L"1) Create trigger.");
    ShowNoticef(0xf, L"2) Delete trigger.");
    ShowNoticef(0xf, L"3) ");
    ShowNoticef(0xf, L"4) Select trigger.");
    ShowNoticef(0xf, g_value_0068f0fd == 0 ? L"5) Toggle trigger display [now off]."
                                           : L"5) Toggle trigger display [now on].");
    ShowNoticef(0xf, L"6) Volume Triggers.");
    if (g_mipe_state_0068f100->trigger == 0) {
        if (g_mipe_state_0068f100->selecting == 0) {
            return;
        }
        status = L"---> SELECTING TRIGGER <---";
    } else {
        kind = g_mipe_state_0068f100->trigger->trigger_kind_018;
        if (kind == 1) {
            status = L"Trigger is Switch";
            if (g_mipe_state_0068f100->selecting == 0) {
                ShowNoticef(8, status);
                return;
            }
        } else if (kind == 2) {
            status = L"Trigger is Invisible";
            if (g_mipe_state_0068f100->selecting == 0) {
                ShowNoticef(8, status);
                return;
            }
        } else {
            status = L"Trigger Type Unknown";
            if (g_mipe_state_0068f100->selecting == 0) {
                ShowNoticef(0xf, status);
                return;
            }
        }
    }
    ShowNoticef(3, status);
}

/* The status line block the ','/'.' and 'k'/'l' handlers repaint. */
// FUNCTION: WIZ8 0x00577f10
void ShowMonsterSpeedStatus00577F10(void)
{
    W8PathAI* path;
    float speed;

    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Type ',' to decrease speed, '.' to increase.");
    ShowNoticef(6, L"Type 'k' to decrease increment, 'l' to increase.");
    ShowNoticef(0xf, &g_wchar_00689b34);
    ShowNoticef(0xf, L"Increment: %g", g_mipe_state_0068f100->speed_step);
    ShowNoticef(0xf, &g_wchar_00689b34);
    ShowNoticef(0xf, &g_wchar_00689b34);
    if (g_mipe_state_0068f100->monster == 0) {
        ShowNoticef(8, L"No monster available.");
        return;
    }
    path = static_cast<W8PathAI*>(MonsterGetObject0C(g_mipe_state_0068f100->monster));
    if (path != 0 && PathAIRecordFlag004A9740(path) == 0) {
        speed = PathAIGetScale004AAA50(path);
    } else {
        if (g_mipe_state_0068f100->monster == 0) {
            ShowNoticef(8, L"Monster has no path AI.");
            return;
        }
        speed = MonsterGetNavigatorValue120(g_mipe_state_0068f100->monster);
    }
    ShowNoticef(0xf, L"Current speed: %g", speed);
}

/* The "Parameters" pane for the selected volume cube. */
// FUNCTION: WIZ8 0x005780f0
void ShowCubeParameters005780F0(void)
{
    ResetEditorStatusLine0058AA20(-1);
    if (g_mipe_cube_0068f12c == 0) {
        ShowNoticef(6, L"No cube selected.");
        return;
    }
    ShowNoticef(6, L"Parameters:");
    if (g_mipe_cube_param_0068f130 == 0) {
        ShowNoticef(0, L"Message: %d",
                    GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c, 0));
    } else {
        ShowNoticef(0xf, L"Message: %d",
                    GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c, 0));
    }
    if (g_mipe_cube_param_0068f130 == 1) {
        ShowNoticef(0, L"Search: %d", GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c, 1));
    } else {
        ShowNoticef(0xf, L"Search: %d",
                    GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c, 1));
    }
    if (g_mipe_cube_param_0068f130 == 2) {
        ShowNoticef(0, L"Function: %d",
                    GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c, 2));
        return;
    }
    ShowNoticef(0xf, L"Function: %d", GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c, 2));
}

/* The monster generator top menu plus its selection status line. */
// FUNCTION: WIZ8 0x005781f0
void ShowMonsterGeneratorStatus005781F0(void)
{
    const char* state;

    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Monster Generators (%d)", GetMonsterGeneratorCount());
    ShowNoticef(0xf, L"1) Create 2) Delete");
    ShowNoticef(0xf, L"3) Edit   4) Select");
    state = "On";
    if (g_mipe_mongen_visible_0068f0fe == 0) {
        state = "Off";
    }
    ShowNoticef(0xf, L"5) Toggle Display [%s]", state);
    state = "Off";
    if (g_generator_save_flag == 0) {
        state = "On";
    }
    ShowNoticef(0xf, L"6) Toggle Active  [%s]", state);
    ShowNoticef(0xf, &g_wchar_00689b34);
    if (g_mipe_state_0068f100->generator != 0) {
        if (g_mipe_state_0068f100->selecting != 0) {
            ShowNoticef(3, L"<--- MOUSE OVER MONGEN --->");
            return;
        }
        ShowNoticef(8, L"MONGEN selected");
        return;
    }
    if (g_mipe_state_0068f100->selecting != 0) {
        ShowNoticef(3, L"---> SELECTING MONGEN <---");
        return;
    }
    ShowNoticef(0xf, &g_wchar_00689b34);
}

/* The "Edit Monster Generator" pane. */
// FUNCTION: WIZ8 0x005782d0
void ShowMonsterGeneratorEditor005782D0(void)
{
    MonGen* generator;
    W8EncounterTableRuntime* table;
    const char* state;
    int chance;
    short interval;
    int color;
    const wchar_t* format;

    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Edit Monster Generator");
    ShowNoticef(0xf, L"1) Name: %hs", g_mipe_state_0068f100->generator->name);
    if (g_mipe_state_0068f100->generator->encounter_table_index == -1) {
        ShowNoticef(0xf, L"2) Table: Not Selected");
    } else {
        table = GetEncounterTable(g_mipe_state_0068f100->generator->encounter_table_index);
        ShowNoticef(0xf, L"2) Table: %hs", table->name);
    }
    state = "On";
    if (g_mipe_state_0068f100->generator->generation_enabled == 0) {
        state = "Off";
    }
    ShowNoticef(0xf, L"3) Toggle Active [%s]", state);
    generator = g_mipe_state_0068f100->generator;
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
static void ShowMipeEncounterCategory005783C0(void)
{
    W8EncounterTableRuntime* entry;
    int row;
    unsigned int palette;

    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Category: %S", *g_encounter_names.GetAt(g_mipe_category_0068f114 & 0xff));
    if (g_mipe_category_list_0068f11c != 0) {
        row = 0;
        do {
            entry = static_cast<W8EncounterTableRuntime*>(
                PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + row));
            if (entry == 0) {
                ShowNoticef(0xf, &g_wchar_00689b34);
            } else {
                palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
                ShowNoticef(palette, L"    %S", entry->name);
            }
            ++row;
        } while (row < 6);
    }
}

/* Mode-0x1d menu: current item-table category plus its six rows. */
// FUNCTION: WIZ8 0x00578470
static void ShowMipeItemTableCategory00578470(void)
{
    W8ItemTableRecord* entry;
    int row;
    unsigned int palette;

    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Category: %S", g_item_table_category_names[g_mipe_category_0068f114 & 0xff]);
    if (g_mipe_category_list_0068f11c != 0) {
        row = 0;
        do {
            entry = static_cast<W8ItemTableRecord*>(
                PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + row));
            if (entry == 0) {
                ShowNoticef(0xf, &g_wchar_00689b34);
            } else {
                palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
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
static unsigned char HandleMipeMonsterCreateKey00578500(unsigned short key)
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

    if (g_mipe_category_list_0068f11c == 0) {
        list = PLCreate();
        g_mipe_category_list_0068f11c = list;
        if (list != 0) {
            PListClear(list);
            index = 0;
            if (0 < static_cast<int>(gXStatus.uiMonstersInDatabase)) {
                do {
                    entry = static_cast<W8MipeMonsterEntry*>(
                        PLGet(g_mipe_monster_entries_0068f124, index));
                    if (entry->kind == g_mipe_category_0068f114 && entry->selectable != 0) {
                        PLAdoptAppend(list, entry);
                    }
                    ++index;
                } while (index < static_cast<int>(gXStatus.uiMonstersInDatabase));
            }
        }
    }
    list = g_mipe_category_list_0068f11c;
    switch (key) {
    case 8:
        g_mipe_count_0068f10c = g_mipe_count_0068f10c / 10;
        ShowMipeMonsterStatus00577BF0();
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
        g_mipe_count_0068f10c = static_cast<int>(static_cast<char>(static_cast<char>(key) - 0x30)) +
                                g_mipe_count_0068f10c * 10;
        /* fall through */
    case 0x20:
        ShowMipeMonsterStatus00577BF0();
        return 1;
    case 0x43:
        g_mipe_count_0068f10c = 1;
        g_mipe_mode_0068f108 = 6;
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(
            6, L"Category: %s",
            gppStringList[g_special_category_name_ids_61ea78[g_mipe_category_0068f114 & 0xff]]);
        ShowMipeTableRows00577D80(list);
        return 1;
    case 0x4f:
        g_mipe_state_0068f100->unknown_30 = g_mipe_state_0068f100->unknown_30 + 1;
        if (2 < g_mipe_state_0068f100->unknown_30) {
            g_mipe_state_0068f100->unknown_30 = 0;
        }
        ShowMipeMonsterStatus00577BF0();
        return 1;
    }
    if (g_mipe_count_0068f10c != 0) {
        visible = g_mipe_table_row_0068f118 + g_mipe_table_base_0068f120;
        index = 0;
        monster_index = 0;
        if (0 < static_cast<int>(gXStatus.uiMonstersInDatabase)) {
            do {
                entry = static_cast<W8MipeMonsterEntry*>(
                    PLGet(g_mipe_monster_entries_0068f124, monster_index));
                if (entry->kind == g_mipe_category_0068f114 && entry->selectable != 0) {
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
        GetWorldCursorAnchor00490C20(&anchor);
        monster_group = CreateGroup(
            static_cast<unsigned int>(static_cast<unsigned short>(record.record_id_187)),
            static_cast<unsigned int>(g_mipe_count_0068f10c), &anchor, '\x01', '\x01', '\x01');
        if (monster_group == 0) {
            g_mipe_count_0068f10c = reinterpret_cast<int>(
                monster_group); /* reinterpret-ok: retail stores the null group pointer as the count sentinel */
            return 1;
        }
        formation.x = anchor.x;
        formation.y = anchor.y;
        formation.z = anchor.z;
        SetMonsterGroupFormation(monster_group, &formation);
        monster_group->unknown_2d[0x6d] = g_mipe_state_0068f100->unknown_30;
    }
    return 1;
}

/* Mode-7 key handler: the item-category picker. Enter resolves the highlighted
   record back to a database index and drops into item-create mode; the arrows
   page and walk the six-row view and cycle categories. */
// FUNCTION: WIZ8 0x00578850
static void HandleMipeItemCategoryKey00578850(unsigned short key)
{
    W8PList* list;
    W8ItemDatabaseRecord* entry;
    int found;
    unsigned int item_index;

    list = g_mipe_category_list_0068f11c;
    switch (key) {
    case 0xd:
        PLGet(g_mipe_category_list_0068f11c,
              g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118);
        found = 0;
        item_index = 0;
        if (0 < static_cast<int>(gXStatus.uiItemsInDatabase)) {
            do {
                if (g_item_records[item_index].equip_class == g_mipe_category_0068f114 &&
                    g_item_records[item_index].unknown_0cb == 0) {
                    if (found == g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118) {
                        break;
                    }
                    ++found;
                }
                ++item_index;
            } while (static_cast<int>(item_index) < static_cast<int>(gXStatus.uiItemsInDatabase));
        }
        if (item_index == gXStatus.uiItemsInDatabase) {
            item_index = 0;
        }
        g_mipe_item_index_0068f110 = static_cast<short>(item_index);
        ShowMipeItemStatus00577CB0();
        g_mipe_mode_0068f108 = 2;
        return;
    default:
        return;
    case 0x20:
        break;
    case 0x21:
        if (g_mipe_table_base_0068f120 == 0) {
            return;
        }
        g_mipe_table_base_0068f120 = g_mipe_table_base_0068f120 - 6;
        if (g_mipe_table_base_0068f120 < 0) {
            g_mipe_table_base_0068f120 = 0;
        }
        break;
    case 0x22:
        if (static_cast<int>(PLLength(g_mipe_category_list_0068f11c) - 6) <=
            g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118) {
            if (static_cast<int>(PLLength(g_mipe_category_list_0068f11c)) <=
                g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118) {
                return;
            }
            g_mipe_table_base_0068f120 = g_mipe_table_base_0068f120 + 6;
            g_mipe_table_row_0068f118 = 0;
            if (static_cast<int>(PLLength(g_mipe_category_list_0068f11c)) <=
                g_mipe_table_base_0068f120) {
                g_mipe_table_base_0068f120 =
                    static_cast<int>(PLLength(g_mipe_category_list_0068f11c));
                --g_mipe_table_base_0068f120;
            }
        } else {
            g_mipe_table_base_0068f120 = g_mipe_table_base_0068f120 + 6;
        }
        break;
    case 0x25:
        if (g_mipe_category_0068f114 == 0) {
            return;
        }
        g_mipe_category_0068f114 = g_mipe_category_0068f114 - 1;
        g_mipe_table_row_0068f118 = 0;
        g_mipe_table_base_0068f120 = 0;
        if (g_mipe_category_list_0068f11c != 0) {
            PListClear(g_mipe_category_list_0068f11c);
            found = 0;
            if (0 < static_cast<int>(gXStatus.uiItemsInDatabase)) {
                do {
                    entry = &g_item_records[found];
                    if (entry->equip_class == g_mipe_category_0068f114 && entry->unknown_0cb == 0) {
                        PLAdoptAppend(list, entry);
                    }
                    ++found;
                } while (found < static_cast<int>(gXStatus.uiItemsInDatabase));
            }
        }
        break;
    case 0x26:
        if (g_mipe_table_row_0068f118 == 0) {
            if (g_mipe_table_base_0068f120 != 0) {
                --g_mipe_table_base_0068f120;
            }
        } else {
            --g_mipe_table_row_0068f118;
        }
        break;
    case 0x27:
        if (0x18 < g_mipe_category_0068f114) {
            return;
        }
        g_mipe_category_0068f114 = g_mipe_category_0068f114 + 1;
        g_mipe_table_row_0068f118 = 0;
        g_mipe_table_base_0068f120 = 0;
        if (g_mipe_category_list_0068f11c != 0) {
            PListClear(g_mipe_category_list_0068f11c);
            found = 0;
            if (0 < static_cast<int>(gXStatus.uiItemsInDatabase)) {
                do {
                    entry = &g_item_records[found];
                    if (entry->equip_class == g_mipe_category_0068f114 && entry->unknown_0cb == 0) {
                        PLAdoptAppend(list, entry);
                    }
                    ++found;
                } while (found < static_cast<int>(gXStatus.uiItemsInDatabase));
            }
        }
        break;
    case 0x28:
        if (g_mipe_table_row_0068f118 < 5 &&
            g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118 <
                static_cast<int>(PLLength(g_mipe_category_list_0068f11c) - 1)) {
            ++g_mipe_table_row_0068f118;
            break;
        }
        if (static_cast<int>(PLLength(g_mipe_category_list_0068f11c) - 1) <=
            g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118) {
            return;
        }
        ++g_mipe_table_base_0068f120;
    }
    list = g_mipe_category_list_0068f11c;
    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Category: %s",
                gppStringList[g_equip_class_name_ids_61e7dc[g_mipe_category_0068f114 & 0xff]]);
    ShowMipeTableRows00577D80(list);
}

/* Mode-2 key handler: digits accumulate the count, Enter spawns that many of
   the selected item at the world cursor, 'A' flips the invisible-item display
   and activates/deactivates the flagged items, 'C' opens the item category
   picker and 'H' toggles the spawned item's hidden flag. */
// FUNCTION: WIZ8 0x00578d00
static unsigned char HandleMipeItemCreateKey00578D00(unsigned short key)
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

    if (g_mipe_category_list_0068f11c == 0) {
        list = PLCreate();
        g_mipe_category_list_0068f11c = list;
        if (list != 0) {
            PListClear(list);
            index = 0;
            if (0 < static_cast<int>(gXStatus.uiItemsInDatabase)) {
                do {
                    if (g_item_records[index].equip_class == g_mipe_category_0068f114 &&
                        g_item_records[index].unknown_0cb == 0) {
                        PLAdoptAppend(list, &g_item_records[index]);
                    }
                    ++index;
                } while (index < static_cast<int>(gXStatus.uiItemsInDatabase));
            }
        }
    }
    list = g_mipe_category_list_0068f11c;
    switch (key) {
    case 8:
        g_mipe_count_0068f10c = g_mipe_count_0068f10c / 10;
        ShowMipeItemStatus00577CB0();
        break;
    default:
        return 0;
    case 0xd:
        found = 0;
        item_index = 0;
        if (0 < static_cast<int>(gXStatus.uiItemsInDatabase)) {
            do {
                if (g_item_records[item_index].equip_class == g_mipe_category_0068f114 &&
                    g_item_records[item_index].unknown_0cb == 0) {
                    if (found == g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118) {
                        break;
                    }
                    ++found;
                }
                ++item_index;
            } while (static_cast<int>(item_index) < static_cast<int>(gXStatus.uiItemsInDatabase));
        }
        if (item_index == gXStatus.uiItemsInDatabase) {
            item_index = 0;
        }
        GetWorldCursorAnchor00490C20(&anchor);
        spawned_index = 0;
        if (g_mipe_count_0068f10c != 0) {
            do {
                int flags = 3;
                if (g_mipe_item_hidden_0068f128 != 0) {
                    flags = 0x83;
                }
                spawned = SpawnItem(item_index & 0xffff, &anchor, flags, '\x01');
                if (g_mipe_item_hidden_0068f128 != 0) {
                    SetItemFlags(spawned, 1, '\x01');
                    RegisterSearchableWorldItem00516E20(spawned);
                }
                ++spawned_index;
            } while (spawned_index < static_cast<unsigned int>(g_mipe_count_0068f10c));
        }
        return 1;
    case 0x20:
        ShowMipeItemStatus00577CB0();
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
        g_mipe_count_0068f10c = static_cast<int>(static_cast<char>(static_cast<char>(key) - 0x30)) +
                                g_mipe_count_0068f10c * 10;
        ShowMipeItemStatus00577CB0();
        break;
    case 0x41:
        show_invisible = g_byte_0064a1cd == 0;
        g_byte_0064a1cd = show_invisible;
        item_index = 0;
        if (PLLength(gXStatus.plsItemList) != 0) {
            do {
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
                ++item_index;
            } while (item_index < PLLength(gXStatus.plsItemList));
        }
        ShowMipeItemStatus00577CB0();
        return 1;
    case 0x43:
        g_mipe_count_0068f10c = 0;
        g_mipe_mode_0068f108 = 7;
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Category: %s",
                    gppStringList[g_equip_class_name_ids_61e7dc[g_mipe_category_0068f114 & 0xff]]);
        ShowMipeTableRows00577D80(list);
        return 1;
    case 0x48:
        g_mipe_item_hidden_0068f128 = g_mipe_item_hidden_0068f128 == 0;
        ShowMipeItemStatus00577CB0();
        break;
    }
    return 1;
}

/* Mode-6 key handler: the monster-category picker. Enter resolves the
   highlighted entry to a monster record, seeds the spawn count from the
   creation method and drops back to mode 1; the arrows page/walk rows and
   cycle through non-empty categories. */
// FUNCTION: WIZ8 0x00579300
static void HandleMipeMonsterCategoryKey00579300(unsigned short key)
{
    W8PList* list;
    W8MipeMonsterEntry* entry;
    int index;
    int found;
    int wraps;
    unsigned int monster_index;
    W8MonsterRecord record;
    W8MonsterRecord selected;

    list = g_mipe_category_list_0068f11c;
    switch (key) {
    case 0xd:
        PLGet(g_mipe_category_list_0068f11c,
              g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118);
        index = g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118;
        found = 0;
        monster_index = 0;
        if (0 < static_cast<int>(gXStatus.uiMonstersInDatabase)) {
            do {
                entry = static_cast<W8MipeMonsterEntry*>(
                    PLGet(g_mipe_monster_entries_0068f124, monster_index));
                if (entry->kind == g_mipe_category_0068f114 && entry->selectable != 0) {
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
        g_mipe_monster_index_0068f112 = record.record_id_187;
        if (g_mipe_state_0068f100->unknown_30 != 1) {
            if (g_mipe_state_0068f100->unknown_30 == 2) {
                LoadMonsterDatabaseRecord(static_cast<int>(record.record_id_187), &selected);
                g_mipe_count_0068f10c =
                    selected.group_size_dice_0c1.sides * selected.group_size_dice_0c1.count +
                    static_cast<int>(selected.group_size_dice_0c1.base);
            }
            ShowMipeMonsterStatus00577BF0();
            g_mipe_mode_0068f108 = 1;
            return;
        }
        g_mipe_count_0068f10c = 1;
        ShowMipeMonsterStatus00577BF0();
        g_mipe_mode_0068f108 = 1;
        return;
    default:
        return;
    case 0x20:
        break;
    case 0x21:
        if (g_mipe_table_base_0068f120 == 0) {
            return;
        }
        g_mipe_table_base_0068f120 = g_mipe_table_base_0068f120 - 6;
        if (g_mipe_table_base_0068f120 < 0) {
            g_mipe_table_base_0068f120 = 0;
        }
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(
            6, L"Category: %s",
            gppStringList[g_special_category_name_ids_61ea78[g_mipe_category_0068f114 & 0xff]]);
        ShowMipeTableRows00577D80(list);
        return;
    case 0x22:
        if (g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118 <
            static_cast<int>(PLLength(g_mipe_category_list_0068f11c) - 6)) {
            g_mipe_table_base_0068f120 = g_mipe_table_base_0068f120 + 6;
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(
                6, L"Category: %s",
                gppStringList[g_special_category_name_ids_61ea78[g_mipe_category_0068f114 & 0xff]]);
            ShowMipeTableRows00577D80(list);
            return;
        }
        if (static_cast<int>(PLLength(g_mipe_category_list_0068f11c)) <=
            g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118) {
            return;
        }
        g_mipe_table_base_0068f120 = g_mipe_table_base_0068f120 + 6;
        g_mipe_table_row_0068f118 = 0;
        if (static_cast<int>(PLLength(g_mipe_category_list_0068f11c)) <=
            g_mipe_table_base_0068f120) {
            g_mipe_table_base_0068f120 = static_cast<int>(PLLength(g_mipe_category_list_0068f11c));
            --g_mipe_table_base_0068f120;
        }
        break;
    case 0x25:
        wraps = 0;
        g_mipe_table_row_0068f118 = 0;
        g_mipe_table_base_0068f120 = 0;
        do {
            list = g_mipe_category_list_0068f11c;
            if (g_mipe_category_0068f114 == 0) {
                g_mipe_category_0068f114 = 0x20;
                ++wraps;
            } else {
                g_mipe_category_0068f114 = g_mipe_category_0068f114 - 1;
            }
            if (g_mipe_category_list_0068f11c != 0) {
                PListClear(g_mipe_category_list_0068f11c);
                index = 0;
                if (0 < static_cast<int>(gXStatus.uiMonstersInDatabase)) {
                    do {
                        entry = static_cast<W8MipeMonsterEntry*>(
                            PLGet(g_mipe_monster_entries_0068f124, index));
                        if (entry->kind == g_mipe_category_0068f114 && entry->selectable != 0) {
                            PLAdoptAppend(list, entry);
                        }
                        ++index;
                    } while (index < static_cast<int>(gXStatus.uiMonstersInDatabase));
                }
            }
        } while (PLLength(g_mipe_category_list_0068f11c) == 0 && wraps < 2);
        break;
    case 0x26:
        if (g_mipe_table_row_0068f118 == 0) {
            if (g_mipe_table_base_0068f120 != 0) {
                --g_mipe_table_base_0068f120;
            }
        } else {
            --g_mipe_table_row_0068f118;
        }
        break;
    case 0x27:
        wraps = 0;
        g_mipe_table_row_0068f118 = 0;
        g_mipe_table_base_0068f120 = 0;
        do {
            list = g_mipe_category_list_0068f11c;
            if (g_mipe_category_0068f114 < 0x20) {
                g_mipe_category_0068f114 = g_mipe_category_0068f114 + 1;
            } else {
                g_mipe_category_0068f114 = 0;
                ++wraps;
            }
            if (g_mipe_category_list_0068f11c != 0) {
                PListClear(g_mipe_category_list_0068f11c);
                index = 0;
                if (0 < static_cast<int>(gXStatus.uiMonstersInDatabase)) {
                    do {
                        entry = static_cast<W8MipeMonsterEntry*>(
                            PLGet(g_mipe_monster_entries_0068f124, index));
                        if (entry->kind == g_mipe_category_0068f114 && entry->selectable != 0) {
                            PLAdoptAppend(list, entry);
                        }
                        ++index;
                    } while (index < static_cast<int>(gXStatus.uiMonstersInDatabase));
                }
            }
        } while (PLLength(g_mipe_category_list_0068f11c) == 0 && wraps < 2);
        break;
    case 0x28:
        if (g_mipe_table_row_0068f118 < 5 &&
            g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118 <
                static_cast<int>(PLLength(g_mipe_category_list_0068f11c) - 1)) {
            ++g_mipe_table_row_0068f118;
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(
                6, L"Category: %s",
                gppStringList[g_special_category_name_ids_61ea78[g_mipe_category_0068f114 & 0xff]]);
            ShowMipeTableRows00577D80(list);
            return;
        }
        if (static_cast<int>(PLLength(g_mipe_category_list_0068f11c) - 1) <=
            g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118) {
            return;
        }
        ++g_mipe_table_base_0068f120;
    }
    list = g_mipe_category_list_0068f11c;
    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Category: %s",
                gppStringList[g_special_category_name_ids_61ea78[g_mipe_category_0068f114 & 0xff]]);
    ShowMipeTableRows00577D80(list);
}

/* '1' halves the selected monster's animation scale, '2' doubles it, '3'
   arms speed editing, '4' sends it to the world cursor, '5' advances it one
   subcycle. */
// FUNCTION: WIZ8 0x00579900
void HandleMonsterDebugKey00579900(unsigned short key)
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
        monster = GetMonsterByLocationID(IListGetAt(&g_mipe_state_0068f100->monster_ids, 0));
        if (monster != 0) {
            scale = monster->GetCurrentAnimationScale();
            monster->SetCurrentAnimationScale(scale * g_float_005ebc7c);
            return;
        }
        break;
    case 0x32:
        monster = GetMonsterByLocationID(IListGetAt(&g_mipe_state_0068f100->monster_ids, 0));
        if (monster != 0) {
            scale = monster->GetCurrentAnimationScale();
            monster->SetCurrentAnimationScale(scale + scale);
            return;
        }
        break;
    case 0x33:
        if (ILLength(&g_mipe_state_0068f100->monster_ids) == 1) {
            g_mipe_mode_0068f108 = 0xb;
            ShowMonsterSpeedStatus00577F10();
            return;
        }
        break;
    case 0x34:
        if (ILLength(&g_mipe_state_0068f100->monster_ids) == 1) {
            info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x6e3, MIPE_CPP, IListGetAt(&g_mipe_state_0068f100->monster_ids, 0), 1));
            if (info == 0 || info->p3D == 0) {
                srAssertFail("pMonsterInfo && pMonsterInfo->p3D", MIPE_CPP, 0x6e5, 0);
            }
            monster = info->p3D;
            GetWorldCursorPosition00490BF0(&position);
            MonsterGetLocation(monster, &location);
            MonsterForward453690(monster, &position);
            MonsterSetAnimating(monster, 1);
            MonsterSetCycle(monster, 4);
            MonsterSetNavigatorObjectFlag38(monster, 1);
            g_mipe_state_0068f100->monster = monster;
            return;
        }
        break;
    case 0x35:
        if (ILLength(&g_mipe_state_0068f100->monster_ids) == 1) {
            info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x703, MIPE_CPP, IListGetAt(&g_mipe_state_0068f100->monster_ids, 0), 1));
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
                            monster->SetRuntimeValueA6(0);
                            monster->flags_1dc = monster->flags_1dc | 0x10;
                            goto cycle_done;
                        }
                    scan_wrap:
                        ++cycle;
                    } while (cycle < 0x1b);
                    cycle = 0;
                } while (true);
            }
            monster->SetRuntimeValueA6(cycle + 1);
        cycle_done:
            monster->SetSubCycle(0);
            monster->m_pRep->pending_subcycle_066 = 0;
            world = GetWorld();
            WorldGetCameraLocation(world, &location);
            position = location;
            monster->SelectLOD004A7BE0(&position);
            monster->ReplacePath004A8400(0);
        }
    }
}

/* The ',', '.', 'k' and 'l' keys while speed editing the debug monster. */
// FUNCTION: WIZ8 0x00579bf0
void AdjustMonsterSpeed00579BF0(unsigned short key)
{
    W8PathAI* path;
    float speed;
    float factor;

    factor = 2.0f;
    if (g_mipe_state_0068f100->monster == 0) {
        return;
    }
    path = static_cast<W8PathAI*>(MonsterGetObject0C(g_mipe_state_0068f100->monster));
    if (path == 0) {
        factor = 1.0f;
        speed = MonsterGetNavigatorValue120(g_mipe_state_0068f100->monster);
    } else {
        if (PathAIRecordFlag004A9740(path) != 0) {
            return;
        }
        speed = PathAIGetScale004AAA50(path);
        speed = speed + speed;
    }
    switch (key) {
    case 0x4b:
        g_mipe_state_0068f100->speed_step =
            g_mipe_state_0068f100->speed_step - g_camera_transition_epsilon_005ebc84;
        if (g_mipe_state_0068f100->speed_step < static_cast<float>(g_double_005ec8d0)) {
            g_mipe_state_0068f100->speed_step = 0.001f;
        }
        break;
    case 0x4c:
        g_mipe_state_0068f100->speed_step =
            g_mipe_state_0068f100->speed_step + g_camera_transition_epsilon_005ebc84;
        break;
    default:
        goto speed_done;
    case 0xbc:
        speed = speed - factor * g_mipe_state_0068f100->speed_step;
        if (path != 0) {
            PathAISetScale004AA9C0(path, speed);
        }
        MonsterSetNavigatorValue120(g_mipe_state_0068f100->monster, speed);
        break;
    case 0xbe:
        speed = factor * g_mipe_state_0068f100->speed_step + speed;
        if (path != 0) {
            PathAISetScale004AA9C0(path, speed);
        }
        MonsterSetNavigatorValue120(g_mipe_state_0068f100->monster, speed);
        break;
    }
    ShowMonsterSpeedStatus00577F10();
speed_done:
    if (path != 0 && PathAIGetScale004AAA50(path) < g_float_005ebc90) {
        PathAISetScale004AA9C0(path, 0.0001f);
    }
}

/* 'C' drops a waypoint monster at the cursor, 'X' removes the newest one. */
// FUNCTION: WIZ8 0x00579df0
void HandleWaypointKey00579DF0(unsigned short key)
{
    W8Monster* monster;
    W8World* world;
    srVector3T<float> position;
    W8GrCycleLoadContext context;

    if (key == 0x43) {
        monster = 0;
        context.directory_08 = "Data\\Monsters";
        context.world_00 = GetWorld();
        LoadMonsterCycle004C5910(&context, "waypoint", &monster, -1, 1);
        GetWorldCursorPosition00490BF0(&position);
        monster->SetPosition004A6DF0(&position);
        world = GetWorld();
        AddMonsterToWorld0046E580(world, monster);
        world = GetWorld();
        UpdateCycleRepresentation004C59B0(monster, world);
        g_mipe_state_0068f100->waypoint_count = g_mipe_state_0068f100->waypoint_count + 1;
        PLAdoptAppend(&g_mipe_state_0068f100->waypoints, monster);
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Type 'C' to create a waypoint.");
        ShowNoticef(0xf, &g_wchar_00689b34);
        ShowNoticef(3, L"Laying down waypoint %d", g_mipe_state_0068f100->waypoint_count);
        ShowNoticef(0xf, &g_wchar_00689b34);
        ShowNoticef(0xf, &g_wchar_00689b34);
        ShowNoticef(0xf, &g_wchar_00689b34);
        ShowNoticef(0xf, L"Type X to delete last waypoint.");
    } else if (key == 0x58) {
        if (g_mipe_state_0068f100 != 0) {
            unsigned int count = PLLength(&g_mipe_state_0068f100->waypoints);
            if (count != 0) {
                monster =
                    static_cast<W8Monster*>(PLGet(&g_mipe_state_0068f100->waypoints, count - 1));
                if (monster != 0) {
                    PLRemoveAt(&g_mipe_state_0068f100->waypoints, count - 1);
                    world = GetWorld();
                    RemoveMonsterFromWorldList(world, monster);
                    world = GetWorld();
                    DetachMonsterRepresentation(monster, world);
                    g_mipe_state_0068f100->waypoint_count =
                        g_mipe_state_0068f100->waypoint_count - 1;
                }
            }
        }
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Type 'C' to create a waypoint.");
        ShowNoticef(0xf, &g_wchar_00689b34);
        ShowNoticef(3, L"Laying down waypoint %d", g_mipe_state_0068f100->waypoint_count);
        ShowNoticef(0xf, &g_wchar_00689b34);
        ShowNoticef(0xf, &g_wchar_00689b34);
        ShowNoticef(0xf, &g_wchar_00689b34);
        ShowNoticef(0xf, L"Type X to delete last waypoint.");
    }
}

/* Mode-0xd key handler: scans the world's props for the picked one with a
   trigger, stores it as the edit target and opens the locks & traps ('1') or
   treasure-table ('2') editors. */
// FUNCTION: WIZ8 0x00579ff0
static void HandleMipePropEditKey00579FF0(unsigned short key)
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

    g_mipe_count_0068f10c = 0;
    world = GetWorld();
    count = WorldGetPropCount(world);
    index = 0;
    if (0 < count) {
        while (true) {
            world = GetWorld();
            prop = WorldGetPropAt(world, index);
            trigger = prop->GetValue18();
            if (trigger != 0 && prop->IsPickedProp0044D680(g_world)) {
                break;
            }
            ++index;
            if (count <= index) {
                return;
            }
        }
        if (prop != 0) {
            g_mipe_state_0068f100->prop = prop;
            if (key == 0x31) {
                g_mipe_mode_0068f108 = 0x1b;
                trigger = g_mipe_state_0068f100->prop->GetValue18();
                ResetEditorStatusLine0058AA20(-1);
                ShowNoticef(6, L"Edit Locks & Traps");
                ShowNoticef(0xf, L"1) Type: %s", g_lock_type_names_0064a1d0[trigger->value_368]);
                index = trigger->value_380;
                if (index < 0) {
                    key_name = &g_wchar_00689b34;
                } else {
                    key_name = g_item_records[index].display_name;
                }
                ShowNoticef(0xf, L"2) Key Id: (%d) %s", index, key_name);
                ShowNoticef(0xf, L" Difficulty (3+/4-): %d", trigger->value_36c);
            } else if (key == 0x32) {
                trigger = prop->GetValue18();
                table_index = FindItemTableByName(trigger->inline_action_data_24c);
                if (g_mipe_category_list_0068f11c == 0) {
                    g_mipe_category_list_0068f11c = PLCreate();
                }
                list = g_mipe_category_list_0068f11c;
                if (table_index < 0) {
                    g_mipe_category_0068f114 = 0;
                    g_mipe_table_base_0068f120 = 0;
                    g_mipe_table_row_0068f118 = 0;
                    if (g_mipe_category_list_0068f11c != 0) {
                        PListClear(g_mipe_category_list_0068f11c);
                        index = 0;
                        if (0 < static_cast<int>(gXStatus.uiItemTablesInDatabase)) {
                            do {
                                table = g_item_tables[index];
                                if (table->category_id == 0) {
                                    PLAdoptAppend(list, table);
                                }
                                ++index;
                            } while (index < static_cast<int>(gXStatus.uiItemTablesInDatabase));
                        }
                    }
                } else {
                    unsigned char category = g_item_tables[table_index]->category_id;
                    g_mipe_category_0068f114 = category;
                    if (g_mipe_category_list_0068f11c != 0) {
                        PListClear(g_mipe_category_list_0068f11c);
                        index = 0;
                        if (0 < static_cast<int>(gXStatus.uiItemTablesInDatabase)) {
                            do {
                                table = g_item_tables[index];
                                if (table->category_id == static_cast<unsigned int>(category)) {
                                    PLAdoptAppend(list, table);
                                }
                                ++index;
                            } while (index < static_cast<int>(gXStatus.uiItemTablesInDatabase));
                        }
                    }
                    index = 0;
                    found = static_cast<int>(PLLength(g_mipe_category_list_0068f11c));
                    if (0 < found) {
                        do {
                            table = static_cast<W8ItemTableRecord*>(
                                PLGet(g_mipe_category_list_0068f11c, index));
                            if (table == g_item_tables[table_index]) {
                                break;
                            }
                            ++index;
                            found = static_cast<int>(PLLength(g_mipe_category_list_0068f11c));
                        } while (index < found);
                    }
                    g_mipe_table_base_0068f120 = (index / 6) * 6;
                    g_mipe_table_row_0068f118 = index % 6;
                }
                g_flag_68f104 = 0;
                g_mipe_mode_0068f108 = 0x1d;
                ResetEditorStatusLine0058AA20(-1);
                ShowNoticef(6, L"Category: %S",
                            g_item_table_category_names[g_mipe_category_0068f114 & 0xff]);
                if (g_mipe_category_list_0068f11c != 0) {
                    row = 0;
                    do {
                        table = static_cast<W8ItemTableRecord*>(
                            PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + row));
                        if (table == 0) {
                            ShowNoticef(0xf, &g_wchar_00689b34);
                        } else {
                            palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
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
int HandleCubeMenuKey0057A310(unsigned int key)
{
    const wchar_t* prompt;
    srVector3T<float> position;
    srVector3T<float> anchor;
    char* name;
    int serial;
    char scale_planes[4];

    switch (key & 0xffff) {
    case 0x20:
        ResetEditorStatusLine0058AA20(-1);
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
        g_mipe_cube_0068f12c = CreateWorldCursorCube0048D080();
        GetCameraForwardPoint00421100(2500.0f, &position);
        position.y = SettlePositionToGround00420C30(&position, 0);
        MoveWorldCursorNode0048DBF0(g_mipe_cube_0068f12c, &position);
        AttachWorldCursorNode0048ED30(g_mipe_cube_0068f12c, 1);
        SetWorldCursorNodeColorComponents0048E420(g_mipe_cube_0068f12c, 0.0f, 1.0f, 0.0f);
        serial = gXStatus.mipe_cube_serial;
        gXStatus.mipe_cube_serial = gXStatus.mipe_cube_serial + 1;
        name = FormatString("Cube%3.3", serial);
        SetWorldCursorNodeName0048F110(g_mipe_cube_0068f12c, name);
        return 1;
    case 0x32:
        if (g_mipe_cube_0068f12c != 0) {
            DestroyWorldCursorCube0048DA80(g_mipe_cube_0068f12c);
            g_mipe_cube_0068f12c = 0;
        }
        return 1;
    case 0x33:
        if (g_mipe_cube_0068f12c != 0) {
            g_mipe_mode_0068f108 = 0x10;
            g_flag_68f104 = 0;
            ShowCubeParameters005780F0();
        }
        return 1;
    case 0x34:
        if (g_mipe_cube_0068f12c == 0) {
            return 1;
        }
        g_mipe_mode_0068f108 = 0x11;
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Move Volume Trigger.");
        if (g_mipe_cube_0068f12c == 0) {
            ShowNoticef(0xf, L"Click on trigger to move.");
        } else {
            ShowNoticef(0xf, L"Move trigger. Hold down SHIFT to");
            ShowNoticef(0xf, L"change elevation.");
        }
        g_flag_68f104 = 0;
        ShowWorldCursor00490B10();
        WarpSystemCursor(0x140, 0xf0);
        GetWorldCursorAnchor00490C20(&anchor);
        MoveWorldCursorNode0048DBF0(g_mipe_cube_0068f12c, &anchor);
        g_mipe_state_0068f100->dragging = 1;
        g_flag_68f104 = 0;
        return 1;
    case 0x35:
        if (g_mipe_cube_0068f12c == 0) {
            return 1;
        }
        g_mipe_mode_0068f108 = 0x12;
        scale_planes[0] = 'X';
        scale_planes[1] = 'Y';
        scale_planes[2] = 'Z';
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Scale Volume Trigger.");
        if (g_mipe_cube_0068f12c != 0) {
            ShowNoticef(0xf, L"Scaling in the %c plane. ",
                        static_cast<int>(scale_planes[g_mipe_scale_plane_0068f134]));
            ShowNoticef(0xf, L"Press X/Y/Z to change plane.");
            g_flag_68f104 = 0;
            return 1;
        }
        prompt = L"Click on trigger to scale.";
        break;
    case 0x36:
        g_mipe_mode_0068f108 = 0x13;
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Select cube:");
        if (g_mipe_cube_0068f12c == 0) {
            prompt = L"Click on a cube to select it.";
        } else {
            prompt = L"Click on another cube to select it.";
        }
        break;
    }
    ShowNoticef(0xf, prompt);
    g_flag_68f104 = 0;
    return 1;
}

/* The digit keys editing the selected cube parameter, plus the arrow keys
   stepping between Message, Search and Function. */
// FUNCTION: WIZ8 0x0057a630
int HandleCubeParameterKey0057A630(unsigned int key)
{
    int value;

    switch (key & 0xffff) {
    case 8:
        if (g_mipe_cube_0068f12c != 0) {
            value = GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c,
                                                        g_mipe_cube_param_0068f130);
            SetWorldCursorNodeParameter0048E2D0(g_mipe_cube_0068f12c, g_mipe_cube_param_0068f130,
                                                value / 10);
            DrawWorldCursorNodeLabel0048DCB0(g_mipe_cube_0068f12c);
            ShowCubeParameters005780F0();
            return 1;
        }
        break;
    default:
        return 0;
    case 0x20:
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Choose an action:");
        ShowNoticef(0xf, L"1) Create cube.");
        ShowNoticef(0xf, L"2) Delete cube.");
        ShowNoticef(0xf, L"3) Edit cube parameters.");
        ShowNoticef(0xf, L"4) Move cube.");
        ShowNoticef(0xf, L"5) Scale cube.");
        ShowNoticef(0xf, L"6) Select cube.");
        return 1;
    case 0x26:
        if (0 < g_mipe_cube_param_0068f130) {
            g_mipe_cube_param_0068f130 = g_mipe_cube_param_0068f130 - 1;
            ShowCubeParameters005780F0();
            return 1;
        }
        break;
    case 0x28:
        if (g_mipe_cube_param_0068f130 < 2) {
            g_mipe_cube_param_0068f130 = g_mipe_cube_param_0068f130 + 1;
            ShowCubeParameters005780F0();
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
        if (g_mipe_cube_0068f12c != 0 && (value = GetWorldCursorNodeParameter0048E2B0(
                                              g_mipe_cube_0068f12c, g_mipe_cube_param_0068f130),
                                          value < 99999)) {
            value = GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c,
                                                        g_mipe_cube_param_0068f130);
            SetWorldCursorNodeParameter0048E2D0(g_mipe_cube_0068f12c, g_mipe_cube_param_0068f130,
                                                ((key & 0xffff) - 0x30) + value * 10);
            DrawWorldCursorNodeLabel0048DCB0(g_mipe_cube_0068f12c);
            ShowCubeParameters005780F0();
        }
        break;
    }
    return 1;
}

/* The scale editor's keys: arrows resize the current plane and X/Y/Z pick the
   plane. */
// FUNCTION: WIZ8 0x0057a800
int HandleCubeScaleKey0057A800(unsigned short key)
{
    char scale_planes[8];

    switch (key) {
    case 0x20:
        scale_planes[0] = 'X';
        scale_planes[1] = 'Y';
        scale_planes[2] = 'Z';
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Scale Volume Trigger.");
        if (g_mipe_cube_0068f12c == 0) {
            ShowNoticef(0xf, L"Click on trigger to scale.");
            return 1;
        }
        ShowNoticef(0xf, L"Scaling in the %c plane. ",
                    static_cast<int>(scale_planes[g_mipe_scale_plane_0068f134]));
        ShowNoticef(0xf, L"Press X/Y/Z to change plane.");
        return 1;
    default:
        return 0;
    case 0x26:
        if (g_mipe_cube_0068f12c != 0) {
            if (g_mipe_scale_plane_0068f134 == 0) {
                ScaleWorldCursorNodeX0048DE40(g_mipe_cube_0068f12c, 1.1);
                return 1;
            }
            if (g_mipe_scale_plane_0068f134 == 1) {
                ScaleWorldCursorNodeY0048DE90(g_mipe_cube_0068f12c, 1.1);
                return 1;
            }
            if (g_mipe_scale_plane_0068f134 == 2) {
                ScaleWorldCursorNodeZ0048DEE0(g_mipe_cube_0068f12c, 1.1);
                return 1;
            }
        }
        break;
    case 0x28:
        if (g_mipe_cube_0068f12c != 0) {
            if (g_mipe_scale_plane_0068f134 == 0) {
                ScaleWorldCursorNodeX0048DE40(g_mipe_cube_0068f12c, 0.9);
                return 1;
            }
            if (g_mipe_scale_plane_0068f134 == 1) {
                ScaleWorldCursorNodeY0048DE90(g_mipe_cube_0068f12c, 0.9);
                return 1;
            }
            if (g_mipe_scale_plane_0068f134 == 2) {
                ScaleWorldCursorNodeZ0048DEE0(g_mipe_cube_0068f12c, 0.9);
                return 1;
            }
        }
        break;
    case 0x58:
        g_mipe_scale_plane_0068f134 = 0;
        return 1;
    case 0x59:
        g_mipe_scale_plane_0068f134 = 1;
        return 1;
    case 0x5a:
        g_mipe_scale_plane_0068f134 = 2;
        break;
    }
    return 1;
}

/* The monster generator top-menu keys. */
// FUNCTION: WIZ8 0x0057aa00
int HandleMonsterGeneratorKey0057AA00(unsigned short key)
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
        GetWorldCursorAnchor00490C20(&anchor);
        count = GetMonsterGeneratorCount();
        index = 0;
        if (0 < count) {
            do {
                GetMonsterGenerator(index)->SetActive(1, 0);
                ++index;
            } while (index < count);
        }
        g_mipe_mongen_visible_0068f0fe = 1;
        ShowMonsterGeneratorStatus005781F0();
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
        g_mipe_state_0068f100->generator = generator;
        if (generator->marker_item != 0) {
            static_cast<W8ItemRep*>(generator->marker_item->m_pRep)->SetFlags(0x10, 1);
            generator->marker_item->SetHighlight(true);
        }
        return 1;
    case 0x32:
        if (g_mipe_state_0068f100->generator != 0) {
            RemoveMonsterGenerator(g_mipe_state_0068f100->generator);
            g_mipe_state_0068f100->generator = 0;
            ShowMonsterGeneratorStatus005781F0();
        }
        return 1;
    case 0x33:
        if (g_mipe_state_0068f100->generator != 0) {
            g_mipe_mode_0068f108 = 0x16;
            ShowMonsterGeneratorEditor005782D0();
        }
        return 1;
    case 0x34:
        g_mipe_state_0068f100->selecting = 1;
        HideWorldCursor00490B90();
        ShowMonsterGeneratorStatus005781F0();
        return 1;
    case 0x35: {
        unsigned char visible = g_mipe_mongen_visible_0068f0fe == 0;
        count = GetMonsterGeneratorCount();
        index = 0;
        if (0 < count) {
            do {
                GetMonsterGenerator(index)->SetActive(visible, 0);
                ++index;
            } while (index < count);
        }
        g_mipe_mongen_visible_0068f0fe = visible;
        ShowMonsterGeneratorStatus005781F0();
        ShowMonsterGeneratorStatus005781F0();
        return 1;
    }
    case 0x36:
        g_generator_save_flag = g_generator_save_flag == 0;
        break;
    }
    ShowMonsterGeneratorStatus005781F0();
    return 1;
}

/* The edit-menu keys for the selected monster generator: name entry, table
   picker, active toggle, per-generator or shared chance and interval, and the
   shared-interval flag. */
// FUNCTION: WIZ8 0x0057ad10
int HandleMonsterGeneratorEditKey0057AD10(unsigned short key)
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

    if (g_mipe_category_list_0068f11c == 0) {
        list = PLCreate();
        g_mipe_category_list_0068f11c = list;
        if (list != 0) {
            PListClear(list);
            count = g_encounter_tables.count;
            index = 0;
            if (0 < g_encounter_tables.count) {
                do {
                    table = GetEncounterTable(index);
                    if (table->unknown_150 == static_cast<unsigned int>(g_mipe_category_0068f114)) {
                        PLAdoptAppend(list, table);
                    }
                    ++index;
                } while (index < count);
            }
        }
    }
    switch (key) {
    case 0x20:
        ShowMonsterGeneratorStatus005781F0();
        return 1;
    default:
        return 0;
    case 0x31:
        g_mipe_mode_0068f108 = 0x19;
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Enter the name for this generator:");
        ShowNoticef(0xf, L"%S", g_mipe_state_0068f100->generator->name);
        return 1;
    case 0x32:
        current_index = g_mipe_state_0068f100->generator->encounter_table_index;
        if (current_index < 0) {
            g_mipe_table_base_0068f120 = 0;
            g_mipe_table_row_0068f118 = 0;
        } else {
            table = GetEncounterTable(current_index);
            list = g_mipe_category_list_0068f11c;
            g_mipe_category_0068f114 = static_cast<unsigned char>(table->unknown_150);
            if (g_mipe_category_list_0068f11c != 0) {
                PListClear(g_mipe_category_list_0068f11c);
                count = g_encounter_tables.count;
                index = 0;
                if (0 < g_encounter_tables.count) {
                    do {
                        table = GetEncounterTable(index);
                        if (table->unknown_150 ==
                            static_cast<unsigned int>(g_mipe_category_0068f114)) {
                            PLAdoptAppend(list, table);
                        }
                        ++index;
                    } while (index < count);
                }
            }
            found = 0;
            if (0 < static_cast<int>(PLLength(g_mipe_category_list_0068f11c))) {
                do {
                    table = static_cast<W8EncounterTableRuntime*>(
                        PLGet(g_mipe_category_list_0068f11c, found));
                    entry = GetEncounterTable(current_index);
                    if (table == entry) {
                        break;
                    }
                    ++found;
                } while (found < static_cast<int>(PLLength(g_mipe_category_list_0068f11c)));
            }
            g_mipe_table_base_0068f120 = (found / 6) * 6;
            g_mipe_table_row_0068f118 = found % 6;
        }
        g_mipe_mode_0068f108 = 0x17;
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Category: %S", *g_encounter_names.GetAt(g_mipe_category_0068f114 & 0xff));
        if (g_mipe_category_list_0068f11c == 0) {
            return 1;
        }
        slot = 0;
        do {
            entry = static_cast<W8EncounterTableRuntime*>(
                PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + slot));
            if (entry == 0) {
                ShowNoticef(0xf, &g_wchar_00689b34);
            } else {
                ShowNoticef(slot == g_mipe_table_row_0068f118 ? 3 : 0xf, L"    %S", entry->name);
            }
            ++slot;
        } while (slot < 6);
        return 1;
    case 0x33:
        g_mipe_state_0068f100->generator->generation_enabled =
            g_mipe_state_0068f100->generator->generation_enabled == 0;
        ShowMonsterGeneratorEditor005782D0();
        return 1;
    case 0x34:
        generator = g_mipe_state_0068f100->generator;
        if ((generator->flags >> 3 & 1) == 0) {
            if ('\0' < generator->custom_spawn_chance) {
                generator->custom_spawn_chance = generator->custom_spawn_chance - 10;
                ShowMonsterGeneratorEditor005782D0();
                return 1;
            }
        } else if ('\0' < static_cast<char>(g_generator_interval_min)) {
            g_generator_interval_min = static_cast<short>(
                static_cast<char>(static_cast<char>(g_generator_interval_min) - 10));
            ShowMonsterGeneratorEditor005782D0();
            return 1;
        }
        break;
    case 0x35:
        generator = g_mipe_state_0068f100->generator;
        if ((generator->flags >> 3 & 1) == 0) {
            if (generator->custom_spawn_chance < 'd') {
                generator->custom_spawn_chance = generator->custom_spawn_chance + '\n';
                ShowMonsterGeneratorEditor005782D0();
                return 1;
            }
        } else if (static_cast<char>(g_generator_interval_min) < 'd') {
            g_generator_interval_min = static_cast<short>(
                static_cast<char>(static_cast<char>(g_generator_interval_min) + '\n'));
            ShowMonsterGeneratorEditor005782D0();
            return 1;
        }
        break;
    case 0x36:
        generator = g_mipe_state_0068f100->generator;
        if ((generator->flags >> 3 & 1) == 0) {
            if (0 < generator->custom_interval_seconds) {
                generator->custom_interval_seconds = generator->custom_interval_seconds - 10;
                g_mipe_state_0068f100->generator->Reset();
                ShowMonsterGeneratorEditor005782D0();
                return 1;
            }
        } else if (0 < g_generator_default_interval) {
            g_generator_default_interval = g_generator_default_interval - 10;
            ShowMonsterGeneratorEditor005782D0();
            return 1;
        }
        break;
    case 0x37:
        generator = g_mipe_state_0068f100->generator;
        if ((generator->flags >> 3 & 1) == 0) {
            generator->custom_interval_seconds = generator->custom_interval_seconds + 10;
            g_mipe_state_0068f100->generator->Reset();
            ShowMonsterGeneratorEditor005782D0();
            return 1;
        }
        g_generator_default_interval = g_generator_default_interval + 10;
        ShowMonsterGeneratorEditor005782D0();
        return 1;
    case 0x38:
        generator = g_mipe_state_0068f100->generator;
        if ((generator->flags >> 3 & 1) != 0) {
            generator->flags = generator->flags & 0xfffffff7;
            ShowMonsterGeneratorEditor005782D0();
            return 1;
        }
        generator->flags = generator->flags | 8;
        break;
    }
    ShowMonsterGeneratorEditor005782D0();
    return 1;
}

/* Mode-0x17 key handler: the generator's encounter-table picker. Enter binds
   the highlighted table to the selected generator; the arrows page/walk the
   rows and cycle through the categories. */
// FUNCTION: WIZ8 0x0057b1a0
static void HandleMipeGeneratorTableKey0057B1A0(unsigned short key)
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
        if (g_mipe_state_0068f100->generator != 0) {
            found = 0;
            table_index = 0;
            index = g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118;
            if (0 < g_encounter_tables.count) {
                unsigned int category = g_mipe_category_0068f114 & 0xff;
                do {
                    entry = GetEncounterTable(table_index);
                    if (entry->unknown_150 == category) {
                        if (found == index) {
                            break;
                        }
                        ++found;
                    }
                    ++table_index;
                } while (static_cast<int>(table_index) < count);
            }
            g_mipe_state_0068f100->generator->SetEncounterTable(table_index & 0xffff);
        }
        ShowMonsterGeneratorEditor005782D0();
        g_mipe_mode_0068f108 = 0x16;
        return;
    case 0x20:
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Category: %S", *g_encounter_names.GetAt(g_mipe_category_0068f114 & 0xff));
        if (g_mipe_category_list_0068f11c != 0) {
            row = 0;
            do {
                entry = static_cast<W8EncounterTableRuntime*>(
                    PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + row));
                if (entry == 0) {
                    ShowNoticef(0xf, &g_wchar_00689b34);
                } else {
                    palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", entry->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x21:
        if (g_mipe_table_base_0068f120 == 0) {
            return;
        }
        g_mipe_table_base_0068f120 = g_mipe_table_base_0068f120 - 6;
        if (g_mipe_table_base_0068f120 < 0) {
            g_mipe_table_base_0068f120 = 0;
            ShowMipeEncounterCategory005783C0();
            return;
        }
        ShowMipeEncounterCategory005783C0();
        break;
    case 0x22:
        if (g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118 <
            static_cast<int>(PLLength(g_mipe_category_list_0068f11c) - 6)) {
            g_mipe_table_base_0068f120 = g_mipe_table_base_0068f120 + 6;
            ShowMipeEncounterCategory005783C0();
            return;
        }
        if (static_cast<int>(PLLength(g_mipe_category_list_0068f11c)) <=
            g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118) {
            return;
        }
        g_mipe_table_base_0068f120 = g_mipe_table_base_0068f120 + 6;
        g_mipe_table_row_0068f118 = 0;
        if (static_cast<int>(PLLength(g_mipe_category_list_0068f11c)) <=
            g_mipe_table_base_0068f120) {
            g_mipe_table_base_0068f120 =
                static_cast<int>(PLLength(g_mipe_category_list_0068f11c)) - 1;
        }
        ShowMipeEncounterCategory005783C0();
        break;
    case 0x25:
        g_mipe_table_row_0068f118 = 0;
        g_mipe_table_base_0068f120 = 0;
        wraps = 0;
        do {
            list = g_mipe_category_list_0068f11c;
            unsigned char category;
            if (g_mipe_category_0068f114 == 0) {
                g_mipe_category_0068f114 = static_cast<unsigned char>(g_encounter_names.count);
                ++wraps;
            }
            category = g_mipe_category_0068f114 - 1;
            g_mipe_category_0068f114 = category;
            if (g_mipe_category_list_0068f11c != 0) {
                PListClear(g_mipe_category_list_0068f11c);
                count = g_encounter_tables.count;
                index = 0;
                if (0 < g_encounter_tables.count) {
                    do {
                        entry = GetEncounterTable(index);
                        if (entry->unknown_150 == static_cast<unsigned int>(category)) {
                            PLAdoptAppend(list, entry);
                        }
                        ++index;
                    } while (index < count);
                }
            }
        } while (PLLength(g_mipe_category_list_0068f11c) == 0 && wraps < 2);
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Category: %S", *g_encounter_names.GetAt(g_mipe_category_0068f114 & 0xff));
        if (g_mipe_category_list_0068f11c != 0) {
            row = 0;
            do {
                entry = static_cast<W8EncounterTableRuntime*>(
                    PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + row));
                if (entry == 0) {
                    ShowNoticef(0xf, &g_wchar_00689b34);
                } else {
                    palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", entry->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x26:
        if (g_mipe_table_row_0068f118 == 0) {
            if (g_mipe_table_base_0068f120 != 0) {
                --g_mipe_table_base_0068f120;
            }
        } else {
            --g_mipe_table_row_0068f118;
        }
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Category: %S", *g_encounter_names.GetAt(g_mipe_category_0068f114 & 0xff));
        if (g_mipe_category_list_0068f11c != 0) {
            row = 0;
            do {
                entry = static_cast<W8EncounterTableRuntime*>(
                    PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + row));
                if (entry == 0) {
                    ShowNoticef(0xf, &g_wchar_00689b34);
                } else {
                    palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", entry->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x27:
        g_mipe_table_row_0068f118 = 0;
        g_mipe_table_base_0068f120 = 0;
        wraps = 0;
        do {
            list = g_mipe_category_list_0068f11c;
            if ((g_mipe_category_0068f114 & 0xff) < g_encounter_names.count - 1) {
                g_mipe_category_0068f114 = g_mipe_category_0068f114 + 1;
            } else {
                ++wraps;
                g_mipe_category_0068f114 = 0;
            }
            if (g_mipe_category_list_0068f11c != 0) {
                PListClear(g_mipe_category_list_0068f11c);
                count = g_encounter_tables.count;
                index = 0;
                if (0 < g_encounter_tables.count) {
                    do {
                        entry = GetEncounterTable(index);
                        if (entry->unknown_150 ==
                            static_cast<unsigned int>(g_mipe_category_0068f114)) {
                            PLAdoptAppend(list, entry);
                        }
                        ++index;
                    } while (index < count);
                }
            }
        } while (PLLength(g_mipe_category_list_0068f11c) == 0 && wraps < 2);
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Category: %S", *g_encounter_names.GetAt(g_mipe_category_0068f114 & 0xff));
        if (g_mipe_category_list_0068f11c != 0) {
            row = 0;
            do {
                entry = static_cast<W8EncounterTableRuntime*>(
                    PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + row));
                if (entry == 0) {
                    ShowNoticef(0xf, &g_wchar_00689b34);
                } else {
                    palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", entry->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x28:
        if (g_mipe_table_row_0068f118 < 5 &&
            g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118 <
                static_cast<int>(PLLength(g_mipe_category_list_0068f11c) - 1)) {
            ++g_mipe_table_row_0068f118;
            ShowMipeEncounterCategory005783C0();
            return;
        }
        if (g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118 <
            static_cast<int>(PLLength(g_mipe_category_list_0068f11c) - 1)) {
            ++g_mipe_table_base_0068f120;
            ShowMipeEncounterCategory005783C0();
            return;
        }
    }
}

/* Character entry for the generator name, capped at the 32-byte field. */
// FUNCTION: WIZ8 0x0057b7e0
void EditMonsterGeneratorName0057B7E0(unsigned short key)
{
    char* name;
    int length;

    name = g_mipe_state_0068f100->generator->name;
    length = strlen(name);
    if (key == 8) {
        if (0 < length) {
            name[length - 1] = '\0';
        }
    } else if ((((0x2f < key) && (key < 0x3a)) || ((0x40 < key) && (key < 0x5b))) &&
               (length < 0x1f)) {
        if (gfKeyState[0x10] == 0) {
            key = key + 0x20;
        }
        name[length] = static_cast<char>(key);
        name[length + 1] = '\0';
    }
    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Enter the name for this generator:");
    ShowNoticef(0xf, L"%S", g_mipe_state_0068f100->generator->name);
}

/* Mode-0x1b key handler: the selected prop trigger's locks & traps editor.
   '1' cycles the lock type and re-rolls the pin state, '2' enters key-id
   input, '3'/'4' bump the difficulty, and every path repaints the menu. */
// FUNCTION: WIZ8 0x0057b880
static void HandleMipeLockTrapKey0057B880(unsigned short key)
{
    Trigger* trigger;
    Trigger* action_trigger;
    W8TriggerActionData* action;
    const wchar_t* key_name;
    int* lock_state;
    int key_id;
    bool pending;

    trigger = g_mipe_state_0068f100->prop->GetValue18();
    lock_state = &trigger->value_368;
    action_trigger = g_mipe_state_0068f100->prop->GetValue18();
    action = action_trigger->m_pActionData;
    if (action == 0 || action->type_004 != '\n') {
        action = 0;
    }
    switch (key) {
    case 0x31:
        *lock_state = *lock_state + 1;
        if (3 < *lock_state) {
            *lock_state = 0;
        }
        UpdateTriggerLock00445730(lock_state);
        if (action == 0) {
            if (*lock_state == 3) {
                key_id = trigger->value_380;
                g_mipe_state_0068f100->prop->GetValue18()->value_23c = key_id;
            } else {
                g_mipe_state_0068f100->prop->GetValue18()->value_23c = 0xffffffff;
            }
        } else {
            if (*lock_state == 0 || trigger->state_370.state != 0) {
                pending = 0;
            } else {
                pending = 1;
            }
            static_cast<W8DoorTriggerActionData*>(action)->flags_008 =
                (pending << 2) | (static_cast<W8DoorTriggerActionData*>(action)->flags_008 & 0xfb);
            static_cast<W8DoorTriggerActionData*>(action)->item_00a =
                static_cast<short>(trigger->value_380);
        }
        break;
    case 0x32:
        g_mipe_mode_0068f108 = 0x1c;
        trigger = g_mipe_state_0068f100->prop->GetValue18();
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Enter Key ID:");
        ShowNoticef(0xf, g_format_d_0060aa20, trigger->value_380);
        return;
    case 0x33:
        trigger->value_36c = trigger->value_36c + 1;
        if (10 < trigger->value_36c) {
            trigger->value_36c = 10;
        }
        break;
    case 0x34:
        trigger->value_36c = trigger->value_36c - 1;
        if (trigger->value_36c < 0) {
            trigger->value_36c = 0;
        }
    }
    trigger = g_mipe_state_0068f100->prop->GetValue18();
    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Edit Locks & Traps");
    ShowNoticef(0xf, L"1) Type: %s", g_lock_type_names_0064a1d0[trigger->value_368]);
    key_id = trigger->value_380;
    if (key_id < 0) {
        key_name = &g_wchar_00689b34;
    } else {
        key_name = g_item_records[key_id].display_name;
    }
    ShowNoticef(0xf, L"2) Key Id: (%d) %s", key_id, key_name);
    ShowNoticef(0xf, L" Difficulty (3+/4-): %d", trigger->value_36c);
}

/* The digit keys editing the selected prop trigger's key id. */
// FUNCTION: WIZ8 0x0057ba60
void EditTriggerKeyID0057BA60(unsigned int key)
{
    Trigger* trigger;
    W8TriggerActionData* action;
    int key_id;
    unsigned char pending;

    trigger = g_mipe_state_0068f100->prop->GetValue18();
    action = g_mipe_state_0068f100->prop->GetValue18()->m_pActionData;
    if (action == 0 || action->type_004 != '\n') {
        action = 0;
    }
    key_id = trigger->value_380;
    switch (key & 0xffff) {
    case 8:
        key_id = key_id / 10;
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
    trigger->value_380 = key_id;
    if (action == 0) {
        if (trigger->value_368 == 3) {
            g_mipe_state_0068f100->prop->GetValue18()->value_23c = key_id;
        } else {
            g_mipe_state_0068f100->prop->GetValue18()->value_23c = 0xffffffff;
        }
    } else {
        if (trigger->value_368 == 0 || trigger->state_370.state != 0) {
            pending = 0;
        } else {
            pending = 1;
        }
        static_cast<W8DoorTriggerActionData*>(action)->flags_008 =
            (pending << 2) | (static_cast<W8DoorTriggerActionData*>(action)->flags_008 & 0xfb);
        static_cast<W8DoorTriggerActionData*>(action)->item_00a =
            static_cast<short>(trigger->value_380);
    }
    trigger = g_mipe_state_0068f100->prop->GetValue18();
    ResetEditorStatusLine0058AA20(-1);
    ShowNoticef(6, L"Enter Key ID:");
    ShowNoticef(0xf, g_format_d_0060aa20, trigger->value_380);
}

/* Mode-0x1d key handler: the prop trigger's treasure-table picker. Enter
   copies the highlighted table's name into the trigger's inline action data
   and returns to the prop menu; the arrows page/walk rows and cycle the item
   table categories. */
// FUNCTION: WIZ8 0x0057bbd0
static void HandleMipeItemTableKey0057BBD0(unsigned short key)
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
        if (g_mipe_state_0068f100->prop != 0) {
            table_index = FindCategoryItemTable0057DBD0(g_mipe_category_0068f114 & 0xff,
                                                        g_mipe_table_base_0068f120 +
                                                            g_mipe_table_row_0068f118);
            table = g_item_tables[table_index & 0xffff];
            trigger = g_mipe_state_0068f100->prop->GetValue18();
            strcpy(trigger->inline_action_data_24c, table->name);
            trigger->flag_350 = 0;
        }
        ShowMipePropMenu00577EB0();
        g_mipe_mode_0068f108 = 0xd;
        return;
    case 0x20:
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Category: %S",
                    g_item_table_category_names[g_mipe_category_0068f114 & 0xff]);
        if (g_mipe_category_list_0068f11c != 0) {
            row = 0;
            do {
                table = static_cast<W8ItemTableRecord*>(
                    PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + row));
                if (table == 0) {
                    ShowNoticef(0xf, &g_wchar_00689b34);
                } else {
                    palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", table->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x21:
        if (g_mipe_table_base_0068f120 != 0) {
            g_mipe_table_base_0068f120 = g_mipe_table_base_0068f120 - 6;
            if (g_mipe_table_base_0068f120 < 0) {
                g_mipe_table_base_0068f120 = 0;
            }
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"Category: %S",
                        g_item_table_category_names[g_mipe_category_0068f114 & 0xff]);
            if (g_mipe_category_list_0068f11c != 0) {
                row = 0;
                do {
                    table = static_cast<W8ItemTableRecord*>(
                        PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + row));
                    if (table == 0) {
                        ShowNoticef(0xf, &g_wchar_00689b34);
                    } else {
                        palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
                        ShowNoticef(palette, L"    %S", table->name);
                    }
                    ++row;
                } while (row < 6);
                return;
            }
        }
        break;
    case 0x22:
        if (g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118 <
            static_cast<int>(PLLength(g_mipe_category_list_0068f11c) - 6)) {
            g_mipe_table_base_0068f120 = g_mipe_table_base_0068f120 + 6;
            ShowMipeItemTableCategory00578470();
            return;
        }
        if (g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118 <
            static_cast<int>(PLLength(g_mipe_category_list_0068f11c))) {
            g_mipe_table_base_0068f120 = g_mipe_table_base_0068f120 + 6;
            g_mipe_table_row_0068f118 = 0;
            if (static_cast<int>(PLLength(g_mipe_category_list_0068f11c)) <=
                g_mipe_table_base_0068f120) {
                g_mipe_table_base_0068f120 =
                    static_cast<int>(PLLength(g_mipe_category_list_0068f11c)) - 1;
            }
            ShowMipeItemTableCategory00578470();
        }
        break;
    case 0x25:
        wraps = 0;
        g_mipe_table_row_0068f118 = 0;
        g_mipe_table_base_0068f120 = 0;
        do {
            unsigned char category;
            list = g_mipe_category_list_0068f11c;
            category = g_mipe_category_0068f114;
            if (g_mipe_category_0068f114 == 0) {
                ++wraps;
                category = static_cast<unsigned char>(gXStatus.uiItemTableCategories);
            }
            g_mipe_category_0068f114 = category - 1;
            if (g_mipe_category_list_0068f11c != 0) {
                PListClear(g_mipe_category_list_0068f11c);
                index = 0;
                if (0 < static_cast<int>(gXStatus.uiItemTablesInDatabase)) {
                    do {
                        table = g_item_tables[index];
                        if (table->category_id == static_cast<unsigned int>(category - 1)) {
                            PLAdoptAppend(list, table);
                        }
                        ++index;
                    } while (index < static_cast<int>(gXStatus.uiItemTablesInDatabase));
                }
            }
        } while (PLLength(g_mipe_category_list_0068f11c) == 0 && wraps < 2);
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Category: %S",
                    g_item_table_category_names[g_mipe_category_0068f114 & 0xff]);
        if (g_mipe_category_list_0068f11c != 0) {
            row = 0;
            do {
                table = static_cast<W8ItemTableRecord*>(
                    PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + row));
                if (table == 0) {
                    ShowNoticef(0xf, &g_wchar_00689b34);
                } else {
                    palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", table->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x26:
        if (g_mipe_table_row_0068f118 == 0) {
            if (g_mipe_table_base_0068f120 != 0) {
                --g_mipe_table_base_0068f120;
            }
        } else {
            --g_mipe_table_row_0068f118;
        }
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Category: %S",
                    g_item_table_category_names[g_mipe_category_0068f114 & 0xff]);
        if (g_mipe_category_list_0068f11c != 0) {
            row = 0;
            do {
                table = static_cast<W8ItemTableRecord*>(
                    PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + row));
                if (table == 0) {
                    ShowNoticef(0xf, &g_wchar_00689b34);
                } else {
                    palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", table->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x27:
        wraps = 0;
        g_mipe_table_row_0068f118 = 0;
        g_mipe_table_base_0068f120 = 0;
        do {
            list = g_mipe_category_list_0068f11c;
            if ((g_mipe_category_0068f114 & 0xff) < gXStatus.uiItemTableCategories - 1) {
                g_mipe_category_0068f114 = g_mipe_category_0068f114 + 1;
            } else {
                ++wraps;
                g_mipe_category_0068f114 = 0;
            }
            if (g_mipe_category_list_0068f11c != 0) {
                PListClear(g_mipe_category_list_0068f11c);
                index = 0;
                if (0 < static_cast<int>(gXStatus.uiItemTablesInDatabase)) {
                    do {
                        table = g_item_tables[index];
                        if (table->category_id ==
                            static_cast<unsigned int>(g_mipe_category_0068f114 & 0xff)) {
                            PLAdoptAppend(list, table);
                        }
                        ++index;
                    } while (index < static_cast<int>(gXStatus.uiItemTablesInDatabase));
                }
            }
        } while (PLLength(g_mipe_category_list_0068f11c) == 0 && wraps < 2);
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Category: %S",
                    g_item_table_category_names[g_mipe_category_0068f114 & 0xff]);
        if (g_mipe_category_list_0068f11c != 0) {
            row = 0;
            do {
                table = static_cast<W8ItemTableRecord*>(
                    PLGet(g_mipe_category_list_0068f11c, g_mipe_table_base_0068f120 + row));
                if (table == 0) {
                    ShowNoticef(0xf, &g_wchar_00689b34);
                } else {
                    palette = row == g_mipe_table_row_0068f118 ? 3 : 0xf;
                    ShowNoticef(palette, L"    %S", table->name);
                }
                ++row;
            } while (row < 6);
            return;
        }
        break;
    case 0x28:
        if (g_mipe_table_row_0068f118 < 5 &&
            g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118 <
                static_cast<int>(PLLength(g_mipe_category_list_0068f11c) - 1)) {
            ++g_mipe_table_row_0068f118;
            ShowMipeItemTableCategory00578470();
            return;
        }
        if (g_mipe_table_base_0068f120 + g_mipe_table_row_0068f118 <
            static_cast<int>(PLLength(g_mipe_category_list_0068f11c) - 1)) {
            ++g_mipe_table_base_0068f120;
            ShowMipeItemTableCategory00578470();
            return;
        }
    }
}

/* The MIPE master key handler, fed input atoms while the panel is up. The
   early Escape block unwinds the mode stack, the combat-timer shortcut keeps
   only 'V'/'X' live, and the mode switch routes everything else into the
   per-mode handlers above. */
// FUNCTION: WIZ8 0x0057c230
unsigned char HandleMipeKey0057C230(const InputAtom* event)
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
    if (g_mipe_state_0068f100->dragging != 0) {
        return 1;
    }
    key = static_cast<unsigned short>(event->usParam);
    if (key == 0x1b) {
        if (event->usEvent == 2 || event->usEvent == 4) {
            g_mipe_count_0068f10c = 0;
            if (g_mipe_mode_0068f108 == 6) {
                g_mipe_mode_0068f108 = 1;
                ShowMipeMonsterStatus00577BF0();
            } else if (g_mipe_mode_0068f108 == 7) {
                g_mipe_mode_0068f108 = 2;
                ShowMipeItemStatus00577CB0();
            } else if (g_mipe_mode_0068f108 == 2 || g_mipe_mode_0068f108 == 1) {
                g_mipe_mode_0068f108 = 0;
                if (g_mipe_category_list_0068f11c != 0) {
                    PLDestroy(g_mipe_category_list_0068f11c);
                    g_mipe_category_list_0068f11c = 0;
                }
            } else {
                if (g_mipe_mode_0068f108 == 5) {
                    g_mipe_mode_0068f108 = 0;
                } else {
                    if (g_mipe_mode_0068f108 == 8) {
                        g_mipe_mode_0068f108 = 5;
                        ShowMipeEditMenu00577E60();
                        g_debug_monster_cycle_0068f0fc = 0;
                        return 1;
                    }
                    if (g_mipe_mode_0068f108 == 0xd) {
                        g_mipe_mode_0068f108 = 5;
                        ShowWorldCursor00490B10();
                        ShowMipeEditMenu00577E60();
                    } else if (g_mipe_mode_0068f108 == 0xe) {
                        g_mipe_mode_0068f108 = 0xd;
                        HandleMipeEditPropKey005C3880(0x1b);
                        ShowMipePropMenu00577EB0();
                    } else if (g_mipe_mode_0068f108 == 3) {
                        g_mipe_mode_0068f108 = 8;
                        ResetEditorStatusLine0058AA20(-1);
                        ShowNoticef(6, L"Edit Monster");
                        ShowNoticef(0xf, L"1) Slow <<");
                        ShowNoticef(0xf, L"2) Fast >>");
                        ShowNoticef(0xf, L"3) Set Speed");
                        ShowNoticef(0xf, L"4) Assign Path");
                        ShowNoticef(0xf, L"5) Next Cycle");
                        ShowNoticef(0xf, L"6) Direction");
                    } else if (g_mipe_mode_0068f108 == 9) {
                        ShowWorldCursor00490B10();
                        g_mipe_mode_0068f108 = 0;
                        g_mipe_state_0068f100->selecting = 0;
                    } else {
                        if (g_mipe_mode_0068f108 != 4) {
                            if (g_mipe_mode_0068f108 == 0xb) {
                                g_mipe_mode_0068f108 = 8;
                                g_mipe_state_0068f100->selecting = 0;
                                ResetEditorStatusLine0058AA20(-1);
                                ShowNoticef(6, L"Edit Monster");
                                ShowNoticef(0xf, L"1) Slow <<");
                                ShowNoticef(0xf, L"2) Fast >>");
                                ShowNoticef(0xf, L"3) Set Speed");
                                ShowNoticef(0xf, L"4) Assign Path");
                                ShowNoticef(0xf, L"5) Next Cycle");
                                ShowNoticef(0xf, L"6) Direction");
                            } else if (g_mipe_mode_0068f108 == 0xc) {
                                if (g_mipe_state_0068f100->selecting == 0) {
                                    g_mipe_mode_0068f108 = 0;
                                    trigger = g_mipe_state_0068f100->trigger;
                                    if (trigger != 0 &&
                                        (rep_item = trigger->rep_item_114, rep_item != 0)) {
                                        rep_item->SetHighlight(false);
                                    }
                                    g_mipe_state_0068f100->trigger = 0;
                                } else {
                                    ShowWorldCursor00490B10();
                                    g_mipe_state_0068f100->selecting = 0;
                                    ShowMipeTriggerMenu00578000();
                                }
                            } else if (g_mipe_mode_0068f108 == 0x15) {
                                if (g_mipe_state_0068f100->selecting == 0) {
                                    g_mipe_mode_0068f108 = 0;
                                    if (g_mipe_state_0068f100->generator != 0 &&
                                        g_mipe_state_0068f100->generator->marker_item != 0) {
                                        g_mipe_state_0068f100->generator->marker_item->SetHighlight(
                                            false);
                                    }
                                    g_mipe_state_0068f100->generator = 0;
                                    if (g_mipe_category_list_0068f11c != 0) {
                                        PLDestroy(g_mipe_category_list_0068f11c);
                                        g_mipe_category_list_0068f11c = 0;
                                    }
                                } else {
                                    ShowWorldCursor00490B10();
                                    g_mipe_state_0068f100->selecting = 0;
                                    ShowMonsterGeneratorStatus005781F0();
                                }
                            } else if (g_mipe_mode_0068f108 == 0x16) {
                                g_mipe_state_0068f100->selecting = 0;
                                g_mipe_mode_0068f108 = 0x15;
                                ShowMonsterGeneratorStatus005781F0();
                            } else if (g_mipe_mode_0068f108 == 0x17) {
                                g_mipe_state_0068f100->selecting = 0;
                                g_mipe_mode_0068f108 = 0x16;
                                ShowMonsterGeneratorEditor005782D0();
                            } else if (g_mipe_mode_0068f108 == 0xf) {
                                if (g_mipe_state_0068f100->selecting == 0) {
                                    ShowMipeTriggerMenu00578000();
                                    SetWorldCursorNodesVisible0048ED70(g_value_0068f0fd);
                                    g_mipe_mode_0068f108 = 0xc;
                                    g_flag_68f104 = 0;
                                } else {
                                    ShowWorldCursor00490B10();
                                    g_mipe_state_0068f100->selecting = 0;
                                    ResetEditorStatusLine0058AA20(-1);
                                    ShowNoticef(6, L"Choose an action:");
                                    ShowNoticef(0xf, L"1) Create cube.");
                                    ShowNoticef(0xf, L"2) Delete cube.");
                                    ShowNoticef(0xf, L"3) Edit cube parameters.");
                                    ShowNoticef(0xf, L"4) Move cube.");
                                    ShowNoticef(0xf, L"5) Scale cube.");
                                    ShowNoticef(0xf, L"6) Select cube.");
                                }
                            } else {
                                if (g_mipe_mode_0068f108 != 0x10) {
                                    if (g_mipe_mode_0068f108 == 0x11) {
                                        g_mipe_mode_0068f108 = 0xf;
                                        g_mipe_state_0068f100->selecting = 0;
                                        HideWorldCursor00490B90();
                                        g_mipe_state_0068f100->dragging = 0;
                                        g_flag_68f104 = 1;
                                        ResetEditorStatusLine0058AA20(-1);
                                        ShowNoticef(6, L"Choose an action:");
                                        ShowNoticef(0xf, L"1) Create cube.");
                                        ShowNoticef(0xf, L"2) Delete cube.");
                                        ShowNoticef(0xf, L"3) Edit cube parameters.");
                                        ShowNoticef(0xf, L"4) Move cube.");
                                        ShowNoticef(0xf, L"5) Scale cube.");
                                        ShowNoticef(0xf, L"6) Select cube.");
                                        return 1;
                                    }
                                    if (g_mipe_mode_0068f108 == 0x12 ||
                                        g_mipe_mode_0068f108 == 0x13) {
                                        g_mipe_mode_0068f108 = 0xf;
                                        ResetEditorStatusLine0058AA20(-1);
                                        ShowNoticef(6, L"Choose an action:");
                                        ShowNoticef(0xf, L"1) Create cube.");
                                        ShowNoticef(0xf, L"2) Delete cube.");
                                        ShowNoticef(0xf, L"3) Edit cube parameters.");
                                        ShowNoticef(0xf, L"4) Move cube.");
                                        ShowNoticef(0xf, L"5) Scale cube.");
                                        ShowNoticef(0xf, L"6) Select cube.");
                                        g_mipe_state_0068f100->selecting = 0;
                                    } else if (g_mipe_mode_0068f108 == 0x18) {
                                        g_mipe_mode_0068f108 = 0x15;
                                        ShowWorldCursor00490B10();
                                        ShowMonsterGeneratorStatus005781F0();
                                    } else if (g_mipe_mode_0068f108 == 0x19) {
                                        g_mipe_mode_0068f108 = 0x16;
                                        ShowWorldCursor00490B10();
                                        ShowMonsterGeneratorEditor005782D0();
                                    } else if (g_mipe_mode_0068f108 == 0x1a) {
                                        g_mipe_mode_0068f108 = 0x15;
                                        ShowWorldCursor00490B10();
                                        ShowMonsterGeneratorStatus005781F0();
                                    } else if (g_mipe_mode_0068f108 == 0x1b) {
                                        g_mipe_mode_0068f108 = 0xd;
                                        ShowMipePropMenu00577EB0();
                                    } else if (g_mipe_mode_0068f108 == 0x1c) {
                                        g_mipe_mode_0068f108 = 0x1b;
                                        trigger = g_mipe_state_0068f100->prop->GetValue18();
                                        ResetEditorStatusLine0058AA20(-1);
                                        ShowNoticef(6, L"Edit Locks & Traps");
                                        ShowNoticef(0xf, L"1) Type: %s",
                                                    g_lock_type_names_0064a1d0[trigger->value_368]);
                                        key_id = trigger->value_380;
                                        if (key_id < 0) {
                                            key_name = &g_wchar_00689b34;
                                        } else {
                                            key_name = g_item_records[key_id].display_name;
                                        }
                                        ShowNoticef(0xf, L"2) Key Id: (%d) %s", key_id, key_name);
                                        ShowNoticef(0xf, L" Difficulty (3+/4-): %d",
                                                    trigger->value_36c);
                                    } else {
                                        if (g_mipe_mode_0068f108 == 0x1d) {
                                            g_mipe_mode_0068f108 = 0xd;
                                            ShowMipePropMenu00577EB0();
                                            g_flag_68f104 = 1;
                                            if (g_mipe_category_list_0068f11c != 0) {
                                                PLDestroy(g_mipe_category_list_0068f11c);
                                                g_mipe_category_list_0068f11c = 0;
                                            }
                                            return 1;
                                        }
                                        if (g_mipe_mode_0068f108 < 1) {
                                            g_mipe_state_0068f100->monster = 0;
                                            ToggleMipePanel0057D740();
                                            return 1;
                                        }
                                        g_mipe_mode_0068f108 = 0;
                                    }
                                } else {
                                    g_mipe_mode_0068f108 = 0xf;
                                    ResetEditorStatusLine0058AA20(-1);
                                    ShowNoticef(6, L"Choose an action:");
                                    ShowNoticef(0xf, L"1) Create cube.");
                                    ShowNoticef(0xf, L"2) Delete cube.");
                                    ShowNoticef(0xf, L"3) Edit cube parameters.");
                                    ShowNoticef(0xf, L"4) Move cube.");
                                    ShowNoticef(0xf, L"5) Scale cube.");
                                    ShowNoticef(0xf, L"6) Select cube.");
                                    g_flag_68f104 = 1;
                                }
                            }
                        } else {
                            ShowWorldCursor00490B10();
                            g_mipe_mode_0068f108 = 0;
                            g_mipe_state_0068f100->selecting = 0;
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
            if (g_mipe_mode_0068f108 != 6 && g_mipe_mode_0068f108 != 7 &&
                g_mipe_mode_0068f108 != 0xf && g_mipe_mode_0068f108 != 0x12) {
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
    if (g_monster_combat_timer_enabled_006f0531 != 0) {
        if (key == 0x56) {
            HideWorldCursor00490B90();
            g_mipe_mode_0068f108 = 0xf;
            g_flag_68f104 = 1;
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"Choose an action:");
            ShowNoticef(0xf, L"1) Create cube.");
            ShowNoticef(0xf, L"2) Delete cube.");
            ShowNoticef(0xf, L"3) Edit cube parameters.");
            ShowNoticef(0xf, L"4) Move cube.");
            ShowNoticef(0xf, L"5) Scale cube.");
            ShowNoticef(0xf, L"6) Select cube.");
            SetWorldCursorNodesVisible0048ED70(1);
            return handled;
        }
        if (key != 0x58) {
            return handled;
        }
        ToggleMipePanel0057D740();
        return handled;
    }
    switch (g_mipe_mode_0068f108) {
    case 0:
        switch (key) {
        case 0x30:
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"What would you like to do?");
            ShowNoticef(0xf, L"1) Create a monster.");
            ShowNoticef(0xf, L"2) Create an item.");
            ShowNoticef(0xf, L"3) Edit object(s).");
            ShowNoticef(0xf, L"4) Monster Generators.");
            ShowNoticef(0xf, L"5) Select object(s).");
            ShowNoticef(0xf, L"6) Handle triggers.");
            g_mipe_mode_0068f108 = 0;
            return handled;
        case 0x31:
            g_mipe_count_0068f10c = 0;
            ShowMipeMonsterStatus00577BF0();
            g_mipe_mode_0068f108 = 1;
            return handled;
        case 0x32:
            g_mipe_count_0068f10c = 0;
            ShowMipeItemStatus00577CB0();
            g_mipe_mode_0068f108 = 2;
            return handled;
        case 0x33:
            ShowMipeEditMenu00577E60();
            g_mipe_mode_0068f108 = 5;
            return handled;
        case 0x34:
            ShowMonsterGeneratorStatus005781F0();
            g_mipe_mode_0068f108 = 0x15;
            return handled;
        case 0x35:
            ShowMipeChooseMenu00577DE0();
            HideWorldCursor00490B90();
            g_mipe_state_0068f100->selecting = 1;
            g_mipe_mode_0068f108 = 9;
            return handled;
        case 0x36:
            ShowMipeTriggerMenu00578000();
            g_mipe_mode_0068f108 = 0xc;
            return handled;
        default:
            break;
        }
        break;
    case 1:
        return HandleMipeMonsterCreateKey00578500(key);
    case 2:
        return HandleMipeItemCreateKey00578D00(key);
    case 3:
        if (key == 0x35) {
            g_mipe_mode_0068f108 = 10;
            g_mipe_state_0068f100->waypoint_count = 0;
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"Type 'C' to create a waypoint.");
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(3, L"Laying down waypoint %d", g_mipe_state_0068f100->waypoint_count);
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf, L"Type X to delete last waypoint.");
            return handled;
        }
        break;
    case 4:
        if (key == 0x43) {
            g_mipe_choose_group_0064a1cc = g_mipe_choose_group_0064a1cc == 0;
            if (g_mipe_state_0068f100 != 0) {
                IListClear(&g_mipe_state_0068f100->monster_ids);
            }
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"MOVE IT!!");
            ShowNoticef(0xf, L"Type C to change how to choose.");
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf,
                        g_mipe_choose_group_0064a1cc == 0 ? L"Choosing: One" : L"Choosing: Group");
            return handled;
        }
        break;
    case 5:
        if (key == 0x31) {
            g_mipe_count_0068f10c = 0;
            g_mipe_mode_0068f108 = 8;
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"Edit Monster");
            ShowNoticef(0xf, L"1) Slow <<");
            ShowNoticef(0xf, L"2) Fast >>");
            ShowNoticef(0xf, L"3) Set Speed");
            ShowNoticef(0xf, L"4) Assign Path");
            ShowNoticef(0xf, L"5) Next Cycle");
            ShowNoticef(0xf, L"6) Direction");
            g_debug_monster_cycle_0068f0fc = 1;
            return handled;
        }
        if (key == 0x32) {
            g_mipe_count_0068f10c = 0;
            g_mipe_mode_0068f108 = 0xd;
            HideWorldCursor00490B90();
            ShowMipePropMenu00577EB0();
            return handled;
        }
        if (key == 0x34) {
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"MOVE IT!!");
            ShowNoticef(0xf, L"Type C to change how to choose.");
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf,
                        g_mipe_choose_group_0064a1cc == 0 ? L"Choosing: One" : L"Choosing: Group");
            g_mipe_mode_0068f108 = 4;
            g_mipe_state_0068f100->selecting = 1;
            HideWorldCursor00490B90();
            return handled;
        }
        break;
    case 6:
        HandleMipeMonsterCategoryKey00579300(key);
        return handled;
    case 7:
        HandleMipeItemCategoryKey00578850(key);
        return handled;
    case 8:
        HandleMonsterDebugKey00579900(key);
        return handled;
    case 9:
        if (key == 0x43) {
            g_mipe_choose_group_0064a1cc = g_mipe_choose_group_0064a1cc == 0;
            if (g_mipe_state_0068f100 != 0) {
                IListClear(&g_mipe_state_0068f100->monster_ids);
            }
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"Choose monster or item to edit.");
            ShowNoticef(0xf, L"Type C to change how to choose.");
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf, &g_wchar_00689b34);
            ShowNoticef(0xf,
                        g_mipe_choose_group_0064a1cc == 0 ? L"Choosing: One" : L"Choosing: Group");
            return handled;
        }
        break;
    case 10:
        HandleWaypointKey00579DF0(key);
        return handled;
    case 0xb:
        AdjustMonsterSpeed00579BF0(key);
        return handled;
    case 0xc:
        switch (key) {
        case 0x20:
            ShowMipeTriggerMenu00578000();
            return handled;
        case 0x35:
            g_value_0068f0fd = g_value_0068f0fd == 0;
            SetWorldCursorNodesVisible0048ED70(g_value_0068f0fd);
            ShowMipeTriggerMenu00578000();
            return handled;
        case 0x36:
            g_mipe_mode_0068f108 = 0xf;
            HideWorldCursor00490B90();
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"Choose an action:");
            ShowNoticef(0xf, L"1) Create cube.");
            ShowNoticef(0xf, L"2) Delete cube.");
            ShowNoticef(0xf, L"3) Edit cube parameters.");
            ShowNoticef(0xf, L"4) Move cube.");
            ShowNoticef(0xf, L"5) Scale cube.");
            ShowNoticef(0xf, L"6) Select cube.");
            g_flag_68f104 = 1;
            SetWorldCursorNodesVisible0048ED70(1);
            return handled;
        }
        break;
    case 0xd:
        HandleMipePropEditKey00579FF0(key);
        return handled;
    case 0xe:
        HandleMipeEditPropKey005C3880(key);
        return handled;
    case 0xf:
        HandleCubeMenuKey0057A310(key);
        return handled;
    case 0x10:
        HandleCubeParameterKey0057A630(key);
        return handled;
    case 0x12:
        HandleCubeScaleKey0057A800(key);
        return handled;
    case 0x14:
        if (key == 0x31) {
            ShowMonsterGeneratorStatus005781F0();
            g_mipe_mode_0068f108 = 0x15;
            return handled;
        }
        break;
    case 0x15:
        HandleMonsterGeneratorKey0057AA00(key);
        return handled;
    case 0x16:
        HandleMonsterGeneratorEditKey0057AD10(key);
        return handled;
    case 0x17:
        HandleMipeGeneratorTableKey0057B1A0(key);
        return handled;
    case 0x18:
        switch (key) {
        case 8:
            g_random_encounter_limit = g_random_encounter_limit / 10;
            break;
        default:
            return handled;
        case 0x20:
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"Enter the limit for random encounters:");
            ShowNoticef(0xf, g_format_d_0060aa20, g_random_encounter_limit);
            return handled;
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
        }
        ResetEditorStatusLine0058AA20(-1);
        ShowNoticef(6, L"Enter the limit for random encounters:");
        ShowNoticef(0xf, g_format_d_0060aa20, g_random_encounter_limit);
        return handled;
    case 0x19:
        EditMonsterGeneratorName0057B7E0(key);
        return handled;
    case 0x1a:
        switch (key) {
        case 8:
            g_encounter_culling_time_seconds = g_encounter_culling_time_seconds / 10;
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"Enter the encounter culling time (sec):");
            shown = g_encounter_culling_time_seconds;
            break;
        default:
            return handled;
        case 0x20:
            ResetEditorStatusLine0058AA20(-1);
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
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"Enter the encounter culling time (sec):");
            ShowNoticef(0xf, g_format_d_0060aa20, g_encounter_culling_time_seconds);
            return handled;
        }
        ShowNoticef(0xf, g_format_d_0060aa20, shown);
        return handled;
    case 0x1b:
        HandleMipeLockTrapKey0057B880(key);
        return handled;
    case 0x1c:
        EditTriggerKeyID0057BA60(key);
        return handled;
    case 0x1d:
        HandleMipeItemTableKey0057BBD0(key);
        return handled;
    }
    return handled;
}

// FUNCTION: WIZ8 0x0057dbb0
unsigned char GetFlag68F105(void)
{
    return g_flag_68f105;
}
// FUNCTION: WIZ8 0x0057dbc0
unsigned char GetFlag68F104(void)
{
    return g_flag_68f104;
}

/* Index of the `ordinal`-th item table in `category`, or the table count when
   fewer than `ordinal` tables match. */
// FUNCTION: WIZ8 0x0057dbd0
int FindCategoryItemTable0057DBD0(unsigned int category, int ordinal)
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
                found = found + 1;
            }
            index = index + 1;
            tables = tables + 1;
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
void UpdateMipeSelection0057DC20(void)
{
    POINT point;
    MonGen* picked;
    W8MonsterInfo* info;
    W8Item* marker;
    float best_distance;
    int group_id;
    int location_id;
    int index;

    if (g_mipe_state_0068f100->selecting == 0) {
        return;
    }
    SGPMouseGetPos(&point);
    if (g_mipe_mode_0068f108 == 0x15) {
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
        g_mipe_state_0068f100->generator = picked;
        ShowMonsterGeneratorStatus005781F0();
    }
    location_id = PickNearestMonsterUnderCursor005396D0(point.x, point.y);
    if (g_mipe_choose_group_0064a1cc != 0) {
        if (location_id == -1) {
            group_id = W8_MIPE_NO_GROUP;
        } else {
            group_id = MonsterGetScriptPartByLocationIndex(
                           MonsterGetIndexByLocationID(0x10bb, MIPE_CPP, location_id, 1))
                           ->monster_group_id;
        }
        if (group_id == g_mipe_state_0068f100->selected_group_id) {
            return;
        }
        for (index = 0; index < static_cast<int>(ILLength(&g_mipe_state_0068f100->monster_ids));
             ++index) {
            int listed = IListGetAt(&g_mipe_state_0068f100->monster_ids, index);
            info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x10cd, MIPE_CPP, listed, 1));
            if (info->fActive != 0) {
                SetMonsterHighlight(0, listed, 0);
            }
        }
        IListClear(&g_mipe_state_0068f100->monster_ids);
        for (index = 0; index < static_cast<int>(PLLength(gXStatus.plsMonsterList)); ++index) {
            info = static_cast<W8MonsterInfo*>(PLGet(gXStatus.plsMonsterList, index));
            if (info->fActive != 0 && info->monster_group_id == group_id) {
                SetMonsterHighlight(0, info->location_id, 1);
                IListAdd(&g_mipe_state_0068f100->monster_ids, info->location_id);
            }
        }
        if (ILLength(&g_mipe_state_0068f100->monster_ids) == 1) {
            info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x10e8, MIPE_CPP, location_id, 1));
            if (info == 0) {
                srAssertFail("pMonsterInfo", MIPE_CPP, 0x10ea, 0);
            }
            g_mipe_state_0068f100->monster = info->p3D;
        }
        g_mipe_state_0068f100->selected_group_id = group_id;
        SetWorldCursorGroupId004916A0(group_id);
        return;
    }
    if (location_id != -1 && IListIndexOf(&g_mipe_state_0068f100->monster_ids, location_id) != -1) {
        return;
    }
    if (ILLength(&g_mipe_state_0068f100->monster_ids) != 0) {
        int old_id = IListGetAt(&g_mipe_state_0068f100->monster_ids, 0);
        IListClear(&g_mipe_state_0068f100->monster_ids);
        SetMonsterHighlight(0, old_id, 0);
    }
    if (location_id == -1) {
        return;
    }
    SetMonsterHighlight(0, location_id, 1);
    IListAdd(&g_mipe_state_0068f100->monster_ids, location_id);
    info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0x110c, MIPE_CPP, location_id, 1));
    if (info == 0) {
        srAssertFail("pMonsterInfo", MIPE_CPP, 0x110e, 0);
    }
    g_mipe_state_0068f100->monster = info->p3D;
    SetWorldCursorGroupId004916A0(info->monster_group_id);
}

/* Drag the selected monsters, or the selected trigger, by the world cursor's
   delta from the drag anchor. */
// FUNCTION: WIZ8 0x0057df80
void DragSelectionWithCursor0057DF80(void)
{
    srVector3T<float> cursor;
    srVector3T<float> position;
    srVector3T<float> moved;
    W8MonsterInfo* info;
    int index;

    GetWorldCursorPosition00490BF0(&cursor);
    if (g_mipe_state_0068f100->trigger != 0) {
        g_mipe_state_0068f100->trigger->GetPosition(&position);
        moved.x = cursor.x - g_mipe_state_0068f100->drag_anchor.x + position.x;
        moved.y = cursor.y - g_mipe_state_0068f100->drag_anchor.y + position.y;
        moved.z = cursor.z - g_mipe_state_0068f100->drag_anchor.z + position.z;
        g_mipe_state_0068f100->trigger->SetPosition004416F0(&moved);
    } else {
        for (index = 0; index < static_cast<int>(ILLength(&g_mipe_state_0068f100->monster_ids));
             ++index) {
            info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x114c, MIPE_CPP, IListGetAt(&g_mipe_state_0068f100->monster_ids, index), 1));
            MonsterGetLocalLocation(info->p3D, &position);
            moved.x = cursor.x - g_mipe_state_0068f100->drag_anchor.x + position.x;
            moved.y = cursor.y - g_mipe_state_0068f100->drag_anchor.y + position.y;
            moved.z = cursor.z - g_mipe_state_0068f100->drag_anchor.z + position.z;
            info->p3D->SetPosition004A6DF0(&moved);
        }
    }
    g_mipe_state_0068f100->drag_anchor = cursor;
}

/* The world-view share of MIPE input: left-down starts a cube drag in mode 4
   or picks another cube in mode 0x13, left-up drops the drag and un-highlights
   the trigger's item, right-up leaves cube selection for the action menu, and
   mouse motion drags or tracks the cube. Returns whether it consumed the
   event. */
// FUNCTION: WIZ8 0x0057e0e0
unsigned char MipeWorldViewEvent0057E0E0(int event, const POINT* point)
{
    unsigned char result;

    result = 0;
    if (g_mipe_state_0068f100 == 0) {
        return 0;
    }
    switch (event & 0xffff) {
    case LEFT_BUTTON_DOWN:
        if (g_mipe_mode_0068f108 == 4) {
            result = 1;
            g_mipe_state_0068f100->dragging = 1;
            ShowWorldCursor00490B10();
            GetWorldCursorPosition00490BF0(&g_mipe_state_0068f100->drag_anchor);
            WarpSystemCursor(0x140, 0xf0);
            g_mipe_state_0068f100->trigger = 0;
        } else if (g_mipe_mode_0068f108 == 0x13) {
            if (g_mipe_cube_0068f12c != 0) {
                SetWorldCursorNodeColorComponents0048E420(g_mipe_cube_0068f12c, 0.0f, 0.0f, 0.5f);
                RefreshWorldCursorNodeLabel0048DCA0(g_mipe_cube_0068f12c);
            }
            g_mipe_cube_0068f12c = PickWorldCursorNodeAtScreenPoint0048E3E0(point->x, point->y);
            if (g_mipe_cube_0068f12c != 0) {
                SetWorldCursorNodeColorComponents0048E420(g_mipe_cube_0068f12c, 0.0f, 1.0f, 0.0f);
                RefreshWorldCursorNodeLabel0048DCA0(g_mipe_cube_0068f12c);
            }
            ResetEditorStatusLine0058AA20(-1);
            ShowNoticef(6, L"Select cube:");
            if (g_mipe_cube_0068f12c == 0) {
                ShowNoticef(0xf, L"Click on a cube to select it.");
            } else {
                ShowNoticef(0xf, L"Click on another cube to select ");
            }
        }
        break;
    case LEFT_BUTTON_UP:
        if (g_mipe_mode_0068f108 == 4) {
            DragSelectionWithCursor0057DF80();
            result = 1;
            g_mipe_state_0068f100->dragging = 0;
            HideWorldCursor00490B90();
            if (g_mipe_state_0068f100->trigger != 0) {
                Trigger* trigger = g_mipe_state_0068f100->trigger;
                W8Item* item;

                trigger->flags_0a0 &= ~0x20u;
                item = trigger->rep_item_114;
                if ((trigger->flags_0a0 & W8_TRIGGER_ON) != 0 && item != 0) {
                    static_cast<W8ItemRep*>(item->m_pRep)->SetFlags(0x10, 0);
                    item->SetHighlight(0);
                }
                g_mipe_state_0068f100->trigger = 0;
            }
        }
        break;
    case RIGHT_BUTTON_UP:
        if (g_mipe_mode_0068f108 == 0x11) {
            g_mipe_mode_0068f108 = 0xf;
            g_mipe_state_0068f100->selecting = 0;
            HideWorldCursor00490B90();
            g_mipe_state_0068f100->dragging = 0;
            g_flag_68f104 = 1;
            ResetEditorStatusLine0058AA20(-1);
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
        if (g_mipe_state_0068f100->dragging != 0) {
            if (g_mipe_mode_0068f108 == 4) {
                DragSelectionWithCursor0057DF80();
                result = 1;
            } else if (g_mipe_mode_0068f108 == 0x11) {
                result = 1;
                if (g_mipe_cube_0068f12c != 0) {
                    srVector3T<float> position;

                    GetWorldCursorAnchor00490C20(&position);
                    MoveWorldCursorNode0048DBF0(g_mipe_cube_0068f12c, &position);
                }
            } else if (g_mipe_mode_0068f108 == 0x12) {
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
static int g_last_reachable_mongen_marker_0064a1e0 = -1;

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
    if (0 <= g_last_reachable_mongen_marker_0064a1e0 &&
        g_last_reachable_mongen_marker_0064a1e0 < count) {
        generator = GetMonsterGenerator(g_last_reachable_mongen_marker_0064a1e0);
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
            g_last_reachable_mongen_marker_0064a1e0 = index;
            return 1;
        }
    }
    return 0;
}
