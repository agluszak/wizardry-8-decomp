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

// STRING: WIZ8 0x006081fc
#define NAVIGATOR_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Navigator.cpp"
// GLOBAL: WIZ8 0x005ec2f8
const float g_float_005ec2f8 = 5000.0f;
// GLOBAL: WIZ8 0x005ec030
const double g_double_005ec030 = 2500.0;

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
    flags |= W8_NAV_ATTACHMENT_POSITION_RECORDED;
    position6 = *position;
}

/* Trims the recorded route to the boundary of the `radius` sphere around
   `target`: walks stored positions while they stay inside, then lerps the
   crossing point into position1 and over the first outside waypoint, moves
   path_position_index there, and clears flag 0x00400000. */
// FUNCTION: WIZ8 0x004566C0
unsigned char W8NavigatorAttachment::TruncatePathAtRadius(const srVector3T<float>* target,
                                                          float radius)
{
    unsigned int index = path_cursor;
    float distance = (position7[index] - *target).Length();
    /* Retail leaves this slot cold when position7[path_cursor] is already
       outside the radius, so the interpolation below reads whatever occupied
       the stack. Preserved intentionally. */
    float previous_distance;

    if (distance < radius) {
        do {
            if (path_position_index <= index) {
                break;
            }
            previous_distance = distance;
            ++index;
            distance = (position7[index] - *target).Length();
        } while (distance < radius);
    }
    if (distance > radius) {
        float fraction = (distance - radius) / (distance - previous_distance);
        position1.Set(
            (position7[index].x - position7[index - 1].x) * fraction + position7[index - 1].x,
            (position7[index].y - position7[index - 1].y) * fraction + position7[index - 1].y,
            (position7[index].z - position7[index - 1].z) * fraction +
                position7[index - 1].z);
        position7[index] = position1;
        path_position_index = static_cast<unsigned short>(index);
        flags &= ~W8_NAV_ATTACHMENT_PATH_LENGTH_CACHED;
        return 1;
    }
    flags &= ~W8_NAV_ATTACHMENT_PATH_LENGTH_CACHED;
    return 0;
}

/* The stored route's total length, measured on first use from position4
   through every recorded position at or past the current index and cached in
   path_length under the 0x00400000 flag. Entries whose preceding path value
   carries bit 0x2 contribute nothing. */
// FUNCTION: WIZ8 0x00456B00
float W8NavigatorAttachment::MeasurePathLength()
{
    unsigned int index = 1;
    if ((flags & W8_NAV_ATTACHMENT_PATH_LENGTH_CACHED) == 0) {
        path_length = 0;
        if (path_cursor > 0) {
            index = path_cursor;
        }
        srVector3T<float> previous = position4;
        for (; index <= path_position_index; ++index) {
            if ((path_values[index - 1] & 2) == 0) {
                path_length += (position7[index] - previous).Length();
            }
            previous = position7[index];
        }
        flags |= W8_NAV_ATTACHMENT_PATH_LENGTH_CACHED;
    }
    return path_length;
}

/* Grow the attachment's parallel route-position and per-position value arrays
   by ten slots. Both arrays retain every entry through the current index. */
// FUNCTION: WIZ8 0x00456BD0
void W8NavigatorAttachment::GrowPathStorage()
{
    int new_capacity = capacity + 10;
    srVector3T<float>* new_positions = new srVector3T<float>[new_capacity];
    unsigned int index;

    for (index = 0; index <= path_position_index; ++index) {
        new_positions[index] = position7[index];
    }
    delete[] position7;
    position7 = new_positions;

    unsigned short* new_values =
        static_cast<unsigned short*>(malloc(new_capacity * sizeof(unsigned short)));
    memset(new_values, 0, new_capacity * sizeof(unsigned short));
    for (index = 0; index <= path_position_index; ++index) {
        new_values[index] = path_values[index];
    }
    free(path_values);
    path_values = new_values;
    capacity = static_cast<unsigned short>(new_capacity);
}

// FUNCTION: WIZ8 0x00451ec0
W8Navigator::W8Navigator() : reactivated(0)
{
    navigation_mode = 0;
    flags = 0;
    collision_margin = 0.0;
    movement_target.SetZero();
    movement_stopped = 1;
    halted = 0;
    movement_complete = 1;
    position_dirty = 0;
    unknown_027 = 0;
    position2.SetZero();
    position5.SetZero();
    unknown_048 = 0;
    linked_navigator = 0;
    unknown_060 = 0;
    unknown_064 = 0;
    target_navigator = 0;
    path_ai = 0;
    minimum = -500.0f;
    maximum = 500.0f;
    movement.vertical_offset = 0.0f;
    active = 1;
    trace_mask = 0;
    target_last_position.SetZero();
    minimum_height = 10000.0f;
    maximum_height = 20000.0f;
    movement.alternate_radius = 500.0f;
    movement.collision_radius = 500.0f;
    radius = 500.0f;
    movement.height_offset = 500.0f;
    movement.secondary_height_offset = 500.0f;
    movement.scale = 1.0f;
    movement.position_adjusted = 0;
    movement_callback = NavigatorDefaultCallback;
    unknown_094 = 0;
    unknown_098 = 0;
    owned_object = 0;
    tracked_distance = 500.0f;
    group_linked = 0;
    tracked_dirty = 0;
    movement_plan_failed = 0;
    linked_update_time = 0;
    movement.Reset();
    tracked_position.SetZero();
    g_registered_navigators.Add(this);
    /* 0x004520A4 re-zeroes the movement target, the eight-byte collision margin
       and the target link immediately before the node allocation, so the retail
       constructor initialises that group twice. */
    movement_target.SetZero();
    collision_margin = 0.0;
    target_navigator = 0;
    node = new srNode(0);
}

/* The callback a navigator starts with: mark it and stop it dead. */
// FUNCTION: WIZ8 0x00451ea0
void NavigatorDefaultCallback(W8Navigator* navigator)
{
    navigator->halted = 1;
    navigator->movement.velocity.SetZero();
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

            navigator->halted = 0;
            if (navigator->path_ai != 0) {
                PathAIResetTick(navigator->path_ai);
            }
            if (navigator->group_linked != 0) {
                W8MonsterInfo* monster_info =
                    MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                        0x4fe, NAVIGATOR_CPP, navigator->movement.location_id, 1));
                if (monster_info->fActive != 0 && monster_info->monster_group_id != 0) {
                    W8MonsterGroup* group = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                        0x508, NAVIGATOR_CPP, monster_info->monster_group_id, 1));
                    srVector3T<float> position = navigator->movement.position;

                    if ((group->fInCombat == 0 ||
                         PositionMonsterGroupNearCamera(group, 0.0f, navigator->movement.yaw,
                                                        0) == 0) &&
                        (MoveMonsterGroupToPosition(group, &position, navigator->movement.yaw,
                                                    0, 1, 0, 0),
                         g_combat_inactive != 0)) {
                        navigator->linked_update_time = 0;
                        g_navigator_group.Clear();
                        navigator->CollectGroupNavigators(&g_navigator_group);
                        for (int group_index = 0; group_index < g_navigator_group.GetCount();
                             ++group_index) {
                            W8Navigator* group_navigator = *g_navigator_group.GetAt(group_index);

                            group_navigator->movement.attachment->CopyPathFrom(
                                navigator->movement.attachment);
                            group_navigator->position5 = navigator->position5;
                            group_navigator->movement.attachment->flags &= 0xff7effff;
                            group_navigator->linked_update_time = 0;
                        }
                    }
                }
            }
        }
    } else {
        for (int index = 0; index < g_registered_navigators.GetCount(); ++index) {
            W8Navigator* navigator = *g_registered_navigators.GetAt(index);

            NavigatorDefaultCallback(navigator);
            navigator->group_linked = 0;
        }
    }
}

// FUNCTION: WIZ8 0x00453160
void StopAllNavigators(void)
{
    int count = g_registered_navigators.GetCount();
    for (int index = 0; index < count; ++index) {
        W8Navigator* navigator = *g_registered_navigators.GetAt(index);
        NavigatorDefaultCallback(navigator);
    }
}

// FUNCTION: WIZ8 0x004531a0
void ResumeAllNavigators(void)
{
    int count = g_registered_navigators.GetCount();
    for (int index = 0; index < count; ++index) {
        W8Navigator* navigator = *g_registered_navigators.GetAt(index);
        navigator->halted = 0;
        if (navigator->path_ai != 0) {
            PathAIResetTick(navigator->path_ai);
        }
        if (navigator->path_ai != 0) {
            PathAIResetTick(navigator->path_ai);
        }
    }
}

/* The attachment owns two allocations from construction: ten srVector3T<float>
   of recorded positions from srHeap, and a zeroed twenty-byte record. */
