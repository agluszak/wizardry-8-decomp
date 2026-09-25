#include "FileMan.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/startup_world.h"
#include "wiz8/xstatus.h"
#include "wiz8/float_constants.h"
#include "surrender/srNode.h"
#include "surrender/srHeap.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/engine_code/GameTimeAccumulator.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/utility.h"
#include "wiz8/sr_api.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/PolyPick.h"
// GLOBAL: WIZ8 0x005ec2f8
float g_float_005ec2f8 = 5000.0f;
// GLOBAL: WIZ8 0x005ec030
double g_double_005ec030 = 2500.0;

/* The world object the navigator notifies when it leaves a location, and
   the notification itself. 0x0042E880 sits outside every assertion-backed
   interval, so it keeps an address-qualified name. */

namespace {

// GLOBAL: WIZ8 0x00659b30
W8GrowableVector<W8Navigator*> g_registered_navigators(5);

} // namespace

// FUNCTION: WIZ8 0x00456ae0
void W8NavigatorAttachment::RecordPosition(const srVector3T<float>* position)
{
    flags_00 |= 0x2000000;
    position_40 = *position;
}

/* Trims the recorded route to the boundary of the `radius` sphere around
   `target`: walks stored positions while they stay inside, then lerps the
   crossing point into position_1c and over the first outside waypoint, moves
   path_position_index_08 there, and clears flag 0x00400000. */
// FUNCTION: WIZ8 0x004566C0
unsigned char W8NavigatorAttachment::TruncatePathAtRadius(const srVector3T<float>* target,
                                                          float radius)
{
    unsigned int index = path_cursor_04;
    float distance = (position_4c[index] - *target).Length();
    /* Retail leaves this slot cold when position_4c[path_cursor_04] is already
       outside the radius, so the interpolation below reads whatever occupied
       the stack. Preserved intentionally. */
    float previous_distance;

    if (distance < radius) {
        do {
            if (path_position_index_08 <= index) {
                break;
            }
            previous_distance = distance;
            ++index;
            distance = (position_4c[index] - *target).Length();
        } while (distance < radius);
    }
    if (distance > radius) {
        float fraction = (distance - radius) / (distance - previous_distance);
        position_1c.x =
            (position_4c[index].x - position_4c[index - 1].x) * fraction + position_4c[index - 1].x;
        position_1c.y =
            (position_4c[index].y - position_4c[index - 1].y) * fraction + position_4c[index - 1].y;
        position_1c.z =
            (position_4c[index].z - position_4c[index - 1].z) * fraction + position_4c[index - 1].z;
        position_4c[index] = position_1c;
        path_position_index_08 = static_cast<unsigned short>(index);
        flags_00 &= ~0x400000;
        return 1;
    }
    flags_00 &= ~0x400000;
    return 0;
}

/* The stored route's total length, measured on first use from position_34
   through every recorded position at or past the current index and cached in
   path_length_058 under the 0x00400000 flag. Entries whose preceding path value
   carries bit 0x2 contribute nothing. */
// FUNCTION: WIZ8 0x00456B00
float W8NavigatorAttachment::MeasurePathLength()
{
    unsigned int index = 1;
    if ((flags_00 & 0x400000) == 0) {
        path_length_058 = 0;
        if (path_cursor_04 > 0) {
            index = path_cursor_04;
        }
        srVector3T<float> previous = position_34;
        for (; index <= path_position_index_08; ++index) {
            if ((path_values_50[index - 1] & 2) == 0) {
                path_length_058 += (position_4c[index] - previous).Length();
            }
            previous = position_4c[index];
        }
        flags_00 |= 0x400000;
    }
    return path_length_058;
}

/* Grow the attachment's parallel route-position and per-position value arrays
   by ten slots. Both arrays retain every entry through the current index. */
// FUNCTION: WIZ8 0x00456BD0
void W8NavigatorAttachment::GrowPathStorage()
{
    unsigned short new_capacity = capacity_0a + 10;
    srVector3T<float>* new_positions =
        static_cast<srVector3T<float>*>(srHeap.allocate(new_capacity * sizeof(srVector3T<float>)));
    unsigned int index;

    for (index = 0; index <= path_position_index_08; ++index) {
        new_positions[index] = position_4c[index];
    }
    srHeap.free(position_4c);
    position_4c = new_positions;

    unsigned short* new_values =
        static_cast<unsigned short*>(malloc(new_capacity * sizeof(unsigned short)));
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
    reactivated_09d = 0;
    navigation_mode_008 = 0;
    flags_00c = 0;
    collision_margin_010 = 0.0;
    movement_target_018.SetZero();
    movement_stopped_024 = 1;
    halted_025 = 0;
    movement_complete_026 = 1;
    position_dirty_09c = 0;
    padding_027 = 0;
    position_028.SetZero();
    position_03c.SetZero();
    unknown_048 = 0;
    linked_navigator_05c = 0;
    unknown_060 = 0;
    unknown_064 = 0;
    target_navigator_04c = 0;
    path_ai_068 = 0;
    minimum_06c = -500.0f;
    maximum_078 = 500.0f;
    movement_0c0.vertical_offset_0c0 = 0.0f;
    active_088 = 1;
    trace_mask_090 = 0;
    target_last_position_050.SetZero();
    minimum_height_034 = 10000.0f;
    maximum_height_038 = 20000.0f;
    movement_0c0.alternate_radius_0b4 = 500.0f;
    movement_0c0.collision_radius_0b0 = 500.0f;
    radius_084 = 500.0f;
    movement_0c0.height_offset_0b8 = 500.0f;
    movement_0c0.secondary_height_offset_0bc = 500.0f;
    movement_0c0.scale_0c4 = 1.0f;
    movement_0c0.position_adjusted_0c8 = 0;
    movement_callback_08c = NavigatorDefaultCallback;
    unknown_094 = 0;
    unknown_098 = 0;
    owned_object_0a0 = 0;
    tracked_distance_0b0 = 500.0f;
    group_linked_0bd = 0;
    tracked_dirty_0b4 = 0;
    unknown_0bc = 0;
    linked_update_time_0b8 = 0;
    movement_0c0.Reset();
    tracked_position_0a4.x = 0.0f;
    node_18c = new srNode(0);
    g_registered_navigators.Add(this);
}

/* The callback a navigator starts with: mark it and stop it dead. */
// FUNCTION: WIZ8 0x00451ea0
void NavigatorDefaultCallback(W8Navigator* navigator)
{
    navigator->halted_025 = 1;
    navigator->movement_0c0.velocity_034.SetZero();
}

/* Switch every registered navigator to the requested link mode. Mode zero
   clears their stop flags and resets their path state, then re-bases any
   monster group already following a navigator onto that navigator's path and
   position; every other mode stops them and clears their velocity. */
// FUNCTION: WIZ8 0x00452F50
void SetNavigatorLinkMode(unsigned char mode)
{
    if (g_navigator_link_mode == mode) {
        return;
    }
    g_navigator_link_mode = mode;
    if (mode == 0) {
        for (int index = 0; index < g_registered_navigators.GetCount(); ++index) {
            W8Navigator* navigator = *g_registered_navigators.GetAt(index);

            navigator->halted_025 = 0;
            if (navigator->path_ai_068 != 0) {
                PathAIResetTick(navigator->path_ai_068);
            }
            if (navigator->group_linked_0bd != 0) {
                W8MonsterInfo* monster_info =
                    MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                        0x4fe, "C:\\Projects\\Wizardry 8\\Engine Code\\Navigator.cpp",
                        navigator->movement_0c0.location_id_004, 1));
                if (monster_info->fActive != 0 && monster_info->monster_group_id != 0) {
                    W8MonsterGroup* group = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                        0x508, "C:\\Projects\\Wizardry 8\\Engine Code\\Navigator.cpp",
                        monster_info->monster_group_id, 1));
                    srVector3T<float> position = navigator->movement_0c0.position_040;

                    if ((group->fInCombat == 0 ||
                         PositionMonsterGroupNearCamera(group, 0.0f, navigator->movement_0c0.yaw,
                                                        0) == 0) &&
                        (MoveMonsterGroupToPosition(group, &position, navigator->movement_0c0.yaw,
                                                    0, 1, 0, 0),
                         g_combat_inactive != 0)) {
                        navigator->linked_update_time_0b8 = 0;
                        g_navigator_group.Clear();
                        navigator->CollectGroupNavigators(&g_navigator_group);
                        for (int group_index = 0; group_index < g_navigator_group.GetCount();
                             ++group_index) {
                            W8Navigator* group_navigator = *g_navigator_group.GetAt(group_index);

                            group_navigator->movement_0c0.attachment_0ac->CopyPathFrom(
                                navigator->movement_0c0.attachment_0ac);
                            group_navigator->position_03c = navigator->position_03c;
                            group_navigator->movement_0c0.attachment_0ac->flags_00 &= 0xff7effff;
                            group_navigator->linked_update_time_0b8 = 0;
                        }
                    }
                }
            }
        }
    } else {
        for (int index = 0; index < g_registered_navigators.GetCount(); ++index) {
            W8Navigator* navigator = *g_registered_navigators.GetAt(index);

            navigator->halted_025 = 1;
            navigator->movement_0c0.velocity_034.SetZero();
            navigator->group_linked_0bd = 0;
        }
    }
}

// FUNCTION: WIZ8 0x00453160
void StopAllNavigators(void)
{
    int count = g_registered_navigators.GetCount();
    for (int index = 0; index < count; ++index) {
        W8Navigator* navigator = *g_registered_navigators.GetAt(index);
        navigator->halted_025 = 1;
        navigator->movement_0c0.velocity_034.SetZero();
    }
}

