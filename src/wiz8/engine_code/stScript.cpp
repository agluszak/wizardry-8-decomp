#include "wiz8/engine_code/stScript.h"
#include "surrender/srCore.h"

#include <string.h>

// VTABLE: WIZ8 0x005ED328
// class stScript

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

// FUNCTION: WIZ8 0x004CF7C0
unsigned long stScript::getClassID() const
{
    return 0x1000d;
}

// FUNCTION: WIZ8 0x004CF7D0
const char* stScript::getClassName() const
{
    return "stScript";
}

// FUNCTION: WIZ8 0x004CF7E0
srRegistry::ClassNode* stScript::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1000d);

    if (!node) {
        node = registry->registerClass(
            "stScript", srClass::sGetClassNode(), 0x1000d, 1);
    }
    return node;
}
