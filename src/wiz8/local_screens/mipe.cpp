#include "wiz8/local_screens/mipe.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/sr_api.h"
#include "wiz8/world_cursor.h"
#include "wiz8/xstatus.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* Local Screens\mipe.cpp. The MIPE state is a diagnostic monster/item
   selection panel used while the debug flag is active. */

// GLOBAL: WIZ8 0x0068f108
int g_mipe_selection_0068f108;

// GLOBAL: WIZ8 0x0068f110
short g_mipe_item_index_0068f110;

// GLOBAL: WIZ8 0x0068f112
short g_mipe_monster_index_0068f112;

// GLOBAL: WIZ8 0x0068f114
unsigned char g_mipe_kind_0068f114;

// GLOBAL: WIZ8 0x0068f118
int g_mipe_monster_offset_0068f118;

// GLOBAL: WIZ8 0x0068f120
int g_mipe_item_offset_0068f120;

// GLOBAL: WIZ8 0x0068f124
W8PList* g_mipe_monster_entries_0068f124;

// GLOBAL: WIZ8 0x0068f12c
W8WorldCursorNode0048DB30* g_mipe_cursor_node_0068f12c;

static short MonsterRecordSelectionValue(const W8MonsterRecord* record)
{
    return *reinterpret_cast<const short*>(
        reinterpret_cast<const unsigned char*>(record) +
        0x1c1); // reinterpret-ok: retail record field is unmodeled
}

// FUNCTION: WIZ8 0x0057d740
void Function57D740(void)
{
    if (g_flag_68f105 != 0) {
        g_level_block->flag_271 = 1;
        g_level_block->flag_272 = 0;
        Function58AA20(-1);
        if (gXStatus.fCombatMode != 0) {
            Function58F6B0(1);
        }
        ReleaseWorldCursor004909C0();
        g_flag_68f105 = 0;
        g_flag_68f104 = 1;
        g_debug_monster_cycle_0068f0fc = 0;
        if (g_mipe_cursor_node_0068f12c != 0) {
            Function48E420(g_mipe_cursor_node_0068f12c, 0, 0, 0.5f);
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
        IListFreeData(&g_debug_monster_ids_0068f100->monster_ids);
        PListFreeData(&g_debug_monster_ids_0068f100->item_entries);
        free(g_debug_monster_ids_0068f100);
        g_debug_monster_ids_0068f100 = 0;
        SetWorldCursorNodesVisible0048ED70(g_value_0068f0fd);
        return;
    }

    g_level_block->flag_271 = 0;
    g_level_block->flag_272 = 1;
    Function58F6B0(0);
    Function58AA20(-1);
    g_flag_68f105 = 1;
    Function58AA20(-1);
    Function58AAD0(6, L"What would you like to do?");
    Function58AAD0(15, L"1) Create a monster.");
    Function58AAD0(15, L"2) Create an item.");
    Function58AAD0(15, L"3) Edit object(s).");
    Function58AAD0(15, L"4) Monster Generators.");
    Function58AAD0(15, L"5) Select object(s).");
    Function58AAD0(15, L"6) Handle triggers.");
    g_mipe_selection_0068f108 = 0;
    Function490210();
    g_flag_68f105 = 1;
    g_flag_68f104 = 0;
    g_mipe_cursor_node_0068f12c = 0;
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
        entry->selectable =
            records[index].deleted == 0 && MonsterRecordSelectionValue(&records[index]) == -1;
        PLAdoptAppend(g_mipe_monster_entries_0068f124, entry);
    }
    free(records);

    g_debug_monster_ids_0068f100 = static_cast<W8MipeState*>(malloc(sizeof(W8MipeState)));
    memset(g_debug_monster_ids_0068f100, 0, sizeof(W8MipeState));
    IListInit(&g_debug_monster_ids_0068f100->monster_ids);
    g_debug_monster_ids_0068f100->monster_ids.capacity = 1000000;
    g_debug_monster_ids_0068f100->unknown_0c[4] = 0;
    *reinterpret_cast<float*>(reinterpret_cast<unsigned char*>(g_debug_monster_ids_0068f100) +
                              0x34) =
        1.0f; // reinterpret-ok: retail diagnostic state field is unmodeled
    *reinterpret_cast<float*>(reinterpret_cast<unsigned char*>(g_debug_monster_ids_0068f100) +
                              0x4c) =
        0.020000000f; // reinterpret-ok: retail diagnostic state field is unmodeled
    *reinterpret_cast<unsigned char*>(
        reinterpret_cast<unsigned char*>(g_debug_monster_ids_0068f100) + 0x61) =
        0xff; // reinterpret-ok: retail diagnostic state field is unmodeled
    PListInit(&g_debug_monster_ids_0068f100->item_entries);

    for (int cursor_index = 0, count = GetWorldCursorNodeCount0048ED00(); cursor_index < count;
         ++cursor_index) {
        W8WorldCursorNode0048DB30* node = Function48ED10(cursor_index);
        Function48E420(node, 0, 0, 0.5f);
        Function48DCA0(node);
    }

    int selection = g_mipe_monster_offset_0068f118 + g_mipe_item_offset_0068f120;
    unsigned int monster_index = 0;
    int visible = 0;
    while (monster_index < gXStatus.uiMonstersInDatabase) {
        W8MipeMonsterEntry* entry =
            static_cast<W8MipeMonsterEntry*>(PLGet(g_mipe_monster_entries_0068f124, monster_index));
        if (entry->kind == g_mipe_kind_0068f114 && entry->selectable != 0) {
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
        g_mipe_monster_index_0068f112 = MonsterRecordSelectionValue(&record);
    }

    unsigned int item_index = 0;
    visible = 0;
    while (item_index < gXStatus.uiItemsInDatabase) {
        if (g_item_records[item_index].equip_class == g_mipe_kind_0068f114 &&
            g_item_records[item_index].unknown_0cb[0] == 0) {
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
