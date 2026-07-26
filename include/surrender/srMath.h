#pragma once

/*
 * SurRender math types named by the original SR.DLL export table. The layouts
 * are fixed by the exported srBinIStream operators: vectors store adjacent
 * scalars, and matrices store three or four adjacent vector elements.
 */
template <class T>
class srVector2T {
public:
    T x;
    T y;
};

template <class T>
class srVector3T {
public:
    T x;
    T y;
    T z;
};

template <>
class srVector3T<float> {
public:
    float x;
    float y;
    float z;

    /* Exact inline bodies; the original member names are not exported. */
    srVector3T<float>* method_00421680(double source_0, double source_1, double source_2);
    srVector3T<float>* method_00446110(const srVector3T<double>* source);
};

template <class T>
class srVector4T {
public:
    T x;
    T y;
    T z;
    T w;
};

template <class T>
class srMatrix3T {
public:
    srVector3T<T> vectors[3];
};

template <class T>
class srMatrix4T {
public:
    srVector4T<T> vectors[4];
};

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
