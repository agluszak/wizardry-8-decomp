#include "surrender/srDebugVP.h"

#include "surrender/srCore.h"

// GLOBAL: SURRENDER 0x100A9250
const char* srDebugVP::command_names[COMMAND_COUNT];

// FUNCTION: SURRENDER 0x10068FD0
srDebugVP::srDebugVP(srVP* processor)
{
    int iteration;

    this->processor = processor;
    call_times[0] = 0.0;
    for (iteration = 0; iteration < 0x2710; ++iteration) {
        ScopeTimer scope(this, 0, COMMAND_DUMMY, 0, 0, 0, 0);
    }
    call_overhead = call_times[0] * 0.0001;
    resetInternalStatistics();
    command_names[COMMAND_DUMMY] = "dummy command";
    command_names[COMMAND_MEMCMP] =
        "_memcmp  (const void* src0, const void* src1,  const SRDWORD bytes)";
    command_names[COMMAND_MEMCOPY_VOID_ARRAY_VOID_ARRAY] =
        "_memcopy (void* dest, const void* src,  const SRDWORD bytes)";
    command_names[COMMAND_MEMCOPY_VOID_ARRAY_BYTE] =
        "_memcopy (void* dest, const SRBYTE src,  const SRDWORD bytes)";
    command_names[COMMAND_PREFETCH] = "_prefetch (const void* dest, const SRDWORD bytes)";
    command_names[COMMAND_COPY_INTERLEAVED] =
        "_copyInterleaved(void* dest, const void* src, SRDWORD dstPitch, SRDWORD "
        "srcPitch, SRDWORD width, SRDWORD n)";
    command_names[COMMAND_SWAP] = "_swap (void* d0, void* d1, const SRDWORD bytes)";
    command_names[COMMAND_COPY_DWORD_ARRAY_DWORD] =
        "_copy (SRDWORD* dest, const SRDWORD c, const SRDWORD n)";
    command_names[COMMAND_REVERSE] = "_reverse (SRDWORD* dest, const SRDWORD* s, const SRDWORD n)";
    command_names[COMMAND_AND_DWORD_ARRAY_DWORD_ARRAY_DWORD] =
        "_and  (SRDWORD* dest, const SRDWORD* s,  const SRDWORD c, const SRDWORD n)";
    command_names[COMMAND_OR_DWORD_ARRAY_DWORD_ARRAY_DWORD] =
        "_or  (SRDWORD* dest, const SRDWORD* s,  const SRDWORD c, const SRDWORD n)";
    command_names[COMMAND_XOR_DWORD_ARRAY_DWORD_ARRAY_DWORD] =
        "_xor  (SRDWORD* dest, const SRDWORD* s,  const SRDWORD c, const SRDWORD n)";
    command_names[COMMAND_AND_DWORD_ARRAY_DWORD_ARRAY_DWORD_ARRAY] =
        "_and  (SRDWORD* dest, const SRDWORD* s0,  const SRDWORD* s1, const SRDWORD n)";
    command_names[COMMAND_OR_DWORD_ARRAY_DWORD_ARRAY_DWORD_ARRAY] =
        "_or  (SRDWORD* dest, const SRDWORD* s0,  const SRDWORD* s1, const SRDWORD n)";
    command_names[COMMAND_XOR_DWORD_ARRAY_DWORD_ARRAY_DWORD_ARRAY] =
        "_xor  (SRDWORD* dest, const SRDWORD* s0,  const SRDWORD* s1, const SRDWORD n)";
    command_names[COMMAND_ASR] =
        "_asr  (SRDWORD* dest, const SRDWORD* s0, const SRDWORD sh, const SRDWORD n)";
    command_names[COMMAND_ASR_AND] =
        "_asrAnd  (SRDWORD* dest, const SRDWORD* s0, const SRDWORD sh, const "
        "SRDWORD mask, const SRDWORD n)";
    command_names[COMMAND_LSR] =
        "_lsr  (SRDWORD* dest, const SRDWORD* s0, const SRDWORD sh, const SRDWORD n)";
    command_names[COMMAND_LSL] =
        "_lsl  (SRDWORD* dest, const SRDWORD* s0, const SRDWORD sh, const SRDWORD n)";
    command_names[COMMAND_LSL_AND] =
        "_lslAnd  (SRDWORD* dest, const SRDWORD* s0, const SRDWORD sh, const "
        "SRDWORD mask, const SRDWORD n)";
    command_names[COMMAND_IS_EQUAL_DWORD_ARRAY_DWORD] =
        "_isEqual (const SRDWORD* s0, const SRDWORD c,  const SRDWORD n)";
    command_names[COMMAND_IS_EQUAL_DWORD_ARRAY_DWORD_ARRAY] =
        "_isEqual (const SRDWORD* s0, const SRDWORD* s1,  const SRDWORD n)";
    command_names[COMMAND_MIN_DWORD_ARRAY] = "_min  (const SRDWORD* s0, const SRDWORD n)";
    command_names[COMMAND_MAX_DWORD_ARRAY] = "_max  (const SRDWORD* s0, const SRDWORD n)";
    command_names[COMMAND_COPY_INDEXED_DWORD_ARRAY_DWORD_ARRAY_DWORD_ARRAY] =
        "_copyIndexed (SRDWORD* dst, const SRDWORD* src,  const SRDWORD* ixTable, SRDWORD n)";
    command_names[COMMAND_ADD_S_BYTE_ARRAY_BYTE_ARRAY_BYTE] =
        "_addS  (SRBYTE* d,  const SRBYTE* s,  const SRBYTE c,  const SRDWORD n)";
    command_names[COMMAND_SUB_S_BYTE_ARRAY_BYTE_ARRAY_BYTE] =
        "_subS  (SRBYTE* d,  const SRBYTE* s,  const SRBYTE c,  const SRDWORD n)";
    command_names[COMMAND_SUB_S_BYTE_ARRAY_BYTE_BYTE_ARRAY] =
        "_subS  (SRBYTE* d,  const SRBYTE c,  const SRBYTE* s, const SRDWORD n)";
    command_names[COMMAND_ADD_S_BYTE_ARRAY_BYTE_ARRAY_BYTE_ARRAY] =
        "_addS  (SRBYTE* d,  const SRBYTE* s0,  const SRBYTE* s1, const SRDWORD n)";
    command_names[COMMAND_SUB_S_BYTE_ARRAY_BYTE_ARRAY_BYTE_ARRAY] =
        "_subS  (SRBYTE* d,  const SRBYTE* s0,  const SRBYTE* s1, const SRDWORD n)";
    command_names[COMMAND_TO_FLOAT] = "_toFloat (float* d,  const SRBYTE* s,  const SRDWORD n)";
    command_names[COMMAND_ADD_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY] =
        "_add (float* d, const float cf, const float* source, const SRDWORD n)";
    command_names[COMMAND_SUB_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY] =
        "_sub (float* d, const float cf, const float* source, const SRDWORD n)";
    command_names[COMMAND_MUL_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY] =
        "_mul (float* d, const float cf, const float* multiplier,const SRDWORD n)";
    command_names[COMMAND_DIV_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY] =
        "_div (float* d, const float cf, const float* divisor, const SRDWORD n)";
    command_names[COMMAND_ADD_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY] =
        "_add (float* d, const float *fs0, const float *fs1, const SRDWORD n)";
    command_names[COMMAND_SUB_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY] =
        "_sub (float* d, const float *fs0, const float *fs1, const SRDWORD n)";
    command_names[COMMAND_MUL_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY] =
        "_mul (float* d, const float *fs0, const float *fs1, const SRDWORD n)";
    command_names[COMMAND_DIV_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY] =
        "_div (float* d, const float *fs0, const float *fs1, const SRDWORD n)";
    command_names[COMMAND_MUL_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY_FLOAT_ARRAY] =
        "_mul (float* d, const float c, const float *fs0, const float *fs1, const SRDWORD n)";
    command_names[COMMAND_CLAMP] =
        "_clamp (float* dest, const float* source, const float min, const float "
        "max, const SRDWORD n);";
    command_names[COMMAND_CLAMP_MIN] =
        "_clampMin (float* dest, const float* source, const float min, const SRDWORD n);";
    command_names[COMMAND_CLAMP_MAX] =
        "_clampMax (float* dest, const float* source, const float max, const SRDWORD n);";
    command_names[COMMAND_CLAMP_UNIT] =
        "_clampUnit (float* dest, const float* source, const SRDWORD n);";
    command_names[COMMAND_SQRT] = "_sqrt (float* dest, const float* source, SRDWORD n);";
    command_names[COMMAND_ISQRT] = "_isqrt (float* dest, const float* source, SRDWORD n);";
    command_names[COMMAND_LERP] =
        "_lerp (float* dest, const float* target, const float* source, const float "
        "constant, SRDWORD n);";
    command_names[COMMAND_IS_NEG] = "_isNeg (const float* dest, const SRDWORD n);";
    command_names[COMMAND_IS_POS] = "_isPos (const float* dest, const SRDWORD n);";
    command_names[COMMAND_IS_ZERO] = "_isZero (const float* dest, const SRDWORD n);";
    command_names[COMMAND_MIN_FLOAT_ARRAY] = "_min (const float* src, const SRDWORD n);";
    command_names[COMMAND_MAX_FLOAT_ARRAY] = "_max (const float* src, const SRDWORD n);";
    command_names[COMMAND_MIN_MAX_FLOAT_ARRAY_FLOAT_CONSTANT_FLOAT_CONSTANT] =
        "_minMax (const float* src, float& min, float& max, const SRDWORD n)";
    command_names[COMMAND_SUM] = "_sum (const float* src, const SRDWORD n);";
    command_names[COMMAND_AXPY_FLOAT_ARRAY_FLOAT_FLOAT_FLOAT_ARRAY] =
        "_axpy (float* dest, const float ca, const float constant, const float* "
        "srcm, const SRDWORD n);";
    command_names[COMMAND_AXPY_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY_FLOAT_ARRAY] =
        "_axpy (float* dest, const float ca, const float* srcs, const float* srcm, "
        "const SRDWORD n);";
    command_names[COMMAND_AXPY_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY] =
        "_axpy (float* dest, const float* srca, const float constant, const float* "
        "srcm, const SRDWORD n);";
    command_names[COMMAND_AXPY_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY] =
        "_axpy (float* dest, const float* srca, const float* srcs, const float* "
        "srcm, const SRDWORD n);";
    command_names[COMMAND_AXPY_FLOAT_ARRAY_FLOAT_FLOAT_FLOAT_ARRAY_FLOAT_ARRAY] =
        "_axpy (float* dest, const float ca, const float scale, const float* srcs, "
        "const float* srcm, const SRDWORD n)";
    command_names[COMMAND_AXPY_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY_FLOAT_ARRAY] =
        "_axpy (float* dest, const float* srca, const float scale, const float* "
        "srcs, const float* srcm, const SRDWORD n)";
    command_names[COMMAND_MUL_INDEXED_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY_DWORD_ARRAY] =
        "_mulIndexed (float* dest, const float* linear, const float* indexed, "
        "const SRDWORD* indices, const SRDWORD n); ";
    command_names[COMMAND_MUL_INDEXED_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY_DWORD_ARRAY] =
        "_mulIndexed (float* dest, const float constant, const float* indexed, "
        "const SRDWORD* indices, const SRDWORD n); ";
    command_names[COMMAND_TO_INT] = "_toInt  (SRLONG* dest, const float* fs, const SRDWORD n); ";
    command_names[COMMAND_INV_POLY] =
        "_invPoly\t\t(float* dest, const float* source, const srVector3& poly, "
        "const SRDWORD count)";
    command_names[COMMAND_ABS] = "_abs (float* dest, const float* source, const SRDWORD n)";
    command_names[COMMAND_NEG] = "_neg (float* dest, const float* source, const SRDWORD n)";
    command_names[COMMAND_CUBIC] =
        "_cubic (float* dest, const float* source, const SRDWORD count);";
    command_names[COMMAND_COPY_VEC3_ARRAY_VEC3_CONSTANT] =
        "_copy  (srVector3* dest, const srVector3& cv, const SRDWORD n)";
    command_names[COMMAND_COPY_VEC3_ARRAY_VEC4_ARRAY] =
        "_copy  (srVector3* dest, const srVector4* vs, const SRDWORD n)";
    command_names[COMMAND_ADD_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY] =
        "_add  (srVector3* dest, const srVector3& cv, const srVector3* vs, const SRDWORD n)";
    command_names[COMMAND_SUB_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY] =
        "_sub  (srVector3* dest, const srVector3& cv, const srVector3* vs, const SRDWORD n)";
    command_names[COMMAND_MUL_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY] =
        "_mul  (srVector3* dest, const srVector3& cv, const srVector3* vs, const SRDWORD n)";
    command_names[COMMAND_DIV_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY] =
        "_div  (srVector3* dest, const srVector3& cv, const srVector3* vs, const SRDWORD n)";
    command_names[COMMAND_ADD_VEC3_ARRAY_VEC3_CONSTANT_FLOAT_ARRAY] =
        "_add  (srVector3* dest, const srVector3& cv, const float* fs, const SRDWORD n)";
    command_names[COMMAND_SUB_VEC3_ARRAY_VEC3_CONSTANT_FLOAT_ARRAY] =
        "_sub  (srVector3* dest, const srVector3& cv, const float* fs, const SRDWORD n)";
    command_names[COMMAND_MUL_VEC3_ARRAY_VEC3_CONSTANT_FLOAT_ARRAY] =
        "_mul  (srVector3* dest, const srVector3& cv, const float* fs, const SRDWORD n)";
    command_names[COMMAND_DIV_VEC3_ARRAY_VEC3_CONSTANT_FLOAT_ARRAY] =
        "_div  (srVector3* dest, const srVector3& cv, const float* fs, const SRDWORD n)";
    command_names[COMMAND_ADD_VEC3_ARRAY_VEC3_ARRAY_FLOAT_ARRAY] =
        "_add  (srVector3* dest, const srVector3* vs, const float* fs, const SRDWORD n)";
    command_names[COMMAND_SUB_VEC3_ARRAY_VEC3_ARRAY_FLOAT_ARRAY] =
        "_sub  (srVector3* dest, const srVector3* vs, const float* fs, const SRDWORD n)";
    command_names[COMMAND_MUL_VEC3_ARRAY_VEC3_ARRAY_FLOAT_ARRAY] =
        "_mul  (srVector3* dest, const srVector3* vs, const float* fs, const SRDWORD n)";
    command_names[COMMAND_DIV_VEC3_ARRAY_VEC3_ARRAY_FLOAT_ARRAY] =
        "_div  (srVector3* dest, const srVector3* vs, const float* fs, const SRDWORD n)";
    command_names[COMMAND_SUB_VEC3_ARRAY_FLOAT_ARRAY_VEC3_ARRAY] =
        "_sub  (srVector3* dest, const float* fs, const srVector3* vs, const SRDWORD n)";
    command_names[COMMAND_DIV_VEC3_ARRAY_FLOAT_ARRAY_VEC3_ARRAY] =
        "_div  (srVector3* dest, const float* fs, const srVector3* vs, const SRDWORD n)";
    command_names[COMMAND_DOT_FLOAT_ARRAY_VEC3_CONSTANT_VEC3_ARRAY] =
        "_dot  (float* dest, const srVector3& cv, const srVector3* vs, const SRDWORD n)";
    command_names[COMMAND_DOT_FLOAT_ARRAY_VEC3_ARRAY_VEC3_ARRAY] =
        "_dot  (float* dest, const srVector3* vs0, const srVector3* vs1, const SRDWORD n)";
    command_names[COMMAND_CROSS] =
        "_cross  (srVector3* dest, const srVector3* vs0, const srVector3* vs1, const SRDWORD n)";
    command_names[COMMAND_LENGTH_FLOAT_ARRAY_VEC3_ARRAY] =
        "_length (float* dest, const srVector3* vs, SRDWORD n)";
    command_names[COMMAND_NORMALIZE_VEC3_ARRAY_VEC3_ARRAY_FLOAT] =
        "_normalize (srVector3* dest, const srVector3* vs, const float length, const SRDWORD n)";
    command_names[COMMAND_MIN_MAX_VEC3_ARRAY_VEC3_CONSTANT_VEC3_CONSTANT] =
        "_minMax  (const srVector3* s0, srVector3& min, srVector3& max, const SRDWORD n)";
    command_names[COMMAND_TRANSFORM_VEC3_ARRAY_VEC3_ARRAY_MAT4_CONSTANT] =
        "_transform (srVector3* dest, const srVector3* vs, const srMatrix4& "
        "matrix, const SRDWORD n)";
    command_names[COMMAND_COPY_INDEXED_VEC3_ARRAY_VEC2_ARRAY_DWORD_ARRAY] =
        "_copyIndexed (srVector3* dest, const srVector2* src, const SRDWORD* indices, SRDWORD n)";
    command_names[COMMAND_COPY_INDEXED_VEC3_ARRAY_VEC3_ARRAY_DWORD_ARRAY] =
        "_copyIndexed (srVector3* dest, const srVector3* src, const SRDWORD* indices, SRDWORD n)";
    command_names[COMMAND_COPY_INDEXED_VEC3_ARRAY_VEC4_ARRAY_DWORD_ARRAY] =
        "_copyIndexed (srVector3* dest, const srVector4* src, const SRDWORD* indices, SRDWORD n)";
    command_names[COMMAND_MUL_INDEXED_VEC3_ARRAY_VEC3_ARRAY_VEC3_ARRAY_DWORD_ARRAY] =
        "_mulIndexed (srVector3* dest, const srVector3* linearSource, const "
        "srVector3* indexedSource, const SRDWORD* indices, SRDWORD n)";
    command_names[COMMAND_MUL_INDEXED_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY_DWORD_ARRAY] =
        "_mulIndexed (srVector3* dest, const srVector3& constant, const srVector3* "
        "indexedSource,  const SRDWORD* indices, SRDWORD n)";
    command_names[COMMAND_DIR_VEC3_ARRAY_FLOAT_ARRAY_VEC3_ARRAY] =
        "_dir  (srVector3* dst, float* dst2,  const srVector3* src, const SRDWORD n)";
    command_names[COMMAND_DIR_VEC3_ARRAY_FLOAT_ARRAY_VEC4_ARRAY] =
        "_dir  (srVector3* dst, float* dst2,  const srVector4* src, const SRDWORD n)";
    command_names[COMMAND_COPY_VEC4_ARRAY_VEC4_CONSTANT] =
        "_copy  (srVector4* dest, const srVector4& constant, const SRDWORD n);";
    command_names[COMMAND_COPY_VEC4_ARRAY_VEC3_ARRAY_FLOAT] =
        "_copy  (srVector4* dest, const srVector3* s0, const float c, const SRDWORD n);";
    command_names[COMMAND_COPY_VEC4_ARRAY_VEC3_ARRAY_FLOAT_ARRAY] =
        "_copy  (srVector4* dest, const srVector3* s0, const float* s1, const SRDWORD n)";
    command_names[COMMAND_COPY_W_VEC4_ARRAY_FLOAT] =
        "_copyW  (srVector4* d, const float c, const SRDWORD n);";
    command_names[COMMAND_COPY_W_VEC4_ARRAY_FLOAT_ARRAY] =
        "_copyW  (srVector4* d, const float* s, const SRDWORD n);";
    command_names[COMMAND_COPY_W_FLOAT_ARRAY_VEC4_ARRAY] =
        "_copyW  (float*d, const srVector4* s, const SRDWORD n);";
    command_names[COMMAND_ADD_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY] =
        "_add  (srVector4* dest, const srVector4& cv, const srVector4* vs, const SRDWORD n)";
    command_names[COMMAND_SUB_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY] =
        "_sub  (srVector4* dest, const srVector4& cv, const srVector4* vs, const SRDWORD n)";
    command_names[COMMAND_MUL_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY] =
        "_mul  (srVector4* dest, const srVector4& cv, const srVector4* vs, const SRDWORD n)";
    command_names[COMMAND_DIV_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY] =
        "_div  (srVector4* dest, const srVector4& cv, const srVector4* vs, const SRDWORD n)";
    command_names[COMMAND_ADD_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY] =
        "_add  (srVector4* dest, const srVector4& cv, const float* fs, const SRDWORD n)";
    command_names[COMMAND_SUB_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY] =
        "_sub  (srVector4* dest, const srVector4& cv, const float* fs, const SRDWORD n)";
    command_names[COMMAND_MUL_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY] =
        "_mul  (srVector4* dest, const srVector4& cv, const float* fs, const SRDWORD n)";
    command_names[COMMAND_DIV_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY] =
        "_div  (srVector4* dest, const srVector4& cv, const float* fs, const SRDWORD n)";
    command_names[COMMAND_ADD_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY] =
        "_add  (srVector4* dest, const srVector4* vs, const float* fs, const SRDWORD n)";
    command_names[COMMAND_SUB_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY] =
        "_sub  (srVector4* dest, const srVector4* vs, const float* fs, const SRDWORD n)";
    command_names[COMMAND_MUL_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY] =
        "_mul  (srVector4* dest, const srVector4* vs, const float* fs, const SRDWORD n)";
    command_names[COMMAND_DIV_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY] =
        "_div  (srVector4* dest, const srVector4* vs, const float* fs, const SRDWORD n)";
    command_names[COMMAND_SUB_VEC4_ARRAY_FLOAT_ARRAY_VEC4_ARRAY] =
        "_sub  (srVector4* dest, const float* fs, const srVector4* vs, const SRDWORD n)";
    command_names[COMMAND_DIV_VEC4_ARRAY_FLOAT_ARRAY_VEC4_ARRAY] =
        "_div  (srVector4* dest, const float* fs, const srVector4* vs, const SRDWORD n)";
    command_names[COMMAND_DOT_FLOAT_ARRAY_VEC4_CONSTANT_VEC4_ARRAY] =
        "_dot  (float* dest, const srVector4& cv, const srVector4* vs, const SRDWORD n)";
    command_names[COMMAND_DOT_FLOAT_ARRAY_VEC4_ARRAY_VEC4_ARRAY] =
        "_dot  (float* dest, const srVector4* vs0, const srVector4* vs1, const SRDWORD n)";
    command_names[COMMAND_LENGTH_FLOAT_ARRAY_VEC4_ARRAY] =
        "_length (float* dest, const srVector4* vs, SRDWORD n)";
    command_names[COMMAND_NORMALIZE_VEC4_ARRAY_VEC4_ARRAY_FLOAT] =
        "_normalize (srVector4* dest, const srVector4* vs, const float length, const SRDWORD n)";
    command_names[COMMAND_MIN_MAX_VEC4_ARRAY_VEC4_CONSTANT_VEC4_CONSTANT] =
        "_minMax  (const srVector4* s0, srVector4& min, srVector4& max, const SRDWORD n)";
    command_names[COMMAND_TRANSFORM_VEC4_ARRAY_VEC4_ARRAY_MAT4_CONSTANT] =
        "_transform  (srVector4* dst, const srVector4* src, const srMatrix4& m, const SRDWORD n)";
    command_names[COMMAND_TRANSFORM_VEC4_ARRAY_VEC3_ARRAY_MAT4_CONSTANT] =
        "_transform  (srVector4* dst, const srVector3* src, const srMatrix4& m, const SRDWORD n)";
    command_names[COMMAND_TRANSFORM_ORTHO] =
        "_transformOrtho  (srVector4* dst, const srVector4* src, const srMatrix4& "
        "m, const SRDWORD n)";
    command_names[COMMAND_TRANSFORM_PERSPECTIVE] =
        "_transformPerspective (srVector4* dst, const srVector4* src, const "
        "srMatrix4& m, const SRDWORD n)";
    command_names[COMMAND_AXPY_VEC4_ARRAY_VEC4_CONSTANT_VEC4_CONSTANT_FLOAT_ARRAY] =
        "_axpy  (srVector4* d, const srVector4& srca, const srVector4& c, const "
        "float* srcm, const SRDWORD n)";
    command_names[COMMAND_AXPY_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY_FLOAT_ARRAY] =
        "_axpy  (srVector4* d, const srVector4& srca, const srVector4* c, const "
        "float* srcm, const SRDWORD n)";
    command_names[COMMAND_AXPY_VEC4_ARRAY_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY] =
        "_axpy  (srVector4* d, const srVector4* srca, const srVector4& c, const "
        "float* srcm, const SRDWORD n)";
    command_names[COMMAND_AXPY_VEC4_ARRAY_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY] =
        "_axpy  (srVector4* d, const srVector4* srca, const srVector4* c, const "
        "float* srcm, const SRDWORD n)";
    command_names[COMMAND_AXPY_VEC4_ARRAY_VEC4_CONSTANT_VEC4_CONSTANT_FLOAT_ARRAY_FLOAT_ARRAY] =
        "_axpy  (srVector4* d, const srVector4& ca, const srVector4& cm, const "
        "float* srcm0, const float* srcm1, const SRDWORD n)";
    command_names[COMMAND_AXPY_VEC4_ARRAY_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY_FLOAT_ARRAY] =
        "_axpy  (srVector4* d, const srVector4* srca, const srVector4& cm, const "
        "float* srcm0, const float* srcm1, const SRDWORD n)";
    command_names[COMMAND_MUL_ADD_VEC4_ARRAY_VEC4_CONSTANT_VEC4_CONSTANT_VEC4_ARRAY] =
        "_mulAdd  (srVector4* d, const srVector4& ca, const srVector4& cm, const "
        "srVector4* srcm, const SRDWORD n);";
    command_names[COMMAND_MUL_ADD_VEC4_ARRAY_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY] =
        "_mulAdd  (srVector4* d, const srVector4* srca, const srVector4& cm, "
        "const srVector4* srcm, const SRDWORD n);";
    command_names[COMMAND_MUL_ADD_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY_VEC4_ARRAY] =
        "_mulAdd  (srVector4* d, const srVector4& ca, const srVector4* srcm0, "
        "const srVector4* srcm1, const SRDWORD n)";
    command_names[COMMAND_DIV_BY_W] =
        "_divByW  (srVector4* dst, const srVector4* src, const SRDWORD n)";
    command_names[COMMAND_COPY_INDEXED_VEC4_ARRAY_VEC2_ARRAY_DWORD_ARRAY] =
        "_copyIndexed (srVector4* dest, const srVector2* src, const SRDWORD* indices, SRDWORD n)";
    command_names[COMMAND_COPY_INDEXED_VEC4_ARRAY_VEC3_ARRAY_DWORD_ARRAY] =
        "_copyIndexed (srVector4* dest, const srVector3* src, const SRDWORD* indices, SRDWORD n)";
    command_names[COMMAND_COPY_INDEXED_VEC4_ARRAY_VEC4_ARRAY_DWORD_ARRAY] =
        "_copyIndexed (srVector4* dest, const srVector4* src, const SRDWORD* indices, SRDWORD n)";
    command_names[COMMAND_COPY_INDEXED_VEC4_ARRAY_ARGB_ARRAY_DWORD_ARRAY] =
        "_copyIndexed (srVector4* dest, const srARGB*    src, const SRDWORD* indices, SRDWORD n)";
    command_names[COMMAND_MUL_INDEXED_VEC4_ARRAY_VEC4_ARRAY_VEC4_ARRAY_DWORD_ARRAY] =
        "_mulIndexed (srVector4* dest, const srVector4* linearSource, const "
        "srVector4* indexedSource, const SRDWORD* indices, SRDWORD n)";
    command_names[COMMAND_MUL_INDEXED_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY_DWORD_ARRAY] =
        "_mulIndexed (srVector4* dest, const srVector4& constant, const "
        "srVector4* indexedSource,  const SRDWORD* indices, SRDWORD n)";
    command_names[COMMAND_MUL_MAT4_CONSTANT_MAT4_CONSTANT_MAT4_CONSTANT] =
        "_mul (srMatrix4& dst,\tconst\tsrMatrix4& ms0,\tconst srMatrix4& ms1)";
    command_names[COMMAND_MUL_MAT4_ARRAY_MAT4_ARRAY_MAT4_ARRAY] =
        "_mul (srMatrix4* dst, const srMatrix4* ms0, const srMatrix4* ms1, const SRDWORD n)";
    command_names[COMMAND_SR_TEST_BOUNDING_BOX] =
        "_srTestBoundingBox(const srMatrix4& m, const srVector3& min, const srVector3& max)";
    command_names[COMMAND_SR_SPECULAR_POW] =
        "_srSpecularPow\t(float* dest, const float* source, const float exponent, "
        "const SRDWORD count)";
    command_names[COMMAND_SR_COPY_INDEXED_REMAP] =
        "_srCopyIndexedRemap (srVector3i* dst, const srVector3i* src, const "
        "SRDWORD* ixTable, const SRDWORD* remap, const SRDWORD count)";
    command_names[COMMAND_SR_SET_INDEXED] =
        "_srSetIndexed (SRBYTE *dst, const srVector3i* src, const SRDWORD "
        "*ixTable, const SRDWORD count)";
    command_names[COMMAND_SR_COLLECT_POS] =
        "_srCollectPos (SRDWORD* dst, const float *src,  const SRDWORD count)";
    command_names[COMMAND_SR_COLLECT_NEG] =
        "_srCollectNeg (SRDWORD* dst, const float *src,  const SRDWORD count)";
    command_names[COMMAND_SR_COLLECT_NON_ZERO] =
        "_srCollectNonZero (SRDWORD* dst, const SRBYTE* src,  const SRDWORD count)";
    command_names[COMMAND_SR_REMAP_INVERSE] =
        "_srRemapInverse   (SRDWORD* dst, const SRDWORD* map, const SRDWORD n)";
    command_names[COMMAND_SR_DIRECT3_DCONVERT_COLOR] =
        "_srDirect3DConvertColor (SRDWORD* dst, const srVector4* src, const SRDWORD n)";
    command_names[COMMAND_TRANSFORM_INDEXED_VEC3_ARRAY_VEC3_ARRAY_DWORD_ARRAY_MAT4_CONSTANT] =
        "_transformIndexed (srVector3* dst, const srVector3* src, const SRDWORD* "
        "ixTable, const srMatrix4& m, const SRDWORD n)";
    command_names[COMMAND_TRANSFORM_INDEXED_VEC4_ARRAY_VEC3_ARRAY_DWORD_ARRAY_MAT4_CONSTANT] =
        "_transformIndexed (srVector4* dst, const srVector3* src, const SRDWORD* "
        "ixTable, const srMatrix4& m, const SRDWORD n)";
    command_names[COMMAND_DOT_INDEXED] =
        "_dotIndexed  (float* dest, const srVector4& cv, const srVector4* vs, "
        "const SRDWORD* ixTable, const SRDWORD n)";
    command_names[COMMAND_DOT_FLOAT_ARRAY_VEC4_CONSTANT_VEC3_ARRAY] =
        "_dot  (float* dest, const srVector4& cv, const srVector3* vs, const SRDWORD n)";
    command_names[COMMAND_COPY_VEC2_ARRAY_VEC2_CONSTANT] =
        "_copy  (srVector2* dest, const srVector2& cv, const SRDWORD n)";
    command_names[COMMAND_COPY_INDEXED_VEC2_ARRAY_VEC2_ARRAY_DWORD_ARRAY] =
        "_copyIndexed (srVector2* dest, const srVector2* src, const SRDWORD* indices, SRDWORD n)";
    command_names[COMMAND_DIV_VEC2_ARRAY_VEC2_ARRAY_FLOAT_ARRAY] =
        "_div  (srVector2* dest, const srVector2* vs, const float* fs, const SRDWORD n)";
    command_names[COMMAND_SR_CULL_NO_CLIP] =
        "_srCullNoClip (SRDWORD* dest, const srVector4& cv, const srVector4* vs, const SRDWORD n)";
    command_names[COMMAND_SR_FLOAT_TO_LINEAR] =
        "_srFloatToLinear (SRDWORD* d, const float* s, const SRDWORD n)";
    command_names[COMMAND_SR_LINEAR_TO_FLOAT] =
        "_srLinearToFloat (float* d, const SRDWORD* s, const SRDWORD n)";
    command_names[COMMAND_SR_GET_CLIP_FLAGS] =
        "_srGetClipFlags\t(SRBYTE* d, const srVector4* s, const SRDWORD n)";
}