// FUNCTION: WIZ8 0x00456210
W8NavigatorAttachment::W8NavigatorAttachment()
{
    flags = 0;
    path_position_index = 0;
    path_cursor = 0;
    follow_offset = 0;
    capacity = 10;
    position7 = new srVector3T<float>[10];
    path_values = static_cast<unsigned short*>(malloc(capacity * sizeof(unsigned short)));
    memset(path_values, 0, capacity * sizeof(unsigned short));
    path_length = 0;
    separation = 0.0f;
    position0.SetZero();
    position1.SetZero();
}

/* The from/to attachment: a ready-made two-position route that starts at the
   first recorded position, with path_length carrying the direct segment length
   until a real path is built over it. */
// FUNCTION: WIZ8 0x00456280
W8NavigatorAttachment::W8NavigatorAttachment(const srVector3T<float>* from,
                                             const srVector3T<float>* to)
{
    flags = W8_NAV_ATTACHMENT_POSITION_RECORDED;
    path_position_index = 1;
    path_cursor = 1;
    follow_offset = 0;
    capacity = 10;
    position7 = new srVector3T<float>[10];
    path_values = static_cast<unsigned short*>(malloc(capacity * sizeof(unsigned short)));
    memset(path_values, 0, capacity * sizeof(unsigned short));
    path_length = 0;
    separation = 0.0f;
    position0 = *from;
    position7[0] = *from;
    position1 = *to;
    position7[1] = *to;
    position4 = position0;
    path_length = (position1 - position0).Length();
}

/* A second pass of defaults over the same tail, run straight after the
   constructor. Where the constructor cleared the orientation, this one gives it
   a facing of three quarter turns, a ten-thousand callback threshold, a turn
   rate of a sixteenth turn, unit scale and speed, and an identity basis in the
   three vectors at +0x88. */
// FUNCTION: WIZ8 0x004573d0
void W8NavigatorMovementState::Reset()
{
    movement_flags = 0;
    flags = 0;
    yaw_velocity = 0.0f;
    pitch = 0.0f;
    target_pitch = 0.0f;
    roll = 0.0f;
    target_roll = 0.0f;
    target_yaw = 4.712389f;
    yaw = 4.712389f;
    velocity.SetZero();
    position.SetZero();
    callback_progress = 0.0f;
    pitch_enabled = 0;
    roll_enabled = 0;
    vertical_velocity = 0.0f;
    callback_threshold = 10000.0f;
    turn_rate = 0.19634953f;
    boundary_enabled = 1;
    target_location_id = -1;
    movement_scale = 1.0f;
    movement_speed = 1.0f;
    basis.SetIdentity();
}

/* The whole 0xCC tail starts cleared apart from a unit movement speed, the byte
   at +0x76, the invalidated target_location_id and the attachment, which the tail
   allocates and owns from construction rather than acquiring later. */
// FUNCTION: WIZ8 0x004572c0
W8NavigatorMovementState::W8NavigatorMovementState()
{
    movement_flags = 0;
    location_id = 0;
    leadership_rank = 0;
    active_rank = 0;
    yaw = 0.0f;
    target_yaw = 0.0f;
    yaw_velocity = 0.0f;
    pitch = 0.0f;
    target_pitch = 0.0f;
    roll = 0.0f;
    target_roll = 0.0f;
    unknown_030 = 0.0f;
    velocity.SetZero();
    position.SetZero();
    target_position.SetZero();
    callback_threshold = 0.0f;
    callback_progress = 0.0f;
    movement_scale = 0.0f;
    movement_speed = 1.0f;
    turn_rate = 0.0f;
    pitch_enabled = 0;
    roll_enabled = 0;
    boundary_enabled = 1;
    vertical_velocity = 0.0f;
    target_location_id = -1;
    vertical_base = 0.0f;
    vertical_amplitude = 0.0f;
    vertical_phase = 0.0f;
    basis.vectors[0].SetZero();
    basis.vectors[1] = basis.vectors[0];
    basis.vectors[2] = basis.vectors[0];
    attachment = new W8NavigatorAttachment();
    flags = 0;
}

/* Only these eleven fields survive a transfer between navigators; everything
   else in the tail stays whatever the destination already had, and target_location_id is
   invalidated rather than copied. */
// FUNCTION: WIZ8 0x004574d0
void W8NavigatorMovementState::CopySettingsFrom(const W8NavigatorMovementState& other)
{
    movement_flags = other.movement_flags;
    leadership_rank = other.leadership_rank;
    callback_threshold = other.callback_threshold;
    callback_progress = other.callback_progress;
    movement_scale = other.movement_scale;
    turn_rate = other.turn_rate;
    pitch_enabled = other.pitch_enabled;
    roll_enabled = other.roll_enabled;
    boundary_enabled = other.boundary_enabled;
    vertical_base = other.vertical_base;
    vertical_amplitude = other.vertical_amplitude;
    target_location_id = -1;
}

