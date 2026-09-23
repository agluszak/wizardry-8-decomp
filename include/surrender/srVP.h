#pragma once

#include <string.h>

#include "srHeap.h"
#include "srMath.h"

class srARGB;

// Generic exports API 0x119, ID 0, and a factory returning an owned srVP.
// All three take no arguments; cdecl is the loader spelling, not proof that
// the SDK source could not have used the x86-equivalent stdcall spelling.
enum { SR_VP_MIN_API_VERSION = 0x119 };
class srVP;
typedef unsigned long(__cdecl* srGetVectorProcessorAPIFn)();
typedef long(__cdecl* srGetVectorProcessorIDFn)();
typedef srVP*(__cdecl* srInitVectorProcessorFn)();

/* These names are present in srDebugVP's own signature table. */
typedef unsigned char SRBYTE;
typedef long SRLONG;
typedef unsigned long SRDWORD;
typedef srVector2T<float> srVector2;
typedef srVector3T<float> srVector3;
typedef srVector4T<float> srVector4;
typedef srMatrix4T<float> srMatrix4;

/* srDebugVP forwards each entry to the same slot of its wrapped srVP; its
   vtable holds 167 slots (+0x000..+0x298) and every forwarder pushes the
   index of its own signature string, which fixes each slot's overload.
   VC6 does not emit same-named virtual overloads in declaration order:
   each newly declared overload lands at the head of its name group's run,
   so the slots of an overload set appear in reverse declaration order and
   the declarations below are sequenced to reproduce the retail vtable.
   The debug signature for _prefetch omits its third
   word even though the checked debug, generic, AMD3DNow and KNI
   implementations return with RET 0x0c. That argument stays an
   address-qualified 32-bit scalar until its original type name and
   signedness are recovered. Purity and member cv qualifiers are likewise not
   asserted when the surviving evidence cannot distinguish them.
   novtable like srDD: no ??_7srVP emission exists in retail, the table
   initializer at 0x10064C40 stores no vftable, and both concrete
   destructors (0x100658D0 generic, 0x1006FCA0 srDebugVP) store only their
   own vftable. */
