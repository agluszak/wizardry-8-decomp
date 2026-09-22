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

/* Axis-aligned minimum/maximum box used by Wizardry's runtime/build geometry.
   The region builder stores counted 0x18-byte arrays of these and the octree
   consumes one complete pair through AddCollidablePropBounds. Original source
   spelling is not recovered. */
struct W8BoundingBox {
    srVector3T<float> minimum;
    srVector3T<float> maximum;
};

static_assert(sizeof(W8BoundingBox) == 0x18, "W8BoundingBox_must_be_0x18");

struct W8GDSurface {
    unsigned int flags_00;
    unsigned int index_04;
    int trigger_index_08;
    int edge_link_0c[3];
    int vertex_indices_18[3];
    /* The first three plane coefficients are its surface normal. The region
       word at +0x32 belongs to W8OctRegionPolygon, not this surface. */
    float plane_24[4];
    float distance_34;
    /* Hit plane ProbePropsAlongMotion fills for ResolveCollision0041DC10. */
    srVector4T<float>* hit_plane_38;
    unsigned char footstep_surface_3c;  /* W8FootstepSurface selector */
    unsigned char footstep_material_3d; /* W8FootstepMaterial selector */
    unsigned char positional_3e[2];
    float contact_margin_40;
    unsigned int chance_44;
    float slope_48; /* face slope; generated surfaces derive it from plane_24[1] */

    /* The plane's leading three floats read as the surface normal. */
    const srVector3T<float>* Normal() const
    {
        // reinterpret-ok: plane_24's leading three floats are the unit normal
        return reinterpret_cast<const srVector3T<float>*>(&plane_24);
    }

    /* 0x0041CF90: segment-vs-surface test used by env motion. On a hit `from`
       advances to the contact point and `hit_distance` gets the travelled
       length; distance_34 takes the surface's updated limit. */
    unsigned char TestSegment0041CF90(srVector3T<float>* from, const srVector3T<float>* direction,
                                      float* hit_distance, srVector3T<float>* vertices);
    /* 0x0041D9D0: shrink `limit` to the remaining in-plane distance against
       the nearest triangle edge; fails when no edge improves it. */
    unsigned char ClampHitToEdge0041D9D0(const srVector3T<float>* point,
                                         const srVector3T<float>* vertices, float* limit);
    /* 0x0041DC10: collision response for a hit surface; `origin` is advanced
       to `hit_point` and `direction` is bent along the contact plane. */
    unsigned char ResolveCollision0041DC10(srVector3T<float>* origin,
                                           const srVector3T<float>* hit_point,
                                           srVector3T<float>* direction, int collision_index);
    /* 0x0041E8E0: whether moving `from` to `to` pushes this surface's
       centroid away from surface `surface_index`'s centroid. */
    unsigned char CentroidsDiverging0041E8E0(int surface_index, const srVector3T<float>* from,
                                             const srVector3T<float>* to);
    /* 0x0041EA90: environment response for a walkable contact surface;
       adjusts `direction` and the active environ record. */
    unsigned char ApplyEnvironContact0041EA90(srVector3T<float>* direction);
};

static_assert(sizeof(W8GDSurface) == 0x4c, "W8GDSurface_must_be_0x4c");

/* Newell cyclic normal plus centroid plane distance. Independent TUs:
   GDFileIO BuildTrianglePlane 0x00449A40 and 3d BuildPlaneFromPoints
   0x0046D660. Retail instruction streams match after register renaming in
   the Newell and distance loops; the three-point copy lowers as unrolled
   vector assignment in one TU and a component countdown in the other.
   Counted fors are the authored form. No Wiz8 COMDAT. */
inline void SetPlaneFromThreePoints(srVector4T<float>* plane, const srVector3T<float>* first,
                                    const srVector3T<float>* second, const srVector3T<float>* third)
{
    srVector3T<float> vertices[3];
    vertices[0] = *first;
    vertices[1] = *second;
    vertices[2] = *third;

    plane->x = 0.0f;
    plane->y = 0.0f;
    plane->z = 0.0f;
    plane->w = 0.0f;

    for (int index = 0; index < 3; ++index) {
        const srVector3T<float>& current = vertices[index];
        const srVector3T<float>& next = vertices[(index + 1) % 3];
        const srVector3T<float>& previous = vertices[(index + 2) % 3];
        plane->x += current.y * (next.z - previous.z);
        plane->y += current.z * (next.x - previous.x);
        plane->z += current.x * (next.y - previous.y);
    }

    srVector3T<float> normal(plane->x, plane->y, plane->z);
    float scale = g_float_005ebb38 / normal.Length();
    plane->x *= scale;
    plane->y *= scale;
    plane->z *= scale;

    float distances[3];
    for (int vertex_index = 0; vertex_index < 3; ++vertex_index) {
        distances[vertex_index] = plane->x * vertices[vertex_index].x +
                                  plane->y * vertices[vertex_index].y +
                                  plane->z * vertices[vertex_index].z;
    }
    plane->w = (distances[0] + distances[1] + distances[2]) * g_float_005ec1a8;
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
void BuildTrianglePlane00449A40(srVector4T<float>* plane, const srVector3T<float>* first,
                                const srVector3T<float>* second, const srVector3T<float>* third);
#endif
