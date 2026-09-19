#include "wiz8/local_screens/mipe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "input.h"

#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/engine_code/3d.h"
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
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_code/MonsterGenerator.h"
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

// GLOBAL: WIZ8 0x006850ba
int g_mipe_cube_serial_006850ba;

// GLOBAL: WIZ8 0x0068f0fe
unsigned char g_mipe_mongen_visible_0068f0fe;

// GLOBAL: WIZ8 0x0068f108
int g_mipe_mode_0068f108;

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
            Function58F6B0(1);
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
    Function58F6B0(0);
    ResetEditorStatusLine0058AA20(-1);
    g_flag_68f105 = 1;
    ResetEditorStatusLine0058AA20(-1);
    WriteGameLogAmount(6, L"What would you like to do?", 0);
    WriteGameLogAmount(15, L"1) Create a monster.", 0);
    WriteGameLogAmount(15, L"2) Create an item.", 0);
    WriteGameLogAmount(15, L"3) Edit object(s).", 0);
    WriteGameLogAmount(15, L"4) Monster Generators.", 0);
    WriteGameLogAmount(15, L"5) Select object(s).", 0);
    WriteGameLogAmount(15, L"6) Handle triggers.", 0);
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
    g_mipe_state_0068f100->monster_ids.capacity = 1000000;
    g_mipe_state_0068f100->selecting = 0;
    g_mipe_state_0068f100->value_34 = 1.0f;
    g_mipe_state_0068f100->speed_step = 0.020000000f;
    g_mipe_state_0068f100->flag_61 = 0xff;
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

// GLOBAL: WIZ8 0x0068f130
int g_mipe_cube_param_0068f130;

// GLOBAL: WIZ8 0x0068f134
int g_mipe_scale_plane_0068f134;

/* The status line block the ','/'.' and 'k'/'l' handlers repaint. */
// FUNCTION: WIZ8 0x00577f10
void ShowMonsterSpeedStatus00577F10(void)
{
    W8PathAI* path;
    float speed;

    ResetEditorStatusLine0058AA20(-1);
    WriteGameLogAmount(6, L"Type ',' to decrease speed, '.' to increase.");
    WriteGameLogAmount(6, L"Type 'k' to decrease increment, 'l' to increase.");
    WriteGameLogAmount(0xf, &g_wchar_00689b34);
    WriteGameLogAmount(0xf, L"Increment: %g", (double)g_mipe_state_0068f100->speed_step);
    WriteGameLogAmount(0xf, &g_wchar_00689b34);
    WriteGameLogAmount(0xf, &g_wchar_00689b34);
    if (g_mipe_state_0068f100->monster == 0) {
        WriteGameLogAmount(8, L"No monster available.");
        return;
    }
    path = (W8PathAI*)MonsterGetObject0C(g_mipe_state_0068f100->monster);
    if (path != 0 && PathAIRecordFlag004A9740(path) == 0) {
        speed = PathAIGetScale004AAA50(path);
    } else {
        if (g_mipe_state_0068f100->monster == 0) {
            WriteGameLogAmount(8, L"Monster has no path AI.");
            return;
        }
        speed = MonsterGetNavigatorValue120(g_mipe_state_0068f100->monster);
    }
    WriteGameLogAmount(0xf, L"Current speed: %g", (double)speed);
}

/* The "Parameters" pane for the selected volume cube. */
// FUNCTION: WIZ8 0x005780f0
void ShowCubeParameters005780F0(void)
{
    ResetEditorStatusLine0058AA20(-1);
    if (g_mipe_cube_0068f12c == 0) {
        WriteGameLogAmount(6, L"No cube selected.");
        return;
    }
    WriteGameLogAmount(6, L"Parameters:");
    if (g_mipe_cube_param_0068f130 == 0) {
        WriteGameLogAmount(0, L"Message: %d",
                           GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c, 0));
    } else {
        WriteGameLogAmount(0xf, L"Message: %d",
                           GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c, 0));
    }
    if (g_mipe_cube_param_0068f130 == 1) {
        WriteGameLogAmount(0, L"Search: %d",
                           GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c, 1));
    } else {
        WriteGameLogAmount(0xf, L"Search: %d",
                           GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c, 1));
    }
    if (g_mipe_cube_param_0068f130 == 2) {
        WriteGameLogAmount(0, L"Function: %d",
                           GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c, 2));
        return;
    }
    WriteGameLogAmount(0xf, L"Function: %d",
                       GetWorldCursorNodeParameter0048E2B0(g_mipe_cube_0068f12c, 2));
}

