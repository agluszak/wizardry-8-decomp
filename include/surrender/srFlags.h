#pragma once

template <class Enum> class srFlags {
public:
    srFlags() : value(0) {}
    explicit srFlags(unsigned long bits) : value(bits) {}

    /* Set or clear one flag bit. Monster.cpp's damage-number poster raises a
       model instance's alignment flag through it; the out-of-line emission is
       the 0x004CA880 explicit specialization there. */
    void set(int bit, int on);

    unsigned long value;
};

static_assert(sizeof(srFlags<int>) == 0x04, "srFlags_must_be_0x04");
