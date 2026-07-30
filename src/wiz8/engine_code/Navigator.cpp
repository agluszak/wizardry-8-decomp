#include "wiz8/grcycle.h"

#include "surrender/srNode.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_state_006598a4.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/utility.h"

#include <string.h>
#include <math.h>

namespace {

unsigned int float_bits(float value)
{
    union {
        float floating;
        unsigned int bits;
    } representation;
    representation.floating = value;
    return representation.bits;
}

W8Navigator** g_navigators_659b3c;
int g_navigator_count_659b34;
int g_navigator_capacity_659b38;

void RegisterNavigator(W8Navigator* navigator)
{
    W8Navigator** replacement;
    int index;
    int new_count = g_navigator_count_659b34 + 1;

    if (g_navigator_capacity_659b38 < new_count) {
        replacement = static_cast<W8Navigator**>(
            ::operator new(new_count * sizeof(*replacement)));
        if (replacement == 0) {
            return;
        }
        for (index = 0; index != g_navigator_count_659b34; ++index) {
            replacement[index] = g_navigators_659b3c[index];
        }
        ::operator delete(g_navigators_659b3c);
        g_navigators_659b3c = replacement;
        g_navigator_capacity_659b38 = new_count;
    }
    g_navigators_659b3c[g_navigator_count_659b34++] = navigator;
}

}

// FUNCTION: WIZ8 0x00456ae0
void W8NavigatorAttachment::RecordPosition00456AE0(
    const srVector3T<float>* position)
{
    flags_00 |= 0x2000000;
    position_40 = *position;
}

// FUNCTION: WIZ8 0x00451ec0
W8Navigator::W8Navigator()
{
    memset(unknown_004, 0, sizeof(unknown_004));
    unknown_004[0] = 0;
    unknown_004[3] = 0xffffffff;
    unknown_004[8] = 0x00010001;
    unknown_004[12] = 0x461c4000;
    unknown_004[13] = 0x469c4000;
    unknown_004[26] = 0xc3fa0000;
    unknown_004[27] = 0xc3fa0000;
    unknown_004[28] = 0xc3fa0000;
    unknown_004[29] = 0x43fa0000;
    unknown_004[30] = 0x43fa0000;
    unknown_004[31] = 0x43fa0000;
    unknown_004[32] = 0x43fa0000;
    unknown_004[33] = 1;
    unknown_004[43] = 0x43fa0000;
    unknown_004[91] = 0x43fa0000;
    unknown_004[92] = 0x43fa0000;
    unknown_004[93] = 0x43fa0000;
    unknown_004[94] = 0x43fa0000;
    unknown_004[95] = 0;
    unknown_004[96] = 0x3f800000;
    node_18c = new srNode(0);
    RegisterNavigator(this);
}

// FUNCTION: WIZ8 0x004534c0
srVector3T<float> W8Navigator::GetPosition()
{
    return fields.movement_0c0.position_040;
}

// FUNCTION: WIZ8 0x00454950
unsigned char W8Navigator::UpdateTrackedPosition00454950()
{
    float dx = fields.tracked_position_0a4.x - fields.movement_0c0.position_040.x;
    float dy = fields.tracked_position_0a4.y - fields.movement_0c0.position_040.y;
    float dz = fields.tracked_position_0a4.z - fields.movement_0c0.position_040.z;
    float distance = (float)sqrt(dx * dx + dy * dy + dz * dz);

    if (distance > fields.tracked_distance_0b0) {
        fields.tracked_position_0a4 = fields.movement_0c0.position_040;
        fields.tracked_dirty_0b4 = 1;
    }
    return fields.tracked_dirty_0b4;
}

// FUNCTION: WIZ8 0x00453880
void W8Navigator::SetMovementStopped00453880()
{
    if (fields.flag_024 == 0) {
        fields.flag_024 = 1;
        if (fields.navigation_mode_008 != 5 && fields.navigation_mode_008 != 6) {
            fields.movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
        }
    }
}

// FUNCTION: WIZ8 0x004538f0
void W8Navigator::SetAngles004538F0(float angle)
{
    fields.movement_0c0.angle_014 = NormalizeAngle(angle);
    fields.movement_0c0.target_angle_018 = NormalizeAngle(angle);
}

// FUNCTION: WIZ8 0x00453940
void W8Navigator::SetPitch00453940(float pitch)
{
    fields.movement_0c0.pitch_020 = NormalizeAngle(pitch);
    fields.movement_0c0.target_pitch_024 = NormalizeAngle(pitch);
}

