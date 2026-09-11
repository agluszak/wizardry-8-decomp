#include "surrender/srMath.h"

/* The linker folded the empty float-vector constructors for the two-, three-,
   and four-component instantiations to one body. */
// TEMPLATE: WIZ8 0x004D6930
// srVector2T<float>::srVector2T (folded with srVector3T<float> and srVector4T<float>)

// TEMPLATE: WIZ8 0x00421650
// srVector3T<float>::srVector3T(float,float,float)

// TEMPLATE: WIZ8 0x00421670
// srVector3T<float>::SetZero

// TEMPLATE: WIZ8 0x00421680
// srVector3T<float>::Set

// TEMPLATE: WIZ8 0x004216A0
// srVector3T<float>::operator+=

// TEMPLATE: WIZ8 0x00421700
// srVector3T<float>::Length

// TEMPLATE: WIZ8 0x004218E0
// DotProduct<float>

// TEMPLATE: WIZ8 0x00438C00
// operator+<float>(srVector3T<float> const &,srVector3T<float> const &)

// TEMPLATE: WIZ8 0x00421900
// operator*<float>(srVector3T<float> const &,double)

// TEMPLATE: WIZ8 0x00421950
// operator/<float>(srVector3T<float> const &,double)

// TEMPLATE: WIZ8 0x004846D0
// srVector3T<float>::operator*=

// TEMPLATE: WIZ8 0x0049B510
// srVector3T<float>::operator/=

// TEMPLATE: WIZ8 0x00446110
// srVector3T<float>::SetFromDouble

// TEMPLATE: WIZ8 0x00451A10
// srVector3T<float>::RotateAboutY

// TEMPLATE: WIZ8 0x0049BA80
// srVector3T<float>::RotateAboutX

// TEMPLATE: WIZ8 0x004219F0
// srMatrix3T<float>::SetRows

// TEMPLATE: WIZ8 0x00421A40
// srMatrix3T<float>::MultiplyBy

// TEMPLATE: WIZ8 0x00467310
// srMatrix3T<float>::SetIdentity

// TEMPLATE: WIZ8 0x00438F90
// srMatrix3T<float>::RotateAboutY

// FUNCTION: WIZ8 0x0042b910
template <>
srMatrix3T<float>* srMatrix3T<float>::RotateAroundAxis(
    double sine,
    double cosine,
    const srVector3T<float>& axis)
{
    srVector3T<float> basis[3];
    srMatrix3T<float> rotation;
    float one_minus_cosine = 1.0f - (float)cosine;

    basis[0].x = axis.x * axis.x + (1.0f - axis.x * axis.x) * (float)cosine;
    basis[0].y = axis.x * axis.y * one_minus_cosine - axis.z * (float)sine;
    basis[0].z = axis.x * axis.z * one_minus_cosine + axis.y * (float)sine;
    basis[1].x = axis.y * axis.x * one_minus_cosine + axis.z * (float)sine;
    basis[1].y = axis.y * axis.y + (1.0f - axis.y * axis.y) * (float)cosine;
    basis[1].z = axis.y * axis.z * one_minus_cosine - axis.x * (float)sine;
    basis[2].x = axis.z * axis.x * one_minus_cosine - axis.y * (float)sine;
    basis[2].y = axis.z * axis.y * one_minus_cosine + axis.x * (float)sine;
    basis[2].z = axis.z * axis.z + (1.0f - axis.z * axis.z) * (float)cosine;
    rotation.vectors[0] = basis[0];
    rotation.vectors[1] = basis[1];
    rotation.vectors[2] = basis[2];
    MultiplyBy(rotation);
    return this;
}

// TEMPLATE: WIZ8 0x004817E0
// srVector4T<float>::Set (double arguments; no source caller retains it)

// TEMPLATE: WIZ8 0x004D6B30
// srVector4T<float>::Set

// TEMPLATE: WIZ8 0x0049BAB0
// srMatrix4T<float>::Invert

// FUNCTION: WIZ8 0x0049BD00
float Det3(
    float param_1,
    float param_2,
    float param_3,
    float param_4,
    float param_5,
    float param_6,
    float param_7,
    float param_8,
    float param_9)
{
    return (param_2 * param_6 - param_3 * param_5) * param_7
        + ((param_5 * param_9 - param_6 * param_8) * param_1
            - (param_2 * param_9 - param_3 * param_8) * param_4);
}

// TEMPLATE: WIZ8 0x0049BD50
// srMatrix4T<float>::Scale

// TEMPLATE: WIZ8 0x0049BDF0
// srMatrix4T<float>::Det

// TEMPLATE: WIZ8 0x0049BF20
// srMatrix4T<float>::AdjugateFrom
