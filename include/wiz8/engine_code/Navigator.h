#pragma once

/* Engine Code\Navigator.cpp owns these declarations, split out of the combined
   grcycle.h. */

#include "surrender/srMath.h"
#include "wiz8/geometry.h"
#include "wiz8/vector.h"

class srNode;
struct W8PathAI;

struct W8NavigatorAttachment {
    unsigned int flags_00;
    unsigned short value_04;
    unsigned short unknown_06;
    unsigned short value_08;
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
    srVector3T<float>* position_4c;
    /* 0x00457530 releases this one with free while +0x4c goes back to srHeap,
       so the two allocations do not share an owner. */
    void* allocation_50;
    float separation_54;
    unsigned int value_058;
    unsigned char unknown_05c[4];

    W8NavigatorAttachment();             /* 0x00456210 */

    void RecordPosition(const srVector3T<float>* position);
    void InitializeSegment004563E0(
        const srVector3T<float>* source,
        const srVector3T<float>* destination);
};

class W8Navigator;

/* The polymorphic object the navigator owns at +0xa0. 0x00452120 deletes it
   through its own virtual slot and nothing in Navigator.cpp ever assigns one,
   so its identity is not established - only that the navigator owns it and that
   it has a virtual destructor. The name is positional and claims nothing. */
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
    int value_010;
    float yaw;
    float target_yaw;
    float unknown_01c;
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
    unsigned char pitch_enabled_074;
    unsigned char roll_enabled_075;
    unsigned char unknown_076[2];
    float vertical_velocity_078;
    float vertical_base_07c;
    float vertical_amplitude_080;
    float vertical_phase_084;
    srVector3T<float> vector_088;
    srVector3T<float> vector_094;
    srVector3T<float> vector_0a0;
    W8NavigatorAttachment* attachment_0ac;
    /* 0x00451EC0 writes 500.0f here as a dword, alongside the three radii
       that follow. */
    float value_0b0;
    float alternate_radius_0b4;
    float height_offset_0b8;
    float secondary_height_offset_0bc;
    float vertical_offset_0c0;
    float value_0c4;
    unsigned char position_adjusted_0c8;
    unsigned char unknown_0c9[3];

    W8NavigatorMovementState();       /* 0x004572C0 */
    /* A second, different set of defaults over the same subobject, run by
       W8Navigator's constructor immediately after this one. */
    void Reset();        /* 0x004573D0 */
    ~W8NavigatorMovementState();      /* 0x00457530 */

    /* Copies the eleven fields a navigator carries across from another's
       movement tail and invalidates value_010. It returns nothing, so it is a
       named member rather than an assignment operator. */
    void CopySettingsFrom(const W8NavigatorMovementState& other);

};

/* 0x004572C0 allocates one with operator new(0x60) before running its
   constructor at 0x00456210, which is what fixes the size; the destructor
   at 0x00457530 only proves it reaches +0x50. */
static_assert(sizeof(W8NavigatorAttachment) == 0x60,
              "W8NavigatorAttachment_size_must_be_0x60");
static_assert(sizeof(W8NavigatorMovementState) == 0xcc,
              "W8NavigatorMovementState_size_must_be_0xcc");

/* Navigator.cpp owns the path, position, orientation, and scene-node state
   below. It is GrCycle's ordinary second base, not a representation object. */
#pragma pack(push, 4)
class W8Navigator {
public:
    W8Navigator();                        /* 0x00451EC0 */
    W8Navigator(const W8Navigator& other); /* 0x00452220 */
    virtual ~W8Navigator();               /* 0x00452120 */
    virtual void SetPathAI(W8PathAI* path_ai);
    virtual W8PathAI* GetPathAI();
    void ResetPathAI();
    virtual unsigned char Function4A7140(int) const { return 1; }
    virtual void SetPosition(const srVector3T<float>* position); /* 0x00456020 */

    void configureStartupRange(float range);
    void configureStartupDepth(float near_depth, float far_depth);

