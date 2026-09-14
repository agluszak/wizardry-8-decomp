#include "wiz8/geometry.h"
#include "wiz8/engine_code/GDCamera.h"

#include <float.h>
#include "wiz8/engine_code/GameData.h"

/* Unresolved fragment in the gap between the proved Engine Code\quad.cpp
   (upper 0x004BE200) and Engine Code\Monster.cpp (lower 0x004BF0F0)
   intervals. */

// FUNCTION: WIZ8 0x004BE5C0
float GetCameraFacingYaw004BE5C0(srVector3T<float>* position)
{
    float position_x = position->x;
    float position_z = position->z;
    srVector3T<float> camera_position;

    GetCameraPosition(&camera_position);
    if (camera_position.z - position_z == g_float_005ebb34) {
        if (g_float_005ebb34 < camera_position.x - position_x) {
            return g_camera_half_pi_005ec3fc;
        }
        return g_float_005ed1e8;
    }
    {
        float angle = static_cast<float>(
            atan2(camera_position.x - position_x, camera_position.z - position_z));
        if (!_finite(angle)) {
            return g_float_005ebb34;
        }
        return angle;
    }
}

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

// FUNCTION: WIZ8 0x004BE8D0
unsigned char BoundsOverlap004BE8D0(const srVector3T<float>* first_minimum,
                                    const srVector3T<float>* first_maximum,
                                    const srVector3T<float>* second_minimum,
                                    const srVector3T<float>* second_maximum)
{
    if (first_maximum->x >= second_minimum->x && second_maximum->x >= first_minimum->x &&
        first_maximum->y >= second_minimum->y && second_maximum->y >= first_minimum->y &&
        first_maximum->z >= second_minimum->z && second_maximum->z >= first_minimum->z) {
        return 1;
    }
    return 0;
}
