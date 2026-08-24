#include "wiz8/grcycle.h"
#include "wiz8/float_constants.h"

#include "surrender/srNode.h"
#include "surrender/srHeap.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/engine_code/PathAI.h"
#include "surrender/srNode.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/utility.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* The world object the navigator notifies when it leaves a location, and
   the notification itself. 0x0042E880 sits outside every assertion-backed
   interval, so it keeps an address-qualified name. */
extern void* g_object_6598a4;
extern "C" void LeaveLocation0042E880(unsigned short location_id, int reason);
/* Tracks the largest radius any navigator has been given. */
extern float g_navigator_largest_extent_6081e8;

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

/* The removal counterpart, inlined into the destructor: find this navigator by
   address, shift the tail down over it and drop the count. The array itself is
   never shrunk, so the capacity survives the removal. */
inline int FindNavigator(const W8Navigator* navigator)
{
    int index;

    for (index = 0; index < g_navigator_count_659b34; ++index) {
        if (g_navigators_659b3c[index] == navigator) {
            return index;
        }
    }
    return -1;
}

inline void UnregisterNavigator(const W8Navigator* navigator)
{
    int index = FindNavigator(navigator);

    if (index < g_navigator_count_659b34 && index >= 0) {
        for (; index < g_navigator_count_659b34 - 1; ++index) {
            g_navigators_659b3c[index] = g_navigators_659b3c[index + 1];
        }
        --g_navigator_count_659b34;
    }
}

}

void NavigatorDefaultCallback00451EA0(W8Navigator* navigator);

// FUNCTION: WIZ8 0x00456ae0
void W8NavigatorAttachment::RecordPosition(
    const srVector3T<float>* position)
{
    flags_00 |= 0x2000000;
    position_40 = *position;
}

/* Grow the attachment's parallel route-position and per-position value arrays
   by ten slots. Both arrays retain every entry through the current index. */
// FUNCTION: WIZ8 0x00456BD0
void W8NavigatorAttachment::GrowPathStorage00456BD0()
{
    unsigned short new_capacity = capacity_0a + 10;
    srVector3T<float>* new_positions = static_cast<srVector3T<float>*>(
        srHeap.allocate(new_capacity * sizeof(srVector3T<float>)));
    unsigned int index;

    for (index = 0; index <= path_position_index_08; ++index) {
        new_positions[index] = position_4c[index];
    }
    srHeap.free(position_4c);
    position_4c = new_positions;

    unsigned short* new_values = static_cast<unsigned short*>(
        malloc(new_capacity * sizeof(unsigned short)));
    memset(new_values, 0, new_capacity * sizeof(unsigned short));
    for (index = 0; index <= path_position_index_08; ++index) {
        new_values[index] = path_values_50[index];
    }
    free(path_values_50);
    path_values_50 = new_values;
    capacity_0a = new_capacity;
}

// FUNCTION: WIZ8 0x00451ec0
W8Navigator::W8Navigator()
{
    unknown_09d[0] = 0;
    navigation_mode_008 = 0;
    flags_00c = 0;
    collision_margin_010 = 0.0;
    movement_target_018.x = 0.0f;
    movement_target_018.y = 0.0f;
    movement_target_018.z = 0.0f;
    flag_024 = 1;
    flag_025 = 0;
    movement_complete_026 = 1;
    position_dirty_09c = 0;
    unknown_027 = 0;
    position_028.x = 0.0f;
    position_028.y = 0.0f;
    position_028.z = 0.0f;
    position_03c.x = 0.0f;
    position_03c.y = 0.0f;
    position_03c.z = 0.0f;
    unknown_048 = 0;
    linked_navigator_05c = 0;
    unknown_060 = 0;
    unknown_064 = 0;
    target_navigator_04c = 0;
    path_ai_068 = 0;
    minimum_06c.x = -500.0f;
    minimum_06c.y = -500.0f;
    minimum_06c.z = -500.0f;
    maximum_078.x = 500.0f;
    maximum_078.y = 500.0f;
    maximum_078.z = 500.0f;
    movement_0c0.vertical_offset_0c0 = 0.0f;
    state_088 = 1;
    unknown_090 = 0;
    target_last_position_050.x = 0.0f;
    target_last_position_050.y = 0.0f;
    target_last_position_050.z = 0.0f;
    minimum_height_034 = 10000.0f;
    maximum_height_038 = 20000.0f;
    movement_0c0.alternate_radius_0b4 = 500.0f;
    movement_0c0.value_0b0 = 500.0f;
    radius_084 = 500.0f;
    movement_0c0.height_offset_0b8 = 500.0f;
    movement_0c0.secondary_height_offset_0bc = 500.0f;
    movement_0c0.value_0c4 = 1.0f;
    movement_0c0.position_adjusted_0c8 = 0;
    movement_callback_08c = NavigatorDefaultCallback00451EA0;
    unknown_094 = 0;
    unknown_098 = 0;
    owned_object_0a0 = 0;
    tracked_distance_0b0 = 500.0f;
    unknown_0bc[1] = 0;
    tracked_dirty_0b4 = 0;
    unknown_0bc[0] = 0;
    linked_update_time_0b8 = 0;
    movement_0c0.Reset();
    tracked_position_0a4.x = 0.0f;
    node_18c = new srNode(0);
    RegisterNavigator(this);
}

/* The callback a navigator starts with: mark it and stop it dead. */
// FUNCTION: WIZ8 0x00451ea0
void NavigatorDefaultCallback00451EA0(W8Navigator* navigator)
{
    navigator->flag_025 = 1;
    navigator->movement_0c0.velocity_034.x = 0.0f;
    navigator->movement_0c0.velocity_034.y = 0.0f;
    navigator->movement_0c0.velocity_034.z = 0.0f;
}

// FUNCTION: WIZ8 0x00453160
void Function453160(void)
{
    int count = g_navigator_count_659b34;
    for (int index = 0; index < count; ++index) {
        W8Navigator* navigator = g_navigators_659b3c[index];
        navigator->flag_025 = 1;
        navigator->movement_0c0.velocity_034.x = 0.0f;
        navigator->movement_0c0.velocity_034.y = 0.0f;
        navigator->movement_0c0.velocity_034.z = 0.0f;
    }
}

