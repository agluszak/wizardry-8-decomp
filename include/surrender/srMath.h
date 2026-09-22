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
template <class T> class srMatrix3T;

template <class T> class srVector2T {
public:
    srVector2T<T>() {}
    srVector2T<T>(T source_0, T source_1) : x(source_0), y(source_1) {}

    srVector2T<T>* Set(T source_0, T source_1)
    {
        x = source_0;
        y = source_1;
        return this;
    }

    void SetZero()
    {
        x = (T)0;
        y = (T)0;
    }

    T Length() const
    {
        return (T)sqrt(x * x + y * y);
    }

    srVector2T<T>& operator*=(double scalar)
    {
        x = (T)(x * scalar);
        y = (T)(y * scalar);
        return *this;
    }

    /* Guarded XZ/2D unitize via reciprocal-sqrt. Independent TUs: OctPath
       0x0045aac0 / 0x0045ef90 / 0x0045BE30. GDCamera SnapToTarget/LookAt and
       ApplyRotationMatrix use a different unguarded 1/Length form and stay
       as Length() then *=. No Wiz8 COMDAT. */
    srVector2T<T>* Normalize()
    {
        T length_squared = x * x + y * y;
        if ((double)length_squared != 0.0) {
            T scale = (T)(1.0 / sqrt((double)length_squared));
            *this *= scale;
        }
        return this;
    }

    srVector2T<T>* SetLength(double length)
    {
        T length_squared = x * x + y * y;
        if ((double)length_squared != 0.0) {
            T scale = (T)(length / sqrt((double)length_squared));
            *this *= scale;
        }
        return this;
    }

    T x;
    T y;
};

template <class T> class srVector3T {
public:
    srVector3T<T>() {}
    srVector3T<T>(T source_0, T source_1, T source_2) : x(source_0), y(source_1), z(source_2) {}

    void* operator new[](unsigned int size)
    {
        return srHeap.allocate(size);
    }

    void operator delete[](void* allocation)
    {
        srHeap.free(allocation);
    }

    void SetZero();
    srVector3T<T>* Set(double source_0, double source_1, double source_2);
    srVector3T<T>& operator=(const srVector3T<double>& source)
    {
        SetFromDouble(&source);
        return *this;
    }
    srVector3T<T>& operator=(T value)
    {
        x = value;
        y = value;
        z = value;
        return *this;
    }
    srVector3T<T>& operator+=(const srVector3T<T>& other);
    srVector3T<T>& operator-=(const srVector3T<T>& other);
    srVector3T<T>& operator*=(double scalar);
    srVector3T<T>& operator/=(double scalar);
    bool operator==(const srVector3T<T>& other) const;
    T Length() const;
    T LengthSquared() const;
    srVector3T<T>* SetFromDouble(const srVector3T<double>* source);
    srVector3T<T>* SetFromFloat(const srVector3T<float>* source);
    void SetSaturated(const srVector3T<T>& source);
    srVector3T<T>* RotateAboutY(double sine, double cosine);
    srVector3T<T>* RotateAboutX(double sine, double cosine);
    srVector3T<T>* RotateAboutZ(double sine, double cosine);
    srVector3T<T>* Normalize();
    srVector3T<T>* SetLength(double length);
    srVector3T<T>* Unitize();
    srVector3T<T>& Transform(const srMatrix3T<T>& matrix);

    T x;
    T y;
    T z;
};

// TEMPLATE: WIZ8 0x00421670
// srVector3T<float>::SetZero
template <class T> void srVector3T<T>::SetZero()
{
    x = (T)0;
    y = (T)0;
    z = (T)0;
}

// TEMPLATE: WIZ8 0x00421680
// srVector3T<float>::Set
template <class T>
srVector3T<T>* srVector3T<T>::Set(double source_0, double source_1, double source_2)
{
    x = (T)source_0;
    y = (T)source_1;
    z = (T)source_2;
    return this;
}

