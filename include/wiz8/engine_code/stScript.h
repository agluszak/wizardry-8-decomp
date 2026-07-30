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
   removes and frees every pointed-to line/label before the vector storage.
   The 0x14-byte prefix belongs to the SurRender class-support specialization;
   it stays opaque until that published template is reconstructed globally. */
class stScript : public srClass {
public:
    unsigned long getClassID() const override;     /* 0x004CF7C0 */
    const char* getClassName() const override;     /* 0x004CF7D0 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x004CF7E0 */

    int FindLabelLine004CF730(const char* label) const;
    int GetSourceLine004CF790(int line) const;

    unsigned char class_support_004[0x14];
    W8GrowableVector<stScriptLine*> lines;       /* 0x18 */
    W8GrowableVector<stScriptLabel*> labels;     /* 0x28 */
};

static_assert(sizeof(stScript) == 0x38, "stScript_must_be_0x38");