// FUNCTION: WIZ8 0x004531a0
void ResumeAllNavigators(void)
{
    int count = g_registered_navigators.GetCount();
    for (int index = 0; index < count; ++index) {
        W8Navigator* navigator = *g_registered_navigators.GetAt(index);
        navigator->halted_025 = 0;
        if (navigator->path_ai_068 != 0) {
            PathAIResetTick(navigator->path_ai_068);
        }
        if (navigator->path_ai_068 != 0) {
            PathAIResetTick(navigator->path_ai_068);
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
    path_cursor_04 = 0;
    follow_offset_0c = 0;
    capacity_0a = 10;
    position_4c = static_cast<srVector3T<float>*>(srHeap.allocate(10 * sizeof(srVector3T<float>)));
    path_values_50 = static_cast<unsigned short*>(malloc(capacity_0a * sizeof(unsigned short)));
    memset(path_values_50, 0, capacity_0a * sizeof(unsigned short));
    path_length_058 = 0;
    separation_54 = 0.0f;
    position_10.SetZero();
    position_1c.SetZero();
}

/* The from/to attachment: a ready-made two-position route that starts at the
   first recorded position, with path_length_058 carrying the direct segment length
   until a real path is built over it. */
// FUNCTION: WIZ8 0x00456280
W8NavigatorAttachment::W8NavigatorAttachment(const srVector3T<float>* from,
                                             const srVector3T<float>* to)
{
    flags_00 = 0x2000000;
    path_position_index_08 = 1;
    path_cursor_04 = 1;
    follow_offset_0c = 0;
    capacity_0a = 10;
    position_4c = static_cast<srVector3T<float>*>(srHeap.allocate(10 * sizeof(srVector3T<float>)));
    path_values_50 = static_cast<unsigned short*>(malloc(capacity_0a * sizeof(unsigned short)));
    memset(path_values_50, 0, capacity_0a * sizeof(unsigned short));
    path_length_058 = 0;
    separation_54 = 0.0f;
    position_10 = *from;
    position_4c[0] = *from;
    position_1c = *to;
    position_4c[1] = *to;
    position_34 = position_10;
    path_length_058 = (position_1c - position_10).Length();
}

/* A second pass of defaults over the same tail, run straight after the
   constructor. Where the constructor cleared the orientation, this one gives it
   a facing of three quarter turns, a ten-thousand callback threshold, a turn
   rate of a sixteenth turn, unit scale and speed, and an identity basis in the
   three vectors at +0x88. */
// FUNCTION: WIZ8 0x004573d0
void W8NavigatorMovementState::Reset()
{
    flags_000 = 0;
    flags_06c = 0;
    yaw_velocity_01c = 0.0f;
    pitch_020 = 0.0f;
    target_pitch_024 = 0.0f;
    roll_028 = 0.0f;
    target_roll_02c = 0.0f;
    target_yaw = 4.712389f;
    yaw = 4.712389f;
    velocity_034.SetZero();
    position_040.SetZero();
    callback_progress_05c = 0.0f;
    pitch_enabled_074 = 0;
    roll_enabled_075 = 0;
    vertical_velocity_078 = 0.0f;
    callback_threshold_058 = 10000.0f;
    turn_rate_068 = 0.19634955f;
    boundary_enabled_076 = 1;
    target_location_id_010 = -1;
    movement_scale_060 = 1.0f;
    movement_speed_064 = 1.0f;
    vector_088.Set(1.0f, 0.0f, 0.0f);
    vector_094.Set(0.0f, 1.0f, 0.0f);
    vector_0a0.Set(0.0f, 0.0f, 1.0f);
}

/* The whole 0xCC tail starts cleared apart from a unit movement speed, the byte
   at +0x76, the invalidated target_location_id_010 and the attachment, which the tail
   allocates and owns from construction rather than acquiring later. */
// FUNCTION: WIZ8 0x004572c0
W8NavigatorMovementState::W8NavigatorMovementState()
{
    flags_000 = 0;
    location_id_004 = 0;
    leadership_rank_008 = 0;
    active_rank_00c = 0;
    yaw = 0.0f;
    target_yaw = 0.0f;
    yaw_velocity_01c = 0.0f;
    pitch_020 = 0.0f;
    target_pitch_024 = 0.0f;
    roll_028 = 0.0f;
    target_roll_02c = 0.0f;
    padding_030 = 0.0f;
    velocity_034.SetZero();
    position_040.SetZero();
    target_position_04c.SetZero();
    callback_threshold_058 = 0.0f;
    callback_progress_05c = 0.0f;
    movement_scale_060 = 0.0f;
    movement_speed_064 = 1.0f;
    turn_rate_068 = 0.0f;
    pitch_enabled_074 = 0;
    roll_enabled_075 = 0;
    boundary_enabled_076 = 1;
    vertical_velocity_078 = 0.0f;
    target_location_id_010 = -1;
    vertical_base_07c = 0.0f;
    vertical_amplitude_080 = 0.0f;
    vertical_phase_084 = 0.0f;
    vector_088.SetZero();
    vector_094 = vector_088;
    vector_0a0 = vector_088;
    attachment_0ac = new W8NavigatorAttachment();
    flags_06c = 0;
}

/* Only these eleven fields survive a transfer between navigators; everything
   else in the tail stays whatever the destination already had, and target_location_id_010 is
   invalidated rather than copied. */
// FUNCTION: WIZ8 0x004574d0
void W8NavigatorMovementState::CopySettingsFrom(const W8NavigatorMovementState& other)
{
    flags_000 = other.flags_000;
    leadership_rank_008 = other.leadership_rank_008;
    callback_threshold_058 = other.callback_threshold_058;
    callback_progress_05c = other.callback_progress_05c;
    movement_scale_060 = other.movement_scale_060;
    turn_rate_068 = other.turn_rate_068;
    pitch_enabled_074 = other.pitch_enabled_074;
    roll_enabled_075 = other.roll_enabled_075;
    boundary_enabled_076 = other.boundary_enabled_076;
    vertical_base_07c = other.vertical_base_07c;
    vertical_amplitude_080 = other.vertical_amplitude_080;
    target_location_id_010 = -1;
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
        delete attachment;
    }
    attachment_0ac = 0;
}

/* A copy shares nothing that ties it to the original's navigation. The path is
   DISCARDED rather than cloned or shared, both navigator links are cleared, and
   the owned object is not carried over. The movement tail is default
   constructed, then six trailing radius/height/offset/scale fields are copied
   directly and CopySettingsFrom transfers its separate eleven-field settings
   subset. The outer navigator also carries across its height band, bounding
   corners, radius and callback. The copy allocates its own scene node and
   registers itself, so it is a live navigator from birth. */
// FUNCTION: WIZ8 0x00452220
W8Navigator::W8Navigator(const W8Navigator& other)
{
    flags_00c = 0;
    collision_margin_010 = 0.0;
    movement_target_018.SetZero();
    movement_stopped_024 = 1;
    halted_025 = 0;
    movement_complete_026 = 1;
    padding_027 = 0;
    position_028.SetZero();
    minimum_height_034 = other.minimum_height_034;
    maximum_height_038 = other.maximum_height_038;
    position_03c = other.position_03c;
    unknown_048 = 0;
    target_navigator_04c = 0;
    target_last_position_050.SetZero();
    linked_navigator_05c = 0;
    unknown_060 = 0;
    unknown_064 = 0;
    path_ai_068 = 0;
    radius_084 = other.radius_084;
    active_088 = 1;
    movement_callback_08c = other.movement_callback_08c;
    trace_mask_090 = other.trace_mask_090;
    unknown_094 = other.unknown_094;
    unknown_098 = 0;
    reactivated_09d = other.reactivated_09d;
    owned_object_0a0 = 0;
    tracked_distance_0b0 = other.tracked_distance_0b0;
    group_linked_0bd = other.group_linked_0bd;
    movement_0c0.collision_radius_0b0 = other.movement_0c0.collision_radius_0b0;
    movement_0c0.alternate_radius_0b4 = other.movement_0c0.alternate_radius_0b4;
    movement_0c0.height_offset_0b8 = other.movement_0c0.height_offset_0b8;
    movement_0c0.secondary_height_offset_0bc = other.movement_0c0.secondary_height_offset_0bc;
    movement_0c0.vertical_offset_0c0 = other.movement_0c0.vertical_offset_0c0;
    movement_0c0.scale_0c4 = other.movement_0c0.scale_0c4;
    minimum_06c = other.minimum_06c;
    maximum_078 = other.maximum_078;
    if (g_runtime_world_scale < movement_0c0.collision_radius_0b0) {
        g_runtime_world_scale = movement_0c0.collision_radius_0b0;
    }
    if (g_runtime_world_scale < movement_0c0.alternate_radius_0b4) {
        g_runtime_world_scale = movement_0c0.alternate_radius_0b4;
    }
    if (g_runtime_world_scale < radius_084) {
        g_runtime_world_scale = radius_084;
    }
    movement_0c0.CopySettingsFrom(other.movement_0c0);
    movement_0c0.height_offset_0b8 -= other.movement_0c0.vertical_base_07c;
    movement_0c0.secondary_height_offset_0bc -= other.movement_0c0.vertical_base_07c;
    SetNavigationMode(other.navigation_mode_008);
    position_dirty_09c = 0;
    movement_0c0.position_adjusted_0c8 = 0;
    tracked_dirty_0b4 = 0;
    unknown_0bc = 0;
    linked_update_time_0b8 = 0;
    tracked_position_0a4.SetZero();
    node_18c = SR_NEW(srNode)(static_cast<srNode*>(0));
    node_18c->setLocation(other.node_18c->getLocation());
    g_registered_navigators.Add(this);
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
        DestroyOwnedPathAI(path_ai_068);
    }
    g_registered_navigators.RemoveAt(g_registered_navigators.IndexOf(this));
    delete owned_object_0a0;
    owned_object_0a0 = 0;
    if (movement_0c0.location_id_004 != 0 && g_octree != 0) {
        g_octree->UnregisterLocationObject(movement_0c0.location_id_004, W8_OCTREE_KIND_NAVIGATOR);
    }
    if (node_18c != 0) {
        node_18c->release();
    }
}

// SYNTHETIC: WIZ8 0x00452100
// W8Navigator::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00452E10
bool W8Navigator::IsLinkedToNavigator(W8Navigator* other)
{
    W8Navigator* linked;

    linked = linked_navigator_05c;
    if (linked == 0) {
        return other->linked_navigator_05c == this;
    }
    if (other != linked && other->linked_navigator_05c != linked) {
        return false;
    }
    return true;
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
        path = CreateRecord(0);
        PathAISetAnimated(path, 1);
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
        path = CreateRecord(0);
        PathAISetAnimated(path, 1);
        SetPathAI(path);
        movement_0c0.pitch_enabled_074 = 1;
        movement_0c0.roll_enabled_075 = 0;
        break;
    case 6:
        path = CreateRecord(0);
        PathAISetAnimated(path, 1);
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
void W8Navigator::SetBounds(const srVector3T<float>* minimum, const srVector3T<float>* maximum)
{
    minimum_06c = *minimum;
    maximum_078 = *maximum;
    radius_084 = movement_0c0.alternate_radius_0b4;
}

/* The attachment the octree's navigator-target probe is currently filling in;
   parked globally while PrepareNavigatorTarget runs so the path walk can reach
   it, then cleared. */
// GLOBAL: WIZ8 0x00659BF4
W8NavigatorAttachment* g_active_navigator_attachment;

// FUNCTION: WIZ8 0x004534c0
srVector3T<float> W8Navigator::GetPosition()
{
    return movement_0c0.position_040;
}

// FUNCTION: WIZ8 0x004534f0
void W8Navigator::GetVelocity(srVector3T<float>* velocity)
{
    *velocity = movement_0c0.velocity_034;
}

// FUNCTION: WIZ8 0x00454950
unsigned char W8Navigator::UpdateTrackedPosition()
{
    float distance = (tracked_position_0a4 - movement_0c0.position_040).Length();

    if (distance > tracked_distance_0b0) {
        tracked_position_0a4 = movement_0c0.position_040;
        tracked_dirty_0b4 = 1;
    }
    return tracked_dirty_0b4;
}

// FUNCTION: WIZ8 0x00453880
void W8Navigator::SetMovementStopped()
{
    if (movement_stopped_024 == 0) {
        movement_stopped_024 = 1;
        if (navigation_mode_008 != 5 && navigation_mode_008 != 6) {
            movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
        }
    }
}

/* Save the movement state LoadMovementState consumes: a presence
   byte, then for an ungrouped navigator whose 0x20000000 movement flag is set
   the height bounds, the position and the attachment's segment target. */
// FUNCTION: WIZ8 0x004549d0
unsigned char W8Navigator::SaveMovementState(unsigned int hFile)
{
    unsigned char has_state = 0;
    unsigned char ok;
    srVector3T<float> position;
    srVector3T<float> target;

    if (hFile == 0) {
        return 0;
    }
    if (linked_navigator_05c == 0 && (flags_00c & 0x20000000) != 0) {
        has_state = 1;
        ok = FileWrite(hFile, &has_state, 1, 0);
        ok &= FileWrite(hFile, &minimum_height_034, 4, 0);
        ok &= FileWrite(hFile, &maximum_height_038, 4, 0);
        position = position_03c;
        ok &= FileWrite(hFile, &position, 0xc, 0);
        target = movement_0c0.attachment_0ac->position_1c;
        ok &= FileWrite(hFile, &target, 0xc, 0);
        return ok;
    }
    ok = FileWrite(hFile, &has_state, 1, 0);
    return ok;
}

/* Load the movement state saved by 0x004549D0: a presence byte, the height
   bounds, the position and the movement target. A present state re-primes the
   attachment for a segment toward the saved target, raises the movement flags,
   accepts the target through SetMovementTarget and releases the navigator and
   its group to move again. */
// FUNCTION: WIZ8 0x00454ad0
unsigned char W8Navigator::LoadMovementState(unsigned int hFile)
{
    srVector3T<float> loaded;
    srVector3T<float> target;
    unsigned char has_state;
    unsigned char ok;
    int index;

    ok = FileRead(hFile, &has_state, 1, 0);
    if (has_state == 0) {
        return 0;
    }
    ok &= FileRead(hFile, &minimum_height_034, 4, 0);
    ok &= FileRead(hFile, &maximum_height_038, 4, 0);
    ok &= FileRead(hFile, &loaded, 0xc, 0);
    position_03c = loaded;
    ok &= FileRead(hFile, &loaded, 0xc, 0);
    target = loaded;
    if (ok == 0) {
        return 0;
    }
    movement_0c0.attachment_0ac->InitializeSegment(&movement_0c0.position_040, &target);
    movement_target_018 = target;
    flags_00c |= 0x20000000;
    movement_0c0.attachment_0ac->flags_00 |= 0x800000;
    movement_0c0.target_position_04c = position_03c;
    if (SetMovementTarget(&movement_target_018, 1) == 0) {
        return 0;
    }
    flags_00c |= 0x6;
    movement_stopped_024 = 0;
    if (g_combat_inactive != 0 && linked_navigator_05c == 0) {
        g_navigator_group.Clear();
        CollectGroupNavigators(&g_navigator_group);
        for (index = 0; index < g_navigator_group.GetCount(); ++index) {
            (*g_navigator_group.GetAt(index))->movement_stopped_024 = 0;
        }
    }
    halted_025 = 0;
    movement_target_018 = movement_0c0.attachment_0ac->position_1c;
    return 1;
}

/* Push this navigator's path and position onto every navigator in its group,
   and let the whole group move again if this one may. */
// FUNCTION: WIZ8 0x00454c80
void W8Navigator::PropagateGroupPosition()
{
    if (g_combat_inactive != 0) {
        linked_update_time_0b8 = 0;
        g_navigator_group.Clear();
        CollectGroupNavigators(&g_navigator_group);
        for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
            W8Navigator* navigator = *g_navigator_group.GetAt(index);
            navigator->movement_0c0.attachment_0ac->CopyPathFrom(movement_0c0.attachment_0ac);
            navigator->position_03c = position_03c;
            navigator->movement_0c0.attachment_0ac->flags_00 &= 0xff7effff;
            navigator->linked_update_time_0b8 = 0;
        }
    }
    if (movement_stopped_024 == 0) {
        /* Retail writes the zero the caller already proved; the store is dead
           but faithful. */
        movement_stopped_024 = 0;
        if (g_combat_inactive != 0 && linked_navigator_05c == 0) {
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                (*g_navigator_group.GetAt(index))->movement_stopped_024 = 0;
            }
        }
    }
}

