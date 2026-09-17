#include "surrender/srVP_generic.h"

#include "surrender/srARGB.h"

#include <string.h>

#pragma intrinsic(memset)

// FUNCTION: SURRENDER 0x10065880
const char* srVP_generic::getName()
{
    return "Generic";
}

// FUNCTION: SURRENDER 0x10065890
void srVP_generic::unknown_2a0(SRDWORD arg0, SRDWORD arg1) {}

// FUNCTION: SURRENDER 0x100658A0
void srVP_generic::unknown_2a4() {}

// FUNCTION: SURRENDER 0x100658D0
srVP_generic::~srVP_generic() {}

// FUNCTION: SURRENDER 0x100658E0
void srVP_generic::_addS(SRBYTE* destination, const SRBYTE* source_0, const SRBYTE* source_1,
                         SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD sum = source_0[index] + source_1[index];
        destination[index] = sum > 0xff ? static_cast<SRBYTE>(-1) : static_cast<SRBYTE>(sum);
    }
}

// FUNCTION: SURRENDER 0x10065920
void srVP_generic::_addS(SRBYTE* destination, const SRBYTE* source, SRBYTE constant, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD sum = source[index] + constant;
        destination[index] = sum > 0xff ? static_cast<SRBYTE>(-1) : static_cast<SRBYTE>(sum);
    }
}

// FUNCTION: SURRENDER 0x10065960
void srVP_generic::_subS(SRBYTE* destination, const SRBYTE* source_0, const SRBYTE* source_1,
                         SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        int difference = source_0[index] - source_1[index];
        destination[index] = static_cast<SRBYTE>(difference < 0 ? 0 : difference);
    }
}

// FUNCTION: SURRENDER 0x100659A0
void srVP_generic::_subS(SRBYTE* destination, SRBYTE constant, const SRBYTE* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        int difference = constant - source[index];
        destination[index] = static_cast<SRBYTE>(difference < 0 ? 0 : difference);
    }
}

// FUNCTION: SURRENDER 0x100659E0
void srVP_generic::_subS(SRBYTE* destination, const SRBYTE* source, SRBYTE constant, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        int difference = source[index] - constant;
        destination[index] = static_cast<SRBYTE>(difference < 0 ? 0 : difference);
    }
}

// FUNCTION: SURRENDER 0x10065A20
void srVP_generic::_toFloat(float* destination, const SRBYTE* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source[index] * (1.0f / 255.0f);
    }
}

// FUNCTION: SURRENDER 0x10065A60
void srVP_generic::_copy(SRDWORD* destination, SRDWORD constant, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = constant;
    }
}

// FUNCTION: SURRENDER 0x10065A80
void srVP_generic::_and(SRDWORD* destination, const SRDWORD* source, SRDWORD constant,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source[index] & constant;
    }
}

// FUNCTION: SURRENDER 0x10065AB0
void srVP_generic::_or(SRDWORD* destination, const SRDWORD* source, SRDWORD constant, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source[index] | constant;
    }
}

// FUNCTION: SURRENDER 0x10065AE0
void srVP_generic::_xor(SRDWORD* destination, const SRDWORD* source, SRDWORD constant,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source[index] ^ constant;
    }
}

// FUNCTION: SURRENDER 0x10065B10
void srVP_generic::_and(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source_0[index] & source_1[index];
    }
}

// FUNCTION: SURRENDER 0x10065B40
void srVP_generic::_or(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                       SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source_0[index] | source_1[index];
    }
}

// FUNCTION: SURRENDER 0x10065B70
void srVP_generic::_xor(SRDWORD* destination, const SRDWORD* source_0, const SRDWORD* source_1,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source_0[index] ^ source_1[index];
    }
}

// FUNCTION: SURRENDER 0x10065BA0
int srVP_generic::_isEqual(const SRDWORD* source_0, const SRDWORD* source_1, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        if (source_0[index] != source_1[index]) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: SURRENDER 0x10065BE0
int srVP_generic::_isEqual(const SRDWORD* source, SRDWORD constant, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        if (source[index] != constant) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: SURRENDER 0x10065C10
void srVP_generic::_copyIndexed(SRDWORD* destination, const SRDWORD* source, const SRDWORD* indices,
                                SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index] = source[source_index];
    }
}

// FUNCTION: SURRENDER 0x10065C40
void srVP_generic::_swap(void* first, void* second, SRDWORD bytes)
{
    SRBYTE* first_bytes = static_cast<SRBYTE*>(first);
    SRBYTE* second_bytes = static_cast<SRBYTE*>(second);
    for (SRDWORD index = 0; index < bytes; ++index) {
        SRBYTE value = first_bytes[index];
        first_bytes[index] = second_bytes[index];
        second_bytes[index] = value;
    }
}

// FUNCTION: SURRENDER 0x10065C70
SRDWORD srVP_generic::_max(const SRDWORD* source, SRDWORD count)
{
    SRDWORD maximum = source[0];
    for (SRDWORD index = 0; index < count; ++index) {
        if (maximum < source[index]) {
            maximum = source[index];
        }
    }
    return maximum;
}

// FUNCTION: SURRENDER 0x10065CA0
SRDWORD srVP_generic::_min(const SRDWORD* source, SRDWORD count)
{
    SRDWORD minimum = source[0];
    for (SRDWORD index = 0; index < count; ++index) {
        if (source[index] < minimum) {
            minimum = source[index];
        }
    }
    return minimum;
}

// FUNCTION: SURRENDER 0x10065CD0
void srVP_generic::_reverse(SRDWORD* destination, const SRDWORD* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count / 2; ++index) {
        destination[index] = source[count - 1 - index];
        destination[count - 1 - index] = source[index];
    }
    if ((count & 1) != 0) {
        destination[count / 2] = source[count / 2];
    }
}

// FUNCTION: SURRENDER 0x10065D30
void srVP_generic::_asr(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = static_cast<SRLONG>(source[index]) >> shift;
    }
}

// FUNCTION: SURRENDER 0x10065D60
void srVP_generic::_asrAnd(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD mask,
                           SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = (static_cast<SRLONG>(source[index]) >> shift) & mask;
    }
}

// FUNCTION: SURRENDER 0x10065DA0
void srVP_generic::_lsr(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source[index] >> shift;
    }
}

// FUNCTION: SURRENDER 0x10065DD0
void srVP_generic::_lsl(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source[index] << shift;
    }
}

// FUNCTION: SURRENDER 0x10065E00
void srVP_generic::_lslAnd(SRDWORD* destination, const SRDWORD* source, SRDWORD shift, SRDWORD mask,
                           SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = (source[index] << shift) & mask;
    }
}

// FUNCTION: SURRENDER 0x10065E40
void srVP_generic::_axpy(float* destination, float add_constant, float multiply_constant,
                         const float* multiply_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = multiply_constant * multiply_source[index] + add_constant;
    }
}

// FUNCTION: SURRENDER 0x10065E70
void srVP_generic::_axpy(float* destination, const float* add_source, float multiply_constant,
                         const float* multiply_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = multiply_constant * multiply_source[index] + add_source[index];
    }
}

// FUNCTION: SURRENDER 0x10065EB0
void srVP_generic::_axpy(float* destination, float add_constant, const float* scale_source,
                         const float* multiply_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = scale_source[index] * multiply_source[index] + add_constant;
    }
}

// FUNCTION: SURRENDER 0x10065EF0
void srVP_generic::_axpy(float* destination, const float* add_source, const float* scale_source,
                         const float* multiply_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = scale_source[index] * multiply_source[index] + add_source[index];
    }
}

