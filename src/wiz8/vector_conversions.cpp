#include "gameplay_boundaries.h"

// FUNCTION: WIZ8 0x00421680
W8Vector3* W8Vector3::VectorFromThreeFloats(
    double source_x,
    double source_y,
    double source_z)
{
    x = (float)source_x;
    y = (float)source_y;
    z = (float)source_z;
    return this;
}

// FUNCTION: WIZ8 0x00446110
W8Vector3* W8Vector3::Copy3DVector(const W8Vector3Double* source)
{
    x = (float)source->x;
    y = (float)source->y;
    z = (float)source->z;
    return this;
}
