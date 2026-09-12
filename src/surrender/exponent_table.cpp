#include "surrender/srExponentTable.h"

#include <math.h>

// FUNCTION: SURRENDER 0x100030e0
srExponentTable::srExponentTable(float exponent)
{
    exponent_ = 0.0f;
    setExponent(exponent);
}

// FUNCTION: SURRENDER 0x10003100
float srExponentTable::getExponent() const
{
    return exponent_;
}

// FUNCTION: SURRENDER 0x10003110
float srExponentTable::getValue(float x) const
{
    return values_[(int)(x * 1023.0f)];
}

// FUNCTION: SURRENDER 0x10002e60
void srExponentTable::setExponent(float exponent)
{
    float index;
    int i;

    if (exponent != exponent_) {
        exponent_ = exponent;
        index = 0.0f;
        for (i = 0; i < 0x400; ++i) {
            values_[i] = (float)pow(index, exponent);
            index += 1.0f / 1023.0f;
        }
    }
}
