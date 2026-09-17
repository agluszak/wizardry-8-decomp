#include "surrender/srDebugVP.h"

#include "surrender/srCore.h"

// GLOBAL: SURRENDER 0x100A9250
const char* srDebugVP::command_names[0xa6];

// FUNCTION: SURRENDER 0x10068FD0
srDebugVP::srDebugVP(srVP* processor)
{
    int iteration;

    processor_444 = processor;
    call_times_450[0] = 0.0;
    for (iteration = 0; iteration < 0x2710; ++iteration) {
        ScopeTimer scope(this, 0, 0, 0, 0, 0, 0);
    }
    call_overhead_448 = call_times_450[0] * 0.0001;
    resetInternalStatistics();
    command_names[0] = "dummy command";
    command_names[1] = "_memcmp  (const void* src0, const void* src1,  const SRDWORD bytes)";
    command_names[2] = "_memcopy (void* dest, const void* src,  const SRDWORD bytes)";
    command_names[3] = "_memcopy (void* dest, const SRBYTE src,  const SRDWORD bytes)";
    command_names[4] = "_prefetch (const void* dest, const SRDWORD bytes)";
    command_names[5] = "_copyInterleaved(void* dest, const void* src, SRDWORD dstPitch, SRDWORD "
                       "srcPitch, SRDWORD width, SRDWORD n)";
    command_names[6] = "_swap (void* d0, void* d1, const SRDWORD bytes)";
    command_names[7] = "_copy (SRDWORD* dest, const SRDWORD c, const SRDWORD n)";
    command_names[8] = "_reverse (SRDWORD* dest, const SRDWORD* s, const SRDWORD n)";
    command_names[9] = "_and  (SRDWORD* dest, const SRDWORD* s,  const SRDWORD c, const SRDWORD n)";
    command_names[10] = "_or  (SRDWORD* dest, const SRDWORD* s,  const SRDWORD c, const SRDWORD n)";
    command_names[11] =
        "_xor  (SRDWORD* dest, const SRDWORD* s,  const SRDWORD c, const SRDWORD n)";
    command_names[12] =
        "_and  (SRDWORD* dest, const SRDWORD* s0,  const SRDWORD* s1, const SRDWORD n)";
    command_names[13] =
        "_or  (SRDWORD* dest, const SRDWORD* s0,  const SRDWORD* s1, const SRDWORD n)";
    command_names[14] =
        "_xor  (SRDWORD* dest, const SRDWORD* s0,  const SRDWORD* s1, const SRDWORD n)";
    command_names[15] =
        "_asr  (SRDWORD* dest, const SRDWORD* s0, const SRDWORD sh, const SRDWORD n)";
    command_names[16] = "_asrAnd  (SRDWORD* dest, const SRDWORD* s0, const SRDWORD sh, const "
                        "SRDWORD mask, const SRDWORD n)";
    command_names[17] =
        "_lsr  (SRDWORD* dest, const SRDWORD* s0, const SRDWORD sh, const SRDWORD n)";
    command_names[18] =
        "_lsl  (SRDWORD* dest, const SRDWORD* s0, const SRDWORD sh, const SRDWORD n)";
    command_names[19] = "_lslAnd  (SRDWORD* dest, const SRDWORD* s0, const SRDWORD sh, const "
                        "SRDWORD mask, const SRDWORD n)";
    command_names[20] = "_isEqual (const SRDWORD* s0, const SRDWORD c,  const SRDWORD n)";
    command_names[21] = "_isEqual (const SRDWORD* s0, const SRDWORD* s1,  const SRDWORD n)";
    command_names[22] = "_min  (const SRDWORD* s0, const SRDWORD n)";
    command_names[23] = "_max  (const SRDWORD* s0, const SRDWORD n)";
    command_names[24] =
        "_copyIndexed (SRDWORD* dst, const SRDWORD* src,  const SRDWORD* ixTable, SRDWORD n)";
    command_names[25] = "_addS  (SRBYTE* d,  const SRBYTE* s,  const SRBYTE c,  const SRDWORD n)";
    command_names[26] = "_subS  (SRBYTE* d,  const SRBYTE* s,  const SRBYTE c,  const SRDWORD n)";
    command_names[27] = "_subS  (SRBYTE* d,  const SRBYTE c,  const SRBYTE* s, const SRDWORD n)";
    command_names[28] = "_addS  (SRBYTE* d,  const SRBYTE* s0,  const SRBYTE* s1, const SRDWORD n)";
    command_names[29] = "_subS  (SRBYTE* d,  const SRBYTE* s0,  const SRBYTE* s1, const SRDWORD n)";
    command_names[30] = "_toFloat (float* d,  const SRBYTE* s,  const SRDWORD n)";
    command_names[31] = "_add (float* d, const float cf, const float* source, const SRDWORD n)";
    command_names[32] = "_sub (float* d, const float cf, const float* source, const SRDWORD n)";
    command_names[33] = "_mul (float* d, const float cf, const float* multiplier,const SRDWORD n)";
    command_names[34] = "_div (float* d, const float cf, const float* divisor, const SRDWORD n)";
    command_names[35] = "_add (float* d, const float *fs0, const float *fs1, const SRDWORD n)";
    command_names[36] = "_sub (float* d, const float *fs0, const float *fs1, const SRDWORD n)";
    command_names[37] = "_mul (float* d, const float *fs0, const float *fs1, const SRDWORD n)";
    command_names[38] = "_div (float* d, const float *fs0, const float *fs1, const SRDWORD n)";
    command_names[39] =
        "_mul (float* d, const float c, const float *fs0, const float *fs1, const SRDWORD n)";
    command_names[40] = "_clamp (float* dest, const float* source, const float min, const float "
                        "max, const SRDWORD n);";
    command_names[41] =
        "_clampMin (float* dest, const float* source, const float min, const SRDWORD n);";
    command_names[42] =
        "_clampMax (float* dest, const float* source, const float max, const SRDWORD n);";
    command_names[43] = "_clampUnit (float* dest, const float* source, const SRDWORD n);";
    command_names[44] = "_sqrt (float* dest, const float* source, SRDWORD n);";
    command_names[45] = "_isqrt (float* dest, const float* source, SRDWORD n);";
    command_names[46] = "_lerp (float* dest, const float* target, const float* source, const float "
                        "constant, SRDWORD n);";
    command_names[47] = "_isNeg (const float* dest, const SRDWORD n);";
    command_names[48] = "_isPos (const float* dest, const SRDWORD n);";
    command_names[49] = "_isZero (const float* dest, const SRDWORD n);";
    command_names[50] = "_min (const float* src, const SRDWORD n);";
    command_names[51] = "_max (const float* src, const SRDWORD n);";
    command_names[52] = "_minMax (const float* src, float& min, float& max, const SRDWORD n)";
    command_names[53] = "_sum (const float* src, const SRDWORD n);";
    command_names[54] = "_axpy (float* dest, const float ca, const float constant, const float* "
                        "srcm, const SRDWORD n);";
    command_names[55] = "_axpy (float* dest, const float ca, const float* srcs, const float* srcm, "
                        "const SRDWORD n);";
    command_names[56] = "_axpy (float* dest, const float* srca, const float constant, const float* "
                        "srcm, const SRDWORD n);";
    command_names[57] = "_axpy (float* dest, const float* srca, const float* srcs, const float* "
                        "srcm, const SRDWORD n);";
    command_names[58] = "_axpy (float* dest, const float ca, const float scale, const float* srcs, "
                        "const float* srcm, const SRDWORD n)";
    command_names[59] = "_axpy (float* dest, const float* srca, const float scale, const float* "
                        "srcs, const float* srcm, const SRDWORD n)";
    command_names[60] = "_mulIndexed (float* dest, const float* linear, const float* indexed, "
                        "const SRDWORD* indices, const SRDWORD n); ";
    command_names[61] = "_mulIndexed (float* dest, const float constant, const float* indexed, "
                        "const SRDWORD* indices, const SRDWORD n); ";
    command_names[62] = "_toInt  (SRLONG* dest, const float* fs, const SRDWORD n); ";
    command_names[63] = "_invPoly\t\t(float* dest, const float* source, const srVector3& poly, "
                        "const SRDWORD count)";
    command_names[64] = "_abs (float* dest, const float* source, const SRDWORD n)";
    command_names[65] = "_neg (float* dest, const float* source, const SRDWORD n)";
    command_names[66] = "_cubic (float* dest, const float* source, const SRDWORD count);";
    command_names[67] = "_copy  (srVector3* dest, const srVector3& cv, const SRDWORD n)";
    command_names[68] = "_copy  (srVector3* dest, const srVector4* vs, const SRDWORD n)";
    command_names[69] =
        "_add  (srVector3* dest, const srVector3& cv, const srVector3* vs, const SRDWORD n)";
    command_names[70] =
        "_sub  (srVector3* dest, const srVector3& cv, const srVector3* vs, const SRDWORD n)";
    command_names[71] =
        "_mul  (srVector3* dest, const srVector3& cv, const srVector3* vs, const SRDWORD n)";
    command_names[72] =
        "_div  (srVector3* dest, const srVector3& cv, const srVector3* vs, const SRDWORD n)";
    command_names[73] =
        "_add  (srVector3* dest, const srVector3& cv, const float* fs, const SRDWORD n)";
    command_names[74] =
        "_sub  (srVector3* dest, const srVector3& cv, const float* fs, const SRDWORD n)";
    command_names[75] =
        "_mul  (srVector3* dest, const srVector3& cv, const float* fs, const SRDWORD n)";
    command_names[76] =
        "_div  (srVector3* dest, const srVector3& cv, const float* fs, const SRDWORD n)";
    command_names[77] =
        "_add  (srVector3* dest, const srVector3* vs, const float* fs, const SRDWORD n)";
    command_names[78] =
        "_sub  (srVector3* dest, const srVector3* vs, const float* fs, const SRDWORD n)";
    command_names[79] =
        "_mul  (srVector3* dest, const srVector3* vs, const float* fs, const SRDWORD n)";
    command_names[80] =
        "_div  (srVector3* dest, const srVector3* vs, const float* fs, const SRDWORD n)";
    command_names[81] =
        "_sub  (srVector3* dest, const float* fs, const srVector3* vs, const SRDWORD n)";
    command_names[82] =
        "_div  (srVector3* dest, const float* fs, const srVector3* vs, const SRDWORD n)";
    command_names[83] =
        "_dot  (float* dest, const srVector3& cv, const srVector3* vs, const SRDWORD n)";
    command_names[84] =
        "_dot  (float* dest, const srVector3* vs0, const srVector3* vs1, const SRDWORD n)";
    command_names[85] =
        "_cross  (srVector3* dest, const srVector3* vs0, const srVector3* vs1, const SRDWORD n)";
    command_names[86] = "_length (float* dest, const srVector3* vs, SRDWORD n)";
    command_names[87] =
        "_normalize (srVector3* dest, const srVector3* vs, const float length, const SRDWORD n)";
    command_names[88] =
        "_minMax  (const srVector3* s0, srVector3& min, srVector3& max, const SRDWORD n)";
    command_names[89] = "_transform (srVector3* dest, const srVector3* vs, const srMatrix4& "
                        "matrix, const SRDWORD n)";
    command_names[90] =
        "_copyIndexed (srVector3* dest, const srVector2* src, const SRDWORD* indices, SRDWORD n)";
    command_names[91] =
        "_copyIndexed (srVector3* dest, const srVector3* src, const SRDWORD* indices, SRDWORD n)";
    command_names[92] =
        "_copyIndexed (srVector3* dest, const srVector4* src, const SRDWORD* indices, SRDWORD n)";
    command_names[93] = "_mulIndexed (srVector3* dest, const srVector3* linearSource, const "
                        "srVector3* indexedSource, const SRDWORD* indices, SRDWORD n)";
    command_names[94] = "_mulIndexed (srVector3* dest, const srVector3& constant, const srVector3* "
                        "indexedSource,  const SRDWORD* indices, SRDWORD n)";
    command_names[95] =
        "_dir  (srVector3* dst, float* dst2,  const srVector3* src, const SRDWORD n)";
    command_names[96] =
        "_dir  (srVector3* dst, float* dst2,  const srVector4* src, const SRDWORD n)";
    command_names[97] = "_copy  (srVector4* dest, const srVector4& constant, const SRDWORD n);";
    command_names[98] =
        "_copy  (srVector4* dest, const srVector3* s0, const float c, const SRDWORD n);";
    command_names[99] =
        "_copy  (srVector4* dest, const srVector3* s0, const float* s1, const SRDWORD n)";
    command_names[100] = "_copyW  (srVector4* d, const float c, const SRDWORD n);";
    command_names[101] = "_copyW  (srVector4* d, const float* s, const SRDWORD n);";
    command_names[102] = "_copyW  (float*d, const srVector4* s, const SRDWORD n);";
    command_names[103] =
        "_add  (srVector4* dest, const srVector4& cv, const srVector4* vs, const SRDWORD n)";
    command_names[104] =
        "_sub  (srVector4* dest, const srVector4& cv, const srVector4* vs, const SRDWORD n)";
    command_names[105] =
        "_mul  (srVector4* dest, const srVector4& cv, const srVector4* vs, const SRDWORD n)";
    command_names[106] =
        "_div  (srVector4* dest, const srVector4& cv, const srVector4* vs, const SRDWORD n)";
    command_names[107] =
        "_add  (srVector4* dest, const srVector4& cv, const float* fs, const SRDWORD n)";
    command_names[108] =
        "_sub  (srVector4* dest, const srVector4& cv, const float* fs, const SRDWORD n)";
    command_names[109] =
        "_mul  (srVector4* dest, const srVector4& cv, const float* fs, const SRDWORD n)";
    command_names[110] =
        "_div  (srVector4* dest, const srVector4& cv, const float* fs, const SRDWORD n)";
    command_names[111] =
        "_add  (srVector4* dest, const srVector4* vs, const float* fs, const SRDWORD n)";
    command_names[112] =
        "_sub  (srVector4* dest, const srVector4* vs, const float* fs, const SRDWORD n)";
    command_names[113] =
        "_mul  (srVector4* dest, const srVector4* vs, const float* fs, const SRDWORD n)";
    command_names[114] =
        "_div  (srVector4* dest, const srVector4* vs, const float* fs, const SRDWORD n)";
    command_names[115] =
        "_sub  (srVector4* dest, const float* fs, const srVector4* vs, const SRDWORD n)";
    command_names[116] =
        "_div  (srVector4* dest, const float* fs, const srVector4* vs, const SRDWORD n)";
    command_names[117] =
        "_dot  (float* dest, const srVector4& cv, const srVector4* vs, const SRDWORD n)";
    command_names[118] =
        "_dot  (float* dest, const srVector4* vs0, const srVector4* vs1, const SRDWORD n)";
    command_names[119] = "_length (float* dest, const srVector4* vs, SRDWORD n)";
    command_names[120] =
        "_normalize (srVector4* dest, const srVector4* vs, const float length, const SRDWORD n)";
    command_names[121] =
        "_minMax  (const srVector4* s0, srVector4& min, srVector4& max, const SRDWORD n)";
    command_names[122] =
        "_transform  (srVector4* dst, const srVector4* src, const srMatrix4& m, const SRDWORD n)";
    command_names[123] =
        "_transform  (srVector4* dst, const srVector3* src, const srMatrix4& m, const SRDWORD n)";
    command_names[124] = "_transformOrtho  (srVector4* dst, const srVector4* src, const srMatrix4& "
                         "m, const SRDWORD n)";
    command_names[125] = "_transformPerspective (srVector4* dst, const srVector4* src, const "
                         "srMatrix4& m, const SRDWORD n)";
    command_names[126] = "_axpy  (srVector4* d, const srVector4& srca, const srVector4& c, const "
                         "float* srcm, const SRDWORD n)";
    command_names[127] = "_axpy  (srVector4* d, const srVector4& srca, const srVector4* c, const "
                         "float* srcm, const SRDWORD n)";
    command_names[128] = "_axpy  (srVector4* d, const srVector4* srca, const srVector4& c, const "
                         "float* srcm, const SRDWORD n)";
    command_names[129] = "_axpy  (srVector4* d, const srVector4* srca, const srVector4* c, const "
                         "float* srcm, const SRDWORD n)";
    command_names[130] = "_axpy  (srVector4* d, const srVector4& ca, const srVector4& cm, const "
                         "float* srcm0, const float* srcm1, const SRDWORD n)";
    command_names[131] = "_axpy  (srVector4* d, const srVector4* srca, const srVector4& cm, const "
                         "float* srcm0, const float* srcm1, const SRDWORD n)";
    command_names[132] = "_mulAdd  (srVector4* d, const srVector4& ca, const srVector4& cm, const "
                         "srVector4* srcm, const SRDWORD n);";
    command_names[133] = "_mulAdd  (srVector4* d, const srVector4* srca, const srVector4& cm, "
                         "const srVector4* srcm, const SRDWORD n);";
    command_names[134] = "_mulAdd  (srVector4* d, const srVector4& ca, const srVector4* srcm0, "
                         "const srVector4* srcm1, const SRDWORD n)";
    command_names[135] = "_divByW  (srVector4* dst, const srVector4* src, const SRDWORD n)";
    command_names[136] =
        "_copyIndexed (srVector4* dest, const srVector2* src, const SRDWORD* indices, SRDWORD n)";
    command_names[137] =
        "_copyIndexed (srVector4* dest, const srVector3* src, const SRDWORD* indices, SRDWORD n)";
    command_names[138] =
        "_copyIndexed (srVector4* dest, const srVector4* src, const SRDWORD* indices, SRDWORD n)";
    command_names[139] =
        "_copyIndexed (srVector4* dest, const srARGB*    src, const SRDWORD* indices, SRDWORD n)";
    command_names[140] = "_mulIndexed (srVector4* dest, const srVector4* linearSource, const "
                         "srVector4* indexedSource, const SRDWORD* indices, SRDWORD n)";
    command_names[141] = "_mulIndexed (srVector4* dest, const srVector4& constant, const "
                         "srVector4* indexedSource,  const SRDWORD* indices, SRDWORD n)";
    command_names[142] = "_mul (srMatrix4& dst,\tconst\tsrMatrix4& ms0,\tconst srMatrix4& ms1)";
    command_names[143] =
        "_mul (srMatrix4* dst, const srMatrix4* ms0, const srMatrix4* ms1, const SRDWORD n)";
    command_names[144] =
        "_srTestBoundingBox(const srMatrix4& m, const srVector3& min, const srVector3& max)";
    command_names[145] = "_srSpecularPow\t(float* dest, const float* source, const float exponent, "
                         "const SRDWORD count)";
    command_names[146] = "_srCopyIndexedRemap (srVector3i* dst, const srVector3i* src, const "
                         "SRDWORD* ixTable, const SRDWORD* remap, const SRDWORD count)";
    command_names[147] = "_srSetIndexed (SRBYTE *dst, const srVector3i* src, const SRDWORD "
                         "*ixTable, const SRDWORD count)";
    command_names[148] = "_srCollectPos (SRDWORD* dst, const float *src,  const SRDWORD count)";
    command_names[149] = "_srCollectNeg (SRDWORD* dst, const float *src,  const SRDWORD count)";
    command_names[150] =
        "_srCollectNonZero (SRDWORD* dst, const SRBYTE* src,  const SRDWORD count)";
    command_names[151] = "_srRemapInverse   (SRDWORD* dst, const SRDWORD* map, const SRDWORD n)";
    command_names[152] =
        "_srDirect3DConvertColor (SRDWORD* dst, const srVector4* src, const SRDWORD n)";
    command_names[153] = "_transformIndexed (srVector3* dst, const srVector3* src, const SRDWORD* "
                         "ixTable, const srMatrix4& m, const SRDWORD n)";
    command_names[154] = "_transformIndexed (srVector4* dst, const srVector3* src, const SRDWORD* "
                         "ixTable, const srMatrix4& m, const SRDWORD n)";
    command_names[155] = "_dotIndexed  (float* dest, const srVector4& cv, const srVector4* vs, "
                         "const SRDWORD* ixTable, const SRDWORD n)";
    command_names[156] =
        "_dot  (float* dest, const srVector4& cv, const srVector3* vs, const SRDWORD n)";
    command_names[157] = "_copy  (srVector2* dest, const srVector2& cv, const SRDWORD n)";
    command_names[158] =
        "_copyIndexed (srVector2* dest, const srVector2* src, const SRDWORD* indices, SRDWORD n)";
    command_names[159] =
        "_div  (srVector2* dest, const srVector2* vs, const float* fs, const SRDWORD n)";
    command_names[160] =
        "_srCullNoClip (SRDWORD* dest, const srVector4& cv, const srVector4* vs, const SRDWORD n)";
    command_names[161] = "_srFloatToLinear (SRDWORD* d, const float* s, const SRDWORD n)";
    command_names[162] = "_srLinearToFloat (float* d, const SRDWORD* s, const SRDWORD n)";
    command_names[165] = "_srGetClipFlags\t(SRBYTE* d, const srVector4* s, const SRDWORD n)";
}