// FUNCTION: SURRENDER 0x10065F30
void srVP_generic::_axpy(float* destination, float add_constant, float scale,
                         const float* scale_source, const float* multiply_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = scale * scale_source[index] * multiply_source[index] + add_constant;
    }
}

// FUNCTION: SURRENDER 0x10065F70
void srVP_generic::_axpy(float* destination, const float* add_source, float scale,
                         const float* scale_source, const float* multiply_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] =
            scale_source[index] * multiply_source[index] * scale + add_source[index];
    }
}

// FUNCTION: SURRENDER 0x10065FB0
void srVP_generic::_add(float* destination, float constant, const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = constant + source[index];
    }
}

// FUNCTION: SURRENDER 0x10065FE0
void srVP_generic::_add(float* destination, const float* source_0, const float* source_1,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source_0[index] + source_1[index];
    }
}

// FUNCTION: SURRENDER 0x10066010
void srVP_generic::_sub(float* destination, float constant, const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = constant - source[index];
    }
}

// FUNCTION: SURRENDER 0x10066040
void srVP_generic::_sub(float* destination, const float* source_0, const float* source_1,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source_0[index] - source_1[index];
    }
}

// FUNCTION: SURRENDER 0x10066070
void srVP_generic::_toInt(SRLONG* destination, const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = static_cast<SRLONG>(source[index]);
    }
}

// FUNCTION: SURRENDER 0x100660A0
void srVP_generic::_mul(float* destination, float constant, const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = constant * source[index];
    }
}

// FUNCTION: SURRENDER 0x100660D0
void srVP_generic::_mul(float* destination, const float* source_0, const float* source_1,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source_0[index] * source_1[index];
    }
}

// FUNCTION: SURRENDER 0x10066100
void srVP_generic::_mul(float* destination, float constant, const float* source_0,
                        const float* source_1, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source_0[index] * source_1[index] * constant;
    }
}

// FUNCTION: SURRENDER 0x10066140
void srVP_generic::_sqrt(float* destination, const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = (float)sqrt(source[index]);
    }
}

// FUNCTION: SURRENDER 0x10066170
void srVP_generic::_isqrt(float* destination, const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = 1.0 / sqrt(source[index]);
    }
}

// FUNCTION: SURRENDER 0x100661A0
double srVP_generic::_sum(const float* source, SRDWORD count)
{
    double sum = 0.0;
    for (SRDWORD index = 0; index < count; ++index) {
        sum += source[index];
    }
    return sum;
}

// FUNCTION: SURRENDER 0x100661C0
void srVP_generic::_lerp(float* destination, const float* target, const float* source,
                         float constant, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = constant * target[index] + (1.0f - constant) * source[index];
    }
}

// FUNCTION: SURRENDER 0x10066210
void srVP_generic::_div(float* destination, float constant, const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = constant / source[index];
    }
}

// FUNCTION: SURRENDER 0x10066240
void srVP_generic::_div(float* destination, const float* source_0, const float* source_1,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source_0[index] / source_1[index];
    }
}

// FUNCTION: SURRENDER 0x10066270
void srVP_generic::_clampMin(float* destination, const float* source, float minimum, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source[index] <= minimum ? minimum : source[index];
    }
}

// FUNCTION: SURRENDER 0x100662B0
void srVP_generic::_clampMax(float* destination, const float* source, float maximum, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source[index] >= maximum ? maximum : source[index];
    }
}

// FUNCTION: SURRENDER 0x100662F0
void srVP_generic::_clampUnit(float* destination, const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] =
            source[index] < 0.0f ? 0.0f : (source[index] > 1.0f ? 1.0f : source[index]);
    }
}

// FUNCTION: SURRENDER 0x10066340
void srVP_generic::_clamp(float* destination, const float* source, float minimum, float maximum,
                          SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] =
            source[index] < minimum ? minimum : (source[index] > maximum ? maximum : source[index]);
    }
}

// FUNCTION: SURRENDER 0x10066390
void srVP_generic::_mulIndexed(float* destination, float constant, const float* indexed_source,
                               const SRDWORD* indices, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index] = constant * indexed_source[source_index];
    }
}

// FUNCTION: SURRENDER 0x100663C0
void srVP_generic::_mulIndexed(float* destination, const float* linear_source,
                               const float* indexed_source, const SRDWORD* indices, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index] = indexed_source[source_index] * linear_source[index];
    }
}

// FUNCTION: SURRENDER 0x10066400
float srVP_generic::_min(const float* source, SRDWORD count)
{
    float minimum = source[0];
    for (SRDWORD index = 0; index < count; ++index) {
        if (source[index] < minimum) {
            minimum = source[index];
        }
    }
    return minimum;
}

// FUNCTION: SURRENDER 0x10066430
float srVP_generic::_max(const float* source, SRDWORD count)
{
    float maximum = source[0];
    for (SRDWORD index = 0; index < count; ++index) {
        if (maximum < source[index]) {
            maximum = source[index];
        }
    }
    return maximum;
}

// FUNCTION: SURRENDER 0x10066460
void srVP_generic::_minMax(const float* source, float& minimum, float& maximum, SRDWORD count)
{
    minimum = source[0];
    maximum = source[0];
    for (SRDWORD index = 1; index < count; ++index) {
        if (source[index] < minimum) {
            minimum = source[index];
        } else if (source[index] > maximum) {
            maximum = source[index];
        }
    }
}

// FUNCTION: SURRENDER 0x100664B0
int srVP_generic::_isZero(const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        if (source[index] != 0.0f) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: SURRENDER 0x100664F0
int srVP_generic::_isNeg(const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        if (source[index] > 0.0f) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: SURRENDER 0x10066530
int srVP_generic::_isPos(const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        if (source[index] < 0.0f) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: SURRENDER 0x10066570
void srVP_generic::_invPoly(float* destination, const float* source, const srVector3& poly,
                            SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = 1.0f / ((poly.z * source[index] + poly.y) * source[index] + poly.x);
    }
}

// FUNCTION: SURRENDER 0x100665B0
void srVP_generic::_abs(float* destination, const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = (float)fabs(source[index]);
    }
}

// FUNCTION: SURRENDER 0x100665E0
void srVP_generic::_neg(float* destination, const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = -source[index];
    }
}

// FUNCTION: SURRENDER 0x10066610
void srVP_generic::_copy(srVector2* destination, const srVector2& constant, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = constant.x;
        destination[index].y = constant.y;
    }
}

// FUNCTION: SURRENDER 0x10066640
void srVP_generic::_copyIndexed(srVector2* destination, const srVector2* source,
                                const SRDWORD* indices, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = source[source_index].x;
        destination[index].y = source[source_index].y;
    }
}

// FUNCTION: SURRENDER 0x10066680
void srVP_generic::_div(srVector2* destination, const srVector2* vector_source,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vector_source[index].x / float_source[index];
        destination[index].y = vector_source[index].y / float_source[index];
    }
}

// FUNCTION: SURRENDER 0x100666D0
void srVP_generic::_minMax(const srVector3* source, srVector3& minimum, srVector3& maximum,
                           SRDWORD count)
{
    minimum = source[0];
    maximum = source[0];
    float* minimum_components = &minimum.x;
    float* maximum_components = &maximum.x;
    for (SRDWORD index = 1; index < count; ++index) {
        const float* components = &source[index].x;
        for (int component = 0; component < 3; ++component) {
            if (components[component] < minimum_components[component]) {
                minimum_components[component] = components[component];
            } else if (components[component] > maximum_components[component]) {
                maximum_components[component] = components[component];
            }
        }
    }
}