/* The monster generator top menu plus its selection status line. */
// FUNCTION: WIZ8 0x005781f0
void ShowMonsterGeneratorStatus005781F0(void)
{
    const char* state;

    ResetEditorStatusLine0058AA20(-1);
    WriteGameLogAmount(6, L"Monster Generators (%d)", GetMonsterGeneratorCount());
    WriteGameLogAmount(0xf, L"1) Create 2) Delete");
    WriteGameLogAmount(0xf, L"3) Edit   4) Select");
    state = "On";
    if (g_mipe_mongen_visible_0068f0fe == 0) {
        state = "Off";
    }
    WriteGameLogAmount(0xf, L"5) Toggle Display [%s]", state);
    state = "Off";
    if (g_generator_save_flag == 0) {
        state = "On";
    }
    WriteGameLogAmount(0xf, L"6) Toggle Active  [%s]", state);
    WriteGameLogAmount(0xf, &g_wchar_00689b34);
    if (g_mipe_state_0068f100->generator != 0) {
        if (g_mipe_state_0068f100->selecting != 0) {
            WriteGameLogAmount(3, L"<--- MOUSE OVER MONGEN --->");
            return;
        }
        WriteGameLogAmount(8, L"MONGEN selected");
        return;
    }
    if (g_mipe_state_0068f100->selecting != 0) {
        WriteGameLogAmount(3, L"---> SELECTING MONGEN <---");
        return;
    }
    WriteGameLogAmount(0xf, &g_wchar_00689b34);
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
    WriteGameLogAmount(6, L"Edit Monster Generator");
    WriteGameLogAmount(0xf, L"1) Name: %hs", g_mipe_state_0068f100->generator->name);
    if (g_mipe_state_0068f100->generator->encounter_table_index == -1) {
        WriteGameLogAmount(0xf, L"2) Table: Not Selected");
    } else {
        table = GetEncounterTable(g_mipe_state_0068f100->generator->encounter_table_index);
        WriteGameLogAmount(0xf, L"2) Table: %hs", table->name);
    }
    state = "On";
    if (g_mipe_state_0068f100->generator->generation_enabled == 0) {
        state = "Off";
    }
    WriteGameLogAmount(0xf, L"3) Toggle Active [%s]", state);
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
    WriteGameLogAmount(color, format, chance, (int)interval);
    WriteGameLogAmount(0xf, L"8) to toggle chance default");
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
            if (info == 0 || info->monster == 0) {
                srAssertFail("pMonsterInfo && pMonsterInfo->p3D", MIPE_CPP, 0x6e5, 0);
            }
            monster = info->monster;
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
            monster = info->monster;
            if (monster == 0) {
                srAssertFail("pMonster", MIPE_CPP, 0x707, 0);
            }
            cycle = monster->m_pRep->current_subcycle;
            if ((char)monster->m_pRep->GetNumSubsPerCycle(-1) - 1 <= cycle) {
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
            monster->m_pRep->value_066 = 0;
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
    path = (W8PathAI*)MonsterGetObject0C(g_mipe_state_0068f100->monster);
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
        if (g_mipe_state_0068f100->speed_step < (float)g_double_005ec8d0) {
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
        WriteGameLogAmount(6, L"Type 'C' to create a waypoint.");
        WriteGameLogAmount(0xf, &g_wchar_00689b34);
        WriteGameLogAmount(3, L"Laying down waypoint %d", g_mipe_state_0068f100->waypoint_count);
        WriteGameLogAmount(0xf, &g_wchar_00689b34);
        WriteGameLogAmount(0xf, &g_wchar_00689b34);
        WriteGameLogAmount(0xf, &g_wchar_00689b34);
        WriteGameLogAmount(0xf, L"Type X to delete last waypoint.");
    } else if (key == 0x58) {
        if (g_mipe_state_0068f100 != 0) {
            unsigned int count = PLLength(&g_mipe_state_0068f100->waypoints);
            if (count != 0) {
                monster = (W8Monster*)PLGet(&g_mipe_state_0068f100->waypoints, count - 1);
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
        WriteGameLogAmount(6, L"Type 'C' to create a waypoint.");
        WriteGameLogAmount(0xf, &g_wchar_00689b34);
        WriteGameLogAmount(3, L"Laying down waypoint %d", g_mipe_state_0068f100->waypoint_count);
        WriteGameLogAmount(0xf, &g_wchar_00689b34);
        WriteGameLogAmount(0xf, &g_wchar_00689b34);
        WriteGameLogAmount(0xf, &g_wchar_00689b34);
        WriteGameLogAmount(0xf, L"Type X to delete last waypoint.");
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
        WriteGameLogAmount(6, L"Choose an action:");
        WriteGameLogAmount(0xf, L"1) Create cube.");
        WriteGameLogAmount(0xf, L"2) Delete cube.");
        WriteGameLogAmount(0xf, L"3) Edit cube parameters.");
        WriteGameLogAmount(0xf, L"4) Move cube.");
        WriteGameLogAmount(0xf, L"5) Scale cube.");
        WriteGameLogAmount(0xf, L"6) Select cube.");
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
        serial = g_mipe_cube_serial_006850ba;
        g_mipe_cube_serial_006850ba = g_mipe_cube_serial_006850ba + 1;
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
        WriteGameLogAmount(6, L"Move Volume Trigger.");
        if (g_mipe_cube_0068f12c == 0) {
            WriteGameLogAmount(0xf, L"Click on trigger to move.");
        } else {
            WriteGameLogAmount(0xf, L"Move trigger. Hold down SHIFT to");
            WriteGameLogAmount(0xf, L"change elevation.");
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
        WriteGameLogAmount(6, L"Scale Volume Trigger.");
        if (g_mipe_cube_0068f12c != 0) {
            WriteGameLogAmount(0xf, L"Scaling in the %c plane. ",
                               (int)scale_planes[g_mipe_scale_plane_0068f134]);
            WriteGameLogAmount(0xf, L"Press X/Y/Z to change plane.");
            g_flag_68f104 = 0;
            return 1;
        }
        prompt = L"Click on trigger to scale.";
        break;
    case 0x36:
        g_mipe_mode_0068f108 = 0x13;
        ResetEditorStatusLine0058AA20(-1);
        WriteGameLogAmount(6, L"Select cube:");
        if (g_mipe_cube_0068f12c == 0) {
            prompt = L"Click on a cube to select it.";
        } else {
            prompt = L"Click on another cube to select it.";
        }
        break;
    }
    WriteGameLogAmount(0xf, prompt);
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
        WriteGameLogAmount(6, L"Choose an action:");
        WriteGameLogAmount(0xf, L"1) Create cube.");
        WriteGameLogAmount(0xf, L"2) Delete cube.");
        WriteGameLogAmount(0xf, L"3) Edit cube parameters.");
        WriteGameLogAmount(0xf, L"4) Move cube.");
        WriteGameLogAmount(0xf, L"5) Scale cube.");
        WriteGameLogAmount(0xf, L"6) Select cube.");
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
        WriteGameLogAmount(6, L"Scale Volume Trigger.");
        if (g_mipe_cube_0068f12c == 0) {
            WriteGameLogAmount(0xf, L"Click on trigger to scale.");
            return 1;
        }
        WriteGameLogAmount(0xf, L"Scaling in the %c plane. ",
                           (int)scale_planes[g_mipe_scale_plane_0068f134]);
        WriteGameLogAmount(0xf, L"Press X/Y/Z to change plane.");
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
        sprintf(name, "MonGen%3.3d", g_saved_encounter_budget++);
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
                    if (table->unknown_150 == (unsigned int)g_mipe_category_0068f114) {
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
        WriteGameLogAmount(6, L"Enter the name for this generator:");
        WriteGameLogAmount(0xf, L"%S", g_mipe_state_0068f100->generator->name);
        return 1;
    case 0x32:
        current_index = g_mipe_state_0068f100->generator->encounter_table_index;
        if (current_index < 0) {
            g_mipe_table_base_0068f120 = 0;
            g_mipe_table_row_0068f118 = 0;
        } else {
            table = GetEncounterTable(current_index);
            list = g_mipe_category_list_0068f11c;
            g_mipe_category_0068f114 = (unsigned char)table->unknown_150;
            if (g_mipe_category_list_0068f11c != 0) {
                PListClear(g_mipe_category_list_0068f11c);
                count = g_encounter_tables.count;
                index = 0;
                if (0 < g_encounter_tables.count) {
                    do {
                        table = GetEncounterTable(index);
                        if (table->unknown_150 == (unsigned int)g_mipe_category_0068f114) {
                            PLAdoptAppend(list, table);
                        }
                        ++index;
                    } while (index < count);
                }
            }
            found = 0;
            if (0 < static_cast<int>(PLLength(g_mipe_category_list_0068f11c))) {
                do {
                    table = (W8EncounterTableRuntime*)PLGet(g_mipe_category_list_0068f11c, found);
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
        WriteGameLogAmount(6, L"Category: %S",
                           *g_encounter_names.GetAt(g_mipe_category_0068f114 & 0xff));
        if (g_mipe_category_list_0068f11c == 0) {
            return 1;
        }
        slot = 0;
        do {
            entry = (W8EncounterTableRuntime*)PLGet(g_mipe_category_list_0068f11c,
                                                    g_mipe_table_base_0068f120 + slot);
            if (entry == 0) {
                WriteGameLogAmount(0xf, &g_wchar_00689b34);
            } else {
                WriteGameLogAmount(slot == g_mipe_table_row_0068f118 ? 3 : 0xf, L"    %S",
                                   entry->name);
            }
            ++slot;
        } while (slot < 6);
        return 1;
    case 0x33:
        g_mipe_state_0068f100->generator->generation_enabled = g_mipe_state_0068f100->generator->generation_enabled == 0;
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
        } else if ('\0' < (char)g_generator_interval_min) {
            g_generator_interval_min = (short)(char)((char)g_generator_interval_min - 10);
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
        } else if ((char)g_generator_interval_min < 'd') {
            g_generator_interval_min = (short)(char)((char)g_generator_interval_min + '\n');
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
        name[length] = (char)key;
        name[length + 1] = '\0';
    }
    ResetEditorStatusLine0058AA20(-1);
    WriteGameLogAmount(6, L"Enter the name for this generator:");
    WriteGameLogAmount(0xf, L"%S", g_mipe_state_0068f100->generator->name);
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
        action->flags_008 = (pending << 2) | (action->flags_008 & 0xfb);
        action->item_00a = (short)trigger->value_380;
    }
    trigger = g_mipe_state_0068f100->prop->GetValue18();
    ResetEditorStatusLine0058AA20(-1);
    WriteGameLogAmount(6, L"Enter Key ID:");
    WriteGameLogAmount(0xf, g_format_d_0060aa20, trigger->value_380);
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
    if (0 < (int)gXStatus.uiItemTablesInDatabase) {
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
        } while (index < (int)gXStatus.uiItemTablesInDatabase);
    }
    return index;
}

/* 0x0064A1CC: 'C' toggles between single-monster ("Choosing: One") and
   whole-group ("Choosing: Group") selection in UpdateMipeSelection0057DC20. */
// GLOBAL: WIZ8 0x0064a1cc
static unsigned char g_mipe_choose_group_0064a1cc;

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
            group_id = 1000000;
        } else {
            group_id = MonsterGetScriptPartByLocationIndex(
                           MonsterGetIndexByLocationID(0x10bb, MIPE_CPP, location_id, 1))
                           ->monster_group_id;
        }
        if (group_id == g_mipe_state_0068f100->value_0c) {
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
        for (index = 0; index < static_cast<int>(ILLength(reinterpret_cast<W8IList*>(
                                    gXStatus.plsMonsterList))); // reinterpret-ok: retail spells the
             // IList length on the W8PList (0x5e2c70)
             ++index) {
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
            g_mipe_state_0068f100->monster = info->monster;
        }
        g_mipe_state_0068f100->value_0c = group_id;
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
    g_mipe_state_0068f100->monster = info->monster;
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
        for (index = 0; index < (int)ILLength(&g_mipe_state_0068f100->monster_ids); ++index) {
            info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x114c, MIPE_CPP, IListGetAt(&g_mipe_state_0068f100->monster_ids, index), 1));
            MonsterGetLocalLocation(info->monster, &position);
            moved.x = cursor.x - g_mipe_state_0068f100->drag_anchor.x + position.x;
            moved.y = cursor.y - g_mipe_state_0068f100->drag_anchor.y + position.y;
            moved.z = cursor.z - g_mipe_state_0068f100->drag_anchor.z + position.z;
            info->monster->SetPosition004A6DF0(&moved);
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
            g_mipe_cube_0068f12c = Function48E3E0(point->x, point->y);
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
                if ((trigger->flags_0a0 & 0x10) != 0 && item != 0) {
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
        if (marker != 0 && IsWorldItemWithinReach(marker, &camera.x, 250000.0f)) {
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
        if (marker != 0 && IsWorldItemWithinReach(marker, &camera.x, 250000.0f)) {
            g_last_reachable_mongen_marker_0064a1e0 = index;
            return 1;
        }
    }
    return 0;
}