// FUNCTION: SURRENDER 0x1006A290
void srDebugVP::resetInternalStatistics()
{
    int command;

    for (command = 0; command < 0xa6; ++command) {
        call_times_450[command] = 0.0;
        call_counts_eb0[command] = 0;
        element_counts_980[command] = 0.0;
        misaligned8_1148[command] = 0;
        misaligned16_13e0[command] = 0;
    }
}

// FUNCTION: SURRENDER 0x1006A2D0
srDebugVP::ScopeTimer::ScopeTimer(srDebugVP* owner, SRDWORD elements, int index,
                                  const void* pointer_0, const void* pointer_1,
                                  const void* pointer_2, const void* pointer_3)
{
    elements_00 = elements;
    owner_04 = owner;
    index_08 = index;
    if (owner->check_misalignments_440 != 0) {
        /* reinterpret-ok: the forwarded pointer arguments are OR-ed together
           so a single mask reports any address that is not 8- or 16-byte
           aligned. */
        unsigned long mask = reinterpret_cast<unsigned long>(pointer_0) |
                             reinterpret_cast<unsigned long>(pointer_1) |
                             reinterpret_cast<unsigned long>(pointer_2) |
                             reinterpret_cast<unsigned long>(pointer_3);
        if ((mask & 7) != 0) {
            ++owner->misaligned8_1148[index];
        }
        if ((mask & 0xf) != 0) {
            ++owner->misaligned16_13e0[index];
        }
    }
    start_10 = srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT);
}

