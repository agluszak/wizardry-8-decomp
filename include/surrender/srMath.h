#pragma once

#include "srHeap.h"

#include <math.h>

/*
 * SurRender math types named by the original SR.DLL export table. The layouts
 * are fixed by the exported srBinIStream operators: vectors store adjacent
 * scalars, and matrices store three or four adjacent vector elements.
 *
 * The callable float bodies in Wiz8.exe are ordinary emissions of these
 * primary templates. They do not establish separately authored float
 * specializations.
 */
template <class T>
class srVector2T {
public:
    srVector2T<T>() {}

    T x;
    T y;
};

template <class T>
class srVector3T {
public:
    srVector3T<T>() {}
    srVector3T<T>(T source_0, T source_1, T source_2)
        : x(source_0), y(source_1), z(source_2) {}

    void* operator new[](unsigned int size)
    {
        return srHeap.allocate(size);
    }

    void operator delete[](void* allocation)
    {
        srHeap.free(allocation);
    }

    void method_00421670();
    srVector3T<T>* method_00421680(
        double source_0, double source_1, double source_2);
    srVector3T<T>& operator+=(const srVector3T<T>& other);
    srVector3T<T>& operator*=(double scalar);
    srVector3T<T>& operator/=(double scalar);
    T method_00421700() const;
    srVector3T<T>* method_00446110(const srVector3T<double>* source);
    srVector3T<T>* method_004A90E0(const srVector3T<float>* source);
    srVector3T<T>* method_00451A10(double sine, double cosine);
    srVector3T<T>* method_0049BA80(double sine, double cosine);

    T x;
    T y;
    T z;
};

template <class T>
void srVector3T<T>::method_00421670()
{
    x = (T)0;
    y = (T)0;
    z = (T)0;
}

template <class T>
srVector3T<T>* srVector3T<T>::method_00421680(
    double source_0,
    double source_1,
    double source_2)
{
    x = (T)source_0;
    y = (T)source_1;
    z = (T)source_2;
    return this;
}

