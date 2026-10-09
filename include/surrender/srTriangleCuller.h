#pragma once

#include "srHeap.h"
#include "srMath.h"

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srTriangleCuller {
public:
    struct Output {
        w8_ulong* indices;
        w8_ulong* avt;
        w8_ulong* vertex_remap;
        w8_ulong triangle_count;
        w8_ulong vertex_count;
        int linear;
    };

    struct Input {
        w8_ulong triangle_count;
        w8_ulong vertex_count;
        w8_ulong active_triangle_count;
        int cull_mode;
        const w8_ulong* active_triangles;
        const srVector4T<float>* projected_vertices;
        const srVector3i* triangles;
        const srVector3T<float>* vertices;
        const srVector4T<float>* clip_planes;
        const srMatrix4T<float>* model_view;
        const srMatrix4T<float>* inverse_model_view;
        srMatrix4T<float>::e_scaleType scale_type;
        w8_ulong clip_mask;
    };

    /* Transforms one view-space clip plane back into object space with the inverse model-view.
       scale_type is accepted but never read. */
    static SR_DLL_IMPORT srVector4T<float>
    transformClipPlane(const srVector4T<float>& plane, const srMatrix4T<float>& matrix,
                       srMatrix4T<float>::e_scaleType scale_type);
    /* Fills distances with plane·vertex, then shifts each distance's IEEE
       sign bit into clip_flags at bit position shift. first assigns the
       flags; otherwise they are OR-merged. Returns whether any sign bit
       was set. */
    static SR_DLL_IMPORT int setClipFlags(w8_ulong* clip_flags, float* distances,
                                          const srVector3T<float>* vertices,
                                          const srVector4T<float>& plane, w8_ulong shift,
                                          w8_ulong count, int first);

    /* Sphere-vs-plane-mask test over the six axis frustum planes plus every
       set bit of mask. depth becomes the 0..1 penetration fraction when the
       sphere clips any of the six primary planes. */
    static SR_DLL_IMPORT w8_ulong getClipMask(const srVector3T<float>& center, float radius,
                                              const srVector4T<float>* planes, w8_ulong mask,
                                              float& depth);
    /* Builds the active-vertex table: marks the vertices referenced by the
       surviving triangle indices, collects their indices into avt, then
       rewrites vertex_scratch as the vertex->avt inverse remap. Returns the
       active vertex count. */
    static SR_DLL_IMPORT w8_ulong buildAVT(w8_ulong* avt, w8_ulong* vertex_scratch,
                                           const w8_ulong* indices, const srVector3i* triangles,
                                           w8_ulong triangle_count, w8_ulong vertex_count);
    static SR_DLL_IMPORT void setupLinearArray(w8_ulong* indices, w8_ulong count);
    static SR_DLL_IMPORT int cull(Output& output, const Input& input);

private:
    /* Chunks vertices by 0x100, runs setClipFlags per plane/bit pair, and
       ANDs the per-vertex masks. shared_out ORs each plane's any-outside
       result. Returns 0 when every vertex shares an outside bit (fully
       clipped). */
    static int setClipFlagsObjectSpace(w8_ulong* clip_flags, const srVector3T<float>* vertices,
                                       const srVector4T<float>* planes, const w8_ulong* plane_bits,
                                       w8_ulong plane_count, w8_ulong vertex_count,
                                       w8_ulong& shared_out);
    /* Collects first+index of each negative distance into indices; returns
       the collected count. */
    static w8_ulong collectNegative(w8_ulong* indices, const float* distances, w8_ulong first,
                                    w8_ulong count);
    static w8_ulong cullNoClip(w8_ulong* indices, const srVector4T<float>* projected,
                               const srVector4T<float>& constant, w8_ulong count);
    static w8_ulong cullNoClipAPT(w8_ulong* indices, const w8_ulong* active,
                                  const srVector4T<float>* projected,
                                  const srVector4T<float>& constant, w8_ulong count);
    /* Filters the index list in place, keeping triangles whose three vertex
       clip masks share no set bit. */
    static w8_ulong collectCF(w8_ulong* indices, const w8_ulong* clip_flags,
                              const srVector3i* triangles, w8_ulong count);
    static w8_ulong cullClip(w8_ulong* indices, const w8_ulong* clip_flags,
                             const srVector4T<float>* projected, const srVector3i* triangles,
                             const srVector4T<float>& constant, w8_ulong count);
    /* Same keep-test as collectCF but walks the triangle table directly. */
    static w8_ulong clip(w8_ulong* indices, const w8_ulong* clip_flags, const srVector3i* triangles,
                         w8_ulong count);
};

W8_ABI_ASSERT(sizeof(srTriangleCuller::Output) == 0x18, "srTriangleCuller_Output_must_be_0x18");
W8_ABI_ASSERT(sizeof(srTriangleCuller::Input) == 0x34, "srTriangleCuller_Input_must_be_0x34");