// FUNCTION: WIZ8 0x00453970
float W8Navigator::GetAngleD400453970()
{
    return fields.movement_0c0.angle_014;
}

// FUNCTION: WIZ8 0x00453980
float W8Navigator::GetAngleE000453980()
{
    return fields.movement_0c0.pitch_020;
}

// FUNCTION: WIZ8 0x004538b0
void W8Navigator::SetPathAI(W8PathAI* path_ai)
{
    fields.path_ai_068 = path_ai;
}

// FUNCTION: WIZ8 0x004538c0
W8PathAI* W8Navigator::GetPathAI()
{
    return fields.path_ai_068;
}

// FUNCTION: WIZ8 0x00454930
void W8Navigator::ResetPathAI()
{
    if (fields.path_ai_068 != 0) {
        fields.path_ai_068->value_04 = 0.0f;
        fields.path_ai_068->value_24 = 0.0f;
        PathAIResetTick004A9C20(fields.path_ai_068);
    }
}

void W8Navigator::configureStartupRange(float range)
{
    unknown_004[32] = float_bits(range);
    unknown_004[35] = 1;
    unknown_004[91] = float_bits(range);
    unknown_004[92] = float_bits(range);
}

void W8Navigator::configureStartupDepth(float near_depth, float far_depth)
{
    unknown_004[93] = float_bits(near_depth);
    unknown_004[94] = float_bits(far_depth);
}

extern "C" {
extern W8Navigator* g_startup_world_659c0c;
extern unsigned char g_navigator_position_changed_659c11;
}

extern void Function454780(int changed);

extern unsigned char g_in_combat_00683f94;
extern float g_navigator_default_turn_rate_005ec2f4;
extern float g_frame_scale_006068ec;
extern W8Object0043A910* g_object_6598bc;
extern const float g_one_005ebb38;
extern const float g_negative_one_005ebc38;
extern float g_navigator_snap_angle_005ec2f0;
extern float g_navigator_mode3_scale_005ebca4;
extern float g_float_005ebb34;
extern unsigned char g_flag_006081e4;

// FUNCTION: WIZ8 0x004526c0
unsigned short W8Navigator::Function4526C0(
    W8Navigator* target, double separation)
{
    unsigned short result = 0;

    fields.movement_target_018.x = 0.0f;
    fields.movement_target_018.y = 0.0f;
    fields.movement_target_018.z = 0.0f;
    fields.collision_margin_010 = separation;
    fields.target_navigator_04c = target;
    if (target == g_startup_world_659c0c ||
        target->fields.movement_0c0.location_id_004 != 0) {
        fields.movement_0c0.value_010 =
            target->fields.movement_0c0.location_id_004;
    }
    else {
        fields.movement_0c0.value_010 = -1;
    }
    PathAIClearOwned004A9BB0(fields.path_ai_068);
    if (g_flag_006081e4 == 0) {
        fields.movement_0c0.attachment_0ac->flags_00 |= 0x10000;
    }
    if (SetMovementTarget00454170(
            &target->fields.movement_0c0.position_040, 0) == 0) {
        if (g_flag_006081e4 == 0) {
            fields.navigation_mode_008 = 0;
            fields.unknown_0bc[0] = 1;
        }
    }
    else {
        fields.navigation_mode_008 = 5;
        result = 1;
        if (g_flag_006081e4 == 0) {
            result = (unsigned short)(
                fields.movement_0c0.attachment_0ac->flags_00 & 7);
        }
    }
    fields.target_last_position_050 =
        target->fields.movement_0c0.position_040;
    return result;
}

// FUNCTION: WIZ8 0x00453cc0
void W8Navigator::StartPatrol00453CC0(
    const W8Position* home, float distance, float variation)
{
    W8Navigator* navigator = this;

    while (navigator->fields.linked_navigator_05c != 0) {
        navigator = navigator->fields.linked_navigator_05c;
    }
    navigator->fields.navigation_mode_008 = 0;
    navigator->fields.position_03c.x = home->x;
    navigator->fields.position_03c.y = home->y;
    navigator->fields.position_03c.z = home->z;
    navigator->fields.minimum_height_034 = distance;
    navigator->fields.maximum_height_038 = variation;
    navigator->ConfigureMovement00453D20(distance, variation);
}
extern unsigned char g_navigator_vertical_enabled_006081f8;
extern unsigned char g_navigator_link_mode_00659c10;
extern float g_navigator_speed_006850ff;
extern float g_navigator_condition_scale_005ebc7c;
extern float g_navigator_linked_radius_scale_005ebc98;
extern float g_navigator_vertical_phase_step_005ebcc8;
extern double g_navigator_cycle_angle_005ec318;
extern float g_navigator_target_refresh_distance_005ec030;
extern float g_navigator_startup_refresh_distance_005ec150;
extern float g_navigator_minimum_speed_006081ec;
extern float g_navigator_minimum_speed_mode23_006081f0;
extern const float g_world_scale_005ebc40;
extern W8GrowableVector<W8Navigator*> g_navigator_group_659bf8;
extern float Function4BE420(
    const W8Position* from, const W8Position* to);