template <class T> srVector3T<T>& srVector3T<T>::operator+=(const srVector3T<T>& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

template <class T> srVector3T<T>& srVector3T<T>::operator-=(const srVector3T<T>& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

template <class T> srVector3T<T>& srVector3T<T>::operator*=(double scalar)
{
    x = (T)(x * scalar);
    y = (T)(y * scalar);
    z = (T)(z * scalar);
    return *this;
}

template <class T> srVector3T<T>& srVector3T<T>::operator/=(double scalar)
{
    double reciprocal = 1.0 / scalar;
    x = (T)(x * reciprocal);
    y = (T)(y * reciprocal);
    z = (T)(z * reciprocal);
    return *this;
}

/* Guarded vec3 → length 1 via reciprocal-sqrt of the squared length.
   Independent TUs: stParticle 0x00499A50, OctPath 0x0045b730 / 0x0045e840 /
   0x00465130, GDCamera 0x00476950 / 0x00476F90. Prop 0x0044aee0 goes through
   srModelInstance::setAlignAxis, whose body is this same form with z,y,x
   square order. No Wiz8 COMDAT; header-visible inlining is the retail shape.

   ReadLevel 0x004BD0D0 is a second unitize family (Length(); if != 0; /=)
   and keeps that authored form. Do not force this reciprocal-sqrt body there. */
template <class T> srVector3T<T>* srVector3T<T>::Normalize()
{
    T length_squared = x * x + y * y + z * z;
    if ((double)length_squared != 0.0) {
        T scale = (T)(1.0 / sqrt((double)length_squared));
        *this *= scale;
    }
    return this;
}

/* Guarded vec3 → requested magnitude: desired / sqrt(len²). Independent TUs:
   GDCamera::GetForwardPoint 0x00478CE0 and OctPath 0x00462570 / 0x00465D70.
   Separate from Normalize(); the 1.0 vs requested-length constant is authored. */
template <class T> srVector3T<T>* srVector3T<T>::SetLength(double length)
{
    T length_squared = x * x + y * y + z * z;
    if ((double)length_squared != 0.0) {
        T scale = (T)(length / sqrt((double)length_squared));
        *this *= scale;
    }
    return this;
}

/* Length(); if != 0; /=. Distinct from the reciprocal-sqrt Normalize().
   ReadLevel 0x004BD0D0 is the recovered site. Original spelling unknown. */
template <class T> srVector3T<T>* srVector3T<T>::Unitize()
{
    T length = Length();
    if (length != (T)0) {
        *this /= length;
    }
    return this;
}

// TEMPLATE: WIZ8 0x00421700
// srVector3T<float>::Length
template <class T> T srVector3T<T>::Length() const
{
    return (T)sqrt(x * x + y * y + z * z);
}

/* Squared length without sqrt. Independent TUs: Navigator UpdateLinkedNavigator
   0x00454D70, Octree MarkVisibleRegions 0x004301C0, OctPath nearest-trigger
   0x00463xxx. Original spelling unknown. */
template <class T> T srVector3T<T>::LengthSquared() const
{
    return x * x + y * y + z * z;
}

template <class T> srVector3T<T>* srVector3T<T>::SetFromDouble(const srVector3T<double>* source)
{
    x = (T)source->x;
    y = (T)source->y;
    z = (T)source->z;
    return this;
}

template <class T> srVector3T<T>* srVector3T<T>::SetFromFloat(const srVector3T<float>* source)
{
    x = (T)source->x;
    y = (T)source->y;
    z = (T)source->z;
    return this;
}

// TEMPLATE: WIZ8 0x004258B0
// srVector3T<float>::SetSaturated
template <class T> void srVector3T<T>::SetSaturated(const srVector3T<T>& source)
{
    x = (T)source.x;
    y = (T)source.y;
    z = (T)source.z;
    if (x <= 0.0f)
        x = 0;
    else if (x >= 1.0f)
        x = 1.0f;
    if (y <= 0.0f)
        y = 0;
    else if (y >= 1.0f)
        y = 1.0f;
    if (z <= 0.0f)
        z = 0;
    else if (z >= 1.0f)
        z = 1.0f;
}

// TEMPLATE: WIZ8 0x00451A10
// srVector3T<float>::RotateAboutY
template <class T> srVector3T<T>* srVector3T<T>::RotateAboutY(double sine, double cosine)
{
    T new_z = (T)(z * cosine - x * sine);
    x = (T)(z * sine + x * cosine);
    z = new_z;
    return this;
}

// TEMPLATE: WIZ8 0x0049BA80
// srVector3T<float>::RotateAboutX
template <class T> srVector3T<T>* srVector3T<T>::RotateAboutX(double sine, double cosine)
{
    T new_z = (T)(z * cosine + y * sine);
    y = (T)(y * cosine - z * sine);
    z = new_z;
    return this;
}

template <class T> srVector3T<T>* srVector3T<T>::RotateAboutZ(double sine, double cosine)
{
    T new_x = (T)(x * cosine - y * sine);
    y = (T)(x * sine + y * cosine);
    x = new_x;
    return this;
}

template <class T> T DotProduct(const srVector3T<T>& first, const srVector3T<T>& second)
{
    return first.x * second.x + first.y * second.y + first.z * second.z;
}

/* Ordinary edge×edge cross. OctPath GetPathSurfaceNormal 0x0045b730 expands
   this; the Newell cyclic sum in the plane builders is a different helper. */
template <class T>
srVector3T<T> CrossProduct(const srVector3T<T>& first, const srVector3T<T>& second)
{
    srVector3T<T> result;
    result.x = first.y * second.z - first.z * second.y;
    result.y = first.z * second.x - first.x * second.z;
    result.z = first.x * second.y - first.y * second.x;
    return result;
}

template <class T> srVector3T<T> operator+(const srVector3T<T>& first, const srVector3T<T>& second)
{
    srVector3T<T> result;
    result.x = first.x + second.x;
    result.y = first.y + second.y;
    result.z = first.z + second.z;
    return result;
}

template <class T> srVector3T<T> operator-(const srVector3T<T>& first, const srVector3T<T>& second)
{
    srVector3T<T> result;
    result.x = first.x - second.x;
    result.y = first.y - second.y;
    result.z = first.z - second.z;
    return result;
}

/* Component negation: the slerp sign fix in PathAIApply 0x004AA520 and Prop
   0x0044C830. Independent emission at 0x0044EE70. */
template <class T> srVector3T<T> operator-(const srVector3T<T>& vector)
{
    srVector3T<T> result;
    result.x = -vector.x;
    result.y = -vector.y;
    result.z = -vector.z;
    return result;
}

/* Element equality: the same slerps gate interpolation on the two keyframe
   rotations differing. Independent emission at 0x0044EC60. */
template <class T> bool srVector3T<T>::operator==(const srVector3T<T>& other) const
{
    return x == other.x && y == other.y && z == other.z;
}

template <class T> srVector3T<T> operator*(const srVector3T<T>& vector, double scalar)
{
    srVector3T<T> result;
    result.x = (T)(vector.x * scalar);
    result.y = (T)(vector.y * scalar);
    result.z = (T)(vector.z * scalar);
    return result;
}

/* Scalar-first form: the slerps in PathAIApply 0x004AA520 and Prop 0x0044C830
   push scalar then vector, matching the independent emission at 0x0044EEB0. */
template <class T> srVector3T<T> operator*(double scalar, const srVector3T<T>& vector)
{
    srVector3T<T> result;
    result.x = (T)(vector.x * scalar);
    result.y = (T)(vector.y * scalar);
    result.z = (T)(vector.z * scalar);
    return result;
}

template <class T> srVector3T<T> operator/(const srVector3T<T>& vector, double scalar)
{
    double reciprocal = 1.0 / scalar;
    srVector3T<T> result;
    result.x = (T)(vector.x * reciprocal);
    result.y = (T)(vector.y * reciprocal);
    result.z = (T)(vector.z * reciprocal);
    return result;
}

template <class T> class srVector4T {
public:
    srVector4T<T>() {}

    srVector4T<T>* Set(T source_0, T source_1, T source_2, T source_3);
    T Length() const;
    srVector4T<T>& operator*=(double scalar);
    srVector4T<T>& operator=(T value)
    {
        x = value;
        y = value;
        z = value;
        w = value;
        return *this;
    }

    T x;
    T y;
    T z;
    T w;
};

// TEMPLATE: WIZ8 0x004D6B30
// srVector4T<float>::Set
template <class T> srVector4T<T>* srVector4T<T>::Set(T source_0, T source_1, T source_2, T source_3)
{
    x = source_0;
    y = source_1;
    z = source_2;
    w = source_3;
    return this;
}

template <class T> T srVector4T<T>::Length() const
{
    return (T)sqrt(x * x + y * y + z * z + w * w);
}

template <class T> srVector4T<T>& srVector4T<T>::operator*=(double scalar)
{
    x = (T)(x * scalar);
    y = (T)(y * scalar);
    z = (T)(z * scalar);
    w = (T)(w * scalar);
    return *this;
}

template <class T> class srMatrix2T {
public:
    srMatrix2T<T>* MultiplyBy(const srMatrix2T<T>& other);

    srVector2T<T> vectors[2];
};

// TEMPLATE: WIZ8 0x004D6B80
// srMatrix2T<float>::MultiplyBy
template <class T> srMatrix2T<T>* srMatrix2T<T>::MultiplyBy(const srMatrix2T<T>& other)
{
    T result[4];
    const T* right = &other.vectors[0].x;
    const T* left = &vectors[0].x;

    for (int index = 0; index != 2; ++index) {
        T x = right[index];
        T y = right[index + 2];

        result[index] = x * left[0] + y * left[1];
        result[index + 2] = x * left[2] + y * left[3];
    }
    vectors[0].x = result[0];
    vectors[0].y = result[1];
    vectors[1].x = result[2];
    vectors[1].y = result[3];
    return this;
}

template <class T> class srMatrix3T {
public:
    srMatrix3T<T>* SetRows(const srVector3T<T>& first, const srVector3T<T>& second,
                           const srVector3T<T>& third);
    srMatrix3T<T>* MultiplyBy(const srMatrix3T<T>& other);
    void SetIdentity();
    srMatrix3T<T>* RotateAboutY(double sine, double cosine);
    srMatrix3T<T>* RotateAboutX(double sine, double cosine);
    srMatrix3T<T>* RotateAboutZ(double sine, double cosine);
    srMatrix3T<T>* RotateAroundAxis(double sine, double cosine, const srVector3T<T>& axis);
    /* The single-angle overloads evaluate the trigonometry themselves. */
    srMatrix3T<T>* RotateAboutY(double angle);
    srMatrix3T<T>* RotateAboutX(double angle);
    srMatrix3T<T>* RotateAboutZ(double angle);
    srMatrix3T<T>* RotateAroundAxis(double angle, const srVector3T<T>& axis);
    srVector3T<T> Transform(const srVector3T<T>& value) const;
    /* Column products: result = M^T * value. */
    srVector3T<T> TransformTransposed(const srVector3T<T>& value) const;
    bool operator==(const srMatrix3T<T>& other) const;

    srVector3T<T> vectors[3];
};

// TEMPLATE: WIZ8 0x004219F0
// srMatrix3T<float>::SetRows
template <class T>
srMatrix3T<T>* srMatrix3T<T>::SetRows(const srVector3T<T>& first, const srVector3T<T>& second,
                                      const srVector3T<T>& third)
{
    vectors[0] = first;
    vectors[1] = second;
    vectors[2] = third;
    return this;
}

// TEMPLATE: WIZ8 0x00421A40
// srMatrix3T<float>::MultiplyBy
template <class T> srMatrix3T<T>* srMatrix3T<T>::MultiplyBy(const srMatrix3T<T>& other)
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

template <class T> void srMatrix3T<T>::SetIdentity()
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

/* Row-wise equality: the keyframe slerps compare the current and next
   rotation records before interpolating. No retail out-of-line copy; every
   site inlines the three vector compares. */
template <class T> bool srMatrix3T<T>::operator==(const srMatrix3T<T>& other) const
{
    return vectors[0] == other.vectors[0] && vectors[1] == other.vectors[1] &&
           vectors[2] == other.vectors[2];
}

// TEMPLATE: WIZ8 0x00438F90
// srMatrix3T<float>::RotateAboutY
template <class T> srMatrix3T<T>* srMatrix3T<T>::RotateAboutY(double sine, double cosine)
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
    MultiplyBy(rotation);
    return this;
}

// TEMPLATE: WIZ8 0x00478EB0
// srMatrix3T<float>::RotateAboutX
template <class T> srMatrix3T<T>* srMatrix3T<T>::RotateAboutX(double sine, double cosine)
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
    MultiplyBy(rotation);
    return this;
}

