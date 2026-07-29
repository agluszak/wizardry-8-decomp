#ifndef WIZ8_STARTUP_RUNTIME_STATE_H
#define WIZ8_STARTUP_RUNTIME_STATE_H

#include "wiz8/vector.h"

struct W8Character;

struct W8StartupStateElement005EE748 {
    unsigned char handled_00;
    unsigned char unknown_01[3];
    W8Character* character_04;
    unsigned int type_08;
    unsigned char unknown_0c[4];
    unsigned int flags_10;
    unsigned char unknown_14[0x10];
    int item_id_24;
};

class W8StartupStateVector005EE748
    : public W8GrowableVector<W8StartupStateElement005EE748*> {
};

class W8StartupStateVector005EE744
    : public W8GrowableVector<W8StartupStateElement005EE748*> {
public:
    W8StartupStateElement005EE748* RemoveEntryAt(int position);
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
    void ProcessNextPendingEntry();

};

typedef char W8StartupRuntimeState_must_be_0x6c[
    sizeof(W8StartupRuntimeState) == 0x6c ? 1 : -1];

#endif