// FUNCTION: WIZ8 0x004538d0
void W8Navigator::SetTargetYaw(float angle)
{
    movement_0c0.target_yaw = NormalizeAngle(angle);
}

// FUNCTION: WIZ8 0x004538f0
void W8Navigator::SetAngles(float angle)
{
    movement_0c0.yaw = NormalizeAngle(angle);
    movement_0c0.target_yaw = NormalizeAngle(angle);
}

// FUNCTION: WIZ8 0x00453920
void W8Navigator::SetTargetPitch(float angle)
{
    movement_0c0.target_pitch_024 = NormalizeAngle(angle);
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

// FUNCTION: WIZ8 0x00453ef0
void W8Navigator::SetHeightRange(float minimum, float maximum)
{
    if (minimum >= g_float_005ebb34) {
        minimum_height_034 = minimum;
    }
    if (maximum >= g_float_005ebb34) {
        maximum_height_038 = maximum;
    }
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
        path_ai_068->position = 0.0f;
        path_ai_068->interpolation_fraction = 0.0f;
        PathAIResetTick(path_ai_068);
    }
}

void W8Navigator::configureStartupRange(float range)
{
    radius_084 = range;
    trace_mask_090 = 1;
    movement_0c0.collision_radius_0b0 = range;
    movement_0c0.alternate_radius_0b4 = range;
}

void W8Navigator::configureStartupDepth(float near_depth, float far_depth)
{
    movement_0c0.height_offset_0b8 = near_depth;
    movement_0c0.secondary_height_offset_0bc = far_depth;
}

// GLOBAL: WIZ8 0x005ec2f4
float g_navigator_default_turn_rate = 4.398229598999023f;

// GLOBAL: WIZ8 0x005ec2f0
float g_navigator_snap_angle = 0.029999999329447746f;
// GLOBAL: WIZ8 0x005ebca4
float g_navigator_mode3_scale = 0.4000000059604645f;
// GLOBAL: WIZ8 0x006081e4
unsigned char g_combat_inactive = 1;

// FUNCTION: WIZ8 0x004526c0
unsigned short W8Navigator::SetMovementTargetToNavigator(W8Navigator* target, double separation)
{
    unsigned short result = 0;

    movement_target_018.SetZero();
    collision_margin_010 = separation;
    target_navigator_04c = target;
    if (target == g_startup_world || target->movement_0c0.location_id_004 != 0) {
        movement_0c0.target_location_id_010 = target->movement_0c0.location_id_004;
    } else {
        movement_0c0.target_location_id_010 = -1;
    }
    PathAIClearOwned(path_ai_068);
    if (g_combat_inactive == 0) {
        movement_0c0.attachment_0ac->flags_00 |= 0x10000;
    }
    if (SetMovementTarget(&target->movement_0c0.position_040, 0) == 0) {
        if (g_combat_inactive == 0) {
            navigation_mode_008 = 0;
            unknown_0bc = 1;
        }
    } else {
        navigation_mode_008 = 5;
        result = 1;
        if (g_combat_inactive == 0) {
            result = static_cast<unsigned short>(movement_0c0.attachment_0ac->flags_00 & 7);
        }
    }
    target_last_position_050 = target->movement_0c0.position_040;
    return result;
}

// FUNCTION: WIZ8 0x00453cc0
bool W8Navigator::StartPatrol(const srVector3T<float>* home, float distance, float variation)
{
    W8Navigator* navigator = this;

    while (navigator->linked_navigator_05c != 0) {
        navigator = navigator->linked_navigator_05c;
    }
    navigator->navigation_mode_008 = 0;
    navigator->position_03c = *home;
    navigator->minimum_height_034 = distance;
    navigator->maximum_height_038 = variation;
    return navigator->ConfigureMovement(distance, variation);
}
// GLOBAL: WIZ8 0x00659c10
unsigned char g_navigator_link_mode;
// GLOBAL: WIZ8 0x005ebc98
float g_navigator_linked_radius_scale = 4.0f;
// GLOBAL: WIZ8 0x005ebcc8
float g_navigator_vertical_phase_step = 0.25f;
// GLOBAL: WIZ8 0x005ec150
extern const double g_double_005ec150 = 500.0;
// GLOBAL: WIZ8 0x006081ec
float g_navigator_minimum_speed = 0.5f;
// GLOBAL: WIZ8 0x006081f0
float g_navigator_minimum_speed_mode23 = 0.8999999761581421f;
/* Runtime scale for the camera-sphere radius in the octree trace resolver;
   the retail image carries link-time 1.0 here. */
// GLOBAL: WIZ8 0x006081f4
float g_float_006081f4 = 1.0f;
// GLOBAL: WIZ8 0x00659bf8
W8GrowableVector<W8Navigator*> g_navigator_group;

/* 0x005EC2A8: a quarter turn, shared with the world elevation helper. */
// GLOBAL: WIZ8 0x005ec2a8
const float g_float_005ec2a8 = 1.57079625f;
// GLOBAL: WIZ8 0x005ec314
static const float NAVIGATOR_THREE_QUARTER_TURN = 4.712389f;
// GLOBAL: WIZ8 0x005ec310
static const float NAVIGATOR_MAXIMUM_DROP = 1500.0f;
// GLOBAL: WIZ8 0x00603acc
float g_navigator_gravity = 187.5f;
// GLOBAL: WIZ8 0x005ec320
static const float NAVIGATOR_LINK_DISTANCE_SQUARED = 400.0f;

// FUNCTION: WIZ8 0x00454d70
void W8Navigator::UpdateLinkedNavigator()
{
    if (linked_navigator_05c == 0) {
        return;
    }
    srVector3T<float> own_position = movement_0c0.position_040;
    srVector3T<float> linked_position = linked_navigator_05c->movement_0c0.position_040;
    srVector3T<float> delta = linked_position - own_position;
    if (delta.LengthSquared() <= radius_084 * radius_084 * NAVIGATOR_LINK_DISTANCE_SQUARED) {
        linked_update_time_0b8 = 0;
        return;
    }
    if (static_cast<W8Monster*>(this)->IsWithinWorldRange() == 0 &&
        static_cast<W8Monster*>(linked_navigator_05c)->IsWithinWorldRange() == 0) {
        UpdateLinkedPosition();
        return;
    }
    int tick = static_cast<int>(g_game_time_accumulator->GetElapsed());
    if (static_cast<unsigned int>(tick - linked_update_time_0b8) <= 50) {
        return;
    }
    linked_update_time_0b8 = tick;
    srVector3T<float> camera;
    GetCameraPosition(&camera);
    if (static_cast<W8Monster*>(linked_navigator_05c)->IsWithinWorldRange() != 0) {
        MonsterGetWorldAnimationBounds(static_cast<W8Monster*>(linked_navigator_05c),
                                       &linked_position, &own_position);
        if (ShowTargetMarker(&camera, &linked_position, &own_position) != 0) {
            goto follow_path;
        }
    }
    if (static_cast<W8Monster*>(this)->IsWithinWorldRange() != 0) {
        MonsterGetWorldAnimationBounds(static_cast<W8Monster*>(this), &linked_position,
                                       &own_position);
        if (ShowTargetMarker(&camera, &linked_position, &own_position) != 0) {
            goto follow_path;
        }
    }
    if (UpdateLinkedPosition() != 0) {
        return;
    }
follow_path:
    if (g_octree->pathing_180->PrepareLinkedNavigator(&movement_0c0) != 0) {
        if (movement_stopped_024 != 0) {
            movement_stopped_024 = 0;
            if (g_combat_inactive != 0 && linked_navigator_05c == 0) {
                g_navigator_group.Clear();
                CollectGroupNavigators(&g_navigator_group);
                for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                    (*g_navigator_group.GetAt(index))->movement_stopped_024 = 0;
                }
            }
        }
    } else {
        if (movement_stopped_024 == 0) {
            movement_stopped_024 = 1;
            if (navigation_mode_008 != 5 && navigation_mode_008 != 6) {
                movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
            }
        }
        AimAtPosition(&camera);
    }
}

// FUNCTION: WIZ8 0x00454fe0
unsigned char W8Navigator::UpdateLinkedPosition()
{
    if (linked_navigator_05c == 0) {
        return 0;
    }
    srVector3T<float> position;
    if (linked_navigator_05c->movement_stopped_024 != 0) {
        if (g_octree->FindNavigatorPosition(&linked_navigator_05c->movement_0c0.position_040,
                                            linked_navigator_05c->movement_0c0.yaw,
                                            movement_0c0.collision_radius_0b0 +
                                                movement_0c0.collision_radius_0b0,
                                            1, &position, 1, 0, 0, 5, 1) == 0) {
            return 0;
        }
        if (movement_stopped_024 == 0) {
            movement_stopped_024 = 1;
            if (navigation_mode_008 != 5 && navigation_mode_008 != 6) {
                movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
            }
        }
    } else {
        position = linked_navigator_05c->movement_0c0.position_040;
        if (movement_stopped_024 != 0) {
            movement_stopped_024 = 0;
        }
    }
    movement_0c0.attachment_0ac->CopyPathFrom(linked_navigator_05c->movement_0c0.attachment_0ac);
    movement_0c0.attachment_0ac->flags_00 &= 0xff7fffff;
    movement_0c0.yaw = linked_navigator_05c->movement_0c0.yaw;
    movement_0c0.velocity_034 = linked_navigator_05c->movement_0c0.velocity_034 * 0.5;
    SetPosition(&position);
    g_octree->QueueOctreeKind13(movement_0c0.location_id_004, &position);
    linked_update_time_0b8 = 0;
    return 1;
}

