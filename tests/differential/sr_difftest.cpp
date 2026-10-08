/* Differential runner for SurRender's exported vector-processor API.

   The same executable is run once against retail SR.DLL and once against a
   rebuilt SR.DLL. It installs the built-in processor with
   srVectorProcessor::initBaseVP(), feeds every case the same deterministic
   inputs and prints raw 32-bit output words. The trace contains values and
   logical identities only; process addresses appear only on "info" lines,
   which the driver does not compare.

   Usage: sr_difftest [--seed N] [--generated K] [--window FIRST COUNT]
                      [--list] [case-name ...]
*/

#include <excpt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "surrender/srVectorProcessor.h"

/* ------------------------------------------------------------------------ */
/* Deterministic input generation.                                          */

static unsigned long g_rng;

static unsigned long nextRandom()
{
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 17;
    g_rng ^= g_rng << 5;
    return g_rng;
}

static unsigned long hashName(const char* text)
{
    unsigned long hash = 2166136261UL;
    while (*text != 0) {
        hash ^= static_cast<unsigned char>(*text++);
        hash *= 16777619UL;
    }
    return hash;
}

static unsigned long floatBits(float value)
{
    unsigned long bits;
    memcpy(&bits, &value, 4);
    return bits;
}

static float bitsFloat(unsigned long bits)
{
    float value;
    memcpy(&value, &bits, 4);
    return value;
}

enum Domain {
    DOM_NONE,
    DOM_FLOAT,     /* finite, mixed dyadic and full-mantissa values */
    DOM_POSITIVE,  /* finite, > 0 */
    DOM_UNITISH,   /* [-0.25, 1.25] for clamps/colors */
    DOM_DWORD,     /* any 32-bit pattern */
    DOM_SHIFT,     /* 0..31 */
    DOM_BYTE,      /* bytes 0..255 */
    DOM_SMALLBYTE, /* bytes 0..3, with zeros */
    DOM_INDEX,     /* filled by the case: index into an indexed pool */
    DOM_PERM       /* filled by the case: permutation 0..n-1 */
};

static float randomFloat(int domain)
{
    unsigned long choice = nextRandom() & 3;
    float value;
    if (domain == DOM_UNITISH) {
        return static_cast<float>(nextRandom() % 1537) / 1024.0f - 0.25f;
    }
    if (choice == 0) {
        value = static_cast<float>(static_cast<long>(nextRandom() % 257) - 128) / 16.0f;
    } else if (choice == 3) {
        unsigned long exponent = 127 - 20 + nextRandom() % 41;
        value = bitsFloat((nextRandom() & 0x807fffffUL) | (exponent << 23));
    } else {
        value =
            static_cast<float>(static_cast<double>(nextRandom()) / 4294967296.0 * 200.0 - 100.0);
    }
    if (domain == DOM_POSITIVE) {
        value = static_cast<float>(fabs(value)) + 0.015625f;
    }
    return value;
}

static void fillWords(unsigned char* storage, int bytes, int domain)
{
    int offset;
    for (offset = 0; offset + 4 <= bytes; offset += 4) {
        unsigned long word;
        if (domain == DOM_DWORD) {
            word = nextRandom();
        } else if (domain == DOM_SHIFT) {
            word = nextRandom() & 31;
        } else if (domain == DOM_BYTE) {
            word = nextRandom();
        } else if (domain == DOM_SMALLBYTE) {
            word = nextRandom() & 0x03030303UL;
        } else {
            word = floatBits(randomFloat(domain));
        }
        memcpy(storage + offset, &word, 4);
    }
    for (; offset < bytes; ++offset) {
        storage[offset] = static_cast<unsigned char>(domain == DOM_SMALLBYTE ? nextRandom() & 3
                                                                             : nextRandom() & 0xff);
    }
}

/* ------------------------------------------------------------------------ */
/* Operation table.                                                         */

struct Context {
    void* destination;
    void* destination_1;
    const void* source[3];
    const SRDWORD* indices;
    float constants[8];
    srMatrix4 matrix;
    SRDWORD count;
    int has_word;
    unsigned long word;
    int has_double;
    double value;
};

typedef void (*RunFunction)(srVP* vp, Context& context);
typedef int (*SlotFunction)();

struct Array {
    int bytes; /* element size; 0 = unused */
    int domain;
};

enum Alias {
    ALIAS_NONE = 0,
    ALIAS_DEST_SOURCE0 = 1 /* destination == source[0] */
};

struct Operation {
    const char* name;
    Array destination;
    Array destination_1;
    Array source[3];
    int constant_count;
    int constant_domain;
    int uses_matrix;
    int indexed_source; /* -1, or the source[] that `indices` selects from */
    int alias;          /* aliasing patterns retail callers use (see README) */
    SRDWORD minimum_count;
    RunFunction run;
    SlotFunction slot;
};

/* VC6 represents a pointer to a virtual member of a single-inheritance class
   as the address of a vcall thunk: mov eax,[ecx]; jmp [eax+disp]. Decoding it
   gives the vtable offset this runner was compiled against. */
static int thunkOffset(const void* pointer_storage)
{
    const unsigned char* code;
    memcpy(&code, pointer_storage, sizeof(code));
    if (code[0] == 0xe9) {
        long displacement;
        memcpy(&displacement, code + 1, 4);
        code = code + 5 + displacement;
    }
    if (code[0] == 0x8b && code[1] == 0x01 && code[2] == 0xff && code[3] == 0x60) {
        return code[4];
    }
    if (code[0] == 0x8b && code[1] == 0x01 && code[2] == 0xff && code[3] == 0xa0) {
        long displacement;
        memcpy(&displacement, code + 4, 4);
        return displacement;
    }
    return -1;
}

#define F(n) static_cast<float*>(c.n)
#define CF(i) static_cast<const float*>(c.source[i])
#define V2(n) static_cast<srVector2*>(c.n)
#define V3(n) static_cast<srVector3*>(c.n)
#define V4(n) static_cast<srVector4*>(c.n)
#define CV2(i) static_cast<const srVector2*>(c.source[i])
#define CV3(i) static_cast<const srVector3*>(c.source[i])
#define CV4(i) static_cast<const srVector4*>(c.source[i])
#define DW(n) static_cast<SRDWORD*>(c.n)
#define CDW(i) static_cast<const SRDWORD*>(c.source[i])
#define B(n) static_cast<SRBYTE*>(c.n)
#define CB(i) static_cast<const SRBYTE*>(c.source[i])
#define K3 (*reinterpret_cast<const srVector3*>(c.constants))
#define K4 (*reinterpret_cast<const srVector4*>(c.constants))
#define K4B (*reinterpret_cast<const srVector4*>(c.constants + 4))

#define DEFINE_OP(id, method, signature, body)                                                     \
    typedef signature;                                                                             \
    static void run_##id(srVP* vp, Context& c)                                                     \
    {                                                                                              \
        body;                                                                                      \
    }                                                                                              \
    static int slot_##id()                                                                         \
    {                                                                                              \
        pointer_##id pointer = &srVP::method;                                                      \
        return thunkOffset(&pointer);                                                              \
    }

/* Transform family: called through the public srVectorProcessor wrappers. */
DEFINE_OP(transform3, _transform,
          void (srVP::*pointer_transform3)(srVector3*, const srVector3*, const srMatrix4&, SRDWORD),
          (void)vp;
          srVectorProcessor::transform(V3(destination), CV3(0), c.matrix, c.count))
DEFINE_OP(transform4, _transform,
          void (srVP::*pointer_transform4)(srVector4*, const srVector4*, const srMatrix4&, SRDWORD),
          (void)vp;
          srVectorProcessor::transform(V4(destination), CV4(0), c.matrix, c.count))
DEFINE_OP(transform3to4, _transform,
          void (srVP::*pointer_transform3to4)(srVector4*, const srVector3*, const srMatrix4&,
                                              SRDWORD),
          vp->_transform(V4(destination), CV3(0), c.matrix, c.count))
DEFINE_OP(transformOrtho, _transformOrtho,
          void (srVP::*pointer_transformOrtho)(srVector4*, const srVector4*, const srMatrix4&,
                                               SRDWORD),
          (void)vp;
          srVectorProcessor::transformOrtho(V4(destination), CV4(0), c.matrix, c.count))
