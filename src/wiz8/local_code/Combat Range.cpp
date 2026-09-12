#include "wiz8/character.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/xstatus.h"
#include "wiz8/combat_state.h"
#include "wiz8/float_constants.h"
#include "wiz8/magic.h"
#include "wiz8/sr_api.h"
#include "wiz8/local_code/CombatRange.h"

/*
 * Local Code\Combat Range.cpp.
 *
 * How far apart two combatants are, and which of the attacks either of them
 * has will reach that far. Range is carried as a category rather than a
 * distance; CalcRangeDistance is the one place the two are related.
 */

#define COMBAT_RANGE_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Combat Range.cpp"

/* The range categories are W8RangeCategory, declared with the spell record
   that shares the domain. Above W8_RANGE_SHORT an attack is out of the
   close-quarters band the melee rule restricts itself to. */

/* Three party positions per formation row, and the row stride of the formation
   table. */
enum { W8_FORMATION_ROW_WIDTH = 3 };

/* 0x00519AC0 */
extern float g_range_constant_005ec35c;
// GLOBAL
float g_range_constant_005ec35c;

/* The formation. Three party positions per row at 0x00687511, and each
   position's own row number at 0x00687525 with a twelve-byte stride. -1 marks
   an empty place. Both live inside the block Formation & Facing.cpp saves and
   restores whole. */

/* The furthest range category any of this character's hands can reach at. */
// FUNCTION: WIZ8 0x00519ba0
W8RangeCategory GetBestHandRangeCategory(const W8Character* character)
{
    W8RangeCategory best = W8_RANGE_NONE;
    W8RangeCategory category;
    unsigned int hand;

    for (hand = 0; hand < 2; ++hand) {
        if (character->hand_attacks[hand].in_play != 0) {
            category = static_cast<W8RangeCategory>(
                CalcRangeCategoryToTarget(character, hand));
            if (category > best) {
                best = category;
            }
        }
    }
    return best;
}

/* Whether the first lighting condition applies at distant or extreme range. */
// FUNCTION: WIZ8 0x00519be0
unsigned char RangeCategoryUsesSightCondition(
    const W8MonsterInfo* monster, W8RangeCategory range_category)
{
    if (range_category >= W8_RANGE_LONG && range_category <= W8_RANGE_EXTREME) {
        return GetSightCondition37A(monster);
    }
    return false;
}

/* The furthest range category among a monster's three attacks. Asking for the
   close-quarters band only considers the two categories inside it. */
// FUNCTION: WIZ8 0x0051a800
W8RangeCategory GetBestMonsterAttackRange(const W8MonsterRecord* record, char close_quarters_only)
{
    W8RangeCategory best = W8_RANGE_NONE;
    int attack;
    unsigned char category;

    for (attack = 0; attack < W8_MAX_MONSTER_ATTACKS; ++attack) {
        if (record->attacks[attack].fHasAttack != 0) {
            category = record->attacks[attack].range_category;
            if ((close_quarters_only == 0 || category < W8_RANGE_LONG) &&
                (int)category > best) {
                best = static_cast<W8RangeCategory>(category);
            }
        }
    }
    return best;
}

/* The range category one monster action works at. A spell takes the range off
   the spell record; two of the actions have a fixed answer and the rest have
   none. A plain attack takes it from the attack itself, which has to exist. */
// FUNCTION: WIZ8 0x0051a730
W8RangeCategory GetMonsterActionRangeCategory(
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
        return W8_RANGE_NONE;
    }

    if (attack >= W8_MAX_MONSTER_ATTACKS) {
        srAssertFail("uiAttack < MAX_MONSTER_ATTACKS", COMBAT_RANGE_CPP, 949, 0);
    }
    if (record->attacks[attack].fHasAttack == 0) {
        srAssertFail("pMonsterDB->Attack[uiAttack].fHasAttack", COMBAT_RANGE_CPP, 950, 0);
    }
    return static_cast<W8RangeCategory>(record->attacks[attack].range_category);
}

/* How far a range category actually is. The four categories step 2, 4, 25, 50
   before the world scale multiplies them; no range at all is zero distance. */
// FUNCTION: WIZ8 0x0051a9a0
float CalcRangeDistance(W8RangeCategory range_category)
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

    if (gXStatus.fCombatMode == 0) {
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
        if (slot != -1 && g_party_characters[slot].bonus_1770.out_of_formation == 0) {
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
        if (slot != -1 && g_party_characters[slot].bonus_1770.out_of_formation == 0) {
            ++found;
        }
    }
    return found != 0;
}

/* Two seven-byte constant readers the range rules share. */
// FUNCTION: WIZ8 0x0051b300
float GetRangeConstant5EC360(void)
{
    return g_float_005ec360;
}

// FUNCTION: WIZ8 0x0051b310
float GetRangeConstant5EC35C(void)
{
    return g_range_constant_005ec35c;
}

// FUNCTION: WIZ8 0x0051b3f0
unsigned char Function51B3F0(int mode)
{
    switch (mode) {
    case 0:
    case 1:
        return 0;
    case 2:
    case 3:
        return 1;
    default:
        return static_cast<unsigned char>(mode);
    }
}

/* Choose what one monster aims at: the player when it can see them, otherwise
   the nearest hostile visible monster. Answers the chosen distance and fills
   the two-word target output. */
// FUNCTION: WIZ8 0x0051ac30
float MonsterChooseTarget(W8MonsterInfo* monster_info, int* out, int kind)
{
    float best = 1000000.0f;

    *out = 0;
    if (monster_info->flag_16 == 1
        && IsVisibleUnderConditions(
               monster_info, &monster_info->player_visibility, kind)
        && (best = monster_info->monster->GetDistanceToPlayer004C7CB0(),
            best < 1000000.0f)) {
        *out = 2;
    }
    if (*out == 0 || monster_info->pCombat->unknown_151[1] == 0) {
        unsigned int count = PLLength(gXStatus.plsMonsterList);

        for (unsigned int index = 0; index < count; ++index) {
            W8MonsterInfo* other = MonsterGetScriptPartByLocationIndex(index);

            if (other != monster_info && other->flag_14 != 0
                && other->hp_current != 0 && other->fInCombat != 0
                && MonsterHostility00546F80(monster_info, other) == 1) {
                W8VisibilityRecord* row =
                    FindMonToMonVisibility(monster_info, other);
                if (IsVisibleUnderConditions(monster_info, row, kind)) {
                    float distance =
                        monster_info->monster->GetDistanceToMonster004C7DD0(
                            other->monster);
                    if (distance < best) {
                        *out = 3;
                        out[2] = other->location_id;
                        best = distance;
                    }
                }
            }
            count = PLLength(gXStatus.plsMonsterList);
        }
    }
    return best;
}
