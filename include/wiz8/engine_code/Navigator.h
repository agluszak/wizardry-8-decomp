#pragma once

struct W8MonsterInfo;

/* Engine Code\Navigator.cpp owns these declarations. */

#include "surrender/srMath.h"
#include "wiz8/geometry.h"
#include "wiz8/vector.h"

#include <stddef.h>
#include <stdlib.h>

class srNode;
struct W8PathAI;

struct W8NavigatorAttachment {
    unsigned int flags_00;
    /* Live waypoint cursor. AdvanceAlongPathPositions compacts consumed
       waypoints and returns this cursor to one. */
    unsigned short path_cursor_04;
    unsigned short unknown_06;
    unsigned short path_position_index_08;
    /* 0x00456210 sets this to ten and allocates position_4c as ten
       srVector3T<float>, so it is that array's capacity. */
    unsigned short capacity_0a;
    unsigned short value_0c;
    unsigned short unknown_0e;
    srVector3T<float> position_10;
    srVector3T<float> position_1c;
    srVector3T<float> position_28;
    srVector3T<float> position_34;
    srVector3T<float> position_40;
    /* The vector type's new[]/delete[] overloads route this allocation to
       srHeap; CopyPathFrom uses those operators while growth calls the heap
       directly. */
    srVector3T<float>* position_4c;
    /* 0x00457530 releases this one with free while +0x4c goes back to srHeap,
       so the two allocations do not share an owner. */
    unsigned short* path_values_50;
    float separation_54;
    /* Direct segment length after initialization; MeasurePathLength later
       replaces it with the eligible route length and latches flag 0x00400000. */
    float path_length_058;
    unsigned char unknown_05c[4];

    W8NavigatorAttachment(); /* 0x00456210 */
    /* The from/to form 0x004604B0 constructs on the stack: both endpoints of
       the segment are seeded as recorded positions and path_length_058 holds the
       straight-line distance. */
    W8NavigatorAttachment(const srVector3T<float>* from,
                          const srVector3T<float>* to); /* 0x00456280 */
    /* Header-visible: 0x004604B0's scope exit inlines this pair of releases,
       while its unwind funclet tail-calls the out-of-line emission the
       linker kept at 0x004563A0. */
    // FUNCTION: WIZ8 0x004563A0
    ~W8NavigatorAttachment()
    {
        srVector3T<float>* positions = position_4c;
        if (positions != 0) {
            position_4c = 0;
            srHeap.free(positions);
        }
        unsigned short* values = path_values_50;
        if (values != 0) {
            path_values_50 = 0;
            free(values);
        }
    }

    /* Lazily sums the stored segment lengths into path_length_058, skipping
       entries whose preceding path value carries bit 0x2. */
    float MeasurePathLength00456B00(); /* 0x00456B00 */

    void RecordPosition(const srVector3T<float>* position);
    void GrowPathStorage00456BD0();
    void CopyPathFrom004564F0(const W8NavigatorAttachment* other);
    void GetNextPosition00456660(srVector3T<float>* position);
    void InitializeSegment004563E0(const srVector3T<float>* source,
                                   const srVector3T<float>* destination);
    /* Step `position` forward along the recorded route by the 2-D `distance`,
       consuming waypoints the step covers; returns zero once the route's last
       waypoint is reached. */
    unsigned char AdvanceAlongPathPositions00456830(float distance,
                                                    srVector3T<float>* position); /* 0x00456830 */
    /* Whether `position`'s plan-view distance to the hop leaving the current
       index stays under the path height interpolated along that segment. */
    unsigned char
    CheckPositionHopHeight00456CB0(const srVector3T<float>* position); /* 0x00456CB0 */
    /* The two-segment form of the hop-height check used on predicted
       positions: the nearer of the current or following segment wins. */
    unsigned char
    CheckPredictedHopHeight00456DD0(const srVector3T<float>* position); /* 0x00456DD0 */
    /* Move `position` toward the route's next waypoint by up to `distance`,
       spilling into the following segment; returns nonzero once `distance`
       exceeded the remainder of the live segment. */
    unsigned char AdvancePositionTowardWaypoint00456F60(srVector3T<float>* position,
                                                        float distance); /* 0x00456F60 */
    /* Trims the recorded route to end at the sphere of `radius` around
       `target`: walks stored positions while they stay inside, interpolates
       the boundary point into position_1c and the route slot, moves the end
       index there, and clears flag 0x400000. One when a boundary point was
       installed. */
    unsigned char TruncatePathAtRadius004566C0(const srVector3T<float>* target,
                                               float radius); /* 0x004566C0 */
    /* Advances `position` along the recorded route by `distance`, writing the
       unit direction toward the current waypoint into `direction`; one once
       the final waypoint is reached. */
    unsigned char
    AdvancePositionWithDirection00457150(srVector3T<float>* position, float distance,
                                         srVector3T<float>* direction); /* 0x00457150 */
};

