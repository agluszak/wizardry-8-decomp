#include "surrender/srExponentTable.h"

#include <math.h>
#include <string.h>

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
    x *= 1023.0f;
    return values_[srFloatToInt(x)];
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

// FUNCTION: SURRENDER 0x10003140
srExponentTable& srExponentTable::operator=(const srExponentTable& other)
{
    memcpy(this, &other, sizeof(srExponentTable));
    return *this;
}

// GLOBAL: SURRENDER 0x100A0284
srCachedExponentTable* srCachedExponentTable::first;

// GLOBAL: SURRENDER 0x100A0288
srCachedExponentTable* srCachedExponentTable::lastResult;

// GLOBAL: SURRENDER 0x100A028C
long srCachedExponentTable::count;

/* Initialized to a value no real exponent query can equal, so the first get()
   always takes the table walk. */
// GLOBAL: SURRENDER 0x100981D0
float srCachedExponentTable::lastQuery = -19192304.0f;

// FUNCTION: SURRENDER 0x10002EC0
srCachedExponentTable::srCachedExponentTable(float exponent) : srExponentTable(exponent)
{
    previous_1008 = 0;
    next_100c = first;
    first = this;
    if (next_100c != 0) {
        next_100c->previous_1008 = this;
    }
    ref_count_1004 = 1;
    count += 1;
}

// FUNCTION: SURRENDER 0x10002F20
srCachedExponentTable::~srCachedExponentTable()
{
    if (this == lastResult) {
        lastResult = 0;
    }
    if (previous_1008 != 0) {
        previous_1008->next_100c = next_100c;
    }
    if (next_100c != 0) {
        next_100c->previous_1008 = previous_1008;
    }
    if (this == first) {
        first = next_100c;
    }
    count -= 1;
}

// FUNCTION: SURRENDER 0x10002F80
void srCachedExponentTable::freeAll()
{
    while (first != 0) {
        delete first;
    }
}

// FUNCTION: SURRENDER 0x10002FB0
void srCachedExponentTable::freeUnused()
{
    srCachedExponentTable* table = first;
    while (table != 0) {
        srCachedExponentTable* next = table->next_100c;
        if (table->ref_count_1004 <= 0) {
            delete table;
        }
        table = next;
    }
}

// FUNCTION: SURRENDER 0x10002FF0
void srCachedExponentTable::release()
{
    ref_count_1004 -= 1;
}

// FUNCTION: SURRENDER 0x10003000
srCachedExponentTable* srCachedExponentTable::get(float exponent)
{
    if (exponent == lastQuery && lastResult != 0) {
        lastResult->ref_count_1004 += 1;
        return lastResult;
    }
    lastQuery = exponent;
    for (srCachedExponentTable* table = first; table != 0; table = table->next_100c) {
        if (table->exponent_ == exponent) {
            lastResult = table;
            table->ref_count_1004 += 1;
            return table;
        }
    }
    if (count > 0xf) {
        for (srCachedExponentTable* table = first; table != 0; table = table->next_100c) {
            if (table->ref_count_1004 < 1) {
                table->setExponent(exponent);
                table->ref_count_1004 += 1;
                lastResult = table;
                return table;
            }
        }
    }
    lastResult = new srCachedExponentTable(exponent);
    return lastResult;
}

// FUNCTION: SURRENDER 0x10003180
srCachedExponentTable& srCachedExponentTable::operator=(const srCachedExponentTable& other)
{
    memcpy(this, &other, sizeof(srCachedExponentTable));
    return *this;
}

/* Emitted inside this TU by the constructor defaults. */
// SYNTHETIC: SURRENDER 0x10003160
// srExponentTable default constructor closure

// SYNTHETIC: SURRENDER 0x100031A0
// srCachedExponentTable default constructor closure

// SYNTHETIC: SURRENDER 0X100031C0
// srBoxFilter global static-init block

// SYNTHETIC: SURRENDER 0X100031D0
// srBoxFilter global atexit registrar

// SYNTHETIC: SURRENDER 0X10003200
// srTriangleFilter global static-init block

// SYNTHETIC: SURRENDER 0X10003210
// srTriangleFilter global atexit registrar

// SYNTHETIC: SURRENDER 0X10003240
// srBellFilter global static-init block

// SYNTHETIC: SURRENDER 0X10003250
// srBellFilter global atexit registrar

// SYNTHETIC: SURRENDER 0X10003280
// srBSplineFilter global static-init block

// SYNTHETIC: SURRENDER 0X10003290
// srBSplineFilter global atexit registrar