// FUNCTION: SURRENDER 0x1006A290
void srDebugVP::resetInternalStatistics()
{
    int command;

    for (command = 0; command < COMMAND_COUNT; ++command) {
        call_times[command] = 0.0;
        call_counts[command] = 0;
        element_counts[command] = 0.0;
        misaligned8[command] = 0;
        misaligned16[command] = 0;
    }
}

// FUNCTION: SURRENDER 0x1006A2D0
srDebugVP::ScopeTimer::ScopeTimer(srDebugVP* owner, SRDWORD elements, e_command index,
                                  const void* pointer_0, const void* pointer_1,
                                  const void* pointer_2, const void* pointer_3)
{
    this->elements = elements;
    this->owner = owner;
    this->index = index;
    if (owner->check_misalignments != 0) {
        /* reinterpret-ok: the forwarded pointer arguments are OR-ed together
           so a single mask reports any address that is not 8- or 16-byte
           aligned. */
        unsigned long mask = reinterpret_cast<unsigned long>(pointer_0) |
                             reinterpret_cast<unsigned long>(pointer_1) |
                             reinterpret_cast<unsigned long>(pointer_2) |
                             reinterpret_cast<unsigned long>(pointer_3);
        if ((mask & 7) != 0) {
            ++owner->misaligned8[index];
        }
        if ((mask & 0xf) != 0) {
            ++owner->misaligned16[index];
        }
    }
    start_time = srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT);
}

