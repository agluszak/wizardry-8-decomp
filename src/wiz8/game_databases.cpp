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
