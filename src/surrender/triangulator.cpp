#include "surrender/srTriangulator.h"

// FUNCTION: SURRENDER 0x1003bee0
int srTriangulator::sameSide(const srVector2T<float>& p1, const srVector2T<float>& p2,
                             const srVector2T<float>& a, const srVector2T<float>& b)
{
    float first = (p1.x - a.x) * (b.y - a.y) - (p1.y - a.y) * (b.x - a.x);
    float second = (p2.x - a.x) * (b.y - a.y) - (p2.y - a.y) * (b.x - a.x);
    return first * second > 0.0f;
}

// FUNCTION: SURRENDER 0x1003bf40
int srTriangulator::isInsideTriangle(const srVector2T<float>& p, const srVector2T<float>& a,
                                     const srVector2T<float>& b, const srVector2T<float>& c)
{
    return sameSide(p, a, b, c) && sameSide(p, b, a, c) && sameSide(p, c, a, b);
}
