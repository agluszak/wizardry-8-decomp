#include "wiz8/engine_code/stScript.h"
#include "surrender/srCore.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"

#include <stdlib.h>
#include <string.h>

/* Read one byte at a time through the virtual-file layer.  End-of-file after
   at least one byte still returns a line, while an empty end-of-file clears
   `more`.  The terminator is not retained and CRLF is normalised by removing
   the CR after the loop. */
// FUNCTION: WIZ8 0x004CEE40
unsigned char ReadTextLine004CEE40(
    int handle, char* destination, int capacity, unsigned char* more)
{
    unsigned char result;
    unsigned int transferred;
    char character;
    int length = 0;

    destination[0] = 0;
    *more = 1;

    for (;;) {
        result = ReadVirtualFile(handle, &character, 1, &transferred);
        if (transferred == 0) {
            result = length != 0;
            *more = 0;
            break;
        }
        if (result == 0) {
            *more = 0;
        }
        else {
            if (character == '\n') {
                break;
            }
            destination[length++] = character;
        }
        if (length >= capacity - 1) {
            result = 0;
            break;
        }
        if (result == 0) {
            break;
        }
    }

    destination[length] = 0;
    if (length != 0 && destination[length - 1] == '\r') {
        destination[length - 1] = 0;
    }
    return result;
}

// VTABLE: WIZ8 0x005ED328
// class stScript

// VTABLE: WIZ8 0x005ED358
// class srClassSupport<stScript,srClass,1,65549>

// SYNTHETIC: WIZ8 0x004CF020
// stScript::stScript

// FUNCTION: WIZ8 0x004CF110
srClass* stScript::vInstance()
{
    return new stScript;
}

// SYNTHETIC: WIZ8 0x004CF230
// stScript::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004CF7C0
// srClassSupport<stScript,srClass,1,65549>::getClassID

// TEMPLATE: WIZ8 0x004CF7D0
// srClassSupport<stScript,srClass,1,65549>::getClassName

// TEMPLATE: WIZ8 0x004CF7E0
// srClassSupport<stScript,srClass,1,65549>::getClassNode

// TEMPLATE: WIZ8 0x004CF820
// srClassSupport<stScript,srClass,1,65549>::clone

// TEMPLATE: WIZ8 0x004CF940
// srClassSupport<stScript,srClass,1,65549>::~srClassSupport<stScript,srClass,1,65549>

// SYNTHETIC: WIZ8 0x004CF9D0
// srClassSupport<stScript,srClass,1,65549>::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004CF260
stScript::~stScript()
{
    while (lines.GetCount() != 0) {
        stScriptLine* line = lines.RemoveAt(0);
        if (line != 0) {
            free(line->text);
            delete line;
        }
    }
    while (labels.GetCount() != 0) {
        delete labels.RemoveAt(0);
    }
}

// FUNCTION: WIZ8 0x004CF3B0
unsigned char stScript::Load004CF3B0(const char* path)
{
    unsigned char more = 1;
    int source_line = 0;
    int script_line = 0;
    char buffer[256];
    int handle;

    if (path == 0 || (handle = FileOpen(const_cast<char*>(path), 0x41, 0)) == 0) {
        return 0;
    }

    for (;;) {
        if (more == 0) {
            CloseVirtualFile(handle);
            return 1;
        }
        while (ReadTextLine004CEE40(
                   handle, buffer, sizeof(buffer), &more) == 0) {
            if (more == 0) {
                CloseVirtualFile(handle);
                return 1;
            }
        }

        ++source_line;
        if (buffer[0] == 0) {
            continue;
        }

        char* owned_text = static_cast<char*>(malloc(strlen(buffer) + 1));
        if (owned_text == 0) {
            srAssertFail("pNewLine", "C:\\Projects\\Wizardry 8\\Engine Code\\stScript.cpp", 69, 0);
        }
        strcpy(owned_text, buffer);

        char* token = strtok(buffer, " \t");
        if (token == 0 || token[0] == '*') {
            free(owned_text);
            continue;
        }

        int length = strlen(token);
        if (token[length - 1] == ':') {
            token[length - 1] = 0;
            int index;
            unsigned char add_label = 1;
            for (index = 0; index < labels.GetCount(); ++index) {
                stScriptLabel* existing = *labels.GetAt(index);
                if (_strnicmp(existing->name, token, 0x1f) == 0) {
                    if (existing->line != -1) {
                        add_label = 0;
                    }
                    break;
                }
            }
            if (add_label != 0) {
                stScriptLabel* label = new stScriptLabel;
                if (label != 0) {
                    strncpy(label->name, token, 0x1f);
                    label->name[0x1f] = 0;
                    label->line = script_line;
                    labels.Add(label);
                }
            }
            free(owned_text);
            continue;
        }

        stScriptLine* line = new stScriptLine;
        if (line != 0) {
            line->text = owned_text;
            line->source_line = source_line;
            lines.Add(line);
        }
        ++script_line;
    }

}

/* The label table stores fixed 31-character, case-insensitive identifiers and
   the script line to resume at. A null query or a miss has the source's -1
   sentinel. */
// FUNCTION: WIZ8 0x004CF730
int stScript::FindLabelLine004CF730(const char* label) const
{
    int index;

    if (label != 0) {
        for (index = 0; index < labels.GetCount(); ++index) {
            stScriptLabel* entry = *labels.GetAt(index);
            if (_strnicmp(entry->name, label, 0x1f) == 0) {
                return entry->line;
            }
        }
    }
    return -1;
}

/* Diagnostics report the original file line carried by each parsed script
   line. Invalid or empty entries use the same -1 sentinel as label lookup. */
// FUNCTION: WIZ8 0x004CF790
int stScript::GetSourceLine004CF790(int line) const
{
    int count;
    stScriptLine** entry_pointer;
    stScriptLine* entry;

    if (line < 0) {
        return -1;
    }
    count = lines.GetCount();
    if (line >= count) {
        return -1;
    }
    entry_pointer = lines.data;
    if (line < count) {
        entry_pointer += line;
    }
    entry = *entry_pointer;
    if (entry != 0) {
        return entry->source_line;
    }
    return -1;
}