// FUNCTION: SURRENDER 0x10066760
void srVP_generic::_copy(srVector3* destination, const srVector4* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = source[index].x;
        destination[index].y = source[index].y;
        destination[index].z = source[index].z;
    }
}

// FUNCTION: SURRENDER 0x100667A0
void srVP_generic::_copy(srVector3* destination, const srVector3& constant, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = constant.x;
        destination[index].y = constant.y;
        destination[index].z = constant.z;
    }
}

// FUNCTION: SURRENDER 0x100667D0
void srVP_generic::_add(srVector3* destination, const srVector3& constant,
                        const srVector3* vector_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vector_source[index].x + constant.x;
        destination[index].y = vector_source[index].y + constant.y;
        destination[index].z = vector_source[index].z + constant.z;
    }
}

// FUNCTION: SURRENDER 0x10066820
void srVP_generic::_sub(srVector3* destination, const srVector3& constant,
                        const srVector3* vector_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = constant.x - vector_source[index].x;
        destination[index].y = constant.y - vector_source[index].y;
        destination[index].z = constant.z - vector_source[index].z;
    }
}

// FUNCTION: SURRENDER 0x10066870
void srVP_generic::_mul(srVector3* destination, const srVector3& constant,
                        const srVector3* vector_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vector_source[index].x * constant.x;
        destination[index].y = vector_source[index].y * constant.y;
        destination[index].z = vector_source[index].z * constant.z;
    }
}

// FUNCTION: SURRENDER 0x100668C0
void srVP_generic::_div(srVector3* destination, const srVector3& constant,
                        const srVector3* vector_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = constant.x / vector_source[index].x;
        destination[index].y = constant.y / vector_source[index].y;
        destination[index].z = constant.z / vector_source[index].z;
    }
}

// FUNCTION: SURRENDER 0x10066910
void srVP_generic::_add(srVector3* destination, const srVector3& constant,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = float_source[index] + constant.x;
        destination[index].y = constant.y + float_source[index];
        destination[index].z = float_source[index] + constant.z;
    }
}

// FUNCTION: SURRENDER 0x10066950
void srVP_generic::_sub(srVector3* destination, const srVector3& constant,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = constant.x - float_source[index];
        destination[index].y = constant.y - float_source[index];
        destination[index].z = constant.z - float_source[index];
    }
}

// FUNCTION: SURRENDER 0x10066990
void srVP_generic::_mul(srVector3* destination, const srVector3& constant,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = float_source[index] * constant.x;
        destination[index].y = float_source[index] * constant.y;
        destination[index].z = float_source[index] * constant.z;
    }
}

// FUNCTION: SURRENDER 0x100669D0
void srVP_generic::_div(srVector3* destination, const srVector3& constant,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = constant.x / float_source[index];
        destination[index].y = constant.y / float_source[index];
        destination[index].z = constant.z / float_source[index];
    }
}

// FUNCTION: SURRENDER 0x10066A10
void srVP_generic::_add(srVector3* destination, const srVector3* vector_source,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = float_source[index] + vector_source[index].x;
        destination[index].y = vector_source[index].y + float_source[index];
        destination[index].z = vector_source[index].z + float_source[index];
    }
}

// FUNCTION: SURRENDER 0x10066A60
void srVP_generic::_sub(srVector3* destination, const srVector3* vector_source,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vector_source[index].x - float_source[index];
        destination[index].y = vector_source[index].y - float_source[index];
        destination[index].z = vector_source[index].z - float_source[index];
    }
}

// FUNCTION: SURRENDER 0x10066AB0
void srVP_generic::_mul(srVector3* destination, const srVector3* vector_source,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vector_source[index].x * float_source[index];
        destination[index].y = vector_source[index].y * float_source[index];
        destination[index].z = vector_source[index].z * float_source[index];
    }
}

// FUNCTION: SURRENDER 0x10066B00
void srVP_generic::_div(srVector3* destination, const srVector3* vector_source,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vector_source[index].x / float_source[index];
        destination[index].y = vector_source[index].y / float_source[index];
        destination[index].z = vector_source[index].z / float_source[index];
    }
}

// FUNCTION: SURRENDER 0x10066B50
void srVP_generic::_sub(srVector3* destination, const float* float_source,
                        const srVector3* vector_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = float_source[index] - vector_source[index].x;
        destination[index].y = float_source[index] - vector_source[index].y;
        destination[index].z = float_source[index] - vector_source[index].z;
    }
}

// FUNCTION: SURRENDER 0x10066BA0
void srVP_generic::_div(srVector3* destination, const float* float_source,
                        const srVector3* vector_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = float_source[index] / vector_source[index].x;
        destination[index].y = float_source[index] / vector_source[index].y;
        destination[index].z = float_source[index] / vector_source[index].z;
    }
}

// FUNCTION: SURRENDER 0x10066BF0
void srVP_generic::_length(float* destination, const srVector3* vectors, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] =
            (float)sqrt(vectors[index].x * vectors[index].x + vectors[index].y * vectors[index].y +
                        vectors[index].z * vectors[index].z);
    }
}

// FUNCTION: SURRENDER 0x10066C40
void srVP_generic::_dot(float* destination, const srVector3& constant, const srVector3* vectors,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = vectors[index].x * constant.x + vectors[index].y * constant.y +
                             vectors[index].z * constant.z;
    }
}

// FUNCTION: SURRENDER 0x10066C80
void srVP_generic::_dot(float* destination, const srVector4& constant, const srVector3* vectors,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = vectors[index].x * constant.x + vectors[index].y * constant.y +
                             vectors[index].z * constant.z + constant.w;
    }
}

// FUNCTION: SURRENDER 0x10066CC0
void srVP_generic::_dot(float* destination, const srVector3* vectors_0, const srVector3* vectors_1,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = vectors_0[index].x * vectors_1[index].x +
                             vectors_0[index].y * vectors_1[index].y +
                             vectors_0[index].z * vectors_1[index].z;
    }
}

// FUNCTION: SURRENDER 0x10066D10
void srVP_generic::_normalize(srVector3* destination, const srVector3* vectors, float length,
                              SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        float scale = length / (float)sqrt(vectors[index].x * vectors[index].x +
                                           vectors[index].y * vectors[index].y +
                                           vectors[index].z * vectors[index].z);
        destination[index].x = scale * vectors[index].x;
        destination[index].y = scale * vectors[index].y;
        destination[index].z = scale * vectors[index].z;
    }
}

// FUNCTION: SURRENDER 0x10066D80
void srVP_generic::_mulIndexed(srVector3* destination, const srVector3& constant,
                               const srVector3* indexed_source, const SRDWORD* indices,
                               SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = constant.x * indexed_source[source_index].x;
        destination[index].y = indexed_source[source_index].y * constant.y;
        destination[index].z = indexed_source[source_index].z * constant.z;
    }
}

// FUNCTION: SURRENDER 0x10066DD0
void srVP_generic::_mulIndexed(srVector3* destination, const srVector3* linear_source,
                               const srVector3* indexed_source, const SRDWORD* indices,
                               SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = linear_source[index].x * indexed_source[source_index].x;
        destination[index].y = linear_source[index].y * indexed_source[source_index].y;
        destination[index].z = linear_source[index].z * indexed_source[source_index].z;
    }
}

