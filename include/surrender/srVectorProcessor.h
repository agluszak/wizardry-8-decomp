#pragma once

#include <ostream>

#include "srVP.h"

class srDebugVP;

/* The private imported implementation pointer is exposed to clients by this
   ordinary header-visible facade; the direct load and virtual call are visible
   in the recovered retail clients, and no exported wrapper exists. The public
   spelling is inferred from the named `_minMax` implementation boundary.

   Extra inlines below are the srVP slots Wiz8 actually reaches through the
   imported vp global (IAT 0x005eb7e8): load vp, then CALL [vtable+offset].
   Confirmed Wiz8 loaders include FUN_0046e8a0, FUN_00470040,
   CopyDwordBuffer00470180,
   FUN_00471ad0, FUN_00472270, FUN_004729f0, FUN_00473190, FUN_00474700,
   FUN_00474730, FlushSlots00475600, FUN_0047f930, FUN_00486970,
   PrepareGeometry004B6F30, GDProp::Initialize, FUN_00580270 and FUN_005809f0.
   Offsets +0x210/+0x218/+0x224 are the srVector3 `_length`, `_normalize`
   and `_transform` slots. FillDwordBuffer00474700 / AddFloatBuffer00474730 call the dword
   `_copy` and float `_add` overloads; both compare exact against retail
   CALLIND +0x38 / +0xd8. */
class srVectorProcessor {
public:
    static SR_DLL_IMPORT const char* getName();
    /* startDebug wraps the active processor in an srDebugVP and makes vp
       point at it while debug_active is set; its argument enables the
       wrapper's pointer-alignment counters. endDebug restores the base
       processor and destroys the wrapper. dump prints the wrapper's
       per-command statistics and resetStatistics clears them. */
    static SR_DLL_IMPORT void startDebug(int check_misalignments);
    static SR_DLL_IMPORT void endDebug();
    static SR_DLL_IMPORT void dump(std::ostream& stream);
    static SR_DLL_IMPORT void resetStatistics();
    static SR_DLL_IMPORT int load(const char* filename);
    static SR_DLL_IMPORT void initBaseVP();
    static SR_DLL_IMPORT long getID(const char* filename);
    static SR_DLL_IMPORT int loadBest(const char* path);
    static SR_DLL_IMPORT void release();

    static inline void memcopy(void* destination, const void* source, SRDWORD bytes)
    {
        vp->_memcopy(destination, source, bytes);
    }

    static inline void memcopy(void* destination, SRBYTE source, SRDWORD bytes)
    {
        vp->_memcopy(destination, source, bytes);
    }

    /* Retail also emits a guarded variant at 0x10027BA0 that skips the call
       when count is zero. */
    // FUNCTION: SURRENDER 0x10027BC0 SYMBOL
    // ?copy@srVectorProcessor@@SAXPAKKK@Z
    static inline void copy(SRDWORD* destination, SRDWORD constant, SRDWORD count)
    {
        vp->_copy(destination, constant, count);
    }

    static inline void copy(srVector2* destination, const srVector2& constant, SRDWORD count)
    {
        vp->_copy(destination, constant, count);
    }

    static inline void copy(srVector3* destination, const srVector3& constant, SRDWORD count)
    {
        vp->_copy(destination, constant, count);
    }

    static inline void copyIndexed(SRDWORD* destination, const SRDWORD* source,
                                   const SRDWORD* indices, SRDWORD count)
    {
        vp->_copyIndexed(destination, source, indices, count);
    }

    static inline void copyIndexed(srVector3* destination, const srVector3* source,
                                   const SRDWORD* indices, SRDWORD count)
    {
        vp->_copyIndexed(destination, source, indices, count);
    }

    /* srVertexPipe's record paths dispatch the vertex4 sources through the
       three-vector4 copyIndexed overloads at vtable +0x84/+0x88/+0x8c. */
    static inline void copyIndexed(srVector4* destination, const srARGB* source,
                                   const SRDWORD* indices, SRDWORD count)
    {
        vp->_copyIndexed(destination, source, indices, count);
    }

    static inline void copyIndexed(srVector4* destination, const srVector4* source,
                                   const SRDWORD* indices, SRDWORD count)
    {
        vp->_copyIndexed(destination, source, indices, count);
    }

