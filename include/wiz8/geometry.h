#ifndef WIZ8_GEOMETRY_H
#define WIZ8_GEOMETRY_H

#include "surrender/srMath.h"
#include "wiz8/float_constants.h"

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
    unsigned int positional_34;
    unsigned int value_38;
    unsigned char surface_flag_3c;
    unsigned char vertex_flag_3d;
    unsigned char positional_3e[2];
    float value_40;
    unsigned int positional_44;
    float value_48;
};

static_assert(sizeof(W8GDSurface) == 0x4c, "W8GDSurface_must_be_0x4c");

/* Newell normal of a triangle plus centroid plane distance. Counted loops
   copy the three vertices and accumulate the cyclic sum; both GDFileIO
   0x00449A40 and 3d.cpp 0x0046D660 expand this helper. */
inline void SetPlaneFromThreePoints(float* plane, const srVector3T<float>* first,
                                    const srVector3T<float>* second, const srVector3T<float>* third)
{
    srVector3T<float> vertices[3];
    short index = 2;

    vertices[0] = *first;
    vertices[1] = *second;
    vertices[2] = *third;
    plane[0] = 0.0f;
    plane[1] = 0.0f;
    plane[2] = 0.0f;
    plane[3] = 0.0f;

    do {
        short next = (short)((index - 1) % 3);
        short following = (short)(index % 3);
        srVector3T<float>& vertex = vertices[index - 2];

        plane[0] += vertex.y * (vertices[next].z - vertices[following].z);
        plane[1] += vertex.z * (vertices[next].x - vertices[following].x);
        plane[2] += vertex.x * (vertices[next].y - vertices[following].y);
        ++index;
    } while ((short)(index - 2) < 3);

    float scale = g_float_005ebb38 /
                  (float)sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
    plane[0] *= scale;
    plane[1] *= scale;
    plane[2] *= scale;

    float distances[3];
    for (int vertex_index = 0; vertex_index != 3; ++vertex_index) {
        distances[vertex_index] = plane[0] * vertices[vertex_index].x +
                                  plane[1] * vertices[vertex_index].y +
                                  plane[2] * vertices[vertex_index].z;
    }
    plane[3] = (distances[0] + distances[1] + distances[2]) * g_float_005ec1a8;
}

void ClassifySurfacePlane004498C0(const srVector3T<float>* vertices, W8GDSurface* surface);
void BuildTrianglePlane00449A40(float* plane, const srVector3T<float>* first,
                                const srVector3T<float>* second, const srVector3T<float>* third);
unsigned char PointInsideBounds004BE870(const srVector3T<float>* point,
                                        const srVector3T<float>* minimum,
                                        const srVector3T<float>* maximum);

#endif