template <class T> srMatrix3T<T>* srMatrix3T<T>::RotateAboutZ(double sine, double cosine)
{
    srVector3T<T> basis[3];
    srMatrix3T<T> rotation;

    basis[0].x = (T)cosine;
    basis[0].y = (T)-sine;
    basis[0].z = (T)0;
    basis[1].x = (T)sine;
    basis[1].y = (T)cosine;
    basis[1].z = (T)0;
    basis[2].x = (T)0;
    basis[2].y = (T)0;
    basis[2].z = (T)1;
    rotation.vectors[0] = basis[0];
    rotation.vectors[1] = basis[1];
    rotation.vectors[2] = basis[2];
    MultiplyBy(rotation);
    return this;
}

// srMatrix3T<float>::RotateAboutY
template <class T> srMatrix3T<T>* srMatrix3T<T>::RotateAboutY(double angle)
{
    srVector3T<T> basis[3];
    srMatrix3T<T> rotation;
    T cosine;
    T sine;

    if (angle != 0.0) {
        cosine = (T)cos(angle);
        sine = (T)sin(angle);
        basis[0].x = cosine;
        basis[0].y = (T)0;
        basis[0].z = sine;
        basis[1].x = (T)0;
        basis[1].y = (T)1;
        basis[1].z = (T)0;
        basis[2].x = -sine;
        basis[2].y = (T)0;
        basis[2].z = cosine;
        rotation.vectors[0] = basis[0];
        rotation.vectors[1] = basis[1];
        rotation.vectors[2] = basis[2];
        MultiplyBy(rotation);
    }
    return this;
}

