#pragma once

#include "srHeap.h"

/* Recovered SR provider utility. Its methods are provider exports, not imports
   in the known consumers, so the class must not carry consumer dllimport. */
class srExponentTable {
public:
    srExponentTable(float exponent = 1.0f);
    srExponentTable& operator=(const srExponentTable& other);

    float getExponent() const;
    float getValue(float x) const;

protected:
    void setExponent(float exponent);

    float values_[0x400];
    float exponent_;
};

static_assert((sizeof(srExponentTable) == 0x1004), "srExponentTable_must_be_0x1004");

/* The cached table keeps a process-wide doubly linked freelist of up to 0x10
   exponent tables. get() bumps a reference count and reuses the last result;
   the renderer releases tables back to the pool instead of deleting them. */
class srCachedExponentTable : public srExponentTable {
public:
    static srCachedExponentTable* get(float exponent);
    static void freeAll();
    static void freeUnused();

    void release();

    srCachedExponentTable& operator=(const srCachedExponentTable& other);

protected:
    /* The ??_F default-constructor closures (0x10003160 base, 0x100031A0
       derived) pass 1.0, so both ctors default exponent to 1.0f. */
    srCachedExponentTable(float exponent = 1.0f);
    ~srCachedExponentTable();

    long ref_count_1004;
    srCachedExponentTable* previous_1008;
    srCachedExponentTable* next_100c;

    static srCachedExponentTable* first;
    static srCachedExponentTable* lastResult;
    static long count;
    static float lastQuery;
};

static_assert((sizeof(srCachedExponentTable) == 0x1010), "srCachedExponentTable_must_be_0x1010");
