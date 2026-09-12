#pragma once

#include "srMath.h"

class SR_DLL_IMPORT srTriangulator {
public:
    class CircularList {
    public:
        class ListIterator;
    };

    srTriangulator(srVector2T<float>* points, int count);
    ~srTriangulator();
    srTriangulator& operator=(const srTriangulator& other);

    srVector3i next();

protected:
    int sameSide(const srVector2T<float>& p1, const srVector2T<float>& p2,
                 const srVector2T<float>& a, const srVector2T<float>& b);
    int isInsideTriangle(const srVector2T<float>& p, const srVector2T<float>& a,
                         const srVector2T<float>& b, const srVector2T<float>& c);
    int satisfyConstraints(CircularList::ListIterator iterator);

    unsigned char unknown_00_[0x10];
};

static_assert((sizeof(srTriangulator) == 0x10), "srTriangulator_must_be_0x10");
