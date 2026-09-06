#include "wiz8/geometry.h"

/* Geometry bounds helper. Its original translation-unit spelling is not
   established; the body lies between the proved Engine Code\quad.cpp and
   Engine Code\Monster.cpp intervals. */

// FUNCTION: WIZ8 0x004BE870
unsigned char PointInsideBounds004BE870(
    const srVector3T<float>* point,
    const srVector3T<float>* minimum,
    const srVector3T<float>* maximum)
{
    if (point->x >= minimum->x && point->x <= maximum->x &&
        point->y >= minimum->y && point->y <= maximum->y &&
        point->z >= minimum->z && point->z <= maximum->z) {
        return 1;
    }
    return 0;
}