// FUNCTION: SURRENDER 0x10066E30
void srVP_generic::_copyIndexed(srVector3* destination, const srVector2* source,
                                const SRDWORD* indices, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = source[source_index].x;
        destination[index].y = source[source_index].y;
        destination[index].z = 0.0f;
    }
}

// FUNCTION: SURRENDER 0x10066E70
void srVP_generic::_copyIndexed(srVector3* destination, const srVector3* source,
                                const SRDWORD* indices, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = source[source_index].x;
        destination[index].y = source[source_index].y;
        destination[index].z = source[source_index].z;
    }
}

// FUNCTION: SURRENDER 0x10066EC0
void srVP_generic::_copyIndexed(srVector3* destination, const srVector4* source,
                                const SRDWORD* indices, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = source[source_index].x;
        destination[index].y = source[source_index].y;
        destination[index].z = source[source_index].z;
    }
}

// FUNCTION: SURRENDER 0x10066F10
void srVP_generic::_transform(srVector3* destination, const srVector3* vectors,
                              const srMatrix4& matrix, SRDWORD count)
{
    const float* m = &matrix.vectors[0].x;
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x =
            vectors[index].x * m[0] + vectors[index].y * m[1] + vectors[index].z * m[2] + m[3];
        destination[index].y =
            vectors[index].x * m[4] + vectors[index].y * m[5] + vectors[index].z * m[6] + m[7];
        destination[index].z =
            vectors[index].x * m[8] + vectors[index].y * m[9] + vectors[index].z * m[10] + m[11];
    }
}

// FUNCTION: SURRENDER 0x10066FA0
void srVP_generic::_dir(srVector3* destination, float* lengths, const srVector3* source,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        float length =
            (float)sqrt(source[index].x * source[index].x + source[index].y * source[index].y +
                        source[index].z * source[index].z);
        lengths[index] = length;
        destination[index].x = source[index].x / length;
        destination[index].y = source[index].y / length;
        destination[index].z = source[index].z / length;
    }
}

// FUNCTION: SURRENDER 0x10067010
void srVP_generic::_dir(srVector3* destination, float* lengths, const srVector4* source,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        float length =
            (float)sqrt(source[index].x * source[index].x + source[index].y * source[index].y +
                        source[index].z * source[index].z);
        lengths[index] = length;
        destination[index].x = source[index].x / length;
        destination[index].y = source[index].y / length;
        destination[index].z = source[index].z / length;
    }
}

// FUNCTION: SURRENDER 0x10067080
void srVP_generic::_cross(srVector3* destination, const srVector3* vectors_0,
                          const srVector3* vectors_1, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x =
            vectors_0[index].y * vectors_1[index].z - vectors_0[index].z * vectors_1[index].y;
        destination[index].y =
            vectors_0[index].z * vectors_1[index].x - vectors_0[index].x * vectors_1[index].z;
        destination[index].z =
            vectors_0[index].x * vectors_1[index].y - vectors_0[index].y * vectors_1[index].x;
    }
}

// FUNCTION: SURRENDER 0x10067100
void srVP_generic::_copy(srVector4* destination, const srVector3* source_0, const float* source_1,
                         SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = source_0[index].x;
        destination[index].y = source_0[index].y;
        destination[index].z = source_0[index].z;
        destination[index].w = source_1[index];
    }
}

// FUNCTION: SURRENDER 0x10067150
void srVP_generic::_copy(srVector4* destination, const srVector3* source, float constant,
                         SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = source[index].x;
        destination[index].y = source[index].y;
        destination[index].z = source[index].z;
        destination[index].w = constant;
    }
}

// FUNCTION: SURRENDER 0x10067190
void srVP_generic::_copy(srVector4* destination, const srVector4& constant, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = constant.x;
        destination[index].y = constant.y;
        destination[index].z = constant.z;
        destination[index].w = constant.w;
    }
}

// FUNCTION: SURRENDER 0x100671D0
void srVP_generic::_add(srVector4* destination, const srVector4& constant,
                        const srVector4* vector_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vector_source[index].x + constant.x;
        destination[index].y = vector_source[index].y + constant.y;
        destination[index].z = vector_source[index].z + constant.z;
        destination[index].w = vector_source[index].w + constant.w;
    }
}

// FUNCTION: SURRENDER 0x10067230
void srVP_generic::_mul(srVector4* destination, const srVector4& constant,
                        const srVector4* vector_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vector_source[index].x * constant.x;
        destination[index].y = vector_source[index].y * constant.y;
        destination[index].z = vector_source[index].z * constant.z;
        destination[index].w = vector_source[index].w * constant.w;
    }
}

// FUNCTION: SURRENDER 0x10067290
void srVP_generic::_sub(srVector4* destination, const srVector4& constant,
                        const srVector4* vector_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = constant.x - vector_source[index].x;
        destination[index].y = constant.y - vector_source[index].y;
        destination[index].z = constant.z - vector_source[index].z;
        destination[index].w = constant.w - vector_source[index].w;
    }
}

// FUNCTION: SURRENDER 0x100672F0
void srVP_generic::_div(srVector4* destination, const srVector4& constant,
                        const srVector4* vector_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = constant.x / vector_source[index].x;
        destination[index].y = constant.y / vector_source[index].y;
        destination[index].z = constant.z / vector_source[index].z;
        destination[index].w = constant.w / vector_source[index].w;
    }
}

// FUNCTION: SURRENDER 0x10067350
void srVP_generic::_add(srVector4* destination, const srVector4& constant,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = float_source[index] + constant.x;
        destination[index].y = float_source[index] + constant.y;
        destination[index].z = float_source[index] + constant.z;
        destination[index].w = float_source[index] + constant.w;
    }
}

// FUNCTION: SURRENDER 0x100673A0
void srVP_generic::_mul(srVector4* destination, const srVector4& constant,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = float_source[index] * constant.x;
        destination[index].y = float_source[index] * constant.y;
        destination[index].z = float_source[index] * constant.z;
        destination[index].w = float_source[index] * constant.w;
    }
}

// FUNCTION: SURRENDER 0x100673F0
void srVP_generic::_sub(srVector4* destination, const srVector4& constant,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = constant.x - float_source[index];
        destination[index].y = constant.y - float_source[index];
        destination[index].z = constant.z - float_source[index];
        destination[index].w = constant.w - float_source[index];
    }
}

// FUNCTION: SURRENDER 0x10067440
void srVP_generic::_div(srVector4* destination, const srVector4& constant,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = constant.x / float_source[index];
        destination[index].y = constant.y / float_source[index];
        destination[index].z = constant.z / float_source[index];
        destination[index].w = constant.w / float_source[index];
    }
}

// FUNCTION: SURRENDER 0x10067490
void srVP_generic::_add(srVector4* destination, const srVector4* vector_source,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vector_source[index].x + float_source[index];
        destination[index].y = vector_source[index].y + float_source[index];
        destination[index].z = vector_source[index].z + float_source[index];
        destination[index].w = vector_source[index].w + float_source[index];
    }
}

// FUNCTION: SURRENDER 0x100674F0
void srVP_generic::_mul(srVector4* destination, const srVector4* vector_source,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vector_source[index].x * float_source[index];
        destination[index].y = vector_source[index].y * float_source[index];
        destination[index].z = vector_source[index].z * float_source[index];
        destination[index].w = vector_source[index].w * float_source[index];
    }
}

// FUNCTION: SURRENDER 0x10067550
void srVP_generic::_sub(srVector4* destination, const srVector4* vector_source,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vector_source[index].x - float_source[index];
        destination[index].y = vector_source[index].y - float_source[index];
        destination[index].z = vector_source[index].z - float_source[index];
        destination[index].w = vector_source[index].w - float_source[index];
    }
}

