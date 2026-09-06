#pragma once

#pragma pack(push, 1)
struct W8Dice {
    short base;
    unsigned char count;
    unsigned char sides;
};
#pragma pack(pop)

static_assert(sizeof(W8Dice) == 4, "W8Dice_must_be_4");