// FUNCTION: SURRENDER 0x1006A340
srDebugVP::ScopeTimer::~ScopeTimer()
{
    owner->call_times[index] +=
        srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT) - start_time;
    ++owner->call_counts[index];
    owner->element_counts[index] += elements;
}

// FUNCTION: SURRENDER 0x1006A3C0
const char* srDebugVP::getName()
{
    return "srDebugVP";
}

// FUNCTION: SURRENDER 0x1006A3D0
int srDebugVP::_memcmp(const void* source_0, const void* source_1, SRDWORD bytes)
{
    ScopeTimer scope_timer(this, bytes, COMMAND_MEMCMP, source_0, source_1, 0, 0);
    return processor->_memcmp(source_0, source_1, bytes);
}

// FUNCTION: SURRENDER 0x1006A450
void srDebugVP::_memcopy(void* destination, const void* source, SRDWORD bytes)
{
    ScopeTimer scope_timer(this, bytes, COMMAND_MEMCOPY_VOID_ARRAY_VOID_ARRAY, destination, source,
                           0, 0);
    processor->_memcopy(destination, source, bytes);
}

// FUNCTION: SURRENDER 0x1006A4D0
void srDebugVP::_memcopy(void* destination, int source, SRDWORD bytes)
{
    ScopeTimer scope_timer(this, bytes, COMMAND_MEMCOPY_VOID_ARRAY_BYTE, destination, 0, 0, 0);
    processor->_memcopy(destination, source, bytes);
}