    static inline void copyIndexed(srVector4* destination, const srVector3* source,
                                   const SRDWORD* indices, SRDWORD count)
    {
        vp->_copyIndexed(destination, source, indices, count);
    }

    static inline void copy(srVector4* destination, const srVector4& constant, SRDWORD count)
    {
        vp->_copy(destination, constant, count);
    }

    static inline void copy(srVector4* destination, const srVector3* source, const float* w,
                            SRDWORD count)
    {
        vp->_copy(destination, source, w, count);
    }

    /* srFog::process adds a scalar fog offset through vtable +0xdc. */
    static inline void add(float* destination, float constant, const float* source, SRDWORD count)
    {
        vp->_add(destination, constant, source, count);
    }

    static inline void add(float* destination, const float* source_0, const float* source_1,
                           SRDWORD count)
    {
        vp->_add(destination, source_0, source_1, count);
    }

    static inline void add(srVector4* destination, const srVector4& constant,
                           const float* float_source, SRDWORD count)
    {
        vp->_add(destination, constant, float_source, count);
    }

    /* srVertexPipe adds a constant color into a diffuse/specular vector run
       through the srVector4-source overload. */
    static inline void add(srVector4* destination, const srVector4& constant,
                           const srVector4* vector_source, SRDWORD count)
    {
        vp->_add(destination, constant, vector_source, count);
    }

    static inline void add(srVector4* destination, const srVector4* vector_source,
                           const float* float_source, SRDWORD count)
    {
        vp->_add(destination, vector_source, float_source, count);
    }

    /* dest[i] = source[i] + constant for `count` vectors; retail callers reach
       the srVector3 constant overload at vtable +0xD4. */
    static inline void add(srVector3* destination, const srVector3& constant,
                           const srVector3* vector_source, SRDWORD count)
    {
        vp->_add(destination, constant, vector_source, count);
    }

    /* dest[i] = source[i] + (target[i] - source[i]) * constant; the Wiz8 mesh
       code lerps raw float triples through vtable +0x178. */
    static inline void lerp(float* destination, const float* target, const float* source,
                            float constant, SRDWORD count)
    {
        vp->_lerp(destination, target, source, constant, count);
    }

    /* dest[i] = matrix * vectors[i] for `count` vectors through vtable +0x224. */
    static inline void transform(srVector3* destination, const srVector3* vectors,
                                 const srMatrix4& matrix, SRDWORD count)
    {
        vp->_transform(destination, vectors, matrix, count);
    }

    /* srGERD::Renderer's batch ingress dispatches the projection matrix's
       detected mode: the vec4 transform through +0x220, ortho through
       +0x23c, perspective through +0x240, the clip-flag byte stream
       through +0x298, and the gather+remap triangle copy through +0x25c. */
    static inline void transform(srVector4* destination, const srVector4* vectors,
                                 const srMatrix4& matrix, SRDWORD count)
    {
        vp->_transform(destination, vectors, matrix, count);
    }

    static inline void transformOrtho(srVector4* destination, const srVector4* vectors,
                                      const srMatrix4& matrix, SRDWORD count)
    {
        vp->_transformOrtho(destination, vectors, matrix, count);
    }

    static inline void transformPerspective(srVector4* destination, const srVector4* vectors,
                                            const srMatrix4& matrix, SRDWORD count)
    {
        vp->_transformPerspective(destination, vectors, matrix, count);
    }

    static inline void srGetClipFlags(SRBYTE* destination, const srVector4* source, SRDWORD count)
    {
        vp->_srGetClipFlags(destination, source, count);
    }

    static inline void srCopyIndexedRemap(srVector3i* destination, const srVector3i* source,
                                          const SRDWORD* indices, const SRDWORD* remap,
                                          SRDWORD count)
    {
        vp->_srCopyIndexedRemap(destination, source, indices, remap, count);
    }

    /* The vec2 indexed copy the dedup path uses for both texture-coordinate
       streams. */
    static inline void copyIndexed(srVector2* destination, const srVector2* source,
                                   const SRDWORD* indices, SRDWORD count)
    {
        vp->_copyIndexed(destination, source, indices, count);
    }

