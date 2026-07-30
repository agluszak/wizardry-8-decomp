#include "wiz8/record_file_0055a480.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include <stdlib.h>

/* The per-record reader. It builds the record's string table and its 0x12-byte
   entry array, and is recovered separately under wiz8-m6h5's follow-up. */
extern unsigned char ReadFileRecord0055A140(int handle, W8FileRecord0055A140* record);

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
