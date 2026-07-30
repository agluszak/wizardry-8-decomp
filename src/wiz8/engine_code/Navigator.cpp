#include "wiz8/grcycle.h"

#include "surrender/srNode.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_state_006598a4.h"
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

// FUNCTION: WIZ8 0x004538f0
void W8Navigator::SetAngles004538F0(float angle)
{
    fields.movement_0c0.angle_014 = NormalizeAngle(angle);
    fields.movement_0c0.target_angle_018 = NormalizeAngle(angle);
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
extern void* g_startup_world_659c0c;
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