// FUNCTION: SURRENDER 0x1006A340
srDebugVP::ScopeTimer::~ScopeTimer()
{
    owner_04->call_times_450[index_08] +=
        srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT) - start_10;
    ++owner_04->call_counts_eb0[index_08];
    owner_04->element_counts_980[index_08] += elements_00;
}

// FUNCTION: SURRENDER 0x1006A3C0
const char* srDebugVP::getName()
{
    return "srDebugVP";
}

// FUNCTION: SURRENDER 0x1006A3D0
int srDebugVP::_memcmp(const void* source_0, const void* source_1, SRDWORD bytes)
{
    ScopeTimer scope_timer(this, bytes, 1, source_0, source_1, 0, 0);
    return processor_444->_memcmp(source_0, source_1, bytes);
}

// FUNCTION: SURRENDER 0x1006A450
void srDebugVP::_memcopy(void* destination, const void* source, SRDWORD bytes)
{
    ScopeTimer scope_timer(this, bytes, 2, destination, source, 0, 0);
    processor_444->_memcopy(destination, source, bytes);
}

// FUNCTION: SURRENDER 0x1006A4D0
void srDebugVP::_memcopy(void* destination, int source, SRDWORD bytes)
{
    ScopeTimer scope_timer(this, bytes, 3, destination, 0, 0, 0);
    processor_444->_memcopy(destination, source, bytes);
}

