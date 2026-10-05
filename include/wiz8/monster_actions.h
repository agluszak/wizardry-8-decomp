#pragma once

/* The monster action domain is distinct from the party's W8ActionKind.
   AI decisions and the queued/committed records use the same signed word.
   ExecuteMonsterAction and MonsterActionFatigueCost consume -1 through 9. */
enum W8MonsterActionKind {
    W8_MONSTER_ACTION_NONE = -1,
    W8_MONSTER_ACTION_ATTACK = 0,
    W8_MONSTER_ACTION_WAIT = 1,
    W8_MONSTER_ACTION_SPELL = 2,
    W8_MONSTER_ACTION_SPECIAL_ATTACK = 3,
    W8_MONSTER_ACTION_ADVANCE = 4,
    W8_MONSTER_ACTION_APPROACH = 5,
    W8_MONSTER_ACTION_RETURN_TO_START = 6,
    W8_MONSTER_ACTION_BACK_OFF = 7,
    W8_MONSTER_ACTION_PROTECT = 8,
    W8_MONSTER_ACTION_CONTROLLED_MOVE = 9
};

static_assert(sizeof(W8MonsterActionKind) == 4, "W8MonsterActionKind_size");

/* Lure success/resistance state, separate from real-time movement modes. */
enum W8MonsterControlState {
    W8_MONSTER_CONTROL_NONE = 0,
    W8_MONSTER_CONTROL_LURED = 1,
    W8_MONSTER_CONTROL_RESISTED = 2
};

/* BEGINORDERS scripts and save records store this signed byte. */
typedef signed char W8MonsterOrderMode;

enum {
    W8_MONSTER_ORDER_NONE = -1,
    W8_MONSTER_ORDER_GUARD = 0,
    W8_MONSTER_ORDER_PATROL = 1,
    W8_MONSTER_ORDER_POINT_PATROL = 2,
    W8_MONSTER_ORDER_RANDOM_POINT_PATROL = 3,
    W8_MONSTER_ORDER_FACE_DIRECTION = 4
};

/* Low nibble of ai_mode/decision; the higher bits carry independent flags. */
enum W8MonsterRTAIMode {
    W8_RT_AI_IDLE = 0,
    W8_RT_AI_CHARGE_PARTY = 1,
    W8_RT_AI_LINK_TO_PARTY = 2,
    W8_RT_AI_APPROACH_PARTY = 3,
    W8_RT_AI_INVESTIGATE_NOISE = 4,
    W8_RT_AI_PATROL_AREA = 6,
    W8_RT_AI_MOVE_TO_PATROL_POINT = 7,
    W8_RT_AI_FACE_NOISE = 8,
    W8_RT_AI_FOLLOW_LURE = 9,
    W8_RT_AI_FACE_DIRECTION = 10
};

enum W8MonsterAIFlags {
    W8_MONSTER_AI_MODE_MASK = 0x0f,
    W8_MONSTER_AI_RESTORE_SCRIPT = 0x10,
    W8_MONSTER_AI_REAPPLY_MODE = 0x80
};
