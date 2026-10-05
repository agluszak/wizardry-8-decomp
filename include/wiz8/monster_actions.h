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
