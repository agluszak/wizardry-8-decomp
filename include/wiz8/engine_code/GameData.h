#pragma once

#include "wiz8/sgp_bridge.h"

#include "Types.h"
#include "wiz8/engine_code/IntervalGate.h"
#include "wiz8/wiz8_windows.h"

extern unsigned char g_flag_00652dce;

void ResetLevelMovement0041EEE0(float movement_limit, char reset, char fast_move); /* 0x0041EEE0 */

#include "wiz8/geometry.h"
#include "wiz8/layouts/world.h"

/* The current level-data record at 0x00652DAC — a 0xf4-byte allocation whose
   first 0xac bytes mix flag, counter and camera-vector state. GameData.cpp
   establishes the flag word and optional vector; Camera.cpp establishes the
   two derived forward vectors and the scale used to produce the second.
   Retail allocates 0xf4 and its constructor builds a W8IntervalGate at +0xc4;
   the record's destructor at 0x00421890 exists only to tear that member down
   (it is the body previously read as a bare `add ecx,0xc4` adjustor). */
struct W8LevelDataRecord {
    unsigned int flags; /* 0x00 */
    /* 0x04/0x08: prop ids filled by the motion collision path; ToggleBoundProps
       flips setting-6e props referenced here. */
    int primary_contact_prop_id;   /* 0x04 */
    int secondary_contact_prop_id; /* 0x08 */
    signed char sound_environment_0c;
    signed char sound_environment_alt_0d;
    unsigned char pad_0e[2];
    float footstep_accumulator_10; /* 0x10 */
    float camera_scale_14;         /* 0x14 */
    /* 0x18/0x1c: residual segment length and facing metric from the nearest
       contact during UpdateWorldCameraAndPaths. */
    float residual_contact_length_18; /* 0x18 */
    float contact_facing_1c;          /* 0x1c */
    float speed_20;                   /* 0x20 */
    /* 0x24/0x28: pending elapsed times ConsumeLevelElapsedTime0041F170 hands
       to the movement/fatigue pass, then clears. */
    float real_elapsed_24;
    float frame_elapsed_28;
    float movement_limit_2c;
    float movement_progress_30;
    srVector3T<float> camera_position_34;       /* 0x34 */
    srVector3T<float> vector_40;                /* 0x40 */
    srVector3T<float> camera_forward_4c;        /* 0x4c */
    srVector3T<float> vector_58;                /* 0x58 */
    srVector3T<float> vector_64;                /* 0x64 */
    srVector3T<float> vector_70;                /* 0x70 */
    srVector3T<float> scaled_camera_forward_7c; /* 0x7c */
    srVector3T<float> vector_88;                /* 0x88 */
    srVector3T<float> vector_94;                /* 0x94 */
    srVector3T<float> vector_a0;                /* 0xa0 */
    /* 0xac: contact surface normal; 0xb8: scalar stored beside it (retail
       constructor writes 1.0f). Remaining 8 bytes stay unresolved. */
    srVector3T<float> contact_normal_ac; /* 0xac */
    float contact_normal_scale_b8;       /* 0xb8 */
    unsigned char unknown_bc[8];         /* 0xbc */
    W8IntervalGate interval_gate_c4;     /* 0xc4 */
    unsigned char flag_ec;               /* 0xec */
    unsigned char flag_ed;               /* 0xed */
    unsigned char pad_ee[2];
    float value_f0; /* 0xf0 */

    W8LevelDataRecord();  /* 0x0041FD10 */
    ~W8LevelDataRecord(); /* 0x00421890 */
    /* 0x0041FE20: when the camera sits outside the game-data AABB, push
       vector_a0 toward the box, clear environ vector_24, and optionally start
       party movement; returns non-zero when a clamp fired. */
    unsigned char ClampCameraToBounds0041FE20(const srVector3T<float>* minimum,
                                              const srVector3T<float>* maximum);
    /* 0x0041FF00: toggle setting-6e props referenced by primary_contact_prop_id/secondary_contact_prop_id. */
    unsigned char ToggleBoundProps0041FF00();
    /* 0x00420470: integrate camera_forward into vector_64/vector_70. */
    unsigned char IntegrateCameraForward00420470();
    /* 0x00420810: rotate vector_40 by the saved yaw matrix and refresh
       vector_a0; returns the updated fast-move latch. */
    unsigned char ApplySavedMotionMatrix00420810(unsigned char prior_fast, unsigned char fast_move,
                                                 const srMatrix3T<float>* saved);
    /* 0x0041FF90: advance movement progress / footstep state for one tick. */
    void UpdateMotionProgress0041FF90(unsigned char fast_move, unsigned char moved);
    /* 0x00420A60: accumulate footstep distance and optionally play a step. */
    unsigned char UpdateFootstepFromMotion00420A60();
};

