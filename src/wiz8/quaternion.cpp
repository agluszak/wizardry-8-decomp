#include "wiz8/geometry.h"

/* Unresolved fragment: 0x0044ECA0 lies between the Prop.cpp 0x0044E1F0
   and 3dapi.cpp 0x0044F1C0 anchors. Neither proves this helper's TU. */
// FUNCTION: WIZ8 0x0044eca0
W8Quaternion* W8Quaternion::SetFromMatrix(const srMatrix3T<float>& matrix)
{
    const float* m = &matrix.vectors[0].x;
    float temp[4];
    double root;
    double scale;
    float trace;
    int i;
    int j;
    int k;

    trace = m[4] + m[0] + m[8];
    if (0.0 < trace) {
        root = sqrt(static_cast<double>(trace) + 1.0);
        w = static_cast<float>(root * 0.5);
        scale = 0.5 / root;
        v.x = static_cast<float>((m[7] - m[5]) * scale);
        v.y = static_cast<float>((m[2] - m[6]) * scale);
        v.z = static_cast<float>((m[3] - m[1]) * scale);
        return this;
    }

    i = m[0] < m[4];
    if (m[i * 4] < m[8]) {
        i = 2;
    }
    j = (i + 1) % 3;
    k = (j + 1) % 3;
    root = sqrt(static_cast<double>(m[i * 4] - (m[k * 4] + m[j * 4])) + 1.0);
    temp[i] = static_cast<float>(root * 0.5);
    scale = 0.5 / root;
    temp[3] = static_cast<float>((m[k * 3 + j] - m[j * 3 + k]) * scale);
    temp[j] = static_cast<float>((m[i * 3 + j] + m[j * 3 + i]) * scale);
    temp[k] = static_cast<float>((m[i * 3 + k] + m[k * 3 + i]) * scale);
    w = temp[3];
    v.x = temp[0];
    v.y = temp[1];
    v.z = temp[2];
    return this;
}