// srMatrix3T<float>::RotateAboutX
template <class T> srMatrix3T<T>* srMatrix3T<T>::RotateAboutX(double angle)
{
    srVector3T<T> basis[3];
    srMatrix3T<T> rotation;
    T cosine;
    T sine;

    if (angle != 0.0) {
        cosine = (T)cos(angle);
        sine = (T)sin(angle);
        basis[0].x = (T)1;
        basis[0].y = (T)0;
        basis[0].z = (T)0;
        basis[1].x = (T)0;
        basis[1].y = cosine;
        basis[1].z = -sine;
        basis[2].x = (T)0;
        basis[2].y = sine;
        basis[2].z = cosine;
        rotation.vectors[0] = basis[0];
        rotation.vectors[1] = basis[1];
        rotation.vectors[2] = basis[2];
        MultiplyBy(rotation);
    }
    return this;
}

// TEMPLATE: WIZ8 0x004CAB60
// srMatrix3T<float>::RotateAboutZ
template <class T> srMatrix3T<T>* srMatrix3T<T>::RotateAboutZ(double angle)
{
    srVector3T<T> basis[3];
    srMatrix3T<T> rotation;
    T cosine;
    T sine;

    if (angle != 0.0) {
        cosine = (T)cos(angle);
        sine = (T)sin(angle);
        basis[0].x = cosine;
        basis[0].y = -sine;
        basis[0].z = (T)0;
        basis[1].x = sine;
        basis[1].y = cosine;
        basis[1].z = (T)0;
        basis[2].x = (T)0;
        basis[2].y = (T)0;
        basis[2].z = (T)1;
        rotation.vectors[0] = basis[0];
        rotation.vectors[1] = basis[1];
        rotation.vectors[2] = basis[2];
        MultiplyBy(rotation);
    }
    return this;
}