template <class T>
srVector3T<T>& srVector3T<T>::operator+=(const srVector3T<T>& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

template <class T>
srVector3T<T>& srVector3T<T>::operator*=(double scalar)
{
    x = (T)(x * scalar);
    y = (T)(y * scalar);
    z = (T)(z * scalar);
    return *this;
}

template <class T>
srVector3T<T>& srVector3T<T>::operator/=(double scalar)
{
    double reciprocal = 1.0 / scalar;
    x = (T)(x * reciprocal);
    y = (T)(y * reciprocal);
    z = (T)(z * reciprocal);
    return *this;
}

template <class T>
T srVector3T<T>::method_00421700() const
{
    return (T)sqrt(x * x + y * y + z * z);
}

template <class T>
srVector3T<T>* srVector3T<T>::method_00446110(
    const srVector3T<double>* source)
{
    x = (T)source->x;
    y = (T)source->y;
    z = (T)source->z;
    return this;
}

template <class T>
srVector3T<T>* srVector3T<T>::method_00451A10(
    double sine,
    double cosine)
{
    T new_z = (T)(z * cosine - x * sine);
    x = (T)(z * sine + x * cosine);
    z = new_z;
    return this;
}

template <class T>
srVector3T<T>* srVector3T<T>::method_0049BA80(
    double sine,
    double cosine)
{
    T new_z = (T)(z * cosine + y * sine);
    y = (T)(y * cosine - z * sine);
    z = new_z;
    return this;
}

template <class T>
T Function4218E0(
    const srVector3T<T>& first,
    const srVector3T<T>& second)
{
    return first.x * second.x + first.y * second.y + first.z * second.z;
}

template <class T>
srVector3T<T> operator+(
    const srVector3T<T>& first,
    const srVector3T<T>& second)
{
    srVector3T<T> result;
    result.x = first.x + second.x;
    result.y = first.y + second.y;
    result.z = first.z + second.z;
    return result;
}

template <class T>
srVector3T<T> operator*(const srVector3T<T>& vector, double scalar)
{
    srVector3T<T> result;
    result.x = (T)(vector.x * scalar);
    result.y = (T)(vector.y * scalar);
    result.z = (T)(vector.z * scalar);
    return result;
}

template <class T>
srVector3T<T> operator/(const srVector3T<T>& vector, double scalar)
{
    double reciprocal = 1.0 / scalar;
    srVector3T<T> result;
    result.x = (T)(vector.x * reciprocal);
    result.y = (T)(vector.y * reciprocal);
    result.z = (T)(vector.z * reciprocal);
    return result;
}

template <class T>
class srVector4T {
public:
    srVector4T<T>() {}

    srVector4T<T>* method_004817E0(
        double source_0, double source_1, double source_2, double source_3);
    srVector4T<T>* method_004D6B30(
        T source_0, T source_1, T source_2, T source_3);

    T x;
    T y;
    T z;
    T w;
};

template <class T>
srVector4T<T>* srVector4T<T>::method_004817E0(
    double source_0,
    double source_1,
    double source_2,
    double source_3)
{
    x = (T)source_0;
    y = (T)source_1;
    z = (T)source_2;
    w = (T)source_3;
    return this;
}

template <class T>
srVector4T<T>* srVector4T<T>::method_004D6B30(
    T source_0,
    T source_1,
    T source_2,
    T source_3)
{
    x = source_0;
    y = source_1;
    z = source_2;
    w = source_3;
    return this;
}

template <class T>
class srMatrix3T {
public:
    srMatrix3T<T>* method_004219F0(
        const srVector3T<T>& first,
        const srVector3T<T>& second,
        const srVector3T<T>& third);
    srMatrix3T<T>* method_00421A40(const srMatrix3T<T>& other);
    void SetIdentity00467310();
    srMatrix3T<T>* method_00438F90(double sine, double cosine);
    srMatrix3T<T>* method_004A5AB0(double angle);
    srMatrix3T<T>* method_004CAB60(double angle);
    srMatrix3T<T>* method_00478EB0(double sine, double cosine);

    srVector3T<T> vectors[3];
};

template <class T>
srMatrix3T<T>* srMatrix3T<T>::method_004219F0(
    const srVector3T<T>& first,
    const srVector3T<T>& second,
    const srVector3T<T>& third)
{
    vectors[0] = first;
    vectors[1] = second;
    vectors[2] = third;
    return this;
}

template <class T>
srMatrix3T<T>* srMatrix3T<T>::method_00421A40(
    const srMatrix3T<T>& other)
{
    T result[9];
    T* output = result;
    const T* right = &other.vectors[0].x;
    const T* left = &vectors[0].x;

    for (int index = 0; index != 3; ++index) {
        T x = right[index];
        T y = right[index + 3];
        T z = right[index + 6];

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

template <class T>
void srMatrix3T<T>::SetIdentity00467310()
{
    vectors[0].x = (T)1;
    vectors[0].y = (T)0;
    vectors[0].z = (T)0;
    vectors[1].x = (T)0;
    vectors[1].y = (T)1;
    vectors[1].z = (T)0;
    vectors[2].x = (T)0;
    vectors[2].y = (T)0;
    vectors[2].z = (T)1;
}

template <class T>
srMatrix3T<T>* srMatrix3T<T>::method_00438F90(
    double sine,
    double cosine)
{
    srVector3T<T> basis[3];
    srMatrix3T<T> rotation;

    basis[0].x = (T)cosine;
    basis[0].y = (T)0;
    basis[0].z = (T)sine;
    basis[1].x = (T)0;
    basis[1].y = (T)1;
    basis[1].z = (T)0;
    basis[2].x = (T)-sine;
    basis[2].y = (T)0;
    basis[2].z = (T)cosine;
    rotation.vectors[0] = basis[0];
    rotation.vectors[1] = basis[1];
    rotation.vectors[2] = basis[2];
    method_00421A40(rotation);
    return this;
}

template <class T>
srMatrix3T<T>* srMatrix3T<T>::method_00478EB0(
    double sine,
    double cosine)
{
    srVector3T<T> basis[3];
    srMatrix3T<T> rotation;

    basis[0].x = (T)1;
    basis[0].y = (T)0;
    basis[0].z = (T)0;
    basis[1].x = (T)0;
    basis[1].y = (T)cosine;
    basis[1].z = (T)-sine;
    basis[2].x = (T)0;
    basis[2].y = (T)sine;
    basis[2].z = (T)cosine;
    rotation.vectors[0] = basis[0];
    rotation.vectors[1] = basis[1];
    rotation.vectors[2] = basis[2];
    method_00421A40(rotation);
    return this;
}

template <class T>
class srMatrix4T {
public:
    enum e_scaleType {};

    srMatrix4T<T>* InvertMatrix0049BAB0();
    T* Scale0049BD50(double scale);
    void AdjugateFrom0049BF20(T* source);
    T Det0049BDF0() const;

    srVector4T<T> vectors[4];
};

template <class T>
srMatrix4T<T>* srMatrix4T<T>::InvertMatrix0049BAB0()
{
    srMatrix4T<T> inverse;
    inverse.AdjugateFrom0049BF20(&vectors[0].x);

    T determinant = Det0049BDF0();
    if (determinant != (T)1) {
        inverse.Scale0049BD50(1.0 / determinant);
    }

    *this = inverse;
    return this;
}

template <class T>
T* srMatrix4T<T>::Scale0049BD50(double scale)
{
    T* matrix = &vectors[0].x;
    T factor = (T)scale;

    if (factor != (T)1) {
        matrix[0] = factor * matrix[0];
        matrix[1] = factor * matrix[1];
        matrix[2] = factor * matrix[2];
        matrix[3] = factor * matrix[3];
        matrix[4] = factor * matrix[4];
        matrix[5] = factor * matrix[5];
        matrix[6] = factor * matrix[6];
        matrix[7] = factor * matrix[7];
        matrix[8] = factor * matrix[8];
        matrix[9] = factor * matrix[9];
        matrix[10] = factor * matrix[10];
        matrix[11] = factor * matrix[11];
        matrix[12] = factor * matrix[12];
        matrix[13] = factor * matrix[13];
        matrix[14] = factor * matrix[14];
        matrix[15] = factor * matrix[15];
    }
    return matrix;
}

template <class T>
T srMatrix4T<T>::Det0049BDF0() const
{
    const T* matrix = &vectors[0].x;
    T fVar1 = matrix[0];
    T fVar7 = matrix[3];
    T fVar2 = matrix[1];
    T fVar3 = matrix[2];
    T fVar8 = matrix[5];
    T fVar4 = matrix[7];
    T fVar9 = matrix[6];
    T fVar5 = matrix[11];
    T fVar6 = matrix[15];
    T fVar10 = matrix[4];
    T fVar11 = matrix[10];
    T fVar12 = matrix[14];
    T fVar13 = matrix[9];
    T fVar14 = matrix[13];
    T fVar18 = fVar6 * fVar11 - fVar12 * fVar5;
    T fVar15 = matrix[8];
    T fVar16 = matrix[12];
    T fVar17 = fVar6 * fVar13 - fVar14 * fVar5;
    fVar5 = fVar6 * fVar15 - fVar16 * fVar5;
    T fVar22 =
        (fVar15 * fVar14 - fVar16 * fVar13) * fVar9
        + ((fVar13 * fVar12 - fVar14 * fVar11) * fVar10
           - (fVar15 * fVar12 - fVar16 * fVar11) * fVar8);

    return (((fVar17 * fVar10 - fVar5 * fVar8)
                + (fVar14 * fVar15 - fVar16 * fVar13) * fVar4)
               * fVar3
            + (((fVar18 * fVar8 - fVar17 * fVar9)
                    + (fVar12 * fVar13 - fVar14 * fVar11) * fVar4)
                   * fVar1
                - ((fVar18 * fVar10 - fVar5 * fVar9)
                        + (fVar12 * fVar15 - fVar16 * fVar11) * fVar4)
                       * fVar2))
        - fVar22 * fVar7;
}

template <class T>
void srMatrix4T<T>::AdjugateFrom0049BF20(T* source)
{
    T* param_1 = &vectors[0].x;
    T fVar7 = source[6];
    T fVar1 = source[0];
    T fVar2 = source[1];
    T fVar8 = source[7];
    T fVar3 = source[2];
    T fVar4 = source[3];
    T fVar9 = source[8];
    T fVar5 = source[4];
    T fVar6 = source[5];
    T fVar10 = source[9];
    T fVar11 = source[10];
    T fVar12 = source[11];
    T fVar13 = source[12];
    T fVar14 = source[13];
    T fVar15 = source[14];
    T fVar16 = source[15];
    T fVar17 = fVar16 * fVar11 - fVar15 * fVar12;
    T fVar18 = fVar16 * fVar10 - fVar14 * fVar12;
    T fVar19 = fVar15 * fVar10 - fVar14 * fVar11;

    param_1[0] = fVar19 * fVar8 + (fVar17 * fVar6 - fVar18 * fVar7);
    T fVar20 = fVar16 * fVar9 - fVar13 * fVar12;
    T fVar21 = fVar15 * fVar9 - fVar13 * fVar11;
    param_1[4] = -(fVar21 * fVar8 + (fVar17 * fVar5 - fVar20 * fVar7));
    T fVar22 = fVar14 * fVar9 - fVar13 * fVar10;
    param_1[8] = fVar22 * fVar8 + (fVar18 * fVar5 - fVar20 * fVar6);
    param_1[12] = -(fVar22 * fVar7 + (fVar19 * fVar5 - fVar21 * fVar6));
    param_1[1] = -(fVar19 * fVar4 + (fVar17 * fVar2 - fVar18 * fVar3));
    param_1[5] = fVar21 * fVar4 + (fVar17 * fVar1 - fVar20 * fVar3);
    param_1[9] = -(fVar22 * fVar4 + (fVar18 * fVar1 - fVar20 * fVar2));
    param_1[13] = fVar22 * fVar3 + (fVar19 * fVar1 - fVar21 * fVar2);
    fVar19 = fVar16 * fVar7 - fVar15 * fVar8;
    fVar18 = fVar16 * fVar6 - fVar14 * fVar8;
    fVar17 = fVar15 * fVar6 - fVar14 * fVar7;
    param_1[2] = fVar17 * fVar4 + (fVar19 * fVar2 - fVar18 * fVar3);
    fVar16 = fVar16 * fVar5 - fVar13 * fVar8;
    fVar15 = fVar15 * fVar5 - fVar13 * fVar7;
    param_1[6] = -(fVar15 * fVar4 + (fVar19 * fVar1 - fVar16 * fVar3));
    fVar13 = fVar14 * fVar5 - fVar13 * fVar6;
    param_1[10] = fVar13 * fVar4 + (fVar18 * fVar1 - fVar16 * fVar2);
    param_1[14] = -(fVar13 * fVar3 + (fVar17 * fVar1 - fVar15 * fVar2));
    fVar15 = fVar12 * fVar7 - fVar11 * fVar8;
    fVar14 = fVar12 * fVar6 - fVar10 * fVar8;
    fVar13 = fVar11 * fVar6 - fVar10 * fVar7;
    param_1[3] = -(fVar13 * fVar4 + (fVar15 * fVar2 - fVar14 * fVar3));
    fVar8 = fVar12 * fVar5 - fVar9 * fVar8;
    fVar7 = fVar11 * fVar5 - fVar9 * fVar7;
    param_1[7] = fVar7 * fVar4 + (fVar15 * fVar1 - fVar8 * fVar3);
    fVar5 = fVar10 * fVar5 - fVar9 * fVar6;
    param_1[11] = -(fVar5 * fVar4 + (fVar14 * fVar1 - fVar8 * fVar2));
    param_1[15] = fVar5 * fVar3 + (fVar13 * fVar1 - fVar7 * fVar2);
}

float Det3_0049BD00(
    float param_1,
    float param_2,
    float param_3,
    float param_4,
    float param_5,
    float param_6,
    float param_7,
    float param_8,
    float param_9);

template <class T>
class srMatrix4x3T {
public:
    srVector4T<T> rows[3];
};

static_assert(sizeof(srMatrix4x3T<float>) == 0x30,
              "srMatrix4x3T_float_must_be_0x30");
static_assert(sizeof(srMatrix4x3T<double>) == 0x60,
              "srMatrix4x3T_double_must_be_0x60");

class srVector2i {
public:
    int x;
    int y;
};

class srVector3i {
public:
    int x;
    int y;
    int z;
};
