#include "wiz8/geometry.h"

/* Geometry bounds helper. Live query: 0x004BE870 sits between the proved
   Engine Code\quad.cpp (upper 0x004BE200) and Engine Code\Monster.cpp
   (lower 0x004BF0F0) intervals. */

// FUNCTION: WIZ8 0x004BE870
unsigned char PointInsideBounds004BE870(const srVector3T<float>* point,
                                        const srVector3T<float>* minimum,
                                        const srVector3T<float>* maximum)
{
    if (point->x >= minimum->x && point->x <= maximum->x && point->y >= minimum->y &&
        point->y <= maximum->y && point->z >= minimum->z && point->z <= maximum->z) {
        return 1;
    }
    return 0;
}