// FUNCTION: SURRENDER 0x1006A550
void srDebugVP::_prefetch(const void* destination, SRDWORD bytes, SRDWORD value_014)
{
    ScopeTimer scope_timer(this, bytes >> 5, COMMAND_PREFETCH, destination, 0, 0, 0);
    processor->_prefetch(destination, bytes, value_014);
}

// FUNCTION: SURRENDER 0x1006A5D0
void srDebugVP::_copyInterleaved(void* destination, const void* source, SRDWORD destination_pitch,
                                 SRDWORD source_pitch, SRDWORD width, SRDWORD count)
{
    ScopeTimer scope_timer(this, width * count, COMMAND_COPY_INTERLEAVED, destination, source, 0,
                           0);
    processor->_copyInterleaved(destination, source, destination_pitch, source_pitch, width, count);
}

// FUNCTION: SURRENDER 0x1006A660
void srDebugVP::_swap(void* first, void* second, SRDWORD bytes)
{
    ScopeTimer scope_timer(this, bytes, COMMAND_SWAP, first, second, 0, 0);
    processor->_swap(first, second, bytes);
}

// FUNCTION: SURRENDER 0x1006A6E0
void srDebugVP::_copy(SRDWORD* destination, SRDWORD constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_DWORD_ARRAY_DWORD, destination, 0, 0, 0);
    processor->_copy(destination, constant, count);
}

// FUNCTION: SURRENDER 0x1006A760
void srDebugVP::_reverse(SRDWORD* destination, const SRDWORD* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_REVERSE, destination, source, 0, 0);
    processor->_reverse(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006A7E0
void srDebugVP::_and(SRDWORD* destination, const SRDWORD* source, SRDWORD constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_AND_DWORD_ARRAY_DWORD_ARRAY_DWORD, destination,
                           source, 0, 0);
    processor->_and(destination, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006A860
void srDebugVP::_or(SRDWORD* destination, const SRDWORD* source, SRDWORD constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_OR_DWORD_ARRAY_DWORD_ARRAY_DWORD, destination,
                           source, 0, 0);
    processor->_or(destination, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006A8E0
void srDebugVP::_xor(SRDWORD* destination, const SRDWORD* source, SRDWORD constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_XOR_DWORD_ARRAY_DWORD_ARRAY_DWORD, destination,
                           source, 0, 0);
    processor->_xor(destination, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006A960
void srDebugVP::_and(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_AND_DWORD_ARRAY_DWORD_ARRAY_DWORD_ARRAY,
                           destination, source_0, source_1, 0);
    processor->_and(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006A9F0
void srDebugVP::_or(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                    SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_OR_DWORD_ARRAY_DWORD_ARRAY_DWORD_ARRAY, destination,
                           source_0, source_1, 0);
    processor->_or(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006AA80
void srDebugVP::_xor(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_XOR_DWORD_ARRAY_DWORD_ARRAY_DWORD_ARRAY,
                           destination, source_0, source_1, 0);
    processor->_xor(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006AB10
void srDebugVP::_asr(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ASR, destination, source, 0, 0);
    processor->_asr(destination, source, shift, count);
}

// FUNCTION: SURRENDER 0x1006AB90
void srDebugVP::_asrAnd(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD mask,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ASR_AND, destination, source, 0, 0);
    processor->_asrAnd(destination, source, shift, mask, count);
}

// FUNCTION: SURRENDER 0x1006AC20
void srDebugVP::_lsr(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_LSR, destination, source, 0, 0);
    processor->_lsr(destination, source, shift, count);
}

// FUNCTION: SURRENDER 0x1006ACA0
void srDebugVP::_lsl(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_LSL, destination, source, 0, 0);
    processor->_lsl(destination, source, shift, count);
}

// FUNCTION: SURRENDER 0x1006AD20
void srDebugVP::_lslAnd(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD mask,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_LSL_AND, destination, source, 0, 0);
    processor->_lslAnd(destination, source, shift, mask, count);
}

// FUNCTION: SURRENDER 0x1006ADB0
int srDebugVP::_isEqual(const SRDWORD* source, SRDWORD constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_IS_EQUAL_DWORD_ARRAY_DWORD, source, 0, 0, 0);
    return processor->_isEqual(source, constant, count);
}

// FUNCTION: SURRENDER 0x1006AE30
int srDebugVP::_isEqual(const SRDWORD* source_0, const SRDWORD* source_1, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_IS_EQUAL_DWORD_ARRAY_DWORD_ARRAY, source_0,
                           source_1, 0, 0);
    return processor->_isEqual(source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006AEB0
SRDWORD srDebugVP::_max(const SRDWORD* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MIN_DWORD_ARRAY, source, 0, 0, 0);
    return processor->_min(source, count);
}

// FUNCTION: SURRENDER 0x1006AF30
SRDWORD srDebugVP::_min(const SRDWORD* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MAX_DWORD_ARRAY, source, 0, 0, 0);
    return processor->_max(source, count);
}

