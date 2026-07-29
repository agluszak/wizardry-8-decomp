#include "surrender/srMath.h"

// FUNCTION: WIZ8 0x00421680
srVector3T<float>* srVector3T<float>::method_00421680(
    double source_0,
    double source_1,
    double source_2)
{
    x = (float)source_0;
    y = (float)source_1;
    z = (float)source_2;
    return this;
}

// FUNCTION: WIZ8 0x00446110
srVector3T<float>* srVector3T<float>::method_00446110(const srVector3T<double>* source)
{
    x = (float)source->x;
    y = (float)source->y;
    z = (float)source->z;
    return this;
}

/* The four-component sibling of 0x00421680, identical in shape: three doubles
   become three floats there, four become four here, and both hand back this.
   The extra argument is what the trailing ret discriminates - 0x18 against
   0x20 - since neither takes a count. */
// FUNCTION: WIZ8 0x004817e0
srVector4T<float>* srVector4T<float>::method_004817E0(
    double source_0,
    double source_1,
    double source_2,
    double source_3)
{
    x = (float)source_0;
    y = (float)source_1;
    z = (float)source_2;
    w = (float)source_3;
    return this;
}

/* The same four-slot store from floats rather than doubles, so the arguments
   are copied rather than converted. The body alone cannot tell a float from an
   int here - a four-byte copy is the same either way - but the call sites can:
   every one of them computes in the FPU and pushes with fstp dword, so the
   parameters are floats. */
// FUNCTION: WIZ8 0x004d6b30
srVector4T<float>* srVector4T<float>::method_004D6B30(
    float source_0,
    float source_1,
    float source_2,
    float source_3)
{
    x = source_0;
    y = source_1;
    z = source_2;
    w = source_3;
    return this;
}
