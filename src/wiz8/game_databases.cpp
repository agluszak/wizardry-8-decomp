#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern unsigned char ReadVirtualFile(int handle, void* buffer, unsigned int size,
                                     unsigned int* done);
extern int FileOpen(const char* path, int mode, int flags);
/* 0x005E1CE0, the operator new import thunk, and two unidentified members of
   the PList family at 0x005E22C0 and 0x005E2530 that create a list and store an
   element at an index. */
extern void* operator_new_import_thunk(unsigned int size);
extern void* Function5E22C0(void);
extern void Function5E2530(void* list, unsigned int index, void* element);
/* 0x0052DB80 is a member function of the object at 0x00683FD7. VC6 has no way
   to spell __thiscall on a free declaration, but __fastcall passes its first
   argument in ECX, which is the same instruction the canonical emits.
   0x0054B300 resets one of eight slots. */
extern void __fastcall Function52DB80(void* self);
extern void Function54B300(unsigned int slot);
extern W8GlobalStatus g_status_685170;
extern W8GameSettings g_settings_6850c8;
extern int g_dword_68ed10;
extern unsigned char g_flag_68edac;
void Function55EC50(int value);
void Function55EC60(void);
extern unsigned char g_monster_slot_block[0x1a0a];
extern W8MonsterSlot g_monster_slots_6836b8[];
extern int SetCountdownClock(int delay);
extern int Random(int limit);
extern void Function47B5F0(void);
extern void Function47B5B0(int id);
extern unsigned int Function428E60(void);
extern int Function429800(void);
extern int g_dword_685189;
extern int g_dword_68518d;
extern int g_dword_686a70;
extern unsigned int g_dwords_686a50[8];
extern unsigned char g_flag_687511;
extern void Function58FD30(void);
extern void Function520070(W8ItemInstance* item, int a, int b);
extern int Function518230(int a, int b, int c);
extern void Function554580(unsigned char* target);
extern void ReplaceOrCreateItem(W8ItemInstance* item, unsigned int id, int a, int b, int c);
extern void AddItemToParty(W8ItemInstance* item, int a, int b);
extern bool g_flag_68517c;
extern bool g_flag_687599;
extern bool g_flag_683fa0;
extern int g_dword_6850d5;
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
extern void Function4E8290(int slot, int a, int b);
/* 0x0055ADA0, not yet identified; releases one record's sub-list. */
extern void Function55ADA0(void* sub_list);
extern void CloseVirtualFile(int handle);
extern unsigned char FileSeek(int handle, int offset, int origin);

#define GAMEPLAY_DATABASE_CPP "C:\\Projects\\Wizardry 8\\Local Code\\GameplayDatabase.cpp"

/* The three loaders below share one shape: build Data\Databases\<NAME>.DBS,
   open it, read a record count, allocate count * stride, then read the records
   one at a time. They are written out rather than folded into a helper because
   each is its own COMDAT in the original and shares no code with the others.
   All three leak the handle when the allocation fails while every other failure
   closes it; the asymmetry is the original's and is reproduced. */

// FUNCTION: WIZ8 0x0054AD00
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

// FUNCTION: WIZ8 0x0054AE00
void DestroyFactDatabase(void)
{
    free(g_fact_records);
    g_fact_records = 0;
}

// FUNCTION: WIZ8 0x0054A400
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

// FUNCTION: WIZ8 0x0054AE20
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
// FUNCTION: WIZ8 0x0054A510
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
// FUNCTION: WIZ8 0x0054AAC0
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
        g_npc_records[index].sub_list = 0;
        if (g_npc_records[index].flag_9d == 0 && g_npc_records[index].count_00 > 1) {
            entry_count = 0;
            if (!ReadVirtualFile(handle, &entry_count, 4, &transferred)) {
                CloseVirtualFile(handle);
                return 0;
            }
            if (entry_count > 0) {
                g_npc_records[index].sub_list = Function5E22C0();
                for (entry = 0; entry < entry_count; ++entry) {
                    element = operator_new_import_thunk(6);
                    if (!element) {
                        CloseVirtualFile(handle);
                        return 0;
                    }
                    memset(element, 0, 6);
                    if (!ReadVirtualFile(handle, element, 6, &transferred)) {
                        CloseVirtualFile(handle);
                        return 0;
                    }
                    Function5E2530(g_npc_records[index].sub_list, entry, element);
                }
            }
        }
    }
    CloseVirtualFile(handle);
    return 1;
}

