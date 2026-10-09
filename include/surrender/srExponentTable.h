#pragma once

#include "srHeap.h"

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srExponentTable {
public:
    srExponentTable(float exponent = 1.0f);

    float getExponent() const;
    float getValue(float x) const;

protected:
    void setExponent(float exponent);

    float values_[0x400];
    float exponent_;
};

static_assert((sizeof(srExponentTable) == 0x1004), "srExponentTable_must_be_0x1004");

/* Process-wide doubly linked freelist of up to 0x10 exponent tables. get() bumps a reference count
   and reuses the last result; the renderer releases tables back to the pool instead of deleting
   them. */
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srCachedExponentTable : public srExponentTable {
public:
    static srCachedExponentTable* get(float exponent);
    static void freeAll();
    static void freeUnused();

    void release();

protected:
    srCachedExponentTable(float exponent = 1.0f);
    ~srCachedExponentTable();

    w8_long ref_count;
    srCachedExponentTable* previous;
    srCachedExponentTable* next;

    static srCachedExponentTable* first;
    static srCachedExponentTable* lastResult;
    static w8_long count;
    static float lastQuery;
};

W8_ABI_ASSERT((sizeof(srCachedExponentTable) == 0x1010), "srCachedExponentTable_must_be_0x1010");