DEFINE_OP(transformPerspective, _transformPerspective,
          void (srVP::*pointer_transformPerspective)(srVector4*, const srVector4*, const srMatrix4&,
                                                     SRDWORD),
          (void)vp;
          srVectorProcessor::transformPerspective(V4(destination), CV4(0), c.matrix, c.count))
DEFINE_OP(transformIndexed3, _transformIndexed,
          void (srVP::*pointer_transformIndexed3)(srVector3*, const srVector3*, const SRDWORD*,
                                                  const srMatrix4&, SRDWORD),
          vp->_transformIndexed(V3(destination), CV3(0), c.indices, c.matrix, c.count))
DEFINE_OP(transformIndexed3to4, _transformIndexed,
          void (srVP::*pointer_transformIndexed3to4)(srVector4*, const srVector3*, const SRDWORD*,
                                                     const srMatrix4&, SRDWORD),
          vp->_transformIndexed(V4(destination), CV3(0), c.indices, c.matrix, c.count))
DEFINE_OP(mulMatrix, _mul,
          void (srVP::*pointer_mulMatrix)(srMatrix4&, const srMatrix4&, const srMatrix4&),
          vp->_mul(*reinterpret_cast<srMatrix4*>(c.destination),
                   *reinterpret_cast<const srMatrix4*>(c.source[0]),
                   *reinterpret_cast<const srMatrix4*>(c.source[1])))
DEFINE_OP(mulMatrixArray, _mul,
          void (srVP::*pointer_mulMatrixArray)(srMatrix4*, const srMatrix4*, const srMatrix4*,
                                               SRDWORD),
          vp->_mul(reinterpret_cast<srMatrix4*>(c.destination),
                   reinterpret_cast<const srMatrix4*>(c.source[0]),
                   reinterpret_cast<const srMatrix4*>(c.source[1]), c.count))
DEFINE_OP(testBoundingBox, _srTestBoundingBox,
          int (srVP::*pointer_testBoundingBox)(const srMatrix4&, const srVector3&,
                                               const srVector3&),
          c.has_word = 1;
          c.word = vp->_srTestBoundingBox(c.matrix, K3,
                                          *reinterpret_cast<const srVector3*>(c.constants + 3)))

/* Float arithmetic. */
DEFINE_OP(addFK, _add, void (srVP::*pointer_addFK)(float*, float, const float*, SRDWORD),
          vp->_add(F(destination), c.constants[0], CF(0), c.count))
DEFINE_OP(addFF, _add, void (srVP::*pointer_addFF)(float*, const float*, const float*, SRDWORD),
          vp->_add(F(destination), CF(0), CF(1), c.count))
DEFINE_OP(subFK, _sub, void (srVP::*pointer_subFK)(float*, float, const float*, SRDWORD),
          vp->_sub(F(destination), c.constants[0], CF(0), c.count))
DEFINE_OP(subFF, _sub, void (srVP::*pointer_subFF)(float*, const float*, const float*, SRDWORD),
          vp->_sub(F(destination), CF(0), CF(1), c.count))
DEFINE_OP(mulFK, _mul, void (srVP::*pointer_mulFK)(float*, float, const float*, SRDWORD),
          vp->_mul(F(destination), c.constants[0], CF(0), c.count))
DEFINE_OP(mulFF, _mul, void (srVP::*pointer_mulFF)(float*, const float*, const float*, SRDWORD),
          vp->_mul(F(destination), CF(0), CF(1), c.count))
DEFINE_OP(mulFKFF, _mul,
          void (srVP::*pointer_mulFKFF)(float*, float, const float*, const float*, SRDWORD),
          vp->_mul(F(destination), c.constants[0], CF(0), CF(1), c.count))
DEFINE_OP(divKF, _div, void (srVP::*pointer_divKF)(float*, float, const float*, SRDWORD),
          vp->_div(F(destination), c.constants[0], CF(0), c.count))
DEFINE_OP(divFF, _div, void (srVP::*pointer_divFF)(float*, const float*, const float*, SRDWORD),
          vp->_div(F(destination), CF(0), CF(1), c.count))
DEFINE_OP(clamp, _clamp, void (srVP::*pointer_clamp)(float*, const float*, float, float, SRDWORD),
          vp->_clamp(F(destination), CF(0), c.constants[0], c.constants[1], c.count))
DEFINE_OP(clampMin, _clampMin, void (srVP::*pointer_clampMin)(float*, const float*, float, SRDWORD),
          vp->_clampMin(F(destination), CF(0), c.constants[0], c.count))
DEFINE_OP(clampMax, _clampMax, void (srVP::*pointer_clampMax)(float*, const float*, float, SRDWORD),
          vp->_clampMax(F(destination), CF(0), c.constants[0], c.count))
DEFINE_OP(clampUnit, _clampUnit, void (srVP::*pointer_clampUnit)(float*, const float*, SRDWORD),
          vp->_clampUnit(F(destination), CF(0), c.count))
DEFINE_OP(sqrt, _sqrt, void (srVP::*pointer_sqrt)(float*, const float*, SRDWORD),
          vp->_sqrt(F(destination), CF(0), c.count))
DEFINE_OP(isqrt, _isqrt, void (srVP::*pointer_isqrt)(float*, const float*, SRDWORD),
          vp->_isqrt(F(destination), CF(0), c.count))
DEFINE_OP(lerp, _lerp,
          void (srVP::*pointer_lerp)(float*, const float*, const float*, float, SRDWORD),
          vp->_lerp(F(destination), CF(0), CF(1), c.constants[0], c.count))
DEFINE_OP(abs, _abs, void (srVP::*pointer_abs)(float*, const float*, SRDWORD),
          vp->_abs(F(destination), CF(0), c.count))
DEFINE_OP(neg, _neg, void (srVP::*pointer_neg)(float*, const float*, SRDWORD),
          vp->_neg(F(destination), CF(0), c.count))
DEFINE_OP(cubic, _cubic, void (srVP::*pointer_cubic)(float*, const float*, SRDWORD),
          vp->_cubic(F(destination), CF(0), c.count))
DEFINE_OP(invPoly, _invPoly,
          void (srVP::*pointer_invPoly)(float*, const float*, const srVector3&, SRDWORD),
          vp->_invPoly(F(destination), CF(0), K3, c.count))
DEFINE_OP(specularPow, _srSpecularPow,
          void (srVP::*pointer_specularPow)(float*, const float*, float, SRDWORD),
          vp->_srSpecularPow(F(destination), CF(0), c.constants[0], c.count))
DEFINE_OP(toInt, _toInt, void (srVP::*pointer_toInt)(SRLONG*, const float*, SRDWORD),
          vp->_toInt(static_cast<SRLONG*>(c.destination), CF(0), c.count))
DEFINE_OP(sum, _sum, double (srVP::*pointer_sum)(const float*, SRDWORD), c.has_double = 1;
          c.value = vp->_sum(CF(0), c.count))
DEFINE_OP(minMaxF, _minMax, void (srVP::*pointer_minMaxF)(const float*, float&, float&, SRDWORD),
          vp->_minMax(CF(0), *F(destination), *F(destination_1), c.count))
DEFINE_OP(minMax3, _minMax,
          void (srVP::*pointer_minMax3)(const srVector3*, srVector3&, srVector3&, SRDWORD),
          vp->_minMax(CV3(0), *V3(destination), *V3(destination_1), c.count))
DEFINE_OP(minMax4, _minMax,
          void (srVP::*pointer_minMax4)(const srVector4*, srVector4&, srVector4&, SRDWORD),
          vp->_minMax(CV4(0), *V4(destination), *V4(destination_1), c.count))
DEFINE_OP(isZero, _isZero, int (srVP::*pointer_isZero)(const float*, SRDWORD), c.has_word = 1;
          c.word = vp->_isZero(CF(0), c.count))
DEFINE_OP(isNeg, _isNeg, int (srVP::*pointer_isNeg)(const float*, SRDWORD), c.has_word = 1;
          c.word = vp->_isNeg(CF(0), c.count))
