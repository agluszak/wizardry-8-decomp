#pragma once

#include <stddef.h>

#include "input.h"
#include "surrender/srMath.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "input.h"
#include "wiz8/engine_code/stCube.h"
#include <windows.h>

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
   g_mipe_prop_labels table plus the live value slots.  type 1 edits
   float_value, types 2/4/5 edit value as int/bool, 6 edits the heap text
   buffer and 7 picks one of option_count names starting at option_base in the
   g_mipe_key_names table. */
struct W8MipeEditField {
    char type;
    int label_index;
    signed char option_count;
    signed char option_base;
    wchar_t* text;
    float float_value;
    int value;
};

W8_ABI_ASSERT(sizeof(W8MipeEditField) == 0x18, "W8MipeEditField_size");

/* The editor state, allocated once and stored in g_mipe_state. It begins
   with the W8IList of selected monster location ids, and callers pass the
   state pointer itself to IListGetAt/ILLength. */
struct W8MipeState {
    W8IList monster_ids;

    int selected_group_id; /* group id of the last group pick; W8_MIPE_NO_GROUP = none */
    unsigned char selecting;
    unsigned char unknown_11[0x13];
    srVector3T<float> drag_anchor;
    unsigned char creation_method; /* 0 exact, 1 placeholder, 2 selection */
    bool dragging;
    unsigned char padding_32[2];
    float value_34; /* initialised to 1.0 */
    int waypoint_count;
    W8PList waypoints;
    W8Monster* monster;
    float speed_step;
    Trigger* trigger;
    MonGen* generator;
    W8Prop* prop;
    W8MipeEditField* edit_fields; /* mipeEdit.cpp prop field table */
    signed char edit_field_count;
    signed char edit_selection; /* -1 = no row selected */
};

W8_ABI_ASSERT(sizeof(W8MipeState) == 0x64, "W8MipeState_size");
W8_ABI_ASSERT(offsetof(W8MipeState, drag_anchor) == 0x24, "W8MipeState_drag_anchor");
W8_ABI_ASSERT(offsetof(W8MipeState, waypoints) == 0x3c, "W8MipeState_waypoints");
W8_ABI_ASSERT(offsetof(W8MipeState, generator) == 0x54, "W8MipeState_generator");

extern bool g_debug_monster_cycle;
extern W8MipeState* g_mipe_state;
extern int g_mipe_mode;
extern int g_mipe_count;
/* First visible table row; shared by mipe.cpp's table view and mipeEdit.cpp's
   prop field editor. */
extern int g_mipe_table_base;

/* mipeEdit.cpp: prop field-editor key handler, fed MSG wParam key values by
   mipe.cpp's mode-0xd dispatcher. */
void HandleMipeEditPropKey(unsigned short key);

void ShowMonsterSpeedStatus(void);
void ShowCubeParameters(void);
void ShowMonsterGeneratorStatus(void);

/* The MIPE input dispatcher fed from the main game input loop: routes key
   events through the mode state machine and returns nonzero when the event
   was consumed. */
unsigned char HandleMipeKey(const InputAtom* event);
void ShowMonsterGeneratorEditor(void);

void ToggleMipePanel(void);
/* Per-tick world-view pick while selecting: generator markers in mode 0x15,
   otherwise monster hover with single/group select semantics. */
void UpdateMipeSelection(void);
void DragSelectionWithCursor(void);
/* MIPE's world-view input dispatch: cube drag, cube pick, action menu. */
bool MipeWorldViewEvent(int event, const POINT* point);

/* Any armed monster-generator marker within reach of the camera; sticky
   index resumes the scan at the last hit. Used with AnyWorldItemVisible to
   gate world-model picking. */
bool AnyMonsterGeneratorMarkerWithinReach(void);

bool IsMipeActive(void);
bool IsMipeMenuActive(void);
/* MIPE's key-event handler; consumes the atom while the editor is open. */
