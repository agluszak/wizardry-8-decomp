#pragma once

#include <stddef.h>

#include "surrender/srMath.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/stCube.h"

class Trigger;
class W8Monster;
class W8Prop;
class W8WorldCursorNode0048DB30;
struct W8MonsterGenerator;

struct W8MipeMonsterEntry {
    wchar_t name[24];
    unsigned char kind;
    unsigned char selectable;
};

static_assert(sizeof(W8MipeMonsterEntry) == 0x32, "W8MipeMonsterEntry_size");

/* Local Screens\mipe.cpp's editor state, allocated once and stored in
   g_mipe_state_0068f100.  Offset zero is a live W8IList holding the selected
   monster location ids, and callers pass the state pointer itself to
   IListGetAt/ILLength. */
#pragma pack(push, 1)
struct W8MipeState {
    W8IList monster_ids; /* 0x00 */
    unsigned char unknown_0c[4];
    unsigned char selecting; /* 0x10 */
    unsigned char unknown_11[0x13];
    srVector3T<float> drag_anchor; /* 0x24 */
    unsigned char unknown_30;
    unsigned char dragging; /* 0x31 */
    unsigned char unknown_32[6];
    int waypoint_count;            /* 0x38 */
    W8PList waypoints;             /* 0x3c */
    W8Monster* monster;            /* 0x48 */
    float speed_step;              /* 0x4c */
    Trigger* trigger;              /* 0x50 */
    W8MonsterGenerator* generator; /* 0x54 */
    W8Prop* prop;                  /* 0x58 */
    unsigned char unknown_5c[8];
};
#pragma pack(pop)

static_assert(sizeof(W8MipeState) == 0x64, "W8MipeState_size");
static_assert(offsetof(W8MipeState, drag_anchor) == 0x24, "W8MipeState_drag_anchor");
static_assert(offsetof(W8MipeState, waypoints) == 0x3c, "W8MipeState_waypoints");
static_assert(offsetof(W8MipeState, generator) == 0x54, "W8MipeState_generator");

extern W8MipeState* g_mipe_state_0068f100;

void ShowMonsterSpeedStatus00577F10(void);
void ShowCubeParameters005780F0(void);
void ShowMonsterGeneratorStatus005781F0(void);
void ShowMonsterGeneratorEditor005782D0(void);

void Function57D740(void);

unsigned char GetFlag68F105(void);
unsigned char GetFlag68F104(void);

void Function58AA20(int value);
void Function490210(void);
void Function48E420(W8WorldCursorNode0048DB30* node, int value_04, int value_08, float value_0c);
W8WorldCursorNode0048DB30* Function48ED10(int index);
void Function48DCA0(W8WorldCursorNode0048DB30* node);