// FUNCTION: SURRENDER 0x100675B0
void srVP_generic::_div(srVector4* destination, const srVector4* vector_source,
                        const float* float_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vector_source[index].x / float_source[index];
        destination[index].y = vector_source[index].y / float_source[index];
        destination[index].z = vector_source[index].z / float_source[index];
        destination[index].w = vector_source[index].w / float_source[index];
    }
}

// FUNCTION: SURRENDER 0x10067610
void srVP_generic::_sub(srVector4* destination, const float* float_source,
                        const srVector4* vector_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = float_source[index] - vector_source[index].x;
        destination[index].y = float_source[index] - vector_source[index].y;
        destination[index].z = float_source[index] - vector_source[index].z;
        destination[index].w = float_source[index] - vector_source[index].w;
    }
}

// FUNCTION: SURRENDER 0x10067670
void srVP_generic::_div(srVector4* destination, const float* float_source,
                        const srVector4* vector_source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = float_source[index] / vector_source[index].x;
        destination[index].y = float_source[index] / vector_source[index].y;
        destination[index].z = float_source[index] / vector_source[index].z;
        destination[index].w = float_source[index] / vector_source[index].w;
    }
}

// FUNCTION: SURRENDER 0x100676D0
void srVP_generic::_length(float* destination, const srVector4* vectors, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] =
            (float)sqrt(vectors[index].x * vectors[index].x + vectors[index].y * vectors[index].y +
                        vectors[index].z * vectors[index].z + vectors[index].w * vectors[index].w);
    }
}

// FUNCTION: SURRENDER 0x10067720
void srVP_generic::_dot(float* destination, const srVector4& constant, const srVector4* vectors,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = vectors[index].x * constant.x + vectors[index].y * constant.y +
                             vectors[index].z * constant.z + vectors[index].w * constant.w;
    }
}

// FUNCTION: SURRENDER 0x10067770
void srVP_generic::_dotIndexed(float* destination, const srVector4& constant,
                               const srVector4* vectors, const SRDWORD* indices, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        const srVector4* vector = &vectors[source_index];
        destination[index] = vector->x * constant.x + vector->y * constant.y +
                             vector->z * constant.z + vector->w * constant.w;
    }
}

// FUNCTION: SURRENDER 0x100677D0
void srVP_generic::_dot(float* destination, const srVector4* vectors_0, const srVector4* vectors_1,
                        SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] =
            vectors_0[index].x * vectors_1[index].x + vectors_0[index].y * vectors_1[index].y +
            vectors_0[index].z * vectors_1[index].z + vectors_0[index].w * vectors_1[index].w;
    }
}

// FUNCTION: SURRENDER 0x10067830
void srVP_generic::_normalize(srVector4* destination, const srVector4* vectors, float length,
                              SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        float scale =
            length /
            (float)sqrt(vectors[index].x * vectors[index].x + vectors[index].y * vectors[index].y +
                        vectors[index].z * vectors[index].z + vectors[index].w * vectors[index].w);
        destination[index].x = scale * vectors[index].x;
        destination[index].y = scale * vectors[index].y;
        destination[index].z = scale * vectors[index].z;
        destination[index].w = scale * vectors[index].w;
    }
}

// FUNCTION: SURRENDER 0x100678B0
void srVP_generic::_mulIndexed(srVector4* destination, const srVector4& constant,
                               const srVector4* indexed_source, const SRDWORD* indices,
                               SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = constant.x * indexed_source[source_index].x;
        destination[index].y = constant.y * indexed_source[source_index].y;
        destination[index].z = constant.z * indexed_source[source_index].z;
        destination[index].w = indexed_source[source_index].w * constant.w;
    }
}

// FUNCTION: SURRENDER 0x10067910
void srVP_generic::_mulIndexed(srVector4* destination, const srVector4* linear_source,
                               const srVector4* indexed_source, const SRDWORD* indices,
                               SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = linear_source[index].x * indexed_source[source_index].x;
        destination[index].y = indexed_source[source_index].y * linear_source[index].y;
        destination[index].z = indexed_source[source_index].z * linear_source[index].z;
        destination[index].w = indexed_source[source_index].w * linear_source[index].w;
    }
}

// FUNCTION: SURRENDER 0x10067980
void srVP_generic::_copyIndexed(srVector4* destination, const srARGB* source,
                                const SRDWORD* indices, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = source[source_index].red * (1.0f / 255.0f);
        destination[index].y = source[source_index].green * (1.0f / 255.0f);
        destination[index].z = source[source_index].blue * (1.0f / 255.0f);
        destination[index].w = source[source_index].alpha * (1.0f / 255.0f);
    }
}

// FUNCTION: SURRENDER 0x10067A10
void srVP_generic::_copyIndexed(srVector4* destination, const srVector4* source,
                                const SRDWORD* indices, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = source[source_index].x;
        destination[index].y = source[source_index].y;
        destination[index].z = source[source_index].z;
        destination[index].w = source[source_index].w;
    }
}

// FUNCTION: SURRENDER 0x10067A60
void srVP_generic::_copyIndexed(srVector4* destination, const srVector3* source,
                                const SRDWORD* indices, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = source[source_index].x;
        destination[index].y = source[source_index].y;
        destination[index].z = source[source_index].z;
        destination[index].w = 1.0f;
    }
}

// FUNCTION: SURRENDER 0x10067AB0
void srVP_generic::_copyIndexed(srVector4* destination, const srVector2* source,
                                const SRDWORD* indices, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = source[source_index].x;
        destination[index].y = source[source_index].y;
        destination[index].z = 0.0f;
        destination[index].w = 1.0f;
    }
}

// FUNCTION: SURRENDER 0x10067B00
void srVP_generic::_divByW(srVector4* destination, const srVector4* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = source[index].x / source[index].w;
        destination[index].y = source[index].y / source[index].w;
        destination[index].z = source[index].z / source[index].w;
        destination[index].w = 1.0f;
    }
}

// FUNCTION: SURRENDER 0x10067B50
void srVP_generic::_transformOrtho(srVector4* destination, const srVector4* source,
                                   const srMatrix4& matrix, SRDWORD count)
{
    const float* m = &matrix.vectors[0].x;
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = source[index].x * m[0] + source[index].w * m[3];
        destination[index].y = source[index].y * m[5] + source[index].w * m[7];
        destination[index].z = source[index].z * m[10] + source[index].w * m[11];
        destination[index].w = source[index].w * m[15];
    }
}

// FUNCTION: SURRENDER 0x10067BC0
void srVP_generic::_transformPerspective(srVector4* destination, const srVector4* source,
                                         const srMatrix4& matrix, SRDWORD count)
{
    const float* m = &matrix.vectors[0].x;
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = source[index].x * m[0] + source[index].z * m[2];
        destination[index].y = source[index].z * m[6] + source[index].y * m[5];
        destination[index].z = source[index].w * m[11] + source[index].z * m[10];
        destination[index].w = source[index].z * m[14];
    }
}