class W8Navigator;

/* The polymorphic object the navigator owns at +0xa0. 0x00452120 deletes it
   through its own virtual slot. Recovered writes only store null
   (Navigator constructors/destructor and Monster.cpp); no allocation site or
   constructor target is known, so the identity stays unestablished. The name
   is positional and claims nothing. */
class W8NavigatorOwned0A0 {
public:
    virtual ~W8NavigatorOwned0A0();
};

/* Navigator.cpp constructs the 0xCC-byte movement/collision tail at +0xC0
   independently. World collision routines receive this subobject, while the
   surrounding Navigator owns the path and group-following state. */
struct W8NavigatorMovementState {
    unsigned int unknown_000;
    unsigned short location_id_004;
    unsigned short unknown_006;
    int value_008;
    int value_00c;
    /* -1 means no resolved target. Navigation and OctPath use this as the
       location id of the tracked target; one OctPath path overlays the slot
       as a single candidate index while building a path. */
    int target_location_id_010;
    float yaw;
    float target_yaw;
    /* UpdateYawSteering accelerates/decelerates this signed angular rate. */
    float yaw_velocity_01c;
    float pitch_020;
    float target_pitch_024;
    float roll_028;
    float target_roll_02c;
    float unknown_030;
    srVector3T<float> velocity_034;
    srVector3T<float> position_040;
    srVector3T<float> target_position_04c;
    float callback_threshold_058;
    float callback_progress_05c;
    float movement_scale_060;
    float movement_speed_064;
    float turn_rate_068;
    unsigned short flag_06c;
    unsigned char unknown_06e[6];
    char pitch_enabled_074;
    char roll_enabled_075;
    unsigned char unknown_076[2];
    float vertical_velocity_078;
    float vertical_base_07c;
    float vertical_amplitude_080;
    float vertical_phase_084;
    /* Reset installs the identity basis here. The constructor starts all
       three at zero before the owner calls Reset. */
    srVector3T<float> vector_088;
    srVector3T<float> vector_094;
    srVector3T<float> vector_0a0;
    W8NavigatorAttachment* attachment_0ac;
    /* Collision/path radius, initialized to 500 by the outer navigator and
       scaled with the navigator in SetScale. */
    float collision_radius_0b0;
    float alternate_radius_0b4;
    float height_offset_0b8;
    float secondary_height_offset_0bc;
    float vertical_offset_0c0;
    float scale_0c4;
    bool position_adjusted_0c8;
    unsigned char unknown_0c9[3];

    W8NavigatorMovementState(); /* 0x004572C0 */
    /* A second, different set of defaults over the same subobject, run by
       W8Navigator's constructor immediately after this one. */
    void Reset();                /* 0x004573D0 */
    ~W8NavigatorMovementState(); /* 0x00457530 */

    /* Copies the eleven fields a navigator carries across from another's
       movement tail and invalidates target_location_id_010. It returns nothing, so it is a
       named member rather than an assignment operator. */
    void CopySettingsFrom(const W8NavigatorMovementState& other);
};

