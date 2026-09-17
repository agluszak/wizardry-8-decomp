#pragma once

#include "srVP.h"

class srVectorProcessor;

/* Debug wrapper installed over the active srVP by
   srVectorProcessor::startDebug. The constructor at 0x10068FD0 proves the
   layout: a 0x1678-byte object whose srVP base carries the inherited lookup
   tables, followed by the wrapped processor, the calibrated per-call timing
   overhead and five 166-entry statistics arrays, one entry per command in
   the signature table at 0x100A9250 (index = vtable slot - 1; entry 0 is
   the "dummy command" lead-in). Every forwarding override scopes the
   wrapped call in a ScopeTimer, which accumulates elapsed time, element
   count, call count and - while check_misalignments_440 is set - the number
   of pointer arguments that were not 8- or 16-byte aligned. */
// VTABLE: SURRENDER 0x10077960 srDebugVP
class srDebugVP : public srVP {
    friend class srVectorProcessor;

public:
    /* Only the constructor and resetInternalStatistics are exported; the
       forwarders are reached exclusively through the srVP vtable, so the
       class carries no blanket import specifier. */
    SR_DLL_IMPORT srDebugVP(srVP* processor);
    virtual ~srDebugVP() override;
    // SYNTHETIC: SURRENDER 0x1006FC80
    // srDebugVP::`scalar deleting destructor'

    /* Every override below wraps the same-numbered call on processor_444 in
       a ScopeTimer; declaration order mirrors the retail vtable slots. The
       _max(const SRDWORD*)/_min(const SRDWORD*) bodies are swapped in retail
       (each logs the other's command index and forwards to the other's
       slot); see debug_vp.cpp. */
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

protected:
    /* RAII timer constructed at the top of every srDebugVP forwarder and
       destroyed after the wrapped call returns. The four pointer slots hold
       the forwarded destination/source arguments whose alignment the
       constructor counts. */
    class ScopeTimer {
    public:
        ScopeTimer(srDebugVP* owner, SRDWORD elements, int index, const void* pointer_0,
                   const void* pointer_1, const void* pointer_2, const void* pointer_3);
        ~ScopeTimer();

    private:
        SRDWORD elements_00;
        srDebugVP* owner_04;
        int index_08;
        double start_10;
    };
    /* VC6 does not grant a nested class access to the enclosing class's
       protected members, so the statistics arrays stay reachable through an
       explicit friend declaration. */
    friend class ScopeTimer;

    /* Written from srVectorProcessor::startDebug's argument after
       construction; zero disables the ScopeTimer alignment counters and
       makes dump print "misAlignments not checked". */
    int check_misalignments_440;
    srVP* processor_444;
    double call_overhead_448;
    double call_times_450[0xa6];
    double element_counts_980[0xa6];
    unsigned long call_counts_eb0[0xa6];
    unsigned long misaligned8_1148[0xa6];
    unsigned long misaligned16_13e0[0xa6];

    static const char* command_names[0xa6];

private:
    void resetInternalStatistics();
};

static_assert((sizeof(srDebugVP) == 0x1678), "srDebugVP_must_be_0x1678");