extern float Function4BE490(
    const W8Position* from, const W8Position* to);

// FUNCTION: WIZ8 0x00453590
void W8Navigator::SetPositionInternal00453590(const W8Position* position)
{
    srVector3T<double> widened;

    if (position->x != fields.movement_0c0.position_040.x ||
        position->y != fields.movement_0c0.position_040.y ||
        position->z != fields.movement_0c0.position_040.z) {
        fields.movement_0c0.position_040.x = position->x;
        fields.movement_0c0.position_040.y = position->y;
        fields.movement_0c0.position_040.z = position->z;
        widened.x = position->x;
        widened.y = position->y;
        widened.z = position->z;
        node_18c->setLocation(widened);
        if (fields.movement_0c0.location_id_004 != 0 ||
            this == g_startup_world_659c0c) {
            g_navigator_position_changed_659c11 = 1;
        }
        Function454780(1);
        if (fields.movement_0c0.attachment_0ac != 0) {
            *fields.movement_0c0.attachment_0ac->position_4c =
                fields.movement_0c0.position_040;
            fields.movement_0c0.attachment_0ac->position_34 =
                *fields.movement_0c0.attachment_0ac->position_4c;
            fields.movement_0c0.attachment_0ac->position_10 =
                *fields.movement_0c0.attachment_0ac->position_4c;
        }
    }
}

