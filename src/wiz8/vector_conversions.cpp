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
