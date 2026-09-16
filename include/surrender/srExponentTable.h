#pragma once

#include "srHeap.h"

/* Recovered SR provider utility. Its methods are provider exports, not imports
   in the known consumers, so the class must not carry consumer dllimport. */
class srExponentTable {
public:
    srExponentTable(float exponent);
    srExponentTable& operator=(const srExponentTable& other);

    float getExponent() const;
    float getValue(float x) const;

protected:
    void setExponent(float exponent);

    float values_[0x400];
    float exponent_;
};

static_assert((sizeof(srExponentTable) == 0x1004), "srExponentTable_must_be_0x1004");
