#include "gameplay_boundaries.h"

extern unsigned int GetRandomNumber(unsigned int upper_bound);

// FUNCTION: WIZ8 0x00517970
int RollDice(const W8Dice* dice)
{
    unsigned int roll;
    int result = dice->base;

    for (roll = 0; roll < dice->count; ++roll) {
        result += GetRandomNumber(dice->sides) + 1;
    }
    return result;
}

// FUNCTION: WIZ8 0x005179B0
int IntegerPower(int base, unsigned int exponent)
{
    int result;

    for (result = 1; exponent > 0; --exponent) {
        result *= base;
    }
    return result;
}

// FUNCTION: WIZ8 0x005179D0
void ClampInteger(int* value, int minimum, int maximum)
{
    int current = *value;

    if (current > maximum) {
        *value = maximum;
    } else if (current < minimum) {
        *value = minimum;
    }
}