DEFINE_OP(isPos, _isPos, int (srVP::*pointer_isPos)(const float*, SRDWORD), c.has_word = 1;
          c.word = vp->_isPos(CF(0), c.count))
DEFINE_OP(axpyFKF, _axpy,
          void (srVP::*pointer_axpyFKF)(float*, const float*, float, const float*, SRDWORD),
          vp->_axpy(F(destination), CF(0), c.constants[0], CF(1), c.count))
DEFINE_OP(axpyFFF, _axpy,
          void (srVP::*pointer_axpyFFF)(float*, const float*, const float*, const float*, SRDWORD),
          vp->_axpy(F(destination), CF(0), CF(1), CF(2), c.count))

/* Vector arithmetic. */
DEFINE_OP(add3K, _add,
          void (srVP::*pointer_add3K)(srVector3*, const srVector3&, const srVector3*, SRDWORD),
          vp->_add(V3(destination), K3, CV3(0), c.count))
DEFINE_OP(add4V, _add,
          void (srVP::*pointer_add4V)(srVector4*, const srVector4*, const float*, SRDWORD),
          vp->_add(V4(destination), CV4(0), CF(1), c.count))
DEFINE_OP(sub3K, _sub,
          void (srVP::*pointer_sub3K)(srVector3*, const srVector3&, const srVector3*, SRDWORD),
          vp->_sub(V3(destination), K3, CV3(0), c.count))
DEFINE_OP(mul3K, _mul,
          void (srVP::*pointer_mul3K)(srVector3*, const srVector3&, const srVector3*, SRDWORD),
          vp->_mul(V3(destination), K3, CV3(0), c.count))
DEFINE_OP(mul3F, _mul,
          void (srVP::*pointer_mul3F)(srVector3*, const srVector3*, const float*, SRDWORD),
          vp->_mul(V3(destination), CV3(0), CF(1), c.count))
DEFINE_OP(mul4KF, _mul,
          void (srVP::*pointer_mul4KF)(srVector4*, const srVector4&, const float*, SRDWORD),
          vp->_mul(V4(destination), K4, CF(0), c.count))
DEFINE_OP(div4KV, _div,
          void (srVP::*pointer_div4KV)(srVector4*, const srVector4&, const srVector4*, SRDWORD),
          vp->_div(V4(destination), K4, CV4(0), c.count))
DEFINE_OP(dot3K, _dot,
          void (srVP::*pointer_dot3K)(float*, const srVector3&, const srVector3*, SRDWORD),
          vp->_dot(F(destination), K3, CV3(0), c.count))
DEFINE_OP(dot3V, _dot,
          void (srVP::*pointer_dot3V)(float*, const srVector3*, const srVector3*, SRDWORD),
          vp->_dot(F(destination), CV3(0), CV3(1), c.count))
DEFINE_OP(dot4K, _dot,
          void (srVP::*pointer_dot4K)(float*, const srVector4&, const srVector4*, SRDWORD),
          vp->_dot(F(destination), K4, CV4(0), c.count))
DEFINE_OP(dot4K3, _dot,
          void (srVP::*pointer_dot4K3)(float*, const srVector4&, const srVector3*, SRDWORD),
          vp->_dot(F(destination), K4, CV3(0), c.count))
DEFINE_OP(cross, _cross,
          void (srVP::*pointer_cross)(srVector3*, const srVector3*, const srVector3*, SRDWORD),
          vp->_cross(V3(destination), CV3(0), CV3(1), c.count))
DEFINE_OP(length3, _length, void (srVP::*pointer_length3)(float*, const srVector3*, SRDWORD),
          vp->_length(F(destination), CV3(0), c.count))
DEFINE_OP(length4, _length, void (srVP::*pointer_length4)(float*, const srVector4*, SRDWORD),
          vp->_length(F(destination), CV4(0), c.count))
DEFINE_OP(normalize3, _normalize,
          void (srVP::*pointer_normalize3)(srVector3*, const srVector3*, float, SRDWORD),
          vp->_normalize(V3(destination), CV3(0), c.constants[0], c.count))
DEFINE_OP(normalize4, _normalize,
          void (srVP::*pointer_normalize4)(srVector4*, const srVector4*, float, SRDWORD),
          vp->_normalize(V4(destination), CV4(0), c.constants[0], c.count))
DEFINE_OP(dir3, _dir, void (srVP::*pointer_dir3)(srVector3*, float*, const srVector3*, SRDWORD),
          vp->_dir(V3(destination), F(destination_1), CV3(0), c.count))
DEFINE_OP(dir4, _dir, void (srVP::*pointer_dir4)(srVector3*, float*, const srVector4*, SRDWORD),
          vp->_dir(V3(destination), F(destination_1), CV4(0), c.count))
DEFINE_OP(divByW, _divByW, void (srVP::*pointer_divByW)(srVector4*, const srVector4*, SRDWORD),
          vp->_divByW(V4(destination), CV4(0), c.count))
DEFINE_OP(copyWK, _copyW, void (srVP::*pointer_copyWK)(srVector4*, float, SRDWORD),
          vp->_copyW(V4(destination), c.constants[0], c.count))
DEFINE_OP(copyWF, _copyW, void (srVP::*pointer_copyWF)(srVector4*, const float*, SRDWORD),
          vp->_copyW(V4(destination), CF(0), c.count))
DEFINE_OP(copyWOut, _copyW, void (srVP::*pointer_copyWOut)(float*, const srVector4*, SRDWORD),
          vp->_copyW(F(destination), CV4(0), c.count))
DEFINE_OP(mulAddKKV, _mulAdd,
          void (srVP::*pointer_mulAddKKV)(srVector4*, const srVector4&, const srVector4&,
                                          const srVector4*, SRDWORD),
          vp->_mulAdd(V4(destination), K4, K4B, CV4(0), c.count))
DEFINE_OP(axpy4VKF, _axpy,
          void (srVP::*pointer_axpy4VKF)(srVector4*, const srVector4*, const srVector4&,
                                         const float*, SRDWORD),
          vp->_axpy(V4(destination), CV4(0), K4, CF(1), c.count))
DEFINE_OP(axpy4VKFF, _axpy,
          void (srVP::*pointer_axpy4VKFF)(srVector4*, const srVector4*, const srVector4&,
                                          const float*, const float*, SRDWORD),
          vp->_axpy(V4(destination), CV4(0), K4, CF(1), CF(2), c.count))
DEFINE_OP(clipFlags, _srGetClipFlags,
          void (srVP::*pointer_clipFlags)(SRBYTE*, const srVector4*, SRDWORD),
          vp->_srGetClipFlags(B(destination), CV4(0), c.count))
DEFINE_OP(convertColor, _srDirect3DConvertColor,
          void (srVP::*pointer_convertColor)(SRDWORD*, const srVector4*, SRDWORD),
          vp->_srDirect3DConvertColor(DW(destination), CV4(0), c.count))
DEFINE_OP(floatToLinear, _srFloatToLinear,
          void (srVP::*pointer_floatToLinear)(SRDWORD*, const float*, SRDWORD),
          vp->_srFloatToLinear(DW(destination), CF(0), c.count))
DEFINE_OP(linearToFloat, _srLinearToFloat,
          void (srVP::*pointer_linearToFloat)(float*, const SRDWORD*, SRDWORD),
          vp->_srLinearToFloat(F(destination), CDW(0), c.count))
DEFINE_OP(collectPos, _srCollectPos,
          SRDWORD (srVP::*pointer_collectPos)(SRDWORD*, const float*, SRDWORD), c.has_word = 1;
          c.word = vp->_srCollectPos(DW(destination), CF(0), c.count))
DEFINE_OP(collectNeg, _srCollectNeg,
          SRDWORD (srVP::*pointer_collectNeg)(SRDWORD*, const float*, SRDWORD), c.has_word = 1;
          c.word = vp->_srCollectNeg(DW(destination), CF(0), c.count))
DEFINE_OP(collectNonZero, _srCollectNonZero,
          SRDWORD (srVP::*pointer_collectNonZero)(SRDWORD*, const SRBYTE*, SRDWORD), c.has_word = 1;
          c.word = vp->_srCollectNonZero(DW(destination), CB(0), c.count))