// FUNCTION: WIZ8 0x00454440
srVector3T<float>* W8Navigator::AdjustPosition00454440(srVector3T<float>* result,
                                                       const srVector3T<float>* current,
                                                       const srVector3T<float>* previous)
{
    float acceleration_scale = 0.25f;
    if (movement_0c0.location_id_004 == 0 || navigation_mode_008 == 4) {
        *result = *current;
        return result;
    }
    srVector3T<float> probe = *current;
    probe.y += g_world_scale;
    unsigned char hit;
    float ground = g_octree->SettleToGround(&probe, &hit, 1, 500.0f);
    if (hit == 0) {
        movement_0c0.velocity_034.SetZero();
        if (g_combat_inactive == 0) {
            ClearMovement();
        }
        *result = *previous;
        return result;
    }
    if (g_octree->current_prop < 0) {
        movement_0c0.position_adjusted_0c8 = 0;
    } else {
        movement_0c0.position_adjusted_0c8 = 1;
        acceleration_scale = 0.5f;
    }
    if (current->y - ground > NAVIGATOR_MAXIMUM_DROP &&
        (previous->x != probe.x || previous->z != probe.z)) {
        movement_0c0.velocity_034.SetZero();
        if (g_combat_inactive == 0) {
            ClearMovement();
        }
        *result = *previous;
        return result;
    }
    if (current->y - ground > g_startup_near_limit) {
        srVector3T<float> falling = *current;
        float distance;
        if (g_combat_inactive == 0) {
            movement_0c0.vertical_velocity_078 += g_game_time_accumulator->GetFrameDelta() *
                                                  g_settings.monster_movement_speed *
                                                  g_navigator_gravity * acceleration_scale;
            distance = movement_0c0.vertical_velocity_078 *
                       g_game_time_accumulator->GetFrameDelta() * g_settings.monster_movement_speed;
        } else {
            movement_0c0.vertical_velocity_078 +=
                g_game_time_accumulator->GetFrameDelta() * g_navigator_gravity * acceleration_scale;
            distance =
                movement_0c0.vertical_velocity_078 * g_game_time_accumulator->GetFrameDelta();
        }
        falling.y -= distance;
        if (falling.y >= ground) {
            position_dirty_09c = 1;
            *result = falling;
            return result;
        }
    }
    movement_0c0.vertical_velocity_078 = 0.0f;
    position_dirty_09c = 0;
    probe.y = ground;
    *result = probe;
    return result;
}

// FUNCTION: WIZ8 0x00454780
void W8Navigator::UpdateFacing(char immediate)
{
    if (movement_0c0.pitch_enabled_074 == 0 && movement_0c0.roll_enabled_075 == 0) {
        return;
    }
    srVector3T<float> forward(0.0f, 0.0f, 1.0f);
    srVector3T<float> normal;
    forward.RotateAboutY(sin(movement_0c0.yaw), cos(movement_0c0.yaw));
    g_octree->GetPathSurfaceNormal00433A70(&movement_0c0.position_040, &normal);
    if (movement_0c0.pitch_enabled_074 != 0) {
        float angle = static_cast<float>(acos(DotProduct(normal, forward)));
        if (angle < g_float_005ec2a8) {
            angle += NAVIGATOR_THREE_QUARTER_TURN;
        } else {
            angle -= g_float_005ec2a8;
        }
        if (immediate != 0) {
            movement_0c0.pitch_020 = NormalizeAngle(angle);
        }
        movement_0c0.target_pitch_024 = NormalizeAngle(angle);
    }
    if (movement_0c0.roll_enabled_075 != 0) {
        srVector3T<float> side(-forward.z, 0.0f, forward.x);
        float angle = static_cast<float>(acos(DotProduct(side, normal)));
        if (angle < g_float_005ec2a8) {
            angle += NAVIGATOR_THREE_QUARTER_TURN;
        } else {
            angle -= g_float_005ec2a8;
        }
        if (immediate != 0) {
            movement_0c0.roll_028 = NormalizeAngle(angle);
        }
        movement_0c0.target_roll_02c = NormalizeAngle(angle);
    }
}

