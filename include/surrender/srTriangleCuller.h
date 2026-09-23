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

    /* Transforms one view-space clip plane back into object space with the
       inverse model-view: three points on the plane are transformed and the
       rebuilt (unnormalized) equation is flipped to keep the plane's
       positive side. scale_type is accepted but never read. */
    static SR_DLL_IMPORT srVector4T<float>
    transformClipPlane(const srVector4T<float>& plane, const srMatrix4T<float>& matrix,
                       srMatrix4T<float>::e_scaleType scale_type);
    /* Fills distances with plane·vertex, then shifts each distance's IEEE
       sign bit into clip_flags at bit position shift. first assigns the
       flags; otherwise they are OR-merged. Returns whether any sign bit
       was set. */
    static SR_DLL_IMPORT int setClipFlags(unsigned long* clip_flags, float* distances,
                                          const srVector3T<float>* vertices,
                                          const srVector4T<float>& plane, unsigned long shift,
                                          unsigned long count, int first);
    srTriangleCuller& operator=(const srTriangleCuller& other);
    /* Sphere-vs-plane-mask test over the six axis frustum planes plus every
       set bit of mask. depth becomes the 0..1 penetration fraction when the
       sphere clips any of the six primary planes. */
    static SR_DLL_IMPORT unsigned long getClipMask(const srVector3T<float>& center, float radius,
                                                   const srVector4T<float>* planes,
                                                   unsigned long mask, float& depth);
    /* Builds the active-vertex table: marks the vertices referenced by the
       surviving triangle indices, collects their indices into avt, then
       rewrites clip_flags as the vertex->avt inverse remap. Returns the
       active vertex count. */
    static SR_DLL_IMPORT unsigned long
    buildAVT(unsigned long* avt, unsigned long* clip_flags, const unsigned long* indices,
             const srVector3i* triangles, unsigned long triangle_count, unsigned long vertex_count);
    static SR_DLL_IMPORT void setupLinearArray(unsigned long* indices, unsigned long count);
    static SR_DLL_IMPORT int cull(Output& output, const Input& input);

private:
    /* Chunks vertices by 0x100, runs setClipFlags per plane/bit pair, and
       ANDs the per-vertex masks. shared_out ORs each plane's any-outside
       result. Returns 0 when every vertex shares an outside bit (fully
       clipped). */
    static int setClipFlagsObjectSpace(unsigned long* clip_flags, const srVector3T<float>* vertices,
                                       const srVector4T<float>* planes,
                                       const unsigned long* plane_bits, unsigned long plane_count,
                                       unsigned long vertex_count, unsigned long& shared_out);
    /* Collects first+index of each negative distance into indices; returns
       the collected count. */
    static unsigned long collectNegative(unsigned long* indices, const float* distances,
                                         unsigned long first, unsigned long count);
    static unsigned long cullNoClip(unsigned long* indices, const srVector4T<float>* projected,
                                    const srVector4T<float>& constant, unsigned long count);
    static unsigned long cullNoClipAPT(unsigned long* indices, const unsigned long* active,
                                       const srVector4T<float>* projected,
                                       const srVector4T<float>& constant, unsigned long count);
    /* Filters the index list in place, keeping triangles whose three vertex
       clip masks share no set bit. */
    static unsigned long collectCF(unsigned long* indices, const unsigned long* clip_flags,
                                   const srVector3i* triangles, unsigned long count);
    static unsigned long cullClip(unsigned long* indices, const unsigned long* clip_flags,
                                  const srVector4T<float>* projected, const srVector3i* triangles,
                                  const srVector4T<float>& constant, unsigned long count);
    /* Same keep-test as collectCF but walks the triangle table directly. */
    static unsigned long clip(unsigned long* indices, const unsigned long* clip_flags,
                              const srVector3i* triangles, unsigned long count);
};

static_assert(sizeof(srTriangleCuller::Output) == 0x18, "srTriangleCuller_Output_must_be_0x18");
static_assert(sizeof(srTriangleCuller::Input) == 0x34, "srTriangleCuller_Input_must_be_0x34");