// FUNCTION: SURRENDER 0x10067C30
void srVP_generic::_transform(srVector4* destination, const srVector4* vectors,
                              const srMatrix4& matrix, SRDWORD count)
{
    const float* m = &matrix.vectors[0].x;
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = vectors[index].x * m[0] + vectors[index].y * m[1] +
                               vectors[index].z * m[2] + vectors[index].w * m[3];
        destination[index].y = vectors[index].x * m[4] + vectors[index].y * m[5] +
                               vectors[index].z * m[6] + vectors[index].w * m[7];
        destination[index].z = vectors[index].x * m[8] + vectors[index].y * m[9] +
                               vectors[index].z * m[10] + vectors[index].w * m[11];
        destination[index].w = vectors[index].x * m[12] + vectors[index].y * m[13] +
                               vectors[index].z * m[14] + vectors[index].w * m[15];
    }
}

// FUNCTION: SURRENDER 0x10067D00
void srVP_generic::_transform(srVector4* destination, const srVector3* vectors,
                              const srMatrix4& matrix, SRDWORD count)
{
    const float* m = &matrix.vectors[0].x;
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x =
            vectors[index].x * m[0] + vectors[index].y * m[1] + vectors[index].z * m[2] + m[3];
        destination[index].y =
            vectors[index].x * m[4] + vectors[index].y * m[5] + vectors[index].z * m[6] + m[7];
        destination[index].z =
            vectors[index].x * m[8] + vectors[index].y * m[9] + vectors[index].z * m[10] + m[11];
        destination[index].w =
            vectors[index].x * m[12] + vectors[index].y * m[13] + vectors[index].z * m[14] + m[15];
    }
}

// FUNCTION: SURRENDER 0x10067DC0
void srVP_generic::_transformIndexed(srVector3* destination, const srVector3* source,
                                     const SRDWORD* indices, const srMatrix4& matrix, SRDWORD count)
{
    const float* m = &matrix.vectors[0].x;
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        const srVector3* vector = &source[source_index];
        destination[index].x = vector->x * m[0] + vector->y * m[1] + vector->z * m[2] + m[3];
        destination[index].y = vector->x * m[4] + vector->y * m[5] + vector->z * m[6] + m[7];
        destination[index].z = vector->x * m[8] + vector->y * m[9] + vector->z * m[10] + m[11];
    }
}

// FUNCTION: SURRENDER 0x10067E60
void srVP_generic::_transformIndexed(srVector4* destination, const srVector3* source,
                                     const SRDWORD* indices, const srMatrix4& matrix, SRDWORD count)
{
    const float* m = &matrix.vectors[0].x;
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        const srVector3* vector = &source[source_index];
        destination[index].x = vector->x * m[0] + vector->y * m[1] + vector->z * m[2] + m[3];
        destination[index].y = vector->x * m[4] + vector->y * m[5] + vector->z * m[6] + m[7];
        destination[index].z = vector->x * m[8] + vector->y * m[9] + vector->z * m[10] + m[11];
        destination[index].w = vector->x * m[12] + vector->y * m[13] + vector->z * m[14] + m[15];
    }
}

// FUNCTION: SURRENDER 0x10067F20
void srVP_generic::_axpy(srVector4* destination, const srVector4& add_constant,
                         const srVector4& multiply_constant, const float* multiply_source,
                         SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = multiply_source[index] * multiply_constant.x + add_constant.x;
        destination[index].y = multiply_source[index] * multiply_constant.y + add_constant.y;
        destination[index].z = multiply_source[index] * multiply_constant.z + add_constant.z;
        destination[index].w = multiply_source[index] * multiply_constant.w + add_constant.w;
    }
}

// FUNCTION: SURRENDER 0x10067F80
void srVP_generic::_axpy(srVector4* destination, const srVector4* add_source,
                         const srVector4& multiply_constant, const float* multiply_source,
                         SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = multiply_constant.x * multiply_source[index] + add_source[index].x;
        destination[index].y = multiply_constant.y * multiply_source[index] + add_source[index].y;
        destination[index].z = multiply_source[index] * multiply_constant.z + add_source[index].z;
        destination[index].w = multiply_constant.w * multiply_source[index] + add_source[index].w;
    }
}

// FUNCTION: SURRENDER 0x10067FF0
void srVP_generic::_axpy(srVector4* destination, const srVector4& add_constant,
                         const srVector4* multiply_vectors, const float* multiply_source,
                         SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = multiply_vectors[index].x * multiply_source[index] + add_constant.x;
        destination[index].y = multiply_vectors[index].y * multiply_source[index] + add_constant.y;
        destination[index].z = multiply_vectors[index].z * multiply_source[index] + add_constant.z;
        destination[index].w = multiply_source[index] * multiply_vectors[index].w + add_constant.w;
    }
}

// FUNCTION: SURRENDER 0x10068060
void srVP_generic::_axpy(srVector4* destination, const srVector4* add_source,
                         const srVector4* multiply_vectors, const float* multiply_source,
                         SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x =
            multiply_source[index] * multiply_vectors[index].x + add_source[index].x;
        destination[index].y =
            multiply_vectors[index].y * multiply_source[index] + add_source[index].y;
        destination[index].z =
            multiply_vectors[index].z * multiply_source[index] + add_source[index].z;
        destination[index].w =
            multiply_vectors[index].w * multiply_source[index] + add_source[index].w;
    }
}

// FUNCTION: SURRENDER 0x10068100
void srVP_generic::_axpy(srVector4* destination, const srVector4& add_constant,
                         const srVector4& multiply_constant, const float* multiply_source_0,
                         const float* multiply_source_1, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x =
            multiply_constant.x * multiply_source_0[index] * multiply_source_1[index] +
            add_constant.x;
        destination[index].y =
            multiply_constant.y * multiply_source_0[index] * multiply_source_1[index] +
            add_constant.y;
        destination[index].z =
            multiply_source_0[index] * multiply_source_1[index] * multiply_constant.z +
            add_constant.z;
        destination[index].w =
            multiply_constant.w * multiply_source_0[index] * multiply_source_1[index] +
            add_constant.w;
    }
}

// FUNCTION: SURRENDER 0x10068180
void srVP_generic::_axpy(srVector4* destination, const srVector4* add_source,
                         const srVector4& multiply_constant, const float* multiply_source_0,
                         const float* multiply_source_1, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x =
            multiply_constant.x * multiply_source_0[index] * multiply_source_1[index] +
            add_source[index].x;
        destination[index].y =
            multiply_constant.y * multiply_source_0[index] * multiply_source_1[index] +
            add_source[index].y;
        destination[index].z =
            multiply_constant.z * multiply_source_0[index] * multiply_source_1[index] +
            add_source[index].z;
        destination[index].w =
            multiply_constant.w * multiply_source_0[index] * multiply_source_1[index] +
            add_source[index].w;
    }
}

// FUNCTION: SURRENDER 0x10068210
void srVP_generic::_mulAdd(srVector4* destination, const srVector4& add_constant,
                           const srVector4& multiply_constant, const srVector4* multiply_source,
                           SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = multiply_source[index].x * multiply_constant.x + add_constant.x;
        destination[index].y = multiply_source[index].y * multiply_constant.y + add_constant.y;
        destination[index].z = multiply_source[index].z * multiply_constant.z + add_constant.z;
        destination[index].w = multiply_source[index].w * multiply_constant.w + add_constant.w;
    }
}

// FUNCTION: SURRENDER 0x10068280
void srVP_generic::_mulAdd(srVector4* destination, const srVector4* add_source,
                           const srVector4& multiply_constant, const srVector4* multiply_source,
                           SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x = multiply_constant.x * multiply_source[index].x + add_source[index].x;
        destination[index].y = multiply_source[index].y * multiply_constant.y + add_source[index].y;
        destination[index].z = multiply_source[index].z * multiply_constant.z + add_source[index].z;
        destination[index].w = multiply_source[index].w * multiply_constant.w + add_source[index].w;
    }
}

