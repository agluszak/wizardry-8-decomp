#include "gameplay_boundaries.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern unsigned char ReadVirtualFile(int handle, void* buffer, unsigned int size,
                                     unsigned int* done);
extern int FileOpen(const char* path, int mode, int flags);
extern void CloseVirtualFile(int handle);

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

// FUNCTION: WIZ8 0x0054A510
/* ItemTables.DBS carries two arrays: category names, each a fixed 0x100-byte
   buffer, then the tables themselves. Both are arrays of pointers, cleared
   before use. The category reads are unchecked in the original while the table
   reads are not, and the per-table allocation is cleared before its own null
   check rather than after; both are reproduced. */
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