/* Single-angle overload of the Rodrigues rotation above, keeping the
   trigonometry and the basis products in double precision until the float
   stores. The MartensBluff2 arrow trap emits it at 0x004DE940. */
// TEMPLATE: WIZ8 0x004DE940
// srMatrix3T<float>::RotateAroundAxis(double, const srVector3T<float>&)
template <class T>
srMatrix3T<T>* srMatrix3T<T>::RotateAroundAxis(double angle, const srVector3T<T>& axis)
{
    srVector3T<T> basis[3];
    srMatrix3T<T> rotation;
    double sine;
    double cosine;
    double one_minus_cosine;

    if (angle != 0.0) {
        cosine = cos(angle);
        sine = sin(angle);
        one_minus_cosine = 1.0 - cosine;
        basis[0].x = (T)(axis.x * axis.x + ((T)1 - axis.x * axis.x) * cosine);
        basis[0].y = (T)(axis.x * axis.y * one_minus_cosine - axis.z * sine);
        basis[0].z = (T)(axis.x * axis.z * one_minus_cosine + axis.y * sine);
        basis[1].x = (T)(axis.y * axis.x * one_minus_cosine + axis.z * sine);
        basis[1].y = (T)(axis.y * axis.y + ((T)1 - axis.y * axis.y) * cosine);
        basis[1].z = (T)(axis.y * axis.z * one_minus_cosine - axis.x * sine);
        basis[2].x = (T)(axis.z * axis.x * one_minus_cosine - axis.y * sine);
        basis[2].y = (T)(axis.z * axis.y * one_minus_cosine + axis.x * sine);
        basis[2].z = (T)(axis.z * axis.z + ((T)1 - axis.z * axis.z) * cosine);
        rotation.vectors[0] = basis[0];
        rotation.vectors[1] = basis[1];
        rotation.vectors[2] = basis[2];
        MultiplyBy(rotation);
    }
    return this;
}

// TEMPLATE: WIZ8 0x0042B910
// srMatrix3T<float>::RotateAroundAxis(double, double, const srVector3T<float>&)
template <class T>
srMatrix3T<T>* srMatrix3T<T>::RotateAroundAxis(double sine, double cosine,
                                               const srVector3T<T>& axis)
{
    srVector3T<T> basis[3];
    srMatrix3T<T> rotation;
    T one_minus_cosine = (T)1 - (T)cosine;

    basis[0].x = axis.x * axis.x + ((T)1 - axis.x * axis.x) * (T)cosine;
    basis[0].y = axis.x * axis.y * one_minus_cosine - axis.z * (T)sine;
    basis[0].z = axis.x * axis.z * one_minus_cosine + axis.y * (T)sine;
    basis[1].x = axis.y * axis.x * one_minus_cosine + axis.z * (T)sine;
    basis[1].y = axis.y * axis.y + ((T)1 - axis.y * axis.y) * (T)cosine;
    basis[1].z = axis.y * axis.z * one_minus_cosine - axis.x * (T)sine;
    basis[2].x = axis.z * axis.x * one_minus_cosine - axis.y * (T)sine;
    basis[2].y = axis.z * axis.y * one_minus_cosine + axis.x * (T)sine;
    basis[2].z = axis.z * axis.z + ((T)1 - axis.z * axis.z) * (T)cosine;
    rotation.vectors[0] = basis[0];
    rotation.vectors[1] = basis[1];
    rotation.vectors[2] = basis[2];
    MultiplyBy(rotation);
    return this;
}