// FUNCTION: WIZ8 0x00453230
W8Navigator* W8Navigator::ResolveBlockingNavigator(const srVector3T<float>* from,
                                                   srVector3T<float>* to,
                                                   unsigned char include_target)
{
    int hit_location;
    int location;

    if (active_088 == 0) {
        return 0;
    }
    if (target_navigator_04c == 0) {
        hit_location = -3;
    } else {
        hit_location = target_navigator_04c->movement_0c0.location_id_004;
    }
    location = -3;
    if (include_target != 0 && target_navigator_04c != 0) {
        location = target_navigator_04c->movement_0c0.location_id_004;
    }
    if (g_octree->ResolveTraceHit(from, to, movement_0c0.location_id_004, &hit_location, location,
                                  trace_mask_090, 0) != 0) {
        if (hit_location == 0) {
            return g_startup_world;
        }
        if (hit_location > 0) {
            W8MonsterInfo* monster_info =
                MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                    0x5d7, "C:\\Projects\\Wizardry 8\\Engine Code\\Navigator.cpp", hit_location,
                    1));
            if (monster_info->p3D != 0) {
                return monster_info->p3D;
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00453300
double W8Navigator::MeasurePathDistance(const srVector3T<float>* target, float max_range,
                                        int location_id)
{
    static W8NavigatorMovementState movement;
    static W8NavigatorAttachment attachment;
    unsigned char ready;

    movement.CopySettingsFrom(movement_0c0);
    movement.target_location_id_010 = location_id;
    movement.vector_088 = movement_0c0.vector_088;
    movement.vector_094 = movement_0c0.vector_094;
    movement.vector_0a0 = movement_0c0.vector_0a0;
    movement.position_040 = movement_0c0.position_040;
    movement.target_position_04c = *target;
    attachment.InitializeSegment(&movement_0c0.position_040, target);
    movement.attachment_0ac = &attachment;
    if (g_combat_inactive == 0) {
        attachment.flags_00 |= 0xc010000;
    } else {
        attachment.flags_00 &= 0xf3feffff;
    }
    ready = g_octree->PrepareNavigatorTarget(&movement, max_range, radius_084);
    movement.attachment_0ac = 0;
    if (ready != 0 && ((attachment.flags_00 & 0x10000) == 0 || (attachment.flags_00 & 7) != 3)) {
        return attachment.MeasurePathLength();
    }
    return -1.0;
}

// FUNCTION: WIZ8 0x00453480
int W8Navigator::FindNavigatorPathDistance(float max_range, float* out_distance)
{
    double distance =
        MeasurePathDistance(&g_startup_world->movement_0c0.position_040, max_range, 0);

    *out_distance = static_cast<float>(distance);
    if (distance != g_negative_one) {
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00453520
void W8Navigator::SetVelocity(const srVector3T<float>* velocity)
{
    movement_0c0.velocity_034 = *velocity;
}

// FUNCTION: WIZ8 0x00453540
unsigned char W8Navigator::CheckNavigatorCollision(const srVector3T<float>* from,
                                                   const srVector3T<float>* to)
{
    W8Navigator* blocker = ResolveBlockingNavigator(from, const_cast<srVector3T<float>*>(to), 0);

    if (blocker != 0 && OnCollision(blocker) && blocker->OnCollision(this)) {
        return 1;
    }
    return 0;
}
/* Re-prime the attachment for a new movement segment: keep only the link flag
   bits, reseed the path cursors at one, copy source and destination into the
   position fields and the first two waypoints, clear the recorded path values
   and store the segment length. */
// FUNCTION: WIZ8 0x004563e0
void W8NavigatorAttachment::InitializeSegment(const srVector3T<float>* source,
                                              const srVector3T<float>* destination)
{
    flags_00 = (flags_00 & 0xc810000) | 0x2000000;
    path_position_index_08 = 1;
    path_cursor_04 = 1;
    path_length_058 = 0;
    separation_54 = 0;
    position_10 = *source;
    position_4c[0] = *source;
    position_1c = *destination;
    position_4c[1] = *destination;
    position_40 = position_10;
    position_34 = position_10;
    follow_offset_0c = 0;
    memset(path_values_50, 0, capacity_0a * sizeof(unsigned short));
    path_length_058 = (position_1c - position_10).Length();
}

// FUNCTION: WIZ8 0x004564f0
void W8NavigatorAttachment::CopyPathFrom(const W8NavigatorAttachment* other)
{
    flags_00 = other->flags_00;
    path_cursor_04 = other->path_cursor_04;
    path_position_index_08 = other->path_position_index_08;
    follow_offset_0c = other->follow_offset_0c;
    path_length_058 = other->path_length_058;
    separation_54 = other->separation_54;
    position_10 = other->position_10;
    position_34 = other->position_10;
    position_1c = other->position_1c;
    if ((flags_00 & 0x80000) != 0) {
        position_28 = other->position_28;
    }
    if (path_position_index_08 + 1 >= capacity_0a) {
        int capacity = (path_position_index_08 / 10 + 1) * 10;
        srVector3T<float>* positions = new srVector3T<float>[capacity];
        delete[] position_4c;
        position_4c = positions;
        unsigned short* values =
            static_cast<unsigned short*>(malloc(capacity * sizeof(unsigned short)));
        free(path_values_50);
        path_values_50 = values;
        capacity_0a = static_cast<unsigned short>(capacity);
    }
    for (unsigned int index = 1; index <= path_position_index_08; ++index) {
        position_4c[index] = other->position_4c[index];
        path_values_50[index] = other->path_values_50[index];
    }
}

// FUNCTION: WIZ8 0x00456660
void W8NavigatorAttachment::GetNextPosition(srVector3T<float>* position)
{
    if ((flags_00 & 0x80000) != 0) {
        *position = position_28;
        return;
    }
    if (path_cursor_04 < path_position_index_08) {
        *position = position_4c[path_cursor_04];
    } else {
        *position = position_1c;
    }
}

// FUNCTION: WIZ8 0x00456830
unsigned char W8NavigatorAttachment::AdvanceAlongPathPositions(float distance,
                                                               srVector3T<float>* position)
{
    srVector3T<float> local;
    srVector3T<float> delta;
    float segment;
    float t;
    unsigned char on_path;

    local = *position;
    on_path = 1;
    /* Retail evaluates both halves of this predicate and writes one either
       way; the check is dead in the recovered body exactly as shipped. */
    if ((flags_00 & 0x10000) == 0 && distance > NAVIGATOR_MAXIMUM_DROP) {
        on_path = 1;
    }
    flags_00 &= 0xffbfffff;
    delta = position_4c[path_cursor_04] - local;
    segment = srVector2T<float>(delta.x, delta.z).Length();
    if (segment < g_double_005ebc30 && path_cursor_04 == path_position_index_08) {
        return 0;
    }
    if (segment < distance) {
        do {
            if (path_position_index_08 < path_cursor_04) {
                break;
            }
            distance -= segment;
            local = position_4c[path_cursor_04];
            ++path_cursor_04;
            /* Retail reads position_4c[path_cursor_04] before the loop head re-tests
               the cursor: when the consumed waypoint was the last one this
               samples one slot past path_position_index_08, inside the
               ten-entry allocation but never initialized. */
            delta = position_4c[path_cursor_04] - local;
            segment = srVector2T<float>(delta.x, delta.z).Length();
        } while (segment < distance);
    }
    if (path_position_index_08 < path_cursor_04) {
        *position = position_1c;
        path_cursor_04 = path_position_index_08;
        on_path = 0;
    } else {
        t = distance / segment;
        *position =
            local * static_cast<float>(g_double_005ebc30 - t) + position_4c[path_cursor_04] * t;
    }
    if (path_cursor_04 > 1) {
        for (position_cursor_06 = 0; path_cursor_04 + position_cursor_06 <= path_position_index_08;
             ++position_cursor_06) {
            position_4c[position_cursor_06 + 1] = position_4c[position_cursor_06 + path_cursor_04];
        }
        path_position_index_08 += 1 - path_cursor_04;
        path_cursor_04 = 1;
    }
    position_4c[0] = *position;
    position_10 = position_4c[0];
    position_34 = position_4c[0];
    return on_path;
}

// FUNCTION: WIZ8 0x00456CB0
unsigned char W8NavigatorAttachment::CheckPositionHopHeight(const srVector3T<float>* position)
{
    int base;
    int end;
    srVector2T<float> point;
    srVector2T<float> from;
    srVector2T<float> to;
    float fraction;
    float distance;
    float from_height;
    float to_height;
    W8PathSurface* surfaces;

    base = path_cursor_04 - 1;
    if (base == 0) {
        base = 1;
    }
    end = base + 1;
    if (path_position_index_08 < end) {
        return 0;
    }
    point.x = position->x;
    point.y = position->z;
    from.x = position_4c[base].x;
    from.y = position_4c[base].z;
    to.x = position_4c[end].x;
    to.y = position_4c[end].z;
    distance = PointToSegmentDistance2D(&point, &from, &to, 0, &fraction);
    surfaces = g_octree->pathing_180->m_pSurfaces_048;
    from_height = (surfaces[path_values_50[base]].flags_00 >> 0xc) * g_world_scale;
    to_height = (surfaces[path_values_50[end]].flags_00 >> 0xc) * g_world_scale;
    if (from_height != to_height) {
        from_height = (to_height - from_height) * fraction + from_height;
    }
    return distance <= from_height;
}

// FUNCTION: WIZ8 0x00456DD0
unsigned char W8NavigatorAttachment::CheckPredictedHopHeight(const srVector3T<float>* position)
{
    srVector2T<float> point;
    srVector2T<float> from;
    srVector2T<float> to;
    float fraction;
    float distance;
    float other_fraction[2];
    float other_distance;
    unsigned int base;
    float from_height;
    float to_height;
    W8PathSurface* surfaces;

    point.x = position->x;
    point.y = position->z;
    from.x = position_4c[path_cursor_04 - 1].x;
    from.y = position_4c[path_cursor_04 - 1].z;
    to.x = position_4c[path_cursor_04].x;
    to.y = position_4c[path_cursor_04].z;
    distance = PointToSegmentDistance2D(&point, &from, &to, 0, &fraction);
    base = path_cursor_04 - 1;
    if (path_cursor_04 < path_position_index_08 && path_values_50[path_cursor_04 + 1] != 0) {
        from.x = to.x;
        from.y = to.y;
        to.x = position_4c[path_cursor_04 + 1].x;
        to.y = position_4c[path_cursor_04 + 1].z;
        other_distance = PointToSegmentDistance2D(&point, &from, &to, 0, other_fraction);
        if (other_distance < distance) {
            base = path_cursor_04;
            fraction = other_fraction[0];
            distance = other_distance;
        }
    }
    surfaces = g_octree->pathing_180->m_pSurfaces_048;
    from_height = (surfaces[path_values_50[base]].flags_00 >> 0xc) * g_world_scale;
    to_height = (surfaces[path_values_50[base + 1]].flags_00 >> 0xc) * g_world_scale;
    if (from_height != to_height) {
        from_height = (to_height - from_height) * fraction + from_height;
    }
    return distance <= from_height;
}

// FUNCTION: WIZ8 0x00456F60
unsigned char W8NavigatorAttachment::AdvancePositionTowardWaypoint(srVector3T<float>* position,
                                                                   float distance)
{
    srVector2T<float> point;
    srVector2T<float> from;
    srVector2T<float> to;
    float dir_x;
    float dir_z;
    float fraction;
    float remainder;
    float segment;
    bool reached;

    point.x = position->x;
    point.y = position->z;
    from.x = position_4c[path_cursor_04 - 1].x;
    from.y = position_4c[path_cursor_04 - 1].z;
    to.x = position_4c[path_cursor_04].x;
    to.y = position_4c[path_cursor_04].z;
    PointToSegmentDistance2D(&point, &from, &to, 1, &fraction);
    dir_x = to.x - from.x;
    dir_z = to.y - from.y;
    remainder = (g_float_005ebb38 - fraction) * srVector2T<float>(dir_x, dir_z).Length();
    if (distance <= remainder) {
        reached = 0;
    } else {
        reached = 1;
        if (path_cursor_04 < path_position_index_08 && path_values_50[path_cursor_04 + 1] != 0) {
            distance -= remainder;
            point.x = to.x;
            point.y = to.y;
            dir_x = position_4c[path_cursor_04 + 1].x - to.x;
            dir_z = position_4c[path_cursor_04 + 1].z - to.y;
            remainder = srVector2T<float>(dir_x, dir_z).Length();
        }
    }
    if (remainder < distance) {
        distance = remainder;
    }
    if (distance <= g_float_005ebb34) {
        position->x = point.x;
        position->z = point.y;
        return reached;
    }
    segment = srVector2T<float>(dir_x, dir_z).Length();
    if (segment != g_zero_005ebb40) {
        distance /= segment;
        dir_x *= distance;
        dir_z *= distance;
    }
    position->x = dir_x + point.x;
    position->z = dir_z + point.y;
    return reached;
}

/* Advances `position` along the recorded route by `distance`, writing the unit
   direction toward the current waypoint into `direction`; one once the final
   waypoint is reached. */
// FUNCTION: WIZ8 0x00457150
unsigned char W8NavigatorAttachment::AdvancePositionWithDirection(srVector3T<float>* position,
                                                                  float distance,
                                                                  srVector3T<float>* direction)
{
    while (path_cursor_04 <= path_position_index_08) {
        if (distance <= g_zero_005ebb40) {
            break;
        }
        srVector3T<float>* waypoint = &position_4c[path_cursor_04];
        direction->x = waypoint->x - position->x;
        direction->y = waypoint->y - position->y;
        direction->z = waypoint->z - position->z;
        float length = direction->Length();
        if (g_zero_005ebb40 < length) {
            float scale = static_cast<float>(g_double_005ebc30 / length);
            direction->x *= scale;
            direction->y *= scale;
            direction->z *= scale;
        }
        if (length <= distance) {
            *position = *waypoint;
            distance -= length;
            if (path_cursor_04 == path_position_index_08) {
                return 1;
            }
            ++path_cursor_04;
        } else {
            position->x = direction->x * distance + position->x;
            position->y = direction->y * distance + position->y;
            position->z = direction->z * distance + position->z;
            distance = static_cast<float>(g_zero_005ebb40);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00452560
void W8Navigator::SetScale(float scale)
{
    float ratio = scale / movement_0c0.scale_0c4;
    movement_0c0.scale_0c4 = scale;
    radius_084 *= ratio;
    movement_0c0.collision_radius_0b0 *= ratio;
    movement_0c0.alternate_radius_0b4 *= ratio;
    movement_0c0.height_offset_0b8 *= ratio;
    movement_0c0.secondary_height_offset_0bc *= ratio;
    movement_0c0.movement_scale_060 *= ratio;
    if (g_runtime_world_scale < movement_0c0.collision_radius_0b0) {
        g_runtime_world_scale = movement_0c0.collision_radius_0b0;
    }
    if (g_runtime_world_scale < movement_0c0.alternate_radius_0b4) {
        g_runtime_world_scale = movement_0c0.alternate_radius_0b4;
    }
    if (g_runtime_world_scale < radius_084) {
        g_runtime_world_scale = radius_084;
    }
}

// FUNCTION: WIZ8 0x00452630
unsigned char W8Navigator::ConfigureMovementToPosition(const srVector3T<float>* position)
{
    flags_00c = 6;
    movement_0c0.target_location_id_010 = -1;
    movement_target_018.SetZero();
    collision_margin_010 = 0.0;
    target_navigator_04c = 0;
    if (movement_stopped_024 != 0) {
        PathAIClearOwned(path_ai_068);
    }
    srVector3T<float> target = *position;
    flags_00c = 6;
    movement_0c0.target_position_04c = target;
    movement_target_018 = target;
    if (movement_0c0.active_rank_00c == 0) {
        movement_0c0.active_rank_00c = movement_0c0.leadership_rank_008;
    }
    return SetMovementTarget(&movement_target_018, 0);
}

// FUNCTION: WIZ8 0x004531f0
void W8Navigator::SetFlag25(char value)
{
    halted_025 = value;
    if (value == 0) {
        if (path_ai_068 != 0) {
            PathAIResetTick(path_ai_068);
        }
    } else {
        movement_0c0.velocity_034.SetZero();
    }
}

// FUNCTION: WIZ8 0x00453ca0
void W8Navigator::SetPitchRollEnabled(char pitch, char roll)
{
    movement_0c0.pitch_enabled_074 = pitch;
    movement_0c0.roll_enabled_075 = roll;
}

/* 0x004527AD is a split address inside this body: it resumes at the
   g_pathing nonnull branch with the call registers still live, not a
   separate authored function. */
// SYNTHETIC: WIZ8 0x004527AD
// W8Navigator::LinkToNavigator mid-body continuation
// FUNCTION: WIZ8 0x004527a0
unsigned char W8Navigator::LinkToNavigator(W8Navigator* target, double separation)
{
    unsigned char result = 0;
    if (g_pathing == 0) {
        return 0;
    }
    movement_target_018.SetZero();
    collision_margin_010 = separation;
    flags_00c = 0;
    target_navigator_04c = target;
    movement_0c0.target_location_id_010 = target->movement_0c0.location_id_004;
    PathAIClearOwned(path_ai_068);
    movement_0c0.attachment_0ac->InitializeSegment(&movement_0c0.position_040,
                                                   &target->movement_0c0.position_040);
    if (g_combat_inactive == 0) {
        movement_0c0.attachment_0ac->flags_00 |= 0x10000;
        if (g_pathing->PlanMovementToPosition(&movement_0c0, &target->movement_0c0.position_040,
                                              radius_084, static_cast<float>(separation)) == 0) {
            unknown_0bc = 1;
            return 0;
        }
        flags_00c = 9;
        target_last_position_050 = target->movement_0c0.position_040;
        result = static_cast<unsigned char>(movement_0c0.attachment_0ac->flags_00 & 7);
        movement_stopped_024 = 0;
        if (g_combat_inactive != 0 && linked_navigator_05c == 0) {
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                (*g_navigator_group.GetAt(index))->movement_stopped_024 = 0;
            }
        }
    } else if (g_octree->LinkNavigatorTarget(&movement_0c0, &target->movement_0c0.position_040,
                                             static_cast<float>(separation)) != 0) {
        flags_00c = 9;
        target_last_position_050 = target->movement_0c0.position_040;
        movement_stopped_024 = 0;
        result = 1;
        if (g_combat_inactive != 0 && linked_navigator_05c == 0) {
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                (*g_navigator_group.GetAt(index))->movement_stopped_024 = 0;
            }
        }
    }
    return result;
}

// FUNCTION: WIZ8 0x004529a0
unsigned short W8Navigator::ConfigureMovementToNavigator(W8Navigator* target, float separation,
                                                         float maximum_distance,
                                                         srVector3T<float> position, int trace_mode,
                                                         float facing, unsigned char* probe_result)
{
    if (g_pathing == 0) {
        return 0;
    }
    if (g_combat_inactive != 0) {
        return SetMovementTargetToNavigator(target, separation);
    }
    movement_0c0.attachment_0ac->flags_00 |= 0x10000;
    movement_target_018.SetZero();
    collision_margin_010 = separation;
    flags_00c = 0x21;
    target_navigator_04c = target;
    movement_0c0.target_location_id_010 = target->movement_0c0.location_id_004;
    movement_0c0.target_position_04c = target->movement_0c0.position_040;
    PathAIClearOwned(path_ai_068);
    radius_084 = movement_0c0.alternate_radius_0b4;
    unsigned short result = g_pathing->ConfigureMovementSearch(
        &movement_0c0, target->movement_0c0.location_id_004, radius_084, separation,
        maximum_distance, position, trace_mode, target->movement_0c0.height_offset_0b8, facing,
        probe_result);
    target_last_position_050 = target->movement_0c0.position_040;
    if (result == 0) {
        if (movement_stopped_024 == 0) {
            movement_stopped_024 = 1;
            if (navigation_mode_008 != 5 && navigation_mode_008 != 6) {
                movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
            }
        }
        PathAIClearOwned(path_ai_068);
        movement_0c0.velocity_034.SetZero();
        if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
            flags_00c &= 0xff000000;
        } else {
            flags_00c = 0;
        }
        movement_0c0.attachment_0ac->RecordPosition(&movement_0c0.position_040);
        g_octree->QueueOctreeKind13(movement_0c0.location_id_004, &movement_0c0.position_040);
        movement_complete_026 = 1;
        flags_00c = 0;
        unknown_0bc = 1;
        return 0;
    }
    if (movement_stopped_024 != 0) {
        movement_complete_026 = 0;
        movement_stopped_024 = 0;
        if (g_combat_inactive != 0 && linked_navigator_05c == 0) {
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                (*g_navigator_group.GetAt(index))->movement_stopped_024 = 0;
            }
        }
    }
    return result;
}

// FUNCTION: WIZ8 0x00452bd0
void W8Navigator::LinkGroupNavigator(W8Navigator* target, double, int)
{
    if (g_combat_inactive != 0) {
        movement_target_018.SetZero();
        collision_margin_010 = 0.0;
        target_navigator_04c = 0;
    }
    linked_navigator_05c = target;
    if (target == 0) {
        if (g_combat_inactive != 0) {
            flags_00c &= 0xfffffdfe;
            if (flags_00c == 0) {
                movement_0c0.attachment_0ac->InitializeSegment(&movement_0c0.position_040,
                                                               &movement_0c0.position_040);
                if (movement_stopped_024 == 0) {
                    movement_stopped_024 = 1;
                    if (navigation_mode_008 != 5 && navigation_mode_008 != 6) {
                        movement_0c0.target_yaw = NormalizeAngle(0.0f);
                    }
                }
            }
        }
    } else {
        movement_0c0.flags_06c |= 2;
        if (g_combat_inactive != 0) {
            flags_00c = (flags_00c & 0xff000201) | 0x201;
            movement_0c0.attachment_0ac->InitializeSegment(&movement_0c0.position_040,
                                                           &target->movement_0c0.position_040);
        }
    }
}

// FUNCTION: WIZ8 0x00452c90
void W8Navigator::ResetMovementAndGroupState()
{
    movement_target_018.SetZero();
    W8Navigator* linked = linked_navigator_05c;
    collision_margin_010 = 0.0;
    target_navigator_04c = 0;
    if (linked != 0) {
        movement_0c0.flags_06c |= 2;
        if (g_combat_inactive == 0) {
            return;
        }
        flags_00c = 0x201;
    } else {
        if (g_combat_inactive == 0) {
            return;
        }
        flags_00c &= 0xfffffdfe;
        if (flags_00c == 0) {
            movement_0c0.attachment_0ac->InitializeSegment(&movement_0c0.position_040,
                                                           &movement_0c0.position_040);
            if (movement_stopped_024 == 0) {
                int mode = navigation_mode_008;
                movement_stopped_024 = 1;
                if (mode != 5 && mode != 6) {
                    movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
                }
            }
        }
        if (g_combat_inactive != 0) {
            linked_update_time_0b8 = 0;
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                W8Navigator* navigator = *g_navigator_group.GetAt(index);
                navigator->movement_0c0.attachment_0ac->CopyPathFrom(movement_0c0.attachment_0ac);
                navigator->position_03c = position_03c;
                navigator->movement_0c0.attachment_0ac->flags_00 &= 0xff7effff;
                navigator->linked_update_time_0b8 = 0;
            }
        }
        if (movement_stopped_024 == 0) {
            movement_stopped_024 = 0;
            if (g_combat_inactive == 0) {
                return;
            }
            if (linked_navigator_05c == 0) {
                g_navigator_group.Clear();
                CollectGroupNavigators(&g_navigator_group);
                for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                    (*g_navigator_group.GetAt(index))->movement_stopped_024 = 0;
                }
            }
        }
    }
    if (g_combat_inactive != 0) {
        movement_0c0.attachment_0ac->flags_00 &= 0xfffeffff;
    }
}

// FUNCTION: WIZ8 0x00453d20
unsigned char W8Navigator::ConfigureMovement(float minimum, float maximum)
{
    flags_00c |= 0x20000000;
    movement_0c0.attachment_0ac->flags_00 |= 0x800000;
    if (minimum > g_float_005ebb34) {
        minimum_height_034 = minimum;
    }
    if (maximum > g_float_005ec2f8) {
        maximum_height_038 = maximum;
    }
    if (minimum_height_034 + g_float_005ec2f8 < maximum_height_038) {
        movement_0c0.target_position_04c = position_03c;
        if (g_octree->PrepareNavigatorPatrol(&movement_0c0, minimum_height_034,
                                             maximum_height_038) != 0) {
            flags_00c |= 6;
            movement_stopped_024 = 0;
            if (g_combat_inactive != 0 && linked_navigator_05c == 0) {
                g_navigator_group.Clear();
                CollectGroupNavigators(&g_navigator_group);
                for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                    (*g_navigator_group.GetAt(index))->movement_stopped_024 = 0;
                }
            }
            halted_025 = 0;
            movement_target_018 = movement_0c0.attachment_0ac->position_1c;
            movement_0c0.attachment_0ac->separation_54 = 0.0f;
            if (g_combat_inactive != 0) {
                linked_update_time_0b8 = 0;
                g_navigator_group.Clear();
                CollectGroupNavigators(&g_navigator_group);
                for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                    W8Navigator* navigator = *g_navigator_group.GetAt(index);
                    navigator->movement_0c0.attachment_0ac->CopyPathFrom(
                        movement_0c0.attachment_0ac);
                    navigator->position_03c = position_03c;
                    navigator->movement_0c0.attachment_0ac->flags_00 &= 0xff7effff;
                    navigator->linked_update_time_0b8 = 0;
                }
            }
            return 1;
        }
        flags_00c &= 0xdffffff9;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00454170
unsigned char W8Navigator::SetMovementTarget(const srVector3T<float>* target, char propagate)
{
    movement_0c0.target_position_04c = *target;
    movement_target_018 = *target;
    unsigned char result;
    if (g_combat_inactive == 0) {
        radius_084 = movement_0c0.alternate_radius_0b4;
        result = g_octree->PrepareNavigatorTarget(&movement_0c0, radius_084,
                                                  static_cast<float>(collision_margin_010));
    } else {
        radius_084 = movement_0c0.collision_radius_0b0;
        if ((flags_00c & 4) != 0 && (flags_00c & 1) != 0) {
            result = g_octree->PrepareNavigatorTarget(&movement_0c0, radius_084,
                                                      target_navigator_04c->radius_084 +
                                                          static_cast<float>(collision_margin_010));
        } else {
            result = g_octree->PrepareNavigatorTarget(&movement_0c0, radius_084,
                                                      static_cast<float>(collision_margin_010));
        }
    }
    if (result == 0 || IsNavigatorAtTarget(&movement_0c0) != 0) {
        radius_084 = movement_0c0.alternate_radius_0b4;
    }
    if (result == 0) {
        if (movement_stopped_024 == 0) {
            movement_stopped_024 = 1;
            if (navigation_mode_008 != 5 && navigation_mode_008 != 6) {
                movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
            }
        }
        PathAIClearOwned(path_ai_068);
        movement_0c0.velocity_034.SetZero();
        if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
            flags_00c &= 0xff000000;
        } else {
            flags_00c = 0;
        }
        movement_0c0.attachment_0ac->RecordPosition(&movement_0c0.position_040);
        g_octree->QueueOctreeKind13(movement_0c0.location_id_004, &movement_0c0.position_040);
        movement_complete_026 = 1;
    } else if (movement_stopped_024 != 0) {
        movement_complete_026 = 0;
        movement_stopped_024 = 0;
        if (g_combat_inactive != 0 && linked_navigator_05c == 0) {
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                (*g_navigator_group.GetAt(index))->movement_stopped_024 = 0;
            }
        }
    }
    if (propagate == 0 && result != 0 && movement_0c0.attachment_0ac != 0 &&
        (movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0 && g_combat_inactive != 0) {
        linked_update_time_0b8 = 0;
        g_navigator_group.Clear();
        CollectGroupNavigators(&g_navigator_group);
        for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
            W8Navigator* navigator = *g_navigator_group.GetAt(index);
            navigator->movement_0c0.attachment_0ac->CopyPathFrom(movement_0c0.attachment_0ac);
            navigator->position_03c = position_03c;
            navigator->movement_0c0.attachment_0ac->flags_00 &= 0xff7effff;
            navigator->linked_update_time_0b8 = 0;
        }
    }
    return result;
}