// FUNCTION: SURRENDER 0x1006AFB0
void srDebugVP::_copyIndexed(SRDWORD* destination, const SRDWORD* source, const SRDWORD* indices,
                             SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_INDEXED_DWORD_ARRAY_DWORD_ARRAY_DWORD_ARRAY,
                           destination, source, 0, 0);
    processor->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006B040
void srDebugVP::_addS(SRBYTE* destination, const SRBYTE* source, SRBYTE constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ADD_S_BYTE_ARRAY_BYTE_ARRAY_BYTE, destination,
                           source, 0, 0);
    processor->_addS(destination, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006B0D0
void srDebugVP::_subS(SRBYTE* destination, const SRBYTE* source, SRBYTE constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_S_BYTE_ARRAY_BYTE_ARRAY_BYTE, destination,
                           source, 0, 0);
    processor->_subS(destination, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006B160
void srDebugVP::_subS(SRBYTE* destination, SRBYTE constant, const SRBYTE* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_S_BYTE_ARRAY_BYTE_BYTE_ARRAY, destination,
                           source, 0, 0);
    processor->_subS(destination, constant, source, count);
}

// FUNCTION: SURRENDER 0x1006B1F0
void srDebugVP::_addS(SRBYTE* destination, const SRBYTE* source_0, const SRBYTE* source_1,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ADD_S_BYTE_ARRAY_BYTE_ARRAY_BYTE_ARRAY, destination,
                           source_0, source_1, 0);
    processor->_addS(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B280
void srDebugVP::_subS(SRBYTE* destination, const SRBYTE* source_0, const SRBYTE* source_1,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_S_BYTE_ARRAY_BYTE_ARRAY_BYTE_ARRAY, destination,
                           source_0, source_1, 0);
    processor->_subS(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B310
void srDebugVP::_toFloat(float* destination, const SRBYTE* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_TO_FLOAT, destination, destination, source, 0);
    processor->_toFloat(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006B390
void srDebugVP::_add(float* destination, float constant, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ADD_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY, destination,
                           source, 0, 0);
    processor->_add(destination, constant, source, count);
}

// FUNCTION: SURRENDER 0x1006B420
void srDebugVP::_sub(float* destination, float constant, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY, destination,
                           source, 0, 0);
    processor->_sub(destination, constant, source, count);
}

// FUNCTION: SURRENDER 0x1006B4B0
void srDebugVP::_mul(float* destination, float constant, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MUL_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY, destination,
                           source, 0, 0);
    processor->_mul(destination, constant, source, count);
}

// FUNCTION: SURRENDER 0x1006B540
void srDebugVP::_div(float* destination, float constant, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIV_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY, destination,
                           source, 0, 0);
    processor->_div(destination, constant, source, count);
}

// FUNCTION: SURRENDER 0x1006B5D0
void srDebugVP::_add(float* destination, const float* source_0, const float* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ADD_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY,
                           destination, source_0, source_1, 0);
    processor->_add(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B660
void srDebugVP::_sub(float* destination, const float* source_0, const float* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY,
                           destination, source_0, source_1, 0);
    processor->_sub(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B6F0
void srDebugVP::_mul(float* destination, const float* source_0, const float* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MUL_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY,
                           destination, source_0, source_1, 0);
    processor->_mul(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B780
void srDebugVP::_div(float* destination, const float* source_0, const float* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIV_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY,
                           destination, source_0, source_1, 0);
    processor->_div(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B810
void srDebugVP::_mul(float* destination, float constant, const float* source_0,
                     const float* source_1, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MUL_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY_FLOAT_ARRAY,
                           destination, source_0, source_1, 0);
    processor->_mul(destination, constant, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B8A0
void srDebugVP::_clamp(float* destination, const float* source, float minimum, float maximum,
                       SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_CLAMP, destination, source, 0, 0);
    processor->_clamp(destination, source, minimum, maximum, count);
}

// FUNCTION: SURRENDER 0x1006B930
void srDebugVP::_clampMin(float* destination, const float* source, float minimum, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_CLAMP_MIN, destination, source, 0, 0);
    processor->_clampMin(destination, source, minimum, count);
}

// FUNCTION: SURRENDER 0x1006B9C0
void srDebugVP::_clampMax(float* destination, const float* source, float maximum, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_CLAMP_MAX, destination, source, 0, 0);
    processor->_clampMax(destination, source, maximum, count);
}

// FUNCTION: SURRENDER 0x1006BA50
void srDebugVP::_clampUnit(float* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_CLAMP_UNIT, destination, source, 0, 0);
    processor->_clampUnit(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006BAD0
void srDebugVP::_sqrt(float* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SQRT, destination, source, 0, 0);
    processor->_sqrt(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006BB50
void srDebugVP::_isqrt(float* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ISQRT, destination, source, 0, 0);
    processor->_isqrt(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006BBD0
void srDebugVP::_lerp(float* destination, const float* target, const float* source, float constant,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_LERP, destination, target, source, 0);
    processor->_lerp(destination, target, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006BC60
int srDebugVP::_isNeg(const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_IS_NEG, source, 0, 0, 0);
    return processor->_isNeg(source, count);
}

// FUNCTION: SURRENDER 0x1006BCE0
int srDebugVP::_isPos(const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_IS_POS, source, 0, 0, 0);
    return processor->_isPos(source, count);
}

// FUNCTION: SURRENDER 0x1006BD60
int srDebugVP::_isZero(const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_IS_ZERO, source, 0, 0, 0);
    return processor->_isZero(source, count);
}

// FUNCTION: SURRENDER 0x1006BDE0
float srDebugVP::_min(const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MIN_FLOAT_ARRAY, source, 0, 0, 0);
    return processor->_min(source, count);
}

// FUNCTION: SURRENDER 0x1006BE60
float srDebugVP::_max(const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MAX_FLOAT_ARRAY, source, 0, 0, 0);
    return processor->_max(source, count);
}

// FUNCTION: SURRENDER 0x1006BEE0
void srDebugVP::_minMax(const float* source, float& minimum, float& maximum, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MIN_MAX_FLOAT_ARRAY_FLOAT_CONSTANT_FLOAT_CONSTANT,
                           source, 0, 0, 0);
    processor->_minMax(source, minimum, maximum, count);
}

// FUNCTION: SURRENDER 0x1006BF60
double srDebugVP::_sum(const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUM, source, 0, 0, 0);
    return processor->_sum(source, count);
}

