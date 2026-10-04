#pragma once

#include "surrender/srMath.h"

class W8Monster;
struct W8MonsterInfo;
class stParticle;
class srNode;
class stLight;

struct W8WorldCursorState {
    /* 0x00: detached with the world when the cursor hides. */
    W8Monster* monster;
    /* 0x04: deactivated with a zero when the cursor hides. */
    stParticle* particle;
    unsigned char unknown_08;
    /* 0x09: set by the right-button path while light exists; the group
       bind consumes and clears it. */
    bool group_bind_pending;
    unsigned char padding_0a[2];
    /* 0x0c: accumulated cursor input - screen dx, right-drag dy and screen
       dy - scaled by g_float_005ebc88 into the movement vector each update,
       then cleared. The group-bind paths also bump .z. */
    srVector3i input_delta;
    /* 0x18: camera-relative offset the placement update derives from
       position. */
    srVector3T<float> cam_rel_offset;
    /* 0x24: the light node the complete teardown removes from the world. */
    stLight* light;
    /* 0x28: read back by the path-visualization update as a world point. */
    srVector3T<float> position;
    /* 0x34: the last position published to the cursor's nodes and monster;
       the update compares it against position to detect a move. Seeded
       to the -1e7 sentinel by the placement update. */
    srVector3T<float> last_published;
    /* 0x40: authored name fEnabled - the cursor update asserts it. */
    bool enabled;
    /* 0x41: ground tracking. Set by the initializer and the placement
       update; the target march lifts each step to the settled ground height
       while set and re-arms it when the result lands near the ground, and
       the cursor update settles position to the terrain while set. */
    bool track_ground;
    unsigned char padding_42[2];
    /* 0x44: the cursor's march range, initialized to 50000; both movement
       paths clamp the step/offset length to it. */
    float range;
    /* 0x48: left-button latch - releasing the button while set is the
       placement click. */
    bool left_held;
    unsigned char padding_49[3];
    /* 0x4c: the selected monster group id - passed to
       GetMonsterGroupIndexByID, assigned from monster_group->group_id,
       written by SetWorldCursorGroupId and seeded from/restored to
       g_cursor_saved_group_id. */
    int monster_group_id;
    /* 0x50: when set the cursor is detached from the camera - input moves
       position directly and the placement update skips the
       camera-relative cam_rel_offset store. */
    bool detached;
    /* 0x51: while detached, run the ground/sight march on the moved
       point. */
    bool march_enabled;
    unsigned char padding_52[2];
    /* 0x54: midpoint of the probe box; the target march traces its sight fan
       from this offset. */
    srVector3T<float> probe_center;
    /* 0x60: eight probe offsets forming a box around the cursor point. The
       target march sight-traces all eight and ground-settles the last four;
       the target resolver ground-probes the same last four. */
    srVector3T<float> probe_offsets[8];
    /* 0xc0: footprint-placement mode - the target resolver uses the two
       fixed probe offsets and the click path requires the occupied-box
       test before placing. */
    bool footprint_mode;
    unsigned char padding_c1[3];
    /* 0xc4: first fixed probe offset used when footprint_mode is set. */
    srVector3T<float> extent_min;
    /* 0xd0: second fixed probe offset used when footprint_mode is set. */
    srVector3T<float> extent_max;
    /* 0xdc: the monster info latched by the shift-drag; the update drags
       it to the cursor position and releases it when shift lifts. */
    W8MonsterInfo* dragged_info;
};

static_assert(sizeof(W8WorldCursorState) == 0xe0, "W8WorldCursorState_size");

/* 0x65ba8c: authored name gp3DCursor - the cursor update asserts it. */
extern W8WorldCursorState* gp3DCursor;
/* Build the 3D cursor, light, particle and initial camera-relative bounds. */
void InitializeWorldCursor(void);
bool IsWorldCursorVisible(void);
void GetWorldCursorPosition(srVector3T<float>* position);
void SetWorldCursorNodesVisible(bool visible);
bool SelectWorldCursorNode(void);
int GetWorldCursorNodeCount(void);
void HideWorldCursor(void);
void ShowWorldCursor(void);
/* Clamp and install an action-range distance into the live world cursor, then
   invalidate last_published so the next update republishes. */
void SetWorldCursorRange(float distance); /* 0x00491650 */
void SetWorldCursorGroupId(int group_id); /* 0x004916A0 */
/* Arm footprint placement: the target resolver ground-probes the two fixed
   offsets instead of the probe box. */
void SetWorldCursorExtents(const srVector3T<float>* minimum,
                           const srVector3T<float>* maximum); /* 0x00492190 */
/* 0x00490C20: copies the cursor state vector at +0x28, or zero when there is
   no cursor. */
void GetWorldCursorAnchor(srVector3T<float>* position);
void ReleaseWorldCursor(void);
void ReleaseWorldCursorNodes(void);
