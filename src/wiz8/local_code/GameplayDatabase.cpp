#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/screen_state.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern void __fastcall ProcessStartupStateEntry(W8StartupStateElement005EE748* entry);
extern unsigned char IsSoundPlaying(int sound_handle);
extern unsigned char StopSound(int sound_handle);
extern void Function52F890(int party_slot, int value_1, int value_2, int value_3, int value_4);
extern void QueueGameplayEvent(int event_type, int party_slot);
extern void PostCharacterMessage(int party_slot, const W8WideChar* format, ...);
extern W8WideChar* GetItemDisplayName(const W8ItemInstance* item);
extern unsigned char* g_message_table_68c09c;
/* 0x0054B300 resets one of eight slots. */
extern void Function54B300(unsigned int slot);
/* The gStatus object owned by GameplayDatabase.cpp. */
// GLOBAL: WIZ8 0x00685170
W8GlobalStatus g_status_685170;
/* Persistent database roots owned by this translation unit.  Leaving these as
   unresolved externals made the runnable image relocate every load/store to
   the PE image base; the first four-byte count read consequently targeted a
   read-only header instead of game state. */
// GLOBAL: WIZ8 0x006836AC
W8FactDatabaseRecord* g_fact_records;
// GLOBAL: WIZ8 0x00683F8C
int g_fact_record_count;
// GLOBAL: WIZ8 0x0068516C
W8ItemDatabaseRecord* g_item_records;
// GLOBAL: WIZ8 0x00683F78
int g_item_record_count;
// GLOBAL: WIZ8 0x006836A4
W8LevelDatabaseRecord* g_level_records;
// GLOBAL: WIZ8 0x00683F90
int g_level_record_count;
// GLOBAL: WIZ8 0x006836A0
W8NpcDatabaseRecord* g_npc_records;
// GLOBAL: WIZ8 0x00683F88
unsigned int g_npc_record_count;
// GLOBAL: WIZ8 0x00683F84
unsigned int g_monster_record_count;
// GLOBAL: WIZ8 0x006836B0
W8ItemTableRecord** g_item_tables;
// GLOBAL: WIZ8 0x00683F7C
unsigned int g_item_table_count;
// GLOBAL: WIZ8 0x006836B4
char** g_item_table_category_names;
// GLOBAL: WIZ8 0x00683F80
unsigned int g_item_table_category_count;
// GLOBAL: WIZ8 0x0065BE1C
W8SpellRuntimeRecord* g_spell_records;
// GLOBAL: WIZ8 0x0065BE18
unsigned int g_spell_database_version;
extern W8GameSettings g_settings_6850c8;
extern "C" unsigned char g_flag_68edac;
void SetPendingScreenState(int value);
void RequestScreenTransition(void);
unsigned char g_monster_slot_block[0x1a0a];
extern W8MonsterSlot g_monster_slots_6836b8[];
extern void EnableAllRenderOptions(void);
extern void DisableRenderOption(int id);
extern unsigned int GetTotalPhysicalMemory(void);
extern int GetRendererFamily(void);
extern int g_dword_685189;
extern int g_active_party_slot_0068518d;
extern int g_dword_686a70;
extern unsigned int g_dwords_686a50[8];
extern unsigned char g_flag_687511;
extern void Function58FD30(void);
extern void Function520070(W8ItemInstance* item, int a, int b);
extern void Function554580(unsigned char* target);
extern void ReplaceOrCreateItem(W8ItemInstance* item, unsigned int id, int a, int b, int c);
extern bool g_flag_68517c;
extern bool g_flag_683fa0;
extern int g_combat_difficulty_006850d5;
extern int g_dword_6875b7;
extern void Function5A9E70(void* target);
extern void Function482720(int value);
extern void Function482740(int value);
extern void Function509890(void);
extern void Function509920(void);
extern void Function558820(void);
extern void Function535920(void);
extern void Function56C520(void);
/* 0x004E8290, not yet identified; notified when a party slot is reset. */
extern void SetSlotAction(int slot, int a, int b);
#define GAMEPLAY_DATABASE_CPP "C:\\Projects\\Wizardry 8\\Local Code\\GameplayDatabase.cpp"

/* The three loaders below share one shape: build Data\Databases\<NAME>.DBS,
   open it, read a record count, allocate count * stride, then read the records
   one at a time. They are written out rather than folded into a helper because
   each is its own COMDAT in the original and shares no code with the others.
   All three leak the handle when the allocation fails while every other failure
   closes it; the asymmetry is the original's and is reproduced. */