// FUNCTION: SURRENDER 0x1006A550
void srDebugVP::_prefetch(const void* destination, SRDWORD bytes, SRDWORD value_014)
{
    ScopeTimer scope_timer(this, bytes >> 5, 4, destination, 0, 0, 0);
    processor_444->_prefetch(destination, bytes, value_014);
}

// FUNCTION: SURRENDER 0x1006A5D0
void srDebugVP::_copyInterleaved(void* destination, const void* source, SRDWORD destination_pitch,
                                 SRDWORD source_pitch, SRDWORD width, SRDWORD count)
{
    ScopeTimer scope_timer(this, width * count, 5, destination, source, 0, 0);
    processor_444->_copyInterleaved(destination, source, destination_pitch, source_pitch, width,
                                    count);
}

// FUNCTION: SURRENDER 0x1006A660
void srDebugVP::_swap(void* first, void* second, SRDWORD bytes)
{
    ScopeTimer scope_timer(this, bytes, 6, first, second, 0, 0);
    processor_444->_swap(first, second, bytes);
}

// FUNCTION: SURRENDER 0x1006A6E0
void srDebugVP::_copy(SRDWORD* destination, SRDWORD constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 7, destination, 0, 0, 0);
    processor_444->_copy(destination, constant, count);
}

// FUNCTION: SURRENDER 0x1006A760
void srDebugVP::_reverse(SRDWORD* destination, const SRDWORD* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 8, destination, source, 0, 0);
    processor_444->_reverse(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006A7E0
void srDebugVP::_and(SRDWORD* destination, const SRDWORD* source, SRDWORD constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 9, destination, source, 0, 0);
    processor_444->_and(destination, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006A860
void srDebugVP::_or(SRDWORD* destination, const SRDWORD* source, SRDWORD constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 10, destination, source, 0, 0);
    processor_444->_or(destination, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006A8E0
void srDebugVP::_xor(SRDWORD* destination, const SRDWORD* source, SRDWORD constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 11, destination, source, 0, 0);
    processor_444->_xor(destination, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006A960
void srDebugVP::_and(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 12, destination, source_0, source_1, 0);
    processor_444->_and(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006A9F0
void srDebugVP::_or(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                    SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 13, destination, source_0, source_1, 0);
    processor_444->_or(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006AA80
void srDebugVP::_xor(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 14, destination, source_0, source_1, 0);
    processor_444->_xor(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006AB10
void srDebugVP::_asr(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 15, destination, source, 0, 0);
    processor_444->_asr(destination, source, shift, count);
}

// FUNCTION: SURRENDER 0x1006AB90
void srDebugVP::_asrAnd(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD mask,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 16, destination, source, 0, 0);
    processor_444->_asrAnd(destination, source, shift, mask, count);
}

// FUNCTION: SURRENDER 0x1006AC20
void srDebugVP::_lsr(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 17, destination, source, 0, 0);
    processor_444->_lsr(destination, source, shift, count);
}

// FUNCTION: SURRENDER 0x1006ACA0
void srDebugVP::_lsl(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 18, destination, source, 0, 0);
    processor_444->_lsl(destination, source, shift, count);
}

// FUNCTION: SURRENDER 0x1006AD20
void srDebugVP::_lslAnd(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD mask,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 19, destination, source, 0, 0);
    processor_444->_lslAnd(destination, source, shift, mask, count);
}

// FUNCTION: SURRENDER 0x1006ADB0
int srDebugVP::_isEqual(const SRDWORD* source, SRDWORD constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 20, source, 0, 0, 0);
    return processor_444->_isEqual(source, constant, count);
}

// FUNCTION: SURRENDER 0x1006AE30
int srDebugVP::_isEqual(const SRDWORD* source_0, const SRDWORD* source_1, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 21, source_0, source_1, 0, 0);
    return processor_444->_isEqual(source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006AEB0
SRDWORD srDebugVP::_max(const SRDWORD* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 22, source, 0, 0, 0);
    return processor_444->_min(source, count);
}

// FUNCTION: SURRENDER 0x1006AF30
SRDWORD srDebugVP::_min(const SRDWORD* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 23, source, 0, 0, 0);
    return processor_444->_max(source, count);
}

// FUNCTION: SURRENDER 0x1006AFB0
void srDebugVP::_copyIndexed(SRDWORD* destination, const SRDWORD* source, const SRDWORD* indices,
                             SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 24, destination, source, 0, 0);
    processor_444->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006B040