/* Row-wise matrix×vector: out.i = DotProduct(row_i, value). Independent TUs:
   SoundEvent 0x004d5a10, Spells, Environment 0x00482a20, ReadLevel 0x004BD0D0,
   OctPath, Trigger, stParticle, GrCycle, GDCamera GetForwardPoint 0x00478CE0.
   Return-by-value Transform is the adopted ABI; operator* and void
   Transform(in, out) were trialed and did not match the surrounding stores.
   No Wiz8 COMDAT; header-visible inlining is the retail shape. Partial row-0
   multiply-add vs DotProduct on rows 1/2 is inlining/scheduling, not a
   different helper. */
template <class T> srVector3T<T> srMatrix3T<T>::Transform(const srVector3T<T>& value) const
{
    srVector3T<T> result;
    result.x = DotProduct(vectors[0], value);
    result.y = DotProduct(vectors[1], value);
    result.z = DotProduct(vectors[2], value);
    return result;
}

/* The transposed product: each result component is a column dot, so
   result = M^T * value. Emitted standalone at 0x004ED950 for the Combat.cpp
   breath-effect direction rotation. */
// TEMPLATE: WIZ8 0x004ed950
// srMatrix3T<float>::TransformTransposed
template <class T>
srVector3T<T> srMatrix3T<T>::TransformTransposed(const srVector3T<T>& value) const
{
    srVector3T<T> result;
    result.x = vectors[0].x * value.x + vectors[1].x * value.y + vectors[2].x * value.z;
    result.y = vectors[0].y * value.x + vectors[1].y * value.y + vectors[2].y * value.z;
    result.z = vectors[0].z * value.x + vectors[1].z * value.y + vectors[2].z * value.z;
    return result;
}

/* In-place vector×matrix: *this = matrix.Transform(*this). GetForwardPoint
   0x00478CE0 overwrites m_direction_078 this way. Same row DotProduct body
   as srMatrix3T::Transform. */
template <class T> srVector3T<T>& srVector3T<T>::Transform(const srMatrix3T<T>& matrix)
{
    T x = DotProduct(matrix.vectors[0], *this);
    T y = DotProduct(matrix.vectors[1], *this);
    T z = DotProduct(matrix.vectors[2], *this);
    this->x = x;
    this->y = y;
    this->z = z;
    return *this;
}

template <class T> class srMatrix4T {
public:
    /* classifyMatrix on the model-view stack writes these from the 3x3
       column lengths. Original enumerator spellings are not in the binary. */
    enum e_scaleType {
        SCALE_TYPE_POSITIONAL_0 = 0, /* equal column lengths, all ~1 */
        SCALE_TYPE_POSITIONAL_1 = 1, /* equal column lengths, not 1 */
        SCALE_TYPE_POSITIONAL_2 = 2  /* unequal column lengths */
    };
    enum e_type {};

    srMatrix4T<T>* Invert();
    T* Scale(double scale);
    void AdjugateFrom(T* source);
    srMatrix4T<T>* Set(const srMatrix3T<T>& rotation, const srVector3T<T>& translation);
    T Det() const;
    srVector3T<T> TransformPoint(const srVector3T<T>& point) const;
    srVector3T<T> TransformDirection(const srVector3T<T>& direction) const;
    srVector4T<T> Transform(const srVector3T<T>& point) const;

    srVector4T<T> vectors[4];
};

// TEMPLATE: WIZ8 0x0049BAB0
// srMatrix4T<float>::Invert
template <class T> srMatrix4T<T>* srMatrix4T<T>::Invert()
{
    srMatrix4T<T> inverse;
    inverse.AdjugateFrom(&vectors[0].x);

    T determinant = Det();
    if (determinant != 1.0) {
        inverse.Scale(1.0 / determinant);
    }

    *this = inverse;
    return this;
}

/* Affine point transform: dest.i = row_i.xyz·point + row_i.w. Independent
   TUs: GDProp::Initialize 0x004b7060. Particle PrepareRenderer 0x00498DD0
   and Update 0x00499FA0 use the four-component form (Transform) because
   retail evaluates row 3. No Wiz8 COMDAT. */
