#ifndef WIZ8_GEOMETRY_H
#define WIZ8_GEOMETRY_H

#include "surrender/srMath.h"

unsigned char PointInsideBounds004BE870(
    const srVector3T<float>* point,
    const srVector3T<float>* minimum,
    const srVector3T<float>* maximum);

#endif
