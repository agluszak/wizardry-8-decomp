#include "wiz8/engine_code/BitArray.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

#define BITARRAY_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\BitArray.cpp"

/* The word count is computed through a floating divide rather than a shift,
   which is why every sizing path in here goes through ftol. */
#define W8_WHOLE_WORDS(bits) ((int)((bits) / 32.0))

/* How much slack SetAndGrow leaves beyond the bit that overflowed. */
enum { W8_BITARRAY_GROWTH_SLACK = 100 };

/* Reallocate to hold this many bits and clear every one of them. The tail mask
   is rebuilt from the bits that spill past the last whole word; asking for the
   size it already has skips straight to the clear. */
// FUNCTION: WIZ8 0x0043ada0
void BitArray::SetSize(unsigned int new_bit_count)
{
    unsigned int spill;
    unsigned int index;

    cursor_base = 0;
    cursor_bit = 0;
    cursor_word = 0;

    if (bit_count != new_bit_count) {
        spill = new_bit_count - W8_WHOLE_WORDS(new_bit_count) * W8_BITS_PER_WORD;
        cursor_base = spill;
        word_count = W8_WHOLE_WORDS(new_bit_count) + 1;
        tail_mask = 0;
        cursor_bit = 0;
        while ((unsigned int)cursor_bit < spill) {
            tail_mask |= 1 << cursor_bit;
            ++cursor_bit;
        }

        if (puiIndex != 0) {
            free(puiIndex);
        }
        puiIndex = 0;
        if (new_bit_count != 0) {
            puiIndex = (unsigned int*)malloc(word_count * sizeof(unsigned int));
            if (puiIndex == 0) {
                srAssertFail("puiIndex", BITARRAY_CPP, 89,
                             "BitArray: Couldn't allocate bit index.");
            }
        }
        bit_count = new_bit_count;
    }

    memset(puiIndex, 0, word_count * sizeof(unsigned int));
}

/* Release the index. Nothing else is touched, so the caller owns whatever is
   left behind. */
// FUNCTION: WIZ8 0x0043ad90
void BitArray::FreeIndex()
{
    free(puiIndex);
}

/* Take on another array's bits. The source is resized to this array's bit
   count first, which clears it - so this copies a freshly sized array's zeroed
   words and then its count. */
// FUNCTION: WIZ8 0x0043ae80
void BitArray::CopyFrom(BitArray& other)
{
    other.SetSize(other.bit_count);
    memcpy(puiIndex, other.puiIndex, word_count * sizeof(unsigned int));
    set_count = other.set_count;
}

/* Raise one bit, answering whether it was already up. A bit past the end is
   refused. */
// FUNCTION: WIZ8 0x0043b390
bool BitArray::Set(unsigned int bit)
{
    unsigned int mask;
    unsigned int* word;

    if (bit > bit_count) {
        return false;
    }
    mask = 1 << (bit & 0x1f);
    word = &puiIndex[bit >> 5];
    if ((mask & *word) != 0) {
        return true;
    }
    ++set_count;
    *word |= mask;
    return false;
}

/* The same, except a bit past the end grows the array to reach it rather than
   being refused. */
// FUNCTION: WIZ8 0x0043b3d0
bool BitArray::SetAndGrow(unsigned int bit)
{
    unsigned int mask;
    unsigned int* word;

    if (bit > bit_count) {
        Grow(bit + W8_BITARRAY_GROWTH_SLACK, bit + W8_BITARRAY_GROWTH_SLACK);
    }
    mask = 1 << (bit & 0x1f);
    word = &puiIndex[bit >> 5];
    if ((mask & *word) != 0) {
        return true;
    }
    ++set_count;
    *word |= mask;
    return false;
}

/* Raise every bit, including the ones past the end in the last word - the
   count is set to the array's own length regardless. */
// FUNCTION: WIZ8 0x0043b420
bool BitArray::SetAll()
{
    unsigned int index;

    for (index = 0; index < word_count; ++index) {
        puiIndex[index] = 0xffffffff;
    }
    set_count = bit_count;
    return true;
}

/* Lower one bit, answering whether it had been up. */
// FUNCTION: WIZ8 0x0043b450
bool BitArray::Clear(unsigned int bit)
{
    unsigned int mask;
    unsigned int* word;

    if (bit <= bit_count) {
        word = &puiIndex[bit >> 5];
        mask = 1 << (bit & 0x1f);
        if ((mask & *word) != 0) {
            *word &= ~mask;
            --set_count;
            return true;
        }
    }
    return false;
}

/* Lower every bit and rewind the cursor. */
// FUNCTION: WIZ8 0x0043b490
void BitArray::ClearAll()
{
    memset(puiIndex, 0, word_count * sizeof(unsigned int));
    set_count = 0;
    cursor_base = 0;
    cursor_bit = 0;
    cursor_word = 0;
}

/* Whether one bit is up. */
// FUNCTION: WIZ8 0x0043b620
bool BitArray::Test(unsigned int bit)
{
    if (bit > bit_count) {
        return false;
    }
    return (puiIndex[bit >> 5] & (1 << (bit & 0x1f))) != 0;
}

