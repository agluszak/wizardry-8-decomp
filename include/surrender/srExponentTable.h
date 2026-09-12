#pragma once

#include "srHeap.h"

class SR_DLL_IMPORT srExponentTable {
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