// FUNCTION: SURRENDER 0x10068310
void srVP_generic::_mulAdd(srVector4* destination, const srVector4& add_constant,
                           const srVector4* multiply_source_0, const srVector4* multiply_source_1,
                           SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].x =
            multiply_source_1[index].x * multiply_source_0[index].x + add_constant.x;
        destination[index].y =
            multiply_source_0[index].y * multiply_source_1[index].y + add_constant.y;
        destination[index].z =
            multiply_source_0[index].z * multiply_source_1[index].z + add_constant.z;
        destination[index].w =
            multiply_source_1[index].w * multiply_source_0[index].w + add_constant.w;
    }
}

// FUNCTION: SURRENDER 0x100683A0
void srVP_generic::_copyW(srVector4* destination, float constant, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].w = constant;
    }
}

// FUNCTION: SURRENDER 0x100683C0
void srVP_generic::_copyW(srVector4* destination, const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index].w = source[index];
    }
}

// FUNCTION: SURRENDER 0x100683F0
void srVP_generic::_copyW(float* destination, const srVector4* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[index] = source[index].w;
    }
}

// FUNCTION: SURRENDER 0x10068420
void srVP_generic::_minMax(const srVector4* source, srVector4& minimum, srVector4& maximum,
                           SRDWORD count)
{
    minimum = source[0];
    maximum = source[0];
    float* minimum_components = &minimum.x;
    float* maximum_components = &maximum.x;
    for (SRDWORD index = 1; index < count; ++index) {
        const float* components = &source[index].x;
        for (int component = 0; component < 4; ++component) {
            if (components[component] < minimum_components[component]) {
                minimum_components[component] = components[component];
            } else if (components[component] > maximum_components[component]) {
                maximum_components[component] = components[component];
            }
        }
    }
}

// FUNCTION: SURRENDER 0x100684C0
void srVP_generic::_memcopy(void* destination, const void* source, SRDWORD bytes)
{
    SRBYTE* destination_bytes = static_cast<SRBYTE*>(destination);
    const SRBYTE* source_bytes = static_cast<const SRBYTE*>(source);
    for (SRDWORD index = 0; index < bytes; ++index) {
        destination_bytes[index] = source_bytes[index];
    }
}

// FUNCTION: SURRENDER 0x100684E0
void srVP_generic::_memcopy(void* destination, int source, SRDWORD bytes)
{
    if (bytes <= 0) {
        return;
    }
    memset(destination, source, bytes);
}

// FUNCTION: SURRENDER 0x10068520
int srVP_generic::_memcmp(const void* source_0, const void* source_1, SRDWORD bytes)
{
    const SRBYTE* first = static_cast<const SRBYTE*>(source_0);
    const SRBYTE* second = static_cast<const SRBYTE*>(source_1);
    for (SRDWORD index = 0; index < bytes; ++index) {
        if (first[index] != second[index]) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: SURRENDER 0x10068560
void srVP_generic::_prefetch(const void* destination, SRDWORD bytes, SRDWORD value_014) {}

// FUNCTION: SURRENDER 0x10068570
void srVP_generic::_copyInterleaved(void* destination, const void* source,
                                    SRDWORD destination_pitch, SRDWORD source_pitch, SRDWORD width,
                                    SRDWORD count)
{
    SRBYTE* destination_bytes = static_cast<SRBYTE*>(destination);
    const SRBYTE* source_bytes = static_cast<const SRBYTE*>(source);
    for (SRDWORD row = 0; row < count; ++row) {
        for (SRDWORD column = 0; column < width; ++column) {
            destination_bytes[column] = source_bytes[column];
        }
        destination_bytes += destination_pitch;
        source_bytes += source_pitch;
    }
}

/* The per-column term order of each product is a.x, a.y, a.w, a.z; the retail
   bodies at 0x100685C0 and in the shipped srVP_* modules all evaluate the w
   term before the z term. */
// FUNCTION: SURRENDER 0x100685C0
void srVP_generic::_mul(srMatrix4& destination, const srMatrix4& source_0,
                        const srMatrix4& source_1)
{
    float* result = &destination.vectors[0].x;
    const float* left = &source_0.vectors[0].x;
    const float* right = &source_1.vectors[0].x;
    for (int column = 0; column < 4; ++column) {
        float right_0 = right[column];
        float right_1 = right[4 + column];
        float right_2 = right[8 + column];
        float right_3 = right[12 + column];
        result[column] =
            right_0 * left[0] + right_1 * left[1] + right_3 * left[3] + right_2 * left[2];
        result[4 + column] =
            right_0 * left[4] + right_1 * left[5] + right_3 * left[7] + right_2 * left[6];
        result[8 + column] =
            right_0 * left[8] + right_1 * left[9] + right_3 * left[11] + right_2 * left[10];
        result[12 + column] =
            right_0 * left[12] + right_1 * left[13] + right_3 * left[15] + right_2 * left[14];
    }
}

// FUNCTION: SURRENDER 0x10068670
void srVP_generic::_mul(srMatrix4* destination, const srMatrix4* source_0,
                        const srMatrix4* source_1, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        _mul(destination[index], source_0[index], source_1[index]);
    }
}

// FUNCTION: SURRENDER 0x100686C0
void srVP_generic::_cubic(float* destination, const float* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        float value = source[index];
        destination[index] = (3.0f - (value + value)) * value * value;
    }
}

// FUNCTION: SURRENDER 0x10068700
int srVP_generic::_srTestBoundingBox(const srMatrix4& matrix, const srVector3& minimum,
                                     const srVector3& maximum)
{
    const float* m = &matrix.vectors[0].x;
    float z_min = minimum.z * m[14];
    float y_min = m[13] * minimum.y;
    float w_min = m[12] * minimum.x + m[15] + y_min + z_min;
    if (w_min >= fabsf(m[0] * minimum.x + m[1] * minimum.y + m[2] * minimum.z + m[3]) &&
        w_min >= fabsf(m[5] * minimum.y + m[6] * minimum.z + m[4] * minimum.x + m[7]) &&
        w_min >= fabsf(m[9] * minimum.y + m[10] * minimum.z + m[8] * minimum.x + m[11])) {
        return 1;
    }
    float xy_min = m[12] * minimum.x + m[15];
    float w_max = m[12] * maximum.x + m[15];
    float y_max = m[13] * maximum.y;
    float z_max = maximum.z * m[14];
    float w_000 = z_min + y_min + xy_min;
    float w_001 = z_max + y_min + xy_min;
    float w_010 = y_max + z_min + xy_min;
    float w_011 = z_max + y_max + xy_min;
    float w_100 = w_max + z_min + y_min;
    float w_101 = z_max + w_max + y_min;
    float w_110 = y_max + w_max + z_min;
    float w_111 = z_max + y_max + w_max;
    const float* row = m + 9;
    for (int plane = 2; plane >= 0; --plane, row -= 4) {
        float base_min = minimum.x * row[-1] + row[2];
        float ymin = row[0] * minimum.y;
        float zmin = row[1] * minimum.z;
        float base_max = row[-1] * maximum.x + row[2];
        float ymax = row[0] * maximum.y;
        float zmax = maximum.z * row[1];
        float v_000 = zmin + ymin + base_min;
        if (-w_000 < v_000) {
            if (w_000 <= v_000 && w_001 <= zmax + ymin + base_min &&
                w_010 <= ymax + zmin + base_min && w_011 <= zmax + ymax + base_min &&
                w_100 <= base_max + zmin + ymin && w_101 <= zmax + base_max + ymin &&
                w_110 <= ymax + base_max + zmin && w_111 <= zmax + ymax + base_max) {
                return 0;
            }
        } else if (zmax + ymin + base_min <= -w_001 && ymax + zmin + base_min <= -w_010 &&
                   zmax + ymax + base_min <= -w_011 && base_max + zmin + ymin <= -w_100 &&
                   zmax + base_max + ymin <= -w_101 && ymax + base_max + zmin <= -w_110 &&
                   zmax + ymax + base_max <= -w_111) {
            return 0;
        }
    }
    return 1;
}

