#include "wiz8/npc_script_file.h"
#include "wiz8/virtual_file.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "FileMan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Unresolved fragment: all three functions lie in the anchored gap between
   Party Import.cpp (ends 0x00559BC0) and chunk.cpp (0x0055BE80), the same
   interval as npc_items.cpp. No proven ownership. */

/* Read one record: its string table, its entry array, and each entry's
   sub-entries with their own strings. Three levels of dynamic array, each
   allocated from a count that arrives in the slot the pointer then occupies.

   Array counts are cleared before allocation and restored after successful
   allocation. Allocation failures retain the reader's original partial state.

   Record strings arrive as wide characters and are converted through sprintf.
   The format at 0x0061C4B0 is four bytes of data in the reviewed image, which is
   consistent with the wide-to-narrow conversion spelled here but is not
   confirmed byte for byte; the operand is a masked relocation either way.

   Several reads here have no transferred-byte check and two allocations have no
   null check. That is the original's own error handling, not an omission. */

/* Free each record's string table and the first entry's sub-entry array. The
   header, name, quotes array, and remaining entry allocations are left for
   the caller; NPC rebinding overwrites the pointer without releasing them. */
// FUNCTION: WIZ8 0x0055a0a0
void ReleaseNpcScriptFile0055A0A0(W8NpcScriptFile* file)
{
    W8NpcScriptQuote* record;
    unsigned int index;
    unsigned int string_index;

    if (file == 0) {
        return;
    }
    if (file->quote_count == 0) {
        return;
    }
    for (index = 0; index < file->quote_count; ++index) {
        record = file->quotes + index;
        if (record == 0) {
            continue;
        }
        if (record->entries != 0 && record->entries->sub_entries != 0) {
            free(record->entries->sub_entries);
        }
        if (record->subquotes != 0) {
            for (string_index = 0; string_index < record->subquote_count; ++string_index) {
                free(record->subquotes[string_index]);
            }
            free(record->subquotes);
        }
    }
}

// FUNCTION: WIZ8 0x0055a140
unsigned char ReadNpcScriptQuote0055A140(int handle, W8NpcScriptQuote* record)
{
    W8NpcQuoteEntry* entry;
    W8NpcQuoteSubEntry* sub_entry;
    char* text;
    unsigned int transferred;
    unsigned short length;
    unsigned short disk_entry_count;
    unsigned char disk_sub_count;
    unsigned int block_size;
    int index;
    int sub_index;
    wchar_t wide[2000];

    FileRead(handle, record, 0xc, &transferred);
    if (transferred != 0xc) {
        return 0;
    }

    if (record->subquotes != 0) {
        FileRead(handle, record, 1, &transferred);
        record->subquotes = static_cast<char**>(malloc(record->subquote_count * 4));
        for (index = 0; index < record->subquote_count; ++index) {
            FileRead(handle, &length, 2, &transferred);
            if (transferred != 2) {
                return 0;
            }
            if (length != 0) {
                record->subquotes[index] = static_cast<char*>(malloc(length + 1));
                if (record->subquotes[index] == 0) {
                    return 0;
                }
                FileRead(handle, wide, length * 2, &transferred);
                wide[length] = 0;
                sprintf(record->subquotes[index], "%S", wide);
            }
        }
    }

    disk_entry_count = record->entry_count;
    if (disk_entry_count != 0) {
        record->entries = 0;
        record->entry_count = 0;
        block_size = disk_entry_count * 0x12;
        record->entries = static_cast<W8NpcQuoteEntry*>(malloc(block_size));
        if (record->entries != 0) {
            memset(record->entries, 0, block_size);
            record->entry_count = disk_entry_count;
        }
    }

    for (index = 0; index < record->entry_count; ++index) {
        entry = &record->entries[index];
        FileRead(handle, entry, 0x12, &transferred);
        if (transferred != 0x12) {
            return 0;
        }
        disk_sub_count = entry->sub_entry_count;
        if (disk_sub_count != 0) {
            entry->sub_entry_count = 0;
            entry->sub_entries = static_cast<W8NpcQuoteSubEntry*>(malloc(disk_sub_count * 8));
            if (entry->sub_entries == 0) {
                return 0;
            }
            entry->sub_entry_count = disk_sub_count;
            memset(entry->sub_entries, 0, disk_sub_count * 8);
            for (sub_index = 0; sub_index < entry->sub_entry_count; ++sub_index) {
                sub_entry = entry->sub_entries + sub_index;
                FileRead(handle, sub_entry, 8, &transferred);
                if (transferred != 8) {
                    return 0;
                }
                if (sub_entry->text != 0) {
                    FileRead(handle, &length, 2, &transferred);
                    if (transferred != 2) {
                        return 0;
                    }
                    text = static_cast<char*>(malloc(length + 1));
                    sub_entry->text = text;
                    if (text == 0) {
                        return 0;
                    }
                    FileRead(handle, text, length, &transferred);
                    if (transferred != length) {
                        return 0;
                    }
                    text[length] = 0;
                }
            }
        }
    }
    return 1;
}

/* Load the whole file: the fixed header, the optional length-prefixed name, then
   one 0x0c-byte record per header count. Every failure returns without releasing
   what it already allocated, which is the original's behaviour and not an
   omission here. */
// FUNCTION: WIZ8 0x0055a480
W8NpcScriptFile* LoadNpcScriptFile0055A480(char* path)
{
    int handle;
    W8NpcScriptFile* file;
    char* name;
    W8NpcScriptQuote* quotes;
    unsigned int transferred;
    unsigned short length;
    unsigned int index;

    handle = FileOpen(path, 0x41, 0);
    if (handle == 0) {
        return 0;
    }
    file = static_cast<W8NpcScriptFile*>(malloc(0xe));
    if (file == 0) {
        return 0;
    }
    FileRead(handle, file, 0xe, &transferred);
    if (transferred != 0xe) {
        return 0;
    }
    if (file->name != 0) {
        FileRead(handle, &length, 2, &transferred);
        if (transferred != 2) {
            return 0;
        }
        if (length != 0) {
            name = static_cast<char*>(malloc(length + 1));
            file->name = name;
            if (name == 0) {
                return 0;
            }
            FileRead(handle, name, length, &transferred);
            if (transferred != length) {
                return 0;
            }
            file->name[length] = 0;
        }
    }
    quotes = static_cast<W8NpcScriptQuote*>(malloc(file->quote_count * 0xc));
    file->quotes = quotes;
    if (quotes == 0) {
        return 0;
    }
    for (index = 0; index < file->quote_count; ++index) {
        if (!ReadNpcScriptQuote0055A140(handle, &file->quotes[index])) {
            return 0;
        }
    }
    FileClose(handle);
    return file;
}