// FUNCTION: WIZ8 0x0054ad00
unsigned char InitializeFactDatabase(void)
{
    char path[60];
    unsigned int index;
    unsigned int transferred;
    int handle;

    sprintf(path, "%s\\%s.%s", "Data\\Databases", "FACT", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!ReadVirtualFile(handle, &g_fact_record_count, 4, &transferred)) {
        CloseVirtualFile(handle);
        return 0;
    }
    g_fact_records = (W8FactDatabaseRecord*)malloc(g_fact_record_count * 0x1d8);
    if (!g_fact_records) {
        return 0;
    }
    for (index = 0; index < (unsigned int)g_fact_record_count; ++index) {
        if (!ReadVirtualFile(handle, &g_fact_records[index], 0x1d8, &transferred)) {
            CloseVirtualFile(handle);
            return 0;
        }
    }
    CloseVirtualFile(handle);
    return 1;
}

// FUNCTION: WIZ8 0x0054ae00
void DestroyFactDatabase(void)
{
    free(g_fact_records);
    g_fact_records = 0;
}

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
    if (!ReadVirtualFile(handle, &g_item_record_count, 4, &transferred)) {
        CloseVirtualFile(handle);
        return 0;
    }
    g_item_records = (W8ItemDatabaseRecord*)malloc(g_item_record_count * 0x10d);
    if (!g_item_records) {
        return 0;
    }
    for (index = 0; index < (unsigned int)g_item_record_count; ++index) {
        if (!ReadVirtualFile(handle, &g_item_records[index], 0x10d, &transferred)) {
            CloseVirtualFile(handle);
            return 0;
        }
    }
    CloseVirtualFile(handle);
    return 1;
}

// FUNCTION: WIZ8 0x0054ae20
unsigned char InitializeLevelDatabase(void)
{
    char path[60];
    unsigned int index;
    unsigned int transferred;
    int handle;

    sprintf(path, "%s\\%s.%s", "Data\\Databases", "LEVELS", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!ReadVirtualFile(handle, &g_level_record_count, 4, &transferred)) {
        CloseVirtualFile(handle);
        return 0;
    }
    g_level_records = (W8LevelDatabaseRecord*)malloc(g_level_record_count * 0xd8);
    if (!g_level_records) {
        return 0;
    }
    for (index = 0; index < (unsigned int)g_level_record_count; ++index) {
        if (!ReadVirtualFile(handle, &g_level_records[index], 0xd8, &transferred)) {
            CloseVirtualFile(handle);
            return 0;
        }
    }
    CloseVirtualFile(handle);
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
    if (!ReadVirtualFile(handle, &g_item_table_category_count, 4, &transferred)) {
        CloseVirtualFile(handle);
        return 0;
    }
    if (g_item_table_category_count) {
        g_item_table_category_names = (char**)malloc(g_item_table_category_count * 4);
        if (!g_item_table_category_names) {
            return 0;
        }
        memset(g_item_table_category_names, 0, g_item_table_category_count * 4);
        for (index = 0; index < g_item_table_category_count; ++index) {
            g_item_table_category_names[index] = (char*)malloc(0x100);
            ReadVirtualFile(handle, g_item_table_category_names[index], 0x100, &transferred);
        }
    }
    if (!ReadVirtualFile(handle, &g_item_table_count, 4, &transferred)) {
        CloseVirtualFile(handle);
        return 0;
    }
    if (g_item_table_count) {
        g_item_tables = (W8ItemTableRecord**)malloc(g_item_table_count * 4);
        if (!g_item_tables) {
            return 0;
        }
        memset(g_item_tables, 0, g_item_table_count * 4);
        for (index = 0; index < g_item_table_count; ++index) {
            g_item_tables[index] = (W8ItemTableRecord*)malloc(0x1f1);
            memset(g_item_tables[index], 0, 0x1f1);
            if (!g_item_tables[index]) {
                return 0;
            }
            if (!ReadVirtualFile(handle, g_item_tables[index]->name, 0x1f1, &transferred)) {
                CloseVirtualFile(handle);
                return 0;
            }
        }
    }
    CloseVirtualFile(handle);
    return 1;
}

