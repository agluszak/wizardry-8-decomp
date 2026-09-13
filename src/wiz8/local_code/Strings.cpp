#include "wiz8/sr_api.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/string_database.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"

#include <stdlib.h>
#include <string.h>

// GLOBAL: WIZ8 0x0068c098
int giStringListLen;
// GLOBAL: WIZ8 0x0068c09c
wchar_t** gppStringList;

/* 0x0052FF80: read one entry of a .msg string database. The file ends with the
   entry table; each record carries two metadata dwords, then the code-unit
   count and the text itself. The fifth header byte selects the 0x9697 text
   encoding, and the count guard admits at most 0x7D0 code units, which is the
   shared quote buffer's proven extent. */
// FUNCTION: WIZ8 0x0052FF80
unsigned char GetStringFromStringDatabase(const char* path, int index, W8WideChar* output,
                                          unsigned int* metadata_04, unsigned int* metadata_00)
{
    HWFILE handle;
    unsigned char header[5];
    W8WideChar* destination;
    int count;
    int entry_offset;
    int length;
    int character;

    destination = output;
    *output = 0;
    handle = FileOpen(const_cast<char*>(path), 0x41, 0);
    if (!handle) {
        return 0;
    }
    FileRead(handle, header, 5, 0);
    FileSeek(handle, 8, FILE_SEEK_FROM_END);
    FileRead(handle, &count, 4, 0);
    if (index < count) {
        FileSeek(handle, (count - index) * 4 + 8, FILE_SEEK_FROM_END);
        FileRead(handle, &entry_offset, 4, 0);
        FileSeek(handle, entry_offset, FILE_SEEK_FROM_START);
        if (metadata_00) {
            FileRead(handle, metadata_00, 4, 0);
        } else {
            FileSeek(handle, 4, FILE_SEEK_FROM_CURRENT);
        }
        if (metadata_04) {
            FileRead(handle, metadata_04, 4, 0);
        } else {
            FileSeek(handle, 4, FILE_SEEK_FROM_CURRENT);
        }
        FileRead(handle, &length, 4, 0);
        if (length <= 0x7d0) {
            FileRead(handle, destination, length * 2, 0);
            if (header[4] && length > 0) {
                for (character = 0; character < length; ++character) {
                    destination[character] =
                        static_cast<W8WideChar>(~destination[character] + 0x9697);
                }
            }
            FileClose(handle);
            return 1;
        }
    }
    FileClose(handle);
    return 0;
}

// FUNCTION: WIZ8 0x005300e0
void DecodeLocalizedText(unsigned short* text, int character_count)
{
    while (character_count-- > 0) {
        *text = static_cast<unsigned short>(~*text + 0x9697);
        ++text;
    }
}

// FUNCTION: WIZ8 0x00518360
void LoadLocalizedStrings(const char* path)
{
    static const char source[] = "C:\\Projects\\Wizardry 8\\Local Code\\Strings.cpp";
    int handle = FileOpen(const_cast<char*>(path), 0x41, 0);
    int index;

    if (!handle) {
        srAssertFail("hFile", source, 74, "Failed to open localization string table.");
        return;
    }
    if (!FileRead(handle, &giStringListLen, 4, 0) || !giStringListLen) {
        srAssertFail("giStringListLen", source, 79, 0);
        FileClose(handle);
        return;
    }
    gppStringList = static_cast<wchar_t**>(malloc(giStringListLen * sizeof(wchar_t*)));
    if (!gppStringList) {
        srAssertFail("gppStringList", source, 82, 0);
        FileClose(handle);
        return;
    }
    memset(gppStringList, 0, giStringListLen * sizeof(wchar_t*));
    for (index = 0; index != giStringListLen; ++index) {
        unsigned int byte_count;
        if (!FileRead(handle, &byte_count, 4, 0)) {
            break;
        }
        gppStringList[index] = static_cast<wchar_t*>(malloc(byte_count));
        if (!gppStringList[index]) {
            srAssertFail("gppStringList[iCount]", source, 89, 0);
            break;
        }
        if (!FileRead(handle, gppStringList[index], byte_count, 0)) {
            break;
        }
        DecodeLocalizedText(gppStringList[index], byte_count / 2);
    }
    FileClose(handle);
}