DEFINE_OP(remapInverse, _srRemapInverse,
          void (srVP::*pointer_remapInverse)(SRDWORD*, const SRDWORD*, SRDWORD),
          vp->_srRemapInverse(DW(destination), CDW(0), c.count))

/* Indexed operations. */
DEFINE_OP(copyIndexed3, _copyIndexed,
          void (srVP::*pointer_copyIndexed3)(srVector3*, const srVector3*, const SRDWORD*, SRDWORD),
          vp->_copyIndexed(V3(destination), CV3(0), c.indices, c.count))
DEFINE_OP(copyIndexed4from3, _copyIndexed,
          void (srVP::*pointer_copyIndexed4from3)(srVector4*, const srVector3*, const SRDWORD*,
                                                  SRDWORD),
          vp->_copyIndexed(V4(destination), CV3(0), c.indices, c.count))
DEFINE_OP(copyIndexedARGB, _copyIndexed,
          void (srVP::*pointer_copyIndexedARGB)(srVector4*, const srARGB*, const SRDWORD*, SRDWORD),
          vp->_copyIndexed(V4(destination), static_cast<const srARGB*>(c.source[0]), c.indices,
                           c.count))
DEFINE_OP(mulIndexedF, _mulIndexed,
          void (srVP::*pointer_mulIndexedF)(float*, const float*, const float*, const SRDWORD*,
                                            SRDWORD),
          vp->_mulIndexed(F(destination), CF(0), CF(1), c.indices, c.count))
DEFINE_OP(mulIndexed4V, _mulIndexed,
          void (srVP::*pointer_mulIndexed4V)(srVector4*, const srVector4*, const srVector4*,
                                             const SRDWORD*, SRDWORD),
          vp->_mulIndexed(V4(destination), CV4(0), CV4(1), c.indices, c.count))
DEFINE_OP(mulIndexed4K, _mulIndexed,
          void (srVP::*pointer_mulIndexed4K)(srVector4*, const srVector4&, const srVector4*,
                                             const SRDWORD*, SRDWORD),
          vp->_mulIndexed(V4(destination), K4, CV4(0), c.indices, c.count))
DEFINE_OP(dotIndexed, _dotIndexed,
          void (srVP::*pointer_dotIndexed)(float*, const srVector4&, const srVector4*,
                                           const SRDWORD*, SRDWORD),
          vp->_dotIndexed(F(destination), K4, CV4(0), c.indices, c.count))
DEFINE_OP(cullNoClip, _srCullNoClip,
          SRDWORD (srVP::*pointer_cullNoClip)(SRDWORD*, const srVector4&, const srVector4*,
                                              SRDWORD),
          c.has_word = 1;
          c.word = vp->_srCullNoClip(DW(destination), K4, CV4(0), c.count))

/* Integer operations. */
DEFINE_OP(reverse, _reverse, void (srVP::*pointer_reverse)(SRDWORD*, const SRDWORD*, SRDWORD),
          vp->_reverse(DW(destination), CDW(0), c.count))
DEFINE_OP(andK, _and, void (srVP::*pointer_andK)(SRDWORD*, const SRDWORD*, SRDWORD, SRDWORD),
          vp->_and(DW(destination), CDW(0), floatBits(c.constants[0]), c.count))
DEFINE_OP(orK, _or, void (srVP::*pointer_orK)(SRDWORD*, const SRDWORD*, SRDWORD, SRDWORD),
          vp->_or(DW(destination), CDW(0), floatBits(c.constants[0]), c.count))
DEFINE_OP(xorV, _xor, void (srVP::*pointer_xorV)(SRDWORD*, const SRDWORD*, const SRDWORD*, SRDWORD),
          vp->_xor(DW(destination), CDW(0), CDW(1), c.count))
DEFINE_OP(asr, _asr, void (srVP::*pointer_asr)(SRDWORD*, const SRDWORD*, SRDWORD, SRDWORD),
          vp->_asr(DW(destination), CDW(0), floatBits(c.constants[0]) & 31, c.count))
DEFINE_OP(lsr, _lsr, void (srVP::*pointer_lsr)(SRDWORD*, const SRDWORD*, SRDWORD, SRDWORD),
          vp->_lsr(DW(destination), CDW(0), floatBits(c.constants[0]) & 31, c.count))
DEFINE_OP(lslAnd, _lslAnd,
          void (srVP::*pointer_lslAnd)(SRDWORD*, const SRDWORD*, SRDWORD, SRDWORD, SRDWORD),
          vp->_lslAnd(DW(destination), CDW(0), floatBits(c.constants[0]) & 31,
                      floatBits(c.constants[1]), c.count))
DEFINE_OP(maxDW, _max, SRDWORD (srVP::*pointer_maxDW)(const SRDWORD*, SRDWORD), c.has_word = 1;
          c.word = vp->_max(CDW(0), c.count))
DEFINE_OP(minF, _min, float (srVP::*pointer_minF)(const float*, SRDWORD), c.has_word = 1;
          c.word = floatBits(vp->_min(CF(0), c.count)))
DEFINE_OP(isEqualK, _isEqual, int (srVP::*pointer_isEqualK)(const SRDWORD*, SRDWORD, SRDWORD),
          c.has_word = 1;
          c.word = vp->_isEqual(CDW(0), CDW(0)[0], c.count))
DEFINE_OP(addS, _addS, void (srVP::*pointer_addS)(SRBYTE*, const SRBYTE*, const SRBYTE*, SRDWORD),
          vp->_addS(B(destination), CB(0), CB(1), c.count))
DEFINE_OP(subSK, _subS, void (srVP::*pointer_subSK)(SRBYTE*, const SRBYTE*, SRBYTE, SRDWORD),
          vp->_subS(B(destination), CB(0), static_cast<SRBYTE>(floatBits(c.constants[0])), c.count))
DEFINE_OP(toFloat, _toFloat, void (srVP::*pointer_toFloat)(float*, const SRBYTE*, SRDWORD),
          vp->_toFloat(F(destination), CB(0), c.count))

#define NO {0, DOM_NONE}
#define FL(n) {4 * (n), DOM_FLOAT}
#define PO(n) {4 * (n), DOM_POSITIVE}
#define UN(n) {4 * (n), DOM_UNITISH}
#define DWA(n) {4 * (n), DOM_DWORD}
#define BY(n) {(n), DOM_BYTE}
#define SB(n) {(n), DOM_SMALLBYTE}
#define PERM {4, DOM_PERM}
#define ENTRY(id) run_##id, slot_##id

/* `alias` lists only destination/source overlaps used by retail-era callers;
   see tests/differential/README.md for the call sites and how they were
   checked. */