/* NPC.DBS records carry an optional sub-list, stored after the record when its
   leading count exceeds one and its 0x9D flag is clear: a count, then that many
   six-byte elements appended to a freshly created list. */
// FUNCTION: WIZ8 0x0054aac0
unsigned char InitializeNpcDatabase(void)
{
    char path[60];
    unsigned int index;
    unsigned int entry;
    unsigned int entry_count;
    unsigned int transferred;
    int handle;
    void* element;

    sprintf(path, "%s\\%s.%s", "Data\\Databases", "NPC", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!ReadVirtualFile(handle, &g_npc_record_count, 4, &transferred)) {
        CloseVirtualFile(handle);
        return 0;
    }
    g_npc_records = (W8NpcDatabaseRecord*)malloc(g_npc_record_count * 0x309);
    if (!g_npc_records) {
        return 0;
    }
    for (index = 0; index < g_npc_record_count; ++index) {
        if (!ReadVirtualFile(handle, &g_npc_records[index], 0x309, &transferred)) {
            CloseVirtualFile(handle);
            return 0;
        }
        g_npc_records[index].item_stock_rules = 0;
        if (g_npc_records[index].flag_9d == 0 && g_npc_records[index].version > 1) {
            entry_count = 0;
            if (!ReadVirtualFile(handle, &entry_count, 4, &transferred)) {
                CloseVirtualFile(handle);
                return 0;
            }
            if (entry_count > 0) {
                g_npc_records[index].item_stock_rules = PListCreate();
                for (entry = 0; entry < entry_count; ++entry) {
                    element = ::operator new(6);
                    if (!element) {
                        CloseVirtualFile(handle);
                        return 0;
                    }
                    memset(element, 0, 6);
                    if (!ReadVirtualFile(handle, element, 6, &transferred)) {
                        CloseVirtualFile(handle);
                        return 0;
                    }
                    PListInsert(
                        g_npc_records[index].item_stock_rules,
                        entry, element);
                }
            }
        }
    }
    CloseVirtualFile(handle);
    return 1;
}

/* The general typed PList destructor deletes every element before releasing
   the pointer array and the list itself. This is the six-byte NPC item-stock
   specialization, not an NPC-specific lifecycle helper. */
// TEMPLATE: WIZ8 0x0055ada0
// PListDestructor
template <>
void PListDestructor<W8NpcItemStockRule>(W8PList* list)
{
    while (PListGetCount(list) != 0) {
        delete static_cast<W8NpcItemStockRule*>(PListRemoveAt(list, 0));
    }
    PListFreeData(list);
    PListDestroy(list);
}

