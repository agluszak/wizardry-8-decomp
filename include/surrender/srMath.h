#pragma once

#include "srHeap.h"

#include <float.h>
#include <math.h>

inline int srFinite(double value)
{
    return _finite(value);
}

template <class T> class srMatrix3T;

template <class T> class srVector2T {
public:
    srVector2T<T>() {}
    srVector2T<T>(T source_0, T source_1) : x(source_0), y(source_1) {}

    void* operator new[](unsigned int size)
    {
        return srHeap.allocate(size);
    }

    void operator delete[](void* allocation)
    {
        srHeap.free(allocation);
    }

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

    int isValid() const
    {
        return _finite(static_cast<double>(x)) && _finite(static_cast<double>(y));
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
    srVector3T<T>();
    srVector3T<T>(T source_0, T source_1, T source_2) : x(source_0), y(source_1), z(source_2) {}

    srVector2T<T> xz() const
    {
        return srVector2T<T>(x, z);
    }

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

    template <class U> srVector3T<T>& operator=(const srVector3T<U>& source)
    {
        x = (T)source.x;
        y = (T)source.y;
        z = (T)source.z;
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

    srVector3T<T>& operator*=(const srVector3T<T>& other);
    srVector3T<T>& operator*=(double scalar);
    srVector3T<T>& operator/=(double scalar);
    bool operator==(const srVector3T<T>& other) const;
    T Length() const;
    T LengthSquared() const;

    T length() const
    {
        return (T)sqrt(x * x + y * y + z * z);
    }
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

    int isValid() const
    {
        return _finite(static_cast<double>(x)) && _finite(static_cast<double>(y)) &&
               _finite(static_cast<double>(z));
    }

    T x;
    T y;
    T z;
};

template <class T> srVector3T<T>::srVector3T() {}

template <class T> void srVector3T<T>::SetZero()
{
    x = (T)0;
    y = (T)0;
    z = (T)0;
}

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

template <class T> srVector3T<T>& srVector3T<T>::operator*=(const srVector3T<T>& other)
{
    x = other.x * x;
    y = other.y * y;
    z = other.z * z;
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

template <class T> T srVector3T<T>::Length() const
{
    return (T)sqrt(x * x + y * y + z * z);
}

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

template <class T> srVector3T<T>* srVector3T<T>::RotateAboutY(double sine, double cosine)
{
    T new_z = (T)(z * cosine - x * sine);
    x = (T)(z * sine + x * cosine);
    z = new_z;
    return this;
}

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

template <class T> srVector3T<T> operator-(const srVector3T<T>& vector)
{
    srVector3T<T> result;
    result.x = -vector.x;
    result.y = -vector.y;
    result.z = -vector.z;
    return result;
}

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

template <class T> srVector3T<T> operator*(double scalar, const srVector3T<T>& vector)
{
    srVector3T<T> result;
    result.x = (T)(vector.x * scalar);
    result.y = (T)(vector.y * scalar);
    result.z = (T)(vector.z * scalar);
    return result;
}

template <class T> srVector3T<T> operator*(const srVector3T<T>& first, const srVector3T<T>& second)
{
    srVector3T<T> result;
    result.x = first.x * second.x;
    result.y = first.y * second.y;
    result.z = first.z * second.z;
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
    srVector4T<T>();

    srVector3T<T> xyz() const
    {
        srVector3T<T> result;
        result.Set(x, y, z);
        return result;
    }

    void* operator new[](unsigned int size)
    {
        return srHeap.allocate(size);
    }

    void operator delete[](void* allocation)
    {
        srHeap.free(allocation);
    }

    template <class U> srVector4T<T>& operator=(const srVector4T<U>& source)
    {
        x = static_cast<T>(source.x);
        y = static_cast<T>(source.y);
        z = static_cast<T>(source.z);
        w = static_cast<T>(source.w);
        return *this;
    }

    srVector4T<T>* Set(T source_0, T source_1, T source_2, T source_3);
    T Length() const;

    int isValid() const
    {
        return _finite(static_cast<double>(x)) && _finite(static_cast<double>(y)) &&
               _finite(static_cast<double>(z)) && _finite(static_cast<double>(w));
    }

    /* Four-channel saturation, expanded in setClearColor (0x1001CA30),
       setFogColor (0x1001C6B0) and finishDiffuseAlpha (0x1002B200).
       Keep the same <=/>= comparisons as the three-channel SetSaturated
       operation, including the writes at the two endpoints. */
    void SetSaturated(const srVector4T<T>& source)
    {
        *this = source;
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
        if (w <= 0.0f)
            w = 0;
        else if (w >= 1.0f)
            w = 1.0f;
    }

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

template <class T> srVector4T<T>::srVector4T() {}

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

template <class T> srVector4T<T> operator-(const srVector4T<T>& vector)
{
    srVector4T<T> result;
    result.Set(-vector.x, -vector.y, -vector.z, -vector.w);
    return result;
}

template <class T> srVector4T<T> operator+(const srVector4T<T>& first, const srVector4T<T>& second)
{
    srVector4T<T> result;
    result.x = first.x + second.x;
    result.y = first.y + second.y;
    result.z = first.z + second.z;
    result.w = first.w + second.w;
    return result;
}

template <class T> srVector4T<T> operator-(const srVector4T<T>& first, const srVector4T<T>& second)
{
    srVector4T<T> result;
    result.x = first.x - second.x;
    result.y = first.y - second.y;
    result.z = first.z - second.z;
    result.w = first.w - second.w;
    return result;
}

template <class T> srVector4T<T> operator*(const srVector4T<T>& first, const srVector4T<T>& second)
{
    srVector4T<T> result;
    result.x = first.x * second.x;
    result.y = first.y * second.y;
    result.z = first.z * second.z;
    result.w = first.w * second.w;
    return result;
}

template <class T> srVector4T<T> operator*(const srVector4T<T>& vector, double scalar)
{
    srVector4T<T> result;
    result.x = (T)(vector.x * scalar);
    result.y = (T)(vector.y * scalar);
    result.z = (T)(vector.z * scalar);
    result.w = (T)(vector.w * scalar);
    return result;
}

/* 0x10067720 / 0x10067770 / 0x100677D0: the four-dimensional dot includes
   w*w. Plane evaluation with a three-dimensional point is a distinct
   operation and keeps its addition of the plane constant. */
template <class T> T DotProduct(const srVector4T<T>& first, const srVector4T<T>& second)
{
    return first.x * second.x + first.y * second.y + first.z * second.z + first.w * second.w;
}

template <class T> class srMatrix2T {
public:
    srMatrix2T<T>* MultiplyBy(const srMatrix2T<T>& other);

    srVector2T<T> vectors[2];
};

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
    void OrthonormalizeRows();
    void SetIdentity();
    srMatrix3T<T>* RotateAboutY(double sine, double cosine);
    srMatrix3T<T>* RotateAboutX(double sine, double cosine);
    srMatrix3T<T>* RotateAboutZ(double sine, double cosine);
    srMatrix3T<T>* RotateAroundAxis(double sine, double cosine, const srVector3T<T>& axis);

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

template <class T>
srMatrix3T<T>* srMatrix3T<T>::SetRows(const srVector3T<T>& first, const srVector3T<T>& second,
                                      const srVector3T<T>& third)
{
    vectors[0] = first;
    vectors[1] = second;
    vectors[2] = third;
    return this;
}

template <class T> srMatrix3T<T>* srMatrix3T<T>::MultiplyBy(const srMatrix3T<T>& other)
{
    srMatrix3T<T> result;
    T* output = &result.vectors[0].x;
    const T* right = &other.vectors[0].x;
    const T* left = &vectors[0].x;

    for (int index = 0; index < 3; ++index) {
        T x = right[index];
        T y = right[index + 3];
        T z = right[index + 6];

        output[index] = x * left[0] + y * left[1] + z * left[2];
        output[index + 3] = x * left[3] + y * left[4] + z * left[5];
        output[index + 6] = x * left[6] + y * left[7] + z * left[8];
    }
    *this = result;
    return this;
}

/* Row-wise Gram-Schmidt, expanded by srNode::setParent, pitchAt, yawAt,
   rollUp and rollAt (0x10050F00, 0x10052D80, 0x10052FC0, 0x10053210,
   0x10053470). Original method spelling is unknown. Each projection and
   reciprocal is rounded through the vector operators' double arguments;
   zero-length rows remain unguarded, unlike srVector3T::Normalize. */
template <class T> void srMatrix3T<T>::OrthonormalizeRows()
{
    for (int row = 0; row < 3; ++row) {
        for (int earlier = 0; earlier < row; ++earlier) {
            vectors[row] -= vectors[earlier] * DotProduct(vectors[row], vectors[earlier]);
        }
        vectors[row] *= 1.0 / vectors[row].Length();
    }
}

template <class T> void srMatrix3T<T>::SetIdentity()
{
    vectors[0] = srVector3T<T>((T)1, (T)0, (T)0);
    vectors[1] = srVector3T<T>((T)0, (T)1, (T)0);
    vectors[2] = srVector3T<T>((T)0, (T)0, (T)1);
}

template <class T> bool srMatrix3T<T>::operator==(const srMatrix3T<T>& other) const
{
    return vectors[0] == other.vectors[0] && vectors[1] == other.vectors[1] &&
           vectors[2] == other.vectors[2];
}

template <class T> srMatrix3T<T>* srMatrix3T<T>::RotateAboutY(double sine, double cosine)
{
    srMatrix3T<T> rotation;

    rotation.SetRows(srVector3T<T>((T)cosine, (T)0, (T)sine), srVector3T<T>((T)0, (T)1, (T)0),
                     srVector3T<T>((T)-sine, (T)0, (T)cosine));
    MultiplyBy(rotation);
    return this;
}

template <class T> srMatrix3T<T>* srMatrix3T<T>::RotateAboutX(double sine, double cosine)
{
    srMatrix3T<T> rotation;

    rotation.SetRows(srVector3T<T>((T)1, (T)0, (T)0), srVector3T<T>((T)0, (T)cosine, (T)-sine),
                     srVector3T<T>((T)0, (T)sine, (T)cosine));
    MultiplyBy(rotation);
    return this;
}

template <class T> srMatrix3T<T>* srMatrix3T<T>::RotateAboutZ(double sine, double cosine)
{
    srMatrix3T<T> rotation;

    rotation.SetRows(srVector3T<T>((T)cosine, (T)-sine, (T)0),
                     srVector3T<T>((T)sine, (T)cosine, (T)0), srVector3T<T>((T)0, (T)0, (T)1));
    MultiplyBy(rotation);
    return this;
}

// srMatrix3T<float>::RotateAboutY
template <class T> srMatrix3T<T>* srMatrix3T<T>::RotateAboutY(double angle)
{
    srMatrix3T<T> rotation;
    T cosine;
    T sine;

    if (angle != 0.0) {
        cosine = (T)cos(angle);
        sine = (T)sin(angle);
        rotation.SetRows(srVector3T<T>(cosine, (T)0, sine), srVector3T<T>((T)0, (T)1, (T)0),
                         srVector3T<T>(-sine, (T)0, cosine));
        MultiplyBy(rotation);
    }
    return this;
}

// srMatrix3T<float>::RotateAboutX
template <class T> srMatrix3T<T>* srMatrix3T<T>::RotateAboutX(double angle)
{
    srMatrix3T<T> rotation;
    T cosine;
    T sine;

    if (angle != 0.0) {
        cosine = (T)cos(angle);
        sine = (T)sin(angle);
        rotation.SetRows(srVector3T<T>((T)1, (T)0, (T)0), srVector3T<T>((T)0, cosine, -sine),
                         srVector3T<T>((T)0, sine, cosine));
        MultiplyBy(rotation);
    }
    return this;
}

template <class T> srMatrix3T<T>* srMatrix3T<T>::RotateAboutZ(double angle)
{
    srMatrix3T<T> rotation;
    T cosine;
    T sine;

    if (angle != 0.0) {
        cosine = (T)cos(angle);
        sine = (T)sin(angle);
        rotation.SetRows(srVector3T<T>(cosine, -sine, (T)0), srVector3T<T>(sine, cosine, (T)0),
                         srVector3T<T>((T)0, (T)0, (T)1));
        MultiplyBy(rotation);
    }
    return this;
}

/* Single-angle overload of the Rodrigues rotation above, keeping the
   trigonometry and the basis products in double precision until the float
   stores. The MartensBluff2 arrow trap emits it at 0x004DE940. */
template <class T>
srMatrix3T<T>* srMatrix3T<T>::RotateAroundAxis(double angle, const srVector3T<T>& axis)
{
    srMatrix3T<T> rotation;
    double sine;
    double cosine;
    double one_minus_cosine;

    if (angle != 0.0) {
        cosine = cos(angle);
        sine = sin(angle);
        one_minus_cosine = 1.0 - cosine;
        rotation.vectors[0].x = (T)(axis.x * axis.x + ((T)1 - axis.x * axis.x) * cosine);
        rotation.vectors[0].y = (T)(axis.x * axis.y * one_minus_cosine - axis.z * sine);
        rotation.vectors[0].z = (T)(axis.x * axis.z * one_minus_cosine + axis.y * sine);
        rotation.vectors[1].x = (T)(axis.y * axis.x * one_minus_cosine + axis.z * sine);
        rotation.vectors[1].y = (T)(axis.y * axis.y + ((T)1 - axis.y * axis.y) * cosine);
        rotation.vectors[1].z = (T)(axis.y * axis.z * one_minus_cosine - axis.x * sine);
        rotation.vectors[2].x = (T)(axis.z * axis.x * one_minus_cosine - axis.y * sine);
        rotation.vectors[2].y = (T)(axis.z * axis.y * one_minus_cosine + axis.x * sine);
        rotation.vectors[2].z = (T)(axis.z * axis.z + ((T)1 - axis.z * axis.z) * cosine);
        MultiplyBy(rotation);
    }
    return this;
}

template <class T>
srMatrix3T<T>* srMatrix3T<T>::RotateAroundAxis(double sine, double cosine,
                                               const srVector3T<T>& axis)
{
    srMatrix3T<T> rotation;
    T one_minus_cosine = (T)1 - (T)cosine;

    rotation.vectors[0].x = axis.x * axis.x + ((T)1 - axis.x * axis.x) * (T)cosine;
    rotation.vectors[0].y = axis.x * axis.y * one_minus_cosine - axis.z * (T)sine;
    rotation.vectors[0].z = axis.x * axis.z * one_minus_cosine + axis.y * (T)sine;
    rotation.vectors[1].x = axis.y * axis.x * one_minus_cosine + axis.z * (T)sine;
    rotation.vectors[1].y = axis.y * axis.y + ((T)1 - axis.y * axis.y) * (T)cosine;
    rotation.vectors[1].z = axis.y * axis.z * one_minus_cosine - axis.x * (T)sine;
    rotation.vectors[2].x = axis.z * axis.x * one_minus_cosine - axis.y * (T)sine;
    rotation.vectors[2].y = axis.z * axis.y * one_minus_cosine + axis.x * (T)sine;
    rotation.vectors[2].z = axis.z * axis.z + ((T)1 - axis.z * axis.z) * (T)cosine;
    MultiplyBy(rotation);
    return this;
}

template <class T> srVector3T<T> srMatrix3T<T>::Transform(const srVector3T<T>& value) const
{
    srVector3T<T> result;
    result.x = DotProduct(vectors[0], value);
    result.y = DotProduct(vectors[1], value);
    result.z = DotProduct(vectors[2], value);
    return result;
}

template <class T>
srVector3T<T> srMatrix3T<T>::TransformTransposed(const srVector3T<T>& value) const
{
    srVector3T<T> result;
    result.x = vectors[0].x * value.x + vectors[1].x * value.y + vectors[2].x * value.z;
    result.y = vectors[0].y * value.x + vectors[1].y * value.y + vectors[2].y * value.z;
    result.z = vectors[0].z * value.x + vectors[1].z * value.y + vectors[2].z * value.z;
    return result;
}

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
        SCALE_TYPE_UNIT = 0,       /* equal column lengths, all ~1 */
        SCALE_TYPE_UNIFORM = 1,    /* equal column lengths, not 1 */
        SCALE_TYPE_NON_UNIFORM = 2 /* unequal column lengths */
    };
    /* classifyMatrix (0x100215A0) distinguishes these zero patterns.
       1 and 2 are not produced by the recovered classifier. */
    enum e_type {
        TYPE_GENERAL = 0,
        TYPE_AFFINE = 3,
        TYPE_IDENTITY = 4,
        TYPE_ORTHOGRAPHIC = 5,
        TYPE_PERSPECTIVE = 6
    };

    /* The renderer maintains float matrices and exposes double overloads.
       A member template leaves the same-precision implicit copy intact. */
    template <class U> srMatrix4T<T>& operator=(const srMatrix4T<U>& source)
    {
        for (int row = 0; row != 4; ++row) {
            vectors[row] = source.vectors[row];
        }
        return *this;
    }
    void SetIdentity()
    {
        vectors[0].Set(1, 0, 0, 0);
        vectors[1].Set(0, 1, 0, 0);
        vectors[2].Set(0, 0, 1, 0);
        vectors[3].Set(0, 0, 0, 1);
    }

    srMatrix4T<T>* Invert();
    srMatrix4T<T>* Inverse(srMatrix4T<T>& source);
    /* Each completed row is stored before reading the source for the next
       row; the matrices must not alias. Mixed-precision multiplication
       retains the source coefficient width.
       srGERD's double overload uses FMUL double and rounds only each result
       into the float matrix; converting the input first loses precision. */
    template <class U> srMatrix4T<T>* MultiplyBy(const srMatrix4T<U>& other)
    {
        for (int row = 0; row != 4; ++row) {
            T x = vectors[row].x;
            T y = vectors[row].y;
            T z = vectors[row].z;
            T w = vectors[row].w;
            vectors[row].x = static_cast<T>(x * other.vectors[0].x + y * other.vectors[1].x +
                                            z * other.vectors[2].x + w * other.vectors[3].x);
            vectors[row].y = static_cast<T>(x * other.vectors[0].y + y * other.vectors[1].y +
                                            z * other.vectors[2].y + w * other.vectors[3].y);
            vectors[row].z = static_cast<T>(x * other.vectors[0].z + y * other.vectors[1].z +
                                            z * other.vectors[2].z + w * other.vectors[3].z);
            vectors[row].w = static_cast<T>(x * other.vectors[0].w + y * other.vectors[1].w +
                                            z * other.vectors[2].w + w * other.vectors[3].w);
        }
        return this;
    }
    srMatrix4T<T>* Multiply(const srMatrix4T<T>& other, srMatrix4T<T>& result);
    T* Scale(double scale);
    void AdjugateFrom(T* source);
    srMatrix4T<T>* Set(const srMatrix3T<T>& rotation, const srVector3T<T>& translation);
    T Det() const;
    srVector3T<T> TransformPoint(const srVector3T<T>& point) const;
    srVector3T<T> TransformDirection(const srVector3T<T>& direction) const;
    srVector4T<T> Transform(const srVector3T<T>& point) const;

    srVector4T<T> vectors[4];
};

template <class T>
srMatrix4T<T>* srMatrix4T<T>::Multiply(const srMatrix4T<T>& other, srMatrix4T<T>& result)
{
    for (int index = 0; index != 4; ++index) {
        const srVector4T<T>& row = vectors[index];
        result.vectors[index].Set(row.x * other.vectors[0].x + row.y * other.vectors[1].x +
                                      row.z * other.vectors[2].x + row.w * other.vectors[3].x,
                                  row.x * other.vectors[0].y + row.y * other.vectors[1].y +
                                      row.z * other.vectors[2].y + row.w * other.vectors[3].y,
                                  row.x * other.vectors[0].z + row.y * other.vectors[1].z +
                                      row.z * other.vectors[2].z + row.w * other.vectors[3].z,
                                  row.x * other.vectors[0].w + row.y * other.vectors[1].w +
                                      row.z * other.vectors[2].w + row.w * other.vectors[3].w);
    }
    return this;
}

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

/* Assign *this = inverse(source). The float instantiation inside
   srBounder::updateBounds (0x1004A800) takes a self-source branch that stages
   the adjugate through a temporary before copying back. */
template <class T> srMatrix4T<T>* srMatrix4T<T>::Inverse(srMatrix4T<T>& source)
{
    if (&source == this) {
        return Invert();
    }
    AdjugateFrom(&source.vectors[0].x);
    T determinant = source.Det();
    if (determinant != 1.0) {
        Scale(1.0 / determinant);
    }
    return this;
}

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
    T* components = &vectors[0].x;
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

    components[0] = fVar19 * fVar8 + (fVar17 * fVar6 - fVar18 * fVar7);
    T fVar20 = fVar16 * fVar9 - fVar13 * fVar12;
    T fVar21 = fVar15 * fVar9 - fVar13 * fVar11;
    components[4] = -(fVar21 * fVar8 + (fVar17 * fVar5 - fVar20 * fVar7));
    T fVar22 = fVar14 * fVar9 - fVar13 * fVar10;
    components[8] = fVar22 * fVar8 + (fVar18 * fVar5 - fVar20 * fVar6);
    components[12] = -(fVar22 * fVar7 + (fVar19 * fVar5 - fVar21 * fVar6));
    components[1] = -(fVar19 * fVar4 + (fVar17 * fVar2 - fVar18 * fVar3));
    components[5] = fVar21 * fVar4 + (fVar17 * fVar1 - fVar20 * fVar3);
    components[9] = -(fVar22 * fVar4 + (fVar18 * fVar1 - fVar20 * fVar2));
    components[13] = fVar22 * fVar3 + (fVar19 * fVar1 - fVar21 * fVar2);
    fVar19 = fVar16 * fVar7 - fVar15 * fVar8;
    fVar18 = fVar16 * fVar6 - fVar14 * fVar8;
    fVar17 = fVar15 * fVar6 - fVar14 * fVar7;
    components[2] = fVar17 * fVar4 + (fVar19 * fVar2 - fVar18 * fVar3);
    fVar16 = fVar16 * fVar5 - fVar13 * fVar8;
    fVar15 = fVar15 * fVar5 - fVar13 * fVar7;
    components[6] = -(fVar15 * fVar4 + (fVar19 * fVar1 - fVar16 * fVar3));
    fVar13 = fVar14 * fVar5 - fVar13 * fVar6;
    components[10] = fVar13 * fVar4 + (fVar18 * fVar1 - fVar16 * fVar2);
    components[14] = -(fVar13 * fVar3 + (fVar17 * fVar1 - fVar15 * fVar2));
    fVar15 = fVar12 * fVar7 - fVar11 * fVar8;
    fVar14 = fVar12 * fVar6 - fVar10 * fVar8;
    fVar13 = fVar11 * fVar6 - fVar10 * fVar7;
    components[3] = -(fVar13 * fVar4 + (fVar15 * fVar2 - fVar14 * fVar3));
    fVar8 = fVar12 * fVar5 - fVar9 * fVar8;
    fVar7 = fVar11 * fVar5 - fVar9 * fVar7;
    components[7] = fVar7 * fVar4 + (fVar15 * fVar1 - fVar8 * fVar3);
    fVar5 = fVar10 * fVar5 - fVar9 * fVar6;
    components[11] = -(fVar5 * fVar4 + (fVar14 * fVar1 - fVar8 * fVar2));
    components[15] = fVar5 * fVar3 + (fVar13 * fVar1 - fVar7 * fVar2);
}

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

// FUNCTION: WIZ8 0x0049BD00
inline float Det3(float m00, float m01, float m02, float m10, float m11, float m12, float m20,
                  float m21, float m22)
{
    return (m01 * m12 - m02 * m11) * m20 +
           ((m11 * m22 - m12 * m21) * m00 - (m01 * m22 - m02 * m21) * m10);
}

template <class T> class srMatrix4x3T {
public:
    void SetIdentity();
    void SetRotation(const srMatrix3T<T>& rotation);
    srMatrix4x3T<T>* SetTranslation(const srVector3T<T>& translation);
    srMatrix4x3T<T>* Scale(const srVector3T<T>& scale);
    srVector3T<T> TransformPoint(const srVector3T<T>& point) const;

    srVector4T<T> rows[3];
};

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
    srVector3i() {}

    void* operator new[](unsigned int size)
    {
        return srHeap.allocate(size);
    }

    void operator delete[](void* allocation)
    {
        srHeap.free(allocation);
    }

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

class srQuaternion {
public:
    float w;
    srVector3T<float> v;
};

static_assert(sizeof(srQuaternion) == 0x10, "srQuaternion_must_be_0x10");
