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

// GLOBAL: SURRENDER 0x100A0284
srCachedExponentTable* srCachedExponentTable::first;

// GLOBAL: SURRENDER 0x100A0288
srCachedExponentTable* srCachedExponentTable::lastResult;

// GLOBAL: SURRENDER 0x100A028C
w8_long srCachedExponentTable::count;

/* Initialized to a value no real exponent query can equal, so the first get()
   always takes the table walk. */
// GLOBAL: SURRENDER 0x100981D0
float srCachedExponentTable::lastQuery = -19192304.0f;

// FUNCTION: SURRENDER 0x10002EC0
srCachedExponentTable::srCachedExponentTable(float exponent) : srExponentTable(exponent)
{
    previous = 0;
    next = first;
    first = this;
    if (next != 0) {
        next->previous = this;
    }
    ref_count = 1;
    count += 1;
}

// FUNCTION: SURRENDER 0x10002F20
srCachedExponentTable::~srCachedExponentTable()
{
    if (this == lastResult) {
        lastResult = 0;
    }
    if (previous != 0) {
        previous->next = next;
    }
    if (next != 0) {
        next->previous = previous;
    }
    if (this == first) {
        first = next;
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
        srCachedExponentTable* next = table->next;
        if (table->ref_count <= 0) {
            delete table;
        }
        table = next;
    }
}

// FUNCTION: SURRENDER 0x10002FF0
void srCachedExponentTable::release()
{
    ref_count -= 1;
}

// FUNCTION: SURRENDER 0x10003000
srCachedExponentTable* srCachedExponentTable::get(float exponent)
{
    if (exponent == lastQuery && lastResult != 0) {
        lastResult->ref_count += 1;
        return lastResult;
    }
    lastQuery = exponent;
    for (srCachedExponentTable* table = first; table != 0; table = table->next) {
        if (table->exponent_ == exponent) {
            lastResult = table;
            table->ref_count += 1;
            return table;
        }
    }
    if (count > 0xf) {
        for (srCachedExponentTable* table = first; table != 0; table = table->next) {
            if (table->ref_count < 1) {
                table->setExponent(exponent);
                table->ref_count += 1;
                lastResult = table;
                return table;
            }
        }
    }
    lastResult = new srCachedExponentTable(exponent);
    return lastResult;
}
