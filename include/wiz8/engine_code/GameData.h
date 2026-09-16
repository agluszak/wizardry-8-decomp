#pragma once

#include "wiz8/sgp_bridge.h"

#include "Types.h"
#include "wiz8/engine_code/IntervalGate.h"

extern unsigned char g_flag_00652dce;

void CopyLevelDataHandle(unsigned long* destination, const unsigned long* source);
void Function41EEE0(float movement_limit, char reset, char fast_move); /* 0x0041EEE0 */

#include "wiz8/geometry.h"

#pragma pack(push, 1)

/* The current level-data record at 0x00652DAC — a 0xf4-byte allocation whose
   first 0xac bytes mix flag, counter and camera-vector state. GameData.cpp
   establishes the flag word and optional vector; Camera.cpp establishes the
   two derived forward vectors and the scale used to produce the second.
   Retail allocates 0xf4 and its constructor builds a W8IntervalGate at +0xc4;
   the record's destructor at 0x00421890 exists only to tear that member down
   (it is the body previously read as a bare `add ecx,0xc4` adjustor). */
struct W8LevelDataRecord {
    unsigned int flags; /* 0x00 */
    unsigned char unknown_04[0x10];
    float camera_scale_14; /* 0x14 */
    unsigned char unknown_18[0x0c];
    /* 0x24/0x28: pending elapsed times ConsumeLevelElapsedTime0041F170 hands
       to the movement/fatigue pass, then clears. */
    float real_elapsed_24;
    float frame_elapsed_28;
    unsigned char unknown_2c[0x14];
    srVector3T<float> vector_40;         /* 0x40 */
    srVector3T<float> camera_forward_4c; /* 0x4c */
    unsigned char unknown_58[0x0c];
    srVector3T<float> vector_64;                /* 0x64 */
    srVector3T<float> vector_70;                /* 0x70 */
    srVector3T<float> scaled_camera_forward_7c; /* 0x7c */
    float vector_88[3];                         /* 0x88 */
    unsigned char unknown_94[0x0c];
    srVector3T<float> vector_a0; /* 0xa0 */
    unsigned char unknown_ac[0x18];
    W8IntervalGate interval_gate_c4; /* 0xc4 */
    unsigned char flag_ec;           /* 0xec */
    unsigned char flag_ed;           /* 0xed */
    unsigned char pad_ee[2];
    int value_f0; /* 0xf0 */

    ~W8LevelDataRecord(); /* 0x00421890 */
};

struct W8OctBuildTree00446390;
class Trigger;
class srCamera;
class srNode;
class W8Octree;
struct W8OctreeTrace;
struct W8World;

class BitArray;

/* One environment record: seventeen dwords mixing counters and factors. */
struct W8EnvironRecord {
    int value_00;
    unsigned char value_04;
    unsigned char value_05;
    unsigned char pad_06[2];
    int value_08;
    int unknown_0c;
    float value_10;
    float value_14;
    float value_18;
    float value_1c;
    float value_20;
    int value_24;
    float value_28;
    int value_2c;
    float value_30;
    float value_34;
    float value_38;
    float value_3c;
    float value_40;

    unsigned char RescaleToReference(const W8EnvironRecord* reference);
};

static_assert(sizeof(W8EnvironRecord) == 0x44, "W8EnvironRecord_must_be_0x44");

class BitArray;

/* Switch-interface record, 0xc bytes in the processed level data: the count
   and first index of the interface's state records in
   W8GameData::interface_states_6c. Field +0 is the interface's own id. */
struct W8GDInterface {
    int id_00;
    int state_count_04;
    int state_first_08;
};

/* One switch-interface state, 0xc bytes: the group id the selected state is
   compared against, and the count/first-index slice of conditional-poly
   surface indexes in W8GameData::cond_polys_74. */
struct W8GDInterfaceState {
    int group_00;
    int poly_count_04;
    int poly_first_08;
};