    srVector3T<float> GetPosition();
    unsigned char UpdateTrackedPosition00454950();       /* 0x00454950 */
    void UpdateNavigation004553A0(int value, char condition); /* 0x004553A0 */
    void SetAngles004538F0(float angle);                    /* 0x004538F0 */
    void SetPitch(float pitch);                     /* 0x00453940 */
    float GetYaw();                           /* 0x00453970 */
    float GetPitch();                           /* 0x00453980 */
    void SetValue120(float value);                         /* 0x00453C50 */
    float GetValue120();                                  /* 0x00453C60 */
    unsigned char Function452630(const srVector3T<float>* position); /* 0x00452630 */
    unsigned short Function4526C0(
        W8Navigator* target, double separation); /* 0x004526C0 */
    unsigned short ConfigureMovementToNavigator004529A0(
        W8Navigator* target,
        int value_1,
        int value_2,
        srVector3T<float> position,
        int value_3,
        float facing,
        int value_4);                                  /* 0x004529A0 */
    void Function453690(void* argument);                   /* 0x00453690 */
    void SetPositionInternal00453590(const srVector3T<float>* position);
    void SetObject68Flag38(char value);                    /* 0x004537C0 */
    unsigned short LinkToNavigator004527A0(
        W8Navigator* target, double separation);           /* 0x004527A0 */
    void Function454040(const srVector3T<float>* position);       /* 0x00454040 */
    void AimAtPosition(const srVector3T<float>* position); /* 0x00453F30 */
    void StartPatrol(
        const srVector3T<float>* home, float distance, float variation);
    void SetFlag25(char value);                            /* 0x004531F0 */
    void SetMovementStopped00453880();                     /* 0x00453880 */
    void UpdateAngles00453990();                           /* 0x00453990 */
    unsigned char ConfigureMovement00453D20(
        float minimum, float maximum);                     /* 0x00453D20 */
    unsigned char SetMovementTarget(
        const srVector3T<float>* target, char propagate);  /* 0x00454170 */
    srVector3T<float>* AdjustPosition00454440(
        srVector3T<float>* result,
        const srVector3T<float>* current,
        const srVector3T<float>* previous);                /* 0x00454440 */
    void UpdateFacing(char immediate);             /* 0x00454780 */
    void UpdateLinkedNavigator();                  /* 0x00454D70 */
    void CollectGroupNavigators(
        W8GrowableVector<W8Navigator*>* navigators);       /* 0x00455140 */
    int ResolveMovement();                         /* 0x00455CC0 */
    void ClearMovement();                          /* 0x004537E0 */
    void SetNavigationMode(int mode);              /* 0x00452E50 */
    void SetBounds(
        const srVector3T<float>* minimum,
        const srVector3T<float>* maximum);          /* 0x00452F10 */
    void SetTurnRate(float turn_rate);              /* 0x00453C90 */

    /* Monster.cpp's 0x004C3F00 reads a byte where maximum_078.z sits. */
    signed char animationIndex() const
    {
        return *reinterpret_cast<const signed char*>(&fields.maximum_078.z);
    }

public:
    /* Monster.cpp reaches this payload as a secondary base through
       `lea ecx,[monster+0x18]`. It used to be unioned with an unsigned int[98]
       dword view, because the recovered constructor wrote it as a memset plus
       indexed stores. The retail constructor contains no memset and constructs
       movement_0c0 through its own constructor, so the dword view was modelling
       a body that does not exist - and while it existed it made this a union
       member, which C++98 forbids from having a constructor or destructor and
       which therefore blocked both of the movement tail's special members. */
    struct Fields {
            unsigned char unknown_004;
            unsigned char unknown_005[3];
            int navigation_mode_008;
            unsigned int flags_00c;
            double collision_margin_010;
            /* Used both as a movement target and, by Monster.cpp, as three
               loose dwords. It was an anonymous union until movement_0c0 gained
               a constructor, which VC6 will not generate for a struct holding
               one. The float view is the declared one; the dword uses spell out
               their reinterpretation. */
            srVector3T<float> movement_target_018;
            unsigned char flag_024;
            unsigned char flag_025;
            unsigned char movement_complete_026;
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
            /* 0x00451EC0 fills these as -500 and +500 triples, which is what
               makes them a pair rather than five loose floats. Note that
               maximum_078.z overlaps the byte Monster.cpp reads at +0x80 as an
               animation index; animationIndex() spells that access out rather
               than declaring a field the constructor contradicts. */
            srVector3T<float> minimum_06c;
            srVector3T<float> maximum_078;
            float radius_084;
            unsigned char state_088;
            unsigned char unknown_089[3];
            void (__cdecl *movement_callback_08c)(W8Navigator* navigator);
            unsigned int unknown_090;
            unsigned int unknown_094;
            unsigned int unknown_098;
            unsigned char position_dirty_09c;
            unsigned char unknown_09d[3];
            W8NavigatorOwned0A0* owned_object_0a0;
            srVector3T<float> tracked_position_0a4;
            float tracked_distance_0b0;
            unsigned char tracked_dirty_0b4;
            unsigned char unknown_0b5[3];
            int linked_update_time_0b8;
            unsigned char unknown_0bc[4];
            W8NavigatorMovementState movement_0c0;
    };

    Fields fields;
    srNode* node_18c;                    /* 0x18c: constructed srNode */
};                                      /* 0x190 */
#pragma pack(pop)