/* 0x004572C0 allocates one with operator new(0x60) before running its
   constructor at 0x00456210, which is what fixes the size; the destructor
   at 0x00457530 only proves it reaches +0x50. */
static_assert(sizeof(W8NavigatorAttachment) == 0x60, "W8NavigatorAttachment_size_must_be_0x60");
static_assert(offsetof(W8NavigatorAttachment, path_cursor_04) == 0x04,
              "W8NavigatorAttachment_value_04");
static_assert(offsetof(W8NavigatorAttachment, unknown_06) == 0x06,
              "W8NavigatorAttachment_unknown_06");
static_assert(offsetof(W8NavigatorAttachment, path_position_index_08) == 0x08,
              "W8NavigatorAttachment_path_position_index_08");
static_assert(offsetof(W8NavigatorAttachment, capacity_0a) == 0x0a,
              "W8NavigatorAttachment_capacity_0a");
static_assert(offsetof(W8NavigatorAttachment, position_10) == 0x10,
              "W8NavigatorAttachment_position_10");
static_assert(offsetof(W8NavigatorAttachment, position_1c) == 0x1c,
              "W8NavigatorAttachment_position_1c");
static_assert(offsetof(W8NavigatorAttachment, position_28) == 0x28,
              "W8NavigatorAttachment_position_28");
static_assert(offsetof(W8NavigatorAttachment, position_34) == 0x34,
              "W8NavigatorAttachment_position_34");
static_assert(offsetof(W8NavigatorAttachment, position_40) == 0x40,
              "W8NavigatorAttachment_position_40");
static_assert(offsetof(W8NavigatorAttachment, position_4c) == 0x4c,
              "W8NavigatorAttachment_position_4c");
static_assert(offsetof(W8NavigatorAttachment, path_values_50) == 0x50,
              "W8NavigatorAttachment_path_values_50");
static_assert(offsetof(W8NavigatorAttachment, separation_54) == 0x54,
              "W8NavigatorAttachment_separation_54");
static_assert(offsetof(W8NavigatorAttachment, path_length_058) == 0x58,
              "W8NavigatorAttachment_value_058");
static_assert(sizeof(W8NavigatorMovementState) == 0xcc,
              "W8NavigatorMovementState_size_must_be_0xcc");
static_assert(offsetof(W8NavigatorMovementState, location_id_004) == 0x04,
              "W8NavigatorMovementState_location_id_004");
static_assert(offsetof(W8NavigatorMovementState, yaw) == 0x14, "W8NavigatorMovementState_yaw");
static_assert(offsetof(W8NavigatorMovementState, target_yaw) == 0x18,
              "W8NavigatorMovementState_target_yaw");
static_assert(offsetof(W8NavigatorMovementState, pitch_020) == 0x20,
              "W8NavigatorMovementState_pitch_020");
static_assert(offsetof(W8NavigatorMovementState, target_pitch_024) == 0x24,
              "W8NavigatorMovementState_target_pitch_024");
static_assert(offsetof(W8NavigatorMovementState, roll_028) == 0x28,
              "W8NavigatorMovementState_roll_028");
static_assert(offsetof(W8NavigatorMovementState, target_roll_02c) == 0x2c,
              "W8NavigatorMovementState_target_roll_02c");
static_assert(offsetof(W8NavigatorMovementState, velocity_034) == 0x34,
              "W8NavigatorMovementState_velocity_034");
static_assert(offsetof(W8NavigatorMovementState, position_040) == 0x40,
              "W8NavigatorMovementState_position_040");
static_assert(offsetof(W8NavigatorMovementState, target_position_04c) == 0x4c,
              "W8NavigatorMovementState_target_position_04c");
static_assert(offsetof(W8NavigatorMovementState, callback_threshold_058) == 0x58,
              "W8NavigatorMovementState_callback_threshold_058");
static_assert(offsetof(W8NavigatorMovementState, callback_progress_05c) == 0x5c,
              "W8NavigatorMovementState_callback_progress_05c");