// FUNCTION: SURRENDER 0x1006BFE0
void srDebugVP::_axpy(float* destination, float add_constant, float multiply_constant,
                      const float* multiply_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_AXPY_FLOAT_ARRAY_FLOAT_FLOAT_FLOAT_ARRAY,
                           destination, multiply_source, 0, 0);
    processor->_axpy(destination, add_constant, multiply_constant, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006C070
void srDebugVP::_axpy(float* destination, float add_constant, const float* scale_source,
                      const float* multiply_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_AXPY_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY_FLOAT_ARRAY,
                           destination, scale_source, multiply_source, 0);
    processor->_axpy(destination, add_constant, scale_source, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006C100
void srDebugVP::_axpy(float* destination, const float* add_source, float multiply_constant,
                      const float* multiply_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_AXPY_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY,
                           destination, add_source, multiply_source, 0);
    processor->_axpy(destination, add_source, multiply_constant, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006C190
void srDebugVP::_axpy(float* destination, const float* add_source, const float* scale_source,
                      const float* multiply_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_AXPY_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY,
                           destination, add_source, scale_source, multiply_source);
    processor->_axpy(destination, add_source, scale_source, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006C220
void srDebugVP::_axpy(float* destination, float add_constant, float scale,
                      const float* scale_source, const float* multiply_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_AXPY_FLOAT_ARRAY_FLOAT_FLOAT_FLOAT_ARRAY_FLOAT_ARRAY,
                           destination, scale_source, multiply_source, 0);
    processor->_axpy(destination, add_constant, scale, scale_source, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006C2B0
void srDebugVP::_axpy(float* destination, const float* add_source, float scale,
                      const float* scale_source, const float* multiply_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_AXPY_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY_FLOAT_ARRAY,
                           destination, add_source, scale_source, multiply_source);
    processor->_axpy(destination, add_source, scale, scale_source, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006C350
void srDebugVP::_mulIndexed(float* destination, const float* linear_source,
                            const float* indexed_source, const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_MUL_INDEXED_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY_DWORD_ARRAY,
                           destination, linear_source, indexed_source, indices);
    processor->_mulIndexed(destination, linear_source, indexed_source, indices, count);
}

// FUNCTION: SURRENDER 0x1006C3E0
void srDebugVP::_mulIndexed(float* destination, float constant, const float* indexed_source,
                            const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_MUL_INDEXED_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY_DWORD_ARRAY,
                           destination, indexed_source, indices, 0);
    processor->_mulIndexed(destination, constant, indexed_source, indices, count);
}

// FUNCTION: SURRENDER 0x1006C470
void srDebugVP::_toInt(SRLONG* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_TO_INT, destination, source, 0, 0);
    processor->_toInt(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006C4F0
void srDebugVP::_invPoly(float* destination, const float* source, const srVector3& poly,
                         SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_INV_POLY, destination, source, 0, 0);
    processor->_invPoly(destination, source, poly, count);
}

// FUNCTION: SURRENDER 0x1006C580
void srDebugVP::_abs(float* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ABS, destination, source, 0, 0);
    processor->_abs(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006C600
void srDebugVP::_neg(float* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_NEG, destination, source, 0, 0);
    processor->_neg(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006C680
void srDebugVP::_cubic(float* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_CUBIC, destination, source, 0, 0);
    processor->_cubic(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006C700
void srDebugVP::_copy(srVector2* destination, const srVector2& constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_VEC2_ARRAY_VEC2_CONSTANT, destination, 0, 0,
                           0);
    processor->_copy(destination, constant, count);
}

// FUNCTION: SURRENDER 0x1006C780
void srDebugVP::_copyIndexed(srVector2* destination, const srVector2* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_INDEXED_VEC2_ARRAY_VEC2_ARRAY_DWORD_ARRAY,
                           destination, source, 0, 0);
    processor->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006C810
void srDebugVP::_div(srVector2* destination, const srVector2* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIV_VEC2_ARRAY_VEC2_ARRAY_FLOAT_ARRAY, destination,
                           vector_source, float_source, 0);
    processor->_div(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006C8A0
void srDebugVP::_copy(srVector3* destination, const srVector3& constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_VEC3_ARRAY_VEC3_CONSTANT, destination, 0, 0,
                           0);
    processor->_copy(destination, constant, count);
}

// FUNCTION: SURRENDER 0x1006C920
void srDebugVP::_copy(srVector3* destination, const srVector4* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_VEC3_ARRAY_VEC4_ARRAY, destination, source, 0,
                           0);
    processor->_copy(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006C9A0
void srDebugVP::_add(srVector3* destination, const srVector3& constant,
                     const srVector3* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ADD_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY,
                           destination, vector_source, 0, 0);
    processor->_add(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006CA30
void srDebugVP::_sub(srVector3* destination, const srVector3& constant,
                     const srVector3* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY,
                           destination, vector_source, 0, 0);
    processor->_sub(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006CAC0
void srDebugVP::_mul(srVector3* destination, const srVector3& constant,
                     const srVector3* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MUL_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY,
                           destination, vector_source, 0, 0);
    processor->_mul(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006CB50
void srDebugVP::_div(srVector3* destination, const srVector3& constant,
                     const srVector3* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIV_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY,
                           destination, vector_source, 0, 0);
    processor->_div(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006CBE0
void srDebugVP::_add(srVector3* destination, const srVector3& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ADD_VEC3_ARRAY_VEC3_CONSTANT_FLOAT_ARRAY,
                           destination, float_source, 0, 0);
    processor->_add(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CC70
void srDebugVP::_sub(srVector3* destination, const srVector3& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_VEC3_ARRAY_VEC3_CONSTANT_FLOAT_ARRAY,
                           destination, float_source, 0, 0);
    processor->_sub(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CD00
void srDebugVP::_mul(srVector3* destination, const srVector3& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MUL_VEC3_ARRAY_VEC3_CONSTANT_FLOAT_ARRAY,
                           destination, float_source, 0, 0);
    processor->_mul(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CD90
void srDebugVP::_div(srVector3* destination, const srVector3& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIV_VEC3_ARRAY_VEC3_CONSTANT_FLOAT_ARRAY,
                           destination, float_source, 0, 0);
    processor->_div(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CE20
void srDebugVP::_add(srVector3* destination, const srVector3* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ADD_VEC3_ARRAY_VEC3_ARRAY_FLOAT_ARRAY, destination,
                           vector_source, float_source, 0);
    processor->_add(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CEB0
void srDebugVP::_sub(srVector3* destination, const srVector3* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_VEC3_ARRAY_VEC3_ARRAY_FLOAT_ARRAY, destination,
                           vector_source, float_source, 0);
    processor->_sub(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CF40
void srDebugVP::_mul(srVector3* destination, const srVector3* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MUL_VEC3_ARRAY_VEC3_ARRAY_FLOAT_ARRAY, destination,
                           vector_source, float_source, 0);
    processor->_mul(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CFD0
void srDebugVP::_div(srVector3* destination, const srVector3* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIV_VEC3_ARRAY_VEC3_ARRAY_FLOAT_ARRAY, destination,
                           vector_source, float_source, 0);
    processor->_div(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006D060
void srDebugVP::_sub(srVector3* destination, const float* float_source,
                     const srVector3* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_VEC3_ARRAY_FLOAT_ARRAY_VEC3_ARRAY, destination,
                           float_source, vector_source, 0);
    processor->_sub(destination, float_source, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006D0F0
void srDebugVP::_div(srVector3* destination, const float* float_source,
                     const srVector3* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIV_VEC3_ARRAY_FLOAT_ARRAY_VEC3_ARRAY, destination,
                           float_source, vector_source, 0);
    processor->_div(destination, float_source, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006D180
void srDebugVP::_dot(float* destination, const srVector3& constant, const srVector3* vectors,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DOT_FLOAT_ARRAY_VEC3_CONSTANT_VEC3_ARRAY,
                           destination, vectors, 0, 0);
    processor->_dot(destination, constant, vectors, count);
}

// FUNCTION: SURRENDER 0x1006D210
void srDebugVP::_dot(float* destination, const srVector3* vectors_0, const srVector3* vectors_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DOT_FLOAT_ARRAY_VEC3_ARRAY_VEC3_ARRAY, destination,
                           vectors_0, vectors_1, 0);
    processor->_dot(destination, vectors_0, vectors_1, count);
}

// FUNCTION: SURRENDER 0x1006D2A0
void srDebugVP::_cross(srVector3* destination, const srVector3* vectors_0,
                       const srVector3* vectors_1, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_CROSS, destination, vectors_0, vectors_1, 0);
    processor->_cross(destination, vectors_0, vectors_1, count);
}

// FUNCTION: SURRENDER 0x1006D330
void srDebugVP::_length(float* destination, const srVector3* vectors, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_LENGTH_FLOAT_ARRAY_VEC3_ARRAY, destination, vectors,
                           0, 0);
    processor->_length(destination, vectors, count);
}

// FUNCTION: SURRENDER 0x1006D3B0
void srDebugVP::_normalize(srVector3* destination, const srVector3* vectors, float length,
                           SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_NORMALIZE_VEC3_ARRAY_VEC3_ARRAY_FLOAT, destination,
                           vectors, 0, 0);
    processor->_normalize(destination, vectors, length, count);
}

// FUNCTION: SURRENDER 0x1006D440
void srDebugVP::_minMax(const srVector3* source, srVector3& minimum, srVector3& maximum,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MIN_MAX_VEC3_ARRAY_VEC3_CONSTANT_VEC3_CONSTANT,
                           source, 0, 0, 0);
    processor->_minMax(source, minimum, maximum, count);
}

// FUNCTION: SURRENDER 0x1006D4C0
void srDebugVP::_transform(srVector3* destination, const srVector3* vectors,
                           const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_TRANSFORM_VEC3_ARRAY_VEC3_ARRAY_MAT4_CONSTANT,
                           destination, vectors, 0, 0);
    processor->_transform(destination, vectors, matrix, count);
}

// FUNCTION: SURRENDER 0x1006D550
void srDebugVP::_copyIndexed(srVector3* destination, const srVector2* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_INDEXED_VEC3_ARRAY_VEC2_ARRAY_DWORD_ARRAY,
                           destination, source, indices, 0);
    processor->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006D5E0
void srDebugVP::_copyIndexed(srVector3* destination, const srVector3* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_INDEXED_VEC3_ARRAY_VEC3_ARRAY_DWORD_ARRAY,
                           destination, source, indices, 0);
    processor->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006D670
void srDebugVP::_copyIndexed(srVector3* destination, const srVector4* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_INDEXED_VEC3_ARRAY_VEC4_ARRAY_DWORD_ARRAY,
                           destination, source, indices, 0);
    processor->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006D700
void srDebugVP::_mulIndexed(srVector3* destination, const srVector3* linear_source,
                            const srVector3* indexed_source, const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_MUL_INDEXED_VEC3_ARRAY_VEC3_ARRAY_VEC3_ARRAY_DWORD_ARRAY,
                           destination, linear_source, indexed_source, indices);
    processor->_mulIndexed(destination, linear_source, indexed_source, indices, count);
}

// FUNCTION: SURRENDER 0x1006D790
void srDebugVP::_mulIndexed(srVector3* destination, const srVector3& constant,
                            const srVector3* indexed_source, const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_MUL_INDEXED_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY_DWORD_ARRAY,
                           destination, indexed_source, indices, 0);
    processor->_mulIndexed(destination, constant, indexed_source, indices, count);
}

// FUNCTION: SURRENDER 0x1006D820
void srDebugVP::_dir(srVector3* destination, float* lengths, const srVector3* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIR_VEC3_ARRAY_FLOAT_ARRAY_VEC3_ARRAY, destination,
                           lengths, source, 0);
    processor->_dir(destination, lengths, source, count);
}

// FUNCTION: SURRENDER 0x1006D8B0
void srDebugVP::_dir(srVector3* destination, float* lengths, const srVector4* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIR_VEC3_ARRAY_FLOAT_ARRAY_VEC4_ARRAY, destination,
                           lengths, source, 0);
    processor->_dir(destination, lengths, source, count);
}

// FUNCTION: SURRENDER 0x1006D940
void srDebugVP::_copy(srVector4* destination, const srVector4& constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_VEC4_ARRAY_VEC4_CONSTANT, destination, 0, 0,
                           0);
    processor->_copy(destination, constant, count);
}

// FUNCTION: SURRENDER 0x1006D9C0
void srDebugVP::_copy(srVector4* destination, const srVector3* source, float constant,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_VEC4_ARRAY_VEC3_ARRAY_FLOAT, destination,
                           source, 0, 0);
    processor->_copy(destination, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006DA40
void srDebugVP::_copy(srVector4* destination, const srVector3* source_0, const float* source_1,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_VEC4_ARRAY_VEC3_ARRAY_FLOAT_ARRAY, destination,
                           source_0, source_1, 0);
    processor->_copy(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006DAD0
void srDebugVP::_copyW(srVector4* destination, float constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_W_VEC4_ARRAY_FLOAT, destination, 0, 0, 0);
    processor->_copyW(destination, constant, count);
}

// FUNCTION: SURRENDER 0x1006DB50
void srDebugVP::_copyW(srVector4* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_W_VEC4_ARRAY_FLOAT_ARRAY, destination, source,
                           0, 0);
    processor->_copyW(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006DBD0
void srDebugVP::_copyW(float* destination, const srVector4* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_W_FLOAT_ARRAY_VEC4_ARRAY, destination, source,
                           0, 0);
    processor->_copyW(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006DC50
void srDebugVP::_add(srVector4* destination, const srVector4& constant,
                     const srVector4* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ADD_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY,
                           destination, vector_source, 0, 0);
    processor->_add(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006DCE0
void srDebugVP::_sub(srVector4* destination, const srVector4& constant,
                     const srVector4* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY,
                           destination, vector_source, 0, 0);
    processor->_sub(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006DD70
void srDebugVP::_mul(srVector4* destination, const srVector4& constant,
                     const srVector4* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MUL_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY,
                           destination, vector_source, 0, 0);
    processor->_mul(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006DE00
void srDebugVP::_div(srVector4* destination, const srVector4& constant,
                     const srVector4* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIV_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY,
                           destination, vector_source, 0, 0);
    processor->_div(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006DE90
void srDebugVP::_add(srVector4* destination, const srVector4& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ADD_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY,
                           destination, float_source, 0, 0);
    processor->_add(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006DF20
void srDebugVP::_sub(srVector4* destination, const srVector4& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY,
                           destination, float_source, 0, 0);
    processor->_sub(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006DFB0
void srDebugVP::_mul(srVector4* destination, const srVector4& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MUL_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY,
                           destination, float_source, 0, 0);
    processor->_mul(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006E040
void srDebugVP::_div(srVector4* destination, const srVector4& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIV_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY,
                           destination, float_source, 0, 0);
    processor->_div(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006E0D0
void srDebugVP::_add(srVector4* destination, const srVector4* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_ADD_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY, destination,
                           vector_source, float_source, 0);
    processor->_add(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006E160
void srDebugVP::_sub(srVector4* destination, const srVector4* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY, destination,
                           vector_source, float_source, 0);
    processor->_sub(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006E1F0
void srDebugVP::_mul(srVector4* destination, const srVector4* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MUL_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY, destination,
                           vector_source, float_source, 0);
    processor->_mul(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006E280
void srDebugVP::_div(srVector4* destination, const srVector4* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIV_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY, destination,
                           vector_source, float_source, 0);
    processor->_div(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006E310
void srDebugVP::_sub(srVector4* destination, const float* float_source,
                     const srVector4* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SUB_VEC4_ARRAY_FLOAT_ARRAY_VEC4_ARRAY, destination,
                           float_source, vector_source, 0);
    processor->_sub(destination, float_source, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006E3A0
void srDebugVP::_div(srVector4* destination, const float* float_source,
                     const srVector4* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIV_VEC4_ARRAY_FLOAT_ARRAY_VEC4_ARRAY, destination,
                           float_source, vector_source, 0);
    processor->_div(destination, float_source, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006E430
void srDebugVP::_dot(float* destination, const srVector4& constant, const srVector4* vectors,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DOT_FLOAT_ARRAY_VEC4_CONSTANT_VEC4_ARRAY, vectors,
                           0, 0, 0);
    processor->_dot(destination, constant, vectors, count);
}

// FUNCTION: SURRENDER 0x1006E4B0
void srDebugVP::_dot(float* destination, const srVector4* vectors_0, const srVector4* vectors_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DOT_FLOAT_ARRAY_VEC4_ARRAY_VEC4_ARRAY, vectors_0,
                           vectors_1, 0, 0);
    processor->_dot(destination, vectors_0, vectors_1, count);
}

// FUNCTION: SURRENDER 0x1006E540
void srDebugVP::_length(float* destination, const srVector4* vectors, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_LENGTH_FLOAT_ARRAY_VEC4_ARRAY, destination, vectors,
                           0, 0);
    processor->_length(destination, vectors, count);
}

// FUNCTION: SURRENDER 0x1006E5C0
void srDebugVP::_normalize(srVector4* destination, const srVector4* vectors, float length,
                           SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_NORMALIZE_VEC4_ARRAY_VEC4_ARRAY_FLOAT, destination,
                           vectors, 0, 0);
    processor->_normalize(destination, vectors, length, count);
}

// FUNCTION: SURRENDER 0x1006E650
void srDebugVP::_minMax(const srVector4* source, srVector4& minimum, srVector4& maximum,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MIN_MAX_VEC4_ARRAY_VEC4_CONSTANT_VEC4_CONSTANT,
                           source, 0, 0, 0);
    processor->_minMax(source, minimum, maximum, count);
}

// FUNCTION: SURRENDER 0x1006E6D0
void srDebugVP::_transform(srVector4* destination, const srVector4* vectors,
                           const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_TRANSFORM_VEC4_ARRAY_VEC4_ARRAY_MAT4_CONSTANT,
                           destination, vectors, 0, 0);
    processor->_transform(destination, vectors, matrix, count);
}

// FUNCTION: SURRENDER 0x1006E760
void srDebugVP::_transform(srVector4* destination, const srVector3* vectors,
                           const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_TRANSFORM_VEC4_ARRAY_VEC3_ARRAY_MAT4_CONSTANT,
                           destination, vectors, 0, 0);
    processor->_transform(destination, vectors, matrix, count);
}

// FUNCTION: SURRENDER 0x1006E7F0
void srDebugVP::_transformOrtho(srVector4* destination, const srVector4* source,
                                const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_TRANSFORM_ORTHO, destination, source, 0, 0);
    processor->_transformOrtho(destination, source, matrix, count);
}

// FUNCTION: SURRENDER 0x1006E880
void srDebugVP::_transformPerspective(srVector4* destination, const srVector4* source,
                                      const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_TRANSFORM_PERSPECTIVE, destination, source, 0, 0);
    processor->_transformPerspective(destination, source, matrix, count);
}

// FUNCTION: SURRENDER 0x1006E910
void srDebugVP::_axpy(srVector4* destination, const srVector4& add_constant,
                      const srVector4& multiply_constant, const float* multiply_source,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_AXPY_VEC4_ARRAY_VEC4_CONSTANT_VEC4_CONSTANT_FLOAT_ARRAY,
                           destination, multiply_source, 0, 0);
    processor->_axpy(destination, add_constant, multiply_constant, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006E9A0
void srDebugVP::_axpy(srVector4* destination, const srVector4& add_constant,
                      const srVector4* multiply_vectors, const float* multiply_source,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_AXPY_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY_FLOAT_ARRAY,
                           destination, multiply_vectors, multiply_source, 0);
    processor->_axpy(destination, add_constant, multiply_vectors, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006EA30
void srDebugVP::_axpy(srVector4* destination, const srVector4* add_source,
                      const srVector4& multiply_constant, const float* multiply_source,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_AXPY_VEC4_ARRAY_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY,
                           destination, add_source, multiply_source, 0);
    processor->_axpy(destination, add_source, multiply_constant, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006EAC0
void srDebugVP::_axpy(srVector4* destination, const srVector4* add_source,
                      const srVector4* multiply_vectors, const float* multiply_source,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_AXPY_VEC4_ARRAY_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY,
                           destination, add_source, multiply_vectors, multiply_source);
    processor->_axpy(destination, add_source, multiply_vectors, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006EB50
void srDebugVP::_axpy(srVector4* destination, const srVector4& add_constant,
                      const srVector4& multiply_constant, const float* multiply_source_0,
                      const float* multiply_source_1, SRDWORD count)
{
    ScopeTimer scope_timer(
        this, count, COMMAND_AXPY_VEC4_ARRAY_VEC4_CONSTANT_VEC4_CONSTANT_FLOAT_ARRAY_FLOAT_ARRAY,
        destination, multiply_source_0, multiply_source_1, 0);
    processor->_axpy(destination, add_constant, multiply_constant, multiply_source_0,
                     multiply_source_1, count);
}

// FUNCTION: SURRENDER 0x1006EBF0
void srDebugVP::_axpy(srVector4* destination, const srVector4* add_source,
                      const srVector4& multiply_constant, const float* multiply_source_0,
                      const float* multiply_source_1, SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_AXPY_VEC4_ARRAY_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY_FLOAT_ARRAY,
                           destination, add_source, multiply_source_0, multiply_source_1);
    processor->_axpy(destination, add_source, multiply_constant, multiply_source_0,
                     multiply_source_1, count);
}

// FUNCTION: SURRENDER 0x1006EC90
void srDebugVP::_mulAdd(srVector4* destination, const srVector4& add_constant,
                        const srVector4& multiply_constant, const srVector4* multiply_source,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_MUL_ADD_VEC4_ARRAY_VEC4_CONSTANT_VEC4_CONSTANT_VEC4_ARRAY,
                           destination, multiply_source, 0, 0);
    processor->_mulAdd(destination, add_constant, multiply_constant, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006ED20
void srDebugVP::_mulAdd(srVector4* destination, const srVector4* add_source,
                        const srVector4& multiply_constant, const srVector4* multiply_source,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_MUL_ADD_VEC4_ARRAY_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY,
                           destination, add_source, multiply_source, 0);
    processor->_mulAdd(destination, add_source, multiply_constant, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006EDB0
void srDebugVP::_mulAdd(srVector4* destination, const srVector4& add_constant,
                        const srVector4* multiply_source_0, const srVector4* multiply_source_1,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_MUL_ADD_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY_VEC4_ARRAY,
                           destination, multiply_source_0, multiply_source_1, 0);
    processor->_mulAdd(destination, add_constant, multiply_source_0, multiply_source_1, count);
}

// FUNCTION: SURRENDER 0x1006EE40
void srDebugVP::_divByW(srVector4* destination, const srVector4* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DIV_BY_W, destination, source, 0, 0);
    processor->_divByW(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006EEC0
void srDebugVP::_copyIndexed(srVector4* destination, const srVector2* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_INDEXED_VEC4_ARRAY_VEC2_ARRAY_DWORD_ARRAY,
                           destination, source, indices, 0);
    processor->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006EF50
void srDebugVP::_copyIndexed(srVector4* destination, const srVector3* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_INDEXED_VEC4_ARRAY_VEC3_ARRAY_DWORD_ARRAY,
                           destination, source, indices, 0);
    processor->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006EFE0
void srDebugVP::_copyIndexed(srVector4* destination, const srVector4* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_INDEXED_VEC4_ARRAY_VEC4_ARRAY_DWORD_ARRAY,
                           destination, source, indices, 0);
    processor->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006F070
void srDebugVP::_copyIndexed(srVector4* destination, const srARGB* source, const SRDWORD* indices,
                             SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_COPY_INDEXED_VEC4_ARRAY_ARGB_ARRAY_DWORD_ARRAY,
                           destination, source, indices, 0);
    processor->_copyIndexed(destination, source, indices, count);
}

/* Retail swaps these overloads' statistics indices. The linear form tracks
   all four pointer arguments; the constant form omits constant alignment. */
// FUNCTION: SURRENDER 0x1006F190
void srDebugVP::_mulIndexed(srVector4* destination, const srVector4* linear_source,
                            const srVector4* indexed_source, const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_MUL_INDEXED_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY_DWORD_ARRAY,
                           destination, linear_source, indexed_source, indices);
    processor->_mulIndexed(destination, linear_source, indexed_source, indices, count);
}

// FUNCTION: SURRENDER 0x1006F100
void srDebugVP::_mulIndexed(srVector4* destination, const srVector4& constant,
                            const srVector4* indexed_source, const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count,
                           COMMAND_MUL_INDEXED_VEC4_ARRAY_VEC4_ARRAY_VEC4_ARRAY_DWORD_ARRAY,
                           destination, indexed_source, indices, 0);
    processor->_mulIndexed(destination, constant, indexed_source, indices, count);
}

// FUNCTION: SURRENDER 0x1006F220
void srDebugVP::_mul(srMatrix4& destination, const srMatrix4& source_0, const srMatrix4& source_1)
{
    ScopeTimer scope_timer(this, 1, COMMAND_MUL_MAT4_CONSTANT_MAT4_CONSTANT_MAT4_CONSTANT, 0, 0, 0,
                           0);
    processor->_mul(destination, source_0, source_1);
}

// FUNCTION: SURRENDER 0x1006F2A0
void srDebugVP::_mul(srMatrix4* destination, const srMatrix4* source_0, const srMatrix4* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_MUL_MAT4_ARRAY_MAT4_ARRAY_MAT4_ARRAY, destination,
                           source_0, source_1, 0);
    processor->_mul(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006F330
int srDebugVP::_srTestBoundingBox(const srMatrix4& matrix, const srVector3& minimum,
                                  const srVector3& maximum)
{
    ScopeTimer scope_timer(this, 1, COMMAND_SR_TEST_BOUNDING_BOX, 0, 0, 0, 0);
    return processor->_srTestBoundingBox(matrix, minimum, maximum);
}

// FUNCTION: SURRENDER 0x1006F3B0
void srDebugVP::_srSpecularPow(float* destination, const float* source, float exponent,
                               SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SR_SPECULAR_POW, destination, source, 0, 0);
    processor->_srSpecularPow(destination, source, exponent, count);
}

// FUNCTION: SURRENDER 0x1006F440
void srDebugVP::_srCopyIndexedRemap(srVector3i* destination, const srVector3i* source,
                                    const SRDWORD* indices, const SRDWORD* remap, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SR_COPY_INDEXED_REMAP, 0, 0, 0, 0);
    processor->_srCopyIndexedRemap(destination, source, indices, remap, count);
}

// FUNCTION: SURRENDER 0x1006F4D0
void srDebugVP::_srSetIndexed(SRBYTE* destination, const srVector3i* source, const SRDWORD* indices,
                              SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SR_SET_INDEXED, destination, 0, 0, 0);
    processor->_srSetIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006F560
SRDWORD srDebugVP::_srCollectPos(SRDWORD* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SR_COLLECT_POS, destination, source, 0, 0);
    return processor->_srCollectPos(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006F5F0
SRDWORD srDebugVP::_srCollectNeg(SRDWORD* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SR_COLLECT_NEG, destination, source, 0, 0);
    return processor->_srCollectNeg(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006F680
SRDWORD srDebugVP::_srCollectNonZero(SRDWORD* destination, const SRBYTE* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SR_COLLECT_NON_ZERO, destination, source, 0, 0);
    return processor->_srCollectNonZero(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006F710
void srDebugVP::_srRemapInverse(SRDWORD* destination, const SRDWORD* map, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SR_REMAP_INVERSE, destination, 0, 0, 0);
    processor->_srRemapInverse(destination, map, count);
}

// FUNCTION: SURRENDER 0x1006F790
void srDebugVP::_srDirect3DConvertColor(SRDWORD* destination, const srVector4* source,
                                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SR_DIRECT3_DCONVERT_COLOR, destination, source, 0,
                           0);
    processor->_srDirect3DConvertColor(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006F810
void srDebugVP::_transformIndexed(srVector4* destination, const srVector3* source,
                                  const SRDWORD* indices, const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(
        this, count, COMMAND_TRANSFORM_INDEXED_VEC4_ARRAY_VEC3_ARRAY_DWORD_ARRAY_MAT4_CONSTANT,
        source, 0, 0, 0);
    processor->_transformIndexed(destination, source, indices, matrix, count);
}

// FUNCTION: SURRENDER 0x1006F8A0
void srDebugVP::_transformIndexed(srVector3* destination, const srVector3* source,
                                  const SRDWORD* indices, const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(
        this, count, COMMAND_TRANSFORM_INDEXED_VEC3_ARRAY_VEC3_ARRAY_DWORD_ARRAY_MAT4_CONSTANT, 0,
        0, 0, 0);
    processor->_transformIndexed(destination, source, indices, matrix, count);
}

// FUNCTION: SURRENDER 0x1006F930
void srDebugVP::_dotIndexed(float* destination, const srVector4& constant, const srVector4* vectors,
                            const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DOT_INDEXED, vectors, 0, 0, 0);
    processor->_dotIndexed(destination, constant, vectors, indices, count);
}

// FUNCTION: SURRENDER 0x1006F9C0
void srDebugVP::_dot(float* destination, const srVector4& constant, const srVector3* vectors,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_DOT_FLOAT_ARRAY_VEC4_CONSTANT_VEC3_ARRAY,
                           destination, 0, 0, 0);
    processor->_dot(destination, constant, vectors, count);
}

// FUNCTION: SURRENDER 0x1006FA50
SRDWORD srDebugVP::_srCullNoClip(SRDWORD* destination, const srVector4& constant,
                                 const srVector4* vectors, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SR_CULL_NO_CLIP, vectors, 0, 0, 0);
    return processor->_srCullNoClip(destination, constant, vectors, count);
}

// FUNCTION: SURRENDER 0x1006FAE0
void srDebugVP::_srFloatToLinear(SRDWORD* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SR_FLOAT_TO_LINEAR, destination, source, 0, 0);
    processor->_srFloatToLinear(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006FB60
void srDebugVP::_srLinearToFloat(float* destination, const SRDWORD* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SR_LINEAR_TO_FLOAT, destination, source, 0, 0);
    processor->_srLinearToFloat(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006FBE0
void srDebugVP::unknown_2a0(SRDWORD arg0, SRDWORD arg1) {}

// FUNCTION: SURRENDER 0x1006FBF0
void srDebugVP::unknown_2a4() {}

// FUNCTION: SURRENDER 0x1006FC00
void srDebugVP::_srGetClipFlags(SRBYTE* destination, const srVector4* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, COMMAND_SR_GET_CLIP_FLAGS, source, 0, 0, 0);
    processor->_srGetClipFlags(destination, source, count);
}