void srDebugVP::_addS(SRBYTE* destination, const SRBYTE* source, SRBYTE constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 25, destination, source, 0, 0);
    processor_444->_addS(destination, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006B0D0
void srDebugVP::_subS(SRBYTE* destination, const SRBYTE* source, SRBYTE constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 26, destination, source, 0, 0);
    processor_444->_subS(destination, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006B160
void srDebugVP::_subS(SRBYTE* destination, SRBYTE constant, const SRBYTE* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 27, destination, source, 0, 0);
    processor_444->_subS(destination, constant, source, count);
}

// FUNCTION: SURRENDER 0x1006B1F0
void srDebugVP::_addS(SRBYTE* destination, const SRBYTE* source_0, const SRBYTE* source_1,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 28, destination, source_0, source_1, 0);
    processor_444->_addS(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B280
void srDebugVP::_subS(SRBYTE* destination, const SRBYTE* source_0, const SRBYTE* source_1,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 29, destination, source_0, source_1, 0);
    processor_444->_subS(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B310
void srDebugVP::_toFloat(float* destination, const SRBYTE* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 30, destination, destination, source, 0);
    processor_444->_toFloat(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006B390
void srDebugVP::_add(float* destination, float constant, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 31, destination, source, 0, 0);
    processor_444->_add(destination, constant, source, count);
}

// FUNCTION: SURRENDER 0x1006B420
void srDebugVP::_sub(float* destination, float constant, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 32, destination, source, 0, 0);
    processor_444->_sub(destination, constant, source, count);
}

// FUNCTION: SURRENDER 0x1006B4B0
void srDebugVP::_mul(float* destination, float constant, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 33, destination, source, 0, 0);
    processor_444->_mul(destination, constant, source, count);
}

// FUNCTION: SURRENDER 0x1006B540
void srDebugVP::_div(float* destination, float constant, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 34, destination, source, 0, 0);
    processor_444->_div(destination, constant, source, count);
}

// FUNCTION: SURRENDER 0x1006B5D0
void srDebugVP::_add(float* destination, const float* source_0, const float* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 35, destination, source_0, source_1, 0);
    processor_444->_add(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B660
void srDebugVP::_sub(float* destination, const float* source_0, const float* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 36, destination, source_0, source_1, 0);
    processor_444->_sub(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B6F0
void srDebugVP::_mul(float* destination, const float* source_0, const float* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 37, destination, source_0, source_1, 0);
    processor_444->_mul(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B780
void srDebugVP::_div(float* destination, const float* source_0, const float* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 38, destination, source_0, source_1, 0);
    processor_444->_div(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B810
void srDebugVP::_mul(float* destination, float constant, const float* source_0,
                     const float* source_1, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 39, destination, source_0, source_1, 0);
    processor_444->_mul(destination, constant, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006B8A0
void srDebugVP::_clamp(float* destination, const float* source, float minimum, float maximum,
                       SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 40, destination, source, 0, 0);
    processor_444->_clamp(destination, source, minimum, maximum, count);
}

// FUNCTION: SURRENDER 0x1006B930
void srDebugVP::_clampMin(float* destination, const float* source, float minimum, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 41, destination, source, 0, 0);
    processor_444->_clampMin(destination, source, minimum, count);
}

// FUNCTION: SURRENDER 0x1006B9C0
void srDebugVP::_clampMax(float* destination, const float* source, float maximum, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 42, destination, source, 0, 0);
    processor_444->_clampMax(destination, source, maximum, count);
}

// FUNCTION: SURRENDER 0x1006BA50
void srDebugVP::_clampUnit(float* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 43, destination, source, 0, 0);
    processor_444->_clampUnit(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006BAD0
void srDebugVP::_sqrt(float* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 44, destination, source, 0, 0);
    processor_444->_sqrt(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006BB50
void srDebugVP::_isqrt(float* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 45, destination, source, 0, 0);
    processor_444->_isqrt(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006BBD0
void srDebugVP::_lerp(float* destination, const float* target, const float* source, float constant,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 46, destination, target, source, 0);
    processor_444->_lerp(destination, target, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006BC60
int srDebugVP::_isNeg(const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 47, source, 0, 0, 0);
    return processor_444->_isNeg(source, count);
}

// FUNCTION: SURRENDER 0x1006BCE0
int srDebugVP::_isPos(const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 48, source, 0, 0, 0);
    return processor_444->_isPos(source, count);
}

// FUNCTION: SURRENDER 0x1006BD60
int srDebugVP::_isZero(const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 49, source, 0, 0, 0);
    return processor_444->_isZero(source, count);
}

// FUNCTION: SURRENDER 0x1006BDE0
float srDebugVP::_min(const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 50, source, 0, 0, 0);
    return processor_444->_min(source, count);
}

// FUNCTION: SURRENDER 0x1006BE60
float srDebugVP::_max(const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 51, source, 0, 0, 0);
    return processor_444->_max(source, count);
}

// FUNCTION: SURRENDER 0x1006BEE0
void srDebugVP::_minMax(const float* source, float& minimum, float& maximum, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 52, source, 0, 0, 0);
    processor_444->_minMax(source, minimum, maximum, count);
}

// FUNCTION: SURRENDER 0x1006BF60
double srDebugVP::_sum(const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 53, source, 0, 0, 0);
    return processor_444->_sum(source, count);
}

