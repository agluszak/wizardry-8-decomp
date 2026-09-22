#pragma once

#include <stddef.h>

#include "input.h"
#include "surrender/srMath.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "input.h"
#include "wiz8/engine_code/stCube.h"

class Trigger;
class W8Monster;
class W8Prop;
class W8WorldCursorNode;
struct MonGen;

enum { W8_MIPE_NO_GROUP = 1000000 };

struct W8MipeMonsterEntry {
    wchar_t name[24];
    unsigned char kind;
    bool selectable;
};

static_assert(sizeof(W8MipeMonsterEntry) == 0x32, "W8MipeMonsterEntry_size");

/* One row of mipeEdit.cpp's prop field editor: a label index into the
   g_mipe_prop_labels_0064e004 table plus the live value slots.  type 1 edits
   float_value, types 2/4/5 edit value as int/bool, 6 edits the heap text
   buffer and 7 picks one of option_count names starting at option_base in the
   g_mipe_key_names_0064ea04 table. */
struct W8MipeEditField {
    char type;                /* 0x00 */
    int label_index;          /* 0x04 */
    signed char option_count; /* 0x08 */
    signed char option_base;  /* 0x09 */
    wchar_t* text;            /* 0x0c */
    float float_value;        /* 0x10 */
    int value;                /* 0x14 */
};

static_assert(sizeof(W8MipeEditField) == 0x18, "W8MipeEditField_size");

/* Local Screens\mipe.cpp's editor state, allocated once and stored in
   g_mipe_state_0068f100.  Offset zero is a live W8IList holding the selected
   monster location ids, and callers pass the state pointer itself to
   IListGetAt/ILLength. */
struct W8MipeState {
    W8IList monster_ids; /* 0x00 */

    int selected_group_id;   /* 0x0c: group id of the last group pick; W8_MIPE_NO_GROUP = none */
    unsigned char selecting; /* 0x10 */
    unsigned char unknown_11[0x13];
    srVector3T<float> drag_anchor;    /* 0x24 */
    unsigned char creation_method_30; /* 0 exact, 1 placeholder, 2 selection */
    unsigned char dragging;           /* 0x31 */
    unsigned char unknown_32[2];
    float value_34;               /* 0x34: initialised to 1.0 */
    int waypoint_count;           /* 0x38 */
    W8PList waypoints;            /* 0x3c */
    W8Monster* monster;           /* 0x48 */
    float speed_step;             /* 0x4c */
    Trigger* trigger;             /* 0x50 */
    MonGen* generator;            /* 0x54 */
    W8Prop* prop;                 /* 0x58 */
    W8MipeEditField* edit_fields; /* 0x5c: mipeEdit.cpp prop field table */
    signed char edit_field_count; /* 0x60 */
    signed char edit_selection;   /* 0x61: -1 = no row selected */
    unsigned char unknown_62[2];
};

static_assert(sizeof(W8MipeState) == 0x64, "W8MipeState_size");
static_assert(offsetof(W8MipeState, drag_anchor) == 0x24, "W8MipeState_drag_anchor");
static_assert(offsetof(W8MipeState, waypoints) == 0x3c, "W8MipeState_waypoints");
static_assert(offsetof(W8MipeState, generator) == 0x54, "W8MipeState_generator");

extern unsigned char g_debug_monster_cycle_0068f0fc;
extern W8MipeState* g_mipe_state_0068f100;
extern int g_mipe_mode_0068f108;
extern int g_mipe_count_0068f10c;
/* First visible table row; shared by mipe.cpp's table view and mipeEdit.cpp's
   prop field editor. */
extern int g_mipe_table_base_0068f120;

/* mipeEdit.cpp: prop field-editor key handler, fed MSG wParam key values by
   mipe.cpp's mode-0xd dispatcher. */
void HandleMipeEditPropKey005C3880(unsigned short key);

void ShowMonsterSpeedStatus00577F10(void);
void ShowCubeParameters005780F0(void);
void ShowMonsterGeneratorStatus005781F0(void);

/* The MIPE input dispatcher fed from the main game input loop: routes key
   events through the mode state machine and returns nonzero when the event
   was consumed. */
unsigned char HandleMipeKey0057C230(const InputAtom* event);
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
/* MIPE's key-event handler; consumes the atom while the editor is open. */