// FUNCTION: WIZ8 0x004531a0
void Function4531A0(void)
{
    int count = g_navigator_count_659b34;
    for (int index = 0; index < count; ++index) {
        W8Navigator* navigator = g_navigators_659b3c[index];
        navigator->flag_025 = 0;
        if (navigator->path_ai_068 != 0) {
            PathAIResetTick004A9C20(navigator->path_ai_068);
        }
        if (navigator->path_ai_068 != 0) {
            PathAIResetTick004A9C20(navigator->path_ai_068);
        }
    }
}

/* The attachment owns two allocations from construction: ten srVector3T<float>
   of recorded positions from srHeap, and a zeroed twenty-byte record. */
// FUNCTION: WIZ8 0x00456210
W8NavigatorAttachment::W8NavigatorAttachment()
{
    flags_00 = 0;
    path_position_index_08 = 0;
    value_04 = 0;
    value_0c = 0;
    capacity_0a = 10;
    position_4c = static_cast<srVector3T<float>*>(
        srHeap.allocate(10 * sizeof(srVector3T<float>)));
    path_values_50 = static_cast<unsigned short*>(
        malloc(capacity_0a * sizeof(unsigned short)));
    memset(path_values_50, 0, capacity_0a * sizeof(unsigned short));
    value_058 = 0;
    separation_54 = 0.0f;
    position_10.x = 0.0f;
    position_10.y = 0.0f;
    position_10.z = 0.0f;
    position_1c.x = 0.0f;
    position_1c.y = 0.0f;
    position_1c.z = 0.0f;
}

/* A second pass of defaults over the same tail, run straight after the
   constructor. Where the constructor cleared the orientation, this one gives it
   a facing of three quarter turns, a ten-thousand callback threshold, a turn
   rate of a sixteenth turn, unit scale and speed, and an identity basis in the
   three vectors at +0x88. */
// FUNCTION: WIZ8 0x004573d0
void W8NavigatorMovementState::Reset()
{
    unknown_000 = 0;
    flag_06c = 0;
    unknown_01c = 0.0f;
    pitch_020 = 0.0f;
    target_pitch_024 = 0.0f;
    roll_028 = 0.0f;
    target_roll_02c = 0.0f;
    target_yaw = 4.712389f;
    yaw = 4.712389f;
    velocity_034.x = 0.0f;
    velocity_034.y = 0.0f;
    velocity_034.z = 0.0f;
    position_040.x = 0.0f;
    position_040.y = 0.0f;
    position_040.z = 0.0f;
    callback_progress_05c = 0.0f;
    pitch_enabled_074 = 0;
    roll_enabled_075 = 0;
    vertical_velocity_078 = 0.0f;
    callback_threshold_058 = 10000.0f;
    turn_rate_068 = 0.19634955f;
    unknown_076[0] = 1;
    value_010 = -1;
    movement_scale_060 = 1.0f;
    movement_speed_064 = 1.0f;
    vector_088.x = 1.0f;
    vector_088.y = 0.0f;
    vector_088.z = 0.0f;
    vector_094.x = 0.0f;
    vector_094.y = 1.0f;
    vector_094.z = 0.0f;
    vector_0a0.x = 0.0f;
    vector_0a0.y = 0.0f;
    vector_0a0.z = 1.0f;
}

/* The whole 0xCC tail starts cleared apart from a unit movement speed, the byte
   at +0x76, the invalidated value_010 and the attachment, which the tail
   allocates and owns from construction rather than acquiring later. */
// FUNCTION: WIZ8 0x004572c0
W8NavigatorMovementState::W8NavigatorMovementState()
{
    unknown_000 = 0;
    location_id_004 = 0;
    value_008 = 0;
    value_00c = 0;
    yaw = 0.0f;
    target_yaw = 0.0f;
    unknown_01c = 0.0f;
    pitch_020 = 0.0f;
    target_pitch_024 = 0.0f;
    roll_028 = 0.0f;
    target_roll_02c = 0.0f;
    unknown_030 = 0.0f;
    velocity_034.x = 0.0f;
    velocity_034.y = 0.0f;
    velocity_034.z = 0.0f;
    position_040.x = 0.0f;
    position_040.y = 0.0f;
    position_040.z = 0.0f;
    target_position_04c.x = 0.0f;
    target_position_04c.y = 0.0f;
    target_position_04c.z = 0.0f;
    callback_threshold_058 = 0.0f;
    callback_progress_05c = 0.0f;
    movement_scale_060 = 0.0f;
    movement_speed_064 = 1.0f;
    turn_rate_068 = 0.0f;
    pitch_enabled_074 = 0;
    roll_enabled_075 = 0;
    unknown_076[0] = 1;
    vertical_velocity_078 = 0.0f;
    value_010 = -1;
    vertical_base_07c = 0.0f;
    vertical_amplitude_080 = 0.0f;
    vertical_phase_084 = 0.0f;
    vector_088.x = 0.0f;
    vector_088.y = 0.0f;
    vector_088.z = 0.0f;
    vector_094 = vector_088;
    vector_0a0 = vector_088;
    attachment_0ac = new W8NavigatorAttachment();
    flag_06c = 0;
}

/* Only these eleven fields survive a transfer between navigators; everything
   else in the tail stays whatever the destination already had, and value_010 is
   invalidated rather than copied. */
// FUNCTION: WIZ8 0x004574d0
void W8NavigatorMovementState::CopySettingsFrom(
    const W8NavigatorMovementState& other)
{
    unknown_000 = other.unknown_000;
    value_008 = other.value_008;
    callback_threshold_058 = other.callback_threshold_058;
    callback_progress_05c = other.callback_progress_05c;
    movement_scale_060 = other.movement_scale_060;
    turn_rate_068 = other.turn_rate_068;
    pitch_enabled_074 = other.pitch_enabled_074;
    roll_enabled_075 = other.roll_enabled_075;
    unknown_076[0] = other.unknown_076[0];
    vertical_base_07c = other.vertical_base_07c;
    vertical_amplitude_080 = other.vertical_amplitude_080;
    value_010 = -1;
}

