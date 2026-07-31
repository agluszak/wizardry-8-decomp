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
    unsigned char unknown_0a[6];
    srVector3T<float> position_10;
    srVector3T<float> position_1c;
    srVector3T<float> position_28;
    srVector3T<float> position_34;
    srVector3T<float> position_40;
    srVector3T<float>* position_4c;

    void RecordPosition00456AE0(const srVector3T<float>* position);
};

class W8Navigator;

/* Navigator.cpp constructs the 0xCC-byte movement/collision tail at +0xC0
   independently. World collision routines receive this subobject, while the
   surrounding Navigator owns the path and group-following state. */
struct W8NavigatorMovement004572C0 {
    unsigned int unknown_000;
    unsigned short location_id_004;
    unsigned short unknown_006;
    int value_008;
    int value_00c;
    int value_010;
    float angle_014;
    float target_angle_018;
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
    unsigned char unknown_06c[8];
    unsigned char pitch_enabled_074;
    unsigned char roll_enabled_075;
    unsigned char unknown_076[2];
    float vertical_velocity_078;
    float vertical_base_07c;
    float vertical_amplitude_080;
    float vertical_phase_084;
    unsigned char unknown_088[0x24];
    W8NavigatorAttachment* attachment_0ac;
    unsigned char unknown_0b0[4];
    float alternate_radius_0b4;
    float height_offset_0b8;
    float secondary_height_offset_0bc;
    float vertical_offset_0c0;
    unsigned char unknown_0c4[4];
    unsigned char position_adjusted_0c8;
    unsigned char unknown_0c9[3];
};

static_assert(sizeof(W8NavigatorAttachment) == 0x50,
              "W8NavigatorAttachment_size_must_be_0x50");
static_assert(sizeof(W8NavigatorMovement004572C0) == 0xcc,
              "W8NavigatorMovement004572C0_size_must_be_0xcc");

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
    virtual void SetPosition(const W8Position* position); /* 0x00456020 */

    void configureStartupRange(float range);
    void configureStartupDepth(float near_depth, float far_depth);

    srVector3T<float> GetPosition();
    unsigned char UpdateTrackedPosition00454950();       /* 0x00454950 */
    void UpdateNavigation004553A0(int value, char condition); /* 0x004553A0 */
    void SetAngles004538F0(float angle);                    /* 0x004538F0 */
    void SetPitch00453940(float pitch);                     /* 0x00453940 */
    float GetAngleD400453970();                           /* 0x00453970 */
    float GetAngleE000453980();                           /* 0x00453980 */
    void SetValue120(float value);                         /* 0x00453C50 */
    float GetValue120();                                  /* 0x00453C60 */
    unsigned char Function452630(const W8Position* position); /* 0x00452630 */
    unsigned short Function4526C0(
        W8Navigator* target, double separation); /* 0x004526C0 */
    unsigned short ConfigureMovementToNavigator004529A0(
        W8Navigator* target,
        int value_1,
        int value_2,
        W8Position position,
        int value_3,
        float facing,
        int value_4);                                  /* 0x004529A0 */
    void Function453690(void* argument);                   /* 0x00453690 */
    void SetPositionInternal00453590(const W8Position* position);
    void SetObject68Flag38(char value);                    /* 0x004537C0 */
    unsigned short LinkToNavigator004527A0(
        W8Navigator* target, double separation);           /* 0x004527A0 */
    void Function454040(const W8Position* position);       /* 0x00454040 */
    void AimAtPosition00453F30(const W8Position* position); /* 0x00453F30 */
    void StartPatrol00453CC0(
        const W8Position* home, float distance, float variation);
    void SetFlag25(char value);                            /* 0x004531F0 */
    void SetMovementStopped00453880();                     /* 0x00453880 */
    void UpdateAngles00453990();                           /* 0x00453990 */
    unsigned char ConfigureMovement00453D20(
        float minimum, float maximum);                     /* 0x00453D20 */
    unsigned char SetMovementTarget00454170(
        const srVector3T<float>* target, char propagate);  /* 0x00454170 */
    srVector3T<float>* AdjustPosition00454440(
        srVector3T<float>* result,
        const srVector3T<float>* current,
        const srVector3T<float>* previous);                /* 0x00454440 */
    void UpdateFacing00454780(char immediate);             /* 0x00454780 */
    void UpdateLinkedNavigator00454D70();                  /* 0x00454D70 */
    void CollectGroupNavigators00455140(
        W8GrowableVector<W8Navigator*>* navigators);       /* 0x00455140 */
    int ResolveMovement00455CC0();                         /* 0x00455CC0 */
    void ClearMovement004537E0();                          /* 0x004537E0 */

public:
    /* The constructor clears this payload as 98 dwords, while Monster.cpp
       reaches the same secondary base through `lea ecx,[monster+0x18]`.
       The union preserves the constructor's observed dword view and exposes
       only independently witnessed fields. */
    union {
        unsigned int unknown_004[98];
        struct {
            unsigned char unknown_004;
            unsigned char unknown_005[3];
            int navigation_mode_008;
            unsigned int flags_00c;
            double collision_margin_010;
            union {
                srVector3T<float> movement_target_018;
                struct {
                    unsigned int value_018;
                    unsigned int value_01c;
                    unsigned int value_020;
                } values_018;
            };
            unsigned char flag_024;
            unsigned char flag_025;
            unsigned char movement_complete_026;
            unsigned char unknown_027;
            srVector3T<float> position_028;
            float minimum_height_034;
            float maximum_height_038;
            srVector3T<float> position_03c;
            unsigned char unknown_048[4];
            W8Navigator* target_navigator_04c;
            srVector3T<float> target_last_position_050;
            W8Navigator* linked_navigator_05c;
            unsigned char unknown_060_to_068[8];
            W8PathAI* path_ai_068;
            unsigned char unknown_06c_to_080[0x14];
            signed char animation_index_080;
            unsigned char unknown_081[3];
            float radius_084;
            unsigned char state_088;
            unsigned char unknown_089[3];
            void (__cdecl *movement_callback_08c)(W8Navigator* navigator);
            unsigned int unknown_090;
            unsigned int unknown_094;
            unsigned int unknown_098;
            unsigned char position_dirty_09c;
            unsigned char unknown_09d[3];
            void* owned_object_0a0;
            srVector3T<float> tracked_position_0a4;
            float tracked_distance_0b0;
            unsigned char tracked_dirty_0b4;
            unsigned char unknown_0b5[3];
            int linked_update_time_0b8;
            unsigned char unknown_0bc[4];
            W8NavigatorMovement004572C0 movement_0c0;
        } fields;
    };
    srNode* node_18c;                    /* 0x18c: constructed srNode */
};                                      /* 0x190 */
#pragma pack(pop)
