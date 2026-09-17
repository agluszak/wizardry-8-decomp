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

    static inline void copy(SRDWORD* destination, SRDWORD constant, SRDWORD count)
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

    static inline void copy(srVector4* destination, const srVector4& constant, SRDWORD count)
    {
        vp->_copy(destination, constant, count);
    }

    static inline void copy(srVector4* destination, const srVector3* source, const float* w,
                            SRDWORD count)
    {
        vp->_copy(destination, source, w, count);
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

    static inline void mul(float* destination, float constant, const float* source, SRDWORD count)
    {
        vp->_mul(destination, constant, source, count);
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
};
