#pragma once

#include "srVP.h"

/* The private imported implementation pointer is exposed to clients by this
   ordinary header-visible facade; the direct load and virtual call are visible
   in the recovered retail clients, and no exported wrapper exists. The public
   spelling is inferred from the named `_minMax` implementation boundary.

   Extra inlines below are the srVP slots Wiz8 actually reaches through the
   imported vp global (IAT 0x005eb7e8): load vp, then CALL [vtable+offset]. */
class srVectorProcessor {
public:
    static SR_DLL_IMPORT const char* getName();
    static SR_DLL_IMPORT int load(const char* filename);
    static SR_DLL_IMPORT long getID(const char* filename);
    static SR_DLL_IMPORT void release();

    static inline void memcopy(void* destination, SRBYTE source, SRDWORD bytes)
    {
        vp->_memcopy(destination, source, bytes);
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
    static void install10064390(srVP* processor);
    // GLOBAL: SURRENDER 0x100A923C
    static SR_DLL_IMPORT srVP* vp;
    // Original private spellings are not exported.
    // GLOBAL: SURRENDER 0x100A9240
    static srVP* base_100a9240;
    // GLOBAL: SURRENDER 0x100A9244
    static srVP* debug_100a9244;
    // GLOBAL: SURRENDER 0x100A9248
    static unsigned long state_100a9248;
    // GLOBAL: SURRENDER 0x100A924C
    static void* module_100a924c;
};