static const Operation g_operations[] = {
    {"transform3",
     FL(3),
     NO,
     {FL(3), NO, NO},
     0,
     DOM_NONE,
     1,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(transform3)},
    {"transform4",
     FL(4),
     NO,
     {FL(4), NO, NO},
     0,
     DOM_NONE,
     1,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(transform4)},
    {"transform3to4",
     FL(4),
     NO,
     {FL(3), NO, NO},
     0,
     DOM_NONE,
     1,
     -1,
     ALIAS_NONE,
     0,
     ENTRY(transform3to4)},
    {"transformOrtho",
     FL(4),
     NO,
     {FL(4), NO, NO},
     0,
     DOM_NONE,
     1,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(transformOrtho)},
    {"transformPerspective",
     FL(4),
     NO,
     {FL(4), NO, NO},
     0,
     DOM_NONE,
     1,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(transformPerspective)},
    {"transformIndexed3",
     FL(3),
     NO,
     {FL(3), NO, NO},
     0,
     DOM_NONE,
     1,
     0,
     ALIAS_NONE,
     0,
     ENTRY(transformIndexed3)},
    {"transformIndexed3to4",
     FL(4),
     NO,
     {FL(3), NO, NO},
     0,
     DOM_NONE,
     1,
     0,
     ALIAS_NONE,
     0,
     ENTRY(transformIndexed3to4)},
    {"mulMatrix",
     FL(16),
     NO,
     {FL(16), FL(16), NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_NONE,
     1,
     ENTRY(mulMatrix)},
    {"mulMatrixArray",
     FL(16),
     NO,
     {FL(16), FL(16), NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_NONE,
     0,
     ENTRY(mulMatrixArray)},
    {"testBoundingBox",
     NO,
     NO,
     {NO, NO, NO},
     6,
     DOM_FLOAT,
     1,
     -1,
     ALIAS_NONE,
     1,
     ENTRY(testBoundingBox)},
    {"addFK", FL(1), NO, {FL(1), NO, NO}, 1, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(addFK)},
    {"addFF",
     FL(1),
     NO,
     {FL(1), FL(1), NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(addFF)},
    {"subFK", FL(1), NO, {FL(1), NO, NO}, 1, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(subFK)},
    {"subFF",
     FL(1),
     NO,
     {FL(1), FL(1), NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(subFF)},
    {"mulFK", FL(1), NO, {FL(1), NO, NO}, 1, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(mulFK)},
    {"mulFF",
     FL(1),
     NO,
     {FL(1), FL(1), NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(mulFF)},
    {"mulFKFF", FL(1), NO, {FL(1), FL(1), NO}, 1, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(mulFKFF)},
    {"divKF", FL(1), NO, {PO(1), NO, NO}, 1, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(divKF)},
    {"divFF", FL(1), NO, {FL(1), PO(1), NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(divFF)},
    {"clamp", FL(1), NO, {UN(1), NO, NO}, 2, DOM_UNITISH, 0, -1, ALIAS_NONE, 0, ENTRY(clamp)},
    {"clampMin",
     FL(1),
     NO,
     {UN(1), NO, NO},
     1,
     DOM_UNITISH,
     0,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(clampMin)},
    {"clampMax", FL(1), NO, {UN(1), NO, NO}, 1, DOM_UNITISH, 0, -1, ALIAS_NONE, 0, ENTRY(clampMax)},
    {"clampUnit",
     FL(1),
     NO,
     {UN(1), NO, NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(clampUnit)},
    {"sqrt", FL(1), NO, {PO(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(sqrt)},
    {"isqrt", FL(1), NO, {PO(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(isqrt)},
    {"lerp", FL(1), NO, {FL(1), FL(1), NO}, 1, DOM_UNITISH, 0, -1, ALIAS_NONE, 0, ENTRY(lerp)},
    {"abs", FL(1), NO, {FL(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(abs)},
    {"neg", FL(1), NO, {FL(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(neg)},
    {"cubic", FL(1), NO, {UN(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(cubic)},
    {"invPoly", FL(1), NO, {PO(1), NO, NO}, 3, DOM_POSITIVE, 0, -1, ALIAS_NONE, 0, ENTRY(invPoly)},
    {"specularPow",
     FL(1),
     NO,
     {UN(1), NO, NO},
     1,
     DOM_POSITIVE,
     0,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(specularPow)},
    {"toInt", DWA(1), NO, {FL(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(toInt)},
    {"sum", NO, NO, {FL(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(sum)},
    {"minMaxF", FL(1), FL(1), {FL(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 1, ENTRY(minMaxF)},
    {"minMax3", FL(3), FL(3), {FL(3), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 1, ENTRY(minMax3)},
    {"minMax4", FL(4), FL(4), {FL(4), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 1, ENTRY(minMax4)},
    {"isZero", NO, NO, {FL(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(isZero)},
    {"isNeg", NO, NO, {FL(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(isNeg)},
    {"isPos", NO, NO, {FL(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(isPos)},
    {"axpyFKF", FL(1), NO, {FL(1), FL(1), NO}, 1, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(axpyFKF)},
    {"axpyFFF",
     FL(1),
     NO,
     {FL(1), FL(1), FL(1)},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_NONE,
     0,
     ENTRY(axpyFFF)},
    {"add3K", FL(3), NO, {FL(3), NO, NO}, 3, DOM_FLOAT, 0, -1, ALIAS_DEST_SOURCE0, 0, ENTRY(add3K)},
    {"add4V", FL(4), NO, {FL(4), FL(1), NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(add4V)},
    {"sub3K", FL(3), NO, {FL(3), NO, NO}, 3, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(sub3K)},
    {"mul3K", FL(3), NO, {FL(3), NO, NO}, 3, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(mul3K)},
    {"mul3F",
     FL(3),
     NO,
     {FL(3), FL(1), NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(mul3F)},
    {"mul4KF", FL(4), NO, {FL(1), NO, NO}, 4, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(mul4KF)},
    {"div4KV", FL(4), NO, {PO(4), NO, NO}, 4, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(div4KV)},
    {"dot3K", FL(1), NO, {FL(3), NO, NO}, 3, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(dot3K)},
    {"dot3V", FL(1), NO, {FL(3), FL(3), NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(dot3V)},
    {"dot4K", FL(1), NO, {FL(4), NO, NO}, 4, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(dot4K)},
    {"dot4K3", FL(1), NO, {FL(3), NO, NO}, 4, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(dot4K3)},
    {"cross", FL(3), NO, {FL(3), FL(3), NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(cross)},
    {"length3", FL(1), NO, {FL(3), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(length3)},
    {"length4", FL(1), NO, {FL(4), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(length4)},
    {"normalize3",
     FL(3),
     NO,
     {PO(3), NO, NO},
     1,
     DOM_POSITIVE,
     0,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(normalize3)},
    {"normalize4",
     FL(4),
     NO,
     {PO(4), NO, NO},
     1,
     DOM_POSITIVE,
     0,
     -1,
     ALIAS_NONE,
     0,
     ENTRY(normalize4)},
    {"dir3", FL(3), FL(1), {PO(3), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(dir3)},
    {"dir4", FL(3), FL(1), {PO(4), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(dir4)},
    {"divByW", FL(4), NO, {PO(4), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(divByW)},
    {"copyWK", FL(4), NO, {NO, NO, NO}, 1, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(copyWK)},
    {"copyWF", FL(4), NO, {FL(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(copyWF)},
    {"copyWOut", FL(1), NO, {FL(4), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(copyWOut)},
    {"mulAddKKV", FL(4), NO, {FL(4), NO, NO}, 8, DOM_FLOAT, 0, -1, ALIAS_NONE, 0, ENTRY(mulAddKKV)},
    {"axpy4VKF",
     FL(4),
     NO,
     {FL(4), FL(1), NO},
     4,
     DOM_FLOAT,
     0,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(axpy4VKF)},
    {"axpy4VKFF",
     FL(4),
     NO,
     {FL(4), FL(1), FL(1)},
     4,
     DOM_FLOAT,
     0,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(axpy4VKFF)},
    {"clipFlags", BY(1), NO, {FL(4), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(clipFlags)},
    {"convertColor",
     DWA(1),
     NO,
     {UN(4), NO, NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_NONE,
     0,
     ENTRY(convertColor)},
    {"floatToLinear",
     DWA(1),
     NO,
     {UN(1), NO, NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_NONE,
     0,
     ENTRY(floatToLinear)},
    {"linearToFloat",
     FL(1),
     NO,
     {DWA(1), NO, NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_NONE,
     0,
     ENTRY(linearToFloat)},
    {"collectPos",
     DWA(1),
     NO,
     {FL(1), NO, NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_NONE,
     0,
     ENTRY(collectPos)},
    {"collectNeg",
     DWA(1),
     NO,
     {FL(1), NO, NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_NONE,
     0,
     ENTRY(collectNeg)},
    {"collectNonZero",
     DWA(1),
     NO,
     {SB(1), NO, NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_NONE,
     0,
     ENTRY(collectNonZero)},
    {"remapInverse",
     DWA(1),
     NO,
     {PERM, NO, NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_NONE,
     0,
     ENTRY(remapInverse)},
    {"copyIndexed3",
     FL(3),
     NO,
     {FL(3), NO, NO},
     0,
     DOM_NONE,
     0,
     0,
     ALIAS_NONE,
     0,
     ENTRY(copyIndexed3)},
    {"copyIndexed4from3",
     FL(4),
     NO,
     {FL(3), NO, NO},
     0,
     DOM_NONE,
     0,
     0,
     ALIAS_NONE,
     0,
     ENTRY(copyIndexed4from3)},
    {"copyIndexedARGB",
     FL(4),
     NO,
     {{4, DOM_BYTE}, NO, NO},
     0,
     DOM_NONE,
     0,
     0,
     ALIAS_NONE,
     0,
     ENTRY(copyIndexedARGB)},
    {"mulIndexedF",
     FL(1),
     NO,
     {FL(1), FL(1), NO},
     0,
     DOM_NONE,
     0,
     1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(mulIndexedF)},
    {"mulIndexed4V",
     FL(4),
     NO,
     {FL(4), FL(4), NO},
     0,
     DOM_NONE,
     0,
     1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(mulIndexed4V)},
    {"mulIndexed4K",
     FL(4),
     NO,
     {FL(4), NO, NO},
     4,
     DOM_FLOAT,
     0,
     0,
     ALIAS_NONE,
     0,
     ENTRY(mulIndexed4K)},
    {"dotIndexed",
     FL(1),
     NO,
     {FL(4), NO, NO},
     4,
     DOM_FLOAT,
     0,
     0,
     ALIAS_NONE,
     0,
     ENTRY(dotIndexed)},
    {"cullNoClip",
     DWA(1),
     NO,
     {FL(4), NO, NO},
     4,
     DOM_FLOAT,
     0,
     -1,
     ALIAS_NONE,
     0,
     ENTRY(cullNoClip)},
    {"reverse",
     DWA(1),
     NO,
     {DWA(1), NO, NO},
     0,
     DOM_NONE,
     0,
     -1,
     ALIAS_DEST_SOURCE0,
     0,
     ENTRY(reverse)},
    {"andK", DWA(1), NO, {DWA(1), NO, NO}, 1, DOM_DWORD, 0, -1, ALIAS_NONE, 0, ENTRY(andK)},
    {"orK", DWA(1), NO, {DWA(1), NO, NO}, 1, DOM_DWORD, 0, -1, ALIAS_NONE, 0, ENTRY(orK)},
    {"xorV", DWA(1), NO, {DWA(1), DWA(1), NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(xorV)},
    {"asr", DWA(1), NO, {DWA(1), NO, NO}, 1, DOM_SHIFT, 0, -1, ALIAS_NONE, 0, ENTRY(asr)},
    {"lsr", DWA(1), NO, {DWA(1), NO, NO}, 1, DOM_SHIFT, 0, -1, ALIAS_NONE, 0, ENTRY(lsr)},
    {"lslAnd", DWA(1), NO, {DWA(1), NO, NO}, 2, DOM_DWORD, 0, -1, ALIAS_NONE, 0, ENTRY(lslAnd)},
    {"maxDW", NO, NO, {DWA(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 1, ENTRY(maxDW)},
    {"minF", NO, NO, {FL(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 1, ENTRY(minF)},
    {"isEqualK", NO, NO, {SB(4), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 1, ENTRY(isEqualK)},
    {"addS", BY(1), NO, {BY(1), BY(1), NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(addS)},
    {"subSK", BY(1), NO, {BY(1), NO, NO}, 1, DOM_DWORD, 0, -1, ALIAS_NONE, 0, ENTRY(subSK)},
    {"toFloat", FL(1), NO, {BY(1), NO, NO}, 0, DOM_NONE, 0, -1, ALIAS_NONE, 0, ENTRY(toFloat)},
};

static const int g_operation_count = sizeof(g_operations) / sizeof(g_operations[0]);

static const Operation* findOperation(const char* name)
{
    int index;
    for (index = 0; index < g_operation_count; ++index) {
        if (strcmp(g_operations[index].name, name) == 0) {
            return &g_operations[index];
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------ */
/* Case execution.                                                          */

enum { GUARD_BYTES = 32, GUARD_FILL = 0xa5, MAX_BYTES = 64 * 1024 };

struct Buffer {
    unsigned char* storage; /* guard + payload + guard */
    unsigned char* payload;
    int bytes;
};

static void allocateBuffer(Buffer& buffer, int bytes)
{
    buffer.bytes = bytes;
    buffer.storage = static_cast<unsigned char*>(malloc(bytes + 2 * GUARD_BYTES + 16));
    /* 16-byte aligned payload: SIMD-era processors may assume it. */
    buffer.payload = buffer.storage + GUARD_BYTES;
    while ((reinterpret_cast<unsigned long>(buffer.payload) & 15) != 0) {
        ++buffer.payload;
    }
    memset(buffer.payload - GUARD_BYTES, GUARD_FILL, bytes + 2 * GUARD_BYTES);
}

/* Unwritten destination words are visible as a recognizable pattern. */
static void fillDestinationPattern(Buffer& buffer)
{
    int offset;
    for (offset = 0; offset + 4 <= buffer.bytes; offset += 4) {
        unsigned long word = 0x7f3c0000UL + static_cast<unsigned long>(offset / 4);
        memcpy(buffer.payload + offset, &word, 4);
    }
    for (; offset < buffer.bytes; ++offset) {
        buffer.payload[offset] = 0xee;
    }
}

/* Lines are "<tag> <label>[<element>].<word> <hex> [<float>]"; byte arrays
   use ".b<byte>". */
static void printWords(const char* tag, const char* label, const unsigned char* data, int bytes,
                       int element_bytes, int is_float)
{
    int offset;
    if (element_bytes < 4) {
        for (offset = 0; offset < bytes; ++offset) {
            printf("%s %s[%d].b%d %02x\n", tag, label, offset / element_bytes,
                   offset % element_bytes, data[offset]);
        }
        return;
    }
    for (offset = 0; offset + 4 <= bytes; offset += 4) {
        unsigned long word;
        memcpy(&word, data + offset, 4);
        if (is_float) {
            printf("%s %s[%d].%d %08lx %.9g\n", tag, label, offset / element_bytes,
                   (offset % element_bytes) / 4, word, static_cast<double>(bitsFloat(word)));
        } else {
            printf("%s %s[%d].%d %08lx\n", tag, label, offset / element_bytes,
                   (offset % element_bytes) / 4, word);
        }
    }
}

static void printGuards(const char* label, const Buffer& buffer)
{
    int offset;
    for (offset = -GUARD_BYTES; offset < 0; ++offset) {
        if (buffer.payload[offset] != GUARD_FILL) {
            printf("guard %s underrun at %d\n", label, offset);
            return;
        }
    }
    for (offset = 0; offset < GUARD_BYTES; ++offset) {
        if (buffer.payload[buffer.bytes + offset] != GUARD_FILL) {
            printf("guard %s overrun at +%d\n", label, offset);
            return;
        }
    }
    printf("guard %s ok\n", label);
}

static int isFloatDomain(int domain)
{
    return domain == DOM_FLOAT || domain == DOM_POSITIVE || domain == DOM_UNITISH;
}

struct CaseSpec {
    char name[128];
    const Operation* operation;
    int alias;
    SRDWORD count;
    unsigned long seed;
    /* Optional fixed inputs; null means generated. */
    const float* source0;
    const float* matrix;
    const float* constants;
    const SRDWORD* indices;
    SRDWORD pool;
};

static srVP* g_vp;
static int g_window_first = -1;
static int g_window_count = 0;

static int filterException(unsigned long code, unsigned long* stored)
{
    *stored = code;
    return EXCEPTION_EXECUTE_HANDLER;
}

static int invokeProtected(const Operation* operation, Context& context, unsigned long* code)
{
    __try {
        operation->run(g_vp, context);
    } __except (filterException(GetExceptionCode(), code)) {
        return 0;
    }
    return 1;
}

static void runCase(const CaseSpec& spec)
{
    const Operation* operation = spec.operation;
    Context context;
    Buffer destination;
    Buffer destination_1;
    Buffer sources[3];
    Buffer indices;
    SRDWORD count = spec.count;
    SRDWORD pool = spec.pool != 0 ? spec.pool : count * 2 + 1;
    SRDWORD first = 0;
    int index;
    unsigned long code = 0;
    char label[16];

    if (g_window_first >= 0) {
        first = static_cast<SRDWORD>(g_window_first);
        if (first + g_window_count > count) {
            printf("case %s\nerror window outside case\nend %s\n", spec.name, spec.name);
            return;
        }
    }

    memset(&context, 0, sizeof(context));
    memset(sources, 0, sizeof(sources));
    g_rng = hashName(spec.name) ^ (spec.seed * 2654435761UL);
    if (g_rng == 0) {
        g_rng = 1;
    }

    /* Generation order is fixed: matrix, constants, sources, indices. */
    if (operation->uses_matrix) {
        float* m = &context.matrix.vectors[0].x;
        if (spec.matrix != 0) {
            memcpy(m, spec.matrix, sizeof(float) * 16);
        } else {
            for (index = 0; index < 16; ++index) {
                m[index] = randomFloat(DOM_FLOAT);
            }
            if ((nextRandom() & 1) != 0) {
                m[12] = 0.0f;
                m[13] = 0.0f;
                m[14] = 0.0f;
                m[15] = 1.0f;
            }
        }
    }
    for (index = 0; index < operation->constant_count; ++index) {
        if (spec.constants != 0) {
            context.constants[index] = spec.constants[index];
        } else if (operation->constant_domain == DOM_DWORD ||
                   operation->constant_domain == DOM_SHIFT) {
            unsigned long word = nextRandom();
            if (operation->constant_domain == DOM_SHIFT) {
                word &= 31;
            }
            context.constants[index] = bitsFloat(word);
        } else {
            context.constants[index] = randomFloat(operation->constant_domain);
        }
    }
    for (index = 0; index < 3; ++index) {
        const Array& array = operation->source[index];
        SRDWORD elements;
        if (array.bytes == 0) {
            continue;
        }
        elements = operation->indexed_source == index ? pool : count;
        allocateBuffer(sources[index], array.bytes * static_cast<int>(elements));
        if (index == 0 && spec.source0 != 0) {
            memcpy(sources[index].payload, spec.source0, sources[index].bytes);
        } else if (array.domain == DOM_PERM) {
            SRDWORD element;
            SRDWORD* words = reinterpret_cast<SRDWORD*>(sources[index].payload);
            for (element = 0; element < elements; ++element) {
                words[element] = element;
            }
            for (element = elements; element > 1; --element) {
                SRDWORD other = nextRandom() % element;
                SRDWORD swap = words[element - 1];
                words[element - 1] = words[other];
                words[other] = swap;
            }
        } else {
            fillWords(sources[index].payload, sources[index].bytes, array.domain);
        }
        context.source[index] = sources[index].payload;
    }
    if (operation->indexed_source >= 0) {
        SRDWORD element;
        allocateBuffer(indices, 4 * static_cast<int>(count));
        for (element = 0; element < count; ++element) {
            SRDWORD value = spec.indices != 0 ? spec.indices[element] : nextRandom() % pool;
            memcpy(indices.payload + 4 * element, &value, 4);
        }
        context.indices = reinterpret_cast<const SRDWORD*>(indices.payload);
    }

    if (operation->destination.bytes != 0) {
        SRDWORD elements =
            operation->minimum_count == 1 && operation->destination_1.bytes != 0 ? 1 : count;
        if (strcmp(operation->name, "collectNonZero") == 0 ||
            strcmp(operation->name, "collectPos") == 0 ||
            strcmp(operation->name, "collectNeg") == 0 ||
            strcmp(operation->name, "cullNoClip") == 0) {
            elements = count;
        }
        if (spec.alias == ALIAS_DEST_SOURCE0) {
            destination = sources[0];
        } else {
            allocateBuffer(destination, operation->destination.bytes * static_cast<int>(elements));
            fillDestinationPattern(destination);
        }
        context.destination = destination.payload;
    }
    if (operation->destination_1.bytes != 0) {
        SRDWORD elements = operation->minimum_count == 1 ? 1 : count;
        allocateBuffer(destination_1, operation->destination_1.bytes * static_cast<int>(elements));
        fillDestinationPattern(destination_1);
        context.destination_1 = destination_1.payload;
    }
    context.count = count;

    /* A window narrows an elementwise case to [first, first + count) of the
       same generated inputs; the driver uses it to minimize reproducers. */
    if (g_window_first >= 0) {
        for (index = 0; index < 3; ++index) {
            if (context.source[index] != 0 && operation->indexed_source != index) {
                context.source[index] = static_cast<const unsigned char*>(context.source[index]) +
                                        first * operation->source[index].bytes;
                sources[index].payload += first * operation->source[index].bytes;
                sources[index].bytes = g_window_count * operation->source[index].bytes;
            }
        }
        if (context.indices != 0) {
            context.indices += first;
            indices.payload += first * 4;
            indices.bytes = g_window_count * 4;
        }
        if (context.destination != 0) {
            if (spec.alias == ALIAS_DEST_SOURCE0) {
                destination = sources[0];
            } else {
                destination.payload += first * operation->destination.bytes;
                destination.bytes = g_window_count * operation->destination.bytes;
            }
            context.destination = destination.payload;
        }
        count = static_cast<SRDWORD>(g_window_count);
        context.count = count;
        printf("case %s@%lu+%lu\n", spec.name, first, count);
    } else {
        printf("case %s\n", spec.name);
    }

    printf("op %s alias %s count %lu\n", operation->name,
           spec.alias == ALIAS_DEST_SOURCE0 ? "inplace" : "separate", count);
    if (operation->uses_matrix) {
        printWords("in", "matrix", reinterpret_cast<const unsigned char*>(&context.matrix), 64, 16,
                   1);
    }
    for (index = 0; index < operation->constant_count; ++index) {
        unsigned long word = floatBits(context.constants[index]);
        printf("in k[%d] %08lx\n", index, word);
    }
    for (index = 0; index < 3; ++index) {
        if (context.source[index] != 0) {
            sprintf(label, "s%d", index);
            printWords("in", label, sources[index].payload, sources[index].bytes,
                       operation->source[index].bytes,
                       isFloatDomain(operation->source[index].domain));
        }
    }
    if (context.indices != 0) {
        printWords("in", "idx", indices.payload, indices.bytes, 4, 0);
    }

    if (!invokeProtected(operation, context, &code)) {
        printf("crash %08lx\n", code);
        printf("end %s\n", spec.name);
        return;
    }

    if (context.destination != 0) {
        printWords("out", "d", destination.payload, destination.bytes, operation->destination.bytes,
                   isFloatDomain(operation->destination.domain));
        if (g_window_first < 0) {
            printGuards("d", destination);
        }
    }
    if (context.destination_1 != 0) {
        printWords("out", "d1", destination_1.payload, destination_1.bytes,
                   operation->destination_1.bytes, isFloatDomain(operation->destination_1.domain));
        printGuards("d1", destination_1);
    }
    if (context.has_word) {
        printf("ret %08lx\n", context.word);
    }
    if (context.has_double) {
        unsigned long words[2];
        memcpy(words, &context.value, 8);
        printf("ret %08lx%08lx %.17g\n", words[1], words[0], context.value);
    }
    printf("end %s\n", spec.name);
    fflush(stdout);
    /* Buffers are deliberately leaked: the process is short-lived and a
       clobbered guard must not turn into a heap failure in a later case. */
}

/* ------------------------------------------------------------------------ */
/* Fixed cases.                                                             */

static const float g_identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
/* Rows are srMatrix4::vectors; x' = m[0]x + m[1]y + m[2]z + m[3]. */
static const float g_swap_xy[16] = {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
static const float g_translate[16] = {1, 0, 0, 10, 0, 1, 0, -20, 0, 0, 1, 30, 0, 0, 0, 1};
static const float g_rotate_z90[16] = {0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
/* 30 degrees about X, plus translation; general (non-dyadic) coefficients. */
static const float g_rotate_x30[16] = {1, 0,    0,          1.5f, 0, 0.8660254f, -0.5f, -2.25f,
                                       0, 0.5f, 0.8660254f, 4,    0, 0,          0,     1};
/* Perspective-like and orthographic-like layouts the renderer classifies. */
static const float g_perspective[16] = {1.5f, 0, 0.25f, 0,      0, 2, -0.125f, 0,
                                        0,    0, 1.01f, -2.02f, 0, 0, 1,       0};
static const float g_ortho[16] = {0.5f, 0, 0,      -1,     0, 0.25f, 0, 0.5f,
                                  0,    0, 0.125f, -0.75f, 0, 0,     0, 1};
static const float g_general[16] = {0.5f, 1, -2,    3,  4,  -0.25f, 6, -7,
                                    8,    9, 0.75f, 11, -1, 2,      3, 0.5f};

static const float g_vector_123[4] = {1, 2, 3, 1};
static const float g_vector3_many[12] = {1, 2, 3, -4, 5, -6, 0.5f, 0.25f, -0.125f, 7, 8, 9};
static const float g_vector4_many[16] = {1,    2,     3,       1,  -4, 5, -6, 2,
                                         0.5f, 0.25f, -0.125f, -1, 7,  8, 9,  0.5f};
static const SRDWORD g_indices_repeat[4] = {2, 0, 2, 1};

void probeCases();

static int g_selected_count;
static char** g_selected;
static int g_list_only;

static int selected(const char* name)
{
    int index;
    if (g_selected_count == 0) {
        return 1;
    }
    for (index = 0; index < g_selected_count; ++index) {
        size_t length = strlen(g_selected[index]);
        if (g_selected[index][length - 1] == '*') {
            if (strncmp(g_selected[index], name, length - 1) == 0) {
                return 1;
            }
        } else if (strcmp(g_selected[index], name) == 0) {
            return 1;
        }
    }
    return 0;
}

int probeSelected(const char* name)
{
    return selected(name);
}

int probeListOnly()
{
    return g_list_only;
}

static void submit(CaseSpec& spec)
{
    if (!selected(spec.name)) {
        return;
    }
    if (g_list_only) {
        printf("%s\n", spec.name);
        return;
    }
    runCase(spec);
}

static void fixedTransform(const char* op_name, const char* tag, const float* matrix,
                           const float* source, SRDWORD count, const SRDWORD* idx, SRDWORD pool)
{
    CaseSpec spec;
    const Operation* operation = findOperation(op_name);
    int alias;
    for (alias = 0; alias < 2; ++alias) {
        if (alias == ALIAS_DEST_SOURCE0 && (operation->alias & ALIAS_DEST_SOURCE0) == 0) {
            continue;
        }
        memset(&spec, 0, sizeof(spec));
        sprintf(spec.name, "fixed.%s.%s.%s", op_name, tag, alias ? "inplace" : "separate");
        spec.operation = operation;
        spec.alias = alias;
        spec.count = count;
        spec.matrix = matrix;
        spec.source0 = source;
        spec.indices = idx;
        spec.pool = pool;
        submit(spec);
    }
}

static void fixedCases()
{
    static const char* transform_ops[] = {"transform3", "transform4", "transform3to4",
                                          "transformOrtho", "transformPerspective"};
    static const char* matrix_names[] = {"identity", "swapxy", "translate", "rotz90",
                                         "rotx30",   "persp",  "ortho",     "general"};
    static const float* const matrices[] = {g_identity,   g_swap_xy,     g_translate, g_rotate_z90,
                                            g_rotate_x30, g_perspective, g_ortho,     g_general};
    int op;
    int m;

    /* The user's exact case first: X/Y swap of (1, 2, 3). */
    fixedTransform("transform3", "swapxy.v123", g_swap_xy, g_vector_123, 1, 0, 0);
    for (op = 0; op < 5; ++op) {
        int vector4 = strcmp(transform_ops[op], "transform3") != 0 &&
                      strcmp(transform_ops[op], "transform3to4") != 0;
        const float* many = vector4 ? g_vector4_many : g_vector3_many;
        for (m = 0; m < 8; ++m) {
            char tag[64];
            sprintf(tag, "%s.n4", matrix_names[m]);
            fixedTransform(transform_ops[op], tag, matrices[m], many, 4, 0, 0);
        }
        fixedTransform(transform_ops[op], "general.n0", g_general, many, 0, 0, 0);
        fixedTransform(transform_ops[op], "general.n1", g_general, many, 1, 0, 0);
    }
    fixedTransform("transformIndexed3", "swapxy.repeat", g_swap_xy, g_vector3_many, 4,
                   g_indices_repeat, 4);
    fixedTransform("transformIndexed3to4", "general.repeat", g_general, g_vector3_many, 4,
                   g_indices_repeat, 4);
    fixedTransform("transformIndexed3", "general.n0", g_general, g_vector3_many, 0,
                   g_indices_repeat, 4);
}

/* Every operation in each permitted alias mode at counts covering zero, one
   and many elements (and chunk boundaries of chunked implementations). */
static void generatedCases(unsigned long seed, int rounds)
{
    static const SRDWORD counts[] = {0, 1, 2, 3, 5, 8, 17, 64};
    int op;
    int alias;
    int count_index;
    int round;
    for (op = 0; op < g_operation_count; ++op) {
        const Operation* operation = &g_operations[op];
        for (alias = 0; alias < 2; ++alias) {
            if (alias == ALIAS_DEST_SOURCE0 && (operation->alias & ALIAS_DEST_SOURCE0) == 0) {
                continue;
            }
            for (count_index = 0; count_index < 8; ++count_index) {
                SRDWORD count = counts[count_index];
                if (count < operation->minimum_count) {
                    continue;
                }
                if (strcmp(operation->name, "mulMatrix") == 0 && count != 1) {
                    continue;
                }
                if (strcmp(operation->name, "testBoundingBox") == 0 && count != 1) {
                    continue;
                }
                for (round = 0; round < rounds; ++round) {
                    CaseSpec spec;
                    memset(&spec, 0, sizeof(spec));
                    sprintf(spec.name, "gen.%s.%s.n%lu.r%d", operation->name,
                            alias ? "inplace" : "separate", count, round);
                    spec.operation = operation;
                    spec.alias = alias;
                    spec.count = count;
                    spec.seed = seed;
                    submit(spec);
                }
            }
        }
    }
}

static void printSlots()
{
    int index;
    unsigned long base = reinterpret_cast<unsigned long>(GetModuleHandleA("sr.dll"));
    const unsigned long* vtable = *reinterpret_cast<unsigned long* const*>(g_vp);
    for (index = 0; index < g_operation_count; ++index) {
        int offset = g_operations[index].slot();
        printf("slot %s 0x%03x\n", g_operations[index].name, offset);
        if (offset >= 0) {
            printf("info vtable %s rva 0x%08lx\n", g_operations[index].name,
                   vtable[offset / 4] - base);
        }
    }
}

int main(int argc, char** argv)
{
    unsigned long seed = 1;
    int rounds = 2;
    int index;
    HMODULE module;
    srVP** exported_vp;

    g_selected = static_cast<char**>(malloc(sizeof(char*) * (argc + 1)));
    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--seed") == 0 && index + 1 < argc) {
            seed = strtoul(argv[++index], 0, 0);
        } else if (strcmp(argv[index], "--generated") == 0 && index + 1 < argc) {
            rounds = atoi(argv[++index]);
        } else if (strcmp(argv[index], "--window") == 0 && index + 2 < argc) {
            g_window_first = atoi(argv[++index]);
            g_window_count = atoi(argv[++index]);
        } else if (strcmp(argv[index], "--list") == 0) {
            g_list_only = 1;
        } else {
            g_selected[g_selected_count++] = argv[index];
        }
    }

    if (g_list_only) {
        fixedCases();
        generatedCases(seed, rounds);
        probeCases();
        return 0;
    }

    printf("runner sr-difftest 1\n");
    printf("seed %lu rounds %d\n", seed, rounds);

    module = GetModuleHandleA("sr.dll");
    exported_vp =
        reinterpret_cast<srVP**>(GetProcAddress(module, "?vp@srVectorProcessor@@0PAVsrVP@@A"));
    printf("vp-before-init %s\n", exported_vp == 0 ? "missing" : *exported_vp ? "set" : "null");
    srVectorProcessor::initBaseVP();
    if (exported_vp == 0 || *exported_vp == 0) {
        printf("error initBaseVP installed no processor\n");
        return 2;
    }
    g_vp = *exported_vp;
    printf("vp-name %s\n", srVectorProcessor::getName() ? srVectorProcessor::getName() : "(null)");
    printSlots();

    fixedCases();
    generatedCases(seed, rounds);

    srVectorProcessor::release();
    printf("vp-after-release %s\n", *exported_vp ? "set" : "null");
    probeCases();
    printf("done\n");
    return 0;
}
