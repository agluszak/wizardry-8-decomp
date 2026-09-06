#include "surrender/srMath.h"

/* The linker folded the empty float-vector constructors for the two-, three-,
   and four-component instantiations to one body. */
// TEMPLATE: WIZ8 0x004D6930
// srVector2T<float>::srVector2T (folded with srVector3T<float> and srVector4T<float>)

// TEMPLATE: WIZ8 0x00421650
// srVector3T<float>::srVector3T(float,float,float)

// TEMPLATE: WIZ8 0x00421670
// srVector3T<float>::method_00421670

// TEMPLATE: WIZ8 0x00421680
// srVector3T<float>::method_00421680

// TEMPLATE: WIZ8 0x004216A0
// srVector3T<float>::operator+=

// TEMPLATE: WIZ8 0x00421700
// srVector3T<float>::method_00421700

// TEMPLATE: WIZ8 0x004218E0
// Function4218E0<float>

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
// srVector3T<float>::method_00446110

// TEMPLATE: WIZ8 0x00451A10
// srVector3T<float>::method_00451A10

// TEMPLATE: WIZ8 0x0049BA80
// srVector3T<float>::method_0049BA80

// TEMPLATE: WIZ8 0x004219F0
// srMatrix3T<float>::method_004219F0

// TEMPLATE: WIZ8 0x00421A40
// srMatrix3T<float>::method_00421A40

// TEMPLATE: WIZ8 0x00467310
// srMatrix3T<float>::SetIdentity00467310

// TEMPLATE: WIZ8 0x00438F90
// srMatrix3T<float>::method_00438F90

// TEMPLATE: WIZ8 0x004817E0
// srVector4T<float>::method_004817E0

// TEMPLATE: WIZ8 0x004D6B30
// srVector4T<float>::method_004D6B30

// TEMPLATE: WIZ8 0x0049BAB0
// srMatrix4T<float>::InvertMatrix0049BAB0

// FUNCTION: WIZ8 0x0049BD00
float Det3_0049BD00(
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
// srMatrix4T<float>::Scale0049BD50

// TEMPLATE: WIZ8 0x0049BDF0
// srMatrix4T<float>::Det0049BDF0

// TEMPLATE: WIZ8 0x0049BF20
// srMatrix4T<float>::AdjugateFrom0049BF20
