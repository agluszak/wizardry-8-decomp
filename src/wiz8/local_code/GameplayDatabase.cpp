#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/engine_code/Quality.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/item_tables.h"
#include "wiz8/item_spawning.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/utility.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include "random.h"
#include "timer.h"
#include "wiz8/local_code/character_events.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 0x0054B300 resets one of eight slots. */
/* The gStatus object owned by GameplayDatabase.cpp. */
// GLOBAL: WIZ8 0x00685170
W8GlobalStatus g_status;
/* Packed gXStatus named by the database and manager assertions. Record
   arrays remain separate roots at their own addresses. */
// GLOBAL: WIZ8 0x006836B8
W8XStatus gXStatus;
/* Persistent database roots owned by this translation unit.  Leaving these as
   unresolved externals made the runnable image relocate every load/store to
   the PE image base; the first four-byte count read consequently targeted a
   read-only header instead of game state. */
// GLOBAL: WIZ8 0x006836AC
W8FactDatabaseRecord* g_fact_records;
// GLOBAL: WIZ8 0x0068516C
W8ItemDatabaseRecord* g_item_records;
/* The six item stacks granted by the new-game status reset. */
// GLOBAL: WIZ8 0x006164DC
unsigned int g_starting_item_ids[6] = {0x14f, 0x14f, 0x15a, 0x15a, 0x15b, 0x155};
// GLOBAL: WIZ8 0x006836A4
W8LevelDatabaseRecord* g_level_records;
// GLOBAL: WIZ8 0x006836A0
W8NpcDatabaseRecord* g_npc_records;
// GLOBAL: WIZ8 0x006836B0
W8ItemTableRecord** g_item_tables;
// GLOBAL: WIZ8 0x006836B4
char** g_item_table_category_names;
// GLOBAL: WIZ8 0x0065BE1C
W8SpellRuntimeRecord* g_spell_records;
// GLOBAL: WIZ8 0x0065BE18
unsigned int g_spell_database_version;
#define GAMEPLAY_DATABASE_CPP "C:\\Projects\\Wizardry 8\\Local Code\\GameplayDatabase.cpp"

// FUNCTION: WIZ8 0x0054a400
unsigned char InitializeItemDatabase(void)
{
    char path[60];
    unsigned int index;
    unsigned int transferred;
    int handle;

    sprintf(path, "%s\\%s.%s", "Data\\Databases", "Items", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!FileRead(handle, &gXStatus.uiItemsInDatabase, 4, &transferred)) {
        FileClose(handle);
        return 0;
    }
    g_item_records = (W8ItemDatabaseRecord*)malloc(gXStatus.uiItemsInDatabase * 0x10d);
    if (!g_item_records) {
        return 0;
    }
    for (index = 0; index < gXStatus.uiItemsInDatabase; ++index) {
        if (!FileRead(handle, &g_item_records[index], 0x10d, &transferred)) {
            FileClose(handle);
            return 0;
        }
    }
    FileClose(handle);
    return 1;
}

/* ItemTables.DBS carries two arrays: category names, each a fixed 0x100-byte
   buffer, then the tables themselves. Both are arrays of pointers, cleared
   before use. The category reads are unchecked in the original while the table
   reads are not, and the per-table allocation is cleared before its own null
   check rather than after; both are reproduced. */
// FUNCTION: WIZ8 0x0054a510
unsigned char InitializeItemTables(void)
{
    char path[60];
    unsigned int index;
    unsigned int transferred;
    int handle;

    sprintf(path, "%s\\%s.%s", "Data\\Databases", "ItemTables", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!FileRead(handle, &gXStatus.uiItemTableCategories, 4, &transferred)) {
        FileClose(handle);
        return 0;
    }
    if (gXStatus.uiItemTableCategories) {
        g_item_table_category_names = (char**)malloc(gXStatus.uiItemTableCategories * 4);
        if (!g_item_table_category_names) {
            return 0;
        }
        memset(g_item_table_category_names, 0, gXStatus.uiItemTableCategories * 4);
        for (index = 0; index < gXStatus.uiItemTableCategories; ++index) {
            g_item_table_category_names[index] = (char*)malloc(0x100);
            FileRead(handle, g_item_table_category_names[index], 0x100, &transferred);
        }
    }
    if (!FileRead(handle, &gXStatus.uiItemTablesInDatabase, 4, &transferred)) {
        FileClose(handle);
        return 0;
    }
    if (gXStatus.uiItemTablesInDatabase) {
        g_item_tables = (W8ItemTableRecord**)malloc(gXStatus.uiItemTablesInDatabase * 4);
        if (!g_item_tables) {
            return 0;
        }
        memset(g_item_tables, 0, gXStatus.uiItemTablesInDatabase * 4);
        for (index = 0; index < gXStatus.uiItemTablesInDatabase; ++index) {
            g_item_tables[index] = (W8ItemTableRecord*)malloc(0x1f1);
            memset(g_item_tables[index], 0, 0x1f1);
            if (!g_item_tables[index]) {
                return 0;
            }
            if (!FileRead(handle, g_item_tables[index]->name, 0x1f1, &transferred)) {
                FileClose(handle);
                return 0;
            }
        }
    }
    FileClose(handle);
    return 1;
}

