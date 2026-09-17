#pragma once

#include "srVP.h"

/* Built-in fallback vector processor instantiated by
   srVectorProcessor::initBaseVP when no better module is installed. The
   retail vtable at 0x100776B0 proves all 166 command implementations live in
   sr.dll itself; the shipped srVP_Generic.dll packages the same
   implementation set as a loadable module. Slots 164/165 are empty stubs in
   both. */
// VTABLE: SURRENDER 0x100776B0 srVP_generic
class srVP_generic : public srVP {
public:
    virtual ~srVP_generic() override;
    // SYNTHETIC: SURRENDER 0x100658B0
    // srVP_generic::`scalar deleting destructor'

    virtual const char* getName() override;

    virtual int _memcmp(const void* source_0, const void* source_1, SRDWORD bytes) override;

    virtual void _memcopy(void* destination, int source, SRDWORD bytes) override;

    virtual void _memcopy(void* destination, const void* source, SRDWORD bytes) override;

    virtual void _prefetch(const void* destination, SRDWORD bytes, SRDWORD value_014) override;

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

    virtual void unknown_2a0(SRDWORD arg0, SRDWORD arg1) override;

    virtual void unknown_2a4() override;

    virtual void _srGetClipFlags(SRBYTE* destination, const srVector4* source,
                                 SRDWORD count) override;
};
