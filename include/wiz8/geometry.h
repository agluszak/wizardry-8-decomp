#ifndef WIZ8_GEOMETRY_H
#define WIZ8_GEOMETRY_H

#include "surrender/srMath.h"
#include "wiz8/float_constants.h"

#include <math.h>

/* Physical-surface flags corroborated by both retail consumers and Cosmic
   Forge's physical-face editor. Other bits stay unnamed until their retail
   behavior is independently established. */
enum W8GDSurfaceFlags {
    W8_GD_SURFACE_WALKABLE = 0x00000004,
    W8_GD_SURFACE_PATHFINDING = 0x00000040,
};

struct W8GDSurface {
    unsigned int flags_00;
    unsigned int index_04;
    int trigger_index_08;
    int positional_0c;
    int positional_10;
    int positional_14;
    int vertex_indices_18[3];
    union {
        float plane_24[4];
        struct {
            float normal_24[3];
            unsigned short positional_30;
            unsigned short region_32;
        };
    };
    float distance_34;
    unsigned int value_38;
    unsigned char footstep_surface_3c;  /* W8FootstepSurface selector */
    unsigned char footstep_material_3d; /* W8FootstepMaterial selector */
    unsigned char positional_3e[2];
    float value_40;
    unsigned int positional_44;
    float slope_48; /* face slope; generated surfaces derive it from normal_24[1] */
};

static_assert(sizeof(W8GDSurface) == 0x4c, "W8GDSurface_must_be_0x4c");

/* Newell cyclic normal plus centroid plane distance. Independent TUs:
   GDFileIO BuildTrianglePlane 0x00449A40 and 3d BuildPlaneFromPoints
   0x0046D660. Retail instruction streams match after register renaming in
   the Newell and distance loops; the three-point copy lowers as unrolled
   vector assignment in one TU and a component countdown in the other.
   Counted fors are the authored form. No Wiz8 COMDAT. */
inline void SetPlaneFromThreePoints(float* plane, const srVector3T<float>* first,
                                    const srVector3T<float>* second, const srVector3T<float>* third)
{
    srVector3T<float> vertices[3];
    vertices[0] = *first;
    vertices[1] = *second;
    vertices[2] = *third;

    plane[0] = 0.0f;
    plane[1] = 0.0f;
    plane[2] = 0.0f;
    plane[3] = 0.0f;

    for (int index = 0; index < 3; ++index) {
        const srVector3T<float>& current = vertices[index];
        const srVector3T<float>& next = vertices[(index + 1) % 3];
        const srVector3T<float>& previous = vertices[(index + 2) % 3];
        plane[0] += current.y * (next.z - previous.z);
        plane[1] += current.z * (next.x - previous.x);
        plane[2] += current.x * (next.y - previous.y);
    }

    float scale =
        g_float_005ebb38 /
        (float)sqrt((double)(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]));
    plane[0] *= scale;
    plane[1] *= scale;
    plane[2] *= scale;

    float distances[3];
    for (int vertex_index = 0; vertex_index < 3; ++vertex_index) {
        distances[vertex_index] = plane[0] * vertices[vertex_index].x +
                                  plane[1] * vertices[vertex_index].y +
                                  plane[2] * vertices[vertex_index].z;
    }
    plane[3] = (distances[0] + distances[1] + distances[2]) * g_float_005ec1a8;
}

/* Signed plane distance n·p + w. Independent TUs: 3d.cpp PointInsideFrustum
   0x0046D880 and stLight ContainsPoint 0x0049E460. No Wiz8 COMDAT. */
inline float SignedPlaneDistance(const srVector4T<float>& plane, const srVector3T<float>& point)
{
    return plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w;
}

/* Rotation keyframes interpolate through a quaternion: the scalar term is
   first and the imaginary part shares the srVector3T operators. Retail
   callers share the out-of-line matrix conversion at 0x0044ECA0. */
class W8Quaternion {
public:
    W8Quaternion* SetFromMatrix(const srMatrix3T<float>& matrix); /* 0x0044ECA0 */

    float w;
    srVector3T<float> v;
};

void ClassifySurfacePlane004498C0(const srVector3T<float>* vertices, W8GDSurface* surface);
void BuildTrianglePlane00449A40(float* plane, const srVector3T<float>* first,
                                const srVector3T<float>* second, const srVector3T<float>* third);
float GetCameraFacingYaw004BE5C0(srVector3T<float>* position);
/* Euclidean distance between two world-space points. */
float DistanceBetweenPoints004BE6D0(const srVector3T<float>* first,
                                    const srVector3T<float>* second);
unsigned char PointInsideBounds004BE870(const srVector3T<float>* point,
                                        const srVector3T<float>* minimum,
                                        const srVector3T<float>* maximum);
/* Per-axis overlap test for two axis-aligned bounds. */
unsigned char BoundsOverlap004BE8D0(const srVector3T<float>* first_minimum,
                                    const srVector3T<float>* first_maximum,
                                    const srVector3T<float>* second_minimum,
                                    const srVector3T<float>* second_maximum);

#endif