struct W8OctBuildTree00446390;
class Trigger;
class srCamera;
class srNode;
class stModelInstance;
class W8Octree;
struct W8OctreeTrace;
struct W8World;
struct W8LevelFilePlane;

class BitArray;

/* One environment record: seventeen dwords mixing counters and factors. */
struct W8EnvironRecord {
    int value_00;
    unsigned char value_04;
    unsigned char value_05;
    unsigned char pad_06[2];
    int value_08;
    /* Per-frame scale copied from the level camera_scale; 0x00421850 multiplies
       vector_24 by it when advancing the camera under environment load. */
    float scale_0c;
    float value_10;
    float value_14;
    float value_18;
    float value_1c;
    float value_20;
    srVector3T<float> vector_24;
    float value_30;
    float value_34;
    float value_38;
    float value_3c;
    float value_40;

    unsigned char RescaleToReference(const W8EnvironRecord* reference);
    /* 0x00421800: store `motion` / scale_0c as the per-frame vector_24. */
    void SetScaledMotion00421800(const srVector3T<float>* motion);
    /* 0x00421850: add vector_24 * scale_0c into `position`. */
    void AddScaledMotion00421850(srVector3T<float>* position);
};

static_assert(sizeof(W8EnvironRecord) == 0x44, "W8EnvironRecord_must_be_0x44");

class BitArray;

/* Switch-interface record, 0xc bytes in the processed level data: the count
   and first index of the interface's state records in W8GameData::m_pStates.
   Field +0 is the interface's own id. Retail's SetInterfaceState assertion
   names the first-index member iStates. */
struct W8GDInterface {
    int id_00;
    int state_count_04;
    int iStates;
};

