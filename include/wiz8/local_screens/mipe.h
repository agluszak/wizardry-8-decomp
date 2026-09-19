#pragma once

#include <stddef.h>

#include "surrender/srMath.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/stCube.h"

class Trigger;
class W8Monster;
class W8Prop;
class W8WorldCursorNode;
struct W8MonsterGenerator;

extern int g_mipe_cube_serial_006850ba;

struct W8MipeMonsterEntry {
    wchar_t name[24];
    unsigned char kind;
    bool selectable;
};

static_assert(sizeof(W8MipeMonsterEntry) == 0x32, "W8MipeMonsterEntry_size");

/* Local Screens\mipe.cpp's editor state, allocated once and stored in
   g_mipe_state_0068f100.  Offset zero is a live W8IList holding the selected
   monster location ids, and callers pass the state pointer itself to
   IListGetAt/ILLength. */
#pragma pack(push, 1)
struct W8MipeState {
    W8IList monster_ids; /* 0x00 */

    int value_0c;            /* 0x0c: group id of the last group pick; 1000000 = none */
    unsigned char selecting; /* 0x10 */
    unsigned char unknown_11[0x13];
    srVector3T<float> drag_anchor; /* 0x24 */
    unsigned char unknown_30;
    unsigned char dragging; /* 0x31 */
    unsigned char unknown_32[2];
    float value_34;                /* 0x34: initialised to 1.0 */
    int waypoint_count;            /* 0x38 */
    W8PList waypoints;             /* 0x3c */
    W8Monster* monster;            /* 0x48 */
    float speed_step;              /* 0x4c */
    Trigger* trigger;              /* 0x50 */
    W8MonsterGenerator* generator; /* 0x54 */
    W8Prop* prop;                  /* 0x58 */
    unsigned char unknown_5c[5];
    unsigned char flag_61; /* 0x61: initialised to 0xff */
    unsigned char unknown_62[2];
};
#pragma pack(pop)

static_assert(sizeof(W8MipeState) == 0x64, "W8MipeState_size");
static_assert(offsetof(W8MipeState, drag_anchor) == 0x24, "W8MipeState_drag_anchor");
static_assert(offsetof(W8MipeState, waypoints) == 0x3c, "W8MipeState_waypoints");
static_assert(offsetof(W8MipeState, generator) == 0x54, "W8MipeState_generator");

extern unsigned char g_debug_monster_cycle_0068f0fc;
extern W8MipeState* g_mipe_state_0068f100;

void ShowMonsterSpeedStatus00577F10(void);
void ShowCubeParameters005780F0(void);
void ShowMonsterGeneratorStatus005781F0(void);
void ShowMonsterGeneratorEditor005782D0(void);

void ToggleMipePanel0057D740(void);
/* Per-tick world-view pick while selecting: generator markers in mode 0x15,
   otherwise monster hover with single/group select semantics. */
void UpdateMipeSelection0057DC20(void);
void DragSelectionWithCursor0057DF80(void);
/* MIPE's world-view input dispatch: cube drag, cube pick, action menu. */
unsigned char MipeWorldViewEvent0057E0E0(int event, const POINT* point);

/* Any armed monster-generator marker within reach of the camera; sticky
   index resumes the scan at the last hit. Used with AnyWorldItemVisible to
   gate world-model picking. */
bool AnyMonsterGeneratorMarkerWithinReach(void); /* 0x0057E3C0 */

unsigned char GetFlag68F105(void);
unsigned char GetFlag68F104(void);
