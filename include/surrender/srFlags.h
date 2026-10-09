#pragma once

template <class Enum> class srFlags {
public:
    srFlags();
    explicit srFlags(w8_ulong bits) : value(bits) {}

    void set(int bit, int on);

    w8_ulong value;
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

W8_ABI_ASSERT(sizeof(srFlags<int>) == 0x04, "srFlags_must_be_0x04");
