#include "wiz8/record_file_0055a480.h"
#include "wiz8/virtual_file.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "FileMan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read one record: its string table, its entry array, and each entry's
   sub-entries with their own strings. Three levels of dynamic array, each
   allocated from a count that arrives in the slot the pointer then occupies.

   Both arrays are filled by the same idiom: clear the count, allocate, clear the
   block starting at the element the count points past, then add the count back.
   With the count cleared first that offset is always the base, which is why the
   arithmetic looks redundant; it is reproduced as the original spells it.

   Record strings arrive as wide characters and are converted through sprintf.
   The format at 0x0061C4B0 is four bytes of data in the reviewed image, which is
   consistent with the wide-to-narrow conversion spelled here but is not
   confirmed byte for byte; the operand is a masked relocation either way.

   Several reads here have no transferred-byte check and two allocations have no
   null check. That is the original's own error handling, not an omission. */
// FUNCTION: WIZ8 0x0055a140
unsigned char ReadFileRecord0055A140(int handle, W8FileRecord0055A140* record)
{
    W8FileEntry0055A140* entry;
    W8FileSubEntry0055A140* sub_entry;
    char* text;
    unsigned int transferred;
    unsigned short length;
    unsigned short disk_entry_count;
    unsigned char disk_sub_count;
    unsigned int block_size;
    int index;
    int sub_index;
    int offset;
    int total_sub_entries;
    W8WideChar wide[2000];

    total_sub_entries = 0;
    ReadVirtualFile(handle, record, 0xc, &transferred);
    if (transferred != 0xc) {
        return 0;
    }

    if (record->strings != 0) {
        ReadVirtualFile(handle, record, 1, &transferred);
        record->strings = static_cast<char**>(malloc(record->string_count * 4));
        for (index = 0; index < record->string_count; ++index) {
            ReadVirtualFile(handle, &length, 2, &transferred);
            if (transferred != 2) {
                return 0;
            }
            if (length != 0) {
                record->strings[index] = static_cast<char*>(malloc(length + 1));
                if (record->strings[index] == 0) {
                    return 0;
                }
                ReadVirtualFile(handle, wide, length * 2, &transferred);
                wide[length] = 0;
                sprintf(record->strings[index], "%S", wide);
            }
        }
    }

    disk_entry_count = record->entry_count;
    if (disk_entry_count != 0) {
        record->entries = 0;
        record->entry_count = 0;
        block_size = disk_entry_count * 0x12;
        record->entries = static_cast<W8FileEntry0055A140*>(malloc(block_size));
        if (record->entries != 0) {
            memset(record->entries + record->entry_count, 0, block_size);
            record->entry_count = record->entry_count + disk_entry_count;
        }
    }

    index = 0;
    if (record->entry_count != 0) {
        offset = 0;
        do {
            entry = reinterpret_cast<W8FileEntry0055A140*>(
                offset + reinterpret_cast<char*>(record->entries));
            ReadVirtualFile(handle, entry, 0x12, &transferred);
            if (transferred != 0x12) {
                return 0;
            }
            disk_sub_count = entry->sub_entry_count;
            if (disk_sub_count != 0) {
                entry->sub_entry_count = 0;
                entry->sub_entries = static_cast<W8FileSubEntry0055A140*>(
                    malloc(disk_sub_count * 8));
                if (entry->sub_entries == 0) {
                    return 0;
                }
                entry->sub_entry_count = entry->sub_entry_count + disk_sub_count;
                sub_index = 0;
                memset(entry->sub_entries + (entry->sub_entry_count - disk_sub_count),
                       0, disk_sub_count * 8);
                total_sub_entries = total_sub_entries + entry->sub_entry_count;
                if (entry->sub_entry_count != 0) {
                    do {
                        sub_entry = entry->sub_entries + sub_index;
                        ReadVirtualFile(handle, sub_entry, 8, &transferred);
                        if (transferred != 8) {
                            return 0;
                        }
                        if (sub_entry->text != 0) {
                            ReadVirtualFile(handle, &length, 2, &transferred);
                            if (transferred != 2) {
                                return 0;
                            }
                            text = static_cast<char*>(malloc(length + 1));
                            sub_entry->text = text;
                            if (text == 0) {
                                return 0;
                            }
                            ReadVirtualFile(handle, text, length, &transferred);
                            if (transferred != length) {
                                return 0;
                            }
                            text[length] = 0;
                        }
                        ++sub_index;
                    } while (sub_index < entry->sub_entry_count);
                }
            }
            ++index;
            offset = offset + 0x12;
        } while (index < record->entry_count);
    }
    return 1;
}

/* Load the whole file: the fixed header, the optional length-prefixed name, then
   one 0x0c-byte record per header count. Every failure returns without releasing
   what it already allocated, which is the original's behaviour and not an
   omission here. */
// FUNCTION: WIZ8 0x0055a480
W8RecordFile0055A480* LoadRecordFile0055A480(char* path)
{
    int handle;
    W8RecordFile0055A480* file;
    char* name;
    W8FileRecord0055A140* records;
    unsigned int transferred;
    unsigned short length;
    unsigned int index;

    handle = FileOpen(path, 0x41, 0);
    if (handle == 0) {
        return 0;
    }
    file = static_cast<W8RecordFile0055A480*>(malloc(0xe));
    if (file == 0) {
        return 0;
    }
    ReadVirtualFile(handle, file, 0xe, &transferred);
    if (transferred != 0xe) {
        return 0;
    }
    if (file->name != 0) {
        ReadVirtualFile(handle, &length, 2, &transferred);
        if (transferred != 2) {
            return 0;
        }
        if (length != 0) {
            name = static_cast<char*>(malloc(length + 1));
            file->name = name;
            if (name == 0) {
                return 0;
            }
            ReadVirtualFile(handle, name, length, &transferred);
            if (transferred != length) {
                return 0;
            }
            file->name[length] = 0;
        }
    }
    records = static_cast<W8FileRecord0055A140*>(malloc(file->record_count * 0xc));
    file->records = records;
    if (records == 0) {
        return 0;
    }
    for (index = 0; index < file->record_count; ++index) {
        if (!ReadFileRecord0055A140(handle, &file->records[index])) {
            return 0;
        }
    }
    CloseVirtualFile(handle);
    return file;
}