/* The attachment and both allocations hanging off it. Each pointer is cleared
   before its storage goes back, and the two use different allocators. */
// FUNCTION: WIZ8 0x00457530
W8NavigatorMovementState::~W8NavigatorMovementState()
{
    W8NavigatorAttachment* attachment = attachment_0ac;

    if (attachment != 0) {
        srVector3T<float>* position = attachment->position_4c;

        if (position != 0) {
            attachment->position_4c = 0;
            srHeap.free(position);
        }
        if (attachment->path_values_50 != 0) {
            void* allocation = attachment->path_values_50;

            attachment->path_values_50 = 0;
            free(allocation);
        }
        ::operator delete(attachment);
    }
    attachment_0ac = 0;
}

/* A copy shares nothing that ties it to the original's navigation. The path is
   DISCARDED rather than cloned or shared, both navigator links are cleared, the
   owned object is not carried over, and the movement tail is default
   constructed and then given only the eleven fields CopySettingsFrom transfers.
   What does come across is the geometry - the height band, the two bounding
   corners, the radius - and the callback. The copy allocates its own scene node
   and registers itself, so it is a live navigator from birth. */
// FUNCTION: WIZ8 0x00452220
W8Navigator::W8Navigator(const W8Navigator& other)
{
    flags_00c = 0;
    collision_margin_010 = 0.0;
    movement_target_018.x = 0.0f;
    movement_target_018.y = 0.0f;
    movement_target_018.z = 0.0f;
    flag_024 = 1;
    flag_025 = 0;
    movement_complete_026 = 1;
    unknown_027 = 0;
    position_028.x = 0.0f;
    position_028.y = 0.0f;
    position_028.z = 0.0f;
    minimum_height_034 = other.minimum_height_034;
    maximum_height_038 = other.maximum_height_038;
    position_03c = other.position_03c;
    unknown_048 = 0;
    target_navigator_04c = 0;
    target_last_position_050.x = 0.0f;
    target_last_position_050.y = 0.0f;
    target_last_position_050.z = 0.0f;
    linked_navigator_05c = 0;
    unknown_060 = 0;
    unknown_064 = 0;
    path_ai_068 = 0;
    radius_084 = other.radius_084;
    state_088 = 1;
    movement_callback_08c = other.movement_callback_08c;
    unknown_090 = other.unknown_090;
    unknown_094 = other.unknown_094;
    unknown_098 = 0;
    unknown_09d[0] = other.unknown_09d[0];
    owned_object_0a0 = 0;
    tracked_distance_0b0 = other.tracked_distance_0b0;
    unknown_0bc[1] = other.unknown_0bc[1];
    movement_0c0.value_0b0 = other.movement_0c0.value_0b0;
    movement_0c0.alternate_radius_0b4 =
        other.movement_0c0.alternate_radius_0b4;
    movement_0c0.height_offset_0b8 =
        other.movement_0c0.height_offset_0b8;
    movement_0c0.secondary_height_offset_0bc =
        other.movement_0c0.secondary_height_offset_0bc;
    movement_0c0.vertical_offset_0c0 =
        other.movement_0c0.vertical_offset_0c0;
    movement_0c0.value_0c4 = other.movement_0c0.value_0c4;
    minimum_06c = other.minimum_06c;
    maximum_078 = other.maximum_078;
    if (g_navigator_largest_extent_6081e8 < movement_0c0.value_0b0) {
        g_navigator_largest_extent_6081e8 = movement_0c0.value_0b0;
    }
    if (g_navigator_largest_extent_6081e8 <
        movement_0c0.alternate_radius_0b4) {
        g_navigator_largest_extent_6081e8 =
            movement_0c0.alternate_radius_0b4;
    }
    if (g_navigator_largest_extent_6081e8 < radius_084) {
        g_navigator_largest_extent_6081e8 = radius_084;
    }
    movement_0c0.CopySettingsFrom(other.movement_0c0);
    movement_0c0.height_offset_0b8 -=
        other.movement_0c0.vertical_base_07c;
    movement_0c0.secondary_height_offset_0bc -=
        other.movement_0c0.vertical_base_07c;
    SetNavigationMode(other.navigation_mode_008);
    position_dirty_09c = 0;
    movement_0c0.position_adjusted_0c8 = 0;
    tracked_dirty_0b4 = 0;
    unknown_0bc[0] = 0;
    linked_update_time_0b8 = 0;
    tracked_position_0a4.x = 0.0f;
    tracked_position_0a4.y = 0.0f;
    tracked_position_0a4.z = 0.0f;
    node_18c =
        new srClassSupport<srNode, srNode, false, 0x1000>(
            static_cast<srNode*>(0));
    node_18c->setLocation(other.node_18c->getLocation());
    RegisterNavigator(this);
}

/* Everything this navigator owns, in the order the retail body releases it.
   The path goes through the kind-guarded PathAI helper rather than the general
   one; the object at +0xa0 is deleted through its own virtual slot; the scene
   node is reference-counted down, not deleted, so whoever else holds it keeps
   it alive; and the movement tail's own destructor releases the attachment.
   Leaving the world's location index is a notification, not a release. */
// FUNCTION: WIZ8 0x00452120
W8Navigator::~W8Navigator()
{
    if (path_ai_068 != 0) {
        DestroyOwnedPathAI004A9110(path_ai_068);
    }
    UnregisterNavigator(this);
    delete owned_object_0a0;
    owned_object_0a0 = 0;
    if (movement_0c0.location_id_004 != 0 && g_object_6598a4 != 0) {
        LeaveLocation0042E880(movement_0c0.location_id_004, 0xd);
    }
    if (node_18c != 0) {
        node_18c->release();
    }
}

/* Six of the seven modes give the navigator a fresh path record; mode four only
   resets the two movement enables. What the modes actually differ in is that
   pair: mode one and four clear both, two, three and five enable pitch, and six
   enables pitch and roll. */