// FUNCTION: SURRENDER 0x1006BFE0
void srDebugVP::_axpy(float* destination, float add_constant, float multiply_constant,
                      const float* multiply_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 54, destination, multiply_source, 0, 0);
    processor_444->_axpy(destination, add_constant, multiply_constant, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006C070
void srDebugVP::_axpy(float* destination, float add_constant, const float* scale_source,
                      const float* multiply_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 55, destination, scale_source, multiply_source, 0);
    processor_444->_axpy(destination, add_constant, scale_source, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006C100
void srDebugVP::_axpy(float* destination, const float* add_source, float multiply_constant,
                      const float* multiply_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 56, destination, add_source, multiply_source, 0);
    processor_444->_axpy(destination, add_source, multiply_constant, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006C190
void srDebugVP::_axpy(float* destination, const float* add_source, const float* scale_source,
                      const float* multiply_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 57, destination, add_source, scale_source, multiply_source);
    processor_444->_axpy(destination, add_source, scale_source, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006C220
void srDebugVP::_axpy(float* destination, float add_constant, float scale,
                      const float* scale_source, const float* multiply_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 58, destination, scale_source, multiply_source, 0);
    processor_444->_axpy(destination, add_constant, scale, scale_source, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006C2B0
void srDebugVP::_axpy(float* destination, const float* add_source, float scale,
                      const float* scale_source, const float* multiply_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 59, destination, add_source, scale_source, multiply_source);
    processor_444->_axpy(destination, add_source, scale, scale_source, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006C350
void srDebugVP::_mulIndexed(float* destination, const float* linear_source,
                            const float* indexed_source, const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 60, destination, linear_source, indexed_source, indices);
    processor_444->_mulIndexed(destination, linear_source, indexed_source, indices, count);
}

// FUNCTION: SURRENDER 0x1006C3E0
void srDebugVP::_mulIndexed(float* destination, float constant, const float* indexed_source,
                            const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 61, destination, indexed_source, indices, 0);
    processor_444->_mulIndexed(destination, constant, indexed_source, indices, count);
}

// FUNCTION: SURRENDER 0x1006C470
void srDebugVP::_toInt(SRLONG* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 62, destination, source, 0, 0);
    processor_444->_toInt(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006C4F0
void srDebugVP::_invPoly(float* destination, const float* source, const srVector3& poly,
                         SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 63, destination, source, 0, 0);
    processor_444->_invPoly(destination, source, poly, count);
}

// FUNCTION: SURRENDER 0x1006C580
void srDebugVP::_abs(float* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 64, destination, source, 0, 0);
    processor_444->_abs(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006C600
void srDebugVP::_neg(float* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 65, destination, source, 0, 0);
    processor_444->_neg(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006C680
void srDebugVP::_cubic(float* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 66, destination, source, 0, 0);
    processor_444->_cubic(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006C700
void srDebugVP::_copy(srVector2* destination, const srVector2& constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 157, destination, 0, 0, 0);
    processor_444->_copy(destination, constant, count);
}

// FUNCTION: SURRENDER 0x1006C780
void srDebugVP::_copyIndexed(srVector2* destination, const srVector2* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 158, destination, source, 0, 0);
    processor_444->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006C810
void srDebugVP::_div(srVector2* destination, const srVector2* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 159, destination, vector_source, float_source, 0);
    processor_444->_div(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006C8A0
void srDebugVP::_copy(srVector3* destination, const srVector3& constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 67, destination, 0, 0, 0);
    processor_444->_copy(destination, constant, count);
}

// FUNCTION: SURRENDER 0x1006C920
void srDebugVP::_copy(srVector3* destination, const srVector4* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 68, destination, source, 0, 0);
    processor_444->_copy(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006C9A0
void srDebugVP::_add(srVector3* destination, const srVector3& constant,
                     const srVector3* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 69, destination, vector_source, 0, 0);
    processor_444->_add(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006CA30
void srDebugVP::_sub(srVector3* destination, const srVector3& constant,
                     const srVector3* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 70, destination, vector_source, 0, 0);
    processor_444->_sub(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006CAC0
void srDebugVP::_mul(srVector3* destination, const srVector3& constant,
                     const srVector3* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 71, destination, vector_source, 0, 0);
    processor_444->_mul(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006CB50
void srDebugVP::_div(srVector3* destination, const srVector3& constant,
                     const srVector3* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 72, destination, vector_source, 0, 0);
    processor_444->_div(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006CBE0
void srDebugVP::_add(srVector3* destination, const srVector3& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 73, destination, float_source, 0, 0);
    processor_444->_add(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CC70
void srDebugVP::_sub(srVector3* destination, const srVector3& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 74, destination, float_source, 0, 0);
    processor_444->_sub(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CD00
void srDebugVP::_mul(srVector3* destination, const srVector3& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 75, destination, float_source, 0, 0);
    processor_444->_mul(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CD90
void srDebugVP::_div(srVector3* destination, const srVector3& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 76, destination, float_source, 0, 0);
    processor_444->_div(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CE20
void srDebugVP::_add(srVector3* destination, const srVector3* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 77, destination, vector_source, float_source, 0);
    processor_444->_add(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CEB0
void srDebugVP::_sub(srVector3* destination, const srVector3* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 78, destination, vector_source, float_source, 0);
    processor_444->_sub(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CF40
void srDebugVP::_mul(srVector3* destination, const srVector3* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 79, destination, vector_source, float_source, 0);
    processor_444->_mul(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006CFD0
void srDebugVP::_div(srVector3* destination, const srVector3* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 80, destination, vector_source, float_source, 0);
    processor_444->_div(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006D060
void srDebugVP::_sub(srVector3* destination, const float* float_source,
                     const srVector3* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 81, destination, float_source, vector_source, 0);
    processor_444->_sub(destination, float_source, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006D0F0
void srDebugVP::_div(srVector3* destination, const float* float_source,
                     const srVector3* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 82, destination, float_source, vector_source, 0);
    processor_444->_div(destination, float_source, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006D180
void srDebugVP::_dot(float* destination, const srVector3& constant, const srVector3* vectors,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 83, destination, vectors, 0, 0);
    processor_444->_dot(destination, constant, vectors, count);
}

// FUNCTION: SURRENDER 0x1006D210
void srDebugVP::_dot(float* destination, const srVector3* vectors_0, const srVector3* vectors_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 84, destination, vectors_0, vectors_1, 0);
    processor_444->_dot(destination, vectors_0, vectors_1, count);
}

// FUNCTION: SURRENDER 0x1006D2A0
void srDebugVP::_cross(srVector3* destination, const srVector3* vectors_0,
                       const srVector3* vectors_1, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 85, destination, vectors_0, vectors_1, 0);
    processor_444->_cross(destination, vectors_0, vectors_1, count);
}

// FUNCTION: SURRENDER 0x1006D330
void srDebugVP::_length(float* destination, const srVector3* vectors, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 86, destination, vectors, 0, 0);
    processor_444->_length(destination, vectors, count);
}

// FUNCTION: SURRENDER 0x1006D3B0
void srDebugVP::_normalize(srVector3* destination, const srVector3* vectors, float length,
                           SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 87, destination, vectors, 0, 0);
    processor_444->_normalize(destination, vectors, length, count);
}

// FUNCTION: SURRENDER 0x1006D440
void srDebugVP::_minMax(const srVector3* source, srVector3& minimum, srVector3& maximum,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 88, source, 0, 0, 0);
    processor_444->_minMax(source, minimum, maximum, count);
}

// FUNCTION: SURRENDER 0x1006D4C0
void srDebugVP::_transform(srVector3* destination, const srVector3* vectors,
                           const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 89, destination, vectors, 0, 0);
    processor_444->_transform(destination, vectors, matrix, count);
}

// FUNCTION: SURRENDER 0x1006D550
void srDebugVP::_copyIndexed(srVector3* destination, const srVector2* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 90, destination, source, indices, 0);
    processor_444->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006D5E0
void srDebugVP::_copyIndexed(srVector3* destination, const srVector3* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 91, destination, source, indices, 0);
    processor_444->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006D670
void srDebugVP::_copyIndexed(srVector3* destination, const srVector4* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 92, destination, source, indices, 0);
    processor_444->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006D700
void srDebugVP::_mulIndexed(srVector3* destination, const srVector3* linear_source,
                            const srVector3* indexed_source, const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 93, destination, linear_source, indexed_source, indices);
    processor_444->_mulIndexed(destination, linear_source, indexed_source, indices, count);
}

// FUNCTION: SURRENDER 0x1006D790
void srDebugVP::_mulIndexed(srVector3* destination, const srVector3& constant,
                            const srVector3* indexed_source, const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 94, destination, indexed_source, indices, 0);
    processor_444->_mulIndexed(destination, constant, indexed_source, indices, count);
}

// FUNCTION: SURRENDER 0x1006D820
void srDebugVP::_dir(srVector3* destination, float* lengths, const srVector3* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 95, destination, lengths, source, 0);
    processor_444->_dir(destination, lengths, source, count);
}

// FUNCTION: SURRENDER 0x1006D8B0
void srDebugVP::_dir(srVector3* destination, float* lengths, const srVector4* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 96, destination, lengths, source, 0);
    processor_444->_dir(destination, lengths, source, count);
}

// FUNCTION: SURRENDER 0x1006D940
void srDebugVP::_copy(srVector4* destination, const srVector4& constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 97, destination, 0, 0, 0);
    processor_444->_copy(destination, constant, count);
}

// FUNCTION: SURRENDER 0x1006D9C0
void srDebugVP::_copy(srVector4* destination, const srVector3* source, float constant,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 98, destination, source, 0, 0);
    processor_444->_copy(destination, source, constant, count);
}

// FUNCTION: SURRENDER 0x1006DA40
void srDebugVP::_copy(srVector4* destination, const srVector3* source_0, const float* source_1,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 99, destination, source_0, source_1, 0);
    processor_444->_copy(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006DAD0
void srDebugVP::_copyW(srVector4* destination, float constant, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 100, destination, 0, 0, 0);
    processor_444->_copyW(destination, constant, count);
}

// FUNCTION: SURRENDER 0x1006DB50
void srDebugVP::_copyW(srVector4* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 101, destination, source, 0, 0);
    processor_444->_copyW(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006DBD0
void srDebugVP::_copyW(float* destination, const srVector4* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 102, destination, source, 0, 0);
    processor_444->_copyW(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006DC50
void srDebugVP::_add(srVector4* destination, const srVector4& constant,
                     const srVector4* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 103, destination, vector_source, 0, 0);
    processor_444->_add(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006DCE0
void srDebugVP::_sub(srVector4* destination, const srVector4& constant,
                     const srVector4* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 104, destination, vector_source, 0, 0);
    processor_444->_sub(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006DD70
void srDebugVP::_mul(srVector4* destination, const srVector4& constant,
                     const srVector4* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 105, destination, vector_source, 0, 0);
    processor_444->_mul(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006DE00
void srDebugVP::_div(srVector4* destination, const srVector4& constant,
                     const srVector4* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 106, destination, vector_source, 0, 0);
    processor_444->_div(destination, constant, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006DE90
void srDebugVP::_add(srVector4* destination, const srVector4& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 107, destination, float_source, 0, 0);
    processor_444->_add(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006DF20
void srDebugVP::_sub(srVector4* destination, const srVector4& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 108, destination, float_source, 0, 0);
    processor_444->_sub(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006DFB0
void srDebugVP::_mul(srVector4* destination, const srVector4& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 109, destination, float_source, 0, 0);
    processor_444->_mul(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006E040
void srDebugVP::_div(srVector4* destination, const srVector4& constant, const float* float_source,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 110, destination, float_source, 0, 0);
    processor_444->_div(destination, constant, float_source, count);
}

// FUNCTION: SURRENDER 0x1006E0D0
void srDebugVP::_add(srVector4* destination, const srVector4* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 111, destination, vector_source, float_source, 0);
    processor_444->_add(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006E160
void srDebugVP::_sub(srVector4* destination, const srVector4* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 112, destination, vector_source, float_source, 0);
    processor_444->_sub(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006E1F0
void srDebugVP::_mul(srVector4* destination, const srVector4* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 113, destination, vector_source, float_source, 0);
    processor_444->_mul(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006E280
void srDebugVP::_div(srVector4* destination, const srVector4* vector_source,
                     const float* float_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 114, destination, vector_source, float_source, 0);
    processor_444->_div(destination, vector_source, float_source, count);
}

// FUNCTION: SURRENDER 0x1006E310
void srDebugVP::_sub(srVector4* destination, const float* float_source,
                     const srVector4* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 115, destination, float_source, vector_source, 0);
    processor_444->_sub(destination, float_source, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006E3A0
void srDebugVP::_div(srVector4* destination, const float* float_source,
                     const srVector4* vector_source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 116, destination, float_source, vector_source, 0);
    processor_444->_div(destination, float_source, vector_source, count);
}

// FUNCTION: SURRENDER 0x1006E430
void srDebugVP::_dot(float* destination, const srVector4& constant, const srVector4* vectors,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 117, vectors, 0, 0, 0);
    processor_444->_dot(destination, constant, vectors, count);
}

// FUNCTION: SURRENDER 0x1006E4B0
void srDebugVP::_dot(float* destination, const srVector4* vectors_0, const srVector4* vectors_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 118, vectors_0, vectors_1, 0, 0);
    processor_444->_dot(destination, vectors_0, vectors_1, count);
}

// FUNCTION: SURRENDER 0x1006E540
void srDebugVP::_length(float* destination, const srVector4* vectors, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 119, destination, vectors, 0, 0);
    processor_444->_length(destination, vectors, count);
}

// FUNCTION: SURRENDER 0x1006E5C0
void srDebugVP::_normalize(srVector4* destination, const srVector4* vectors, float length,
                           SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 120, destination, vectors, 0, 0);
    processor_444->_normalize(destination, vectors, length, count);
}

// FUNCTION: SURRENDER 0x1006E650
void srDebugVP::_minMax(const srVector4* source, srVector4& minimum, srVector4& maximum,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 121, source, 0, 0, 0);
    processor_444->_minMax(source, minimum, maximum, count);
}

// FUNCTION: SURRENDER 0x1006E6D0
void srDebugVP::_transform(srVector4* destination, const srVector4* vectors,
                           const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 122, destination, vectors, 0, 0);
    processor_444->_transform(destination, vectors, matrix, count);
}

// FUNCTION: SURRENDER 0x1006E760
void srDebugVP::_transform(srVector4* destination, const srVector3* vectors,
                           const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 123, destination, vectors, 0, 0);
    processor_444->_transform(destination, vectors, matrix, count);
}

// FUNCTION: SURRENDER 0x1006E7F0
void srDebugVP::_transformOrtho(srVector4* destination, const srVector4* source,
                                const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 124, destination, source, 0, 0);
    processor_444->_transformOrtho(destination, source, matrix, count);
}

// FUNCTION: SURRENDER 0x1006E880
void srDebugVP::_transformPerspective(srVector4* destination, const srVector4* source,
                                      const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 125, destination, source, 0, 0);
    processor_444->_transformPerspective(destination, source, matrix, count);
}

// FUNCTION: SURRENDER 0x1006E910
void srDebugVP::_axpy(srVector4* destination, const srVector4& add_constant,
                      const srVector4& multiply_constant, const float* multiply_source,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 126, destination, multiply_source, 0, 0);
    processor_444->_axpy(destination, add_constant, multiply_constant, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006E9A0
void srDebugVP::_axpy(srVector4* destination, const srVector4& add_constant,
                      const srVector4* multiply_vectors, const float* multiply_source,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 127, destination, multiply_vectors, multiply_source, 0);
    processor_444->_axpy(destination, add_constant, multiply_vectors, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006EA30
void srDebugVP::_axpy(srVector4* destination, const srVector4* add_source,
                      const srVector4& multiply_constant, const float* multiply_source,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 128, destination, add_source, multiply_source, 0);
    processor_444->_axpy(destination, add_source, multiply_constant, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006EAC0
void srDebugVP::_axpy(srVector4* destination, const srVector4* add_source,
                      const srVector4* multiply_vectors, const float* multiply_source,
                      SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 129, destination, add_source, multiply_vectors,
                           multiply_source);
    processor_444->_axpy(destination, add_source, multiply_vectors, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006EB50
void srDebugVP::_axpy(srVector4* destination, const srVector4& add_constant,
                      const srVector4& multiply_constant, const float* multiply_source_0,
                      const float* multiply_source_1, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 130, destination, multiply_source_0, multiply_source_1, 0);
    processor_444->_axpy(destination, add_constant, multiply_constant, multiply_source_0,
                         multiply_source_1, count);
}

// FUNCTION: SURRENDER 0x1006EBF0
void srDebugVP::_axpy(srVector4* destination, const srVector4* add_source,
                      const srVector4& multiply_constant, const float* multiply_source_0,
                      const float* multiply_source_1, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 131, destination, add_source, multiply_source_0,
                           multiply_source_1);
    processor_444->_axpy(destination, add_source, multiply_constant, multiply_source_0,
                         multiply_source_1, count);
}

// FUNCTION: SURRENDER 0x1006EC90
void srDebugVP::_mulAdd(srVector4* destination, const srVector4& add_constant,
                        const srVector4& multiply_constant, const srVector4* multiply_source,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 132, destination, multiply_source, 0, 0);
    processor_444->_mulAdd(destination, add_constant, multiply_constant, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006ED20
void srDebugVP::_mulAdd(srVector4* destination, const srVector4* add_source,
                        const srVector4& multiply_constant, const srVector4* multiply_source,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 133, destination, add_source, multiply_source, 0);
    processor_444->_mulAdd(destination, add_source, multiply_constant, multiply_source, count);
}

// FUNCTION: SURRENDER 0x1006EDB0
void srDebugVP::_mulAdd(srVector4* destination, const srVector4& add_constant,
                        const srVector4* multiply_source_0, const srVector4* multiply_source_1,
                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 134, destination, multiply_source_0, multiply_source_1, 0);
    processor_444->_mulAdd(destination, add_constant, multiply_source_0, multiply_source_1, count);
}

// FUNCTION: SURRENDER 0x1006EE40
void srDebugVP::_divByW(srVector4* destination, const srVector4* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 135, destination, source, 0, 0);
    processor_444->_divByW(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006EEC0
void srDebugVP::_copyIndexed(srVector4* destination, const srVector2* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 136, destination, source, indices, 0);
    processor_444->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006EF50
void srDebugVP::_copyIndexed(srVector4* destination, const srVector3* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 137, destination, source, indices, 0);
    processor_444->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006EFE0
void srDebugVP::_copyIndexed(srVector4* destination, const srVector4* source,
                             const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 138, destination, source, indices, 0);
    processor_444->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006F070
void srDebugVP::_copyIndexed(srVector4* destination, const srARGB* source, const SRDWORD* indices,
                             SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 139, destination, source, indices, 0);
    processor_444->_copyIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006F100
void srDebugVP::_mulIndexed(srVector4* destination, const srVector4* linear_source,
                            const srVector4* indexed_source, const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 140, destination, indexed_source, indices, 0);
    processor_444->_mulIndexed(destination, linear_source, indexed_source, indices, count);
}

// FUNCTION: SURRENDER 0x1006F190
void srDebugVP::_mulIndexed(srVector4* destination, const srVector4& constant,
                            const srVector4* indexed_source, const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 141, destination, &constant, indexed_source, indices);
    processor_444->_mulIndexed(destination, constant, indexed_source, indices, count);
}

// FUNCTION: SURRENDER 0x1006F220
void srDebugVP::_mul(srMatrix4& destination, const srMatrix4& source_0, const srMatrix4& source_1)
{
    ScopeTimer scope_timer(this, 1, 142, 0, 0, 0, 0);
    processor_444->_mul(destination, source_0, source_1);
}

// FUNCTION: SURRENDER 0x1006F2A0
void srDebugVP::_mul(srMatrix4* destination, const srMatrix4* source_0, const srMatrix4* source_1,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 143, destination, source_0, source_1, 0);
    processor_444->_mul(destination, source_0, source_1, count);
}

// FUNCTION: SURRENDER 0x1006F330
int srDebugVP::_srTestBoundingBox(const srMatrix4& matrix, const srVector3& minimum,
                                  const srVector3& maximum)
{
    ScopeTimer scope_timer(this, 1, 144, 0, 0, 0, 0);
    return processor_444->_srTestBoundingBox(matrix, minimum, maximum);
}

// FUNCTION: SURRENDER 0x1006F3B0
void srDebugVP::_srSpecularPow(float* destination, const float* source, float exponent,
                               SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 145, destination, source, 0, 0);
    processor_444->_srSpecularPow(destination, source, exponent, count);
}

// FUNCTION: SURRENDER 0x1006F440
void srDebugVP::_srCopyIndexedRemap(srVector3i* destination, const srVector3i* source,
                                    const SRDWORD* indices, const SRDWORD* remap, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 146, 0, 0, 0, 0);
    processor_444->_srCopyIndexedRemap(destination, source, indices, remap, count);
}

// FUNCTION: SURRENDER 0x1006F4D0
void srDebugVP::_srSetIndexed(SRBYTE* destination, const srVector3i* source, const SRDWORD* indices,
                              SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 147, destination, 0, 0, 0);
    processor_444->_srSetIndexed(destination, source, indices, count);
}

// FUNCTION: SURRENDER 0x1006F560
SRDWORD srDebugVP::_srCollectPos(SRDWORD* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 148, destination, source, 0, 0);
    return processor_444->_srCollectPos(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006F5F0
SRDWORD srDebugVP::_srCollectNeg(SRDWORD* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 149, destination, source, 0, 0);
    return processor_444->_srCollectNeg(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006F680
SRDWORD srDebugVP::_srCollectNonZero(SRDWORD* destination, const SRBYTE* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 150, destination, source, 0, 0);
    return processor_444->_srCollectNonZero(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006F710
void srDebugVP::_srRemapInverse(SRDWORD* destination, const SRDWORD* map, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 151, destination, 0, 0, 0);
    processor_444->_srRemapInverse(destination, map, count);
}

// FUNCTION: SURRENDER 0x1006F790
void srDebugVP::_srDirect3DConvertColor(SRDWORD* destination, const srVector4* source,
                                        SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 152, destination, source, 0, 0);
    processor_444->_srDirect3DConvertColor(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006F810
void srDebugVP::_transformIndexed(srVector4* destination, const srVector3* source,
                                  const SRDWORD* indices, const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 154, source, 0, 0, 0);
    processor_444->_transformIndexed(destination, source, indices, matrix, count);
}

// FUNCTION: SURRENDER 0x1006F8A0
void srDebugVP::_transformIndexed(srVector3* destination, const srVector3* source,
                                  const SRDWORD* indices, const srMatrix4& matrix, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 153, 0, 0, 0, 0);
    processor_444->_transformIndexed(destination, source, indices, matrix, count);
}

// FUNCTION: SURRENDER 0x1006F930
void srDebugVP::_dotIndexed(float* destination, const srVector4& constant, const srVector4* vectors,
                            const SRDWORD* indices, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 155, vectors, 0, 0, 0);
    processor_444->_dotIndexed(destination, constant, vectors, indices, count);
}

// FUNCTION: SURRENDER 0x1006F9C0
void srDebugVP::_dot(float* destination, const srVector4& constant, const srVector3* vectors,
                     SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 156, destination, 0, 0, 0);
    processor_444->_dot(destination, constant, vectors, count);
}

// FUNCTION: SURRENDER 0x1006FA50
SRDWORD srDebugVP::_srCullNoClip(SRDWORD* destination, const srVector4& constant,
                                 const srVector4* vectors, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 160, vectors, 0, 0, 0);
    return processor_444->_srCullNoClip(destination, constant, vectors, count);
}

// FUNCTION: SURRENDER 0x1006FAE0
void srDebugVP::_srFloatToLinear(SRDWORD* destination, const float* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 161, destination, source, 0, 0);
    processor_444->_srFloatToLinear(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006FB60
void srDebugVP::_srLinearToFloat(float* destination, const SRDWORD* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 162, destination, source, 0, 0);
    processor_444->_srLinearToFloat(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006FBE0
void srDebugVP::unknown_2a0(SRDWORD arg0, SRDWORD arg1) {}

// FUNCTION: SURRENDER 0x1006FBF0
void srDebugVP::unknown_2a4() {}

// FUNCTION: SURRENDER 0x1006FC00
void srDebugVP::_srGetClipFlags(SRBYTE* destination, const srVector4* source, SRDWORD count)
{
    ScopeTimer scope_timer(this, count, 165, source, 0, 0, 0);
    processor_444->_srGetClipFlags(destination, source, count);
}

// FUNCTION: SURRENDER 0x1006FCA0
srDebugVP::~srDebugVP() {}