static_assert(offsetof(W8NavigatorMovementState, movement_scale_060) == 0x60,
              "W8NavigatorMovementState_movement_scale_060");
static_assert(offsetof(W8NavigatorMovementState, movement_speed_064) == 0x64,
              "W8NavigatorMovementState_movement_speed_064");
static_assert(offsetof(W8NavigatorMovementState, turn_rate_068) == 0x68,
              "W8NavigatorMovementState_turn_rate_068");
static_assert(offsetof(W8NavigatorMovementState, flag_06c) == 0x6c,
              "W8NavigatorMovementState_flag_06c");
static_assert(offsetof(W8NavigatorMovementState, pitch_enabled_074) == 0x74,
              "W8NavigatorMovementState_pitch_enabled_074");
static_assert(offsetof(W8NavigatorMovementState, roll_enabled_075) == 0x75,
              "W8NavigatorMovementState_roll_enabled_075");
static_assert(offsetof(W8NavigatorMovementState, vertical_velocity_078) == 0x78,
              "W8NavigatorMovementState_vertical_velocity_078");
static_assert(offsetof(W8NavigatorMovementState, vertical_base_07c) == 0x7c,
              "W8NavigatorMovementState_vertical_base_07c");
static_assert(offsetof(W8NavigatorMovementState, vertical_amplitude_080) == 0x80,
              "W8NavigatorMovementState_vertical_amplitude_080");
static_assert(offsetof(W8NavigatorMovementState, vertical_phase_084) == 0x84,
              "W8NavigatorMovementState_vertical_phase_084");
static_assert(offsetof(W8NavigatorMovementState, vector_088) == 0x88,
              "W8NavigatorMovementState_vector_088");
static_assert(offsetof(W8NavigatorMovementState, vector_094) == 0x94,
              "W8NavigatorMovementState_vector_094");
static_assert(offsetof(W8NavigatorMovementState, vector_0a0) == 0xa0,
              "W8NavigatorMovementState_vector_0a0");
static_assert(offsetof(W8NavigatorMovementState, attachment_0ac) == 0xac,
              "W8NavigatorMovementState_attachment_0ac");
static_assert(offsetof(W8NavigatorMovementState, collision_radius_0b0) == 0xb0,
              "W8NavigatorMovementState_value_0b0");
static_assert(offsetof(W8NavigatorMovementState, alternate_radius_0b4) == 0xb4,
              "W8NavigatorMovementState_alternate_radius_0b4");
static_assert(offsetof(W8NavigatorMovementState, height_offset_0b8) == 0xb8,
              "W8NavigatorMovementState_height_offset_0b8");
static_assert(offsetof(W8NavigatorMovementState, secondary_height_offset_0bc) == 0xbc,
              "W8NavigatorMovementState_secondary_height_offset_0bc");
static_assert(offsetof(W8NavigatorMovementState, vertical_offset_0c0) == 0xc0,
              "W8NavigatorMovementState_vertical_offset_0c0");
static_assert(offsetof(W8NavigatorMovementState, scale_0c4) == 0xc4,
              "W8NavigatorMovementState_value_0c4");
static_assert(offsetof(W8NavigatorMovementState, position_adjusted_0c8) == 0xc8,
              "W8NavigatorMovementState_position_adjusted_0c8");

/* The wait state Monster.cpp's cycle 0x17 parks in a Navigator's
   movement_target_018 while the monster idles between cycles: the tick the
   wait began, its millisecond duration, and the arming cycle id. The
   Navigator's vector view is the declared type; this is the named overlay
   for the reinterpreted dwords. */
struct W8MonsterCycleDelayState {
    unsigned int started_at;
    unsigned int duration;
    unsigned int cycle;
};

/* Navigator.cpp owns the path, position, orientation, and scene-node state
   below. It is GrCycle's ordinary second base, not a representation object. */
