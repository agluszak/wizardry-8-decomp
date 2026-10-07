#pragma once

#include <ostream>

#include "srVP.h"

class srDebugVP;

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

    // FUNCTION: SURRENDER 0x10027BC0 SYMBOL
    // RECOMP: ?copy@srVectorProcessor@@SAXPAKKK@Z
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

    // FUNCTION: SURRENDER 0x1005CC60 SYMBOL
    // RECOMP: ?copyIndexed@srVectorProcessor@@SAXPAKPBK1K@Z
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

    /* dest[i] = source[i] + constant for `count` vectors. */
    static inline void add(srVector3* destination, const srVector3& constant,
                           const srVector3* vector_source, SRDWORD count)
    {
        vp->_add(destination, constant, vector_source, count);
    }

    /* dest[i] = source[i] + (target[i] - source[i]) * constant. */
    static inline void lerp(float* destination, const float* target, const float* source,
                            float constant, SRDWORD count)
    {
        vp->_lerp(destination, target, source, constant, count);
    }

    /* dest[i] = matrix * vectors[i] for `count` vectors. */
    static inline void transform(srVector3* destination, const srVector3* vectors,
                                 const srMatrix4& matrix, SRDWORD count)
    {
        vp->_transform(destination, vectors, matrix, count);
    }

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

    /* destination[i] = |vectors[i]|. */
    static inline void length(float* destination, const srVector3* vectors, SRDWORD count)
    {
        vp->_length(destination, vectors, count);
    }

    static inline void mul(float* destination, float constant, const float* source, SRDWORD count)
    {
        vp->_mul(destination, constant, source, count);
    }

    static inline void mul(float* destination, const float* source_0, const float* source_1,
                           SRDWORD count)
    {
        vp->_mul(destination, source_0, source_1, count);
    }

    static inline void sub(float* destination, float constant, const float* source, SRDWORD count)
    {
        vp->_sub(destination, constant, source, count);
    }

    static inline void sub(float* destination, const float* source_0, const float* source_1,
                           SRDWORD count)
    {
        vp->_sub(destination, source_0, source_1, count);
    }

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

    static inline void reverse(SRDWORD* destination, const SRDWORD* source, SRDWORD count)
    {
        vp->_reverse(destination, source, count);
    }

    static inline void bitwiseAnd(SRDWORD* destination, const SRDWORD* source, SRDWORD constant,
                                  SRDWORD count)
    {
        vp->_and(destination, source, constant, count);
    }

    static inline void bitwiseOr(SRDWORD* destination, const SRDWORD* source, SRDWORD constant,
                                 SRDWORD count)
    {
        vp->_or(destination, source, constant, count);
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

    static inline void mulIndexed(srVector4* destination, const srVector4* linear_source,
                                  const srVector4* indexed_source, const SRDWORD* indices,
                                  SRDWORD count)
    {
        vp->_mulIndexed(destination, linear_source, indexed_source, indices, count);
    }

    /* destination[i] += constant * source[i] ( * source_1[i] ). */
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
       vector_processor and dispatches vtable slots through it. */
    friend class srVertexPipe;
    friend class srGERD;
    /* srTriangleCuller dispatches _dot/_dotIndexed/_srCullNoClip and the
       buildAVT scratch ops through vp from its own TU. */
    friend class srTriangleCuller;
};
