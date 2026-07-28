#ifndef WIZ8_ENGINE_CODE_BITARRAY_H
#define WIZ8_ENGINE_CODE_BITARRAY_H

/*
 * Engine Code\BitArray.cpp.
 *
 * A flat array of bits over a malloc'd run of 32-bit words, with a running
 * count of how many are set and a resumable cursor for walking them. The class
 * name and the index member's spelling are the assertions' own: "BitArray:
 * Couldn't allocate bit index." over assert(puiIndex).
 *
 * Every method is __thiscall, and the layout below is what they agree on.
 * The cursor is three separate members rather than one bit number because the
 * walk keeps the word index, the bit within the word, and the bit number of
 * the word's first bit, and advances them independently.
 */

#pragma pack(push, 1)

class BitArray {
public:
    /* Reallocate to hold this many bits and clear every one of them. Asking
       for the size it already has only clears. */
    void SetSize(unsigned int bit_count);       /* 0x0043ADA0 */
    /* Release the index. Not a destructor: nothing restores a vtable and the
       object is left holding a dangling pointer. */
    void FreeIndex();                            /* 0x0043AD90 */
    /* Copy another array's bits and count into this one, sizing the source to
       this one's bit count first. */
    void CopyFrom(BitArray& other);              /* 0x0043AE80 */

    /* Raise one bit. Answers whether it was already up; a bit past the end is
       refused rather than grown into. */
    bool Set(unsigned int bit);                  /* 0x0043B390 */
    /* The same, except that a bit past the end grows the array to reach it,
       with a hundred bits of slack. */
    bool SetAndGrow(unsigned int bit);           /* 0x0043B3D0 */
    bool SetAll();                               /* 0x0043B420 */
    /* Lower one bit, answering whether it had been up. */
    bool Clear(unsigned int bit);                /* 0x0043B450 */
    /* Lower every bit and rewind the cursor. */
    void ClearAll();                             /* 0x0043B490 */
    bool Test(unsigned int bit);                 /* 0x0043B620 */

    /* Keep only the bits both arrays have; the shorter one bounds the walk. */
    bool IntersectWith(BitArray& other);         /* 0x0043B4C0 */
    /* Take on every bit either array has. */
    bool UnionWith(BitArray& other);             /* 0x0043B510 */
    /* Become the complement of another array, masked back to whichever of the
       two ends sooner so the bits past the end stay down. */
    void SetToComplementOf(BitArray& other);     /* 0x0043B560 */

    /* Count the bits that are up by walking them, rather than reading the
       running count. */
    int CountSetBits();                          /* 0x0043B5F0 */
    /* The next bit that is up, one-based, or zero at the end. Restarting
       rewinds first and answers -1 when there is no index at all. */
    int NextSetBit(char restart);                /* 0x0043B660 */
    /* Grow to hold this many bits, keeping what is already set. */
    void Grow(unsigned int bit_count, unsigned int new_bit_count);  /* 0x0043B700 */

    int set_count;                    /* 0x00: how many bits are up */
    unsigned int word_count;          /* 0x04 */
    int cursor_word;                  /* 0x08 */
    int cursor_bit;                   /* 0x0c: within cursor_word */
    unsigned int cursor_base;         /* 0x10: bit number of cursor_word's bit zero */
    /* 0x14: the bits of the last word that are inside the array. Only the
       complement uses it, which is where the bits past the end would
       otherwise come up. */
    unsigned int tail_mask;
    unsigned int* puiIndex;           /* 0x18 */
    unsigned int bit_count;           /* 0x1c */
};                                    /* 0x20 */

#pragma pack(pop)

enum { W8_BITS_PER_WORD = 32 };

#endif
