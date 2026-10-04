#pragma once

#include <windows.h>

#include "srCore.h"
#include "srPlugin.h"

class
#if defined(SURRENDER_BUILD)
__declspec(dllexport)
#else
SR_DLL_IMPORT
#endif
srExtension {
public:
    srExtension(const char* name);
    ~srExtension();

    static void dumpAll(std::ostream& stream);
    static srExtension* find(const char* name);
    static long getCount();
    const char* getDescription();
    static srExtension* getFirst();
    const char* getName();
    srExtension* getNext();
    static srExtension* load(const char* name, const char* path);
    static void releaseAll();

private:
    // GLOBAL: SURRENDER 0x100A45F0
    static long count;
    // GLOBAL: SURRENDER 0x100A45EC
    static srExtension* firstExt;

    srPlugin* plugin_00;
    char* name_04;
    HMODULE module_08;
    srExtension* previous;
    srExtension* next_10;
};

static_assert(sizeof(srExtension) == 0x14,
              "srExtension_must_be_0x14");
