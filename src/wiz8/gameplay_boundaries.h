#ifndef WIZ8_GAMEPLAY_BOUNDARIES_H
#define WIZ8_GAMEPLAY_BOUNDARIES_H

#pragma pack(push, 1)

typedef struct W8Dice {
    short base;
    unsigned char count;
    unsigned char sides;
} W8Dice;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

int RollDice(const W8Dice* dice);
int IntegerPower(int base, unsigned int exponent);
void ClampInteger(int* value, int minimum, int maximum);

#ifdef __cplusplus
}
#endif

#endif