// FUNCTION: WIZ8 0x00453690
void W8Navigator::AddPathPoint(const srVector3T<float>* position)
{
    if (path_ai_068 == 0) {
        path_ai_068 = CreateRecord(movement_0c0.location_id_004);
        if (path_ai_068 == 0) {
            srAssertFail("pNavAI", "C:\\Projects\\Wizardry 8\\Engine Code\\Navigator.cpp", 0x6c7,
                         0);
        }
    }
    flags_00c &= 0x00ffffff;
    if (flags_00c == 0) {
        flags_00c = 6;
        if (movement_0c0.active_rank_00c == 0) {
            movement_0c0.active_rank_00c = movement_0c0.leadership_rank_008;
        }
        movement_0c0.target_position_04c = *position;
        movement_target_018 = *position;
        movement_stopped_024 = 0;
        if (g_combat_inactive != 0 && linked_navigator_05c == 0) {
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                (*g_navigator_group.GetAt(index))->movement_stopped_024 = 0;
            }
        }
        PathAIAddPoint(path_ai_068, &movement_0c0.position_040);
        SetMovementTarget(&movement_target_018, 0);
    }
    PathAIAddPoint(path_ai_068, position);
}

// FUNCTION: WIZ8 0x004537c0
void W8Navigator::SetObject68Flag38(char value)
{
    if (path_ai_068 != 0) {
        if (value != 0) {
            path_ai_068->looping = 1;
            return;
        }
        path_ai_068->looping = 0;
    }
}

// FUNCTION: WIZ8 0x00453c50
void W8Navigator::SetValue120(float value)
{
    movement_0c0.movement_scale_060 = value;
}

// FUNCTION: WIZ8 0x00453c60
float W8Navigator::GetValue120()
{
    return movement_0c0.movement_scale_060;
}

// FUNCTION: WIZ8 0x00454040
void W8Navigator::SetFacingToward(const srVector3T<float>* target)
{
    srVector3T<float> current = movement_0c0.position_040;
    current.y += movement_0c0.height_offset_0b8;
    if (target->x != current.x || target->y != current.y || target->z != current.z) {
        float angle = GetHeadingAngle(&current, target);
        movement_0c0.yaw = NormalizeAngle(angle);
        movement_0c0.target_yaw = NormalizeAngle(angle);
        if (navigation_mode_008 == 2 || navigation_mode_008 == 3) {
            angle = -GetElevationAngle(&current, target);
            movement_0c0.pitch_020 = NormalizeAngle(angle);
            movement_0c0.target_pitch_024 = NormalizeAngle(angle);
        } else if (navigation_mode_008 == 5 || navigation_mode_008 == 6) {
            UpdateFacing(1);
        }
        if (movement_0c0.location_id_004 != 0) {
            W8MonsterInfo* monster_info =
                MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                    0xa9d, "C:\\Projects\\Wizardry 8\\Engine Code\\Navigator.cpp",
                    movement_0c0.location_id_004, 1));
            if (monster_info->fInCombat != 0) {
                monster_info->pCombat->sight_refresh_pending_151 = 1;
            }
        }
    }
}

