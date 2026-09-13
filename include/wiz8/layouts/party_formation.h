#ifndef WIZ8_LAYOUTS_PARTY_FORMATION_H
#define WIZ8_LAYOUTS_PARTY_FORMATION_H

#include <stddef.h>

#pragma pack(push, 1)

struct W8PartyFormationRow {
    signed char slots[3];
};

struct W8PartyFormationPosition {
    unsigned char row;
    unsigned char unknown_01[2];
    signed char facing;
    unsigned char unknown_04[8];
};

struct W8PartyFormationState {
    W8PartyFormationRow rows[5];
    unsigned char flags_0f[5];
    W8PartyFormationPosition positions[8];
    unsigned char unknown_74[0x10];
};

#pragma pack(pop)

static_assert(sizeof(W8PartyFormationRow) == 0x03, "W8PartyFormationRow_must_be_0x03");
static_assert(sizeof(W8PartyFormationPosition) == 0x0c, "W8PartyFormationPosition_must_be_0x0c");
static_assert(sizeof(W8PartyFormationState) == 0x84, "W8PartyFormationState_must_be_0x84");

#endif