    /* destination[i] = |vectors[i]| for `count` vectors through vtable
       +0x210; the automap cell lighting uses it for per-vertex distances. */
    static inline void length(float* destination, const srVector3* vectors, SRDWORD count)
    {
        vp->_length(destination, vectors, count);
    }

    static inline void mul(float* destination, float constant, const float* source, SRDWORD count)
    {
        vp->_mul(destination, constant, source, count);
    }

    /* srLight::process multiplies work arrays elementwise through +0x12c and
       subtracts a constant through +0x104. */
    static inline void mul(float* destination, const float* source_0, const float* source_1,
                           SRDWORD count)
    {
        vp->_mul(destination, source_0, source_1, count);
    }

    static inline void sub(float* destination, float constant, const float* source, SRDWORD count)
    {
        vp->_sub(destination, constant, source, count);
    }

    /* srLight::process subtracts the per-vertex view directions from the half
       vectors elementwise through +0x100. */
    static inline void sub(float* destination, const float* source_0, const float* source_1,
                           SRDWORD count)
    {
        vp->_sub(destination, source_0, source_1, count);
    }

    /* Light direction vectors: eye-space positions copied out of the vertex
       array (+0x2c), then rebased to the light (+0xFC), normalized with the
       lengths kept (+0x22C). */
    static inline void copy(srVector3* destination, const srVector4* source, SRDWORD count)
    {
        vp->_copy(destination, source, count);
    }

    static inline void sub(srVector3* destination, const srVector3& constant,
                           const srVector3* vector_source, SRDWORD count)
    {
        vp->_sub(destination, constant, vector_source, count);
    }

    static inline void dir(srVector3* destination, float* lengths, const srVector3* source,
                           SRDWORD count)
    {
        vp->_dir(destination, lengths, source, count);
    }

    static inline void dot(float* destination, const srVector3& constant, const srVector3* vectors,
                           SRDWORD count)
    {
        vp->_dot(destination, constant, vectors, count);
    }

    static inline void dot(float* destination, const srVector3* vectors_0,
                           const srVector3* vectors_1, SRDWORD count)
    {
        vp->_dot(destination, vectors_0, vectors_1, count);
    }

    /* Attenuation evaluation: 1/(poly.x + poly.y*d + poly.z*d^2) through
       +0x1E4, lower clamp through +0x164, all-zero test through +0x184, and
       the specular power through +0x258. */
    static inline void invPoly(float* destination, const float* source, const srVector3& poly,
                               SRDWORD count)
    {
        vp->_invPoly(destination, source, poly, count);
    }

    static inline void clampMin(float* destination, const float* source, float minimum,
                                SRDWORD count)
    {
        vp->_clampMin(destination, source, minimum, count);
    }

    static inline int isZero(const float* source, SRDWORD count)
    {
        return vp->_isZero(source, count);
    }

    /* srGERD::Renderer::drawImmediate tests whether the whole batch shares
       one texture set through the constant overload at vtable +0x70. */
    static inline int isEqual(const SRDWORD* source, SRDWORD constant, SRDWORD count)
    {
        return vp->_isEqual(source, constant, count);
    }

    static inline void srSpecularPow(float* destination, const float* source, float exponent,
                                     SRDWORD count)
    {
        vp->_srSpecularPow(destination, source, exponent, count);
    }

    /* srVertexPipe's finish/setup bodies reach these slots through vp. */
    static inline void swap(void* first, void* second, SRDWORD bytes)
    {
        vp->_swap(first, second, bytes);
    }

    /* srColorSurfaceIFace::flipRectangle mirrors a converted ARGB row run
       through the dword reverse slot (+0x3c). */
    static inline void reverse(SRDWORD* destination, const SRDWORD* source, SRDWORD count)
    {
        vp->_reverse(destination, source, count);
    }

    static inline void neg(float* destination, const float* source, SRDWORD count)
    {
        vp->_neg(destination, source, count);
    }

    static inline void axpy(float* destination, const float* add_source, const float* scale_source,
                            const float* multiply_source, SRDWORD count)
    {
        vp->_axpy(destination, add_source, scale_source, multiply_source, count);
    }