// FUNCTION: WIZ8 0x00452e50
void W8Navigator::SetNavigationMode(int mode)
{
    W8PathAI* path;

    navigation_mode_008 = mode;
    switch (mode) {
    case 1:
        path = CreateRecord004A9750(0);
        PathAISetFlag3A004A9B90(path, 1);
        SetPathAI(path);
        /* Falls into mode four's body: the retail block ends where mode four's
           jump-table entry lands. */
    case 4:
        movement_0c0.pitch_enabled_074 = 0;
        movement_0c0.roll_enabled_075 = 0;
        break;
    case 2:
    case 3:
    case 5:
        path = CreateRecord004A9750(0);
        PathAISetFlag3A004A9B90(path, 1);
        SetPathAI(path);
        movement_0c0.pitch_enabled_074 = 1;
        movement_0c0.roll_enabled_075 = 0;
        break;
    case 6:
        path = CreateRecord004A9750(0);
        PathAISetFlag3A004A9B90(path, 1);
        SetPathAI(path);
        movement_0c0.pitch_enabled_074 = 1;
        movement_0c0.roll_enabled_075 = 1;
        break;
    default:
        break;
    }
}

/* Replace the collision bounds and keep the broad-phase radius synchronized
   with the movement state's alternate radius. */
// FUNCTION: WIZ8 0x00452f10
void W8Navigator::SetBounds(
    const srVector3T<float>* minimum,
    const srVector3T<float>* maximum)
{
    minimum_06c = *minimum;
    maximum_078 = *maximum;
    radius_084 = movement_0c0.alternate_radius_0b4;
}

// FUNCTION: WIZ8 0x004534c0
srVector3T<float> W8Navigator::GetPosition()
{
    return movement_0c0.position_040;
}

// FUNCTION: WIZ8 0x00454950
unsigned char W8Navigator::UpdateTrackedPosition00454950()
{
    float dx = tracked_position_0a4.x - movement_0c0.position_040.x;
    float dy = tracked_position_0a4.y - movement_0c0.position_040.y;
    float dz = tracked_position_0a4.z - movement_0c0.position_040.z;
    float distance = (float)sqrt(dx * dx + dy * dy + dz * dz);

    if (distance > tracked_distance_0b0) {
        tracked_position_0a4 = movement_0c0.position_040;
        tracked_dirty_0b4 = 1;
    }
    return tracked_dirty_0b4;
}

// FUNCTION: WIZ8 0x00453880
void W8Navigator::SetMovementStopped00453880()
{
    if (flag_024 == 0) {
        flag_024 = 1;
        if (navigation_mode_008 != 5 && navigation_mode_008 != 6) {
            movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
        }
    }
}

// FUNCTION: WIZ8 0x004538f0
void W8Navigator::SetAngles004538F0(float angle)
{
    movement_0c0.yaw = NormalizeAngle(angle);
    movement_0c0.target_yaw = NormalizeAngle(angle);
}

// FUNCTION: WIZ8 0x00453940
void W8Navigator::SetPitch(float pitch)
{
    movement_0c0.pitch_020 = NormalizeAngle(pitch);
    movement_0c0.target_pitch_024 = NormalizeAngle(pitch);
}

// FUNCTION: WIZ8 0x00453970
float W8Navigator::GetYaw()
{
    return movement_0c0.yaw;
}

// FUNCTION: WIZ8 0x00453980
float W8Navigator::GetPitch()
{
    return movement_0c0.pitch_020;
}

// FUNCTION: WIZ8 0x00453c90
void W8Navigator::SetTurnRate(float turn_rate)
{
    movement_0c0.turn_rate_068 = turn_rate;
}

// FUNCTION: WIZ8 0x004538b0
void W8Navigator::SetPathAI(W8PathAI* path_ai)
{
    path_ai_068 = path_ai;
}

// FUNCTION: WIZ8 0x004538c0
W8PathAI* W8Navigator::GetPathAI()
{
    return path_ai_068;
}

// FUNCTION: WIZ8 0x00454930
void W8Navigator::ResetPathAI()
{
    if (path_ai_068 != 0) {
        path_ai_068->value_04 = 0.0f;
        path_ai_068->value_24 = 0.0f;
        PathAIResetTick004A9C20(path_ai_068);
    }
}

void W8Navigator::configureStartupRange(float range)
{
    radius_084 = range;
    unknown_090 = 1;
    movement_0c0.value_0b0 = range;
    movement_0c0.alternate_radius_0b4 = range;
}

void W8Navigator::configureStartupDepth(float near_depth, float far_depth)
{
    movement_0c0.height_offset_0b8 = near_depth;
    movement_0c0.secondary_height_offset_0bc = far_depth;
}

extern "C" {
extern W8Navigator* g_startup_world_659c0c;
extern unsigned char g_navigator_position_changed_659c11;
}

extern void Function454780(int changed);

extern float g_navigator_default_turn_rate_005ec2f4;
extern float g_frame_scale_006068ec;
extern const float g_negative_one_005ebc38;
extern float g_navigator_snap_angle_005ec2f0;
extern float g_navigator_mode3_scale_005ebca4;
extern unsigned char g_flag_006081e4;

// FUNCTION: WIZ8 0x004526c0
unsigned short W8Navigator::Function4526C0(
    W8Navigator* target, double separation)
{
    unsigned short result = 0;

    movement_target_018.x = 0.0f;
    movement_target_018.y = 0.0f;
    movement_target_018.z = 0.0f;
    collision_margin_010 = separation;
    target_navigator_04c = target;
    if (target == g_startup_world_659c0c ||
        target->movement_0c0.location_id_004 != 0) {
        movement_0c0.value_010 =
            target->movement_0c0.location_id_004;
    }
    else {
        movement_0c0.value_010 = -1;
    }
    PathAIClearOwned004A9BB0(path_ai_068);
    if (g_flag_006081e4 == 0) {
        movement_0c0.attachment_0ac->flags_00 |= 0x10000;
    }
    if (SetMovementTarget(
            &target->movement_0c0.position_040, 0) == 0) {
        if (g_flag_006081e4 == 0) {
            navigation_mode_008 = 0;
            unknown_0bc[0] = 1;
        }
    }
    else {
        navigation_mode_008 = 5;
        result = 1;
        if (g_flag_006081e4 == 0) {
            result = (unsigned short)(
                movement_0c0.attachment_0ac->flags_00 & 7);
        }
    }
    target_last_position_050 =
        target->movement_0c0.position_040;
    return result;
}

