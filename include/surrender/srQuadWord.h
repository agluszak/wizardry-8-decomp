#pragma once

/* SurRender's 64-bit value type: two 32-bit words. */
class srQuadWord {
public:
    unsigned int lo; /* 0x00 */
    unsigned int hi; /* 0x04 */

    /* The timer bodies store ftol results straight into a quad word; this is
       the low/high word store that lowers to. */
    srQuadWord& operator=(unsigned __int64 value)
    {
        lo = static_cast<unsigned int>(value);
        hi = static_cast<unsigned int>(value >> 0x20);
        return *this;
    }

    operator double() const
    {
        // reinterpret-ok: deliberate bit reinterpretation of the pair as one qword
        const unsigned __int64 bits = *reinterpret_cast<const unsigned __int64*>(this);
        return static_cast<unsigned int>(bits >> 32) * 4294967296.0 +
               static_cast<unsigned int>(bits);
    }

    /* The srTimer bodies subtract quad words with an explicit borrow out of
       the low half; that is what this lowers to. */
    srQuadWord operator-(const srQuadWord& other) const
    {
        srQuadWord result;
        result.lo = lo - other.lo;
        result.hi = hi - other.hi - (lo < other.lo);
        return result;
    }

    srQuadWord& operator+=(const srQuadWord& other)
    {
        unsigned int carry = lo;
        lo += other.lo;
        hi += other.hi + (lo < carry);
        return *this;
    }

    double operator*(double factor) const
    {
        return operator double() * factor;
    }

    srQuadWord operator*(unsigned int factor) const
    {
        unsigned __int64 product = (((unsigned __int64)hi << 32) | lo) * factor;
        srQuadWord result;
        result.lo = static_cast<unsigned int>(product);
        result.hi = static_cast<unsigned int>(product >> 32);
        return result;
    }

    srQuadWord operator/(const srQuadWord& divisor) const
    {
        unsigned __int64 dividend = ((unsigned __int64)hi << 32) | lo;
        unsigned __int64 divisor64 = ((unsigned __int64)divisor.hi << 32) | divisor.lo;
        unsigned __int64 quotient = dividend / divisor64;
        srQuadWord result;
        result.lo = static_cast<unsigned int>(quotient);
        result.hi = static_cast<unsigned int>(quotient >> 32);
        return result;
    }
};

static_assert((sizeof(srQuadWord) == 8), "srQuadWord_must_be_8");
