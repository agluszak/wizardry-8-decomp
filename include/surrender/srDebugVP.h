#pragma once

#include "srVP.h"

class srVectorProcessor;

/* Debug wrapper installed over the active srVP by srVectorProcessor::startDebug. Every forwarding
   override scopes the wrapped call in a ScopeTimer, which accumulates elapsed time, element count,
   call count and - while check_misalignments is set - the number of pointer arguments that were not
   8- or 16-byte aligned. */
// VTABLE: SURRENDER 0x10077960 srDebugVP
class srDebugVP : public srVP {
    friend class srVectorProcessor;

public:
    /* Command ids, one per srVP slot; the names are descriptive. Slots 163 and 164 are unnamed. */
    enum e_command {
        COMMAND_DUMMY = 0,
        COMMAND_MEMCMP = 1,
        COMMAND_MEMCOPY_VOID_ARRAY_VOID_ARRAY = 2,
        COMMAND_MEMCOPY_VOID_ARRAY_BYTE = 3,
        COMMAND_PREFETCH = 4,
        COMMAND_COPY_INTERLEAVED = 5,
        COMMAND_SWAP = 6,
        COMMAND_COPY_DWORD_ARRAY_DWORD = 7,
        COMMAND_REVERSE = 8,
        COMMAND_AND_DWORD_ARRAY_DWORD_ARRAY_DWORD = 9,
        COMMAND_OR_DWORD_ARRAY_DWORD_ARRAY_DWORD = 10,
        COMMAND_XOR_DWORD_ARRAY_DWORD_ARRAY_DWORD = 11,
        COMMAND_AND_DWORD_ARRAY_DWORD_ARRAY_DWORD_ARRAY = 12,
        COMMAND_OR_DWORD_ARRAY_DWORD_ARRAY_DWORD_ARRAY = 13,
        COMMAND_XOR_DWORD_ARRAY_DWORD_ARRAY_DWORD_ARRAY = 14,
        COMMAND_ASR = 15,
        COMMAND_ASR_AND = 16,
        COMMAND_LSR = 17,
        COMMAND_LSL = 18,
        COMMAND_LSL_AND = 19,
        COMMAND_IS_EQUAL_DWORD_ARRAY_DWORD = 20,
        COMMAND_IS_EQUAL_DWORD_ARRAY_DWORD_ARRAY = 21,
        COMMAND_MIN_DWORD_ARRAY = 22,
        COMMAND_MAX_DWORD_ARRAY = 23,
        COMMAND_COPY_INDEXED_DWORD_ARRAY_DWORD_ARRAY_DWORD_ARRAY = 24,
        COMMAND_ADD_S_BYTE_ARRAY_BYTE_ARRAY_BYTE = 25,
        COMMAND_SUB_S_BYTE_ARRAY_BYTE_ARRAY_BYTE = 26,
        COMMAND_SUB_S_BYTE_ARRAY_BYTE_BYTE_ARRAY = 27,
        COMMAND_ADD_S_BYTE_ARRAY_BYTE_ARRAY_BYTE_ARRAY = 28,
        COMMAND_SUB_S_BYTE_ARRAY_BYTE_ARRAY_BYTE_ARRAY = 29,
        COMMAND_TO_FLOAT = 30,
        COMMAND_ADD_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY = 31,
        COMMAND_SUB_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY = 32,
        COMMAND_MUL_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY = 33,
        COMMAND_DIV_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY = 34,
        COMMAND_ADD_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY = 35,
        COMMAND_SUB_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY = 36,
        COMMAND_MUL_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY = 37,
        COMMAND_DIV_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY = 38,
        COMMAND_MUL_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY_FLOAT_ARRAY = 39,
        COMMAND_CLAMP = 40,
        COMMAND_CLAMP_MIN = 41,
        COMMAND_CLAMP_MAX = 42,
        COMMAND_CLAMP_UNIT = 43,
        COMMAND_SQRT = 44,
        COMMAND_ISQRT = 45,
        COMMAND_LERP = 46,
        COMMAND_IS_NEG = 47,
        COMMAND_IS_POS = 48,
        COMMAND_IS_ZERO = 49,
        COMMAND_MIN_FLOAT_ARRAY = 50,
        COMMAND_MAX_FLOAT_ARRAY = 51,
        COMMAND_MIN_MAX_FLOAT_ARRAY_FLOAT_CONSTANT_FLOAT_CONSTANT = 52,
        COMMAND_SUM = 53,
        COMMAND_AXPY_FLOAT_ARRAY_FLOAT_FLOAT_FLOAT_ARRAY = 54,
        COMMAND_AXPY_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY_FLOAT_ARRAY = 55,
        COMMAND_AXPY_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY = 56,
        COMMAND_AXPY_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY = 57,
        COMMAND_AXPY_FLOAT_ARRAY_FLOAT_FLOAT_FLOAT_ARRAY_FLOAT_ARRAY = 58,
        COMMAND_AXPY_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY_FLOAT_ARRAY = 59,
        COMMAND_MUL_INDEXED_FLOAT_ARRAY_FLOAT_ARRAY_FLOAT_ARRAY_DWORD_ARRAY = 60,
        COMMAND_MUL_INDEXED_FLOAT_ARRAY_FLOAT_FLOAT_ARRAY_DWORD_ARRAY = 61,
        COMMAND_TO_INT = 62,
        COMMAND_INV_POLY = 63,
        COMMAND_ABS = 64,
        COMMAND_NEG = 65,
        COMMAND_CUBIC = 66,
        COMMAND_COPY_VEC3_ARRAY_VEC3_CONSTANT = 67,
        COMMAND_COPY_VEC3_ARRAY_VEC4_ARRAY = 68,
        COMMAND_ADD_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY = 69,
        COMMAND_SUB_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY = 70,
        COMMAND_MUL_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY = 71,
        COMMAND_DIV_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY = 72,
        COMMAND_ADD_VEC3_ARRAY_VEC3_CONSTANT_FLOAT_ARRAY = 73,
        COMMAND_SUB_VEC3_ARRAY_VEC3_CONSTANT_FLOAT_ARRAY = 74,
        COMMAND_MUL_VEC3_ARRAY_VEC3_CONSTANT_FLOAT_ARRAY = 75,
        COMMAND_DIV_VEC3_ARRAY_VEC3_CONSTANT_FLOAT_ARRAY = 76,
        COMMAND_ADD_VEC3_ARRAY_VEC3_ARRAY_FLOAT_ARRAY = 77,
        COMMAND_SUB_VEC3_ARRAY_VEC3_ARRAY_FLOAT_ARRAY = 78,
        COMMAND_MUL_VEC3_ARRAY_VEC3_ARRAY_FLOAT_ARRAY = 79,
        COMMAND_DIV_VEC3_ARRAY_VEC3_ARRAY_FLOAT_ARRAY = 80,
        COMMAND_SUB_VEC3_ARRAY_FLOAT_ARRAY_VEC3_ARRAY = 81,
        COMMAND_DIV_VEC3_ARRAY_FLOAT_ARRAY_VEC3_ARRAY = 82,
        COMMAND_DOT_FLOAT_ARRAY_VEC3_CONSTANT_VEC3_ARRAY = 83,
        COMMAND_DOT_FLOAT_ARRAY_VEC3_ARRAY_VEC3_ARRAY = 84,
        COMMAND_CROSS = 85,
        COMMAND_LENGTH_FLOAT_ARRAY_VEC3_ARRAY = 86,
        COMMAND_NORMALIZE_VEC3_ARRAY_VEC3_ARRAY_FLOAT = 87,
        COMMAND_MIN_MAX_VEC3_ARRAY_VEC3_CONSTANT_VEC3_CONSTANT = 88,
        COMMAND_TRANSFORM_VEC3_ARRAY_VEC3_ARRAY_MAT4_CONSTANT = 89,
        COMMAND_COPY_INDEXED_VEC3_ARRAY_VEC2_ARRAY_DWORD_ARRAY = 90,
        COMMAND_COPY_INDEXED_VEC3_ARRAY_VEC3_ARRAY_DWORD_ARRAY = 91,
        COMMAND_COPY_INDEXED_VEC3_ARRAY_VEC4_ARRAY_DWORD_ARRAY = 92,
        COMMAND_MUL_INDEXED_VEC3_ARRAY_VEC3_ARRAY_VEC3_ARRAY_DWORD_ARRAY = 93,
        COMMAND_MUL_INDEXED_VEC3_ARRAY_VEC3_CONSTANT_VEC3_ARRAY_DWORD_ARRAY = 94,
        COMMAND_DIR_VEC3_ARRAY_FLOAT_ARRAY_VEC3_ARRAY = 95,
        COMMAND_DIR_VEC3_ARRAY_FLOAT_ARRAY_VEC4_ARRAY = 96,
        COMMAND_COPY_VEC4_ARRAY_VEC4_CONSTANT = 97,
        COMMAND_COPY_VEC4_ARRAY_VEC3_ARRAY_FLOAT = 98,
        COMMAND_COPY_VEC4_ARRAY_VEC3_ARRAY_FLOAT_ARRAY = 99,
        COMMAND_COPY_W_VEC4_ARRAY_FLOAT = 100,
        COMMAND_COPY_W_VEC4_ARRAY_FLOAT_ARRAY = 101,
        COMMAND_COPY_W_FLOAT_ARRAY_VEC4_ARRAY = 102,
        COMMAND_ADD_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY = 103,
        COMMAND_SUB_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY = 104,
        COMMAND_MUL_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY = 105,
        COMMAND_DIV_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY = 106,
        COMMAND_ADD_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY = 107,
        COMMAND_SUB_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY = 108,
        COMMAND_MUL_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY = 109,
        COMMAND_DIV_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY = 110,
        COMMAND_ADD_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY = 111,
        COMMAND_SUB_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY = 112,
        COMMAND_MUL_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY = 113,
        COMMAND_DIV_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY = 114,
        COMMAND_SUB_VEC4_ARRAY_FLOAT_ARRAY_VEC4_ARRAY = 115,
        COMMAND_DIV_VEC4_ARRAY_FLOAT_ARRAY_VEC4_ARRAY = 116,
        COMMAND_DOT_FLOAT_ARRAY_VEC4_CONSTANT_VEC4_ARRAY = 117,
        COMMAND_DOT_FLOAT_ARRAY_VEC4_ARRAY_VEC4_ARRAY = 118,
        COMMAND_LENGTH_FLOAT_ARRAY_VEC4_ARRAY = 119,
        COMMAND_NORMALIZE_VEC4_ARRAY_VEC4_ARRAY_FLOAT = 120,
        COMMAND_MIN_MAX_VEC4_ARRAY_VEC4_CONSTANT_VEC4_CONSTANT = 121,
        COMMAND_TRANSFORM_VEC4_ARRAY_VEC4_ARRAY_MAT4_CONSTANT = 122,
        COMMAND_TRANSFORM_VEC4_ARRAY_VEC3_ARRAY_MAT4_CONSTANT = 123,
        COMMAND_TRANSFORM_ORTHO = 124,
        COMMAND_TRANSFORM_PERSPECTIVE = 125,
        COMMAND_AXPY_VEC4_ARRAY_VEC4_CONSTANT_VEC4_CONSTANT_FLOAT_ARRAY = 126,
        COMMAND_AXPY_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY_FLOAT_ARRAY = 127,
        COMMAND_AXPY_VEC4_ARRAY_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY = 128,
        COMMAND_AXPY_VEC4_ARRAY_VEC4_ARRAY_VEC4_ARRAY_FLOAT_ARRAY = 129,
        COMMAND_AXPY_VEC4_ARRAY_VEC4_CONSTANT_VEC4_CONSTANT_FLOAT_ARRAY_FLOAT_ARRAY = 130,
        COMMAND_AXPY_VEC4_ARRAY_VEC4_ARRAY_VEC4_CONSTANT_FLOAT_ARRAY_FLOAT_ARRAY = 131,
        COMMAND_MUL_ADD_VEC4_ARRAY_VEC4_CONSTANT_VEC4_CONSTANT_VEC4_ARRAY = 132,
        COMMAND_MUL_ADD_VEC4_ARRAY_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY = 133,
        COMMAND_MUL_ADD_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY_VEC4_ARRAY = 134,
        COMMAND_DIV_BY_W = 135,
        COMMAND_COPY_INDEXED_VEC4_ARRAY_VEC2_ARRAY_DWORD_ARRAY = 136,
        COMMAND_COPY_INDEXED_VEC4_ARRAY_VEC3_ARRAY_DWORD_ARRAY = 137,
        COMMAND_COPY_INDEXED_VEC4_ARRAY_VEC4_ARRAY_DWORD_ARRAY = 138,
        COMMAND_COPY_INDEXED_VEC4_ARRAY_ARGB_ARRAY_DWORD_ARRAY = 139,
        COMMAND_MUL_INDEXED_VEC4_ARRAY_VEC4_ARRAY_VEC4_ARRAY_DWORD_ARRAY = 140,
        COMMAND_MUL_INDEXED_VEC4_ARRAY_VEC4_CONSTANT_VEC4_ARRAY_DWORD_ARRAY = 141,
        COMMAND_MUL_MAT4_CONSTANT_MAT4_CONSTANT_MAT4_CONSTANT = 142,
        COMMAND_MUL_MAT4_ARRAY_MAT4_ARRAY_MAT4_ARRAY = 143,
        COMMAND_SR_TEST_BOUNDING_BOX = 144,
        COMMAND_SR_SPECULAR_POW = 145,
        COMMAND_SR_COPY_INDEXED_REMAP = 146,
        COMMAND_SR_SET_INDEXED = 147,
        COMMAND_SR_COLLECT_POS = 148,
        COMMAND_SR_COLLECT_NEG = 149,
        COMMAND_SR_COLLECT_NON_ZERO = 150,
        COMMAND_SR_REMAP_INVERSE = 151,
        COMMAND_SR_DIRECT3_DCONVERT_COLOR = 152,
        COMMAND_TRANSFORM_INDEXED_VEC3_ARRAY_VEC3_ARRAY_DWORD_ARRAY_MAT4_CONSTANT = 153,
        COMMAND_TRANSFORM_INDEXED_VEC4_ARRAY_VEC3_ARRAY_DWORD_ARRAY_MAT4_CONSTANT = 154,
        COMMAND_DOT_INDEXED = 155,
        COMMAND_DOT_FLOAT_ARRAY_VEC4_CONSTANT_VEC3_ARRAY = 156,
        COMMAND_COPY_VEC2_ARRAY_VEC2_CONSTANT = 157,
        COMMAND_COPY_INDEXED_VEC2_ARRAY_VEC2_ARRAY_DWORD_ARRAY = 158,
        COMMAND_DIV_VEC2_ARRAY_VEC2_ARRAY_FLOAT_ARRAY = 159,
        COMMAND_SR_CULL_NO_CLIP = 160,
        COMMAND_SR_FLOAT_TO_LINEAR = 161,
        COMMAND_SR_LINEAR_TO_FLOAT = 162,
        COMMAND_SR_GET_CLIP_FLAGS = 165,
        COMMAND_COUNT = 166
    };

