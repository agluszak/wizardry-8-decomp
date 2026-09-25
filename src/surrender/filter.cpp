#include "surrender/srFilter.h"

// FUNCTION: SURRENDER 0x100032C0
double srBoxFilter::getWeight(double value) const
{
    if (value > -0.5 && value <= 0.5) {
        return 1.0;
    }
    return 0.0;
}

// FUNCTION: SURRENDER 0x10003320
double srBoxFilter::getSupport() const
{
    return 0.5;
}

// FUNCTION: SURRENDER 0x10003470
const char* srBoxFilter::getName() const
{
    return "box filter";
}

// FUNCTION: SURRENDER 0x10003480
srBoxFilter::srBoxFilter() {}

// FUNCTION: SURRENDER 0x100034A0
srBoxFilter::srBoxFilter(const class srBoxFilter& other) {}

// FUNCTION: SURRENDER 0x10003520
class srBoxFilter& srBoxFilter::operator=(const class srBoxFilter& other)
{
    return *this;
}

// FUNCTION: SURRENDER 0x10003530
srBoxFilter::~srBoxFilter() {}

// FUNCTION: SURRENDER 0x10003340
double srTriangleFilter::getWeight(double value) const
{
    if (value < 0.0) {
        value = -value;
    }
    if (value < 1.0) {
        return 1.0 - value;
    }
    return 0.0;
}

// FUNCTION: SURRENDER 0x10003400
double srTriangleFilter::getSupport() const
{
    return 1.0;
}

// FUNCTION: SURRENDER 0x100036A0
const char* srTriangleFilter::getName() const
{
    return "triangle filter";
}

// FUNCTION: SURRENDER 0x100036B0
srTriangleFilter::srTriangleFilter() {}

// FUNCTION: SURRENDER 0x100036C0
srTriangleFilter::srTriangleFilter(const class srTriangleFilter& other) {}

// FUNCTION: SURRENDER 0x100036D0
class srTriangleFilter& srTriangleFilter::operator=(const class srTriangleFilter& other)
{
    return *this;
}

// FUNCTION: SURRENDER 0x100036E0
srTriangleFilter::~srTriangleFilter() {}

// FUNCTION: SURRENDER 0x10003410
double srBellFilter::getWeight(double value) const
{
    if (value < 0.0) {
        value = -value;
    }
    if (value < 0.5) {
        return 0.75 - value * value;
    }
    if (value < 1.5) {
        return (value - 1.5) * (value - 1.5) * 0.5;
    }
    return 0.0;
}

// FUNCTION: SURRENDER 0x10003490
double srBellFilter::getSupport() const
{
    return 1.5;
}

// FUNCTION: SURRENDER 0x100035D0
const char* srBellFilter::getName() const
{
    return "bell-curve filter";
}

// FUNCTION: SURRENDER 0x100035E0
srBellFilter::srBellFilter() {}

// FUNCTION: SURRENDER 0x100035F0
srBellFilter::srBellFilter(const class srBellFilter& other) {}

// FUNCTION: SURRENDER 0x10003600
class srBellFilter& srBellFilter::operator=(const class srBellFilter& other)
{
    return *this;
}

// FUNCTION: SURRENDER 0x10003610
srBellFilter::~srBellFilter() {}

// FUNCTION: SURRENDER 0x100034B0
double srBSplineFilter::getWeight(double value) const
{
    if (value < 0.0) {
        value = -value;
    }
    if (value < 1.0) {
        return value * value * value * 0.5 - value * value + 0.6666666666666666;
    }
    if (value < 2.0) {
        double t = 2.0 - value;
        return t * t * t * 0.16666666666666666;
    }
    return 0.0;
}

// FUNCTION: SURRENDER 0x10003560
double srBSplineFilter::getSupport() const
{
    return 2.0;
}

// FUNCTION: SURRENDER 0x10003770
const char* srBSplineFilter::getName() const
{
    return "B-Spline filter";
}

// FUNCTION: SURRENDER 0x10003780
srBSplineFilter::srBSplineFilter() {}

// FUNCTION: SURRENDER 0x10003790
srBSplineFilter::srBSplineFilter(const class srBSplineFilter& other) {}

// FUNCTION: SURRENDER 0x100037A0
class srBSplineFilter& srBSplineFilter::operator=(const class srBSplineFilter& other)
{
    return *this;
}

// FUNCTION: SURRENDER 0x100037B0
srBSplineFilter::~srBSplineFilter() {}

// GLOBAL: SURRENDER 0x100A0290
class srTriangleFilter srTriangleFilter;

// GLOBAL: SURRENDER 0x100A0294
class srBellFilter srBellFilter;

// GLOBAL: SURRENDER 0x100A0298
class srBSplineFilter srBSplineFilter;

// GLOBAL: SURRENDER 0x100A029C
class srBoxFilter srBoxFilter;

// SYNTHETIC: SURRENDER 0X10003540
// srBoxFilter scalar deleting destructor

// SYNTHETIC: SURRENDER 0X10003620
// srBellFilter scalar deleting destructor

// SYNTHETIC: SURRENDER 0X100036F0
// srTriangleFilter scalar deleting destructor

// SYNTHETIC: SURRENDER 0X100037C0
// srBSplineFilter scalar deleting destructor
