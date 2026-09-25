/* Engine Code\PolyPick.cpp: the demo's __FILE__ path sits in .data between
   "Engine Code\quad.cpp" and "Engine Code\Monster.cpp", matching this
   translation unit's retail .text span between the proved quad.cpp hull
   (upper 0x004BE420) and Monster.cpp's W8MonsterRep (lower 0x004BEA20).
   The demo string cluster carries "ERROR: Suppressed bad float value in
   ElevationToTargetCPP", naming the camera-relative elevation helper. */

#include "wiz8/engine_code/PolyPick.h"

#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"

#include "surrender/srCamera.h"

#include <float.h>
#include <math.h>

/* 0x005ED1E8: the negative half turn the axis-aligned heading case returns. */
// GLOBAL: WIZ8 0x005ed1e8
const float g_float_005ed1e8 = -1.570796012878418f;

/* 0x004BE420: the heading angle from source to target. The angle is measured
   in the x/z plane; an exactly axis-aligned target answers one of the two
   half turns instead of the atan2 of a zero denominator. */
// FUNCTION: WIZ8 0x004BE420
float GetHeadingAngle(const srVector3T<float>* source, const srVector3T<float>* target)
{
    float x = target->x - source->x;
    float z = target->z - source->z;

    if (z == g_float_005ebb34) {
        if (x > g_float_005ebb34) {
            return g_camera_half_pi_005ec3fc;
        }
        return g_float_005ed1e8;
    }
    {
        float angle = static_cast<float>(atan2(x, z));
        if (!_finite(angle)) {
            return g_float_005ebb34;
        }
        return angle;
    }
}

/* 0x004BE490: the elevation from source to target against the horizontal.
   acos of the vertical fraction is measured from straight up, so the half
   turn comes off it to put the horizon at zero. */
// FUNCTION: WIZ8 0x004BE490
float GetElevationAngle(const srVector3T<float>* source, const srVector3T<float>* target)
{
    srVector3T<float> delta = *target - *source;
    float length = delta.Length();
    float angle;

    if (length == g_float_005ebb34) {
        return g_float_005ebb34;
    }
    angle = static_cast<float>(acos(delta.y / length));
    if (!_finite(angle)) {
        return g_float_005ebb34;
    }
    return angle - g_float_005ec2a8;
}

/* 0x004BE520: GetElevationAngle evaluated from the camera to the target;
   the demo string pool names it ElevationToTargetCPP. */
// FUNCTION: WIZ8 0x004BE520
float ElevationToTargetCPP(const srVector3T<float>* target)
{
    srVector3T<float> position;
    GetCameraPosition(&position);
    srVector3T<float> delta = position - *target;
    float length = delta.Length();
    float angle;

    if (length == g_float_005ebb34) {
        return g_float_005ebb34;
    }
    angle = static_cast<float>(acos(delta.y / length));
    if (!_finite(angle)) {
        return g_float_005ebb34;
    }
    return angle - g_float_005ec2a8;
}

// FUNCTION: WIZ8 0x004BE5C0
float GetCameraFacingYaw(const srVector3T<float>* position)
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

/* 0x004BE650: GetHeadingAngle evaluated from the camera to the target; the
   sibling of ElevationToTargetCPP. */
// FUNCTION: WIZ8 0x004BE650
float HeadingToTargetCPP(const srVector3T<float>* target)
{
    srVector3T<float> position;
    GetCameraPosition(&position);
    float x = position.x - target->x;
    float z = position.z - target->z;

    if (z == g_float_005ebb34) {
        if (x > g_float_005ebb34) {
            return g_camera_half_pi_005ec3fc;
        }
        return g_float_005ed1e8;
    }
    {
        float angle = static_cast<float>(atan2(x, z));
        if (!_finite(angle)) {
            return g_float_005ebb34;
        }
        return angle;
    }
}

/* Euclidean distance between two world-space points. The retail callers at
   0x004F7577 and 0x004F759D are both in the ItemManager.cpp interval. */
// FUNCTION: WIZ8 0x004BE6D0
float DistanceBetweenPoints(const srVector3T<float>* first, const srVector3T<float>* second)
{
    srVector3T<float> delta = *first - *second;
    return delta.Length();
}

/* The monster twin of W8Item::DistanceToCamera - animation bounds for the Z
   offset and the monster's GrCycle-tail rep for the centre. Used by the
   monster-list scans in Targeting.cpp. */
// FUNCTION: WIZ8 0x004BE710
float MonsterDistanceToCamera(W8World* world, W8Monster* monster)
{
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    if (!monster->GetAnimationBounds(&minimum, &maximum)) {
        return -1.0f;
    }
    srVector3T<double> camera = world->camera->getLocation();
    srVector3T<float> camera_location;
    camera_location.SetFromDouble(&camera);
    srVector3T<float> center = monster->m_pRep->location_004;
    center.z += (maximum.z - minimum.z) * 0.5f;
    return (camera_location - center).Length();
}

/* The free-function twin of W8Item::DistanceToCamera - same bounds lookup and
   Z-offset centre, taking the item pointer explicitly for the item-list
   scans in ItemManager.cpp. */
// FUNCTION: WIZ8 0x004BE7C0
float ItemDistanceToCamera(W8World* world, W8Item* item)
{
    srVector3T<float> lower;
    srVector3T<float> upper;
    if (!item->GetCachedLocalBounds(&lower, &upper)) {
        return -1.0f;
    }
    srVector3T<double> camera = world->camera->getLocation();
    srVector3T<float> camera_location;
    camera_location.SetFromDouble(&camera);
    srVector3T<float> center = item->m_pRep->location_004;
    center.z += (upper.z - lower.z) * 0.5f;
    return (camera_location - center).Length();
}

// FUNCTION: WIZ8 0x004BE870
bool PointInsideBounds004BE870(const srVector3T<float>* point, const srVector3T<float>* minimum,
                               const srVector3T<float>* maximum)
{
    if (point->x >= minimum->x && point->x <= maximum->x && point->y >= minimum->y &&
        point->y <= maximum->y && point->z >= minimum->z && point->z <= maximum->z) {
        return 1;
    }
    return 0;
}

/* Per-axis overlap test for two axis-aligned bounds. */
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

/* Project one world point through the active world's camera and report
   whether it stays in front of it. */
// FUNCTION: WIZ8 0x004BE940
unsigned char ProjectPointThroughCamera(const srVector3T<float>* position)
{
    srVector3T<float> projected;
    srVector3T<double> input((double)position->x, (double)position->y, (double)position->z);

    return g_world->camera->project(projected, input) == srCamera::PROJECTION_RESULT_POSITIONAL_0;
}
