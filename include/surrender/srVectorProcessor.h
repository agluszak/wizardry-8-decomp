#pragma once

#include "srHeap.h"
#include "srMath.h"

class srARGB;
class srVP;

// Generic exports API 0x119, ID 0, and a factory returning an owned srVP.
// All three take no arguments; cdecl is the loader spelling, not proof that
// the SDK source could not have used the x86-equivalent stdcall spelling.
enum { SR_VP_MIN_API_VERSION = 0x119 };
typedef unsigned long (__cdecl *srGetVectorProcessorAPIFn)();
typedef long (__cdecl *srGetVectorProcessorIDFn)();
typedef srVP* (__cdecl *srInitVectorProcessorFn)();

/* These names are present in srDebugVP's own signature table. */
typedef unsigned char SRBYTE;
typedef unsigned long SRDWORD;
typedef srVector2T<float> srVector2;
typedef srVector3T<float> srVector3;
typedef srVector4T<float> srVector4;
typedef srMatrix4T<float> srMatrix4;

/* srDebugVP forwards each entry to the same slot of its wrapped srVP. This
   prefix therefore records real virtual methods, not padding inserted to
   reach _minMax at +0x18c. The debug signature for _prefetch omits its third
   word even though the checked debug, generic, AMD3DNow and KNI
   implementations return with RET 0x0c. That argument stays an
   address-qualified 32-bit scalar until its original type name and
   signedness are recovered. Purity and member cv qualifiers are likewise not
   asserted when the surviving evidence cannot distinguish them. */
