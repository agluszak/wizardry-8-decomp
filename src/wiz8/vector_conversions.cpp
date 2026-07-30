#include "surrender/srMath.h"
#include "wiz8/engine_code/GDCamera.h"

// FUNCTION: WIZ8 0x004D6930
W8CameraMatrixRow004D6930::W8CameraMatrixRow004D6930()
{
}

// FUNCTION: WIZ8 0x00421680
srVector3T<float>* srVector3T<float>::method_00421680(
    double source_0,
    double source_1,
    double source_2)
{
    x = (float)source_0;
    y = (float)source_1;
    z = (float)source_2;
    return this;
}

// FUNCTION: WIZ8 0x004218E0
float Function4218E0(
    const srVector3T<float>& first,
    const srVector3T<float>& second)
{
    return first.x * second.x + first.y * second.y + first.z * second.z;
}

// FUNCTION: WIZ8 0x00446110
srVector3T<float>* srVector3T<float>::method_00446110(const srVector3T<double>* source)
{
    x = (float)source->x;
    y = (float)source->y;
    z = (float)source->z;
    return this;
}

// FUNCTION: WIZ8 0x004219F0
srMatrix3T<float>* srMatrix3T<float>::method_004219F0(
    const srVector3T<float>& first,
    const srVector3T<float>& second,
    const srVector3T<float>& third)
{
    vectors[0] = first;
    vectors[1] = second;
    vectors[2] = third;
    return this;
}

// FUNCTION: WIZ8 0x00421A40
srMatrix3T<float>* srMatrix3T<float>::method_00421A40(
    const srMatrix3T<float>& other)
{
    float result[9];
    float* output = result;
    const float* right = &other.vectors[0].x;
    const float* left = &vectors[0].x;

    for (int index = 0; index != 3; ++index) {
        float x = right[index];
        float y = right[index + 3];
        float z = right[index + 6];

        output[index] = x * left[0] + y * left[1] + z * left[2];
        output[index + 3] = x * left[3] + y * left[4] + z * left[5];
        output[index + 6] = x * left[6] + y * left[7] + z * left[8];
    }
    vectors[0].x = result[0];
    vectors[0].y = result[1];
    vectors[0].z = result[2];
    vectors[1].x = result[3];
    vectors[1].y = result[4];
    vectors[2].x = result[6];
    vectors[1].z = result[5];
    vectors[2].y = result[7];
    vectors[2].z = result[8];
    return this;
}

// FUNCTION: WIZ8 0x00467310
void srMatrix3T<float>::SetIdentity00467310()
{
    vectors[0].x = 1.0f;
    vectors[0].y = 0.0f;
    vectors[0].z = 0.0f;
    vectors[1].x = 0.0f;
    vectors[1].y = 1.0f;
    vectors[1].z = 0.0f;
    vectors[2].x = 0.0f;
    vectors[2].y = 0.0f;
    vectors[2].z = 1.0f;
}

// FUNCTION: WIZ8 0x00438F90
srMatrix3T<float>* srMatrix3T<float>::method_00438F90(double sine, double cosine)
{
    W8CameraMatrixRow004D6930 basis[3];
    srMatrix3T<float> rotation;

    basis[0].x = (float)cosine;
    basis[0].y = 0.0f;
    basis[0].z = (float)sine;
    basis[1].x = 0.0f;
    basis[1].y = 1.0f;
    basis[1].z = 0.0f;
    basis[2].x = -(float)sine;
    basis[2].y = 0.0f;
    basis[2].z = (float)cosine;
    rotation.vectors[0].x = basis[0].x;
    rotation.vectors[0].y = basis[0].y;
    rotation.vectors[0].z = basis[0].z;
    rotation.vectors[1].x = basis[1].x;
    rotation.vectors[1].y = basis[1].y;
    rotation.vectors[1].z = basis[1].z;
    rotation.vectors[2].x = basis[2].x;
    rotation.vectors[2].y = basis[2].y;
    rotation.vectors[2].z = basis[2].z;
    method_00421A40(rotation);
    return this;
}

/* The four-component sibling of 0x00421680, identical in shape: three doubles
   become three floats there, four become four here, and both hand back this.
   The extra argument is what the trailing ret discriminates - 0x18 against
   0x20 - since neither takes a count. */
// FUNCTION: WIZ8 0x004817e0
srVector4T<float>* srVector4T<float>::method_004817E0(
    double source_0,
    double source_1,
    double source_2,
    double source_3)
{
    x = (float)source_0;
    y = (float)source_1;
    z = (float)source_2;
    w = (float)source_3;
    return this;
}

/* The same four-slot store from floats rather than doubles, so the arguments
   are copied rather than converted. The body alone cannot tell a float from an
   int here - a four-byte copy is the same either way - but the call sites can:
   every one of them computes in the FPU and pushes with fstp dword, so the
   parameters are floats. */
// FUNCTION: WIZ8 0x004d6b30
srVector4T<float>* srVector4T<float>::method_004D6B30(
    float source_0,
    float source_1,
    float source_2,
    float source_3)
{
    x = source_0;
    y = source_1;
    z = source_2;
    w = source_3;
    return this;
}