/* The processed game-data record the octree and world own. The proven prefix
   above is consumed by GameData.cpp and OctBuildTree.cpp; the constructor
   below establishes the rest: two bit sets, paired count/allocation blocks,
   a counted pointer array, the environment count/array pair, and a trailing
   flag. Only straightforward storage is claimed past the prefix. */
struct W8GameData {
    W8GameData(int handle, void* parent);   /* 0x00449010 */
    ~W8GameData();                          /* 0x00449BB0 */
    void ReadProcessedGameData(int handle); /* 0x00449240 */
    /* Writes the game-data block WriteOctFile appends after the terminator. */
    unsigned char WriteGameData0044AA40(int handle); /* 0x0044AA40 */
    unsigned char Function447660(void* file, int index);
    /* Release the level-data record, game-time accumulator and companion
       level-data globals; runs first in ~W8GameData. */
    void ReleaseLevelData0041A9E0();
    /* Selects a switch interface's state: the matching state group's
       conditional polygons clear surface flag 0x10 while every other state's
       polygons set it. */
    char SetInterfaceState(int interface_id, int state); /* 0x0041C680 */
    /* Apply one surface the mover's trace crossed, nearest first: an
       environment boundary (flag 0x1000) swaps the active environment record,
       carrying the live blend fields across; a trigger surface runs its
       trigger in the crossing direction unless the bit sets already hold
       it. */
    void ProcessCrossedSurface(W8GDSurface* surface); /* 0x0041C770 */
    /* Builds the octree trace model and answers its scene node. */
    srNode* CreateTraceModel0041C930(); /* 0x0041c930 */

    W8OctBuildTree00446390* geometry_index_00;
    /* +0x04: the loading octree's back-pointer, stored by W8Octree's file-load
       finish path (retail writes [ESI+4], not +0) and tested by trigger
       integration. geometry_index_00 above is untouched by that store. */
    W8Octree* positional_04;
    srVector3T<float> minimum_08;
    srVector3T<float> maximum_14;
    int vertex_count_20;
    srVector3T<float>* vertices_24;
    int surface_count_28;
    int positional_2c_00;
    int positional_2c_04;
    int integrated_surface_count_34;
    W8GDSurface* surfaces_38;
    int overflow_surface_count_3c;
    int overflow_vertex_count_40;
    int total_surface_count_44;
    W8GDSurface* overflow_surfaces_48;
    srVector3T<float>* overflow_vertices_4c;
    Trigger** trigger_table_50;
    int value_54;
    BitArray* bits_58;
    BitArray* bits_5c;
    int interface_count_60;
    W8GDInterface* interfaces_64;
    int interface_state_count_68;
    W8GDInterfaceState* interface_states_6c;
    int cond_poly_count_70;
    int* cond_polys_74;
    int count_78;
    void** array_7c;
    int environ_count_80;
    W8EnvironRecord** environs_84;
    unsigned char value_88;
    unsigned char pad_89[3];

    void IntegrateTriggers();
    void AddTriggerPlane(const srVector3T<float>* vertices, Trigger* trigger);

    /* Loop the buffered prop ids through TestProp and return the id of the
       last prop that reported a hit, or -1. */
    int TestPropSurfaces(int count, unsigned long* ids, W8OctreeTrace* trace, char skip_flag,
                         char gate); /* 0x0041C0D0 */
    /* Ray-test one collidable prop: swaps the prop's surface/vertex arrays
       into this context, traces in prop-local space through the prop's
       position delta when no pre-tree exists (reseeding the caller's record
       to the world-space hit), and restores the arrays. `gate` skips flag-4
       props when set. */
    unsigned char TestProp(int prop_id, W8OctreeTrace* trace, char skip_flag,
                           char gate); /* 0x0041C140 */
    /* Ray-test `count` surfaces - all of surfaces_38 when `surface_ids` is
       null, else the listed surface indexes - against the trace record.
       value_88, flag and mode filters apply; a closer hit stores index_04
       into value_54, the contact into the record's end_0c and the distance
       into hit_limit_24. */
    char TestTraceResult(int count, unsigned long* surface_ids, W8OctreeTrace* trace,
                         char skip_flag, int mode); /* 0x0041C330 */
};

