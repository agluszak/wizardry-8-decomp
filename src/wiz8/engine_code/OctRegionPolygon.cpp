#include "wiz8/engine_code/OctBuildPreTree.h"

/* Test the polygon's representative point against an inclusive axis-aligned
   box. The method's original translation-unit owner is not yet proved. */
// FUNCTION: WIZ8 0x004cfb30
unsigned char W8OctRegionPolygon::ContainsPoint004CFB30(
    const srVector3T<float>* bounds) const
{
    for (short axis = 0; axis < 3; ++axis) {
        float value = (&position_18.x)[axis];
        if (value < (&bounds[0].x)[axis] ||
            (&bounds[1].x)[axis] < value) {
            return 0;
        }
    }
    return 1;
}