// FUNCTION: WIZ8 0x00453cc0
void W8Navigator::StartPatrol(
    const srVector3T<float>* home, float distance, float variation)
{
    W8Navigator* navigator = this;

    while (navigator->linked_navigator_05c != 0) {
        navigator = navigator->linked_navigator_05c;
    }
    navigator->navigation_mode_008 = 0;
    navigator->position_03c.x = home->x;
    navigator->position_03c.y = home->y;
    navigator->position_03c.z = home->z;
    navigator->minimum_height_034 = distance;
    navigator->maximum_height_038 = variation;
    navigator->ConfigureMovement00453D20(distance, variation);
}
extern unsigned char g_navigator_vertical_enabled_006081f8;
extern unsigned char g_navigator_link_mode_00659c10;
extern float g_navigator_speed_006850ff;
extern float g_navigator_linked_radius_scale_005ebc98;
extern float g_navigator_vertical_phase_step_005ebcc8;
extern float g_navigator_startup_refresh_distance_005ec150;
extern float g_navigator_minimum_speed_006081ec;
extern float g_navigator_minimum_speed_mode23_006081f0;
extern const float g_world_scale_005ebc40;
extern W8GrowableVector<W8Navigator*> g_navigator_group_659bf8;
extern float Function4BE420(
    const srVector3T<float>* from, const srVector3T<float>* to);
extern float Function4BE490(
    const srVector3T<float>* from, const srVector3T<float>* to);

// FUNCTION: WIZ8 0x00453590
void W8Navigator::SetPositionInternal00453590(const srVector3T<float>* position)
{
    srVector3T<double> widened;

    if (position->x != movement_0c0.position_040.x ||
        position->y != movement_0c0.position_040.y ||
        position->z != movement_0c0.position_040.z) {
        movement_0c0.position_040.x = position->x;
        movement_0c0.position_040.y = position->y;
        movement_0c0.position_040.z = position->z;
        widened.x = position->x;
        widened.y = position->y;
        widened.z = position->z;
        node_18c->setLocation(widened);
        if (movement_0c0.location_id_004 != 0 ||
            this == g_startup_world_659c0c) {
            g_navigator_position_changed_659c11 = 1;
        }
        Function454780(1);
        if (movement_0c0.attachment_0ac != 0) {
            *movement_0c0.attachment_0ac->position_4c =
                movement_0c0.position_040;
            movement_0c0.attachment_0ac->position_34 =
                *movement_0c0.attachment_0ac->position_4c;
            movement_0c0.attachment_0ac->position_10 =
                *movement_0c0.attachment_0ac->position_4c;
        }
    }
}

// FUNCTION: WIZ8 0x004537e0
void W8Navigator::ClearMovement()
{
    if (flag_024 == 0) {
        flag_024 = 1;
        if (navigation_mode_008 != 5 && navigation_mode_008 != 6) {
            movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
        }
    }

    PathAIClearOwned004A9BB0(path_ai_068);
    movement_0c0.velocity_034.method_00421670();
    if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
        flags_00c &= 0xff000000;
    } else {
        flags_00c = 0;
    }
    movement_0c0.attachment_0ac->RecordPosition(
        &movement_0c0.position_040);
    g_octree_6598a4->QueueOctreeKind130042E810(
        movement_0c0.location_id_004,
        &movement_0c0.position_040);
    movement_complete_026 = 1;
}

// FUNCTION: WIZ8 0x00453990
void W8Navigator::UpdateAngles00453990()
{
    float step = g_navigator_default_turn_rate_005ec2f4;
    float distance;
    float reverse_distance;
    float direction;

    if (g_in_combat_00683f94 == 0) {
        step = movement_0c0.turn_rate_068;
    }
    step *= g_frame_scale_006068ec * g_object_6598bc->GetValue28();

    if (movement_0c0.yaw != movement_0c0.target_yaw) {
        distance = NormalizeAngle(
            movement_0c0.yaw - movement_0c0.target_yaw);
        reverse_distance = NormalizeAngle(
            movement_0c0.target_yaw - movement_0c0.yaw);
        direction = g_negative_one_005ebc38;
        if (reverse_distance < distance) {
            distance = reverse_distance;
            direction = g_float_005ebb38;
        }
        if (step <= distance) {
            movement_0c0.yaw = NormalizeAngle(
                step * direction + movement_0c0.yaw);
        } else {
            movement_0c0.yaw = movement_0c0.target_yaw;
        }
        if (flag_024 != 0) {
            UpdateFacing(0);
        }
        if ((float)fabs(movement_0c0.yaw -
                        movement_0c0.target_yaw) <
            g_navigator_snap_angle_005ec2f0) {
            movement_0c0.yaw = movement_0c0.target_yaw;
        }
    }

    if (navigation_mode_008 == 3) {
        step *= g_navigator_mode3_scale_005ebca4;
    }
    if (movement_0c0.pitch_enabled_074 != 0 &&
        movement_0c0.pitch_020 != movement_0c0.target_pitch_024) {
        distance = NormalizeAngle(
            movement_0c0.pitch_020 - movement_0c0.target_pitch_024);
        reverse_distance = NormalizeAngle(
            movement_0c0.target_pitch_024 - movement_0c0.pitch_020);
        direction = g_negative_one_005ebc38;
        if (reverse_distance < distance) {
            distance = reverse_distance;
            direction = g_float_005ebb38;
        }
        if (step <= distance) {
            movement_0c0.pitch_020 = NormalizeAngle(
                step * direction + movement_0c0.pitch_020);
        } else {
            movement_0c0.pitch_020 = movement_0c0.target_pitch_024;
        }
    }

    if (movement_0c0.roll_enabled_075 != 0 &&
        movement_0c0.roll_028 != movement_0c0.target_roll_02c) {
        distance = NormalizeAngle(
            movement_0c0.roll_028 - movement_0c0.target_roll_02c);
        reverse_distance = NormalizeAngle(
            movement_0c0.target_roll_02c - movement_0c0.roll_028);
        direction = g_negative_one_005ebc38;
        if (reverse_distance < distance) {
            distance = reverse_distance;
            direction = g_float_005ebb38;
        }
        if (distance < step) {
            movement_0c0.roll_028 = movement_0c0.target_roll_02c;
            return;
        }
        movement_0c0.roll_028 = NormalizeAngle(
            step * direction + movement_0c0.roll_028);
    }
}