// FUNCTION: WIZ8 0x00456020
void W8Navigator::SetPosition(const srVector3T<float>* position)
{
    if (position->x != movement_0c0.position_040.x || position->y != movement_0c0.position_040.y ||
        position->z != movement_0c0.position_040.z) {
        movement_0c0.position_040 = *position;
        srVector3T<double> widened;
        widened.SetFromFloat(position);
        node_18c->setLocation(widened);
        if (movement_0c0.location_id_004 != 0 || this == g_startup_world) {
            g_navigator_position_changed = 1;
        }
        UpdateFacing(1);
        if (movement_0c0.attachment_0ac != 0) {
            *movement_0c0.attachment_0ac->position_4c = *position;
            movement_0c0.attachment_0ac->position_34 = *movement_0c0.attachment_0ac->position_4c;
            movement_0c0.attachment_0ac->position_10 = *movement_0c0.attachment_0ac->position_4c;
        }
    }
    position_dirty_09c = 1;
}

// FUNCTION: WIZ8 0x00453590
void W8Navigator::SetPositionInternal(const srVector3T<float>* position)
{
    srVector3T<double> widened;

    if (position->x != movement_0c0.position_040.x || position->y != movement_0c0.position_040.y ||
        position->z != movement_0c0.position_040.z) {
        movement_0c0.position_040 = *position;
        widened.SetFromFloat(position);
        node_18c->setLocation(widened);
        if (movement_0c0.location_id_004 != 0 || this == g_startup_world) {
            g_navigator_position_changed = 1;
        }
        UpdateFacing(1);
        if (movement_0c0.attachment_0ac != 0) {
            *movement_0c0.attachment_0ac->position_4c = movement_0c0.position_040;
            movement_0c0.attachment_0ac->position_34 = *movement_0c0.attachment_0ac->position_4c;
            movement_0c0.attachment_0ac->position_10 = *movement_0c0.attachment_0ac->position_4c;
        }
    }
}

// FUNCTION: WIZ8 0x004537e0
void W8Navigator::ClearMovement()
{
    if (movement_stopped_024 == 0) {
        movement_stopped_024 = 1;
        if (navigation_mode_008 != 5 && navigation_mode_008 != 6) {
            movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
        }
    }

    PathAIClearOwned(path_ai_068);
    movement_0c0.velocity_034.SetZero();
    if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
        flags_00c &= 0xff000000;
    } else {
        flags_00c = 0;
    }
    movement_0c0.attachment_0ac->RecordPosition(&movement_0c0.position_040);
    g_octree->QueueOctreeKind13(movement_0c0.location_id_004, &movement_0c0.position_040);
    movement_complete_026 = 1;
}

// FUNCTION: WIZ8 0x00453990
void W8Navigator::UpdateAngles()
{
    float step = g_navigator_default_turn_rate;
    float distance;
    float reverse_distance;
    float direction;

    if (gXStatus.fCombatMode == 0) {
        step = movement_0c0.turn_rate_068;
    }
    step *= g_rate * g_game_time_accumulator->GetFrameDelta();

    if (movement_0c0.yaw != movement_0c0.target_yaw) {
        distance = NormalizeAngle(movement_0c0.yaw - movement_0c0.target_yaw);
        reverse_distance = NormalizeAngle(movement_0c0.target_yaw - movement_0c0.yaw);
        direction = g_negative_one;
        if (reverse_distance < distance) {
            distance = reverse_distance;
            direction = g_float_005ebb38;
        }
        if (step <= distance) {
            movement_0c0.yaw = NormalizeAngle(step * direction + movement_0c0.yaw);
        } else {
            movement_0c0.yaw = movement_0c0.target_yaw;
        }
        if (movement_stopped_024 != 0) {
            UpdateFacing(0);
        }
        if (static_cast<float>(fabs(movement_0c0.yaw - movement_0c0.target_yaw)) <
            g_navigator_snap_angle) {
            movement_0c0.yaw = movement_0c0.target_yaw;
        }
    }

    if (navigation_mode_008 == 3) {
        step *= g_navigator_mode3_scale;
    }
    if (movement_0c0.pitch_enabled_074 != 0 &&
        movement_0c0.pitch_020 != movement_0c0.target_pitch_024) {
        distance = NormalizeAngle(movement_0c0.pitch_020 - movement_0c0.target_pitch_024);
        reverse_distance = NormalizeAngle(movement_0c0.target_pitch_024 - movement_0c0.pitch_020);
        direction = g_negative_one;
        if (reverse_distance < distance) {
            distance = reverse_distance;
            direction = g_float_005ebb38;
        }
        if (step <= distance) {
            movement_0c0.pitch_020 = NormalizeAngle(step * direction + movement_0c0.pitch_020);
        } else {
            movement_0c0.pitch_020 = movement_0c0.target_pitch_024;
        }
    }

    if (movement_0c0.roll_enabled_075 != 0 &&
        movement_0c0.roll_028 != movement_0c0.target_roll_02c) {
        distance = NormalizeAngle(movement_0c0.roll_028 - movement_0c0.target_roll_02c);
        reverse_distance = NormalizeAngle(movement_0c0.target_roll_02c - movement_0c0.roll_028);
        direction = g_negative_one;
        if (reverse_distance < distance) {
            distance = reverse_distance;
            direction = g_float_005ebb38;
        }
        if (distance < step) {
            movement_0c0.roll_028 = movement_0c0.target_roll_02c;
            return;
        }
        movement_0c0.roll_028 = NormalizeAngle(step * direction + movement_0c0.roll_028);
    }
}

// FUNCTION: WIZ8 0x00453f30
void W8Navigator::AimAtPosition(const srVector3T<float>* target)
{
    srVector3T<float> current = movement_0c0.position_040;
    current.y += movement_0c0.height_offset_0b8;
    if (target->x != current.x || target->y != current.y || target->z != current.z) {
        movement_0c0.target_yaw = NormalizeAngle(GetHeadingAngle(&current, target));
        if (navigation_mode_008 == 2 || navigation_mode_008 == 3) {
            movement_0c0.target_pitch_024 = NormalizeAngle(-GetElevationAngle(&current, target));
        } else if (navigation_mode_008 == 5 || navigation_mode_008 == 6) {
            UpdateFacing(0);
        }

        if (movement_0c0.location_id_004 != 0) {
            W8MonsterInfo* monster_info =
                MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                    0xa76, "C:\\Projects\\Wizardry 8\\Engine Code\\Navigator.cpp",
                    movement_0c0.location_id_004, 1));
            if (monster_info->fInCombat != 0) {
                monster_info->pCombat->sight_refresh_pending_151 = 1;
            }
        }
    }
}