class srVP {
public:
    virtual ~srVP();
    virtual const char* getName();
    virtual int _memcmp(
        const void* source_0, const void* source_1, SRDWORD bytes);
    virtual void _memcopy(
        void* destination, const void* source, SRDWORD bytes);
    virtual void _memcopy(
        void* destination, SRBYTE source, SRDWORD bytes);
    virtual void _prefetch(
        const void* destination, SRDWORD bytes, SRDWORD value_014);
    virtual void _copyInterleaved(
        void* destination,
        const void* source,
        SRDWORD destination_pitch,
        SRDWORD source_pitch,
        SRDWORD width,
        SRDWORD count);
    virtual void _swap(void* first, void* second, SRDWORD bytes);
    virtual void _copy(
        SRDWORD* destination, SRDWORD constant, SRDWORD count);
    virtual void _copy(
        srVector2* destination,
        const srVector2& constant,
        SRDWORD count);
    virtual void _copy(
        srVector3* destination,
        const srVector3& constant,
        SRDWORD count);
    virtual void _copy(
        srVector3* destination,
        const srVector4* source,
        SRDWORD count);
    virtual void _copy(
        srVector4* destination,
        const srVector4& constant,
        SRDWORD count);
    virtual void _copy(
        srVector4* destination,
        const srVector3* source,
        float constant,
        SRDWORD count);
    virtual void _copy(
        srVector4* destination,
        const srVector3* source_0,
        const float* source_1,
        SRDWORD count);
    virtual void _reverse(
        SRDWORD* destination, const SRDWORD* source, SRDWORD count);
    virtual void _and(
        SRDWORD* destination,
        const SRDWORD* source,
        SRDWORD constant,
        SRDWORD count);
    virtual void _and(
        SRDWORD* destination,
        const SRDWORD* source_0,
        const SRDWORD* source_1,
        SRDWORD count);
    virtual void _or(
        SRDWORD* destination,
        const SRDWORD* source,
        SRDWORD constant,
        SRDWORD count);
    virtual void _or(
        SRDWORD* destination,
        const SRDWORD* source_0,
        const SRDWORD* source_1,
        SRDWORD count);
    virtual void _xor(
        SRDWORD* destination,
        const SRDWORD* source,
        SRDWORD constant,
        SRDWORD count);
    virtual void _xor(
        SRDWORD* destination,
        const SRDWORD* source_0,
        const SRDWORD* source_1,
        SRDWORD count);
    virtual void _asr(
        SRDWORD* destination,
        const SRDWORD* source,
        SRDWORD shift,
        SRDWORD count);
    virtual void _asrAnd(
        SRDWORD* destination,
        const SRDWORD* source,
        SRDWORD shift,
        SRDWORD mask,
        SRDWORD count);
    virtual void _lsr(
        SRDWORD* destination,
        const SRDWORD* source,
        SRDWORD shift,
        SRDWORD count);
    virtual void _lsl(
        SRDWORD* destination,
        const SRDWORD* source,
        SRDWORD shift,
        SRDWORD count);
    virtual void _lslAnd(
        SRDWORD* destination,
        const SRDWORD* source,
        SRDWORD shift,
        SRDWORD mask,
        SRDWORD count);
    virtual int _isEqual(
        const SRDWORD* source, SRDWORD constant, SRDWORD count);
    virtual int _isEqual(
        const SRDWORD* source_0,
        const SRDWORD* source_1,
        SRDWORD count);
    /* srDebugVP names these four operations _max, _min, _min, _max.
       VC6 groups same-named virtual overloads, so the retail interleaving
       proves that their original lexical identifiers were distinct. */
    virtual float _max_074(const float* source, SRDWORD count);
    virtual SRDWORD _min_078(const SRDWORD* source, SRDWORD count);
    virtual float _min_07c(const float* source, SRDWORD count);
    virtual SRDWORD _max_080(const SRDWORD* source, SRDWORD count);
    virtual void _copyIndexed(
        SRDWORD* destination,
        const SRDWORD* source,
        const SRDWORD* indices,
        SRDWORD count);
    virtual void _copyIndexed(
        srVector2* destination,
        const srVector2* source,
        const SRDWORD* indices,
        SRDWORD count);
    virtual void _copyIndexed(
        srVector3* destination,
        const srVector2* source,
        const SRDWORD* indices,
        SRDWORD count);
    virtual void _copyIndexed(
        srVector3* destination,
        const srVector3* source,
        const SRDWORD* indices,
        SRDWORD count);
    virtual void _copyIndexed(
        srVector3* destination,
        const srVector4* source,
        const SRDWORD* indices,
        SRDWORD count);
    virtual void _copyIndexed(
        srVector4* destination,
        const srVector2* source,
        const SRDWORD* indices,
        SRDWORD count);
    virtual void _copyIndexed(
        srVector4* destination,
        const srVector3* source,
        const SRDWORD* indices,
        SRDWORD count);
    virtual void _copyIndexed(
        srVector4* destination,
        const srVector4* source,
        const SRDWORD* indices,
        SRDWORD count);
    virtual void _copyIndexed(
        srVector4* destination,
        const srARGB* source,
        const SRDWORD* indices,
        SRDWORD count);
    virtual void _addS(
        SRBYTE* destination,
        const SRBYTE* source,
        SRBYTE constant,
        SRDWORD count);
    virtual void _addS(
        SRBYTE* destination,
        const SRBYTE* source_0,
        const SRBYTE* source_1,
        SRDWORD count);
    virtual void _subS(
        SRBYTE* destination,
        const SRBYTE* source,
        SRBYTE constant,
        SRDWORD count);
    virtual void _subS(
        SRBYTE* destination,
        SRBYTE constant,
        const SRBYTE* source,
        SRDWORD count);
    virtual void _subS(
        SRBYTE* destination,
        const SRBYTE* source_0,
        const SRBYTE* source_1,
        SRDWORD count);
    virtual void _toFloat(
        float* destination, const SRBYTE* source, SRDWORD count);
    virtual void _add(
        float* destination,
        float constant,
        const float* source,
        SRDWORD count);
    virtual void _add(
        float* destination,
        const float* source_0,
        const float* source_1,
        SRDWORD count);
    virtual void _add(
        srVector3* destination,
        const srVector3& constant,
        const srVector3* vector_source,
        SRDWORD count);
    virtual void _add(
        srVector3* destination,
        const srVector3& constant,
        const float* float_source,
        SRDWORD count);
    virtual void _add(
        srVector3* destination,
        const srVector3* vector_source,
        const float* float_source,
        SRDWORD count);
    virtual void _add(
        srVector4* destination,
        const srVector4& constant,
        const srVector4* vector_source,
        SRDWORD count);
    virtual void _add(
        srVector4* destination,
        const srVector4& constant,
        const float* float_source,
        SRDWORD count);
    virtual void _add(
        srVector4* destination,
        const srVector4* vector_source,
        const float* float_source,
        SRDWORD count);
    virtual void _sub(
        float* destination,
        float constant,
        const float* source,
        SRDWORD count);
    virtual void _sub(
        float* destination,
        const float* source_0,
        const float* source_1,
        SRDWORD count);
    virtual void _sub(
        srVector3* destination,
        const srVector3& constant,
        const srVector3* vector_source,
        SRDWORD count);
    virtual void _sub(
        srVector3* destination,
        const srVector3& constant,
        const float* float_source,
        SRDWORD count);
    virtual void _sub(
        srVector3* destination,
        const srVector3* vector_source,
        const float* float_source,
        SRDWORD count);
    virtual void _sub(
        srVector3* destination,
        const float* float_source,
        const srVector3* vector_source,
        SRDWORD count);
    virtual void _sub(
        srVector4* destination,
        const srVector4& constant,
        const srVector4* vector_source,
        SRDWORD count);
    virtual void _sub(
        srVector4* destination,
        const srVector4& constant,
        const float* float_source,
        SRDWORD count);
    virtual void _sub(
        srVector4* destination,
        const srVector4* vector_source,
        const float* float_source,
        SRDWORD count);
    virtual void _sub(
        srVector4* destination,
        const float* float_source,
        const srVector4* vector_source,
        SRDWORD count);
    virtual void _mul(
        float* destination,
        float constant,
        const float* source,
        SRDWORD count);
    virtual void _mul(
        float* destination,
        const float* source_0,
        const float* source_1,
        SRDWORD count);
    virtual void _mul(
        float* destination,
        float constant,
        const float* source_0,
        const float* source_1,
        SRDWORD count);
    virtual void _mul(
        srVector3* destination,
        const srVector3& constant,
        const srVector3* vector_source,
        SRDWORD count);
    virtual void _mul(
        srVector3* destination,
        const srVector3& constant,
        const float* float_source,
        SRDWORD count);
    virtual void _mul(
        srVector3* destination,
        const srVector3* vector_source,
        const float* float_source,
        SRDWORD count);
    virtual void _mul(
        srVector4* destination,
        const srVector4& constant,
        const srVector4* vector_source,
        SRDWORD count);
    virtual void _mul(
        srVector4* destination,
        const srVector4& constant,
        const float* float_source,
        SRDWORD count);
    virtual void _mul(
        srVector4* destination,
        const srVector4* vector_source,
        const float* float_source,
        SRDWORD count);
    virtual void _mul(
        srMatrix4& destination,
        const srMatrix4& source_0,
        const srMatrix4& source_1);
    virtual void _mul(
        srMatrix4* destination,
        const srMatrix4* source_0,
        const srMatrix4* source_1,
        SRDWORD count);
    virtual void _div(
        float* destination,
        float constant,
        const float* source,
        SRDWORD count);
    virtual void _div(
        float* destination,
        const float* source_0,
        const float* source_1,
        SRDWORD count);
    virtual void _div(
        srVector2* destination,
        const srVector2* vector_source,
        const float* float_source,
        SRDWORD count);
    virtual void _div(
        srVector3* destination,
        const srVector3& constant,
        const srVector3* vector_source,
        SRDWORD count);
    virtual void _div(
        srVector3* destination,
        const srVector3& constant,
        const float* float_source,
        SRDWORD count);
    virtual void _div(
        srVector3* destination,
        const srVector3* vector_source,
        const float* float_source,
        SRDWORD count);
    virtual void _div(
        srVector3* destination,
        const float* float_source,
        const srVector3* vector_source,
        SRDWORD count);
    virtual void _div(
        srVector4* destination,
        const srVector4& constant,
        const srVector4* vector_source,
        SRDWORD count);
    virtual void _div(
        srVector4* destination,
        const srVector4& constant,
        const float* float_source,
        SRDWORD count);
    virtual void _div(
        srVector4* destination,
        const srVector4* vector_source,
        const float* float_source,
        SRDWORD count);
    virtual void _div(
        srVector4* destination,
        const float* float_source,
        const srVector4* vector_source,
        SRDWORD count);
    virtual void _clamp(
        float* destination,
        const float* source,
        float minimum,
        float maximum,
        SRDWORD count);
    virtual void _clampMin(
        float* destination,
        const float* source,
        float minimum,
        SRDWORD count);
    virtual void _clampMax(
        float* destination,
        const float* source,
        float maximum,
        SRDWORD count);
    virtual void _clampUnit(
        float* destination, const float* source, SRDWORD count);
    virtual void _sqrt(
        float* destination, const float* source, SRDWORD count);
    virtual void _isqrt(
        float* destination, const float* source, SRDWORD count);
    virtual void _lerp(
        float* destination,
        const float* target,
        const float* source,
        float constant,
        SRDWORD count);
    virtual int _isNeg(const float* source, SRDWORD count);
    virtual int _isPos(const float* source, SRDWORD count);
    virtual int _isZero(const float* source, SRDWORD count);
    virtual void _minMax(
        const srVector3* source,
        srVector3& minimum,
        srVector3& maximum,
        SRDWORD count);
    virtual void _minMax(
        const srVector4* source,
        srVector4& minimum,
        srVector4& maximum,
        SRDWORD count);
};

/* The private imported implementation pointer is exposed to clients by this
   ordinary header-visible facade; the direct load and virtual call are visible
   in the recovered retail clients, and no exported wrapper exists. The public
   spelling is inferred from the named `_minMax` implementation boundary. */
class srVectorProcessor {
public:
    static SR_DLL_IMPORT const char* getName();
    static SR_DLL_IMPORT int load(const char* filename);
    static SR_DLL_IMPORT long getID(const char* filename);
    static SR_DLL_IMPORT void release();

    static inline void minMax(
        const srVector3* source,
        srVector3& minimum,
        srVector3& maximum,
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