/* The attachment destructor releases its two allocations before deletion. */
// FUNCTION: WIZ8 0x00457530
W8NavigatorMovementState::~W8NavigatorMovementState()
{
    delete attachment;
    attachment = 0;
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
    : flags(0), collision_margin(0.0), movement_target(0.0f, 0.0f, 0.0f),
      movement_stopped(true), halted(false), movement_complete(true), unknown_027(0),
      position2(0.0f, 0.0f, 0.0f), minimum_height(other.minimum_height),
      maximum_height(other.maximum_height), position5(other.position5),
      unknown_048(0), target_navigator(0), target_last_position(0.0f, 0.0f, 0.0f),
      linked_navigator(0), unknown_060(0), unknown_064(0), path_ai(0),
      radius(other.radius), active(true),
      movement_callback(other.movement_callback), trace_mask(other.trace_mask),
      unknown_094(other.unknown_094), unknown_098(0), reactivated(other.reactivated),
      owned_object(0), tracked_distance(other.tracked_distance),
      group_linked(other.group_linked)
{
    movement.collision_radius = other.movement.collision_radius;
    movement.alternate_radius = other.movement.alternate_radius;
    configureStartupDepth(other.movement.height_offset,
                          other.movement.secondary_height_offset);
    movement.vertical_offset = other.movement.vertical_offset;
    movement.scale = other.movement.scale;
    minimum = other.minimum;
    maximum = other.maximum;
    if (g_runtime_world_scale < movement.collision_radius) {
        g_runtime_world_scale = movement.collision_radius;
    }
    if (g_runtime_world_scale < movement.alternate_radius) {
        g_runtime_world_scale = movement.alternate_radius;
    }
    if (g_runtime_world_scale < radius) {
        g_runtime_world_scale = radius;
    }
    movement.CopySettingsFrom(other.movement);
    movement.height_offset -= other.movement.vertical_base;
    movement.secondary_height_offset -= other.movement.vertical_base;
    SetNavigationMode(other.navigation_mode);
    position_dirty = 0;
    movement.position_adjusted = 0;
    tracked_dirty = 0;
    movement_plan_failed = 0;
    linked_update_time = 0;
    tracked_position.SetZero();
    node = SR_NEW(srNode)(static_cast<srNode*>(0));
    node->setLocation(other.node->getLocation());
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
    if (path_ai != 0) {
        DestroyOwnedPathAI(path_ai);
    }
    g_registered_navigators.Remove(this);
    delete owned_object;
    owned_object = 0;
    if (movement.location_id != 0 && g_octree != 0) {
        g_octree->UnregisterLocationObject(movement.location_id, W8_OCTREE_KIND_NAVIGATOR);
    }
    if (node != 0) {
        node->release();
    }
}

// FUNCTION: WIZ8 0x00452E10
bool W8Navigator::IsLinkedToNavigator(W8Navigator* other)
{
    W8Navigator* linked;

    linked = linked_navigator;
    if (linked == 0) {
        return other->linked_navigator == this;
    }
    if (other != linked && other->linked_navigator != linked) {
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

    navigation_mode = mode;
    switch (mode) {
    case 1:
        path = CreateRecord(0);
        PathAISetAnimated(path, 1);
        SetPathAI(path);
        /* Falls into mode four's body: the retail block ends where mode four's
           jump-table entry lands. */
    case 4:
        SetPitchRollEnabled(0, 0);
        break;
    case 2:
    case 3:
    case 5:
        path = CreateRecord(0);
        PathAISetAnimated(path, 1);
        SetPathAI(path);
        SetPitchRollEnabled(1, 0);
        break;
    case 6:
        path = CreateRecord(0);
        PathAISetAnimated(path, 1);
        SetPathAI(path);
        SetPitchRollEnabled(1, 1);
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
    this->minimum = *minimum;
    this->maximum = *maximum;
    radius = movement.alternate_radius;
}

/* The attachment the octree's navigator-target probe is currently filling in;
   parked globally while PrepareNavigatorTarget runs so the path walk can reach
   it, then cleared. */
// GLOBAL: WIZ8 0x00659BF4
W8NavigatorAttachment* g_active_navigator_attachment;

// FUNCTION: WIZ8 0x004534c0
srVector3T<float> W8Navigator::GetPosition()
{
    return movement.position;
}

// FUNCTION: WIZ8 0x004534f0
void W8Navigator::GetVelocity(srVector3T<float>* velocity)
{
    *velocity = movement.velocity;
}

// FUNCTION: WIZ8 0x00454950
unsigned char W8Navigator::UpdateTrackedPosition()
{
    float distance = (tracked_position - movement.position).Length();

    if (distance > tracked_distance) {
        tracked_position = movement.position;
        tracked_dirty = 1;
    }
    return tracked_dirty;
}

// FUNCTION: WIZ8 0x00453880
void W8Navigator::SetMovementStopped()
{
    if (movement_stopped == 0) {
        movement_stopped = 1;
        if (navigation_mode != 5 && navigation_mode != 6) {
            movement.target_pitch = NormalizeAngle(0.0f);
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
    if (linked_navigator == 0 && (flags & 0x20000000) != 0) {
        has_state = 1;
        ok = FileWrite(hFile, &has_state, 1, 0);
        ok &= FileWrite(hFile, &minimum_height, 4, 0);
        ok &= FileWrite(hFile, &maximum_height, 4, 0);
        position = position5;
        ok &= FileWrite(hFile, &position, 0xc, 0);
        target = movement.attachment->position1;
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
    ok &= FileRead(hFile, &minimum_height, 4, 0);
    ok &= FileRead(hFile, &maximum_height, 4, 0);
    ok &= FileRead(hFile, &loaded, 0xc, 0);
    position5 = loaded;
    ok &= FileRead(hFile, &loaded, 0xc, 0);
    target = loaded;
    if (ok == 0) {
        return 0;
    }
    movement.attachment->InitializeSegment(&movement.position, &target);
    movement_target = target;
    flags |= 0x20000000;
    movement.attachment->flags |= 0x800000;
    movement.target_position = position5;
    if (SetMovementTarget(&movement_target, 1) == 0) {
        return 0;
    }
    flags |= 0x6;
    movement_stopped = 0;
    if (g_combat_inactive != 0 && linked_navigator == 0) {
        g_navigator_group.Clear();
        CollectGroupNavigators(&g_navigator_group);
        for (index = 0; index < g_navigator_group.GetCount(); ++index) {
            (*g_navigator_group.GetAt(index))->movement_stopped = 0;
        }
    }
    halted = 0;
    movement_target = movement.attachment->position1;
    return 1;
}

/* Push this navigator's path and position onto every navigator in its group,
   and let the whole group move again if this one may. */
// FUNCTION: WIZ8 0x00454c80
void W8Navigator::PropagateGroupPosition()
{
    if (g_combat_inactive != 0) {
        linked_update_time = 0;
        g_navigator_group.Clear();
        CollectGroupNavigators(&g_navigator_group);
        for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
            W8Navigator* navigator = *g_navigator_group.GetAt(index);
            navigator->movement.attachment->CopyPathFrom(movement.attachment);
            navigator->position5 = position5;
            navigator->movement.attachment->flags &= 0xff7effff;
            navigator->linked_update_time = 0;
        }
    }
    if (movement_stopped == 0) {
        /* Retail writes the zero the caller already proved; the store is dead
           but faithful. */
        movement_stopped = 0;
        if (g_combat_inactive != 0 && linked_navigator == 0) {
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                (*g_navigator_group.GetAt(index))->movement_stopped = 0;
            }
        }
    }
}

// FUNCTION: WIZ8 0x004538d0
void W8Navigator::SetTargetYaw(float angle)
{
    movement.target_yaw = NormalizeAngle(angle);
}

// FUNCTION: WIZ8 0x004538f0
void W8Navigator::SetAngles(float angle)
{
    movement.yaw = NormalizeAngle(angle);
    movement.target_yaw = NormalizeAngle(angle);
}

// FUNCTION: WIZ8 0x00453920
void W8Navigator::SetTargetPitch(float angle)
{
    movement.target_pitch = NormalizeAngle(angle);
}

// FUNCTION: WIZ8 0x00453940
void W8Navigator::SetPitch(float pitch)
{
    movement.pitch = NormalizeAngle(pitch);
    movement.target_pitch = NormalizeAngle(pitch);
}

// FUNCTION: WIZ8 0x00453970
float W8Navigator::GetYaw()
{
    return movement.yaw;
}

// FUNCTION: WIZ8 0x00453980
float W8Navigator::GetPitch()
{
    return movement.pitch;
}

// FUNCTION: WIZ8 0x00453c90
void W8Navigator::SetTurnRate(float turn_rate)
{
    movement.turn_rate = turn_rate;
}

// FUNCTION: WIZ8 0x00453ef0
void W8Navigator::SetHeightRange(float minimum, float maximum)
{
    if (minimum >= g_float_zero) {
        minimum_height = minimum;
    }
    if (maximum >= g_float_zero) {
        maximum_height = maximum;
    }
}

// FUNCTION: WIZ8 0x004538b0
void W8Navigator::SetPathAI(W8PathAI* path_ai)
{
    this->path_ai = path_ai;
}

// FUNCTION: WIZ8 0x004538c0
W8PathAI* W8Navigator::GetPathAI()
{
    return path_ai;
}

// FUNCTION: WIZ8 0x00454930
void W8Navigator::ResetPathAI()
{
    if (path_ai != 0) {
        path_ai->position = 0.0f;
        path_ai->interpolation_fraction = 0.0f;
        PathAIResetTick(path_ai);
    }
}

// GLOBAL: WIZ8 0x005ec2f4
const float g_navigator_default_turn_rate = 4.398229598999023f;

// GLOBAL: WIZ8 0x005ec2f0
const float g_navigator_snap_angle = 0.029999999329447746f;
// GLOBAL: WIZ8 0x005ebca4
const float g_navigator_mode3_scale = 0.4000000059604645f;
// GLOBAL: WIZ8 0x006081e4
unsigned char g_combat_inactive = 1;

// FUNCTION: WIZ8 0x004526c0
unsigned short W8Navigator::SetMovementTargetToNavigator(W8Navigator* target, double separation)
{
    unsigned short result = 0;

    movement_target.SetZero();
    collision_margin = separation;
    target_navigator = target;
    if (target == g_startup_world || target->movement.location_id != 0) {
        movement.target_location_id = target->movement.location_id;
    } else {
        movement.target_location_id = -1;
    }
    PathAIClearOwned(path_ai);
    if (g_combat_inactive == 0) {
        movement.attachment->flags |= W8_NAV_ATTACHMENT_FOLLOW_PATH;
    }
    if (SetMovementTarget(&target->movement.position, 0) == 0) {
        if (g_combat_inactive == 0) {
            flags = 0;
            movement_plan_failed = 1;
        }
    } else {
        flags = 5;
        result = 1;
        if (g_combat_inactive == 0) {
            result = static_cast<unsigned short>(movement.attachment->flags & 7);
        }
    }
    target_last_position = target->movement.position;
    return result;
}

// FUNCTION: WIZ8 0x00453cc0
bool W8Navigator::StartPatrol(const srVector3T<float>* home, float distance, float variation)
{
    W8Navigator* navigator = this;

    while (navigator->linked_navigator != 0) {
        navigator = navigator->linked_navigator;
    }
    navigator->flags = 0;
    navigator->position5 = *home;
    navigator->minimum_height = distance;
    navigator->maximum_height = variation;
    return navigator->ConfigureMovement(distance, variation);
}
// GLOBAL: WIZ8 0x00659c10
unsigned char g_navigator_link_mode;
// GLOBAL: WIZ8 0x005ebc98
const float g_navigator_linked_radius_scale = 4.0f;
// GLOBAL: WIZ8 0x005ebcc8
const float g_navigator_vertical_phase_step = 0.25f;
// GLOBAL: WIZ8 0x005ec150
extern const double g_double_005ec150 = 500.0;
// GLOBAL: WIZ8 0x006081ec
float g_navigator_minimum_speed = 0.5f;
// GLOBAL: WIZ8 0x006081f0
float g_navigator_minimum_speed_mode23 = 0.8999999761581421f;
/* Runtime scale for the camera-sphere radius in the octree trace resolver;
   the retail image carries link-time 1.0 here. */
// GLOBAL: WIZ8 0x006081f4
float g_camera_sphere_radius_scale = 1.0f;
// GLOBAL: WIZ8 0x00659bf8
W8GrowableVector<W8Navigator*> g_navigator_group;

/* 0x005EC2A8: a quarter turn, shared with the world elevation helper. */
// GLOBAL: WIZ8 0x005ec2a8
const float g_quarter_turn = 1.57079625f;
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
    if (linked_navigator == 0) {
        return;
    }
    srVector3T<float> own_position = movement.position;
    srVector3T<float> linked_position = linked_navigator->movement.position;
    srVector3T<float> delta = linked_position - own_position;
    if (delta.LengthSquared() <= radius * radius * NAVIGATOR_LINK_DISTANCE_SQUARED) {
        linked_update_time = 0;
        return;
    }
    if (static_cast<W8Monster*>(this)->IsWithinWorldRange() == 0 &&
        static_cast<W8Monster*>(linked_navigator)->IsWithinWorldRange() == 0) {
        UpdateLinkedPosition();
        return;
    }
    int tick = static_cast<int>(g_game_time_accumulator->GetElapsed());
    if (static_cast<unsigned int>(tick - linked_update_time) <= 50) {
        return;
    }
    linked_update_time = tick;
    srVector3T<float> camera;
    GetCameraPosition(&camera);
    if (static_cast<W8Monster*>(linked_navigator)->IsWithinWorldRange() != 0) {
        MonsterGetWorldAnimationBounds(static_cast<W8Monster*>(linked_navigator),
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
    if (g_octree->pathing->PrepareLinkedNavigator(&movement) != 0) {
        if (movement_stopped != 0) {
            movement_stopped = 0;
            if (g_combat_inactive != 0 && linked_navigator == 0) {
                g_navigator_group.Clear();
                CollectGroupNavigators(&g_navigator_group);
                for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                    (*g_navigator_group.GetAt(index))->movement_stopped = 0;
                }
            }
        }
    } else {
        SetMovementStopped();
        AimAtPosition(&camera);
    }
}

// FUNCTION: WIZ8 0x00454fe0
unsigned char W8Navigator::UpdateLinkedPosition()
{
    if (linked_navigator == 0) {
        return 0;
    }
    srVector3T<float> position;
    if (linked_navigator->movement_stopped != 0) {
        if (g_octree->FindNavigatorPosition(&linked_navigator->movement.position,
                                            linked_navigator->movement.yaw,
                                            movement.collision_radius +
                                                movement.collision_radius,
                                            1, &position, 1, 0, 0, 5, 1) == 0) {
            return 0;
        }
        SetMovementStopped();
    } else {
        position = linked_navigator->movement.position;
        if (movement_stopped != 0) {
            movement_stopped = 0;
        }
    }
    movement.attachment->CopyPathFrom(linked_navigator->movement.attachment);
    movement.attachment->flags &= 0xff7fffff;
    movement.yaw = linked_navigator->movement.yaw;
    movement.velocity = linked_navigator->movement.velocity * 0.5;
    SetPosition(&position);
    g_octree->QueueOctreeKind13(movement.location_id, &position);
    linked_update_time = 0;
    return 1;
}

// FUNCTION: WIZ8 0x00454440
srVector3T<float>* W8Navigator::AdjustPosition(srVector3T<float>* result,
                                               const srVector3T<float>* current,
                                               const srVector3T<float>* previous)
{
    float acceleration_scale = 0.25f;
    if (movement.location_id == 0) {
        *result = *current;
        return result;
    }
    /* 0x0045445D skips the whole settle-and-drop body when navigation_mode is
       4 and falls into the shared stop path below, so the mode test guards the
       body rather than sharing the location-id early return. */
    if (navigation_mode != 4) {
        srVector3T<float> probe = *current;
        probe.y += g_world_scale;
        bool hit;
        float ground = g_octree->SettleToGround(&probe, &hit, 1, 500.0f);
        if (hit != 0) {
            if (g_octree->current_prop < 0) {
                movement.position_adjusted = 0;
            } else {
                movement.position_adjusted = 1;
                acceleration_scale = 0.5f;
            }
            if (current->y - ground <= NAVIGATOR_MAXIMUM_DROP ||
                (previous->x == probe.x && previous->z == probe.z)) {
                if (g_startup_near_limit < current->y - ground) {
                    srVector3T<float> falling = *current;
                    float distance;
                    if (g_combat_inactive == 0) {
                        movement.vertical_velocity +=
                            g_game_time_accumulator->GetFrameDelta() *
                            g_settings.monster_movement_speed * g_navigator_gravity *
                            acceleration_scale;
                        distance = movement.vertical_velocity *
                                   g_game_time_accumulator->GetFrameDelta() *
                                   g_settings.monster_movement_speed;
                    } else {
                        movement.vertical_velocity +=
                            g_game_time_accumulator->GetFrameDelta() * g_navigator_gravity *
                            acceleration_scale;
                        distance = movement.vertical_velocity *
                                   g_game_time_accumulator->GetFrameDelta();
                    }
                    falling.y -= distance;
                    if (falling.y >= ground) {
                        position_dirty = 1;
                        *result = falling;
                        return result;
                    }
                }
                movement.vertical_velocity = 0.0f;
                position_dirty = 0;
            } else {
                movement.velocity.SetZero();
                if (g_combat_inactive == 0) {
                    ClearMovement();
                }
                *result = *previous;
                return result;
            }
            probe.y = ground;
            *result = probe;
            return result;
        }
    }
    movement.velocity.SetZero();
    if (g_combat_inactive == 0) {
        ClearMovement();
    }
    *result = *previous;
    return result;
}

// FUNCTION: WIZ8 0x00454780
void W8Navigator::UpdateFacing(bool immediate)
{
    if (movement.pitch_enabled == 0 && movement.roll_enabled == 0) {
        return;
    }
    srVector3T<float> forward(0.0f, 0.0f, 1.0f);
    srVector3T<float> normal;
    forward.RotateAboutY(sin(movement.yaw), cos(movement.yaw));
    g_octree->GetPathSurfaceNormal(&movement.position, &normal);
    if (movement.pitch_enabled != 0) {
        float angle = static_cast<float>(acos(DotProduct(normal, forward)));
        if (angle < g_quarter_turn) {
            angle += NAVIGATOR_THREE_QUARTER_TURN;
        } else {
            angle -= g_quarter_turn;
        }
        if (immediate != 0) {
            movement.pitch = NormalizeAngle(angle);
        }
        movement.target_pitch = NormalizeAngle(angle);
    }
    if (movement.roll_enabled != 0) {
        srVector3T<float> side(-forward.z, 0.0f, forward.x);
        float angle = static_cast<float>(acos(DotProduct(side, normal)));
        if (angle < g_quarter_turn) {
            angle += NAVIGATOR_THREE_QUARTER_TURN;
        } else {
            angle -= g_quarter_turn;
        }
        if (immediate != 0) {
            movement.roll = NormalizeAngle(angle);
        }
        movement.target_roll = NormalizeAngle(angle);
    }
}

// FUNCTION: WIZ8 0x00453230
W8Navigator* W8Navigator::ResolveBlockingNavigator(const srVector3T<float>* from,
                                                   srVector3T<float>* to, bool include_target)
{
    int hit_location;
    int location;

    if (active == 0) {
        return 0;
    }
    if (target_navigator == 0) {
        hit_location = -3;
    } else {
        hit_location = target_navigator->movement.location_id;
    }
    location = -3;
    if (include_target != 0 && target_navigator != 0) {
        location = target_navigator->movement.location_id;
    }
    if (g_octree->ResolveTraceHit(from, to, movement.location_id, &hit_location, location,
                                  trace_mask, 0) != 0) {
        if (hit_location == 0) {
            return g_startup_world;
        }
        if (hit_location > 0) {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x5d7, NAVIGATOR_CPP, hit_location, 1));
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

    movement.CopySettingsFrom(this->movement);
    movement.target_location_id = location_id;
    movement.basis = this->movement.basis;
    movement.position = this->movement.position;
    movement.target_position = *target;
    attachment.InitializeSegment(&this->movement.position, target);
    movement.attachment = &attachment;
    if (g_combat_inactive == 0) {
        attachment.flags |= 0xc010000;
    } else {
        attachment.flags &= 0xf3feffff;
    }
    ready = g_octree->PrepareNavigatorTarget(&movement, max_range, radius);
    movement.attachment = 0;
    if (ready != 0 && ((attachment.flags & W8_NAV_ATTACHMENT_FOLLOW_PATH) == 0 ||
                       (attachment.flags & 7) != 3)) {
        return attachment.MeasurePathLength();
    }
    return -1.0;
}

// FUNCTION: WIZ8 0x00453480
int W8Navigator::FindNavigatorPathDistance(float max_range, float* out_distance)
{
    double distance =
        MeasurePathDistance(&g_startup_world->movement.position, max_range, 0);

    *out_distance = static_cast<float>(distance);
    if (distance != g_negative_one) {
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00453520
void W8Navigator::SetVelocity(const srVector3T<float>* velocity)
{
    movement.velocity = *velocity;
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
    flags = (flags & 0xc810000) | W8_NAV_ATTACHMENT_POSITION_RECORDED;
    path_position_index = 1;
    path_cursor = 1;
    path_length = 0;
    separation = 0;
    position0 = *source;
    position7[0] = *source;
    position1 = *destination;
    position7[1] = *destination;
    position6 = position0;
    position4 = position0;
    follow_offset = 0;
    memset(path_values, 0, capacity * sizeof(unsigned short));
    path_length = (position1 - position0).Length();
}

// FUNCTION: WIZ8 0x004564f0
void W8NavigatorAttachment::CopyPathFrom(const W8NavigatorAttachment* other)
{
    flags = other->flags;
    path_cursor = other->path_cursor;
    path_position_index = other->path_position_index;
    follow_offset = other->follow_offset;
    path_length = other->path_length;
    separation = other->separation;
    position0 = other->position0;
    position4 = other->position0;
    position1 = other->position1;
    if ((flags & W8_NAV_ATTACHMENT_START_WAYPOINT) != 0) {
        start_waypoint = other->start_waypoint;
    }
    if (path_position_index + 1 >= this->capacity) {
        int capacity = (path_position_index / 10 + 1) * 10;
        srVector3T<float>* positions = new srVector3T<float>[capacity];
        delete[] position7;
        position7 = positions;
        unsigned short* values =
            static_cast<unsigned short*>(malloc(capacity * sizeof(unsigned short)));
        free(path_values);
        path_values = values;
        this->capacity = static_cast<unsigned short>(capacity);
    }
    for (unsigned int index = 1; index <= path_position_index; ++index) {
        position7[index] = other->position7[index];
        path_values[index] = other->path_values[index];
    }
}

// FUNCTION: WIZ8 0x00456660
void W8NavigatorAttachment::GetNextPosition(srVector3T<float>* position)
{
    if ((flags & W8_NAV_ATTACHMENT_START_WAYPOINT) != 0) {
        *position = start_waypoint;
        return;
    }
    if (path_cursor < path_position_index) {
        *position = position7[path_cursor];
    } else {
        *position = position1;
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
    bool on_path;

    local = *position;
    on_path = 1;
    /* Retail evaluates both halves of this predicate and writes one either
       way; the check is dead in the recovered body exactly as shipped. */
    if ((flags & W8_NAV_ATTACHMENT_FOLLOW_PATH) == 0 && distance > NAVIGATOR_MAXIMUM_DROP) {
        on_path = 1;
    }
    flags &= ~W8_NAV_ATTACHMENT_PATH_LENGTH_CACHED;
    delta = position7[path_cursor] - local;
    segment = delta.xz().Length();
    if (segment < g_double_005ebc30 && path_cursor == path_position_index) {
        return 0;
    }
    if (segment < distance) {
        do {
            if (path_position_index < path_cursor) {
                break;
            }
            distance -= segment;
            local = position7[path_cursor];
            ++path_cursor;
            /* Retail reads position7[path_cursor] before the loop head re-tests
               the cursor: when the consumed waypoint was the last one this
               samples one slot past path_position_index, inside the
               ten-entry allocation but never initialized. */
            delta = position7[path_cursor] - local;
            segment = delta.xz().Length();
        } while (segment < distance);
    }
    if (path_position_index < path_cursor) {
        *position = position1;
        path_cursor = path_position_index;
        on_path = 0;
    } else {
        t = distance / segment;
        *position =
            local * static_cast<float>(g_double_005ebc30 - t) + position7[path_cursor] * t;
    }
    if (path_cursor > 1) {
        for (position_cursor = 0; path_cursor + position_cursor <= path_position_index;
             ++position_cursor) {
            position7[position_cursor + 1] = position7[position_cursor + path_cursor];
        }
        path_position_index += 1 - path_cursor;
        path_cursor = 1;
    }
    position7[0] = *position;
    position0 = position7[0];
    position4 = position7[0];
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

    base = path_cursor - 1;
    if (base == 0) {
        base = 1;
    }
    end = base + 1;
    if (path_position_index < end) {
        return 0;
    }
    point = position->xz();
    from = position7[base].xz();
    to = position7[end].xz();
    distance = PointToSegmentDistance2D(&point, &from, &to, 0, &fraction);
    surfaces = g_octree->pathing->m_pSurfaces;
    from_height = (surfaces[path_values[base]].flags >> 0xc) * g_world_scale;
    to_height = (surfaces[path_values[end]].flags >> 0xc) * g_world_scale;
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
    float other_fraction;
    float other_distance;
    unsigned int base;
    float from_height;
    float to_height;
    W8PathSurface* surfaces;

    point = position->xz();
    from = position7[path_cursor - 1].xz();
    to = position7[path_cursor].xz();
    distance = PointToSegmentDistance2D(&point, &from, &to, 0, &fraction);
    base = path_cursor - 1;
    if (path_cursor < path_position_index && path_values[path_cursor + 1] != 0) {
        from = to;
        to = position7[path_cursor + 1].xz();
        other_distance = PointToSegmentDistance2D(&point, &from, &to, 0, &other_fraction);
        if (other_distance < distance) {
            base = path_cursor;
            fraction = other_fraction;
            distance = other_distance;
        }
    }
    surfaces = g_octree->pathing->m_pSurfaces;
    from_height = (surfaces[path_values[base]].flags >> 0xc) * g_world_scale;
    to_height = (surfaces[path_values[base + 1]].flags >> 0xc) * g_world_scale;
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
    srVector2T<float> dir;
    float fraction;
    float remainder;
    float segment;
    bool reached;

    point = position->xz();
    from = position7[path_cursor - 1].xz();
    to = position7[path_cursor].xz();
    PointToSegmentDistance2D(&point, &from, &to, 1, &fraction);
    dir.x = to.x - from.x;
    dir.y = to.y - from.y;
    remainder = (g_float_one - fraction) * dir.Length();
    if (distance <= remainder) {
        reached = 0;
    } else {
        reached = 1;
        if (path_cursor < path_position_index && path_values[path_cursor + 1] != 0) {
            distance -= remainder;
            point = to;
            dir.x = position7[path_cursor + 1].x - to.x;
            dir.y = position7[path_cursor + 1].z - to.y;
            remainder = dir.Length();
        }
    }
    if (remainder < distance) {
        distance = remainder;
    }
    if (distance <= g_float_zero) {
        position->x = point.x;
        position->z = point.y;
        return reached;
    }
    segment = dir.Length();
    if (segment != g_double_zero) {
        distance /= segment;
        dir.x *= distance;
        dir.y *= distance;
    }
    position->x = dir.x + point.x;
    position->z = dir.y + point.y;
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
    while (path_cursor <= path_position_index) {
        if (distance <= g_double_zero) {
            break;
        }
        srVector3T<float>* waypoint = &position7[path_cursor];
        *direction = *waypoint - *position;
        float length = direction->Length();
        if (g_double_zero < length) {
            float scale = static_cast<float>(g_double_005ebc30 / length);
            *direction *= scale;
        }
        if (length <= distance) {
            *position = *waypoint;
            distance -= length;
            if (path_cursor == path_position_index) {
                return 1;
            }
            ++path_cursor;
        } else {
            position->x = direction->x * distance + position->x;
            position->y = direction->y * distance + position->y;
            position->z = direction->z * distance + position->z;
            distance = static_cast<float>(g_double_zero);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00452560
void W8Navigator::SetScale(float scale)
{
    float ratio = scale / movement.scale;
    movement.scale = scale;
    radius *= ratio;
    movement.collision_radius *= ratio;
    movement.alternate_radius *= ratio;
    movement.height_offset *= ratio;
    movement.secondary_height_offset *= ratio;
    movement.movement_scale *= ratio;
    if (g_runtime_world_scale < movement.collision_radius) {
        g_runtime_world_scale = movement.collision_radius;
    }
    if (g_runtime_world_scale < movement.alternate_radius) {
        g_runtime_world_scale = movement.alternate_radius;
    }
    if (g_runtime_world_scale < radius) {
        g_runtime_world_scale = radius;
    }
}

// FUNCTION: WIZ8 0x00452630
unsigned char W8Navigator::ConfigureMovementToPosition(const srVector3T<float>* position)
{
    flags = 6;
    movement.target_location_id = -1;
    movement_target.SetZero();
    collision_margin = 0.0;
    target_navigator = 0;
    if (movement_stopped != 0) {
        PathAIClearOwned(path_ai);
    }
    srVector3T<float> target = *position;
    flags = 6;
    movement.target_position = target;
    movement_target = target;
    if (movement.active_rank == 0) {
        movement.active_rank = movement.leadership_rank;
    }
    return SetMovementTarget(&movement_target, 0);
}

// FUNCTION: WIZ8 0x004531f0
void W8Navigator::SetHalted(bool value)
{
    halted = value;
    if (value == 0) {
        if (path_ai != 0) {
            PathAIResetTick(path_ai);
        }
    } else {
        movement.velocity.SetZero();
    }
}

// FUNCTION: WIZ8 0x00453ca0
void W8Navigator::SetPitchRollEnabled(bool pitch, bool roll)
{
    movement.pitch_enabled = pitch;
    movement.roll_enabled = roll;
}

/* 0x004527AD is a split address inside this body: it resumes at the
   g_pathing nonnull branch with the call registers still live, not a
   separate authored function. */
// FUNCTION: WIZ8 0x004527a0
unsigned char W8Navigator::LinkToNavigator(W8Navigator* target, double separation)
{
    unsigned char result = 0;
    if (g_pathing == 0) {
        return 0;
    }
    movement_target.SetZero();
    collision_margin = separation;
    flags = 0;
    target_navigator = target;
    movement.target_location_id = target->movement.location_id;
    PathAIClearOwned(path_ai);
    movement.attachment->InitializeSegment(&movement.position,
                                                   &target->movement.position);
    if (g_combat_inactive == 0) {
        movement.attachment->flags |= W8_NAV_ATTACHMENT_FOLLOW_PATH;
        if (g_pathing->PlanMovementToPosition(&movement, &target->movement.position,
                                              radius, static_cast<float>(separation)) == 0) {
            movement_plan_failed = 1;
            return 0;
        }
        flags = 9;
        target_last_position = target->movement.position;
        result = static_cast<unsigned char>(movement.attachment->flags & 7);
        movement_stopped = 0;
        if (g_combat_inactive != 0 && linked_navigator == 0) {
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                (*g_navigator_group.GetAt(index))->movement_stopped = 0;
            }
        }
    } else if (g_octree->LinkNavigatorTarget(&movement, &target->movement.position,
                                             static_cast<float>(separation)) != 0) {
        flags = 9;
        target_last_position = target->movement.position;
        movement_stopped = 0;
        result = 1;
        if (g_combat_inactive != 0 && linked_navigator == 0) {
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                (*g_navigator_group.GetAt(index))->movement_stopped = 0;
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
    movement.attachment->flags |= W8_NAV_ATTACHMENT_FOLLOW_PATH;
    movement_target.SetZero();
    collision_margin = separation;
    flags = 0x21;
    target_navigator = target;
    movement.target_location_id = target->movement.location_id;
    movement.target_position = target->movement.position;
    PathAIClearOwned(path_ai);
    radius = movement.alternate_radius;
    unsigned short result = g_pathing->ConfigureMovementSearch(
        &movement, target->movement.location_id, radius, separation,
        maximum_distance, position, trace_mode, target->movement.height_offset, facing,
        probe_result);
    target_last_position = target->movement.position;
    if (result == 0) {
        ClearMovement();
        flags = 0;
        movement_plan_failed = 1;
        return 0;
    }
    if (movement_stopped != 0) {
        movement_complete = 0;
        movement_stopped = 0;
        if (g_combat_inactive != 0 && linked_navigator == 0) {
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                (*g_navigator_group.GetAt(index))->movement_stopped = 0;
            }
        }
    }
    return result;
}

// FUNCTION: WIZ8 0x00452bd0
void W8Navigator::LinkGroupNavigator(W8Navigator* target, double, int)
{
    if (g_combat_inactive != 0) {
        movement_target.SetZero();
        collision_margin = 0.0;
        target_navigator = 0;
    }
    linked_navigator = target;
    if (target == 0) {
        if (g_combat_inactive != 0) {
            flags &= 0xfffffdfe;
            if (flags == 0) {
                movement.attachment->InitializeSegment(&movement.position,
                                                               &movement.position);
                SetMovementStopped();
            }
        }
    } else {
        movement.flags |= 2;
        if (g_combat_inactive != 0) {
            flags = (flags & 0xff000201) | 0x201;
            movement.attachment->InitializeSegment(&movement.position,
                                                           &target->movement.position);
        }
    }
}

// FUNCTION: WIZ8 0x00452c90
void W8Navigator::ResetMovementAndGroupState()
{
    movement_target.SetZero();
    W8Navigator* linked = linked_navigator;
    collision_margin = 0.0;
    target_navigator = 0;
    if (linked != 0) {
        movement.flags |= 2;
        if (g_combat_inactive == 0) {
            return;
        }
        flags = 0x201;
    } else {
        if (g_combat_inactive == 0) {
            return;
        }
        flags &= 0xfffffdfe;
        if (flags == 0) {
            movement.attachment->InitializeSegment(&movement.position,
                                                           &movement.position);
            SetMovementStopped();
        }
        if (g_combat_inactive != 0) {
            linked_update_time = 0;
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                W8Navigator* navigator = *g_navigator_group.GetAt(index);
                navigator->movement.attachment->CopyPathFrom(movement.attachment);
                navigator->position5 = position5;
                navigator->movement.attachment->flags &= 0xff7effff;
                navigator->linked_update_time = 0;
            }
        }
        if (movement_stopped == 0) {
            movement_stopped = 0;
            if (g_combat_inactive == 0) {
                return;
            }
            if (linked_navigator == 0) {
                g_navigator_group.Clear();
                CollectGroupNavigators(&g_navigator_group);
                for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                    (*g_navigator_group.GetAt(index))->movement_stopped = 0;
                }
            }
        }
    }
    if (g_combat_inactive != 0) {
        movement.attachment->flags &= ~W8_NAV_ATTACHMENT_FOLLOW_PATH;
    }
}

// FUNCTION: WIZ8 0x00453d20
unsigned char W8Navigator::ConfigureMovement(float minimum, float maximum)
{
    flags |= 0x20000000;
    movement.attachment->flags |= 0x800000;
    if (minimum > g_float_zero) {
        minimum_height = minimum;
    }
    if (maximum > g_float_005ec2f8) {
        maximum_height = maximum;
    }
    if (minimum_height + g_float_005ec2f8 < maximum_height) {
        movement.target_position = position5;
        if (g_octree->PrepareNavigatorPatrol(&movement, minimum_height,
                                             maximum_height) != 0) {
            flags |= 6;
            movement_stopped = 0;
            if (g_combat_inactive != 0 && linked_navigator == 0) {
                g_navigator_group.Clear();
                CollectGroupNavigators(&g_navigator_group);
                for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                    (*g_navigator_group.GetAt(index))->movement_stopped = 0;
                }
            }
            halted = 0;
            movement_target = movement.attachment->position1;
            movement.attachment->separation = 0.0f;
            if (g_combat_inactive != 0) {
                linked_update_time = 0;
                g_navigator_group.Clear();
                CollectGroupNavigators(&g_navigator_group);
                for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                    W8Navigator* navigator = *g_navigator_group.GetAt(index);
                    navigator->movement.attachment->CopyPathFrom(
                        movement.attachment);
                    navigator->position5 = position5;
                    navigator->movement.attachment->flags &= 0xff7effff;
                    navigator->linked_update_time = 0;
                }
            }
            return 1;
        }
        flags &= 0xdffffff9;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00454170
unsigned char W8Navigator::SetMovementTarget(const srVector3T<float>* target, bool propagate)
{
    movement.target_position = *target;
    movement_target = *target;
    unsigned char result;
    if (g_combat_inactive == 0) {
        radius = movement.alternate_radius;
        result = g_octree->PrepareNavigatorTarget(&movement, radius,
                                                  static_cast<float>(collision_margin));
    } else {
        radius = movement.collision_radius;
        if ((flags & 4) != 0 && (flags & 1) != 0) {
            result = g_octree->PrepareNavigatorTarget(
                &movement, radius,
                static_cast<float>(target_navigator->radius + collision_margin));
        } else {
            result = g_octree->PrepareNavigatorTarget(&movement, radius,
                                                      static_cast<float>(collision_margin));
        }
    }
    if (result == 0 || IsNavigatorAtTarget(&movement) != 0) {
        radius = movement.alternate_radius;
    }
    if (result == 0) {
        ClearMovement();
    } else if (movement_stopped != 0) {
        movement_complete = 0;
        movement_stopped = 0;
        if (g_combat_inactive != 0 && linked_navigator == 0) {
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                (*g_navigator_group.GetAt(index))->movement_stopped = 0;
            }
        }
    }
    if (propagate == 0 && result != 0 && movement.attachment != 0 &&
        (movement.attachment->flags & W8_NAV_ATTACHMENT_FOLLOW_PATH) == 0 &&
        g_combat_inactive != 0) {
        linked_update_time = 0;
        g_navigator_group.Clear();
        CollectGroupNavigators(&g_navigator_group);
        for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
            W8Navigator* navigator = *g_navigator_group.GetAt(index);
            navigator->movement.attachment->CopyPathFrom(movement.attachment);
            navigator->position5 = position5;
            navigator->movement.attachment->flags &= 0xff7effff;
            navigator->linked_update_time = 0;
        }
    }
    return result;
}

// FUNCTION: WIZ8 0x00453690
void W8Navigator::AddPathPoint(const srVector3T<float>* position)
{
    if (path_ai == 0) {
        path_ai = CreateRecord(movement.location_id);
        if (path_ai == 0) {
            srAssertFail("pNavAI", NAVIGATOR_CPP, 0x6c7, 0);
        }
    }
    flags &= 0x00ffffff;
    if (flags == 0) {
        flags = 6;
        if (movement.active_rank == 0) {
            movement.active_rank = movement.leadership_rank;
        }
        movement.target_position = *position;
        movement_target = *position;
        movement_stopped = 0;
        if (g_combat_inactive != 0 && linked_navigator == 0) {
            g_navigator_group.Clear();
            CollectGroupNavigators(&g_navigator_group);
            for (int index = 0; index < g_navigator_group.GetCount(); ++index) {
                (*g_navigator_group.GetAt(index))->movement_stopped = 0;
            }
        }
        PathAIAddPoint(path_ai, &movement.position);
        SetMovementTarget(&movement_target, 0);
    }
    PathAIAddPoint(path_ai, position);
}

// FUNCTION: WIZ8 0x004537c0
void W8Navigator::SetObject68Flag38(char value)
{
    if (path_ai != 0) {
        if (value != 0) {
            path_ai->looping = 1;
            return;
        }
        path_ai->looping = 0;
    }
}

// FUNCTION: WIZ8 0x00453c50
void W8Navigator::SetMovementScale(float value)
{
    movement.movement_scale = value;
}

// FUNCTION: WIZ8 0x00453c60
float W8Navigator::GetMovementScale()
{
    return movement.movement_scale;
}

// FUNCTION: WIZ8 0x00454040
void W8Navigator::SetFacingToward(const srVector3T<float>* target)
{
    srVector3T<float> current = movement.position;
    current.y += movement.height_offset;
    if (target->x != current.x || target->y != current.y || target->z != current.z) {
        float angle = GetHeadingAngle(&current, target);
        SetAngles(angle);
        if (navigation_mode == 2 || navigation_mode == 3) {
            angle = -GetElevationAngle(&current, target);
            SetPitch(angle);
        } else if (navigation_mode == 5 || navigation_mode == 6) {
            UpdateFacing(1);
        }
        if (movement.location_id != 0) {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0xa9d, NAVIGATOR_CPP, movement.location_id, 1));
            if (monster_info->fInCombat != 0) {
                monster_info->pCombat->sight_refresh_pending = 1;
            }
        }
    }
}

// FUNCTION: WIZ8 0x00456020
void W8Navigator::SetPosition(const srVector3T<float>* position)
{
    if (position->x != movement.position.x || position->y != movement.position.y ||
        position->z != movement.position.z) {
        movement.position = *position;
        srVector3T<double> widened;
        widened.SetFromFloat(position);
        node->setLocation(widened);
        if (movement.location_id != 0 || this == g_startup_world) {
            g_navigator_position_changed = 1;
        }
        UpdateFacing(1);
        if (movement.attachment != 0) {
            *movement.attachment->position7 = *position;
            movement.attachment->position4 = *movement.attachment->position7;
            movement.attachment->position0 = *movement.attachment->position7;
        }
    }
    position_dirty = 1;
}

// FUNCTION: WIZ8 0x00453590
void W8Navigator::SetPositionInternal(const srVector3T<float>* position)
{
    srVector3T<double> widened;

    if (position->x != movement.position.x || position->y != movement.position.y ||
        position->z != movement.position.z) {
        movement.position = *position;
        widened.SetFromFloat(position);
        node->setLocation(widened);
        if (movement.location_id != 0 || this == g_startup_world) {
            g_navigator_position_changed = 1;
        }
        UpdateFacing(1);
        if (movement.attachment != 0) {
            *movement.attachment->position7 = movement.position;
            movement.attachment->position4 = *movement.attachment->position7;
            movement.attachment->position0 = *movement.attachment->position7;
        }
    }
}

// FUNCTION: WIZ8 0x004537e0
void W8Navigator::ClearMovement()
{
    SetMovementStopped();

    PathAIClearOwned(path_ai);
    movement.velocity.SetZero();
    if ((movement.attachment->flags & W8_NAV_ATTACHMENT_FOLLOW_PATH) == 0) {
        flags &= 0xff000000;
    } else {
        flags = 0;
    }
    movement.attachment->RecordPosition(&movement.position);
    g_octree->QueueOctreeKind13(movement.location_id, &movement.position);
    movement_complete = 1;
}

// FUNCTION: WIZ8 0x00453990
void W8Navigator::UpdateAngles()
{
    float step = g_navigator_default_turn_rate;
    float distance;
    float reverse_distance;
    float direction;

    if (gXStatus.fCombatMode == 0) {
        step = movement.turn_rate;
    }
    step *= g_rate * g_game_time_accumulator->GetFrameDelta();

    if (movement.yaw != movement.target_yaw) {
        distance = NormalizeAngle(movement.yaw - movement.target_yaw);
        reverse_distance = NormalizeAngle(movement.target_yaw - movement.yaw);
        direction = g_negative_one;
        if (reverse_distance < distance) {
            distance = reverse_distance;
            direction = g_float_one;
        }
        if (step <= distance) {
            movement.yaw = NormalizeAngle(step * direction + movement.yaw);
        } else {
            movement.yaw = movement.target_yaw;
        }
        if (movement_stopped != 0) {
            UpdateFacing(0);
        }
        if (static_cast<float>(fabs(movement.yaw - movement.target_yaw)) <
            g_navigator_snap_angle) {
            movement.yaw = movement.target_yaw;
        }
    }

    if (navigation_mode == 3) {
        step *= g_navigator_mode3_scale;
    }
    if (movement.pitch_enabled != 0 &&
        movement.pitch != movement.target_pitch) {
        distance = NormalizeAngle(movement.pitch - movement.target_pitch);
        reverse_distance = NormalizeAngle(movement.target_pitch - movement.pitch);
        direction = g_negative_one;
        if (reverse_distance < distance) {
            distance = reverse_distance;
            direction = g_float_one;
        }
        if (step <= distance) {
            movement.pitch = NormalizeAngle(step * direction + movement.pitch);
        } else {
            movement.pitch = movement.target_pitch;
        }
    }

    if (movement.roll_enabled != 0 &&
        movement.roll != movement.target_roll) {
        distance = NormalizeAngle(movement.roll - movement.target_roll);
        reverse_distance = NormalizeAngle(movement.target_roll - movement.roll);
        direction = g_negative_one;
        if (reverse_distance < distance) {
            distance = reverse_distance;
            direction = g_float_one;
        }
        if (distance < step) {
            movement.roll = movement.target_roll;
            return;
        }
        movement.roll = NormalizeAngle(step * direction + movement.roll);
    }
}

// FUNCTION: WIZ8 0x00453f30
void W8Navigator::AimAtPosition(const srVector3T<float>* target)
{
    srVector3T<float> current = movement.position;
    current.y += movement.height_offset;
    if (target->x != current.x || target->y != current.y || target->z != current.z) {
        movement.target_yaw = NormalizeAngle(GetHeadingAngle(&current, target));
        if (navigation_mode == 2 || navigation_mode == 3) {
            movement.target_pitch = NormalizeAngle(-GetElevationAngle(&current, target));
        } else if (navigation_mode == 5 || navigation_mode == 6) {
            UpdateFacing(0);
        }

        if (movement.location_id != 0) {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0xa76, NAVIGATOR_CPP, movement.location_id, 1));
            if (monster_info->fInCombat != 0) {
                monster_info->pCombat->sight_refresh_pending = 1;
            }
        }
    }
}

// FUNCTION: WIZ8 0x00455140
void W8Navigator::CollectGroupNavigators(W8GrowableVector<W8Navigator*>* navigators)
{
    W8MonsterInfo* monster_info;
    W8MonsterGroup* group;
    unsigned int member;
    int ally;

    if (linked_navigator != 0) {
        return;
    }

    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0xe7b, NAVIGATOR_CPP, movement.location_id, 1));
    group = GetMonsterGroupByListIndex(
        GetMonsterGroupIndexByID(0xe7c, NAVIGATOR_CPP, monster_info->monster_group_id, 1));

    for (member = 0; member < group->member_count; ++member) {
        int location_id = IListGetAt(group->monsters, member);
        if (location_id != movement.location_id) {
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
void W8Navigator::UpdateNavigation(unsigned char skip_movement, bool slowed)
{
    srVector3T<float> previous = movement.position;
    srVector3T<float> adjusted;
    int movement_result = 0;
    unsigned int movement_kind;
    bool was_stopped;

    tracked_dirty = 0;
    if (position_dirty != 0 || movement.position_adjusted != 0) {
        movement.position =
            *AdjustPosition(&adjusted, &movement.position, &previous);
    }

    if (movement.vertical_amplitude != g_float_zero) {
        if (g_navigator_vertical_enabled == 0) {
            movement.vertical_offset = movement.vertical_base;
        } else {
            float phase =
                movement.vertical_phase +
                g_rate * g_game_time_accumulator->GetFrameDelta() * g_navigator_vertical_phase_step;
            phase -= static_cast<float>(floor(phase));
            movement.vertical_phase = phase;
            movement.vertical_offset = static_cast<float>(sin(phase * g_double_005ec318)) *
                                                   movement.vertical_amplitude +
                                               movement.vertical_base;
        }
    }

    if (skip_movement != 0) {
        return;
    }
    if (g_combat_inactive == 0 && movement_complete != 0) {
        UpdateAngles();
        return;
    }
    if ((flags & 0x200000) != 0) {
        return;
    }
    if (g_navigator_link_mode != 0 && linked_navigator != 0) {
        return;
    }
    if (linked_navigator == 0) {
        movement.attachment->flags |= 0x800000;
    }

    if (g_combat_inactive == 0) {
        movement.movement_speed = g_settings.monster_movement_speed;
        if (slowed != 0) {
            movement.movement_speed *= g_float_005ebc7c;
        }
    }
    if ((flags & 0x20000000) != 0 && (flags & 0xffffff) == 0) {
        ConfigureMovement(-1.0f, -1.0f);
    }
    if (halted != 0 && g_navigator_link_mode == 0) {
        UpdateAngles();
        return;
    }

    if (movement.attachment != 0 &&
        movement.attachment->path_cursor >=
            movement.attachment->path_position_index &&
        (movement.attachment->flags & W8_NAV_ATTACHMENT_START_WAYPOINT) == 0 &&
        PathAIIsComplete(path_ai) != 0) {
        radius = movement.alternate_radius;
    }
    if ((flags & 0x100000) != 0) {
        return;
    }

    was_stopped = movement_stopped;
    movement_kind = flags & 0xffffff;
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

        movement_result = g_octree->AdvanceNavigator(&movement, radius, 0.0f);
        if (movement_result == 1) {
            if (PathAINextPoint(path_ai, &next) != 0) {
                srVector3T<float> target;
                target = next;
                SetMovementTarget(&target, 0);
            } else {
                if (path_ai != 0) {
                    PathAIClearOwned(path_ai);
                    path_ai = 0;
                }
                ClearMovement();
                if (movement_kind == 6 && (flags & 0x20000000) != 0) {
                    ConfigureMovement(-1.0f, -1.0f);
                }
            }
        } else if (movement_result == 3) {
            SetMovementTarget(&movement_target, 0);
        }
        break;
    }

    case 9:
        if (target_navigator != 0) {
            /* 0x004559A2 takes the separation from the linked navigator, not
               from this navigator's collision margin: FLD dword ptr [EAX+0x84]
               with EAX loaded from +0x5c. The load is unguarded in the retail
               and stays that way here. */
            movement_result = g_octree->AdvanceNavigator(&movement, radius,
                                                         linked_navigator->radius);
            if (movement_result == 1 ||
                (movement_result == 3 &&
                 LinkToNavigator(target_navigator, collision_margin) == 0)) {
                ClearMovement();
            }
        }
        break;

    case 0x201:
        if (linked_navigator != 0 && g_combat_inactive != 0) {
            if (linked_navigator->movement_stopped != 0) {
                srVector3T<float> delta =
                    linked_navigator->movement.position - movement.position;
                if (delta.Length() < radius * g_navigator_linked_radius_scale +
                                         linked_navigator->radius) {
                    SetMovementStopped();
                    break;
                }
            }

            halted = 0;
            if (linked_update_time == 0 || movement_stopped == 0) {
                if (movement_stopped != 0) {
                    movement_stopped = 0;
                    if (g_combat_inactive != 0 && linked_navigator == 0) {
                        int index;
                        g_navigator_group.Clear();
                        CollectGroupNavigators(&g_navigator_group);
                        for (index = 0; index < g_navigator_group.GetCount(); ++index) {
                            (*g_navigator_group.GetAt(index))->movement_stopped = 0;
                        }
                    }
                }
                g_octree->AdvanceNavigator(&movement, radius,
                                           linked_navigator->radius);
            }
            UpdateLinkedNavigator();
        }
        break;

    default:
        break;
    }

    if ((flags & 0x800000) != 0 && movement_result == 1) {
        unsigned int previous_flags = flags;
        flags &= 0xff7fffff;
        if (flags == 0) {
            ClearMovement();
        } else {
            movement.active_rank = movement.leadership_rank;
            movement.target_location_id = -1;
            if ((previous_flags & 0xff000000) != 0) {
                flags |= 6;
            }
            SetMovementTarget(&movement_target, 0);
        }
    }

    if (was_stopped == 0 || movement_stopped == 0) {
        if (g_combat_inactive == 0 && movement_stopped == 0) {
            if (!(movement.target_position == movement.position)) {
                srVector3T<float> target;
                target = movement.target_position;
                AimAtPosition(&target);
            }
        }

        movement.position =
            *AdjustPosition(&adjusted, &movement.position, &previous);
        UpdateFacing(0);
        if (movement_stopped == 0) {
            srVector3T<float> velocity;
            float minimum_speed;

            if (g_combat_inactive == 0) {
                velocity.x = movement.movement_speed * movement.movement_scale *
                             g_world_scale;
                velocity.z = 0.0f;
            } else {
                velocity.x = movement.velocity.x;
                velocity.z = movement.velocity.z;
            }
            velocity.y = (movement.position.y - previous.y) /
                         (g_rate * g_game_time_accumulator->GetFrameDelta());
            if (navigation_mode == 2 || navigation_mode == 3) {
                minimum_speed = g_navigator_minimum_speed_mode23;
            } else {
                minimum_speed = g_navigator_minimum_speed;
            }
            movement.movement_speed =
                velocity.Length() / (movement.movement_scale * g_world_scale);
            if (movement.movement_speed < minimum_speed) {
                movement.movement_speed = minimum_speed;
            }
        }
    }

    UpdateAngles();
    if (movement_stopped == 0 && g_combat_inactive == 0 && movement_complete == 0 &&
        flags != 0) {
        movement.callback_progress += g_game_time_accumulator->GetFrameDelta() *
                                              movement.movement_scale * g_rate *
                                              g_world_scale;
        if (movement.callback_threshold <= movement.callback_progress) {
            if (movement_callback != 0) {
                movement_callback(this);
            }
            movement_complete = 1;
        }
    }

    {
        if (tracked_distance < (tracked_position - movement.position).Length()) {
            tracked_position = movement.position;
            tracked_dirty = 1;
        }
    }
}

// FUNCTION: WIZ8 0x00455cc0
int W8Navigator::ResolveMovement()
{
    W8Navigator* target = target_navigator;

    if (target == 0) {
        return 0;
    }
    if (g_combat_inactive == 0) {
        int result = g_octree->AdvanceNavigator(&movement, radius,
                                                static_cast<float>(collision_margin));
        if (result == 1) {
            ClearMovement();
        }
        return result;
    }

    if ((movement.attachment->flags & W8_NAV_ATTACHMENT_FOLLOW_PATH) == 0) {
        float target_motion =
            (target->movement.position - target_last_position).Length();

        if (target_motion > g_double_005ec030 ||
            (target == g_startup_world && target_motion > g_double_005ec150)) {
            target_last_position = target->movement.position;

            if (target->radius + radius + collision_margin <
                (target->movement.position - movement.position).Length()) {
                int index;

                movement_stopped = 0;
                if (linked_navigator == 0) {
                    g_navigator_group.Clear();
                    CollectGroupNavigators(&g_navigator_group);
                    for (index = 0; index < g_navigator_group.GetCount(); ++index) {
                        (*g_navigator_group.GetAt(index))->movement_stopped = 0;
                    }
                }
                halted = 0;
                SetMovementTarget(&target->movement.position, 0);
                flags = 5;
            }
        }

        if (halted == 0 && movement_stopped == 0) {
            int result = g_octree->AdvanceNavigator(&movement, radius,
                                                    static_cast<float>(collision_margin));
            if (result != 1) {
                if (result == 3 && SetMovementTarget(&target->movement.position, 0) == 0) {
                    ClearMovement();
                }
                flags |= 5;
                return result;
            }

            SetMovementStopped();
            PathAIClearOwned(path_ai);
            movement.velocity.SetZero();
            if ((movement.attachment->flags & W8_NAV_ATTACHMENT_FOLLOW_PATH) == 0) {
                flags &= 0xff000000;
            } else {
                flags = 0;
            }
            movement.attachment->RecordPosition(&movement.position);
            g_octree->QueueOctreeKind13(movement.location_id, &movement.position);
            flags |= 5;
            movement_complete = 1;
            return 1;
        }
    } else {
        movement.attachment->flags &= ~W8_NAV_ATTACHMENT_FOLLOW_PATH;
        flags &= 0xff000000;
        ClearMovement();
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
    start = *from;
    end = *to;
    step = end - start;
    length = step.Length();
    hit_limit = length;
    step /= length;
    state = 0;
}

// FUNCTION: WIZ8 0x00457640
W8OctreeTrace::W8OctreeTrace(const srVector3T<float>* from, const srVector3T<float>* to)
{
    Seed(from, to);
}

// FUNCTION: WIZ8 0x00457700
void W8OctreeTrace::Reseed(const srVector3T<float>* from, const srVector3T<float>* to)
{
    Seed(from, to);
}

// FUNCTION: WIZ8 0x004577c0
W8OctreeTrace::W8OctreeTrace()
{
    start.SetZero();
    end.SetZero();
    step.z = 0.0f;
    step.y = 0.0f;
    step.x = 0.0f;
    length = 0.0f;
    state = 0;
    hit_limit = 1.0e20f;
}

// FUNCTION: WIZ8 0x00453c70
void W8Navigator::SetMonsterTurnSpeed(float speed)
{
    movement.callback_threshold = speed * g_world_scale;
}
