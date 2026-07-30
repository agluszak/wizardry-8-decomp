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

/* Engine Code\stScript.cpp. The two members at +0x18 and +0x28 are ordinary
   growable vectors: construction gives each capacity five, and destruction
   removes and frees every pointed-to line/label before the vector storage. */
class stScript : public srClassSupport<stScript, srClass, 0, 0x1000d> {
public:
    static const char* sGetClassName() { return "stScript"; }

    virtual ~stScript() override;
    virtual srClass* vInstance() override;

    int FindLabelLine004CF730(const char* label) const;
    int GetSourceLine004CF790(int line) const;
    unsigned char Load004CF3B0(const char* path);

    W8GrowableVector<stScriptLine*> lines;       /* 0x18 */
    W8GrowableVector<stScriptLabel*> labels;     /* 0x28 */
};

static_assert(sizeof(stScript) == 0x38, "stScript_must_be_0x38");