// FUNCTION: WIZ8 0x00453f30
void W8Navigator::AimAtPosition(const srVector3T<float>* target)
{
    srVector3T<float> current;

    current.x = movement_0c0.position_040.x;
    current.y = movement_0c0.position_040.y +
        movement_0c0.height_offset_0b8;
    current.z = movement_0c0.position_040.z;
    if (target->x != current.x || target->y != current.y ||
        target->z != current.z) {
        movement_0c0.target_yaw = NormalizeAngle(
            Function4BE420(&current, target));
        if (navigation_mode_008 == 2 ||
            navigation_mode_008 == 3) {
            movement_0c0.target_pitch_024 = NormalizeAngle(
                -Function4BE490(&current, target));
        } else if (navigation_mode_008 == 5 ||
                   navigation_mode_008 == 6) {
            UpdateFacing(0);
        }

        if (movement_0c0.location_id_004 != 0) {
            W8MonsterInfo* monster_info =
                MonsterGetScriptPartByLocationIndex(
                    MonsterGetIndexByLocationID(
                        0xa76,
                        "C:\\Projects\\Wizardry 8\\Engine Code\\Navigator.cpp",
                        movement_0c0.location_id_004,
                        1));
            if (monster_info->fInCombat != 0) {
                monster_info->pCombat->unknown_151[0] = 1;
            }
        }
    }
}

