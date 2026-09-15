#ifndef WIZ8_LAYOUTS_PARTY_FORMATION_H
#define WIZ8_LAYOUTS_PARTY_FORMATION_H

#include <stddef.h>

#pragma pack(push, 1)

struct W8PartyFormationPosition {
    /* The row the slot stands in, -1 while unseated. */
    signed char bQuadrant;
    /* The quadrant remembered while the slot is unseated by death; the
       revival path in UpdateFormationSlotState re-seats into it and then
       clears it. */
    signed char bOldQuadrant;
    signed char bQuadrantSlot;
    signed char facing;
    unsigned char unknown_04[8];
};

struct W8PartyFormationState {
    signed char bOccupantChar[5][3];
    unsigned char ubQuadrantOccupants[5]; /* 0x0f */
    W8PartyFormationPosition positions[8];
    unsigned char unknown_74[0x10];
};

#pragma pack(pop)

static_assert(sizeof(W8PartyFormationPosition) == 0x0c, "W8PartyFormationPosition_must_be_0x0c");
static_assert(sizeof(W8PartyFormationState) == 0x84, "W8PartyFormationState_must_be_0x84");

#endif