// FUNCTION: WIZ8 0x0054ac90
void DestroyNpcDatabase(void)
{
    unsigned int index;

    if (g_npc_records) {
        for (index = 0; index < g_npc_record_count; ++index) {
            if (g_npc_records[index].item_stock_rules) {
                PListDestructor<W8NpcItemStockRule>(g_npc_records[index].item_stock_rules);
                g_npc_records[index].item_stock_rules = 0;
            }
        }
        free(g_npc_records);
        g_npc_records = 0;
    }
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

    if (!(index < g_monster_record_count)) {
        srAssertFail("uiMonsterIndex < gXStatus.uiMonstersInDatabase",
                     GAMEPLAY_DATABASE_CPP, 0x140, 0);
    }
    sprintf(path, "%s\\%s.%s", "Data\\Databases", "Monsters", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!FileSeek(handle, index * 0x297 + 4, 1)) {
        return 0;
    }
    if (!ReadVirtualFile(handle, record, 0x297, (unsigned int*)&uiMonsterIndex)) {
        CloseVirtualFile(handle);
        return 0;
    }
    CloseVirtualFile(handle);
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

// FUNCTION: WIZ8 0x0054af10
void DestroyLevelDatabase(void)
{
    free(g_level_records);
    g_level_records = 0;
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

/* Both buffers are cleared only after both allocations succeed, so a failed
   second allocation leaves the first one live and unzeroed. */
// FUNCTION: WIZ8 0x0054b4c0
unsigned char AllocateStatusBuffers(W8StatusBuffers* status)
{
    status->buffer_04 = malloc(0xc310);
    if (!status->buffer_04) {
        return 0;
    }
    status->buffer_08 = malloc(0x830);
    if (!status->buffer_08) {
        return 0;
    }
    memset(status->buffer_04, 0, 0xc310);
    memset(status->buffer_08, 0, 0x830);
    return 1;
}

// FUNCTION: WIZ8 0x0054b520
void FreeStatusBuffers(W8StatusBuffers* status)
{
    if (status->buffer_04) {
        free(status->buffer_04);
        status->buffer_04 = 0;
    }
    if (status->buffer_08) {
        free(status->buffer_08);
        status->buffer_08 = 0;
    }
}

// FUNCTION: WIZ8 0x0054b470
void ResetPartySlotRow(int slot)
{
    W8PartySlotRow* row = &g_party_slot_rows[slot];

    memset(row, 0, sizeof(W8PartySlotRow));
    row->flag_00 = 1;
    row->spell_id = 0;
    row->flag_0d0 = 0xff;
    SetSlotAction(slot, 0, -1);
}

// FUNCTION: WIZ8 0x0054b080
void ResetGameplayStatusBlock(void)
{
    memset(g_status_block_685078, 0, sizeof(g_status_block_685078));
    g_startup_runtime_state->ClearOwnedEntries();
    g_party_moving_006850b5 = 0;
    g_surprise_possible_00683fc5 = 0;
}

// FUNCTION: WIZ8 0x0054b2d0
void ResetTargetingState(void)
{
    unsigned int slot;

    for (slot = 0; slot < 8; ++slot) {
        Function54B300(slot);
    }
    g_target_state_6840b3 = -1;
    g_picked_group_006840b7 = -1;
}

/* The counterpart to InitializeItemTables: the category names first, then the
   tables, each entry freed before its array. Both arrays are re-read after
   every free because nothing tells VC6 that free leaves them alone. */
// FUNCTION: WIZ8 0x0054a6e0
void DestroyItemTables(void)
{
    unsigned int index;

    if (g_item_table_category_names) {
        for (index = 0; index < g_item_table_category_count; ++index) {
            if (g_item_table_category_names[index]) {
                free(g_item_table_category_names[index]);
            }
        }
        free(g_item_table_category_names);
    }
    if (g_item_tables) {
        for (index = 0; index < g_item_table_count; ++index) {
            if (g_item_tables[index]) {
                free(g_item_tables[index]);
            }
        }
        free(g_item_tables);
    }
}

/* Defined here because this unit holds the only site in the image that writes
   it - the store below is the single `mov byte ptr [0x687599], 1` among its 16
   reference sites, and the other fifteen are reads in seven other functions.
   Sole-writer ownership is weaker than a proved layout boundary: the .bss run
   around 0x00687599 is packed with no gap marking a unit edge, so this is the
   best available evidence rather than a settled fact. It is declared once in
   gameplay_boundaries.h; see that declaration for why the type is one byte. */
// GLOBAL: WIZ8 0x00687599
unsigned char g_save_flag_00687599;

/* Raises three flags, optionally hands the caller's target to 0x005A9E70, then
   runs a fixed opening sequence. The two calls into 0x00482720 and 0x00482740
   share one stack cleanup, as consecutive cdecl calls do. */
// FUNCTION: WIZ8 0x0054b250
void Function54B250(unsigned char notify, void* target)
{
    g_flag_68517c = true;
    if (target) {
        g_save_flag_00687599 = 1;
        Function5A9E70(target);
    }
    g_dword_6875b7 = g_combat_difficulty_006850d5;
    g_flag_683fa0 = true;
    Function482720(0x2932e00);
    Function482740(1);
    if (notify) {
        RequestScreenTransition();
    }
    Function509890();
    Function509920();
    Function558820();
    Function535920();
    Function56C520();
    SetPendingScreenState(2);
}

/* Optionally releases the global status block's two buffers, then clears the
   whole block - which zeroes those pointers as a side effect, since they live
   inside it - and allocates them again. Either allocation failing leaves the
   block cleared and the other buffer live, as the original does. */
// FUNCTION: WIZ8 0x0054af30
void Function54AF30(unsigned char release)
{
    if (release) {
        if (g_status_685170.buffers.buffer_04) {
            free(g_status_685170.buffers.buffer_04);
            g_status_685170.buffers.buffer_04 = 0;
        }
        if (g_status_685170.buffers.buffer_08) {
            free(g_status_685170.buffers.buffer_08);
            g_status_685170.buffers.buffer_08 = 0;
        }
    }
    memset(&g_status_685170, 0, sizeof(g_status_685170));
    g_status_685170.buffers.buffer_04 = malloc(0xc310);
    if (!g_status_685170.buffers.buffer_04) {
        return;
    }
    g_status_685170.buffers.buffer_08 = malloc(0x830);
    if (!g_status_685170.buffers.buffer_08) {
        return;
    }
    memset(g_status_685170.buffers.buffer_04, 0, 0xc310);
    memset(g_status_685170.buffers.buffer_08, 0, 0x830);
}

/* Reads MONSTERS.DBS whole: the count into gXStatus, then - only when the
   caller wants them - every record into one allocation handed back through the
   out-parameter. InitializeGameData calls it with null just to publish the count. */
// FUNCTION: WIZ8 0x0054a760
unsigned char Function54A760(W8MonsterRecord** records)
{
    char path[60];
    unsigned int transferred;
    unsigned int index;
    unsigned char* block;
    unsigned char* cursor;
    int handle;

    sprintf(path, "%s\\%s.%s", "Data\\Databases", "Monsters", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!ReadVirtualFile(handle, &g_monster_record_count, 4, &transferred)) {
        CloseVirtualFile(handle);
        return 0;
    }
    if (records) {
        block = (unsigned char*)malloc(g_monster_record_count * 0x297);
        if (!block) {
            return 0;
        }
        for (index = 0, cursor = block; index < g_monster_record_count; ++index) {
            if (!ReadVirtualFile(handle, cursor, 0x297, &transferred)) {
                CloseVirtualFile(handle);
                free(block);
                return 0;
            }
            cursor += 0x297;
        }
        *records = (W8MonsterRecord*)block;
    }
    CloseVirtualFile(handle);
    return 1;
}

/* The range sibling of LoadMonsterDatabaseRecord, named by its own assertion at
   GameplayDatabase.cpp line 378. It seeks to the first record and reads the
   whole inclusive span in one call, computing the length as two separate record
   offsets subtracted rather than from a record count. The bytes-read
   out-parameter is uiEndIndex's own slot, dead once copied into a register, and
   a failed seek leaves the handle open where every other failure closes it. */
// FUNCTION: WIZ8 0x0054a9a0
unsigned char Function54A9A0(unsigned int uiStartIndex, unsigned int uiEndIndex,
                             unsigned int unused, W8MonsterRecord* records)
{
    char path[56];
    int handle;

    if (!(uiEndIndex < g_monster_record_count)) {
        srAssertFail("uiEndIndex < gXStatus.uiMonstersInDatabase",
                     GAMEPLAY_DATABASE_CPP, 0x17a, 0);
    }
    sprintf(path, "%s\\%s.%s", "Data\\Databases", "Monsters", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!FileSeek(handle, uiStartIndex * 0x297 + 4, 1)) {
        return 0;
    }
    if (!ReadVirtualFile(handle, records,
                         (uiEndIndex + 1) * 0x297 - uiStartIndex * 0x297,
                         (unsigned int*)&uiEndIndex)) {
        CloseVirtualFile(handle);
        return 0;
    }
    CloseVirtualFile(handle);
    return 1;
}

/* The new-game reset. It repeats Function54AF30's status-block cycle inline
   rather than calling it, clears the item in hand and the carried pool, then
   grants the starting items. The pool and the id list are both walked by
   address against the symbol that follows them, not by index. */
// FUNCTION: WIZ8 0x0054b100
void Function54B100(void)
{
    W8ItemInstance item;
    W8ItemInstance* slot;
    unsigned int* id;
    unsigned int index;

    if (g_status_685170.buffers.buffer_04) {
        free(g_status_685170.buffers.buffer_04);
        g_status_685170.buffers.buffer_04 = 0;
    }
    if (g_status_685170.buffers.buffer_08) {
        free(g_status_685170.buffers.buffer_08);
        g_status_685170.buffers.buffer_08 = 0;
    }
    memset(&g_status_685170, 0, sizeof(g_status_685170));
    g_status_685170.buffers.buffer_04 = malloc(0xc310);
    if (g_status_685170.buffers.buffer_04) {
        g_status_685170.buffers.buffer_08 = malloc(0x830);
        if (g_status_685170.buffers.buffer_08) {
            memset(g_status_685170.buffers.buffer_04, 0, 0xc310);
            memset(g_status_685170.buffers.buffer_08, 0, 0x830);
        }
    }
    Function58FD30();
    Function520070(&g_item_in_hand, 0, 1);
    slot = g_party_item_pool;
    do {
        Function520070(slot, 0, 1);
        ++slot;
    } while (slot < (W8ItemInstance*)&g_party_item_count);
    id = g_starting_item_ids;
    do {
        if (*id != 0xffffffff) {
            ReplaceOrCreateItem(&item, *id, 1, 1, 1);
            item.stack_count = 1;
            AddItemToParty(&item, 0, 0);
        }
        ++id;
    } while (id < g_starting_item_ids_end);
    g_dword_685189 = 500;
    g_active_party_slot_0068518d = GetNextCharacter(1, 1, -1);
    g_dword_686a70 = -1;
    Function554580(&g_flag_687511);
    for (index = 0; index < 8; ++index) {
        g_dwords_686a50[index] = 0xffffffff;
    }
}

/* Clears the settings block and writes its defaults. The constants 0, 1, 0x40
   and 0xff are each used many times over, which is why VC6 holds them in
   registers rather than spelling out immediates. */
// FUNCTION: WIZ8 0x0054b560
void Function54B560(void)
{
    memset(&g_settings_6850c8, 0, sizeof(g_settings_6850c8));
    g_settings_6850c8.field_02e = 0x40;
    g_settings_6850c8.field_030 = 0x40;
    g_settings_6850c8.field_006 = 1;
    g_settings_6850c8.field_00a = 0;
    g_settings_6850c8.field_00b = 0;
    g_settings_6850c8.field_00c = 1;
    g_settings_6850c8.field_029 = 1;
    g_settings_6850c8.field_02a = 1;
    g_settings_6850c8.field_02b = 1;
    g_settings_6850c8.field_02c = 1;
    g_settings_6850c8.field_000 = 0;
    g_settings_6850c8.field_036 = 0;
    g_settings_6850c8.field_03b = 0;
    g_settings_6850c8.field_00d = 1;
    g_settings_6850c8.field_011 = 0x9c4;
    g_settings_6850c8.field_015 = 1000;
    g_settings_6850c8.field_019 = 5000;
    g_settings_6850c8.field_01d = 1;
    g_settings_6850c8.field_021 = 1;
    g_settings_6850c8.field_025 = 600;
    g_settings_6850c8.field_02f = 0x1f;
    g_settings_6850c8.field_031 = 0x13;
    g_settings_6850c8.field_032 = 0xff;
    g_settings_6850c8.field_033 = 0xff;
    g_settings_6850c8.field_034 = 0xff;
    g_settings_6850c8.field_035 = 0xff;
    g_settings_6850c8.field_037 = 0x40200000;
    g_settings_6850c8.field_03c = 0x3f800000;
    g_settings_6850c8.field_040 = 1;
    g_settings_6850c8.field_042 = 1;
    g_settings_6850c8.field_041 = 0;
    g_settings_6850c8.field_043 = 1;
    g_settings_6850c8.field_001 = 1;
    g_settings_6850c8.field_045 = 0;
    g_settings_6850c8.field_047 = 0;
    g_settings_6850c8.field_048 = 1;
    g_settings_6850c8.field_049 = 1;
    g_settings_6850c8.field_04a = 1;
    g_settings_6850c8.field_04b = 1;
    g_settings_6850c8.field_04c = 0;
    g_settings_6850c8.field_04d = 1;
    g_settings_6850c8.field_04e = 1;
    g_settings_6850c8.field_04f = 0;
    g_settings_6850c8.field_050 = 1;
    EnableAllRenderOptions();
    if (GetTotalPhysicalMemory() <= 0x4000000) {
        DisableRenderOption(0xb);
        DisableRenderOption(0xc);
    }
    if (GetRendererFamily() != 1) {
        DisableRenderOption(0x10);
    }
}

/* Resets one 0x118-byte slot. The tier it stores twice comes from the character
   the slot belongs to: the status block's first buffer is an array of
   W8Character at the 0x1862 stride, and the field at 0x0b01 - the same one
   party-member selection thresholds against 0x12 and 0x0f - decides between 1
   and 2 here. The five countdown clocks and the Random call share one stack
   cleanup, as consecutive cdecl calls do. */
// FUNCTION: WIZ8 0x0054b300
void Function54B300(unsigned int slot)
{
    W8MonsterSlot* record = &g_monster_slots_6836b8[slot];
    int tier;

    memset(record, 0, sizeof(W8MonsterSlot));
    record->field_000 = 0;
    record->field_001 = -1;
    record->field_075 = -1;
    record->field_079 = 6;
    record->field_099 = 0;
    tier = 1;
    if (((W8Character*)g_status_685170.buffers.buffer_04)[slot].unknown_0b01 >= 0xf) {
        tier = 2;
    }
    record->field_089 = tier;
    record->field_08d = tier;
    record->field_085 = -1;
    record->field_09a = 0;
    record->field_09b = 0;
    record->field_07d = SetCountdownClock(0);
    record->field_081 = 0;
    record->field_091 = SetCountdownClock(0);
    record->field_095 = SetCountdownClock(Random(5000) + 5000);
    record->field_0ca = SetCountdownClock(0);
    record->field_09c = 0;
    record->field_09d = 0;
    record->field_09e = 0;
    record->field_09f = 0;
    record->field_0a3 = -1;
    record->field_0a7 = 0;
    record->field_0ab = 0x90;
    record->field_0bd = 0;
    record->field_0c2 = -1;
    record->field_0be = -1;
    record->field_0c6 = 0;
    record->field_071 = 0;
    record->field_0bc = 0;
    record->field_0ac = 0;
    record->field_0b0 = 0;
    record->field_0b8 = 0;
    record->field_0b4 = 0;
    record->field_0ce = 0;
    record->field_0cf = 0;
    record->field_0d0 = 0;
    record->field_0d1 = 0;
    record->field_0d6 = 0;
    record->field_0e8 = 0;
    record->field_0d2 = SetCountdownClock(0);
}

/* 0x0052D460 proves four equal derived growable-vector instantiations followed
   by a fifth instantiation with a distinct vtable and the tail state below. Element identity and the
   complete lifetime remain tracked by wiz8-bxj; this gives startup the real
   allocation and field shape without inventing semantic names. */

// FUNCTION: WIZ8 0x0052d460
W8StartupRuntimeState::W8StartupRuntimeState()
    : value_50(-1), value_54(-1), value_5c(0), value_64(-1)
{
    bytes_68 = static_cast<unsigned char*>(::operator new(0xb1));
    memset(bytes_68, 0, 0xb1);
}

// FUNCTION: WIZ8 0x0052d5b0
W8StartupRuntimeState::~W8StartupRuntimeState()
{
    ::operator delete(bytes_68);
}

// FUNCTION: WIZ8 0x0052db80
void W8StartupRuntimeState::ClearOwnedEntries()
{
    W8StartupStateElement005EE748* entry;
    int count;

    count = vector_40.count;
    while (count > 0) {
        entry = vector_40.RemoveAt(0);
        ProcessStartupStateEntry(entry);
        count = vector_40.count;
    }
    count = vector_30.count;
    while (count > 0) {
        entry = vector_30.RemoveAt(0);
        ::operator delete(entry);
        count = vector_30.count;
    }
    count = vector_10.count;
    while (count > 0) {
        entry = vector_10.RemoveAt(0);
        ::operator delete(entry);
        count = vector_10.count;
    }
    count = vector_00.count;
    while (count > 0) {
        entry = vector_00.RemoveAt(0);
        ::operator delete(entry);
        count = vector_00.count;
    }
}

// FUNCTION: WIZ8 0x0052e3b0
void W8StartupRuntimeState::ProcessNextPendingEntry()
{
    W8StartupStateElement005EE748* entry;

    if (vector_40.count > 0) {
        entry = *vector_40.GetAt(0);
        vector_40.RemoveAt(vector_40.IndexOf(entry));
        if ((value_5c & 1) != 0 && entry->type_08 >= 14 && entry->type_08 < 16) {
            if ((value_5c & 2) != 0) {
                unknown_60 = SetCountdownClock(Random(6000) + 2000);
            }
            else {
                unknown_60 = SetCountdownClock(Random(60000) + 300000);
            }
        }
        ProcessStartupStateEntry(entry);
        ::operator delete(entry);
    }
}

// FUNCTION: WIZ8 0x0052ced0
void __fastcall ProcessStartupStateEntry(W8StartupStateElement005EE748* entry)
{
    W8MonsterSlot* slot;
    int party_slot;
    unsigned char sound_was_active;

    party_slot = CharacterPointerToPartySlot(entry->character_04);
    slot = &g_monster_slots_6836b8[party_slot];
    sound_was_active = slot->field_000;
    slot->field_071 = 0;
    if (sound_was_active != 0) {
        if (IsSoundPlaying(slot->field_001) != 0) {
            entry->handled_00 = 1;
            StopSound(slot->field_001);
        }
        Function52F890(party_slot, 0, -1, 0, 1);
    }
    if (entry->type_08 == 23 || entry->type_08 == 24) {
        if ((entry->flags_10 & 0x40) == 0) {
            if (entry->item_id_24 == -1) {
                PostCharacterMessage(
                    party_slot,
                    *reinterpret_cast<W8WideChar**>(g_message_table_68c09c + 0x1dc4));
            }
            else {
                PostCharacterMessage(
                    party_slot,
                    *reinterpret_cast<W8WideChar**>(g_message_table_68c09c + 0x1dc8),
                    GetItemDisplayName(
                        reinterpret_cast<const W8ItemInstance*>(&entry->item_id_24)));
            }
        }
    }
    else if (entry->type_08 == 51) {
        QueueGameplayEvent(30, party_slot);
    }
}

/* The block 0x0054AF30 and 0x0054B300 also work through; cleared here as bytes,
   which is why it is reached by a second, byte-wide declaration. */
W8StartupRuntimeState* g_startup_runtime_state;
void* g_object_685067;

// FUNCTION: WIZ8 0x0054afd0
void InitializeGameplayRuntimeObjects(void)
{
    memset(g_monster_slot_block, 0, sizeof(g_monster_slot_block));
    g_startup_runtime_state = new W8StartupRuntimeState();
    g_object_685067 = new W8GameTimer(300.0f, 0);
}

/* Stores the value the frame tick and the new-game reset both read back. */
// FUNCTION: WIZ8 0x0055ec50
void SetPendingScreenState(int value)
{
    g_dword_68ed10.id = value;
}

/* Latches the flag the frame tick clears once it has acted on it. */
// FUNCTION: WIZ8 0x0055ec60
void RequestScreenTransition(void)
{
    g_flag_68edac = 1;
}

/* Loads Data\\Databases\\SpellTables.dbs, replacing whatever is already there.
   The header is two dwords: an allocation count and the number of rows to read.
   They are not the same number, and the allocation is sized from the first while
   the loop runs over the second, which is what the original does.
 
   Each row is preceded by 0x101 bytes the loader skips rather than reads. A
   failure anywhere stops the loop and drops the whole table, but the file is
   closed and the row count published either way - including on failure, where
   the count then describes a table that is no longer there. Preserved as found.
 
   The assertion at Spells.cpp:1908 names the table s_pSpellTable, which is why
   this body treats the global as the table itself rather than as a cursor. */
// FUNCTION: WIZ8 0x004acc10
unsigned char InitializeSpellDatabase(void)
{
    int handle;
    unsigned int index;
    int offset = 0;
    unsigned char ok;
    int allocation_count;
    unsigned int row_count;

    if (g_spell_records != 0) {
        ::operator delete(g_spell_records);
        g_spell_records = 0;
        g_spell_database_version = 0;
    }
    handle = FileOpen("Data\\Databases\\SpellTables.dbs", 0x41, 0);
    if (handle == 0) {
        return 0;
    }
    ok = 0;
    if (ReadVirtualFile(handle, &allocation_count, 4, 0) &&
        ReadVirtualFile(handle, &row_count, 4, 0)) {
        ok = 1;
    }
    g_spell_records = static_cast<W8SpellRuntimeRecord*>(
        ::operator new(allocation_count * sizeof(W8SpellRuntimeRecord)));
    if (g_spell_records == 0) {
        srAssertFail(
            "s_pSpellTable",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
            0x774,
            0);
    }
    for (index = 0; index < row_count; ++index) {
        if (ok == 0) {
            goto discard;
        }
        ok = 0;
        if (FileSeek(handle, 0x101, FILE_SEEK_FROM_CURRENT) &&
            ReadVirtualFile(
                handle,
                reinterpret_cast<unsigned char*>(g_spell_records) + offset,
                sizeof(W8SpellRuntimeRecord),
                0)) {
            ok = 1;
        }
        offset += sizeof(W8SpellRuntimeRecord);
    }
    if (ok == 0) {
discard:
        ::operator delete(g_spell_records);
        g_spell_records = 0;
    }
    CloseVirtualFile(handle);
    g_spell_database_version = row_count;
    return ok;
}