class __declspec(novtable) srVP {
public:
    /* The constructor's emitted helper at 0x10064C40 fills the two tables
       below from stack copies of literal arrays; the identical sequence is
       inlined into srDebugVP's constructor and into the shipped srVP_*
       modules, so the original initializer was header-visible. */
    srVP()
    {
        initTables();
    }
    /* Pure like ~srDD: no srVP vftable is emitted anywhere in retail and the
       srDebugVP/generic destructors store only their own vftable. */
    virtual ~srVP() = 0;
    virtual const char* getName();
    virtual int _memcmp(const void* source_0, const void* source_1, SRDWORD bytes);
    virtual void _memcopy(void* destination, const void* source, SRDWORD bytes);
    virtual void _memcopy(void* destination, int source, SRDWORD bytes);
    virtual void _prefetch(const void* destination, SRDWORD bytes, SRDWORD value_014);
    virtual void _copyInterleaved(void* destination, const void* source, SRDWORD destination_pitch,
                                  SRDWORD source_pitch, SRDWORD width, SRDWORD count);
    virtual void _swap(void* first, void* second, SRDWORD bytes);
    virtual void _copy(SRDWORD* destination, SRDWORD constant, SRDWORD count);
    virtual void _copy(srVector2* destination, const srVector2& constant, SRDWORD count);
    virtual void _copy(srVector3* destination, const srVector3& constant, SRDWORD count);
    virtual void _copy(srVector3* destination, const srVector4* source, SRDWORD count);
    virtual void _copy(srVector4* destination, const srVector4& constant, SRDWORD count);
    virtual void _copy(srVector4* destination, const srVector3* source, float constant,
                       SRDWORD count);
    virtual void _copy(srVector4* destination, const srVector3* source_0, const float* source_1,
                       SRDWORD count);
    virtual void _reverse(SRDWORD* destination, const SRDWORD* source, SRDWORD count);
    virtual void _and(SRDWORD* destination, const SRDWORD* source, SRDWORD constant, SRDWORD count);
    virtual void _and(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                      SRDWORD count);
    virtual void _or(SRDWORD* destination, const SRDWORD* source, SRDWORD constant, SRDWORD count);
    virtual void _or(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                     SRDWORD count);
    virtual void _xor(SRDWORD* destination, const SRDWORD* source, SRDWORD constant, SRDWORD count);
    virtual void _xor(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                      SRDWORD count);
    virtual void _asr(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD count);
    virtual void _asrAnd(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD mask,
                         SRDWORD count);
    virtual void _lsr(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD count);
    virtual void _lsl(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD count);
    virtual void _lslAnd(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD mask,
                         SRDWORD count);
    virtual int _isEqual(const SRDWORD* source, SRDWORD constant, SRDWORD count);
    virtual int _isEqual(const SRDWORD* source_0, const SRDWORD* source_1, SRDWORD count);
    /* VC6 emits a new virtual overload at the head of its name group's slot
       run, so within a name group the vtable order is reverse declaration
       order. Declaring the SRDWORD forms first therefore produces the retail
       order _max(float) at +0x74, _max(SRDWORD) at +0x78, _min(float) at
       +0x7c and _min(SRDWORD) at +0x80; the shipped generic implementations
       at those slots confirm which is which. */
    virtual SRDWORD _max(const SRDWORD* source, SRDWORD count);
    virtual float _max(const float* source, SRDWORD count);
    virtual SRDWORD _min(const SRDWORD* source, SRDWORD count);
    virtual float _min(const float* source, SRDWORD count);
    virtual void _copyIndexed(SRDWORD* destination, const SRDWORD* source, const SRDWORD* indices,
                              SRDWORD count);
    virtual void _copyIndexed(srVector2* destination, const srVector2* source,
                              const SRDWORD* indices, SRDWORD count);
    virtual void _copyIndexed(srVector3* destination, const srVector2* source,
                              const SRDWORD* indices, SRDWORD count);
    virtual void _copyIndexed(srVector3* destination, const srVector3* source,
                              const SRDWORD* indices, SRDWORD count);
    virtual void _copyIndexed(srVector3* destination, const srVector4* source,
                              const SRDWORD* indices, SRDWORD count);
    virtual void _copyIndexed(srVector4* destination, const srVector2* source,
                              const SRDWORD* indices, SRDWORD count);
    virtual void _copyIndexed(srVector4* destination, const srVector3* source,
                              const SRDWORD* indices, SRDWORD count);
    virtual void _copyIndexed(srVector4* destination, const srVector4* source,
                              const SRDWORD* indices, SRDWORD count);
    virtual void _copyIndexed(srVector4* destination, const srARGB* source, const SRDWORD* indices,
                              SRDWORD count);
    virtual void _addS(SRBYTE* destination, const SRBYTE* source, SRBYTE constant, SRDWORD count);
    virtual void _addS(SRBYTE* destination, const SRBYTE* source_0, const SRBYTE* source_1,
                       SRDWORD count);
    virtual void _subS(SRBYTE* destination, const SRBYTE* source, SRBYTE constant, SRDWORD count);
    virtual void _subS(SRBYTE* destination, SRBYTE constant, const SRBYTE* source, SRDWORD count);
    virtual void _subS(SRBYTE* destination, const SRBYTE* source_0, const SRBYTE* source_1,
                       SRDWORD count);
    virtual void _toFloat(float* destination, const SRBYTE* source, SRDWORD count);
    virtual void _add(float* destination, float constant, const float* source, SRDWORD count);
    virtual void _add(float* destination, const float* source_0, const float* source_1,
                      SRDWORD count);
    virtual void _add(srVector3* destination, const srVector3& constant,
                      const srVector3* vector_source, SRDWORD count);
    virtual void _add(srVector3* destination, const srVector3& constant, const float* float_source,
                      SRDWORD count);
    virtual void _add(srVector3* destination, const srVector3* vector_source,
                      const float* float_source, SRDWORD count);
    virtual void _add(srVector4* destination, const srVector4& constant,
                      const srVector4* vector_source, SRDWORD count);
    virtual void _add(srVector4* destination, const srVector4& constant, const float* float_source,
                      SRDWORD count);
    virtual void _add(srVector4* destination, const srVector4* vector_source,
                      const float* float_source, SRDWORD count);
    virtual void _sub(float* destination, float constant, const float* source, SRDWORD count);
    virtual void _sub(float* destination, const float* source_0, const float* source_1,
                      SRDWORD count);
    virtual void _sub(srVector3* destination, const srVector3& constant,
                      const srVector3* vector_source, SRDWORD count);
    virtual void _sub(srVector3* destination, const srVector3& constant, const float* float_source,
                      SRDWORD count);
    virtual void _sub(srVector3* destination, const srVector3* vector_source,
                      const float* float_source, SRDWORD count);
    virtual void _sub(srVector3* destination, const float* float_source,
                      const srVector3* vector_source, SRDWORD count);
    virtual void _sub(srVector4* destination, const srVector4& constant,
                      const srVector4* vector_source, SRDWORD count);
    virtual void _sub(srVector4* destination, const srVector4& constant, const float* float_source,
                      SRDWORD count);
    virtual void _sub(srVector4* destination, const srVector4* vector_source,
                      const float* float_source, SRDWORD count);
    virtual void _sub(srVector4* destination, const float* float_source,
                      const srVector4* vector_source, SRDWORD count);
    virtual void _mul(float* destination, float constant, const float* source, SRDWORD count);
    virtual void _mul(float* destination, const float* source_0, const float* source_1,
                      SRDWORD count);
    virtual void _mul(float* destination, float constant, const float* source_0,
                      const float* source_1, SRDWORD count);
    virtual void _mul(srVector3* destination, const srVector3& constant,
                      const srVector3* vector_source, SRDWORD count);
    virtual void _mul(srVector3* destination, const srVector3& constant, const float* float_source,
                      SRDWORD count);
    virtual void _mul(srVector3* destination, const srVector3* vector_source,
                      const float* float_source, SRDWORD count);
    virtual void _mul(srVector4* destination, const srVector4& constant,
                      const srVector4* vector_source, SRDWORD count);
    virtual void _mul(srVector4* destination, const srVector4& constant, const float* float_source,
                      SRDWORD count);
    virtual void _mul(srVector4* destination, const srVector4* vector_source,
                      const float* float_source, SRDWORD count);
    virtual void _mul(srMatrix4& destination, const srMatrix4& source_0, const srMatrix4& source_1);
    virtual void _mul(srMatrix4* destination, const srMatrix4* source_0, const srMatrix4* source_1,
                      SRDWORD count);
    virtual void _div(float* destination, float constant, const float* source, SRDWORD count);
    virtual void _div(float* destination, const float* source_0, const float* source_1,
                      SRDWORD count);
    virtual void _div(srVector2* destination, const srVector2* vector_source,
                      const float* float_source, SRDWORD count);
    virtual void _div(srVector3* destination, const srVector3& constant,
                      const srVector3* vector_source, SRDWORD count);
    virtual void _div(srVector3* destination, const srVector3& constant, const float* float_source,
                      SRDWORD count);
    virtual void _div(srVector3* destination, const srVector3* vector_source,
                      const float* float_source, SRDWORD count);
    virtual void _div(srVector3* destination, const float* float_source,
                      const srVector3* vector_source, SRDWORD count);
    virtual void _div(srVector4* destination, const srVector4& constant,
                      const srVector4* vector_source, SRDWORD count);
    virtual void _div(srVector4* destination, const srVector4& constant, const float* float_source,
                      SRDWORD count);
    virtual void _div(srVector4* destination, const srVector4* vector_source,
                      const float* float_source, SRDWORD count);
    virtual void _div(srVector4* destination, const float* float_source,
                      const srVector4* vector_source, SRDWORD count);
    virtual void _clamp(float* destination, const float* source, float minimum, float maximum,
                        SRDWORD count);
    virtual void _clampMin(float* destination, const float* source, float minimum, SRDWORD count);
    virtual void _clampMax(float* destination, const float* source, float maximum, SRDWORD count);
    virtual void _clampUnit(float* destination, const float* source, SRDWORD count);
    virtual void _sqrt(float* destination, const float* source, SRDWORD count);
    virtual void _isqrt(float* destination, const float* source, SRDWORD count);
    virtual void _lerp(float* destination, const float* target, const float* source, float constant,
                       SRDWORD count);
    virtual int _isNeg(const float* source, SRDWORD count);
    virtual int _isPos(const float* source, SRDWORD count);
    virtual int _isZero(const float* source, SRDWORD count);
    virtual void _minMax(const float* source, float& minimum, float& maximum, SRDWORD count);
    virtual void _minMax(const srVector3* source, srVector3& minimum, srVector3& maximum,
                         SRDWORD count);
    virtual void _minMax(const srVector4* source, srVector4& minimum, srVector4& maximum,
                         SRDWORD count);
    virtual double _sum(const float* source, SRDWORD count);
    virtual void _axpy(float* destination, float add_constant, float multiply_constant,
                       const float* multiply_source, SRDWORD count);
    virtual void _axpy(float* destination, float add_constant, const float* scale_source,
                       const float* multiply_source, SRDWORD count);
    virtual void _axpy(float* destination, const float* add_source, float multiply_constant,
                       const float* multiply_source, SRDWORD count);
    virtual void _axpy(float* destination, const float* add_source, const float* scale_source,
                       const float* multiply_source, SRDWORD count);
    virtual void _axpy(float* destination, float add_constant, float scale,
                       const float* scale_source, const float* multiply_source, SRDWORD count);
    virtual void _axpy(float* destination, const float* add_source, float scale,
                       const float* scale_source, const float* multiply_source, SRDWORD count);
    virtual void _axpy(srVector4* destination, const srVector4& add_constant,
                       const srVector4& multiply_constant, const float* multiply_source,
                       SRDWORD count);
    virtual void _axpy(srVector4* destination, const srVector4& add_constant,
                       const srVector4* multiply_vectors, const float* multiply_source,
                       SRDWORD count);
    virtual void _axpy(srVector4* destination, const srVector4* add_source,
                       const srVector4& multiply_constant, const float* multiply_source,
                       SRDWORD count);
    virtual void _axpy(srVector4* destination, const srVector4* add_source,
                       const srVector4* multiply_vectors, const float* multiply_source,
                       SRDWORD count);
    virtual void _axpy(srVector4* destination, const srVector4& add_constant,
                       const srVector4& multiply_constant, const float* multiply_source_0,
                       const float* multiply_source_1, SRDWORD count);
    virtual void _axpy(srVector4* destination, const srVector4* add_source,
                       const srVector4& multiply_constant, const float* multiply_source_0,
                       const float* multiply_source_1, SRDWORD count);
    virtual void _mulIndexed(float* destination, const float* linear_source,
                             const float* indexed_source, const SRDWORD* indices, SRDWORD count);
    virtual void _mulIndexed(float* destination, float constant, const float* indexed_source,
                             const SRDWORD* indices, SRDWORD count);
    virtual void _mulIndexed(srVector3* destination, const srVector3* linear_source,
                             const srVector3* indexed_source, const SRDWORD* indices,
                             SRDWORD count);
    virtual void _mulIndexed(srVector3* destination, const srVector3& constant,
                             const srVector3* indexed_source, const SRDWORD* indices,
                             SRDWORD count);
    virtual void _mulIndexed(srVector4* destination, const srVector4* linear_source,
                             const srVector4* indexed_source, const SRDWORD* indices,
                             SRDWORD count);
    virtual void _mulIndexed(srVector4* destination, const srVector4& constant,
                             const srVector4* indexed_source, const SRDWORD* indices,
                             SRDWORD count);
    virtual void _toInt(SRLONG* destination, const float* source, SRDWORD count);
    virtual void _invPoly(float* destination, const float* source, const srVector3& poly,
                          SRDWORD count);
    virtual void _abs(float* destination, const float* source, SRDWORD count);
    virtual void _neg(float* destination, const float* source, SRDWORD count);
    virtual void _cubic(float* destination, const float* source, SRDWORD count);
    virtual void _dot(float* destination, const srVector3& constant, const srVector3* vectors,
                      SRDWORD count);
    virtual void _dot(float* destination, const srVector3* vectors_0, const srVector3* vectors_1,
                      SRDWORD count);
    virtual void _dot(float* destination, const srVector4& constant, const srVector4* vectors,
                      SRDWORD count);
    virtual void _dot(float* destination, const srVector4* vectors_0, const srVector4* vectors_1,
                      SRDWORD count);
    virtual void _dot(float* destination, const srVector4& constant, const srVector3* vectors,
                      SRDWORD count);
    virtual void _cross(srVector3* destination, const srVector3* vectors_0,
                        const srVector3* vectors_1, SRDWORD count);
    virtual void _length(float* destination, const srVector3* vectors, SRDWORD count);
    virtual void _length(float* destination, const srVector4* vectors, SRDWORD count);
    virtual void _normalize(srVector3* destination, const srVector3* vectors, float length,
                            SRDWORD count);
    virtual void _normalize(srVector4* destination, const srVector4* vectors, float length,
                            SRDWORD count);
    virtual void _transform(srVector3* destination, const srVector3* vectors,
                            const srMatrix4& matrix, SRDWORD count);
    virtual void _transform(srVector4* destination, const srVector4* vectors,
                            const srMatrix4& matrix, SRDWORD count);
    virtual void _transform(srVector4* destination, const srVector3* vectors,
                            const srMatrix4& matrix, SRDWORD count);
    virtual void _dir(srVector3* destination, float* lengths, const srVector3* source,
                      SRDWORD count);
    virtual void _dir(srVector3* destination, float* lengths, const srVector4* source,
                      SRDWORD count);
    virtual void _copyW(srVector4* destination, float constant, SRDWORD count);
    virtual void _copyW(srVector4* destination, const float* source, SRDWORD count);
    virtual void _copyW(float* destination, const srVector4* source, SRDWORD count);
    virtual void _transformOrtho(srVector4* destination, const srVector4* source,
                                 const srMatrix4& matrix, SRDWORD count);
    virtual void _transformPerspective(srVector4* destination, const srVector4* source,
                                       const srMatrix4& matrix, SRDWORD count);
    virtual void _mulAdd(srVector4* destination, const srVector4& add_constant,
                         const srVector4& multiply_constant, const srVector4* multiply_source,
                         SRDWORD count);
    virtual void _mulAdd(srVector4* destination, const srVector4* add_source,
                         const srVector4& multiply_constant, const srVector4* multiply_source,
                         SRDWORD count);
    virtual void _mulAdd(srVector4* destination, const srVector4& add_constant,
                         const srVector4* multiply_source_0, const srVector4* multiply_source_1,
                         SRDWORD count);
    virtual void _divByW(srVector4* destination, const srVector4* source, SRDWORD count);
    virtual int _srTestBoundingBox(const srMatrix4& matrix, const srVector3& minimum,
                                   const srVector3& maximum);
    virtual void _srSpecularPow(float* destination, const float* source, float exponent,
                                SRDWORD count);
    virtual void _srCopyIndexedRemap(srVector3i* destination, const srVector3i* source,
                                     const SRDWORD* indices, const SRDWORD* remap, SRDWORD count);
    virtual void _srSetIndexed(SRBYTE* destination, const srVector3i* source,
                               const SRDWORD* indices, SRDWORD count);
    virtual SRDWORD _srCollectPos(SRDWORD* destination, const float* source, SRDWORD count);
    virtual SRDWORD _srCollectNeg(SRDWORD* destination, const float* source, SRDWORD count);
    virtual SRDWORD _srCollectNonZero(SRDWORD* destination, const SRBYTE* source, SRDWORD count);
    virtual void _srRemapInverse(SRDWORD* destination, const SRDWORD* map, SRDWORD count);
    virtual void _srFloatToLinear(SRDWORD* destination, const float* source, SRDWORD count);
    virtual void _srLinearToFloat(float* destination, const SRDWORD* source, SRDWORD count);
    virtual void _srDirect3DConvertColor(SRDWORD* destination, const srVector4* source,
                                         SRDWORD count);
    /* The two _transformIndexed overloads land at +0x280/+0x284 in reverse
       declaration order like every other name group. */
    virtual void _transformIndexed(srVector4* destination, const srVector3* source,
                                   const SRDWORD* indices, const srMatrix4& matrix, SRDWORD count);
    virtual void _transformIndexed(srVector3* destination, const srVector3* source,
                                   const SRDWORD* indices, const srMatrix4& matrix, SRDWORD count);
    virtual void _dotIndexed(float* destination, const srVector4& constant,
                             const srVector4* vectors, const SRDWORD* indices, SRDWORD count);
    virtual SRDWORD _srCullNoClip(SRDWORD* destination, const srVector4& constant,
                                  const srVector4* vectors, SRDWORD count);
    /* The slots at +0x2a0/+0x2a4 are plain stubs in both shipped
       implementations (RET 0x8 and RET) and have no entry in srDebugVP's
       signature table, so their original names and parameter types are not
       recoverable; only the stack arity is proven. */
    virtual void unknown_2a0(SRDWORD arg0, SRDWORD arg1);
    virtual void unknown_2a4();
    virtual void _srGetClipFlags(SRBYTE* destination, const srVector4* source, SRDWORD count);

protected:
    /* srVectorProcessor's facade dispatches protected slots for srGERD's
       renderer the way retail does. */
    friend class srVectorProcessor;
    /* FUNCTION 0x10064C40 is this member helper emitted out of line: it runs
       on the whole srVP object, fills the tables at +0x08/+0x248, and stores
       no vftable, so it is not a complete-object constructor. The external
       VP modules reproduce the same storage, making the tables part of the
       srVP ABI; nothing in any shipped binary reads them back, so their
       original member names and consuming operation stay unproved. */
    // FUNCTION: SURRENDER 0x10064C40
    void initTables()
    {
        const double coefficients[0x12][4] = {
            {0, 1e-14, 1, 0},
            {-0.068979736114850004, 0.18179573075354999, 0.88948269483362996,
             -0.0032237222793199999},
            {-0.12576753110003999, 0.34382063299797, 0.78571136674362996, -0.0053941615606599999},
            {-0.17092256892982999, 0.48661079671178997, 0.68891153819327, -0.0067378060870700003},
            {-0.20507900130076001, 0.61089353418348002, 0.59915069732755, -0.0074389531232600002},
            {-0.22891455983919001, 0.71752488060732, 0.51637759148530005, -0.0076474469703200001},
            {-0.24312703917604001, 0.80744183884992005, 0.44045304108798999, -0.00748496029462},
            {-0.24841687096368001, 0.88162612503694004, 0.37117406021430999,
             -0.0070501036072999996},
            {-0.24547440268178, 0.94107687822182995, 0.30829269047641, -0.0064225898414199999},
            {-0.23497080142249999, 0.98679034859172998, 0.25153065736378, -0.0056666352687399996},
            {-0.21755174140842001, 1.01974500915189, 0.20059072482958001, -0.0048337418349100004},
            {-0.19383321994641001, 1.04089087244265, 0.15516544047627001, -0.0039649773154400002},
            {-0.16439899158457999, 1.05114205735211, 0.11494381932562001, -0.0030928469032899999},
            {-0.12979922349142001, 1.0513718575920701, 0.079616400344480004,
             -0.0022428316857099999},
            {-0.090550063549559995, 1.0424097253979201, 0.048879020037899998,
             -0.0014346549735899999},
            {-0.047133881807620001, 1.0250397112049401, 0.022435576368620001,
             -0.00068332584966999997},
            {-1e-14, 1.00000000000001, 0, 0},
            {-1e-14, 1.00000000000001, 0, 0},
        };
        const float points[0x12][7] = {
            {0.000999900047f, 0.0316211991f, 0.177823499f, 0.421691209f, 0.649377584f, 0.805839717f,
             0.897685707f},
            {0.00474390015f, 0.0688759983f, 0.262442291f, 0.512291312f, 0.715745211f, 0.846017301f,
             0.919791996f},
            {0.0081093004f, 0.0900517032f, 0.300086111f, 0.547801077f, 0.740135908f, 0.860311508f,
             0.927529812f},
            {0.0111445002f, 0.1055675f, 0.324911505f, 0.570010126f, 0.75499022f, 0.86890173f,
             0.932148993f},
            {0.0138889998f, 0.1178516f, 0.343295187f, 0.585914016f, 0.76545018f, 0.874900103f,
             0.935360909f},
            {0.0163755994f, 0.127967194f, 0.357724994f, 0.598101199f, 0.773370028f, 0.879414618f,
             0.937771082f},
            {0.0186313f, 0.136496499f, 0.369454414f, 0.607827604f, 0.779632986f, 0.882968307f,
             0.939663887f},
            {0.0206783991f, 0.143799901f, 0.379209489f, 0.615799904f, 0.784729183f, 0.885849416f,
             0.941195726f},
            {0.0225352999f, 0.150117606f, 0.387450188f, 0.622454882f, 0.788958073f, 0.888233185f,
             0.942461193f},
            {0.0242167003f, 0.155617207f, 0.394483387f, 0.628079116f, 0.792514384f, 0.890232801f,
             0.9435215f},
            {0.0257344991f, 0.160419807f, 0.400524408f, 0.632869899f, 0.795531213f, 0.891925573f,
             0.944418073f},
            {0.0270971991f, 0.164612293f, 0.405724406f, 0.636964977f, 0.798100889f, 0.893364906f,
             0.94517982f},
            {0.0283103995f, 0.168256894f, 0.410191387f, 0.640461802f, 0.800288618f, 0.89458847f,
             0.945826888f},
            {0.0293761995f, 0.171394899f, 0.413998604f, 0.643427312f, 0.802139223f, 0.895622194f,
             0.946373224f},
            {0.0302920006f, 0.174045995f, 0.417188197f, 0.645901084f, 0.803679705f, 0.896481812f,
             0.946827292f},
            {0.0310484003f, 0.176205605f, 0.419768512f, 0.647895396f, 0.804919481f, 0.897173107f,
             0.947192192f},
            {0.0316227004f, 0.177827701f, 0.421696186f, 0.649381399f, 0.805842102f, 0.897687078f,
             0.947463512f},
            {0.0316227004f, 0.177827701f, 0.421696186f, 0.649381399f, 0.805842102f, 0.897687078f,
             0.947463512f},
        };
        int row;

        for (row = 0; row < 0x12; ++row) {
            memcpy(coefficients_08[row], coefficients[row], 0x20);
            memcpy(points_248[row], points[row], 0x1c);
        }
    }

    double coefficients_08[0x12][4];
    float points_248[0x12][7];
};

inline srVP::~srVP() {} // pure virtual destructor body

static_assert((sizeof(srVP) == 0x440), "srVP_must_be_0x440");