#pragma pack(push, 4)
class W8Navigator {
public:
    W8Navigator();                         /* 0x00451EC0 */
    W8Navigator(const W8Navigator& other); /* 0x00452220 */
    virtual ~W8Navigator();                /* 0x00452120 */
    virtual void SetPathAI(W8PathAI* path_ai);
    virtual W8PathAI* GetPathAI();
    void ResetPathAI();
    void SetScale(float scale);
    /* Called with what this navigator ran into: the startup world navigator
       or another mover. True once the collision was consumed. Same folded
       body as W8GrCycle::CanEnterCycle at 0x004A7140; /OPT:NOICF emits this
       copy. */
    virtual bool OnCollision(W8Navigator*)
    {
        return true;
    }
    virtual void SetPosition(const srVector3T<float>* position); /* 0x00456020 */

    /* Copy movement_0c0.velocity_034 out - the missile homing step scales this
       by its step count to predict the next position. */
    void GetVelocity(srVector3T<float>* velocity); /* 0x004534F0 */
    /* movement_0c0.velocity_034 = *velocity - the launch direction step. */
    void SetVelocity00453520(const srVector3T<float>* velocity); /* 0x00453520 */
    /* movement_0c0.target_yaw = NormalizeAngle(angle). */
    void SetTargetYaw(float angle); /* 0x004538D0 */
    /* movement_0c0.target_pitch_024 = NormalizeAngle(angle). */
    void SetTargetPitch(float angle); /* 0x00453920 */
    /* The navigator a from->to trace runs into: the startup world, a monster's
       navigator, or null. `include_target` lets the trace report the tracked
       movement target instead of skipping its location id. */
    W8Navigator* ResolveBlockingNavigator00453230(const srVector3T<float>* from,
                                                  srVector3T<float>* to,
                                                  unsigned char include_target); /* 0x00453230 */
    /* Copies this navigator's movement state into a scratch probe, asks the
       octree to route toward `target`, and returns the measured path length or
       -1 when no route inside `max_range` exists. */
    double MeasurePathDistance00453300(const srVector3T<float>* target, float max_range,
                                       int location_id); /* 0x00453300 */
    /* Find the navigator occupying `to`; the move collides when both sides'
       OnCollision accept it. */
    unsigned char CheckNavigatorCollision00453540(const srVector3T<float>* from,
                                                  const srVector3T<float>* to);

    void configureStartupRange(float range);
    void configureStartupDepth(float near_depth, float far_depth);