/* Seeks straight to one record rather than holding the file open, and strips the
   four name fields afterwards. The failed seek leaves the handle open where
   every other failure closes it, as elsewhere in this unit. The bytes-read
   out-parameter is the index's own incoming slot, dead once it has been copied
   into a register. */
// FUNCTION: WIZ8 0x0054a8a0
unsigned char LoadMonsterDatabaseRecord(unsigned int uiMonsterIndex, W8MonsterRecord* record)
{
    char path[60];
    unsigned int index = uiMonsterIndex;
    int handle;

    if (!(index < gXStatus.uiMonstersInDatabase)) {
        srAssertFail("uiMonsterIndex < gXStatus.uiMonstersInDatabase", GAMEPLAY_DATABASE_CPP, 0x140,
                     0);
    }
    sprintf(path, "%s\\%s.%s", "Data\\Databases", "Monsters", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!FileSeek(handle, index * 0x297 + 4, 1)) {
        return 0;
    }
    if (!FileRead(handle, record, 0x297, &uiMonsterIndex)) {
        FileClose(handle);
        return 0;
    }
    FileClose(handle);
    StripMonsterNameSuffix(record->name_00);
    StripMonsterNameSuffix(record->name_30);
    StripMonsterNameSuffix(record->name_60);
    StripMonsterNameSuffix(record->name_90);
    return 1;
}

/* Unlike its fact and level siblings this one guards the free and then leaves
   the pointer dangling rather than clearing it. Both halves of that asymmetry
   are the original's. */
// FUNCTION: WIZ8 0x0054a4f0
void DestroyItemDatabase(void)
{
    if (g_item_records) {
        free(g_item_records);
    }
}

/* A generic guarded free, called from three unrelated subsystems, so it is named
   for what it does rather than for any one database. */
// FUNCTION: WIZ8 0x0054a880
void FreeIfNotNull(void* block)
{
    if (block) {
        free(block);
    }
}

/* The counterpart to InitializeItemTables: the category names first, then the
   tables, each entry freed before its array. Both arrays are re-read after
   every free because nothing tells VC6 that free leaves them alone. */
// FUNCTION: WIZ8 0x0054a6e0
void DestroyItemTables(void)
{
    unsigned int index;

    if (g_item_table_category_names) {
        for (index = 0; index < gXStatus.uiItemTableCategories; ++index) {
            if (g_item_table_category_names[index]) {
                free(g_item_table_category_names[index]);
            }
        }
        free(g_item_table_category_names);
    }
    if (g_item_tables) {
        for (index = 0; index < gXStatus.uiItemTablesInDatabase; ++index) {
            if (g_item_tables[index]) {
                free(g_item_tables[index]);
            }
        }
        free(g_item_tables);
    }
}

/* Reads MONSTERS.DBS whole: the count into gXStatus, then - only when the
   caller wants them - every record into one allocation handed back through the
   out-parameter. InitializeGame calls it with null just to publish the count. */
// FUNCTION: WIZ8 0x0054a760
unsigned char LoadMonsterDatabase(W8MonsterRecord** records)
{
    char path[60];
    unsigned int transferred;
    unsigned int index;
    W8MonsterRecord* block;
    int handle;

    sprintf(path, "%s\\%s.%s", "Data\\Databases", "Monsters", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!FileRead(handle, &gXStatus.uiMonstersInDatabase, 4, &transferred)) {
        FileClose(handle);
        return 0;
    }
    if (records) {
        block = static_cast<W8MonsterRecord*>(
            malloc(gXStatus.uiMonstersInDatabase * sizeof(W8MonsterRecord)));
        if (!block) {
            return 0;
        }
        for (index = 0; index < gXStatus.uiMonstersInDatabase; ++index) {
            if (!FileRead(handle, &block[index], sizeof(W8MonsterRecord), &transferred)) {
                FileClose(handle);
                free(block);
                return 0;
            }
        }
        *records = block;
    }
    FileClose(handle);
    return 1;
}

/* The range sibling of LoadMonsterDatabaseRecord, named by its own assertion at
   GameplayDatabase.cpp line 378. It seeks to the first record and reads the
   whole inclusive span in one call, computing the length as two separate record
   offsets subtracted rather than from a record count. The bytes-read
   out-parameter is uiEndIndex's own slot, dead once copied into a register, and
   a failed seek leaves the handle open where every other failure closes it. */
// FUNCTION: WIZ8 0x0054a9a0
unsigned char LoadMonsterDatabaseRange(unsigned int uiStartIndex, unsigned int uiEndIndex,
                                       unsigned int unused, W8MonsterRecord* records)
{
    char path[56];
    int handle;

    if (!(uiEndIndex < gXStatus.uiMonstersInDatabase)) {
        srAssertFail("uiEndIndex < gXStatus.uiMonstersInDatabase", GAMEPLAY_DATABASE_CPP, 0x17a, 0);
    }
    sprintf(path, "%s\\%s.%s", "Data\\Databases", "Monsters", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!FileSeek(handle, uiStartIndex * 0x297 + 4, 1)) {
        return 0; /* retail: failed seek leaves the handle open */
    }
    if (!FileRead(handle, records, (uiEndIndex + 1) * 0x297 - uiStartIndex * 0x297, &uiEndIndex)) {
        FileClose(handle);
        return 0;
    }
    FileClose(handle);
    return 1;
}
