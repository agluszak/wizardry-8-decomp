#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/xstatus.h"
#include "wiz8/combat_state.h"
#include "wiz8/magic.h"
#include "wiz8/sr_api.h"

/*
 * Local Code\Combat Range.cpp.
 *
 * How far apart two combatants are, and which of the attacks either of them
 * has will reach that far. Range is carried as a category rather than a
 * distance; CalcRangeDistance is the one place the two are related.
 */

#define COMBAT_RANGE_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Combat Range.cpp"

/* The five range categories, and the distance each stands for before the world
   scale is applied. Category -1 is no range at all. */
enum {
    W8_RANGE_NONE = -1,
    W8_RANGE_TOUCH = 0,
    W8_RANGE_SHORT = 1,
    W8_RANGE_LONG = 2,
    W8_RANGE_EXTREME = 3
};

/* Above this category an attack is out of the close-quarters band the melee
   rule restricts itself to. */
enum { W8_RANGE_FIRST_DISTANT = 2 };

/* Three party positions per formation row, and the row stride of the formation
   table. */
enum { W8_FORMATION_ROW_WIDTH = 3 };

extern int CalcRangeCategoryToTarget(const W8Character* character, int hand);
/* 0x00519AC0 */
extern char CountRowsBetween(int from_position, int to_position);        /* 0x0051AEC0 */
extern bool GetSightCondition37A(const void* conditions);               /* 0x00505E60 */
extern float g_world_scale_005ebc40;
extern float g_range_constant_005ec360;
extern float g_range_constant_005ec35c;

/* The formation. Three party positions per row at 0x00687511, and each
   position's own row number at 0x00687525 with a twelve-byte stride. -1 marks
   an empty place. Both live inside the block Formation & Facing.cpp saves and
   restores whole. */

/* The furthest range category any of this character's hands can reach at. */
// FUNCTION: WIZ8 0x00519ba0
int GetBestHandRangeCategory(const W8Character* character)
{
    int best = W8_RANGE_NONE;
    int category;
    unsigned int hand;

    for (hand = 0; hand < 2; ++hand) {
        if (character->hand_attacks[hand].in_play != 0) {
            category = CalcRangeCategoryToTarget(character, hand);
            if (category > best) {
                best = category;
            }
        }
    }
    return best;
}

/* Whether the first lighting condition applies, but only for the two middle
   range categories - at touch and at extreme range it never does. */
// FUNCTION: WIZ8 0x00519be0
bool RangeCategoryUsesSightCondition(const void* conditions, int range_category)
{
    if (range_category > W8_RANGE_SHORT && range_category < W8_RANGE_EXTREME) {
        return GetSightCondition37A(conditions);
    }
    return false;
}

/* The furthest range category among a monster's three attacks. Asking for the
   close-quarters band only considers the two categories inside it. */
// FUNCTION: WIZ8 0x0051a800
int GetBestMonsterAttackRange(const W8MonsterRecord* record, char close_quarters_only)
{
    int best = W8_RANGE_NONE;
    int attack;
    unsigned char category;

    for (attack = 0; attack < W8_MAX_MONSTER_ATTACKS; ++attack) {
        if (record->attacks[attack].fHasAttack != 0) {
            category = record->attacks[attack].range_category;
            if ((close_quarters_only == 0 || category < W8_RANGE_FIRST_DISTANT) &&
                (int)category > best) {
                best = category;
            }
        }
    }
    return best;
}

/* The range category one monster action works at. A spell takes the range off
   the spell record; two of the actions have a fixed answer and the rest have
   none. A plain attack takes it from the attack itself, which has to exist. */