    static inline void mulIndexed(srVector4* destination, const srVector4& constant,
                                  const srVector4* indexed_source, const SRDWORD* indices,
                                  SRDWORD count)
    {
        vp->_mulIndexed(destination, constant, indexed_source, indices, count);
    }

    /* finishDiffuseAlpha/finishSpecularFog fold an indexed specular array into
       a linear destination run through the linear-source overload. */
    static inline void mulIndexed(srVector4* destination, const srVector4* linear_source,
                                  const srVector4* indexed_source, const SRDWORD* indices,
                                  SRDWORD count)
    {
        vp->_mulIndexed(destination, linear_source, indexed_source, indices, count);
    }

    /* Per-vertex light accumulation into the output rows: destination[i] +=
       constant * source[i] ( * source_1[i] ) through +0x1A4/+0x198. */
    static inline void axpy(srVector4* destination, const srVector4* add_source,
                            const srVector4& multiply_constant, const float* multiply_source,
                            SRDWORD count)
    {
        vp->_axpy(destination, add_source, multiply_constant, multiply_source, count);
    }

    static inline void axpy(srVector4* destination, const srVector4* add_source,
                            const srVector4& multiply_constant, const float* multiply_source_0,
                            const float* multiply_source_1, SRDWORD count)
    {
        vp->_axpy(destination, add_source, multiply_constant, multiply_source_0, multiply_source_1,
                  count);
    }

    static inline void mul(srVector3* destination, const srVector3& constant,
                           const srVector3* vector_source, SRDWORD count)
    {
        vp->_mul(destination, constant, vector_source, count);
    }

    static inline void mul(srVector3* destination, const srVector3* vector_source,
                           const float* float_source, SRDWORD count)
    {
        vp->_mul(destination, vector_source, float_source, count);
    }

    static inline void mul(srVector4* destination, const srVector4& constant,
                           const float* float_source, SRDWORD count)
    {
        vp->_mul(destination, constant, float_source, count);
    }

    static inline void normalize(srVector3* destination, const srVector3* vectors, float length,
                                 SRDWORD count)
    {
        vp->_normalize(destination, vectors, length, count);
    }

    /* srFog::process clamps the computed fog factors through vtable +0x16c. */
    static inline void clampUnit(float* destination, const float* source, SRDWORD count)
    {
        vp->_clampUnit(destination, source, count);
    }

    static inline void minMax(const srVector3* source, srVector3& minimum, srVector3& maximum,
                              SRDWORD count)
    {
        vp->_minMax(source, minimum, maximum, count);
    }

    static inline void minMax(const srVector4* source, srVector4& minimum, srVector4& maximum,
                              SRDWORD count)
    {
        vp->_minMax(source, minimum, maximum, count);
    }

private:
    /* srMaterial::postProcess dispatches the per-vertex blend through vp;
       srCore::dump reads it directly for the Vector Processor report line. */
    friend class srMaterial;
    friend class srCore;
    static void install(srVP* processor);
    // GLOBAL: SURRENDER 0x100A923C
    static SR_DLL_IMPORT srVP* vp;
    /* vp is the processor clients dispatch through. While debug_active is
       set vp points at debug, the srDebugVP wrapper; otherwise vp and base
       are the same installed processor. module is the srVP_* library handle
       load() keeps so release() can free it; it is 0 for the built-in
       processor. */
    // GLOBAL: SURRENDER 0x100A9240
    static srVP* base;
    // GLOBAL: SURRENDER 0x100A9244
    static srDebugVP* debug;
    // GLOBAL: SURRENDER 0x100A9248
    static unsigned long debug_active;
    // GLOBAL: SURRENDER 0x100A924C
    static void* module;

    /* srVertexPipe::process snapshots the active processor into its own
       vector_processor_98 and dispatches vtable slots through it. */
    friend class srVertexPipe;
    /* srGERD::testBoundingBox dispatches the processor's bounding-box slot
       through vp (IAT 0x005eb7e8). */
    friend class srGERD;
    /* srTriangleCuller dispatches _dot/_dotIndexed/_srCullNoClip and the
       buildAVT scratch ops through vp from its own TU. */
    friend class srTriangleCuller;
};
