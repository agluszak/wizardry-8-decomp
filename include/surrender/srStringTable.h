#pragma once

#include "srHeap.h"

class srStringTable {
public:
    SR_DLL_IMPORT srStringTable();
    SR_DLL_IMPORT ~srStringTable();

    SR_DLL_IMPORT void addSeparatedStrings(
        const char* strings, const char* separators, int preserve_empty);
    SR_DLL_IMPORT char* getString(long index) const;

    long count() const { return count_; }

private:
    unsigned char unknown_00_[8];
    long count_;
};

typedef char srStringTable_must_be_0x0c[
    (sizeof(srStringTable) == 0x0c) ? 1 : -1];