/* One switch-interface state, 0xc bytes: the group id the selected state is
   compared against, and the count/first-index slice of conditional-poly
   surface indexes in W8GameData::m_piCondPolys. */
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
    W8GameData(int handle, bool secondary); /* 0x00449010 */
    ~W8GameData();                          /* 0x00449BB0 */
    void ReadProcessedGameData(int handle); /* 0x00449240 */
    /* Writes the game-data block WriteOctFile appends after the terminator. */
    unsigned char WriteGameData0044AA40(int handle); /* 0x0044AA40 */
    /* Reads one WGD vertex/polygon list: counts, the scaled vertex bank with
       unscaled bounds tracking, the face records, and — for the non-primary
       pass — the interface name and conditional-face records. */
    unsigned char ReadWGDList00447660(HANDLE file, int poly_type);
    /* Builds m_pInterfaces/m_pStates/m_piCondPolys from the {interface id,
       surface index, group} triples collected by ReadWGDList. */
    void CompileGDInterfaces00447FB0(const int* records, int count);
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
    /* Builds the octree trace mesh and answers its model instance. */
    stModelInstance* CreateTraceModel0041C930(); /* 0x0041c930 */
    /* 0x0041F330: apply world-render camera-motion flags into `rotation` and
       mirror the result into `saved`. Retail call sites pass the owning
       W8GameData in ECX even though the body reads only globals. */
    void ApplyCameraMotionFlags0041F330(unsigned int flags, srMatrix3T<float>* rotation,
                                        srMatrix3T<float>* saved);
    /* 0x0041F5F0: advance the camera position under the same flag set; writes
       the delta into `delta` and answers whether the position changed. */
    unsigned char ApplyCameraMotion0041F5F0(unsigned int flags, srVector3T<float>* position,
                                            srVector3T<float>* delta, srMatrix3T<float>* saved);
    /* 0x0041AB40: advance camera under environment-load motion, tracing
       props/octree/geometry and applying crossed surfaces. */
    unsigned char AdvanceEnvironmentMotion0041AB40();
    /* 0x0041BD60: push the motion delta away from nearby active monsters. */
    unsigned char ProbeMonstersAlongMotion0041BD60(srVector3T<float>* direction,
                                                   srVector3T<float>* position, int mode);
    /* 0x0041B770: probe active collidable props along the motion segment;
       returns the nearest hit surface and may adjust `direction`. */
    W8GDSurface* ProbePropsAlongMotion0041B770(srVector3T<float>* direction,
                                               srVector3T<float>* position,
                                               srVector3T<float>* scratch, float* nearest_distance);

    W8OctBuildTree00446390* geometry_index_00;
    /* +0x04: the loading octree's back-pointer, stored by W8Octree's file-load
       finish path (retail writes [ESI+4], not +0) and tested by trigger
       integration. geometry_index_00 above is untouched by that store. */
    W8Octree* positional_04;
    srVector3T<float> minimum_08;
    srVector3T<float> maximum_14;
    /* Member names through m_ppEnvirons are proven by retail assertion strings
       in ReadProcessedGameData/SetInterfaceState; each m_iNum* count pairs the
       proven array member it counts. */
    int m_iNumVertices;
    srVector3T<float>* m_pVertices;
    int m_iNumSurfaces;
    int positional_2c_00;
    int positional_2c_04;
    int integrated_surface_count_34;
    W8GDSurface* m_pSurfaces;
    int m_iNumTrigSurfaces;
    int m_iNumTrigVertices;
    int m_iNumTriggers;
    W8GDSurface* m_pTrigSurfaces;
    srVector3T<float>* m_pTrigVertices;
    Trigger** m_ppTriggers;
    int value_54;
    BitArray* bits_58;
    BitArray* bits_5c;
    int m_iNumInterfaces;
    W8GDInterface* m_pInterfaces;
    int m_iNumStates;
    W8GDInterfaceState* m_pStates;
    int m_iNumCondPolys;
    int* m_piCondPolys;
    int m_iNumNames;
    char** m_ppNames;
    int m_iNumEnvirons;
    W8EnvironRecord** m_ppEnvirons;
    unsigned char value_88;
    unsigned char pad_89[3];

    void IntegrateTriggers();
    void AddTriggerPlane(const srVector3T<float>* vertices, Trigger* trigger);
    /* Registers a linked record's twelve generated surfaces; the `face`-indexed
       surface also spawns an environment record scaled by `value`. */
    void AddTriggerPlane(const srVector3T<float>* vertices, float value, float scalar,
                         const signed char* face); /* 0x00448C60 */
    /* Grows the environment bank by tens and appends a record whose motion
       derives from the linked surface's scaled plane. */
    void CreateGDEnviron00448E60(const W8GDSurface* surface, float scale);
    /* Folds the trigger vertex/surface banks into the main arrays without
       rebuilding the spatial index; the CompileGameData00449D10 path. */
    void IntegrateTriggerGeometry00448A60();
    /* 1-based ordinal of the m_ppNames entry whose name matches, else -1. */
    int FindPointerByName004482A0(const char* name); /* 0x004482A0 */
    /* Registers one invisible-plane record; the OctBuild driver feeds it the
       level file's plane table. */
    void AddLevelPlane004485F0(W8LevelFilePlane* plane); /* 0x004485F0 */
    /* Copies the linked record's 0x1b0-byte payload into a scratch entry and
       forwards to the 0x00448C60 helper. Retail call sites pass record + 1
       (the payload), the +0x1b3 float, the +0x1b7 scalar and a pointer to the
       +0x1b1 face byte. */
    void AddLinkedRecord00448BF0(const srVector3T<float>* vertices, float value, float scalar,
                                 const signed char* face); /* 0x00448BF0 */
    /* Compiles the read game data into the shared build arrays. */
    void CompileGameData00449D10(); /* 0x00449D10 */

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
    /* Ray-test `count` surfaces - all of m_pSurfaces when `surface_ids` is
       null, else the listed surface indexes - against the trace record.
       value_88, flag and mode filters apply; a closer hit stores index_04
       into value_54, the contact into the record's end_0c and the distance
       into hit_limit_24. */
    char TestTraceResult(int count, unsigned long* surface_ids, W8OctreeTrace* trace,
                         char skip_flag, int mode); /* 0x0041C330 */
};

static_assert(sizeof(W8GameData) == 0x8c, "W8GameData_must_be_0x8c");

static_assert(offsetof(W8LevelDataRecord, primary_contact_prop_id) == 0x04,
              "W8LevelDataRecord_primary_contact_prop_id");
static_assert(offsetof(W8LevelDataRecord, residual_contact_length_18) == 0x18,
              "W8LevelDataRecord_residual_contact_length_18");
static_assert(offsetof(W8LevelDataRecord, contact_normal_ac) == 0xac,
              "W8LevelDataRecord_contact_normal_ac");
static_assert(offsetof(W8LevelDataRecord, contact_normal_scale_b8) == 0xb8,
              "W8LevelDataRecord_contact_normal_scale_b8");
static_assert(sizeof(W8LevelDataRecord) == 0xf4, "W8LevelDataRecord_must_be_0xf4");

