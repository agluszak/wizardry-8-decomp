#pragma once

struct W8Dice {
    /* Smallest and largest results of `count` d`sides` plus `base`. */
    int Minimum() const
    {
        return base + count;
    }
    int Maximum() const
    {
        return base + count * sides;
    }

    short base;
    unsigned char count;
    unsigned char sides;
};

static_assert(sizeof(W8Dice) == 4, "W8Dice_must_be_4");
