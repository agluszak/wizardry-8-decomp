#ifndef WIZ8_STARTUP_RUNTIME_STATE_H
#define WIZ8_STARTUP_RUNTIME_STATE_H

#include "wiz8/vector.h"

struct W8Character;

struct W8StartupStateElement005EE748 {
    W8StartupStateElement005EE748(
        W8Character* character, unsigned int type, int value_0c,
        unsigned int flags, int value_14);

    unsigned char handled_00;
    unsigned char unknown_01[3];
    W8Character* character_04;
    unsigned int type_08;
    int value_0c;
    unsigned int flags_10;
    int value_14;
    int value_18;
    int value_1c;
    unsigned char unknown_20[4];
    int item_id_24;
    unsigned char unknown_28[8];
    int value_30;
    unsigned char unknown_34[4];
};

static_assert(sizeof(W8StartupStateElement005EE748) == 0x38,
              "W8StartupStateElement005EE748_must_be_0x38");

class W8StartupStateVector005EE748
    : public W8GrowableVector<W8StartupStateElement005EE748*> {
};

class W8StartupStateVector005EE744
    : public W8GrowableVector<W8StartupStateElement005EE748*> {
};

struct W8StartupRuntimeState {
    W8StartupStateVector005EE748 vector_00;
    W8StartupStateVector005EE748 vector_10;
    W8StartupStateVector005EE748 vector_20;
    W8StartupStateVector005EE748 vector_30;
    W8StartupStateVector005EE744 vector_40;
    int value_50;
    int value_54;
    int unknown_58;
    int value_5c;
    int unknown_60;
    int value_64;
    unsigned char* bytes_68;

    W8StartupRuntimeState();
    ~W8StartupRuntimeState();
    void ClearOwnedEntries();
    int QueueEntry(W8StartupStateElement005EE748* entry);
    void SetEventCharacterMask(
        unsigned int event_type, unsigned int party_slot, unsigned char enabled);
    void ProcessOwnedEntry(W8StartupStateElement005EE748* entry);
    void ProcessNextPendingEntry();

};

static_assert(sizeof(W8StartupRuntimeState) == 0x6c, "W8StartupRuntimeState_must_be_0x6c");

#endif