// FUNCTION: WIZ8 0x00455140
void W8Navigator::CollectGroupNavigators(
    W8GrowableVector<W8Navigator*>* navigators)
{
    static const char NAVIGATOR_CPP[] =
        "C:\\Projects\\Wizardry 8\\Engine Code\\Navigator.cpp";
    W8MonsterInfo* monster_info;
    W8MonsterGroup* group;
    unsigned int member;
    int ally;

    if (linked_navigator_05c != 0) {
        return;
    }

    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(
            0xe7b,
            NAVIGATOR_CPP,
            movement_0c0.location_id_004,
            1));
    group = GetMonsterGroupByListIndex(
        GetMonsterGroupIndexByID(
            0xe7c, NAVIGATOR_CPP, monster_info->monster_group_id, 1));

    for (member = 0; member < (unsigned int)group->member_count; ++member) {
        int location_id = IListGetAt(group->monsters, member);
        if (location_id != movement_0c0.location_id_004) {
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
    srVector3T<float> previous = movement_0c0.position_040;
    srVector3T<float> adjusted;
    int movement_result = 0;
    unsigned int movement_kind;
    unsigned char was_stopped;

    tracked_dirty_0b4 = 0;
    if (position_dirty_09c != 0 ||
        movement_0c0.position_adjusted_0c8 != 0) {
        movement_0c0.position_040 = *AdjustPosition00454440(
            &adjusted, &movement_0c0.position_040, &previous);
    }

    if (movement_0c0.vertical_amplitude_080 != g_float_005ebb34) {
        if (g_navigator_vertical_enabled_006081f8 == 0) {
            movement_0c0.vertical_offset_0c0 =
                movement_0c0.vertical_base_07c;
        } else {
            float phase = movement_0c0.vertical_phase_084 +
                g_frame_scale_006068ec * g_object_6598bc->GetValue28() *
                g_navigator_vertical_phase_step_005ebcc8;
            phase -= (float)floor((double)phase);
            movement_0c0.vertical_phase_084 = phase;
            movement_0c0.vertical_offset_0c0 =
                (float)sin((double)phase * g_double_005ec318) *
                    movement_0c0.vertical_amplitude_080 +
                movement_0c0.vertical_base_07c;
        }
    }

    if ((signed char)skip_movement != 0) {
        return;
    }
    if (g_flag_006081e4 == 0 && movement_complete_026 != 0) {
        UpdateAngles00453990();
        return;
    }
    if ((flags_00c & 0x200000) != 0) {
        return;
    }
    if (g_navigator_link_mode_00659c10 != 0 &&
        linked_navigator_05c != 0) {
        return;
    }
    if (linked_navigator_05c == 0) {
        movement_0c0.attachment_0ac->flags_00 |= 0x800000;
    }

    if (g_flag_006081e4 == 0) {
        movement_0c0.movement_speed_064 = g_navigator_speed_006850ff;
        if (slowed != 0) {
            movement_0c0.movement_speed_064 *=
                g_float_005ebc7c;
        }
    }
    if ((flags_00c & 0x20000000) != 0 &&
        (flags_00c & 0xffffff) == 0) {
        ConfigureMovement00453D20(-1.0f, -1.0f);
    }
    if (flag_025 != 0 && g_navigator_link_mode_00659c10 == 0) {
        UpdateAngles00453990();
        return;
    }

    if (movement_0c0.attachment_0ac != 0 &&
        movement_0c0.attachment_0ac->value_04 >=
            movement_0c0.attachment_0ac->path_position_index_08 &&
        (movement_0c0.attachment_0ac->flags_00 & 0x80000) == 0 &&
        PathAIIsComplete004A9EF0(path_ai_068) != 0) {
        radius_084 = movement_0c0.alternate_radius_0b4;
    }
    if ((flags_00c & 0x100000) != 0) {
        return;
    }

    was_stopped = flag_024;
    movement_kind = flags_00c & 0xffffff;
    switch (movement_kind) {
    case 5:
    case 0x21:
    case 0x41:
    case 0x81:
        movement_result = ResolveMovement();
        break;

    case 6:
    case 10: {
        srVector3T<float> next;

        movement_result = g_octree_6598a4->AdvanceNavigator(
            &movement_0c0, radius_084, 0.0f);
        if (movement_result == 1) {
            if (PathAINextPoint004A9E90(path_ai_068, &next) != 0) {
                srVector3T<float> target;
                target.x = next.x;
                target.y = next.y;
                target.z = next.z;
                SetMovementTarget(&target, 0);
            } else {
                if (path_ai_068 != 0) {
                    PathAIClearOwned004A9BB0(path_ai_068);
                    path_ai_068 = 0;
                }
                ClearMovement();
                if (movement_kind == 6 &&
                    (flags_00c & 0x20000000) != 0) {
                    ConfigureMovement00453D20(-1.0f, -1.0f);
                }
            }
        } else if (movement_result == 3) {
            SetMovementTarget(&movement_target_018, 0);
        }
        break;
    }

    case 9:
        if (target_navigator_04c != 0) {
            movement_result = g_octree_6598a4->AdvanceNavigator(
                &movement_0c0,
                radius_084,
                (float)collision_margin_010);
            if (movement_result == 1 ||
                (movement_result == 3 &&
                 LinkToNavigator004527A0(
                     target_navigator_04c,
                     collision_margin_010) == 0)) {
                if (flag_024 == 0) {
                    flag_024 = 1;
                    if (navigation_mode_008 != 5 &&
                        navigation_mode_008 != 6) {
                        movement_0c0.target_pitch_024 =
                            NormalizeAngle(0.0f);
                    }
                }
                PathAIClearOwned004A9BB0(path_ai_068);
                movement_0c0.velocity_034.method_00421670();
                if ((movement_0c0.attachment_0ac->flags_00 &
                     0x10000) == 0) {
                    flags_00c &= 0xff000000;
                } else {
                    flags_00c = 0;
                }
                movement_0c0.attachment_0ac->RecordPosition(
                    &movement_0c0.position_040);
                g_octree_6598a4->QueueOctreeKind130042E810(
                    movement_0c0.location_id_004,
                    &movement_0c0.position_040);
                movement_complete_026 = 1;
            }
        }
        break;

    case 0x201:
        if (linked_navigator_05c != 0 && g_flag_006081e4 != 0) {
            if (linked_navigator_05c->flag_024 != 0) {
                srVector3T<float> delta(
                    linked_navigator_05c->movement_0c0.position_040.x -
                        movement_0c0.position_040.x,
                    linked_navigator_05c->movement_0c0.position_040.y -
                        movement_0c0.position_040.y,
                    linked_navigator_05c->movement_0c0.position_040.z -
                        movement_0c0.position_040.z);
                if (delta.method_00421700() <
                    radius_084 * g_navigator_linked_radius_scale_005ebc98 +
                        linked_navigator_05c->radius_084) {
                    if (flag_024 == 0) {
                        flag_024 = 1;
                        if (navigation_mode_008 != 5 &&
                            navigation_mode_008 != 6) {
                            movement_0c0.target_pitch_024 =
                                NormalizeAngle(0.0f);
                        }
                    }
                    break;
                }
            }

            flag_025 = 0;
            if (linked_update_time_0b8 == 0 || flag_024 == 0) {
                if (flag_024 != 0) {
                    flag_024 = 0;
                    if (g_flag_006081e4 != 0 &&
                        linked_navigator_05c == 0) {
                        int index;
                        g_navigator_group_659bf8.Clear();
                        CollectGroupNavigators(
                            &g_navigator_group_659bf8);
                        for (index = 0;
                             index < g_navigator_group_659bf8.GetCount();
                             ++index) {
                            (*g_navigator_group_659bf8.GetAt(index))->
                                flag_024 = 0;
                        }
                    }
                }
                g_octree_6598a4->AdvanceNavigator(
                    &movement_0c0,
                    radius_084,
                    linked_navigator_05c->radius_084);
            }
            UpdateLinkedNavigator();
        }
        break;

    default:
        break;
    }

    if ((flags_00c & 0x800000) != 0 && movement_result == 1) {
        unsigned int previous_flags = flags_00c;
        flags_00c &= 0xff7fffff;
        if (flags_00c == 0) {
            if (flag_024 == 0) {
                flag_024 = 1;
                if (navigation_mode_008 != 5 &&
                    navigation_mode_008 != 6) {
                    movement_0c0.target_pitch_024 =
                        NormalizeAngle(0.0f);
                }
            }
            PathAIClearOwned004A9BB0(path_ai_068);
            movement_0c0.velocity_034.method_00421670();
            if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
                flags_00c &= 0xff000000;
            } else {
                flags_00c = 0;
            }
            movement_0c0.attachment_0ac->RecordPosition(
                &movement_0c0.position_040);
            g_octree_6598a4->QueueOctreeKind130042E810(
                movement_0c0.location_id_004,
                &movement_0c0.position_040);
            movement_complete_026 = 1;
        } else {
            movement_0c0.value_00c = movement_0c0.value_008;
            movement_0c0.value_010 = -1;
            if ((previous_flags & 0xff000000) != 0) {
                flags_00c |= 6;
            }
            SetMovementTarget(&movement_target_018, 0);
        }
    }

    if (was_stopped == 0 || flag_024 == 0) {
        if (g_flag_006081e4 == 0 && flag_024 == 0) {
            if (movement_0c0.target_position_04c.x !=
                    movement_0c0.position_040.x ||
                movement_0c0.target_position_04c.y !=
                    movement_0c0.position_040.y ||
                movement_0c0.target_position_04c.z !=
                    movement_0c0.position_040.z) {
                srVector3T<float> target;
                target.x = movement_0c0.target_position_04c.x;
                target.y = movement_0c0.target_position_04c.y;
                target.z = movement_0c0.target_position_04c.z;
                AimAtPosition(&target);
            }
        }

        movement_0c0.position_040 = *AdjustPosition00454440(
            &adjusted, &movement_0c0.position_040, &previous);
        UpdateFacing(0);
        if (flag_024 == 0) {
            srVector3T<float> velocity;
            float minimum_speed;

            if (g_flag_006081e4 == 0) {
                velocity.x = movement_0c0.movement_speed_064 *
                    movement_0c0.movement_scale_060 *
                    g_world_scale_005ebc40;
                velocity.z = 0.0f;
            } else {
                velocity.x = movement_0c0.velocity_034.x;
                velocity.z = movement_0c0.velocity_034.z;
            }
            velocity.y =
                (movement_0c0.position_040.y - previous.y) /
                (g_frame_scale_006068ec * g_object_6598bc->GetValue28());
            if (navigation_mode_008 == 2 ||
                navigation_mode_008 == 3) {
                minimum_speed = g_navigator_minimum_speed_mode23_006081f0;
            } else {
                minimum_speed = g_navigator_minimum_speed_006081ec;
            }
            movement_0c0.movement_speed_064 =
                (float)sqrt(
                    velocity.x * velocity.x + velocity.y * velocity.y +
                    velocity.z * velocity.z) /
                (movement_0c0.movement_scale_060 *
                 g_world_scale_005ebc40);
            if (movement_0c0.movement_speed_064 < minimum_speed) {
                movement_0c0.movement_speed_064 = minimum_speed;
            }
        }
    }

    UpdateAngles00453990();
    if (flag_024 == 0 && g_flag_006081e4 == 0 &&
        movement_complete_026 == 0 && flags_00c != 0) {
        movement_0c0.callback_progress_05c +=
            g_object_6598bc->GetValue28() *
            movement_0c0.movement_scale_060 *
            g_frame_scale_006068ec * g_world_scale_005ebc40;
        if (movement_0c0.callback_threshold_058 <=
            movement_0c0.callback_progress_05c) {
            if (movement_callback_08c != 0) {
                movement_callback_08c(this);
            }
            movement_complete_026 = 1;
        }
    }

    {
        float dx = tracked_position_0a4.x -
            movement_0c0.position_040.x;
        float dy = tracked_position_0a4.y -
            movement_0c0.position_040.y;
        float dz = tracked_position_0a4.z -
            movement_0c0.position_040.z;
        if (tracked_distance_0b0 <
            (float)sqrt(dx * dx + dy * dy + dz * dz)) {
            tracked_position_0a4 = movement_0c0.position_040;
            tracked_dirty_0b4 = 1;
        }
    }
}