/* Keep only the bits both arrays have. The shorter of the two bounds the walk,
   so this array's bits past the other's end are left alone. */
// FUNCTION: WIZ8 0x0043b4c0
bool BitArray::IntersectWith(BitArray& other)
{
    unsigned int shared = other.word_count;
    unsigned int index;

    if (word_count < other.word_count) {
        shared = word_count;
    }
    for (index = 0; index < shared; ++index) {
        puiIndex[index] &= other.puiIndex[index];
    }
    return true;
}

/* Take on every bit the other array has, over the same shared extent. */
// FUNCTION: WIZ8 0x0043b510
bool BitArray::UnionWith(BitArray& other)
{
    unsigned int shared = other.word_count;
    unsigned int index;

    if (word_count < other.word_count) {
        shared = word_count;
    }
    for (index = 0; index < shared; ++index) {
        puiIndex[index] |= other.puiIndex[index];
    }
    return true;
}

/* Become the complement of another array. The last word is masked back to
   whichever of the two arrays ends sooner, so the bits past the end do not
   come up; the count can only be derived when the two are the same length. */
// FUNCTION: WIZ8 0x0043b560
void BitArray::SetToComplementOf(BitArray& other)
{
    unsigned int shared;
    unsigned int last_mask;
    unsigned int index;

    memset(puiIndex, 0, word_count * sizeof(unsigned int));
    set_count = 0;
    cursor_base = 0;
    cursor_bit = 0;
    cursor_word = 0;

    shared = word_count;
    if (other.word_count <= word_count) {
        shared = other.word_count;
    }
    if (bit_count < other.bit_count) {
        last_mask = tail_mask;
    }
    else {
        last_mask = other.tail_mask;
    }

    for (index = 0; index < shared; ++index) {
        puiIndex[index] = ~other.puiIndex[index];
    }
    puiIndex[index - 1] &= last_mask;

    if (bit_count == other.bit_count) {
        set_count = bit_count - other.set_count;
    }
}

/* Count the bits that are up by walking them from the start, which is a
   different answer from the running count if anything has gone wrong with
   it. */
// FUNCTION: WIZ8 0x0043b5f0
int BitArray::CountSetBits()
{
    set_count = 0;
    while (NextSetBit(1) != 0) {
        ++set_count;
        NextSetBit(0);
    }
    return set_count;
}

/* The next bit that is up, numbered from one so that zero can mean the end.
   Restarting rewinds the cursor first, and answers -1 outright when there is
   no index to walk. Reaching the end rewinds so the next walk starts over. */
// FUNCTION: WIZ8 0x0043b660
int BitArray::NextSetBit(char restart)
{
    if (restart != 0) {
        if (puiIndex == 0) {
            return -1;
        }
        cursor_base = 0;
        cursor_bit = 0;
        cursor_word = 0;
    }

    while (cursor_base < bit_count) {
        if (puiIndex[cursor_word] != 0) {
            while ((unsigned int)cursor_bit < W8_BITS_PER_WORD) {
                if ((puiIndex[cursor_word] & (1 << (cursor_bit & 0x1f))) != 0) {
                    ++cursor_bit;
                    return cursor_base + cursor_bit;
                }
                ++cursor_bit;
            }
        }
        cursor_base += W8_BITS_PER_WORD;
        cursor_bit = 0;
        ++cursor_word;
    }

    cursor_base = 0;
    cursor_bit = 0;
    cursor_word = 0;
    return 0;
}

/* Grow to hold more bits, keeping whatever is already set. The old index is
   copied into the front of the new one and released; an array that had no
   index yet comes out with a word count of zero, which the first store then
   corrects. */
// FUNCTION: WIZ8 0x0043b700
void BitArray::Grow(unsigned int wanted_bits, unsigned int new_bit_count)
{
    unsigned int spill;
    unsigned int new_word_count;
    unsigned int index;
    unsigned int* pulNewArray;

    if (bit_count >= wanted_bits) {
        return;
    }

    spill = wanted_bits - W8_WHOLE_WORDS(wanted_bits) * W8_BITS_PER_WORD;
    new_word_count = W8_WHOLE_WORDS(wanted_bits) + 1;
    tail_mask = 0;
    for (index = 0; index < spill; ++index) {
        tail_mask |= 1 << index;
    }

    pulNewArray = (unsigned int*)malloc(new_word_count * sizeof(unsigned int));
    if (pulNewArray == 0) {
        srAssertFail("pulNewArray", BITARRAY_CPP, 741,
                     "BitArray Expansion: Couldn't allocate bit index.");
    }
    memset(pulNewArray, 0, new_word_count * sizeof(unsigned int));

    if (puiIndex != 0) {
        memcpy(pulNewArray, puiIndex, word_count * sizeof(unsigned int));
        free(puiIndex);
        new_word_count = 0;
    }
    word_count = new_word_count;
    puiIndex = pulNewArray;
    bit_count = new_bit_count;
}