// FUNCTION: WIZ8 0x0051a730
unsigned int GetMonsterActionRangeCategory(
    const W8MonsterInfo* monster_info, const W8MonsterRecord* record, unsigned int attack)
{
    switch (monster_info->action_kind) {
    case 0:
        break;
    case 2:
        return g_spell_records[monster_info->action_detail].range_category;
    case 3:
        return W8_RANGE_LONG;
    case 8:
        return W8_RANGE_TOUCH;
    default:
        return (unsigned int)W8_RANGE_NONE;
    }

    if (attack >= W8_MAX_MONSTER_ATTACKS) {
        srAssertFail("uiAttack < MAX_MONSTER_ATTACKS", COMBAT_RANGE_CPP, 949, 0);
    }
    if (record->attacks[attack].fHasAttack == 0) {
        srAssertFail("pMonsterDB->Attack[uiAttack].fHasAttack", COMBAT_RANGE_CPP, 950, 0);
    }
    return record->attacks[attack].range_category;
}

/* How far a range category actually is. The four categories step 2, 4, 25, 50
   before the world scale multiplies them; no range at all is zero distance. */
// FUNCTION: WIZ8 0x0051a9a0
float CalcRangeDistance(int range_category)
{
    unsigned int steps = 0;

    switch (range_category) {
    case W8_RANGE_TOUCH:
        steps = 2;
        break;
    case W8_RANGE_SHORT:
        steps = 4;
        break;
    case W8_RANGE_LONG:
        steps = 25;
        break;
    case W8_RANGE_EXTREME:
        steps = 50;
        break;
    case W8_RANGE_NONE:
        steps = 0;
        break;
    default:
        srAssertFail("FALSE", COMBAT_RANGE_CPP, 1123,
                     "CalcRangeDistance: ERROR - Invalid range category");
    }
    return steps * g_world_scale_005ebc40;
}

/* Close a gap of rows one row at a time, stopping when either the gap or the
   number of rows that could be crossed runs out. A gap that cannot be closed
   at all is marked unreachable. */
// FUNCTION: WIZ8 0x0051abe0
void CloseFormationGap(int from_position, int to_position, int* rows_apart)
{
    char crossable;

    if (g_in_combat_00683f94 == 0) {
        return;
    }
    if (*rows_apart < 0 || *rows_apart >= 2) {
        return;
    }
    crossable = CountRowsBetween(to_position, from_position);
    if (crossable == 0) {
        return;
    }
    while (*rows_apart != 0) {
        --crossable;
        --*rows_apart;
        if (crossable == 0) {
            return;
        }
    }
    *rows_apart = -1;
}

/* Whether anybody standing ahead of this position is still in formation. */
// FUNCTION: WIZ8 0x0051ae60
bool AnyoneStandsAhead(unsigned char position)
{
    int found = 0;
    unsigned int index;
    signed char slot;

    for (index = 0; index < W8_FORMATION_ROW_WIDTH; ++index) {
        slot = g_status_685170.formation.rows[position].slots[index];
        if (slot != -1 && g_party_characters[slot].out_of_formation == 0) {
            ++found;
        }
    }
    return found != 0;
}

/* Whether the front rank stands between two positions. Only positions exactly
   two rows apart can be screened, and the fifth row is never in the way. */
// FUNCTION: WIZ8 0x0051b000
bool FrontRankScreens(unsigned int from_position, unsigned int to_position)
{
    unsigned char from_row =
        g_status_685170.formation.positions[from_position].row;
    unsigned char to_row =
        g_status_685170.formation.positions[to_position].row;
    int rows_apart;
    int found;
    unsigned int index;
    signed char slot;

    if (from_row == to_row) {
        return false;
    }
    if (from_row == 4 || to_row == 4) {
        return false;
    }
    rows_apart = from_row - (signed char)to_row;
    if (rows_apart < 0) {
        rows_apart = -rows_apart;
    }
    if (rows_apart != 2) {
        return false;
    }

    found = 0;
    for (index = 0; index < W8_FORMATION_ROW_WIDTH; ++index) {
        slot = g_status_685170.formation.rows[4].slots[index];
        if (slot != -1 && g_party_characters[slot].out_of_formation == 0) {
            ++found;
        }
    }
    return found != 0;
}

/* Two seven-byte constant readers the range rules share. */
// FUNCTION: WIZ8 0x0051b300
float GetRangeConstant5EC360(void)
{
    return g_range_constant_005ec360;
}

// FUNCTION: WIZ8 0x0051b310
float GetRangeConstant5EC35C(void)
{
    return g_range_constant_005ec35c;
}
