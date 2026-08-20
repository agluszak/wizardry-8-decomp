#ifndef WIZ8_GEOMETRY_H
#define WIZ8_GEOMETRY_H

#include "surrender/srMath.h"

struct W8GDSurface {
    unsigned int flags_00;
    unsigned int index_04;
    unsigned char positional_08[0x10];
    int vertex_indices_18[3];
    float plane_24[4];
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

void ClassifySurfacePlane004498C0(
    const srVector3T<float>* vertices, W8GDSurface* surface);
void BuildTrianglePlane00449A40(
    float* plane,
    const srVector3T<float>* first,
    const srVector3T<float>* second,
    const srVector3T<float>* third);
unsigned char PointInsideBounds004BE870(
    const srVector3T<float>* point,
    const srVector3T<float>* minimum,
    const srVector3T<float>* maximum);

#endif