/* Table-driven specular power: the exponent is clamped to 127 and halved
   until it lies in [1,2); the fractional part of (reduced-1)*16 selects two
   rows of the inherited coefficient table for lerp, and
   points_248[index+1][squarings] is the per-element dead-zone threshold
   below which the result is 0. */
// FUNCTION: SURRENDER 0x10068A80
void srVP_generic::_srSpecularPow(float* destination, const float* source, float exponent,
                                  SRDWORD count)
{
    SRDWORD squarings = 0;
    if (exponent > 127.0) {
        exponent = 127.0f;
    }
    while (exponent >= 2.0f) {
        exponent *= 0.5f;
        ++squarings;
    }
    float index_value = (exponent - 1.0f) * 16.0f;
    int index = static_cast<int>(index_value);
    index_value -= index;
    double complement = 1.0 - index_value;
    float coefficient_0 = (float)(index_value * coefficients_08[index + 1][0] +
                                  complement * coefficients_08[index][0]);
    float coefficient_1 = (float)(index_value * coefficients_08[index + 1][1] +
                                  complement * coefficients_08[index][1]);
    float coefficient_2 = (float)(index_value * coefficients_08[index + 1][2] +
                                  complement * coefficients_08[index][2]);
    float coefficient_3 = (float)(index_value * coefficients_08[index + 1][3] +
                                  complement * coefficients_08[index][3]);
    float threshold = points_248[index + 1][squarings];
    for (SRDWORD element = 0; element < count; ++element) {
        if (source[element] > threshold) {
            float value = source[element];
            for (SRDWORD square = 0; square < squarings; ++square) {
                value *= value;
            }
            destination[element] =
                ((value * coefficient_0 + coefficient_1) * value + coefficient_2) * value +
                coefficient_3;
        } else {
            destination[element] = 0.0f;
        }
    }
}

// FUNCTION: SURRENDER 0x10068BC0
void srVP_generic::_srCopyIndexedRemap(srVector3i* destination, const srVector3i* source,
                                       const SRDWORD* indices, const SRDWORD* remap, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[index].x = remap[source[source_index].x];
        destination[index].y = remap[source[source_index].y];
        destination[index].z = remap[source[source_index].z];
    }
}

// FUNCTION: SURRENDER 0x10068C20
void srVP_generic::_srSetIndexed(SRBYTE* destination, const srVector3i* source,
                                 const SRDWORD* indices, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD source_index = indices[index];
        destination[source[source_index].x] = 1;
        destination[source[source_index].y] = 1;
        destination[source[source_index].z] = 1;
    }
}

// FUNCTION: SURRENDER 0x10068C60
SRDWORD srVP_generic::_srCollectPos(SRDWORD* destination, const float* source, SRDWORD count)
{
    SRDWORD collected = 0;
    for (SRDWORD index = 0; index < count; ++index) {
        if (source[index] >= 0.0f) {
            destination[collected] = index;
            ++collected;
        }
    }
    return collected;
}

// FUNCTION: SURRENDER 0x10068CB0
SRDWORD srVP_generic::_srCollectNeg(SRDWORD* destination, const float* source, SRDWORD count)
{
    SRDWORD collected = 0;
    for (SRDWORD index = 0; index < count; ++index) {
        if (source[index] < 0.0f) {
            destination[collected] = index;
            ++collected;
        }
    }
    return collected;
}

// FUNCTION: SURRENDER 0x10068D00
SRDWORD srVP_generic::_srCollectNonZero(SRDWORD* destination, const SRBYTE* source, SRDWORD count)
{
    SRDWORD collected = 0;
    for (SRDWORD index = 0; index < count; ++index) {
        if (source[index] != 0) {
            destination[collected] = index;
            ++collected;
        }
    }
    return collected;
}

// FUNCTION: SURRENDER 0x10068D30
void srVP_generic::_srRemapInverse(SRDWORD* destination, const SRDWORD* map, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        destination[map[index]] = index;
    }
}

// FUNCTION: SURRENDER 0x10068D60
void srVP_generic::_srDirect3DConvertColor(SRDWORD* destination, const srVector4* source,
                                           SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        SRDWORD color = static_cast<SRLONG>(source[index].w * 255.0f);
        color = color << 8 | static_cast<SRLONG>(source[index].x * 255.0f);
        color = color << 8 | static_cast<SRLONG>(source[index].y * 255.0f);
        color = color << 8 | static_cast<SRLONG>(source[index].z * 255.0f);
        destination[index] = color;
    }
}

// FUNCTION: SURRENDER 0x10068DE0
SRDWORD srVP_generic::_srCullNoClip(SRDWORD* destination, const srVector4& constant,
                                    const srVector4* vectors, SRDWORD count)
{
    float dots[0x100];
    SRDWORD collected = 0;
    for (SRDWORD offset = 0; offset < count; offset += 0x100) {
        SRDWORD chunk = count - offset;
        if (chunk > 0x100) {
            chunk = 0x100;
        }
        _dot(dots, constant, vectors + offset, chunk);
        for (SRDWORD index = 0; index < chunk; ++index) {
            if (dots[index] < 0.0f) {
                destination[collected] = index + offset;
                ++collected;
            }
        }
    }
    return collected;
}

// FUNCTION: SURRENDER 0x10068EC0
void srVP_generic::_srFloatToLinear(SRDWORD* destination, const float* source, SRDWORD count)
{
    const SRLONG* source_bits = reinterpret_cast<const SRLONG*>(source);
    for (SRDWORD index = 0; index < count; ++index) {
        /* reinterpret-ok: sign-magnitude float bits converted to a
           linear-ordered key. */
        destination[index] = (source_bits[index] >> 0x1f | 0x80000000) ^ source_bits[index];
    }
}

// FUNCTION: SURRENDER 0x10068F00
void srVP_generic::_srLinearToFloat(float* destination, const SRDWORD* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        /* reinterpret-ok: linear-ordered key converted back to float bits. */
        *reinterpret_cast<SRLONG*>(&destination[index]) =
            (~static_cast<SRLONG>(source[index]) >> 0x1f | 0x80000000) ^ source[index];
    }
}

// FUNCTION: SURRENDER 0x10068F40
void srVP_generic::_srGetClipFlags(SRBYTE* destination, const srVector4* source, SRDWORD count)
{
    for (SRDWORD index = 0; index < count; ++index) {
        float w = source[index].w;
        float negative_w = -w;
        SRBYTE flags = 0;
        if (negative_w > source[index].z) {
            flags = 0x10;
        }
        if (source[index].z > w) {
            flags |= 0x20;
        }
        if (negative_w > source[index].x) {
            flags |= 0x01;
        }
        if (source[index].x > w) {
            flags |= 0x02;
        }
        if (negative_w > source[index].y) {
            flags |= 0x04;
        }
        if (source[index].y > w) {
            flags |= 0x08;
        }
        destination[index] = flags;
    }
}
