#pragma once

struct W8Dice {
    short base;
    unsigned char count;
    unsigned char sides;
};

static_assert(sizeof(W8Dice) == 4, "W8Dice_must_be_4");