template <class T> srVector3T<T> srMatrix4T<T>::TransformPoint(const srVector3T<T>& point) const
{
    srVector3T<T> result;
    result.x =
        vectors[0].x * point.x + vectors[0].y * point.y + vectors[0].z * point.z + vectors[0].w;
    result.y =
        vectors[1].x * point.x + vectors[1].y * point.y + vectors[1].z * point.z + vectors[1].w;
    result.z =
        vectors[2].x * point.x + vectors[2].y * point.y + vectors[2].z * point.z + vectors[2].w;
    return result;
}

/* Linear 3×3 of a 4×4: dest.i = row_i.xyz·direction, no translation.
   Monster GetCycleMappedPosition 0x004C7960 adds owner position separately.
   Distinct from TransformPoint, which adds row .w. */
template <class T>
srVector3T<T> srMatrix4T<T>::TransformDirection(const srVector3T<T>& direction) const
{
    srVector3T<T> result;
    result.x = vectors[0].x * direction.x + vectors[0].y * direction.y + vectors[0].z * direction.z;
    result.y = vectors[1].x * direction.x + vectors[1].y * direction.y + vectors[1].z * direction.z;
    result.z = vectors[2].x * direction.x + vectors[2].y * direction.y + vectors[2].z * direction.z;
    return result;
}

template <class T> srVector4T<T> srMatrix4T<T>::Transform(const srVector3T<T>& point) const
{
    srVector4T<T> result;
    result.Set(
        vectors[0].x * point.x + vectors[0].y * point.y + vectors[0].z * point.z + vectors[0].w,
        vectors[1].x * point.x + vectors[1].y * point.y + vectors[1].z * point.z + vectors[1].w,
        vectors[2].x * point.x + vectors[2].y * point.y + vectors[2].z * point.z + vectors[2].w,
        vectors[3].x * point.x + vectors[3].y * point.y + vectors[3].z * point.z + vectors[3].w);
    return result;
}

template <class T> T* srMatrix4T<T>::Scale(double scale)
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

template <class T> T srMatrix4T<T>::Det() const
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
    T fVar22 = (fVar15 * fVar14 - fVar16 * fVar13) * fVar9 +
               ((fVar13 * fVar12 - fVar14 * fVar11) * fVar10 -
                (fVar15 * fVar12 - fVar16 * fVar11) * fVar8);

    return (((fVar17 * fVar10 - fVar5 * fVar8) + (fVar14 * fVar15 - fVar16 * fVar13) * fVar4) *
                fVar3 +
            (((fVar18 * fVar8 - fVar17 * fVar9) + (fVar12 * fVar13 - fVar14 * fVar11) * fVar4) *
                 fVar1 -
             ((fVar18 * fVar10 - fVar5 * fVar9) + (fVar12 * fVar15 - fVar16 * fVar11) * fVar4) *
                 fVar2)) -
           fVar22 * fVar7;
}

template <class T> void srMatrix4T<T>::AdjugateFrom(T* source)
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

/* Build the homogeneous transform whose upper 3x3 is `rotation` and whose
   last column is `translation`; the bottom row is (0,0,0,1). Wiz8's
   BakeInstanceVertexLighting calls this before bulk-transforming vertices. */
// srMatrix4T<float>::Set
template <class T>
srMatrix4T<T>* srMatrix4T<T>::Set(const srMatrix3T<T>& rotation, const srVector3T<T>& translation)
{
    srVector4T<T> target;
    const T* component = &translation.x;
    for (int row = 0; row < 3; ++row) {
        target.Set(rotation.vectors[row].x, rotation.vectors[row].y, rotation.vectors[row].z,
                   *component++);
        vectors[row] = target;
    }
    target.Set((T)0, (T)0, (T)0, (T)1);
    vectors[3] = target;
    return this;
}

float Det3(float param_1, float param_2, float param_3, float param_4, float param_5, float param_6,
           float param_7, float param_8, float param_9);

/* A 3×4 affine transform: three rows of (basis xyz, translation w). srNode
   composes its authored local rotation/location/scale into this cached world
   transform and exports getWorldSpaceMatrix overloads for both this type and
   the homogeneous srMatrix4T expansion. Wiz8.exe itself only imports the
   float 4x4 getter; the 4x3 form is the SurRender cache and SR.DLL API.
   getWorldSpaceMatrix memcpy's the 12-float / 12-double cache; pushMultMatrix
   expands the affine multiply itself. The only float member emissions in
   Wiz8.exe are the translation/scale writes inside GDProp's
   TransformMeshGeometry004B7E50. */
