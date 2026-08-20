#pragma once

#include "srHeap.h"
#include "srMath.h"

class srTriangleCuller {
public:
    struct Output {
        unsigned long* indices_00;
        unsigned long* avt_04;
        unsigned long* clip_flags_08;
        unsigned long triangle_count_0c;
        unsigned long vertex_count_10;
        int linear_14;
    };

    struct Input {
        unsigned long triangle_count_00;
        unsigned long vertex_count_04;
        unsigned long active_triangle_count_08;
        int cull_mode_0c;
        const unsigned long* active_triangles_10;
        const srVector4T<float>* projected_vertices_14;
        const srVector3i* triangles_18;
        const srVector3T<float>* vertices_1c;
        const srVector4T<float>* clip_planes_20;
        const srMatrix4T<float>* model_view_24;
        const srMatrix4T<float>* inverse_model_view_28;
        srMatrix4T<float>::e_scaleType scale_type_2c;
        unsigned long clip_mask_30;
    };

    static SR_DLL_IMPORT int cull(Output& output, const Input& input);
    static SR_DLL_IMPORT unsigned long getClipMask(
        const srVector3T<float>& center, float radius,
        const srVector4T<float>* planes, unsigned long mask,
        float& depth);
};

static_assert(sizeof(srTriangleCuller::Output) == 0x18,
              "srTriangleCuller_Output_must_be_0x18");
static_assert(sizeof(srTriangleCuller::Input) == 0x34,
              "srTriangleCuller_Input_must_be_0x34");
