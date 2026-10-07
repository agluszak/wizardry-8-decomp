#pragma once

template <class Enum> class srFlags {
public:
    srFlags();
    explicit srFlags(unsigned long bits) : value(bits) {}

    void set(int bit, int on);

    unsigned long value;
};

template <class Enum> srFlags<Enum>::srFlags() : value(0) {}

/* Set or clear one flag bit. */
template <class Enum> void srFlags<Enum>::set(int bit, int on)
{
    if (on != 0) {
        value |= 1 << bit;
        return;
    }
    value &= ~(1u << bit);
}

static_assert(sizeof(srFlags<int>) == 0x04, "srFlags_must_be_0x04");
