#pragma once

/* SurRender's 64-bit value type, passed by value and by reference across the
   exported timer interface. The layout is two 32-bit words; nothing exported
   states more, and the conversion below is the shape the game-timer unit's
   constructor at 0x00439550 proves: the high word reached through a 64-bit
   shift that VC6 lowers to __aullshr, each half converted through a signed
   64-bit temporary, and the halves rejoined at 2^32. */
class srQuadWord {
public:
    unsigned int lo;                     /* 0x00 */
    unsigned int hi;                     /* 0x04 */

    operator double() const
    {
        return (double)(unsigned int)(*(const unsigned __int64*)this >> 32) *
                   4294967296.0 +
               (double)(unsigned int)*(const unsigned __int64*)this;
    }
};

typedef char srQuadWord_must_be_8[(sizeof(srQuadWord) == 8) ? 1 : -1];