// FUNCTION: WIZ8 0x00455cc0
int W8Navigator::ResolveMovement()
{
    W8Navigator* target = target_navigator_04c;

    if (target == 0) {
        return 0;
    }
    if (g_flag_006081e4 == 0) {
        int result = g_octree_6598a4->AdvanceNavigator(
            &movement_0c0,
            radius_084,
            (float)collision_margin_010);
        if (result == 1) {
            ClearMovement();
        }
        return result;
    }

    if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
        float dx = target->movement_0c0.position_040.x -
            target_last_position_050.x;
        float dy = target->movement_0c0.position_040.y -
            target_last_position_050.y;
        float dz = target->movement_0c0.position_040.z -
            target_last_position_050.z;
        float target_motion = (float)sqrt(dx * dx + dy * dy + dz * dz);

        if ((double)target_motion > g_double_005ec030 ||
            (target == g_startup_world_659c0c &&
             target_motion > g_navigator_startup_refresh_distance_005ec150)) {
            target_last_position_050 =
                target->movement_0c0.position_040;

            dx = target->movement_0c0.position_040.x -
                movement_0c0.position_040.x;
            dy = target->movement_0c0.position_040.y -
                movement_0c0.position_040.y;
            dz = target->movement_0c0.position_040.z -
                movement_0c0.position_040.z;
            if (target->radius_084 + radius_084 +
                    (float)collision_margin_010 <
                (float)sqrt(dx * dx + dy * dy + dz * dz)) {
                int index;

                flag_024 = 0;
                if (linked_navigator_05c == 0) {
                    g_navigator_group_659bf8.Clear();
                    CollectGroupNavigators(&g_navigator_group_659bf8);
                    for (index = 0;
                         index < g_navigator_group_659bf8.GetCount();
                         ++index) {
                        (*g_navigator_group_659bf8.GetAt(index))->flag_024 = 0;
                    }
                }
                flag_025 = 0;
                SetMovementTarget(
                    &target->movement_0c0.position_040, 0);
                flags_00c = 5;
            }
        }

        if (flag_025 == 0 && flag_024 == 0) {
            int result = g_octree_6598a4->AdvanceNavigator(
                &movement_0c0,
                radius_084,
                (float)collision_margin_010);
            if (result != 1) {
                if (result == 3 &&
                    SetMovementTarget(
                        &target->movement_0c0.position_040, 0) == 0) {
                    ClearMovement();
                }
                flags_00c |= 5;
                return result;
            }

            SetMovementStopped00453880();
            PathAIClearOwned004A9BB0(path_ai_068);
            movement_0c0.velocity_034.method_00421670();
            if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
                flags_00c &= 0xff000000;
            } else {
                flags_00c = 0;
            }
            movement_0c0.attachment_0ac->RecordPosition(
                &movement_0c0.position_040);
            g_octree_6598a4->QueueOctreeKind130042E810(
                movement_0c0.location_id_004,
                &movement_0c0.position_040);
            flags_00c |= 5;
            movement_complete_026 = 1;
            return 1;
        }
    } else {
        movement_0c0.attachment_0ac->flags_00 &= ~0x10000;
        flags_00c &= 0xff000000;
        SetMovementStopped00453880();
        PathAIClearOwned004A9BB0(path_ai_068);
        movement_0c0.velocity_034.method_00421670();
        if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
            flags_00c &= 0xff000000;
        } else {
            flags_00c = 0;
        }
        movement_0c0.attachment_0ac->RecordPosition(
            &movement_0c0.position_040);
        g_octree_6598a4->QueueOctreeKind130042E810(
            movement_0c0.location_id_004,
            &movement_0c0.position_040);
        movement_complete_026 = 1;
    }
    return 0;
}
