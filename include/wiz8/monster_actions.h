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

/* Script command stored while the interpreter waits, including save records. */
enum W8MonsterScriptCommand {
    MONSCR_NONE = -1,
    MONSCR_GOTO,
    MONSCR_WALKTO,
    MONSCR_FACE,
    MONSCR_SAY,
    MONSCR_CYCLE,
    MONSCR_SHOOT,
    MONSCR_GIVE,
    MONSCR_TELEPORT,
    MONSCR_CAST,
    MONSCR_DIE,
    MONSCR_END,
    MONSCR_IF,
    MONSCR_ELSE,
    MONSCR_ENDIF,
    MONSCR_NPCINTERACTION,
    MONSCR_DISPOSITION,
    MONSCR_DISAPPEAR,
    MONSCR_LOOKHERE,
    MONSCR_NPCNUMBER,
    MONSCR_FOLLOW,
    MONSCR_PATROL,
    MONSCR_STOPPATROL,
    MONSCR_TRIGGER,
    MONSCR_DELAY,
    MONSCR_BEGINORDERS,
    MONSCR_ENDORDERS,
    MONSCR_GUARD,
    MONSCR_POINTPATROL,
    MONSCR_RANDOMPOINTPATROL,
    MONSCR_DEAF,
    MONSCR_DOACTION,
    MONSCR_EAST,
    MONSCR_NORTHEAST,
    MONSCR_NORTH,
    MONSCR_NORTHWEST,
    MONSCR_WEST,
    MONSCR_SOUTHWEST,
    MONSCR_SOUTH,
    MONSCR_SOUTHEAST,
    MONSCR_TURNTOFACEPARTY,
    MONSCR_LOOKABOUT,
    MONSCR_PLAY,
    MONSCR_FADEOUT,
    MONSCR_STAYHOME,
    MONSCR_COUNT
};

/* Fade direction is signed; delayed removal precedes the outward fade. */
typedef signed char W8MonsterFadeState;
enum {
    W8_MONSTER_FADE_DELAYED_REMOVAL = -2,
    W8_MONSTER_FADE_OUT = -1,
    W8_MONSTER_FADE_IDLE = 0,
    W8_MONSTER_FADE_IN = 1
};

/* Completion effects for monsters removed by level/NPC scripts. */
typedef signed char W8MonsterRemovalState;
enum {
    W8_MONSTER_REMOVAL_NONE = 0,
    W8_MONSTER_REMOVAL_RETURN_BALBRAK_HOME = 2,
    W8_MONSTER_REMOVAL_CLEAR_SCREG_ACTIVE = 3
};

enum W8MonsterSunlightState {
    W8_MONSTER_SUNLIGHT_UNKNOWN = -1,
    W8_MONSTER_SUNLIGHT_SHADOWED = 0,
    W8_MONSTER_SUNLIGHT_LIT = 1
};

/* Summoned provenance controls cleanup and the friendly summon display. */
enum W8MonsterSummonKind {
    W8_MONSTER_SUMMON_NONE = 0,
    W8_MONSTER_SUMMON_FRIENDLY = 1,
    W8_MONSTER_SUMMON_HOSTILE = 2
};

/* Representation loading policy, independent of the monster's active flag. */
enum W8MonsterActivationMode { W8_MONSTER_LOAD_ALL_CYCLES = 0, W8_MONSTER_LOAD_STARTUP_CYCLE = 1 };