    srVector3T<float> GetPosition();
    unsigned char UpdateTrackedPosition00454950();            /* 0x00454950 */
    void UpdateNavigation004553A0(int value, char condition); /* 0x004553A0 */
    void SetAngles004538F0(float angle);                      /* 0x004538F0 */
    void SetPitch(float pitch);                               /* 0x00453940 */
    float GetYaw();                                           /* 0x00453970 */
    float GetPitch();                                         /* 0x00453980 */
    /* The world-path reachability probe the group engagement check runs:
       fills `out_distance` with the route length and returns nonzero when a
       route inside `max_range` exists. */
    int FindNavigatorPathDistance(float max_range, float* out_distance); /* 0x00453480 */
    void SetValue120(float value);                                       /* 0x00453C50 */
    float GetValue120();                                                 /* 0x00453C60 */
    void SetMonsterTurnSpeed(float speed);                               /* 0x00453C70 */
    unsigned char
    ConfigureMovementToPosition00452630(const srVector3T<float>* position); /* 0x00452630 */
    /* Point the movement target at another navigator's position and enter the
       moving mode; the result is nonzero once the target was accepted. */
    unsigned short SetMovementTargetToNavigator004526C0(W8Navigator* target,
                                                        double separation); /* 0x004526C0 */
    void LinkGroupNavigator00452BD0(W8Navigator* target, double separation, int value);
    /* Whether `other` belongs to this navigator's link group: either side may
       nominate the shared linked navigator directly. */
    bool IsLinkedToNavigator00452E10(W8Navigator* other); /* 0x00452E10 */
    /* Stop this navigator, clear its movement/target state, and either mark
       the linked movement stopped or re-sync the collected group. */
    void ResetMovementAndGroupState00452C90();               /* 0x00452C90 */
    void SetPitchRollEnabled00453CA0(char pitch, char roll); /* 0x00453CA0 */
    unsigned short ConfigureMovementToNavigator004529A0(
        W8Navigator* target, float separation, float maximum_distance, srVector3T<float> position,
        int trace_mode, float facing, unsigned char* probe_result); /* 0x004529A0 */
    void AddPathPoint(const srVector3T<float>* position);           /* 0x00453690 */
    void SetPositionInternal00453590(const srVector3T<float>* position);
    void SetObject68Flag38(char value);                                            /* 0x004537C0 */
    unsigned char LinkToNavigator004527A0(W8Navigator* target, double separation); /* 0x004527A0 */
    void SetFacingToward(const srVector3T<float>* position);                       /* 0x00454040 */
    void AimAtPosition(const srVector3T<float>* position);                         /* 0x00453F30 */
    bool StartPatrol(const srVector3T<float>* home, float distance,
                     float variation); /* 0x00453CC0 */
    /* Stores each non-negative bound as the minimum and maximum height. */
    void SetHeightRange(float minimum, float maximum); /* 0x00453EF0 */
    void SetFlag25(char value);                        /* 0x004531F0 */
    void SetMovementStopped00453880();                 /* 0x00453880 */
    /* Save the presence-gated movement state LoadMovementState00454AD0
       consumes: the flag byte, then for an ungrouped navigator with flag
       0x20000000 set the height bounds, position and movement target. */
    unsigned char LoadMovementState00454AD0(unsigned int hFile);           /* 0x00454AD0 */
    unsigned char SaveMovementState004549D0(unsigned int hFile);           /* 0x004549D0 */
    void PropagateGroupPosition();                                         /* 0x00454C80 */
    void UpdateAngles00453990();                                           /* 0x00453990 */
    unsigned char ConfigureMovement00453D20(float minimum, float maximum); /* 0x00453D20 */
    unsigned char SetMovementTarget(const srVector3T<float>* target,
                                    char propagate); /* 0x00454170 */
    srVector3T<float>* AdjustPosition00454440(srVector3T<float>* result,
                                              const srVector3T<float>* current,
                                              const srVector3T<float>* previous); /* 0x00454440 */
    void UpdateFacing(char immediate);                                            /* 0x00454780 */
    void UpdateLinkedNavigator();                                                 /* 0x00454D70 */
    unsigned char UpdateLinkedPosition00454FE0();
    void CollectGroupNavigators(W8GrowableVector<W8Navigator*>* navigators); /* 0x00455140 */
    int ResolveMovement();                                                   /* 0x00455CC0 */
    void ClearMovement();                                                    /* 0x004537E0 */
    /* Modes 1 and 4 clear pitch/roll; 2, 3 and 5 enable pitch; 6 enables
       both. Mode 4 keeps the existing path; the other named modes build a
       fresh animated path. Later facing logic distinguishes 2/3 from 5/6. */
    void SetNavigationMode(int mode); /* 0x00452E50 */
    void SetBounds(const srVector3T<float>* minimum,
                   const srVector3T<float>* maximum); /* 0x00452F10 */
    void SetTurnRate(float turn_rate);                /* 0x00453C90 */

public:
    /* Monster.cpp reaches this state as a secondary base through
       `lea ecx,[monster+0x18]`. It used to be unioned with an unsigned int[98]
       dword view, because the recovered constructor wrote it as a memset plus
       indexed stores. The retail constructor contains no memset and constructs
       movement_0c0 through its own constructor, so the dword view was modelling
       a body that does not exist - and while it existed it made this a union
       member, which C++98 forbids from having a constructor or destructor and
       which therefore blocked both of the movement tail's special members. */
    unsigned char unknown_004;
    unsigned char unknown_005[3];
    int navigation_mode_008;
    unsigned int flags_00c;
    double collision_margin_010;
    /* Used both as a movement target and, by Monster.cpp's cycle 0x17/0x18
       wait states, as a W8MonsterCycleDelayState overlay. It was an anonymous
       union until movement_0c0 gained a constructor, which VC6 will not
       generate for a struct holding one. The float view is the declared one;
       CycleDelay018() names the dword reinterpretation. */
    srVector3T<float> movement_target_018;