    srDebugVP(srVP* processor);
    /* Destruction is consistent with base-only cleanup; the reconstruction
       leaves the derived destructor implicit. */

    /* Every override below wraps the same-numbered call on processor in a ScopeTimer. The
       _max/_min(const SRDWORD*) bodies swap their command ids and targets. */
    virtual const char* getName() override;
    virtual int _memcmp(const void* source_0, const void* source_1, SRDWORD bytes) override;
    virtual void _memcopy(void* destination, int source, SRDWORD bytes) override;
    virtual void _memcopy(void* destination, const void* source, SRDWORD bytes) override;
    virtual void _prefetch(const void* destination, SRDWORD bytes, SRDWORD unused) override;
    virtual void _copyInterleaved(void* destination, const void* source, SRDWORD destination_pitch,
                                  SRDWORD source_pitch, SRDWORD width, SRDWORD count) override;
    virtual void _swap(void* first, void* second, SRDWORD bytes) override;
    virtual void _copy(srVector4* destination, const srVector3* source_0, const float* source_1,
                       SRDWORD count) override;
    virtual void _copy(srVector4* destination, const srVector3* source, float constant,
                       SRDWORD count) override;
    virtual void _copy(srVector4* destination, const srVector4& constant, SRDWORD count) override;
    virtual void _copy(srVector3* destination, const srVector4* source, SRDWORD count) override;
    virtual void _copy(srVector3* destination, const srVector3& constant, SRDWORD count) override;
    virtual void _copy(srVector2* destination, const srVector2& constant, SRDWORD count) override;
    virtual void _copy(SRDWORD* destination, SRDWORD constant, SRDWORD count) override;
    virtual void _reverse(SRDWORD* destination, const SRDWORD* source, SRDWORD count) override;
    virtual void _and(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                      SRDWORD count) override;
    virtual void _and(SRDWORD* destination, const SRDWORD* source, SRDWORD constant,
                      SRDWORD count) override;
    virtual void _or(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                     SRDWORD count) override;
    virtual void _or(SRDWORD* destination, const SRDWORD* source, SRDWORD constant,
                     SRDWORD count) override;
    virtual void _xor(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                      SRDWORD count) override;
    virtual void _xor(SRDWORD* destination, const SRDWORD* source, SRDWORD constant,
                      SRDWORD count) override;
    virtual void _asr(SRDWORD* destination, const SRDWORD* source, SRDWORD shift,
                      SRDWORD count) override;
    virtual void _asrAnd(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD mask,
                         SRDWORD count) override;
    virtual void _lsr(SRDWORD* destination, const SRDWORD* source, SRDWORD shift,
                      SRDWORD count) override;
    virtual void _lsl(SRDWORD* destination, const SRDWORD* source, SRDWORD shift,
                      SRDWORD count) override;
    virtual void _lslAnd(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD mask,
                         SRDWORD count) override;
    virtual int _isEqual(const SRDWORD* source_0, const SRDWORD* source_1, SRDWORD count) override;
    virtual int _isEqual(const SRDWORD* source, SRDWORD constant, SRDWORD count) override;
    virtual float _max(const float* source, SRDWORD count) override;
    virtual SRDWORD _max(const SRDWORD* source, SRDWORD count) override;
    virtual float _min(const float* source, SRDWORD count) override;
    virtual SRDWORD _min(const SRDWORD* source, SRDWORD count) override;
    virtual void _copyIndexed(srVector4* destination, const srARGB* source, const SRDWORD* indices,
                              SRDWORD count) override;
    virtual void _copyIndexed(srVector4* destination, const srVector4* source,
                              const SRDWORD* indices, SRDWORD count) override;
    virtual void _copyIndexed(srVector4* destination, const srVector3* source,
                              const SRDWORD* indices, SRDWORD count) override;
    virtual void _copyIndexed(srVector4* destination, const srVector2* source,
                              const SRDWORD* indices, SRDWORD count) override;
    virtual void _copyIndexed(srVector3* destination, const srVector4* source,
                              const SRDWORD* indices, SRDWORD count) override;
    virtual void _copyIndexed(srVector3* destination, const srVector3* source,
                              const SRDWORD* indices, SRDWORD count) override;
    virtual void _copyIndexed(srVector3* destination, const srVector2* source,
                              const SRDWORD* indices, SRDWORD count) override;
    virtual void _copyIndexed(srVector2* destination, const srVector2* source,
                              const SRDWORD* indices, SRDWORD count) override;
    virtual void _copyIndexed(SRDWORD* destination, const SRDWORD* source, const SRDWORD* indices,
                              SRDWORD count) override;
    virtual void _addS(SRBYTE* destination, const SRBYTE* source_0, const SRBYTE* source_1,
                       SRDWORD count) override;
    virtual void _addS(SRBYTE* destination, const SRBYTE* source, SRBYTE constant,
                       SRDWORD count) override;
    virtual void _subS(SRBYTE* destination, const SRBYTE* source_0, const SRBYTE* source_1,
                       SRDWORD count) override;
    virtual void _subS(SRBYTE* destination, SRBYTE constant, const SRBYTE* source,
                       SRDWORD count) override;
    virtual void _subS(SRBYTE* destination, const SRBYTE* source, SRBYTE constant,
                       SRDWORD count) override;
    virtual void _toFloat(float* destination, const SRBYTE* source, SRDWORD count) override;
    virtual void _add(srVector4* destination, const srVector4* vector_source,
                      const float* float_source, SRDWORD count) override;
    virtual void _add(srVector4* destination, const srVector4& constant, const float* float_source,
                      SRDWORD count) override;
    virtual void _add(srVector4* destination, const srVector4& constant,
                      const srVector4* vector_source, SRDWORD count) override;
    virtual void _add(srVector3* destination, const srVector3* vector_source,
                      const float* float_source, SRDWORD count) override;
    virtual void _add(srVector3* destination, const srVector3& constant, const float* float_source,
                      SRDWORD count) override;
    virtual void _add(srVector3* destination, const srVector3& constant,
                      const srVector3* vector_source, SRDWORD count) override;
    virtual void _add(float* destination, const float* source_0, const float* source_1,
                      SRDWORD count) override;
    virtual void _add(float* destination, float constant, const float* source,
                      SRDWORD count) override;
    virtual void _sub(srVector4* destination, const float* float_source,
                      const srVector4* vector_source, SRDWORD count) override;
    virtual void _sub(srVector4* destination, const srVector4* vector_source,
                      const float* float_source, SRDWORD count) override;
    virtual void _sub(srVector4* destination, const srVector4& constant, const float* float_source,
                      SRDWORD count) override;
    virtual void _sub(srVector4* destination, const srVector4& constant,
                      const srVector4* vector_source, SRDWORD count) override;
    virtual void _sub(srVector3* destination, const float* float_source,
                      const srVector3* vector_source, SRDWORD count) override;
    virtual void _sub(srVector3* destination, const srVector3* vector_source,
                      const float* float_source, SRDWORD count) override;
    virtual void _sub(srVector3* destination, const srVector3& constant, const float* float_source,
                      SRDWORD count) override;
    virtual void _sub(srVector3* destination, const srVector3& constant,
                      const srVector3* vector_source, SRDWORD count) override;
    virtual void _sub(float* destination, const float* source_0, const float* source_1,
                      SRDWORD count) override;
    virtual void _sub(float* destination, float constant, const float* source,
                      SRDWORD count) override;
    virtual void _mul(srMatrix4* destination, const srMatrix4* source_0, const srMatrix4* source_1,
                      SRDWORD count) override;
    virtual void _mul(srMatrix4& destination, const srMatrix4& source_0,
                      const srMatrix4& source_1) override;
    virtual void _mul(srVector4* destination, const srVector4* vector_source,
                      const float* float_source, SRDWORD count) override;
    virtual void _mul(srVector4* destination, const srVector4& constant, const float* float_source,
                      SRDWORD count) override;
    virtual void _mul(srVector4* destination, const srVector4& constant,
                      const srVector4* vector_source, SRDWORD count) override;
    virtual void _mul(srVector3* destination, const srVector3* vector_source,
                      const float* float_source, SRDWORD count) override;
    virtual void _mul(srVector3* destination, const srVector3& constant, const float* float_source,
                      SRDWORD count) override;
    virtual void _mul(srVector3* destination, const srVector3& constant,
                      const srVector3* vector_source, SRDWORD count) override;
    virtual void _mul(float* destination, float constant, const float* source_0,
                      const float* source_1, SRDWORD count) override;
    virtual void _mul(float* destination, const float* source_0, const float* source_1,
                      SRDWORD count) override;
    virtual void _mul(float* destination, float constant, const float* source,
                      SRDWORD count) override;
    virtual void _div(srVector4* destination, const float* float_source,
                      const srVector4* vector_source, SRDWORD count) override;
    virtual void _div(srVector4* destination, const srVector4* vector_source,
                      const float* float_source, SRDWORD count) override;
    virtual void _div(srVector4* destination, const srVector4& constant, const float* float_source,
                      SRDWORD count) override;
    virtual void _div(srVector4* destination, const srVector4& constant,
                      const srVector4* vector_source, SRDWORD count) override;
    virtual void _div(srVector3* destination, const float* float_source,
                      const srVector3* vector_source, SRDWORD count) override;
    virtual void _div(srVector3* destination, const srVector3* vector_source,
                      const float* float_source, SRDWORD count) override;
    virtual void _div(srVector3* destination, const srVector3& constant, const float* float_source,
                      SRDWORD count) override;
    virtual void _div(srVector3* destination, const srVector3& constant,
                      const srVector3* vector_source, SRDWORD count) override;
    virtual void _div(srVector2* destination, const srVector2* vector_source,
                      const float* float_source, SRDWORD count) override;
    virtual void _div(float* destination, const float* source_0, const float* source_1,
                      SRDWORD count) override;
    virtual void _div(float* destination, float constant, const float* source,
                      SRDWORD count) override;
    virtual void _clamp(float* destination, const float* source, float minimum, float maximum,
                        SRDWORD count) override;
    virtual void _clampMin(float* destination, const float* source, float minimum,
                           SRDWORD count) override;
    virtual void _clampMax(float* destination, const float* source, float maximum,
                           SRDWORD count) override;
    virtual void _clampUnit(float* destination, const float* source, SRDWORD count) override;
    virtual void _sqrt(float* destination, const float* source, SRDWORD count) override;
    virtual void _isqrt(float* destination, const float* source, SRDWORD count) override;
    virtual void _lerp(float* destination, const float* target, const float* source, float constant,
                       SRDWORD count) override;
    virtual int _isNeg(const float* source, SRDWORD count) override;
    virtual int _isPos(const float* source, SRDWORD count) override;
    virtual int _isZero(const float* source, SRDWORD count) override;
    virtual void _minMax(const srVector4* source, srVector4& minimum, srVector4& maximum,
                         SRDWORD count) override;
    virtual void _minMax(const srVector3* source, srVector3& minimum, srVector3& maximum,
                         SRDWORD count) override;
    virtual void _minMax(const float* source, float& minimum, float& maximum,
                         SRDWORD count) override;
    virtual double _sum(const float* source, SRDWORD count) override;
    virtual void _axpy(srVector4* destination, const srVector4* add_source,
                       const srVector4& multiply_constant, const float* multiply_source_0,
                       const float* multiply_source_1, SRDWORD count) override;
    virtual void _axpy(srVector4* destination, const srVector4& add_constant,
                       const srVector4& multiply_constant, const float* multiply_source_0,
                       const float* multiply_source_1, SRDWORD count) override;
    virtual void _axpy(srVector4* destination, const srVector4* add_source,
                       const srVector4* multiply_vectors, const float* multiply_source,
                       SRDWORD count) override;
    virtual void _axpy(srVector4* destination, const srVector4* add_source,
                       const srVector4& multiply_constant, const float* multiply_source,
                       SRDWORD count) override;
    virtual void _axpy(srVector4* destination, const srVector4& add_constant,
                       const srVector4* multiply_vectors, const float* multiply_source,
                       SRDWORD count) override;
    virtual void _axpy(srVector4* destination, const srVector4& add_constant,
                       const srVector4& multiply_constant, const float* multiply_source,
                       SRDWORD count) override;
    virtual void _axpy(float* destination, const float* add_source, float scale,
                       const float* scale_source, const float* multiply_source,
                       SRDWORD count) override;
    virtual void _axpy(float* destination, float add_constant, float scale,
                       const float* scale_source, const float* multiply_source,
                       SRDWORD count) override;
    virtual void _axpy(float* destination, const float* add_source, const float* scale_source,
                       const float* multiply_source, SRDWORD count) override;
    virtual void _axpy(float* destination, const float* add_source, float multiply_constant,
                       const float* multiply_source, SRDWORD count) override;
    virtual void _axpy(float* destination, float add_constant, const float* scale_source,
                       const float* multiply_source, SRDWORD count) override;
    virtual void _axpy(float* destination, float add_constant, float multiply_constant,
                       const float* multiply_source, SRDWORD count) override;
    virtual void _mulIndexed(srVector4* destination, const srVector4& constant,
                             const srVector4* indexed_source, const SRDWORD* indices,
                             SRDWORD count) override;
    virtual void _mulIndexed(srVector4* destination, const srVector4* linear_source,
                             const srVector4* indexed_source, const SRDWORD* indices,
                             SRDWORD count) override;
    virtual void _mulIndexed(srVector3* destination, const srVector3& constant,
                             const srVector3* indexed_source, const SRDWORD* indices,
                             SRDWORD count) override;
    virtual void _mulIndexed(srVector3* destination, const srVector3* linear_source,
                             const srVector3* indexed_source, const SRDWORD* indices,
                             SRDWORD count) override;
    virtual void _mulIndexed(float* destination, float constant, const float* indexed_source,
                             const SRDWORD* indices, SRDWORD count) override;
    virtual void _mulIndexed(float* destination, const float* linear_source,
                             const float* indexed_source, const SRDWORD* indices,
                             SRDWORD count) override;
    virtual void _toInt(SRLONG* destination, const float* source, SRDWORD count) override;
    virtual void _invPoly(float* destination, const float* source, const srVector3& poly,
                          SRDWORD count) override;
    virtual void _abs(float* destination, const float* source, SRDWORD count) override;
    virtual void _neg(float* destination, const float* source, SRDWORD count) override;
    virtual void _cubic(float* destination, const float* source, SRDWORD count) override;
    virtual void _dot(float* destination, const srVector4& constant, const srVector3* vectors,
                      SRDWORD count) override;
    virtual void _dot(float* destination, const srVector4* vectors_0, const srVector4* vectors_1,
                      SRDWORD count) override;
    virtual void _dot(float* destination, const srVector4& constant, const srVector4* vectors,
                      SRDWORD count) override;
    virtual void _dot(float* destination, const srVector3* vectors_0, const srVector3* vectors_1,
                      SRDWORD count) override;
    virtual void _dot(float* destination, const srVector3& constant, const srVector3* vectors,
                      SRDWORD count) override;
    virtual void _cross(srVector3* destination, const srVector3* vectors_0,
                        const srVector3* vectors_1, SRDWORD count) override;
    virtual void _length(float* destination, const srVector4* vectors, SRDWORD count) override;
    virtual void _length(float* destination, const srVector3* vectors, SRDWORD count) override;
    virtual void _normalize(srVector4* destination, const srVector4* vectors, float length,
                            SRDWORD count) override;
    virtual void _normalize(srVector3* destination, const srVector3* vectors, float length,
                            SRDWORD count) override;
    virtual void _transform(srVector4* destination, const srVector3* vectors,
                            const srMatrix4& matrix, SRDWORD count) override;
    virtual void _transform(srVector4* destination, const srVector4* vectors,
                            const srMatrix4& matrix, SRDWORD count) override;
    virtual void _transform(srVector3* destination, const srVector3* vectors,
                            const srMatrix4& matrix, SRDWORD count) override;
    virtual void _dir(srVector3* destination, float* lengths, const srVector4* source,
                      SRDWORD count) override;
    virtual void _dir(srVector3* destination, float* lengths, const srVector3* source,
                      SRDWORD count) override;
    virtual void _copyW(float* destination, const srVector4* source, SRDWORD count) override;
    virtual void _copyW(srVector4* destination, const float* source, SRDWORD count) override;
    virtual void _copyW(srVector4* destination, float constant, SRDWORD count) override;
    virtual void _transformOrtho(srVector4* destination, const srVector4* source,
                                 const srMatrix4& matrix, SRDWORD count) override;
    virtual void _transformPerspective(srVector4* destination, const srVector4* source,
                                       const srMatrix4& matrix, SRDWORD count) override;
    virtual void _mulAdd(srVector4* destination, const srVector4& add_constant,
                         const srVector4* multiply_source_0, const srVector4* multiply_source_1,
                         SRDWORD count) override;
    virtual void _mulAdd(srVector4* destination, const srVector4* add_source,
                         const srVector4& multiply_constant, const srVector4* multiply_source,
                         SRDWORD count) override;
    virtual void _mulAdd(srVector4* destination, const srVector4& add_constant,
                         const srVector4& multiply_constant, const srVector4* multiply_source,
                         SRDWORD count) override;
    virtual void _divByW(srVector4* destination, const srVector4* source, SRDWORD count) override;
    virtual int _srTestBoundingBox(const srMatrix4& matrix, const srVector3& minimum,
                                   const srVector3& maximum) override;
    virtual void _srSpecularPow(float* destination, const float* source, float exponent,
                                SRDWORD count) override;
    virtual void _srCopyIndexedRemap(srVector3i* destination, const srVector3i* source,
                                     const SRDWORD* indices, const SRDWORD* remap,
                                     SRDWORD count) override;
    virtual void _srSetIndexed(SRBYTE* destination, const srVector3i* source,
                               const SRDWORD* indices, SRDWORD count) override;
    virtual SRDWORD _srCollectPos(SRDWORD* destination, const float* source,
                                  SRDWORD count) override;
    virtual SRDWORD _srCollectNeg(SRDWORD* destination, const float* source,
                                  SRDWORD count) override;
    virtual SRDWORD _srCollectNonZero(SRDWORD* destination, const SRBYTE* source,
                                      SRDWORD count) override;
    virtual void _srRemapInverse(SRDWORD* destination, const SRDWORD* map, SRDWORD count) override;
    virtual void _srFloatToLinear(SRDWORD* destination, const float* source,
                                  SRDWORD count) override;
    virtual void _srLinearToFloat(float* destination, const SRDWORD* source,
                                  SRDWORD count) override;
    virtual void _srDirect3DConvertColor(SRDWORD* destination, const srVector4* source,
                                         SRDWORD count) override;
    virtual void _transformIndexed(srVector3* destination, const srVector3* source,
                                   const SRDWORD* indices, const srMatrix4& matrix,
                                   SRDWORD count) override;
    virtual void _transformIndexed(srVector4* destination, const srVector3* source,
                                   const SRDWORD* indices, const srMatrix4& matrix,
                                   SRDWORD count) override;
    virtual void _dotIndexed(float* destination, const srVector4& constant,
                             const srVector4* vectors, const SRDWORD* indices,
                             SRDWORD count) override;
    virtual SRDWORD _srCullNoClip(SRDWORD* destination, const srVector4& constant,
                                  const srVector4* vectors, SRDWORD count) override;
    virtual void unknown_2a0(SRDWORD, SRDWORD) override;
    virtual void unknown_2a4() override;
    virtual void _srGetClipFlags(SRBYTE* destination, const srVector4* source,
                                 SRDWORD count) override;

protected:
    /* RAII timer around every forwarded call; the pointer slots hold the forwarded arguments whose
       alignment the constructor counts. */
    class ScopeTimer {
    public:
        ScopeTimer(srDebugVP* owner, SRDWORD elements, e_command index, const void* pointer_0,
                   const void* pointer_1, const void* pointer_2, const void* pointer_3);
        ~ScopeTimer();

    private:
        SRDWORD elements;
        srDebugVP* owner;
        e_command index;
        double start_time;
    };
    friend class ScopeTimer;

    /* Zero disables the ScopeTimer alignment counters. */
    int check_misalignments;
    srVP* processor;
    double call_overhead;
    double call_times[COMMAND_COUNT];
    double element_counts[COMMAND_COUNT];
    w8_ulong call_counts[COMMAND_COUNT];
    w8_ulong misaligned8[COMMAND_COUNT];
    w8_ulong misaligned16[COMMAND_COUNT];

    static const char* command_names[COMMAND_COUNT];

private:
    void resetInternalStatistics();
};

static_assert(sizeof(srDebugVP::e_command) == 4, "srDebugVP_command_size");

W8_ABI_ASSERT((sizeof(srDebugVP) == 0x1678), "srDebugVP_must_be_0x1678");
