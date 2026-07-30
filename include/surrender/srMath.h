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

float Function4218E0(
    const srVector3T<float>& first,
    const srVector3T<float>& second);

template <class T>
class srVector4T {
public:
    T x;
    T y;
    T z;
    T w;
};

/* Specialised for the same reason srVector3T<float> is: the image carries an
   inline body that converts scalar doubles into this type's floats, and an
   inline is emitted into whatever calls it rather than exported from SR.DLL,
   so it has to exist here for the game's own units to link and match. The
   layout is unchanged - four adjacent floats - and only the body is added. */
template <>
class srVector4T<float> {
public:
    float x;
    float y;
    float z;
    float w;

    /* Exact inline body; the original member name is not exported. */
    srVector4T<float>* method_004817E0(double source_0, double source_1,
                                       double source_2, double source_3);
    srVector4T<float>* method_004D6B30(float source_0, float source_1,
                                       float source_2, float source_3);
};

template <class T>
class srMatrix3T {
public:
    srVector3T<T> vectors[3];
};

template <>
class srMatrix3T<float> {
public:
    srMatrix3T<float>* method_004219F0(
        const srVector3T<float>& first,
        const srVector3T<float>& second,
        const srVector3T<float>& third);         /* 0x004219F0 */

    srMatrix3T<float>* method_00421A40(
        const srMatrix3T<float>& other);         /* 0x00421A40 */
    void SetIdentity00467310();                  /* 0x00467310 */
    srMatrix3T<float>* method_00438F90(
        double sine, double cosine);             /* 0x00438F90 */
    srMatrix3T<float>* method_004A5AB0(double angle); /* 0x004A5AB0 */
    srMatrix3T<float>* method_004CAB60(double angle); /* 0x004CAB60 */
    srMatrix3T<float>* method_00478EB0(
        double sine, double cosine);             /* 0x00478EB0 */

    srVector3T<float> vectors[3];
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
