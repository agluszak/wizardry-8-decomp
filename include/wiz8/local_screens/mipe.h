#pragma once

#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/stCube.h"

struct W8MipeMonsterEntry {
    wchar_t name[24];
    unsigned char kind;
    unsigned char selectable;
};

static_assert(sizeof(W8MipeMonsterEntry) == 0x32, "W8MipeMonsterEntry_size");

#pragma pack(push, 1)
struct W8MipeState {
    W8IList monster_ids;
    unsigned char unknown_0c[0x30];
    W8PList item_entries;
    unsigned char unknown_48[0x1c];
};
#pragma pack(pop)

static_assert(sizeof(W8MipeState) == 0x64, "W8MipeState_size");

void Function57D740(void);

unsigned char GetFlag68F105(void);
unsigned char GetFlag68F104(void);

void Function58AA20(int value);
void Function490210(void);
void Function48E420(W8WorldCursorNode0048DB30* node, int value_04, int value_08, float value_0c);
W8WorldCursorNode0048DB30* Function48ED10(int index);
void Function48DCA0(W8WorldCursorNode0048DB30* node);