template <class T> class srMatrix4x3T {
public:
    /* sr.dll emits out-of-line copies at 0x10055710 for double and 0x10055770
       for float; srNode's constructor calls both. */
    void SetIdentity();
    void SetRotation(const srMatrix3T<T>& rotation);
    srMatrix4x3T<T>* SetTranslation(const srVector3T<T>& translation);
    srMatrix4x3T<T>* Scale(const srVector3T<T>& scale);
    srVector3T<T> TransformPoint(const srVector3T<T>& point) const;

    srVector4T<T> rows[3];
};

// TEMPLATE: SURRENDER 0x10055710
// srMatrix4x3T<double>::SetIdentity
// TEMPLATE: SURRENDER 0x10055770
// srMatrix4x3T<float>::SetIdentity
template <class T> void srMatrix4x3T<T>::SetIdentity()
{
    rows[0].x = static_cast<T>(1);
    rows[0].y = static_cast<T>(0);
    rows[0].z = static_cast<T>(0);
    rows[0].w = static_cast<T>(0);
    rows[1].x = static_cast<T>(0);
    rows[1].y = static_cast<T>(1);
    rows[1].z = static_cast<T>(0);
    rows[1].w = static_cast<T>(0);
    rows[2].x = static_cast<T>(0);
    rows[2].y = static_cast<T>(0);
    rows[2].z = static_cast<T>(1);
    rows[2].w = static_cast<T>(0);
}

template <class T> void srMatrix4x3T<T>::SetRotation(const srMatrix3T<T>& rotation)
{
    rows[0].x = rotation.vectors[0].x;
    rows[0].y = rotation.vectors[0].y;
    rows[0].z = rotation.vectors[0].z;
    rows[1].x = rotation.vectors[1].x;
    rows[1].y = rotation.vectors[1].y;
    rows[1].z = rotation.vectors[1].z;
    rows[2].x = rotation.vectors[2].x;
    rows[2].y = rotation.vectors[2].y;
    rows[2].z = rotation.vectors[2].z;
}

// srMatrix4x3T<float>::SetTranslation
template <class T>
srMatrix4x3T<T>* srMatrix4x3T<T>::SetTranslation(const srVector3T<T>& translation)
{
    rows[0].w = translation.x;
    rows[1].w = translation.y;
    rows[2].w = translation.z;
    return this;
}

// srMatrix4x3T<float>::Scale
template <class T> srMatrix4x3T<T>* srMatrix4x3T<T>::Scale(const srVector3T<T>& scale)
{
    rows[0].x *= scale.x;
    rows[1].x *= scale.x;
    rows[2].x *= scale.x;
    rows[0].y *= scale.y;
    rows[1].y *= scale.y;
    rows[2].y *= scale.y;
    rows[0].z *= scale.z;
    rows[1].z *= scale.z;
    rows[2].z *= scale.z;
    return this;
}

/* Affine point transform: dest.i = row_i.xyz·point + row_i.w. The only Wiz8
   emission is inlined inside GDProp::TransformMeshGeometry004B7E50. */
template <class T> srVector3T<T> srMatrix4x3T<T>::TransformPoint(const srVector3T<T>& point) const
{
    srVector3T<T> result;
    result.x = rows[0].x * point.x + rows[0].y * point.y + rows[0].z * point.z + rows[0].w;
    result.y = rows[1].x * point.x + rows[1].y * point.y + rows[1].z * point.z + rows[1].w;
    result.z = rows[2].x * point.x + rows[2].y * point.y + rows[2].z * point.z + rows[2].w;
    return result;
}

static_assert(sizeof(srMatrix4x3T<float>) == 0x30, "srMatrix4x3T_float_must_be_0x30");
static_assert(sizeof(srMatrix4x3T<double>) == 0x60, "srMatrix4x3T_double_must_be_0x60");
static_assert(sizeof(srMatrix2T<float>) == 0x10, "srMatrix2T_float_must_be_0x10");
static_assert(sizeof(srMatrix2T<double>) == 0x20, "srMatrix2T_double_must_be_0x20");

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

class srVector4i {
public:
    int x;
    int y;
    int z;
    int w;
};

static_assert(sizeof(srVector4i) == 0x10, "srVector4i_must_be_0x10");

/* SurRender's quaternion value type. The exported srBinIStream extraction
   operator (0x10031AF0) reads a scalar w followed by a 3-vector v, fixing the
   member order below. */
class srQuaternion {
public:
    float w;
    srVector3T<float> v;
};

static_assert(sizeof(srQuaternion) == 0x10, "srQuaternion_must_be_0x10");
