#pragma once

#include "surrender/srTypeRegistry.h"
#include "wiz8/vector.h"

struct stScriptLine {
    char* text;
    int source_line;
};

struct stScriptLabel {
    char name[0x20];
    int line;
};

/* Engine Code\stScript.cpp. Construction at 0x004CF020 installs each
   growable-vector base table, then its W8Vector table, at +0x18 and +0x28.
   Load allocates eight-byte lines and 0x24-byte labels; Clear frees those
   pointed-to records before the vector storage. */
class stScript : public srClassSupport<stScript, srClass, 1, 0x1000d> {
public:
    static const char* sGetClassName()
    {
        return "stScript";
    }

    virtual ~stScript() override;
    virtual srClass* vInstance() override;

    int FindLabelLine(const char* label) const;
    int GetSourceLine(int line) const;
    unsigned char Load(const char* path);
    void Clear();

    W8Vector<stScriptLine*> lines;   /* 0x18 */
    W8Vector<stScriptLabel*> labels; /* 0x28 */
};

static_assert(sizeof(stScript) == 0x38, "stScript_must_be_0x38");