// FUNCTION: WIZ8 0x00455140
void W8Navigator::CollectGroupNavigators(W8GrowableVector<W8Navigator*>* navigators)
{
    static const char NAVIGATOR_CPP[] = "C:\\Projects\\Wizardry 8\\Engine Code\\Navigator.cpp";
    W8MonsterInfo* monster_info;
    W8MonsterGroup* group;
    unsigned int member;
    int ally;

    if (linked_navigator_05c != 0) {
        return;
    }

    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0xe7b, NAVIGATOR_CPP, movement_0c0.location_id_004, 1));
    group = GetMonsterGroupByListIndex(
        GetMonsterGroupIndexByID(0xe7c, NAVIGATOR_CPP, monster_info->monster_group_id, 1));

    for (member = 0; member < group->member_count; ++member) {
        int location_id = IListGetAt(group->monsters, member);
        if (location_id != movement_0c0.location_id_004) {
            monster_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0xe82, NAVIGATOR_CPP, location_id, 1));
            navigators->Add(monster_info->p3D);
        }
    }

    for (ally = 0; ally < 4; ++ally) {
        if (group->allied_group_ids[ally] != 0) {
            W8MonsterGroup* allied_group = GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(0xe8a, NAVIGATOR_CPP, group->allied_group_ids[ally], 1));
            for (member = 0; member < allied_group->member_count; ++member) {
                monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                    0xe8e, NAVIGATOR_CPP, IListGetAt(allied_group->monsters, member), 1));
                navigators->Add(monster_info->p3D);
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
    bool was_stopped;

    tracked_dirty_0b4 = 0;
    if (position_dirty_09c != 0 || movement_0c0.position_adjusted_0c8 != 0) {
        movement_0c0.position_040 =
            *AdjustPosition00454440(&adjusted, &movement_0c0.position_040, &previous);
    }

    if (movement_0c0.vertical_amplitude_080 != g_float_005ebb34) {
        if (g_navigator_vertical_enabled == 0) {
            movement_0c0.vertical_offset_0c0 = movement_0c0.vertical_base_07c;
        } else {
            float phase =
                movement_0c0.vertical_phase_084 +
                g_rate * g_game_time_accumulator->GetFrameDelta() * g_navigator_vertical_phase_step;
            phase -= static_cast<float>(floor(phase));
            movement_0c0.vertical_phase_084 = phase;
            movement_0c0.vertical_offset_0c0 = static_cast<float>(sin(phase * g_double_005ec318)) *
                                                   movement_0c0.vertical_amplitude_080 +
                                               movement_0c0.vertical_base_07c;
        }
    }

    if (static_cast<signed char>(skip_movement) != 0) {
        return;
    }
    if (g_combat_inactive == 0 && movement_complete_026 != 0) {
        UpdateAngles();
        return;
    }
    if ((flags_00c & 0x200000) != 0) {
        return;
    }
    if (g_navigator_link_mode != 0 && linked_navigator_05c != 0) {
        return;
    }
    if (linked_navigator_05c == 0) {
        movement_0c0.attachment_0ac->flags_00 |= 0x800000;
    }

    if (g_combat_inactive == 0) {
        movement_0c0.movement_speed_064 = g_settings.monster_movement_speed;
        if (slowed != 0) {
            movement_0c0.movement_speed_064 *= g_float_005ebc7c;
        }
    }
    if ((flags_00c & 0x20000000) != 0 && (flags_00c & 0xffffff) == 0) {
        ConfigureMovement(-1.0f, -1.0f);
    }
    if (halted_025 != 0 && g_navigator_link_mode == 0) {
        UpdateAngles();
        return;
    }

    if (movement_0c0.attachment_0ac != 0 &&
        movement_0c0.attachment_0ac->path_cursor_04 >=
            movement_0c0.attachment_0ac->path_position_index_08 &&
        (movement_0c0.attachment_0ac->flags_00 & 0x80000) == 0 &&
        PathAIIsComplete(path_ai_068) != 0) {
        radius_084 = movement_0c0.alternate_radius_0b4;
    }
    if ((flags_00c & 0x100000) != 0) {
        return;
    }

    was_stopped = movement_stopped_024;
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

        movement_result = g_octree->AdvanceNavigator(&movement_0c0, radius_084, 0.0f);
        if (movement_result == 1) {
            if (PathAINextPoint(path_ai_068, &next) != 0) {
                srVector3T<float> target;
                target = next;
                SetMovementTarget(&target, 0);
            } else {
                if (path_ai_068 != 0) {
                    PathAIClearOwned(path_ai_068);
                    path_ai_068 = 0;
                }
                ClearMovement();
                if (movement_kind == 6 && (flags_00c & 0x20000000) != 0) {
                    ConfigureMovement(-1.0f, -1.0f);
                }
            }
        } else if (movement_result == 3) {
            SetMovementTarget(&movement_target_018, 0);
        }
        break;
    }

    case 9:
        if (target_navigator_04c != 0) {
            movement_result = g_octree->AdvanceNavigator(&movement_0c0, radius_084,
                                                         static_cast<float>(collision_margin_010));
            if (movement_result == 1 ||
                (movement_result == 3 &&
                 LinkToNavigator(target_navigator_04c, collision_margin_010) == 0)) {
                if (movement_stopped_024 == 0) {
                    movement_stopped_024 = 1;
                    if (navigation_mode_008 != 5 && navigation_mode_008 != 6) {
                        movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
                    }
                }
                PathAIClearOwned(path_ai_068);
                movement_0c0.velocity_034.SetZero();
                if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
                    flags_00c &= 0xff000000;
                } else {
                    flags_00c = 0;
                }
                movement_0c0.attachment_0ac->RecordPosition(&movement_0c0.position_040);
                g_octree->QueueOctreeKind13(movement_0c0.location_id_004,
                                            &movement_0c0.position_040);
                movement_complete_026 = 1;
            }
        }
        break;

    case 0x201:
        if (linked_navigator_05c != 0 && g_combat_inactive != 0) {
            if (linked_navigator_05c->movement_stopped_024 != 0) {
                srVector3T<float> delta(
                    linked_navigator_05c->movement_0c0.position_040.x - movement_0c0.position_040.x,
                    linked_navigator_05c->movement_0c0.position_040.y - movement_0c0.position_040.y,
                    linked_navigator_05c->movement_0c0.position_040.z -
                        movement_0c0.position_040.z);
                if (delta.Length() < radius_084 * g_navigator_linked_radius_scale +
                                         linked_navigator_05c->radius_084) {
                    if (movement_stopped_024 == 0) {
                        movement_stopped_024 = 1;
                        if (navigation_mode_008 != 5 && navigation_mode_008 != 6) {
                            movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
                        }
                    }
                    break;
                }
            }

            halted_025 = 0;
            if (linked_update_time_0b8 == 0 || movement_stopped_024 == 0) {
                if (movement_stopped_024 != 0) {
                    movement_stopped_024 = 0;
                    if (g_combat_inactive != 0 && linked_navigator_05c == 0) {
                        int index;
                        g_navigator_group.Clear();
                        CollectGroupNavigators(&g_navigator_group);
                        for (index = 0; index < g_navigator_group.GetCount(); ++index) {
                            (*g_navigator_group.GetAt(index))->movement_stopped_024 = 0;
                        }
                    }
                }
                g_octree->AdvanceNavigator(&movement_0c0, radius_084,
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
            if (movement_stopped_024 == 0) {
                movement_stopped_024 = 1;
                if (navigation_mode_008 != 5 && navigation_mode_008 != 6) {
                    movement_0c0.target_pitch_024 = NormalizeAngle(0.0f);
                }
            }
            PathAIClearOwned(path_ai_068);
            movement_0c0.velocity_034.SetZero();
            if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
                flags_00c &= 0xff000000;
            } else {
                flags_00c = 0;
            }
            movement_0c0.attachment_0ac->RecordPosition(&movement_0c0.position_040);
            g_octree->QueueOctreeKind13(movement_0c0.location_id_004, &movement_0c0.position_040);
            movement_complete_026 = 1;
        } else {
            movement_0c0.active_rank_00c = movement_0c0.leadership_rank_008;
            movement_0c0.target_location_id_010 = -1;
            if ((previous_flags & 0xff000000) != 0) {
                flags_00c |= 6;
            }
            SetMovementTarget(&movement_target_018, 0);
        }
    }

    if (was_stopped == 0 || movement_stopped_024 == 0) {
        if (g_combat_inactive == 0 && movement_stopped_024 == 0) {
            if (movement_0c0.target_position_04c.x != movement_0c0.position_040.x ||
                movement_0c0.target_position_04c.y != movement_0c0.position_040.y ||
                movement_0c0.target_position_04c.z != movement_0c0.position_040.z) {
                srVector3T<float> target;
                target = movement_0c0.target_position_04c;
                AimAtPosition(&target);
            }
        }

        movement_0c0.position_040 =
            *AdjustPosition00454440(&adjusted, &movement_0c0.position_040, &previous);
        UpdateFacing(0);
        if (movement_stopped_024 == 0) {
            srVector3T<float> velocity;
            float minimum_speed;

            if (g_combat_inactive == 0) {
                velocity.x = movement_0c0.movement_speed_064 * movement_0c0.movement_scale_060 *
                             g_world_scale;
                velocity.z = 0.0f;
            } else {
                velocity.x = movement_0c0.velocity_034.x;
                velocity.z = movement_0c0.velocity_034.z;
            }
            velocity.y = (movement_0c0.position_040.y - previous.y) /
                         (g_rate * g_game_time_accumulator->GetFrameDelta());
            if (navigation_mode_008 == 2 || navigation_mode_008 == 3) {
                minimum_speed = g_navigator_minimum_speed_mode23;
            } else {
                minimum_speed = g_navigator_minimum_speed;
            }
            movement_0c0.movement_speed_064 =
                velocity.Length() / (movement_0c0.movement_scale_060 * g_world_scale);
            if (movement_0c0.movement_speed_064 < minimum_speed) {
                movement_0c0.movement_speed_064 = minimum_speed;
            }
        }
    }

    UpdateAngles();
    if (movement_stopped_024 == 0 && g_combat_inactive == 0 && movement_complete_026 == 0 &&
        flags_00c != 0) {
        movement_0c0.callback_progress_05c += g_game_time_accumulator->GetFrameDelta() *
                                              movement_0c0.movement_scale_060 * g_rate *
                                              g_world_scale;
        if (movement_0c0.callback_threshold_058 <= movement_0c0.callback_progress_05c) {
            if (movement_callback_08c != 0) {
                movement_callback_08c(this);
            }
            movement_complete_026 = 1;
        }
    }

    {
        if (tracked_distance_0b0 < (tracked_position_0a4 - movement_0c0.position_040).Length()) {
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
    if (g_combat_inactive == 0) {
        int result = g_octree->AdvanceNavigator(&movement_0c0, radius_084,
                                                static_cast<float>(collision_margin_010));
        if (result == 1) {
            ClearMovement();
        }
        return result;
    }

    if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
        float target_motion =
            (target->movement_0c0.position_040 - target_last_position_050).Length();

        if (target_motion > g_double_005ec030 ||
            (target == g_startup_world && target_motion > g_double_005ec150)) {
            target_last_position_050 = target->movement_0c0.position_040;

            if (target->radius_084 + radius_084 + static_cast<float>(collision_margin_010) <
                (target->movement_0c0.position_040 - movement_0c0.position_040).Length()) {
                int index;

                movement_stopped_024 = 0;
                if (linked_navigator_05c == 0) {
                    g_navigator_group.Clear();
                    CollectGroupNavigators(&g_navigator_group);
                    for (index = 0; index < g_navigator_group.GetCount(); ++index) {
                        (*g_navigator_group.GetAt(index))->movement_stopped_024 = 0;
                    }
                }
                halted_025 = 0;
                SetMovementTarget(&target->movement_0c0.position_040, 0);
                flags_00c = 5;
            }
        }

        if (halted_025 == 0 && movement_stopped_024 == 0) {
            int result = g_octree->AdvanceNavigator(&movement_0c0, radius_084,
                                                    static_cast<float>(collision_margin_010));
            if (result != 1) {
                if (result == 3 && SetMovementTarget(&target->movement_0c0.position_040, 0) == 0) {
                    ClearMovement();
                }
                flags_00c |= 5;
                return result;
            }

            SetMovementStopped();
            PathAIClearOwned(path_ai_068);
            movement_0c0.velocity_034.SetZero();
            if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
                flags_00c &= 0xff000000;
            } else {
                flags_00c = 0;
            }
            movement_0c0.attachment_0ac->RecordPosition(&movement_0c0.position_040);
            g_octree->QueueOctreeKind13(movement_0c0.location_id_004, &movement_0c0.position_040);
            flags_00c |= 5;
            movement_complete_026 = 1;
            return 1;
        }
    } else {
        movement_0c0.attachment_0ac->flags_00 &= ~0x10000;
        flags_00c &= 0xff000000;
        SetMovementStopped();
        PathAIClearOwned(path_ai_068);
        movement_0c0.velocity_034.SetZero();
        if ((movement_0c0.attachment_0ac->flags_00 & 0x10000) == 0) {
            flags_00c &= 0xff000000;
        } else {
            flags_00c = 0;
        }
        movement_0c0.attachment_0ac->RecordPosition(&movement_0c0.position_040);
        g_octree->QueueOctreeKind13(movement_0c0.location_id_004, &movement_0c0.position_040);
        movement_complete_026 = 1;
    }
    return 0;
}

/* The W8OctreeTrace methods sit in the link-order gap between Navigator.cpp's
   last body and OctPath.cpp's first. Their identical seed instructions serve
   three source-level roles proven by the callers: initialize a default trace,
   construct a trace over a segment, and reseed an existing trace. */
// FUNCTION: WIZ8 0x00457580
void W8OctreeTrace::Seed(const srVector3T<float>* from, const srVector3T<float>* to)
{
    start_00 = *from;
    end_0c = *to;
    step_18.x = end_0c.x - start_00.x;
    step_18.y = end_0c.y - start_00.y;
    step_18.z = end_0c.z - start_00.z;
    float length = step_18.Length();
    length_28 = length;
    hit_limit_24 = length;
    float scale = static_cast<float>(g_double_005ebc30) / length_28; /* the retail divisor is a
        double constant folded onto a float ray */
    step_18.x *= scale;
    step_18.y *= scale;
    step_18.z *= scale;
    state_2c = 0;
}

// FUNCTION: WIZ8 0x00457640
W8OctreeTrace::W8OctreeTrace(const srVector3T<float>* from, const srVector3T<float>* to)
{
    start_00 = *from;
    end_0c = *to;
    step_18.x = end_0c.x - start_00.x;
    step_18.y = end_0c.y - start_00.y;
    step_18.z = end_0c.z - start_00.z;
    float length = step_18.Length();
    length_28 = length;
    hit_limit_24 = length;
    float scale = static_cast<float>(g_double_005ebc30) / length_28; /* the retail divisor is a
        double constant folded onto a float ray */
    step_18.x *= scale;
    step_18.y *= scale;
    step_18.z *= scale;
    state_2c = 0;
}

// FUNCTION: WIZ8 0x00457700
void W8OctreeTrace::Reseed(const srVector3T<float>* from, const srVector3T<float>* to)
{
    start_00 = *from;
    end_0c = *to;
    step_18.x = end_0c.x - start_00.x;
    step_18.y = end_0c.y - start_00.y;
    step_18.z = end_0c.z - start_00.z;
    float length = step_18.Length();
    length_28 = length;
    hit_limit_24 = length;
    float scale = static_cast<float>(g_double_005ebc30) / length_28; /* the retail divisor is a
        double constant folded onto a float ray */
    step_18.x *= scale;
    step_18.y *= scale;
    step_18.z *= scale;
    state_2c = 0;
}

// FUNCTION: WIZ8 0x004577c0
W8OctreeTrace::W8OctreeTrace()
{
    start_00.SetZero();
    end_0c.SetZero();
    step_18.z = 0.0f;
    step_18.y = 0.0f;
    step_18.x = 0.0f;
    length_28 = 0.0f;
    state_2c = 0;
    hit_limit_24 = 1.0e20f;
}

// FUNCTION: WIZ8 0x00453c70
void W8Navigator::SetMonsterTurnSpeed(float speed)
{
    movement_0c0.callback_threshold_058 = speed * g_world_scale;
}