// FUNCTION: WIZ8 0x004537e0
void W8Navigator::ClearMovement004537E0()
{
    if (fields.flag_024 == 0) {
        fields.flag_024 = 1;
        if (fields.navigation_mode_008 != 5 && fields.navigation_mode_008 != 6) {
            fields.movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
        }
    }

    PathAIClearOwned004A9BB0(fields.path_ai_068);
    fields.movement_0c0.velocity_034.method_00421670();
    if ((fields.movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
        fields.flags_00c &= 0xff000000;
    } else {
        fields.flags_00c = 0;
    }
    fields.movement_0c0.attachment_0ac->RecordPosition00456AE0(
        &fields.movement_0c0.position_040);
    g_engine_state_6598a4->QueueOctreeKind130042E810(
        fields.movement_0c0.location_id_004,
        &fields.movement_0c0.position_040);
    fields.movement_complete_026 = 1;
}

// FUNCTION: WIZ8 0x00453990
void W8Navigator::UpdateAngles00453990()
{
    float step = g_navigator_default_turn_rate_005ec2f4;
    float distance;
    float reverse_distance;
    float direction;

    if (g_in_combat_00683f94 == 0) {
        step = fields.movement_0c0.turn_rate_068;
    }
    step *= g_frame_scale_006068ec * g_object_6598bc->GetValue28();

    if (fields.movement_0c0.angle_014 != fields.movement_0c0.target_angle_018) {
        distance = NormalizeAngle(
            fields.movement_0c0.angle_014 - fields.movement_0c0.target_angle_018);
        reverse_distance = NormalizeAngle(
            fields.movement_0c0.target_angle_018 - fields.movement_0c0.angle_014);
        direction = g_negative_one_005ebc38;
        if (reverse_distance < distance) {
            distance = reverse_distance;
            direction = g_one_005ebb38;
        }
        if (step <= distance) {
            fields.movement_0c0.angle_014 = NormalizeAngle(
                step * direction + fields.movement_0c0.angle_014);
        } else {
            fields.movement_0c0.angle_014 = fields.movement_0c0.target_angle_018;
        }
        if (fields.flag_024 != 0) {
            UpdateFacing00454780(0);
        }
        if ((float)fabs(fields.movement_0c0.angle_014 -
                        fields.movement_0c0.target_angle_018) <
            g_navigator_snap_angle_005ec2f0) {
            fields.movement_0c0.angle_014 = fields.movement_0c0.target_angle_018;
        }
    }

    if (fields.navigation_mode_008 == 3) {
        step *= g_navigator_mode3_scale_005ebca4;
    }
    if (fields.movement_0c0.pitch_enabled_074 != 0 &&
        fields.movement_0c0.pitch_020 != fields.movement_0c0.target_pitch_024) {
        distance = NormalizeAngle(
            fields.movement_0c0.pitch_020 - fields.movement_0c0.target_pitch_024);
        reverse_distance = NormalizeAngle(
            fields.movement_0c0.target_pitch_024 - fields.movement_0c0.pitch_020);
        direction = g_negative_one_005ebc38;
        if (reverse_distance < distance) {
            distance = reverse_distance;
            direction = g_one_005ebb38;
        }
        if (step <= distance) {
            fields.movement_0c0.pitch_020 = NormalizeAngle(
                step * direction + fields.movement_0c0.pitch_020);
        } else {
            fields.movement_0c0.pitch_020 = fields.movement_0c0.target_pitch_024;
        }
    }

    if (fields.movement_0c0.roll_enabled_075 != 0 &&
        fields.movement_0c0.roll_028 != fields.movement_0c0.target_roll_02c) {
        distance = NormalizeAngle(
            fields.movement_0c0.roll_028 - fields.movement_0c0.target_roll_02c);
        reverse_distance = NormalizeAngle(
            fields.movement_0c0.target_roll_02c - fields.movement_0c0.roll_028);
        direction = g_negative_one_005ebc38;
        if (reverse_distance < distance) {
            distance = reverse_distance;
            direction = g_one_005ebb38;
        }
        if (distance < step) {
            fields.movement_0c0.roll_028 = fields.movement_0c0.target_roll_02c;
            return;
        }
        fields.movement_0c0.roll_028 = NormalizeAngle(
            step * direction + fields.movement_0c0.roll_028);
    }
}

// FUNCTION: WIZ8 0x00453f30
void W8Navigator::AimAtPosition00453F30(const W8Position* target)
{
    W8Position current;

    current.x = fields.movement_0c0.position_040.x;
    current.y = fields.movement_0c0.position_040.y +
        fields.movement_0c0.height_offset_0b8;
    current.z = fields.movement_0c0.position_040.z;
    if (target->x != current.x || target->y != current.y ||
        target->z != current.z) {
        fields.movement_0c0.target_angle_018 = NormalizeAngle(
            Function4BE420(&current, target));
        if (fields.navigation_mode_008 == 2 ||
            fields.navigation_mode_008 == 3) {
            fields.movement_0c0.target_pitch_024 = NormalizeAngle(
                -Function4BE490(&current, target));
        } else if (fields.navigation_mode_008 == 5 ||
                   fields.navigation_mode_008 == 6) {
            UpdateFacing00454780(0);
        }

        if (fields.movement_0c0.location_id_004 != 0) {
            W8MonsterInfo* monster_info =
                MonsterGetScriptPartByLocationIndex(
                    MonsterGetIndexByLocationID(
                        0xa76,
                        "C:\\Projects\\Wizardry 8\\Engine Code\\Navigator.cpp",
                        fields.movement_0c0.location_id_004,
                        1));
            if (monster_info->fInCombat != 0) {
                monster_info->pCombat->unknown_151[0] = 1;
            }
        }
    }
}

// FUNCTION: WIZ8 0x00455140
void W8Navigator::CollectGroupNavigators00455140(
    W8GrowableVector<W8Navigator*>* navigators)
{
    static const char NAVIGATOR_CPP[] =
        "C:\\Projects\\Wizardry 8\\Engine Code\\Navigator.cpp";
    W8MonsterInfo* monster_info;
    W8MonsterGroup* group;
    unsigned int member;
    int ally;

    if (fields.linked_navigator_05c != 0) {
        return;
    }

    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(
            0xe7b,
            NAVIGATOR_CPP,
            fields.movement_0c0.location_id_004,
            1));
    group = GetMonsterGroupByListIndex(
        GetMonsterGroupIndexByID(
            0xe7c, NAVIGATOR_CPP, monster_info->monster_group_id, 1));

    for (member = 0; member < (unsigned int)group->member_count; ++member) {
        int location_id = IListGetAt(group->monsters, member);
        if (location_id != fields.movement_0c0.location_id_004) {
            monster_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(
                    0xe82, NAVIGATOR_CPP, location_id, 1));
            navigators->Add(monster_info->monster);
        }
    }

    for (ally = 0; ally < 4; ++ally) {
        if (group->allied_group_ids[ally] != 0) {
            W8MonsterGroup* allied_group = GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(
                    0xe8a,
                    NAVIGATOR_CPP,
                    group->allied_group_ids[ally],
                    1));
            for (member = 0;
                 member < (unsigned int)allied_group->member_count;
                 ++member) {
                monster_info = MonsterGetScriptPartByLocationIndex(
                    MonsterGetIndexByLocationID(
                        0xe8e,
                        NAVIGATOR_CPP,
                        IListGetAt(allied_group->monsters, member),
                        1));
                navigators->Add(monster_info->monster);
            }
        }
    }
}

