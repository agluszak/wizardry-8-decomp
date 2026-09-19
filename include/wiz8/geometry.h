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

    /* 0x0041CF90: unrecovered segment-vs-surface test used by env motion. */
    unsigned char TestSegment0041CF90(srVector3T<float>* from, const srVector3T<float>* direction,
                                      float* hit_distance, srVector3T<float>* vertices);
    /* 0x0041DC10: unrecovered collision response for a hit surface. */
    unsigned char ResolveCollision0041DC10(const srVector3T<float>* origin,
                                           const srVector3T<float>* hit_point,
                                           srVector3T<float>* direction, int collision_index);
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
    /* Short-arc slerp between two keyframe rotations expanded back into a
       rotation matrix: the copy of `to` is sign-fixed so the dot stays
       positive, a near-coincident pair falls back to a linear blend, and the
       normalized quaternion is written out through the 2/norm^2 expansion.
       Prop 0x0044C830 and PathAIApply 0x004AA520 inline the same sequence. */
    static void InterpolateRotation(const srMatrix3T<float>& from, const srMatrix3T<float>& to,
                                    double amount, srMatrix3T<float>* rotation);

    float w;
    srVector3T<float> v;
};

inline void W8Quaternion::InterpolateRotation(const srMatrix3T<float>& from,
                                              const srMatrix3T<float>& to, double amount,
                                              srMatrix3T<float>* rotation)
{
    W8Quaternion first;
    W8Quaternion second;
    W8Quaternion adjusted;
    double dot;
    double b;
    double angle;
    double sine;
    double w;
    double x;
    double y;
    double z;
    double scale;
    double sx;
    double sy;
    double sz;
    double xx;
    double xy;
    double xz;
    double yy;
    double yz;
    double zz;
    double xw;
    double yw;
    double zw;

    first.SetFromMatrix(from);
    second.SetFromMatrix(to);
    adjusted = second;
    dot = first.w * second.w + first.v.x * second.v.x + first.v.y * second.v.y +
          first.v.z * second.v.z;
    if (dot < g_zero_005ebb40) {
        dot = -dot;
        adjusted.v = -adjusted.v;
        adjusted.w = -adjusted.w;
    }
    if (g_double_005ebc30 - dot <= g_double_005ec1f0) {
        dot = g_double_005ebc30 - amount;
        b = amount;
    } else {
        angle = acos(dot);
        sine = sin(angle);
        dot = sin((g_double_005ebc30 - amount) * angle) / sine;
        b = sin(angle * amount) / sine;
    }
    adjusted.v = dot * first.v + b * adjusted.v;
    w = first.w * dot + adjusted.w * b;
    x = adjusted.v.x;
    y = adjusted.v.y;
    z = adjusted.v.z;
    scale = g_double_005ec1e8 / (w * w + x * x + y * y + z * z);
    sx = scale * x;
    sy = scale * y;
    sz = scale * z;
    xw = sx * w;
    yw = sy * w;
    zw = sz * w;
    xx = sx * x;
    xy = sx * y;
    xz = sx * z;
    yy = sy * y;
    yz = sy * z;
    zz = sz * z;
    rotation->vectors[0].x = static_cast<float>(g_double_005ebc30 - (yy + zz));
    rotation->vectors[1].x = static_cast<float>(xy + zw);
    rotation->vectors[2].x = static_cast<float>(xz - yw);
    rotation->vectors[0].y = static_cast<float>(xy - zw);
    rotation->vectors[1].y = static_cast<float>(g_double_005ebc30 - (xx + zz));
    rotation->vectors[2].y = static_cast<float>(yz + xw);
    rotation->vectors[0].z = static_cast<float>(xz + yw);
    rotation->vectors[1].z = static_cast<float>(yz - xw);
    rotation->vectors[2].z = static_cast<float>(g_double_005ebc30 - (xx + yy));
}

void ClassifySurfacePlane004498C0(const srVector3T<float>* vertices, W8GDSurface* surface);
void BuildTrianglePlane00449A40(float* plane, const srVector3T<float>* first,
                                const srVector3T<float>* second, const srVector3T<float>* third);
#endif