    /* The cycle-0x17 wait state view of movement_target_018: the tick the
       wait began, its millisecond duration and the arming cycle id. */
    W8MonsterCycleDelayState* CycleDelay018()
    {
        return reinterpret_cast<W8MonsterCycleDelayState*>( // reinterpret-ok: monster cycle
            &movement_target_018);                          /* states reuse the 12 target
            bytes as a timer record while no navigation target is live */
    }
    /* Set when the navigator's movement has stopped - the constructors raise
       it, SetMovementStopped00453880 raises it when motion halts (levelling
       pitch unless the navigation mode banks), and a successful
       PrepareLinkedNavigator00466FB0 clears it while a path is active. A
       monster scripts wait on it: CanContinueScript004CA0F0 blocks a WALKTO
       until it is set. */
    bool movement_stopped_024;
    bool flag_025;
    bool movement_complete_026;
    unsigned char unknown_027;
    srVector3T<float> position_028;
    float minimum_height_034;
    float maximum_height_038;
    srVector3T<float> position_03c;
    unsigned int unknown_048;
    W8Navigator* target_navigator_04c;
    srVector3T<float> target_last_position_050;
    W8Navigator* linked_navigator_05c;
    unsigned int unknown_060;
    unsigned int unknown_064;
    W8PathAI* path_ai_068;
    /* 0x00451EC0 fills these as -500 and +500 triples. */
    srVector3T<float> minimum_06c;
    srVector3T<float> maximum_078;
    float radius_084;
    unsigned char state_088;
    unsigned char unknown_089[3];
    void(__cdecl* movement_callback_08c)(W8Navigator* navigator);
    unsigned int unknown_090;
    unsigned int unknown_094;
    unsigned int unknown_098;
    /* 0x09c: byte flag - SetMonsterGroupNavigatorDirty stores its uchar
       parameter raw, with no bool normalization. */
    unsigned char position_dirty_09c;
    unsigned char unknown_09d[3];
    W8NavigatorOwned0A0* owned_object_0a0;
    srVector3T<float> tracked_position_0a4;
    float tracked_distance_0b0;
    bool tracked_dirty_0b4;
    unsigned char unknown_0b5[3];
    int linked_update_time_0b8;
    unsigned char unknown_0bc[4];
    /* Constructed first as its own 0xcc-byte subobject, then Reset by this
       owner. Copy construction constructs a fresh attachment and transfers
       selected settings; it never shares the source's path allocation. */
    W8NavigatorMovementState movement_0c0;
    srNode* node_18c; /* 0x18c: constructed srNode */
}; /* 0x190 */
#pragma pack(pop)

static_assert(sizeof(W8Navigator) == 0x190, "W8Navigator_size_must_be_0x190");

void SetNavigatorLinkMode00452F50(unsigned char mode);
void StopAllNavigators00453160(void);
void ResumeAllNavigators004531A0(void);

void NavigatorDefaultCallback00451EA0(W8Navigator* navigator);

extern float g_navigator_vertical_phase_step_005ebcc8;
extern float g_navigator_snap_angle_005ec2f0;
extern unsigned char g_flag_006081e4;
extern unsigned char g_navigator_link_mode_00659c10;
extern float g_navigator_linked_radius_scale_005ebc98;
extern W8GrowableVector<W8Navigator*> g_navigator_group_659bf8;
/* Runtime scale applied to the startup navigator's radius_084 when the trace
   resolver tests the camera sphere; written during startup, not a constant. */
extern float g_float_006081f4;