// FUNCTION: WIZ8 0x004553a0
void W8Navigator::UpdateNavigation004553A0(int skip_movement, char slowed)
{
    srVector3T<float> previous = fields.movement_0c0.position_040;
    srVector3T<float> adjusted;
    int movement_result = 0;
    unsigned int movement_kind;
    unsigned char was_stopped;

    fields.tracked_dirty_0b4 = 0;
    if (fields.position_dirty_09c != 0 ||
        fields.movement_0c0.position_adjusted_0c8 != 0) {
        fields.movement_0c0.position_040 = *AdjustPosition00454440(
            &adjusted, &fields.movement_0c0.position_040, &previous);
    }

    if (fields.movement_0c0.vertical_amplitude_080 != g_float_005ebb34) {
        if (g_navigator_vertical_enabled_006081f8 == 0) {
            fields.movement_0c0.vertical_offset_0c0 =
                fields.movement_0c0.vertical_base_07c;
        } else {
            float phase = fields.movement_0c0.vertical_phase_084 +
                g_frame_scale_006068ec * g_object_6598bc->GetValue28() *
                g_navigator_vertical_phase_step_005ebcc8;
            phase -= (float)floor((double)phase);
            fields.movement_0c0.vertical_phase_084 = phase;
            fields.movement_0c0.vertical_offset_0c0 =
                (float)sin((double)phase * g_navigator_cycle_angle_005ec318) *
                    fields.movement_0c0.vertical_amplitude_080 +
                fields.movement_0c0.vertical_base_07c;
        }
    }

    if ((signed char)skip_movement != 0) {
        return;
    }
    if (g_flag_006081e4 == 0 && fields.movement_complete_026 != 0) {
        UpdateAngles00453990();
        return;
    }
    if ((fields.flags_00c & 0x200000) != 0) {
        return;
    }
    if (g_navigator_link_mode_00659c10 != 0 &&
        fields.linked_navigator_05c != 0) {
        return;
    }
    if (fields.linked_navigator_05c == 0) {
        fields.movement_0c0.attachment_0ac->flags_00 |= 0x800000;
    }

    if (g_flag_006081e4 == 0) {
        fields.movement_0c0.movement_speed_064 = g_navigator_speed_006850ff;
        if (slowed != 0) {
            fields.movement_0c0.movement_speed_064 *=
                g_navigator_condition_scale_005ebc7c;
        }
    }
    if ((fields.flags_00c & 0x20000000) != 0 &&
        (fields.flags_00c & 0xffffff) == 0) {
        ConfigureMovement00453D20(-1.0f, -1.0f);
    }
    if (fields.flag_025 != 0 && g_navigator_link_mode_00659c10 == 0) {
        UpdateAngles00453990();
        return;
    }

    if (fields.movement_0c0.attachment_0ac != 0 &&
        fields.movement_0c0.attachment_0ac->value_04 >=
            fields.movement_0c0.attachment_0ac->value_08 &&
        (fields.movement_0c0.attachment_0ac->flags_00 & 0x80000) == 0 &&
        PathAIIsComplete004A9EF0(fields.path_ai_068) != 0) {
        fields.radius_084 = fields.movement_0c0.alternate_radius_0b4;
    }
    if ((fields.flags_00c & 0x100000) != 0) {
        return;
    }

    was_stopped = fields.flag_024;
    movement_kind = fields.flags_00c & 0xffffff;
    switch (movement_kind) {
    case 5:
    case 0x21:
    case 0x41:
    case 0x81:
        movement_result = ResolveMovement00455CC0();
        break;

    case 6:
    case 10: {
        W8PathVector3 next;

        movement_result = g_engine_state_6598a4->AdvanceNavigator00434620(
            &fields.movement_0c0, fields.radius_084, 0.0f);
        if (movement_result == 1) {
            if (PathAINextPoint004A9E90(fields.path_ai_068, &next) != 0) {
                srVector3T<float> target;
                target.x = next.x;
                target.y = next.y;
                target.z = next.z;
                SetMovementTarget00454170(&target, 0);
            } else {
                if (fields.path_ai_068 != 0) {
                    PathAIClearOwned004A9BB0(fields.path_ai_068);
                    fields.path_ai_068 = 0;
                }
                ClearMovement004537E0();
                if (movement_kind == 6 &&
                    (fields.flags_00c & 0x20000000) != 0) {
                    ConfigureMovement00453D20(-1.0f, -1.0f);
                }
            }
        } else if (movement_result == 3) {
            SetMovementTarget00454170(&fields.movement_target_018, 0);
        }
        break;
    }

    case 9:
        if (fields.target_navigator_04c != 0) {
            movement_result = g_engine_state_6598a4->AdvanceNavigator00434620(
                &fields.movement_0c0,
                fields.radius_084,
                (float)fields.collision_margin_010);
            if (movement_result == 1 ||
                (movement_result == 3 &&
                 LinkToNavigator004527A0(
                     fields.target_navigator_04c,
                     fields.collision_margin_010) == 0)) {
                if (fields.flag_024 == 0) {
                    fields.flag_024 = 1;
                    if (fields.navigation_mode_008 != 5 &&
                        fields.navigation_mode_008 != 6) {
                        fields.movement_0c0.target_pitch_024 =
                            NormalizeAngle(0.0f);
                    }
                }
                PathAIClearOwned004A9BB0(fields.path_ai_068);
                fields.movement_0c0.velocity_034.method_00421670();
                if ((fields.movement_0c0.attachment_0ac->flags_00 &
                     0x10000) == 0) {
                    fields.flags_00c &= 0xff000000;
                } else {
                    fields.flags_00c = 0;
                }
                fields.movement_0c0.attachment_0ac->RecordPosition00456AE0(
                    &fields.movement_0c0.position_040);
                g_engine_state_6598a4->QueueOctreeKind130042E810(
                    fields.movement_0c0.location_id_004,
                    &fields.movement_0c0.position_040);
                fields.movement_complete_026 = 1;
            }
        }
        break;

    case 0x201:
        if (fields.linked_navigator_05c != 0 && g_flag_006081e4 != 0) {
            if (fields.linked_navigator_05c->fields.flag_024 != 0) {
                srVector3T<float> delta;
                delta.method_00421650(
                    fields.linked_navigator_05c->fields.movement_0c0.position_040.x -
                        fields.movement_0c0.position_040.x,
                    fields.linked_navigator_05c->fields.movement_0c0.position_040.y -
                        fields.movement_0c0.position_040.y,
                    fields.linked_navigator_05c->fields.movement_0c0.position_040.z -
                        fields.movement_0c0.position_040.z);
                if (delta.method_00421700() <
                    fields.radius_084 * g_navigator_linked_radius_scale_005ebc98 +
                        fields.linked_navigator_05c->fields.radius_084) {
                    if (fields.flag_024 == 0) {
                        fields.flag_024 = 1;
                        if (fields.navigation_mode_008 != 5 &&
                            fields.navigation_mode_008 != 6) {
                            fields.movement_0c0.target_pitch_024 =
                                NormalizeAngle(0.0f);
                        }
                    }
                    break;
                }
            }

            fields.flag_025 = 0;
            if (fields.linked_update_time_0b8 == 0 || fields.flag_024 == 0) {
                if (fields.flag_024 != 0) {
                    fields.flag_024 = 0;
                    if (g_flag_006081e4 != 0 &&
                        fields.linked_navigator_05c == 0) {
                        int index;
                        g_navigator_group_659bf8.Clear();
                        CollectGroupNavigators00455140(
                            &g_navigator_group_659bf8);
                        for (index = 0;
                             index < g_navigator_group_659bf8.GetCount();
                             ++index) {
                            (*g_navigator_group_659bf8.GetAt(index))->
                                fields.flag_024 = 0;
                        }
                    }
                }
                g_engine_state_6598a4->AdvanceNavigator00434620(
                    &fields.movement_0c0,
                    fields.radius_084,
                    fields.linked_navigator_05c->fields.radius_084);
            }
            UpdateLinkedNavigator00454D70();
        }
        break;

    default:
        break;
    }

    if ((fields.flags_00c & 0x800000) != 0 && movement_result == 1) {
        unsigned int previous_flags = fields.flags_00c;
        fields.flags_00c &= 0xff7fffff;
        if (fields.flags_00c == 0) {
            if (fields.flag_024 == 0) {
                fields.flag_024 = 1;
                if (fields.navigation_mode_008 != 5 &&
                    fields.navigation_mode_008 != 6) {
                    fields.movement_0c0.target_pitch_024 =
                        NormalizeAngle(0.0f);
                }
            }
            PathAIClearOwned004A9BB0(fields.path_ai_068);
            fields.movement_0c0.velocity_034.method_00421670();
            if ((fields.movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
                fields.flags_00c &= 0xff000000;
            } else {
                fields.flags_00c = 0;
            }
            fields.movement_0c0.attachment_0ac->RecordPosition00456AE0(
                &fields.movement_0c0.position_040);
            g_engine_state_6598a4->QueueOctreeKind130042E810(
                fields.movement_0c0.location_id_004,
                &fields.movement_0c0.position_040);
            fields.movement_complete_026 = 1;
        } else {
            fields.movement_0c0.value_00c = fields.movement_0c0.value_008;
            fields.movement_0c0.value_010 = -1;
            if ((previous_flags & 0xff000000) != 0) {
                fields.flags_00c |= 6;
            }
            SetMovementTarget00454170(&fields.movement_target_018, 0);
        }
    }

    if (was_stopped == 0 || fields.flag_024 == 0) {
        if (g_flag_006081e4 == 0 && fields.flag_024 == 0) {
            if (fields.movement_0c0.target_position_04c.x !=
                    fields.movement_0c0.position_040.x ||
                fields.movement_0c0.target_position_04c.y !=
                    fields.movement_0c0.position_040.y ||
                fields.movement_0c0.target_position_04c.z !=
                    fields.movement_0c0.position_040.z) {
                W8Position target;
                target.x = fields.movement_0c0.target_position_04c.x;
                target.y = fields.movement_0c0.target_position_04c.y;
                target.z = fields.movement_0c0.target_position_04c.z;
                AimAtPosition00453F30(&target);
            }
        }

        fields.movement_0c0.position_040 = *AdjustPosition00454440(
            &adjusted, &fields.movement_0c0.position_040, &previous);
        UpdateFacing00454780(0);
        if (fields.flag_024 == 0) {
            srVector3T<float> velocity;
            float minimum_speed;

            if (g_flag_006081e4 == 0) {
                velocity.x = fields.movement_0c0.movement_speed_064 *
                    fields.movement_0c0.movement_scale_060 *
                    g_world_scale_005ebc40;
                velocity.z = 0.0f;
            } else {
                velocity.x = fields.movement_0c0.velocity_034.x;
                velocity.z = fields.movement_0c0.velocity_034.z;
            }
            velocity.y =
                (fields.movement_0c0.position_040.y - previous.y) /
                (g_frame_scale_006068ec * g_object_6598bc->GetValue28());
            if (fields.navigation_mode_008 == 2 ||
                fields.navigation_mode_008 == 3) {
                minimum_speed = g_navigator_minimum_speed_mode23_006081f0;
            } else {
                minimum_speed = g_navigator_minimum_speed_006081ec;
            }
            fields.movement_0c0.movement_speed_064 =
                (float)sqrt(
                    velocity.x * velocity.x + velocity.y * velocity.y +
                    velocity.z * velocity.z) /
                (fields.movement_0c0.movement_scale_060 *
                 g_world_scale_005ebc40);
            if (fields.movement_0c0.movement_speed_064 < minimum_speed) {
                fields.movement_0c0.movement_speed_064 = minimum_speed;
            }
        }
    }

    UpdateAngles00453990();
    if (fields.flag_024 == 0 && g_flag_006081e4 == 0 &&
        fields.movement_complete_026 == 0 && fields.flags_00c != 0) {
        fields.movement_0c0.callback_progress_05c +=
            g_object_6598bc->GetValue28() *
            fields.movement_0c0.movement_scale_060 *
            g_frame_scale_006068ec * g_world_scale_005ebc40;
        if (fields.movement_0c0.callback_threshold_058 <=
            fields.movement_0c0.callback_progress_05c) {
            if (fields.movement_callback_08c != 0) {
                fields.movement_callback_08c(this);
            }
            fields.movement_complete_026 = 1;
        }
    }

    {
        float dx = fields.tracked_position_0a4.x -
            fields.movement_0c0.position_040.x;
        float dy = fields.tracked_position_0a4.y -
            fields.movement_0c0.position_040.y;
        float dz = fields.tracked_position_0a4.z -
            fields.movement_0c0.position_040.z;
        if (fields.tracked_distance_0b0 <
            (float)sqrt(dx * dx + dy * dy + dz * dz)) {
            fields.tracked_position_0a4 = fields.movement_0c0.position_040;
            fields.tracked_dirty_0b4 = 1;
        }
    }
}

// FUNCTION: WIZ8 0x00455cc0
int W8Navigator::ResolveMovement00455CC0()
{
    W8Navigator* target = fields.target_navigator_04c;

    if (target == 0) {
        return 0;
    }
    if (g_flag_006081e4 == 0) {
        int result = g_engine_state_6598a4->AdvanceNavigator00434620(
            &fields.movement_0c0,
            fields.radius_084,
            (float)fields.collision_margin_010);
        if (result == 1) {
            ClearMovement004537E0();
        }
        return result;
    }

    if ((fields.movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
        float dx = target->fields.movement_0c0.position_040.x -
            fields.target_last_position_050.x;
        float dy = target->fields.movement_0c0.position_040.y -
            fields.target_last_position_050.y;
        float dz = target->fields.movement_0c0.position_040.z -
            fields.target_last_position_050.z;
        float target_motion = (float)sqrt(dx * dx + dy * dy + dz * dz);

        if (target_motion > g_navigator_target_refresh_distance_005ec030 ||
            (target == g_startup_world_659c0c &&
             target_motion > g_navigator_startup_refresh_distance_005ec150)) {
            fields.target_last_position_050 =
                target->fields.movement_0c0.position_040;

            dx = target->fields.movement_0c0.position_040.x -
                fields.movement_0c0.position_040.x;
            dy = target->fields.movement_0c0.position_040.y -
                fields.movement_0c0.position_040.y;
            dz = target->fields.movement_0c0.position_040.z -
                fields.movement_0c0.position_040.z;
            if (target->fields.radius_084 + fields.radius_084 +
                    (float)fields.collision_margin_010 <
                (float)sqrt(dx * dx + dy * dy + dz * dz)) {
                int index;

                fields.flag_024 = 0;
                if (fields.linked_navigator_05c == 0) {
                    g_navigator_group_659bf8.Clear();
                    CollectGroupNavigators00455140(&g_navigator_group_659bf8);
                    for (index = 0;
                         index < g_navigator_group_659bf8.GetCount();
                         ++index) {
                        (*g_navigator_group_659bf8.GetAt(index))->fields.flag_024 = 0;
                    }
                }
                fields.flag_025 = 0;
                SetMovementTarget00454170(
                    &target->fields.movement_0c0.position_040, 0);
                fields.flags_00c = 5;
            }
        }

        if (fields.flag_025 == 0 && fields.flag_024 == 0) {
            int result = g_engine_state_6598a4->AdvanceNavigator00434620(
                &fields.movement_0c0,
                fields.radius_084,
                (float)fields.collision_margin_010);
            if (result != 1) {
                if (result == 3 &&
                    SetMovementTarget00454170(
                        &target->fields.movement_0c0.position_040, 0) == 0) {
                    ClearMovement004537E0();
                }
                fields.flags_00c |= 5;
                return result;
            }

            SetMovementStopped00453880();
            PathAIClearOwned004A9BB0(fields.path_ai_068);
            fields.movement_0c0.velocity_034.method_00421670();
            if ((fields.movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
                fields.flags_00c &= 0xff000000;
            } else {
                fields.flags_00c = 0;
            }
            fields.movement_0c0.attachment_0ac->RecordPosition00456AE0(
                &fields.movement_0c0.position_040);
            g_engine_state_6598a4->QueueOctreeKind130042E810(
                fields.movement_0c0.location_id_004,
                &fields.movement_0c0.position_040);
            fields.flags_00c |= 5;
            fields.movement_complete_026 = 1;
            return 1;
        }
    } else {
        fields.movement_0c0.attachment_0ac->flags_00 &= ~0x10000;
        fields.flags_00c &= 0xff000000;
        SetMovementStopped00453880();
        PathAIClearOwned004A9BB0(fields.path_ai_068);
        fields.movement_0c0.velocity_034.method_00421670();
        if ((fields.movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
            fields.flags_00c &= 0xff000000;
        } else {
            fields.flags_00c = 0;
        }
        fields.movement_0c0.attachment_0ac->RecordPosition00456AE0(
            &fields.movement_0c0.position_040);
        g_engine_state_6598a4->QueueOctreeKind130042E810(
            fields.movement_0c0.location_id_004,
            &fields.movement_0c0.position_040);
        fields.movement_complete_026 = 1;
    }
    return 0;
}
