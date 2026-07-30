#pragma once

template <class Enum>
class srFlags {
public:
    srFlags() : value(0x0100241b) {}
    explicit srFlags(unsigned long bits) : value(bits) {}

    unsigned long value;
};

static_assert(sizeof(srFlags<int>) == 0x04, "srFlags_must_be_0x04");