static_assert(sizeof(W8GameData) == 0x8c, "W8GameData_must_be_0x8c");

#pragma pack(pop)

static_assert(sizeof(W8LevelDataRecord) == 0xf4, "W8LevelDataRecord_must_be_0xf4");

extern W8LevelDataRecord* g_level_data_00652dac;
/* Companion pointer cleared alongside g_level_data_00652dac on level
   transitions; its target's +0 flags have 0x200 masked off at 0x0044FCD0. */
extern unsigned int* g_level_data_sibling_00652da8;
/* Teardown flag tested and cleared by ReleaseLevelData0041A9E0. */
extern unsigned char g_flag_00652dcc;
/* Read by the level-data reset and written by the GameData constructor, which
   now lives in GDFileIO.cpp. */
extern W8EnvironRecord* g_environ_00652DB4;

#include "wiz8/engine_code/GDFileIO.h"

void ResetInactiveLevelDataVectors0041EF50(void);
void UpdateSharedGameDataObject0041F1F0();
void UpdateGameDataRuntime0041F260();
unsigned char LoadSurfaceVertices004214D0(srVector3T<float>* output, const int* vertex_indices);

void ClearLevelDataFlag6(void);
void ResetLevelDataVectors0041F0D0(void);
int IsLevelDataFlag4EffectivelySet(void);
void ResetCurrentEnvironment0041AA40(void);
unsigned char SetEnvironmentLoadFlag(unsigned char flag); /* 0x0041AAE0 */
void BeginCameraSway0041A960(void);
void EndCameraSway0041A9A0(void);

unsigned int GetLevelDataFlag6(void);
unsigned char ConsumeLevelElapsedTime0041F170(float* real_elapsed, float* frame_elapsed);

/* 0x00420BD0: settle a world point onto the octree ground through the
   GameData geometry index; the false branch reports the input height and
   clears the caller's hit byte. */
float SettlePositionToGround00420BD0(const srVector3T<float>* position, unsigned char* hit);
/* 0x00420C30: same ground-settle query with a fixed 500-unit probe range,
   returning the resulting height. */
float SettlePositionToGround00420C30(srVector3T<float>* position, unsigned char* hit);

void ClearLevelDataFlags5To7(void); /* 0x0041F0C0 */
srCamera* CreateOrSetGameCamera(srNode* parent, srCamera* camera);
float GetCameraYawInDegrees();
float GetCameraYawRadians();
float GetCameraPitchInDegrees();
float GetCameraPitchRadians();
void GetCameraOrientation(float* angle, float* pitch);
void BeginManualCameraControl();
void LevelCamera();
void CameraLookAt(const srVector3T<float>* position);     /* 0x00420F90 */
void CameraSnapToTarget(const srVector3T<float>* target); /* 0x00420FB0 */
void TurnCameraToDegrees(float degrees);
void SetCameraYawDegrees(float degrees);
void ApplyCameraRotation(srMatrix3T<float>* rotation);
void SetCameraOrientation(float* angle, float* pitch, srMatrix3T<float>* rotation);
/* 0x00420F40: camera yaw in whole degrees plus an optional yaw-rotation
   matrix copy. */
int GetCameraYawAndRotation00420F40(srMatrix3T<float>* rotation);
/* 0x00421440: project `vector` onto `onto` in place; fails on a degenerate
   target direction. */
unsigned char ProjectVectorOntoVector00421440(srVector3T<float>* vector,
                                              const srVector3T<float>* onto);
/* 0x00421570: restore a saved yaw/pitch into the game camera, reading the
   world camera node's current rotation first and fetching the updated matrix
   (both into the same dead local in retail). */
void RestoreWorldCameraOrientation00421570(float* angle, float* pitch, W8World* world);
void GetCameraPosition(srVector3T<float>* position);
int GetCameraYawDegrees(void);
/* 0x004215E0: point-visibility query through the world octree; false with no
   world, true for a loaded world without an octree. */
bool HasCameraLineOfSight(const srVector3T<float>* position);
void PlacePartyAtPoint(const srVector3T<float>* point);
