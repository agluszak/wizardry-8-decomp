#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"

#include <stdlib.h>
#include <string.h>

extern "C" {

int giStringListLen;
wchar_t** gppStringList;   /* 0x0068C09C */

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
    static const char source[] =
        "C:\\Projects\\Wizardry 8\\Local Code\\Strings.cpp";
    int handle = FileOpen(const_cast<char*>(path), 0x41, 0);
    int index;

    if (!handle) {
        srAssertFail("hFile", source, 74,
                     "Failed to open localization string table.");
        return;
    }
    if (!ReadVirtualFile(handle, &giStringListLen, 4, 0)
        || !giStringListLen) {
        srAssertFail("giStringListLen", source, 79, 0);
        CloseVirtualFile(handle);
        return;
    }
    gppStringList = static_cast<wchar_t**>(
        malloc(giStringListLen * sizeof(wchar_t*)));
    if (!gppStringList) {
        srAssertFail("gppStringList", source, 82, 0);
        CloseVirtualFile(handle);
        return;
    }
    memset(gppStringList, 0,
           giStringListLen * sizeof(wchar_t*));
    for (index = 0; index != giStringListLen; ++index) {
        unsigned int byte_count;
        if (!ReadVirtualFile(handle, &byte_count, 4, 0)) {
            break;
        }
        gppStringList[index] =
            static_cast<wchar_t*>(malloc(byte_count));
        if (!gppStringList[index]) {
            srAssertFail("gppStringList[iCount]", source, 89, 0);
            break;
        }
        if (!ReadVirtualFile(handle, gppStringList[index],
                             byte_count, 0)) {
            break;
        }
        DecodeLocalizedText(reinterpret_cast<unsigned short*>(gppStringList[index]), byte_count / 2);
    }
    CloseVirtualFile(handle);
}

}
