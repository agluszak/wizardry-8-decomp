#pragma once

#include "srHeap.h"

class srStringTable {
public:
    SR_DLL_IMPORT srStringTable();
    SR_DLL_IMPORT srStringTable(const srStringTable& other);
    SR_DLL_IMPORT ~srStringTable();
    SR_DLL_IMPORT srStringTable& operator=(const srStringTable& other);
    SR_DLL_IMPORT char* operator[](int index);

    SR_DLL_IMPORT void addString(const char* string);
    SR_DLL_IMPORT void addSeparatedStrings(
        const char* strings, const char* separators, int preserve_empty);
    SR_DLL_IMPORT long getCount() const;
    SR_DLL_IMPORT char* getString(long index) const;
    SR_DLL_IMPORT void reset();

private:
    char** strings_00;
    long capacity_04;
    long count_08;
};

static_assert((sizeof(srStringTable) == 0x0c), "srStringTable_must_be_0x0c");