// FUNCTION: WIZ8 0x0054AC90
void DestroyNpcDatabase(void)
{
    unsigned int index;

    if (g_npc_records) {
        for (index = 0; index < g_npc_record_count; ++index) {
            if (g_npc_records[index].sub_list) {
                Function55ADA0(g_npc_records[index].sub_list);
                g_npc_records[index].sub_list = 0;
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
// FUNCTION: WIZ8 0x0054A8A0
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
    StripMonsterNameSuffix((unsigned short*)record);
    StripMonsterNameSuffix((unsigned short*)((unsigned char*)record + 0x30));
    StripMonsterNameSuffix((unsigned short*)((unsigned char*)record + 0x60));
    StripMonsterNameSuffix((unsigned short*)((unsigned char*)record + 0x90));
    return 1;
}

/* Unlike its fact and level siblings this one guards the free and then leaves
   the pointer dangling rather than clearing it. Both halves of that asymmetry
   are the original's. */
// FUNCTION: WIZ8 0x0054A4F0
void DestroyItemDatabase(void)
{
    if (g_item_records) {
        free(g_item_records);
    }
}

// FUNCTION: WIZ8 0x0054AF10
void DestroyLevelDatabase(void)
{
    free(g_level_records);
    g_level_records = 0;
}

/* A generic guarded free, called from three unrelated subsystems, so it is named
   for what it does rather than for any one database. */
// FUNCTION: WIZ8 0x0054A880
void FreeIfNotNull(void* block)
{
    if (block) {
        free(block);
    }
}

/* Both buffers are cleared only after both allocations succeed, so a failed
   second allocation leaves the first one live and unzeroed. */
// FUNCTION: WIZ8 0x0054B4C0
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

// FUNCTION: WIZ8 0x0054B520
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

// FUNCTION: WIZ8 0x0054B470
void ResetPartySlotRow(int slot)
{
    W8PartySlotRow* row = &g_party_slot_rows[slot];

    memset(row, 0, sizeof(W8PartySlotRow));
    row->flag_00 = 1;
    row->unknown_075 = 0;
    row->flag_0d0 = 0xff;
    Function4E8290(slot, 0, -1);
}

// FUNCTION: WIZ8 0x0054B080
void ResetGameplayStatusBlock(void)
{
    memset(g_status_block_685078, 0, sizeof(g_status_block_685078));
    Function52DB80(g_object_683fd7);
    g_flag_6850b5 = 0;
    g_flag_683fc5 = 0;
}

// FUNCTION: WIZ8 0x0054B2D0
void ResetTargetingState(void)
{
    unsigned int slot;

    for (slot = 0; slot < 8; ++slot) {
        Function54B300(slot);
    }
    g_target_state_6840b3 = -1;
    g_target_state_6840b7 = -1;
}

/* The counterpart to InitializeItemTables: the category names first, then the
   tables, each entry freed before its array. Both arrays are re-read after
   every free because nothing tells VC6 that free leaves them alone. */
// FUNCTION: WIZ8 0x0054A6E0
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

/* Raises three flags, optionally hands the caller's target to 0x005A9E70, then
   runs a fixed opening sequence. The two calls into 0x00482720 and 0x00482740
   share one stack cleanup, as consecutive cdecl calls do. */
// FUNCTION: WIZ8 0x0054B250
void Function54B250(unsigned char notify, void* target)
{
    g_flag_68517c = true;
    if (target) {
        g_flag_687599 = true;
        Function5A9E70(target);
    }
    g_dword_6875b7 = g_dword_6850d5;
    g_flag_683fa0 = true;
    Function482720(0x2932e00);
    Function482740(1);
    if (notify) {
        Function55EC60();
    }
    Function509890();
    Function509920();
    Function558820();
    Function535920();
    Function56C520();
    Function55EC50(2);
}

/* Optionally releases the global status block's two buffers, then clears the
   whole block - which zeroes those pointers as a side effect, since they live
   inside it - and allocates them again. Either allocation failing leaves the
   block cleared and the other buffer live, as the original does. */
// FUNCTION: WIZ8 0x0054AF30
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
   out-parameter. Function4E2F40 calls it with null just to publish the count. */
// FUNCTION: WIZ8 0x0054A760
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
// FUNCTION: WIZ8 0x0054A9A0
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
// FUNCTION: WIZ8 0x0054B100
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
            item.quantity = 1;
            AddItemToParty(&item, 0, 0);
        }
        ++id;
    } while (id < g_starting_item_ids_end);
    g_dword_685189 = 500;
    g_dword_68518d = Function518230(1, 1, -1);
    g_dword_686a70 = -1;
    Function554580(&g_flag_687511);
    for (index = 0; index < 8; ++index) {
        g_dwords_686a50[index] = 0xffffffff;
    }
}

/* Clears the settings block and writes its defaults. The constants 0, 1, 0x40
   and 0xff are each used many times over, which is why VC6 holds them in
   registers rather than spelling out immediates. */
// FUNCTION: WIZ8 0x0054B560
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
    Function47B5F0();
    if (Function428E60() <= 0x4000000) {
        Function47B5B0(0xb);
        Function47B5B0(0xc);
    }
    if (Function429800() != 1) {
        Function47B5B0(0x10);
    }
}

/* Resets one 0x118-byte slot. The tier it stores twice comes from the character
   the slot belongs to: the status block's first buffer is an array of
   W8Character at the 0x1862 stride, and the field at 0x0b01 - the same one
   party-member selection thresholds against 0x12 and 0x0f - decides between 1
   and 2 here. The five countdown clocks and the Random call share one stack
   cleanup, as consecutive cdecl calls do. */
// FUNCTION: WIZ8 0x0054B300
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

/* Neither class is identified. Only what this constructor call establishes is
   declared: an allocation size and a constructor signature. */
struct W8GameplayObjectA {
    /* Only the allocation size is recovered; no field is identified. */
    unsigned char storage[0x6c];

    W8GameplayObjectA();
};

struct W8GameplayObjectB {
    unsigned char storage[0x24];

    W8GameplayObjectB(float limit, int flag);
};


/* The block 0x0054AF30 and 0x0054B300 also work through; cleared here as bytes,
   which is why it is reached by a second, byte-wide declaration. */
extern unsigned char g_monster_slot_block[0x1a0a];
extern void* g_object_683fd7;
extern void* g_object_685067;

// FUNCTION: WIZ8 0x0054AFD0
void Function54AFD0(void)
{
    memset(g_monster_slot_block, 0, sizeof(g_monster_slot_block));
    g_object_683fd7 = new W8GameplayObjectA();
    g_object_685067 = new W8GameplayObjectB(300.0f, 0);
}

/* Stores the value the frame tick and the new-game reset both read back. */
// FUNCTION: WIZ8 0x0055EC50
void Function55EC50(int value)
{
    g_dword_68ed10 = value;
}

/* Latches the flag the frame tick clears once it has acted on it. */
// FUNCTION: WIZ8 0x0055EC60
void Function55EC60(void)
{
    g_flag_68edac = 1;
}
