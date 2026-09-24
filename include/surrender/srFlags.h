#pragma once

template <class Enum> class srFlags {
public:
    /* Retail's srGERD constructor emits __ehvector_ctor over
       enable_stack_216c_ with the standalone srFlags<e_enable>::srFlags
       emission (0x1001EF50) as the element callback; clang-cl folds the
       provably-constant body to a rep stos — a documented lowering gap. */
    srFlags();
    explicit srFlags(unsigned long bits) : value(bits) {}

    void set(int bit, int on);

    unsigned long value;
};

template <class Enum> srFlags<Enum>::srFlags() : value(0)
{
}

/* Set or clear one flag bit. The 0x004CA880 srFlags<int>::set body in WIZ8 is
   an ordinary primary-template emission, not evidence of an authored int
   specialization. */
template <class Enum> void srFlags<Enum>::set(int bit, int on)
{
    if (on != 0) {
        value |= 1 << bit;
        return;
    }
    value &= ~(1u << bit);
}

static_assert(sizeof(srFlags<int>) == 0x04, "srFlags_must_be_0x04");