extern W8LevelDataRecord* g_level_data_00652dac;
/* Companion pointer cleared alongside g_level_data_00652dac on level
   transitions; its target's +0 flags have 0x200 masked off at 0x0044FCD0. */
extern unsigned int* g_level_flags_00652da8;
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
unsigned int GetLevelDataFlag4(void); /* 0x0041F070 */
unsigned int GetLevelDataFlag8(void); /* 0x0041EFB0 */
void ClearLevelDataFlag8(void);       /* 0x0041EFD0 */
void SetLevelDataFlag8(void);         /* 0x0041EFE0 */
unsigned int GetLevelDataFlag9(void); /* 0x0041EFF0 */
bool HasLevelDataVector(void);        /* 0x0041F010 */
void ResetCurrentEnvironment0041AA40(void);
unsigned char SetEnvironmentLoadFlag(unsigned char flag); /* 0x0041AAE0 */
void BeginCameraSway0041A960(void);
void EndCameraSway0041A9A0(void);

unsigned int GetLevelDataFlag6(void);
unsigned char ConsumeLevelElapsedTime0041F170(float* real_elapsed, float* frame_elapsed);
/* Retail tests level flag 0x008; when set both outputs are -1. */
void GetLevelSoundEnvironment0041FCE0(char* environment, char* secondary);

/* 0x00420BD0: settle a world point onto the octree ground through the
   GameData geometry index; the false branch reports the input height and
   clears the caller's hit byte. */
float SettlePositionToGround00420BD0(const srVector3T<float>* position, unsigned char* hit);
/* 0x00420C30: same ground-settle query with a fixed 500-unit probe range,
   returning the resulting height. */
float SettlePositionToGround00420C30(srVector3T<float>* position, unsigned char* hit);
/* Reports the ground height, surface and material at one position through the
   level's game data: settles a probe with SettleToGround and reads the hit
   W8GDSurface's footstep_surface_3c/footstep_material_3d. */
float GetGroundSurfaceInfo(const srVector3T<float>* position, char* surface,
                           char* material); /* 0x00420CA0 */

void ClearLevelDataFlags5To7(void); /* 0x0041F0C0 */
srCamera* CreateOrSetGameCamera(srNode* parent, srCamera* camera);
float GetCameraYawInDegrees();
float GetCameraYawRadians();
float GetCameraPitchInDegrees();
float GetCameraPitchRadians();
void GetCameraOrientation(W8CameraAngleRecord angle, W8CameraAngleRecord pitch);
void BeginManualCameraControl();
void LevelCamera();
void CameraLookAt(const srVector3T<float>* position);     /* 0x00420F90 */
void CameraSnapToTarget(const srVector3T<float>* target); /* 0x00420FB0 */
void TurnCameraToDegrees(float degrees);
void SetCameraYawDegrees(float degrees);
void ApplyCameraRotation(srMatrix3T<float>* rotation);
void SetCameraOrientation(W8CameraAngleRecord angle, W8CameraAngleRecord pitch,
                          srMatrix3T<float>* rotation);
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
void RestoreWorldCameraOrientation00421570(W8CameraAngleRecord angle, W8CameraAngleRecord pitch,
                                           W8World* world);
void GetCameraPosition(srVector3T<float>* position);
int GetCameraYawDegrees(void);
/* 0x004215E0: point-visibility query through the world octree; false with no
   world, true for a loaded world without an octree. */
bool HasCameraLineOfSight(const srVector3T<float>* position);
void PlacePartyAtPoint(const srVector3T<float>* point);
/* 0x00420E20: start/stop the sustained movement footstep loop. */
void UpdateLevelMovementAudio00420E20(void);
/* 0x004EF9A0: fall-impact override handler owned by GameplayCode.cpp;
   declared in wiz8/local_code/GameplayCode.h. */

extern unsigned char g_level_motion_fast_00652dcd;
extern bool g_level_footstep_pending_00652db9;
extern float g_camera_motion_clamp_00603ac0;
extern float g_camera_motion_divisor_00603ac4;
extern int g_level_footstep_sound_00603ad4;
extern float g_level_footstep_time_00652dd0;
extern const double g_motion_delta_epsilon_005ebc50;
extern const double g_motion_vector_epsilon_005ebc48;
extern const float g_footstep_fall_threshold_005ebcd4;
extern unsigned char g_environment_motion_active_00603ad1;
extern unsigned char g_environ_ground_latch_00652db8;
extern srVector3T<float> g_origin_652940;
