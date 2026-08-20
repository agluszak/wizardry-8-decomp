#include "wiz8/gameplay_boundaries.h"
#include "wiz8/startup_runtime_state.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/magic.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>

/* Local Code\Health Stamina Mana.cpp, named by the assertions these bodies
   embed. The party sweeps in here all share one shape: walk the eight party
   slots, skip the empty ones, and hand each occupied slot to a per-character
   worker. */

#define HEALTH_STAMINA_MANA_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Health Stamina Mana.cpp"

/* The eligibility window the party sweeps use, the same one GetRandomCharacter
   and AnyPartyMemberCanUseItem apply. */
enum { W8_CHARACTER_ELIGIBLE_LIMIT = 0x12 };

/* A negative amount means "as much as they could possibly hold", which the
   restore computes by summing the whole spell-point ceiling. */
enum { W8_RESTORE_EVERYTHING = -1 };

extern void ApplyHealthChangeToCharacter(
    int party_slot, int amount, int arg_3, int arg_4, int arg_5, int arg_6, int arg_7);
/* 0x0052A890 */
void HealCharacter(int party_slot, int amount, char announce);
void RestoreCharacterStamina(int party_slot, int amount, char announce);
void DrainCharacterSpellPoints(int party_slot, unsigned int amount, char announce);
void RestoreCharacterSpellPointsEvenly(int party_slot, int amount);
extern void NotifySpellPointsChanged(int party_slot);      /* 0x0055EE30 */
extern void ClearHighlightIfItIs(W8MonsterInfo* monster_info);
W8WideChar* GetMonsterName(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                           unsigned char name_form);

/* Roll the dice once per eligible party member and apply the result to each of
   them. The roll is separate per character rather than shared. */
// FUNCTION: WIZ8 0x0052a820
void ApplyRolledHealthChangeToParty(const W8Dice* dice, int arg_2, int arg_3)
{
    int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0 &&
            g_party_characters[party_slot].unknown_0b01 < W8_CHARACTER_ELIGIBLE_LIMIT) {
            ApplyHealthChangeToCharacter(
                party_slot, RollDice(dice), 0, arg_3, 0, arg_2, 0);
        }
    }
}

/* Heal every occupied slot by its own roll, announced. The dice are assembled
   from the caller's own arguments rather than passed as a record, which is why
   the two count bytes arrive separately from the base. */
// FUNCTION: WIZ8 0x0052ad70
void HealPartyByDice(unsigned char count, unsigned char sides, short base)
{
    W8Dice dice;
    int party_slot;

    dice.base = base;
    dice.count = count;
    dice.sides = sides;
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0) {
            HealCharacter(party_slot, RollDice(&dice), 1);
        }
    }
}

/* The stamina form, which is the same sweep unannounced. */
// FUNCTION: WIZ8 0x0052b160
void RestorePartyStaminaByDice(unsigned char count, unsigned char sides, short base)
{
    W8Dice dice;
    int party_slot;

    dice.base = base;
    dice.count = count;
    dice.sides = sides;
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0) {
            RestoreCharacterStamina(party_slot, RollDice(&dice), 0);
        }
    }
}

/* Spend spell points from one realm. Spending more than is left is a caller
   error rather than something to clamp. */
// FUNCTION: WIZ8 0x0052b480
void SpendCharacterSpellPoints(int party_slot, int realm, int amount)
{
    W8Character* character = &g_party_characters[party_slot];

    if (amount != 0) {
        if (character->sp_left[realm] < amount) {
            srAssertFail("pPC->iSPLeft[uiRealm] >= (INT32) uiSPs",
                         HEALTH_STAMINA_MANA_CPP, 1067, 0);
        }
        character->sp_left[realm] -= amount;
        NotifySpellPointsChanged(party_slot);
    }
}

/* Give spell points back to one realm, never past its ceiling. */
// FUNCTION: WIZ8 0x0052b4f0
void RestoreCharacterRealmSpellPoints(int party_slot, int realm, int amount)
{
    W8Character* character = &g_party_characters[party_slot];

    character->sp_left[realm] += amount;
    if (character->sp_max[realm] < character->sp_left[realm]) {
        character->sp_left[realm] = character->sp_max[realm];
    }
    NotifySpellPointsChanged(party_slot);
}

/* Drain spell points across the party. Unlike its neighbours this does not
   filter on the eligibility window - an unconscious character still loses
   points. */
// FUNCTION: WIZ8 0x0052b550
void DrainPartySpellPoints(int arg_1, int arg_2)
{
    int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0) {
            DrainCharacterSpellPoints(party_slot, arg_1, arg_2);
        }
    }
}

/* Restore spell points across the party. A negative amount means as much as
   the character could possibly hold, which is the sum of all six realm
   ceilings. */
// FUNCTION: WIZ8 0x0052ba00
void RestorePartySpellPoints(int amount)
{
    int party_slot;
    int realm;
    int granted;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].flag_00 != 0 &&
            g_party_characters[party_slot].unknown_0b01 < W8_CHARACTER_ELIGIBLE_LIMIT &&
            g_party_characters[party_slot].hp_current != 0) {
            granted = amount;
            if (amount < 0) {
                granted = 0;
                for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
                    granted += g_party_characters[party_slot].sp_max[realm];
                }
            }
            RestoreCharacterSpellPointsEvenly(party_slot, granted);
        }
    }
}

/* Heal one monster. A monster that is already dead or already whole is left
   alone; healing it to full says so differently from healing it partway. */
// FUNCTION: WIZ8 0x0052bfd0
void HealMonster(W8MonsterInfo* monster_info, int amount, char announce)
{
    if (monster_info->hp_current == 0 || monster_info->hp_current == monster_info->hp_max ||
        amount == 0) {
        return;
    }

    monster_info->hp_current += amount;
    if (monster_info->hp_current > monster_info->hp_max) {
        monster_info->hp_current = monster_info->hp_max;
    }
    ClearHighlightIfItIs(monster_info);
    UpdateMonsterDamageAppearance(monster_info);

    if (announce) {
        if (monster_info->hp_current == monster_info->hp_max) {
            WriteGameLog(9, (const wchar_t*)gppStringList[0x964 / 4],
                         GetMonsterName(monster_info, 0, 0));
        }
        else {
            WriteGameLog(9, (const wchar_t*)gppStringList[0x96c / 4],
                         GetMonsterName(monster_info, 0, 0), amount);
        }
    }
}

/* What one monster action costs in fatigue, before it is spent. Four of the
   ten actions are free; the plain attack costs markedly more when its detail
   is three. A monster that tires twice as fast pays double. */
// FUNCTION: WIZ8 0x0052c240
int MonsterActionFatigueCost(const W8MonsterInfo* monster_info)
{
    int cost = 0;

    switch (monster_info->action_kind) {
    case 0:
        if (monster_info->action_detail == 3) {
            cost = Random(8) + 5;
        }
        else {
            cost = Random(3) + 2;
        }
        break;
    case 1:
    case 9:
        cost = Random(2) + 1;
        break;
    case 4:
    case 7:
        cost = Random(5) + 5;
        break;
    case 5:
    case 6:
        cost = Random(3) + 3;
        break;
    case -1:
    case 2:
    case 3:
    case 8:
        break;
    default:
        srAssertFail("FALSE", HEALTH_STAMINA_MANA_CPP, 1774,
                     FormatString("MonsterActionFatigueCost: ERROR - Invalid action %d",
                                  monster_info->action_kind));
    }

    if (monster_info->condition_turns[W8_CONDITION_FATIGUE_DOUBLED] == 0) {
        return cost;
    }
    return cost * 2;
}

/* The two fatigue-band thresholds and the four bands they cut the stamina
   fraction into. Both characters and monsters use the same ladder. */
enum {
    W8_FATIGUE_BAND_1 = 0x32,
    W8_FATIGUE_BAND_2 = 0x46,
    W8_FATIGUE_BAND_3 = 0x55,
    W8_FATIGUE_BAND_4 = 0x5e
};

/* How much stamina shakes exhaustion off again. */
enum { W8_STAMINA_TO_SHAKE_OFF_EXHAUSTION = 9 };

extern void PostCharacterNotice(int party_slot, const wchar_t* notice, ...);
/* 0x00590950 */
extern unsigned char CharacterHasEffect(void* effect, int party_slot);   /* 0x0052DD90 */
extern void CalcArmorClasses(W8Character* character);                    /* 0x004EE9D0 */
extern void RemoveCharacterCondition(int party_slot, int condition, int arg_3);
extern void SetCharacterCondition(
    int party_slot, int condition, int duration, int arg_4, int arg_5, int arg_6);
extern void RecalculateCharacterHitPoints(W8Character* character);
extern void FatigueCharacter(int party_slot, int amount, char scale_by_load, int arg_4, int arg_5);
/* 0x0052AF50 */
extern void Function52F2C0(W8Character* character);
extern void ResetTargetSource(W8TargetSource* target_block);               /* 0x00536150 */
extern void SetMonsterCondition(
    int location_id, int condition, int duration, int arg_4, W8TargetSource* target_block, int quiet);
/* 0x00523C00 */
extern void ClearMonsterCondition(int location_id, int condition);       /* 0x00523F40 */
extern void ApplyMonsterCondition(int location_id, int condition, int arg_3);
/* 0x00524110 */
extern void StartMonsterCycle(W8MonsterInfo* monster_info, int cycle, int behavior);
extern unsigned char TargetSourceIsCharacter(int target, int arg_2);
extern unsigned char TargetSourceIsMonster(int target, int arg_2);
extern char MonsterVsCharDisposition(int character_slot, W8MonsterInfo* monster_info);
/* 0x00546F10 */
extern char Function546F80(W8MonsterInfo* aggressor, W8MonsterInfo* monster_info);

/* Two effects the party is holding that a wounded character can no longer
   sustain, and the third that only the deeper threshold breaks. */
extern void* g_effect_005ee594;
extern void* g_effect_005ee590;
extern void* g_effect_005ee5f8;
extern unsigned int g_effect_threshold_005ed904;
extern unsigned int g_effect_threshold_005ed900;
extern unsigned char g_spell_points_free_00687500;
/* 0x0061E518: one notice index per spell realm, giving the realm's name. */
extern const unsigned short g_spell_realm_notice[W8_SPELL_REALM_COUNT];

/* Orders the six realms by how far short of full they are. The body lives at
   0x0052B8E0 and is reached only through qsort. */
extern int __cdecl CompareSpellPointDeficits(const void* first, const void* second);

/* Turn a pool fraction into a band. The same ladder decides a character's
   fatigue band and a monster's, from the percentage of the pool that is
   missing rather than the part that is left. */
static int FatigueBandFromMissing(int missing_percent)
{
    if (missing_percent < W8_FATIGUE_BAND_1) {
        return 0;
    }
    if (missing_percent < W8_FATIGUE_BAND_2) {
        return 1;
    }
    if (missing_percent < W8_FATIGUE_BAND_3) {
        return 2;
    }
    return (missing_percent > W8_FATIGUE_BAND_4) + 3;
}

/* Heal one character, never past their maximum. Recovering enough of their
   hit points breaks the effects that only held while they were badly hurt -
   the deeper threshold breaks two more than the shallower one. */
// FUNCTION: WIZ8 0x0052add0
void HealCharacter(int party_slot, int amount, char announce)
{
    W8Character* character = &g_party_characters[party_slot];
    unsigned int hp_max;
    unsigned int fraction;

    if (g_party_slot_rows[party_slot].flag_00 == 0) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", HEALTH_STAMINA_MANA_CPP, 661, 0);
    }

    if (character->hp_current == 0) {
        return;
    }
    hp_max = character->hp_max;
    if (character->hp_current == hp_max) {
        return;
    }

    character->hp_current += amount;
    if (character->hp_current > hp_max) {
        character->hp_current = hp_max;
    }
    if (announce) {
        if (character->hp_current == hp_max) {
            PostCharacterNotice(party_slot, (const wchar_t*)gppStringList[0x960 / 4]);
        }
        else {
            PostCharacterNotice(party_slot, (const wchar_t*)gppStringList[0x968 / 4], amount);
        }
    }

    fraction = (character->hp_current * 100) / (unsigned int)character->hp_max;
    if (fraction >= g_effect_threshold_005ed904) {
        if (CharacterHasEffect(g_effect_005ee594, party_slot)) {
            g_startup_runtime_state->SetEventCharacterMask(
                reinterpret_cast<unsigned int>(g_effect_005ee594), party_slot, 0);
        }
        if (fraction >= g_effect_threshold_005ed900) {
            if (CharacterHasEffect(g_effect_005ee590, party_slot)) {
                g_startup_runtime_state->SetEventCharacterMask(
                    reinterpret_cast<unsigned int>(g_effect_005ee590), party_slot, 0);
            }
            if (CharacterHasEffect(g_effect_005ee5f8, party_slot)) {
                g_startup_runtime_state->SetEventCharacterMask(
                    reinterpret_cast<unsigned int>(g_effect_005ee5f8), party_slot, 0);
            }
        }
    }
}

/* Give one character stamina back, never past their maximum. Any move re-bands
   their fatigue, and a change of band re-runs the armour class pass because
   fatigue feeds it. Enough stamina also shakes off exhaustion. */
// FUNCTION: WIZ8 0x0052b1c0
void RestoreCharacterStamina(int party_slot, int amount, char announce)
{
    W8Character* character = &g_party_characters[party_slot];
    int stamina_max;
    int previous_band;
    int band;

    if (character->unknown_0b01 >= W8_CHARACTER_ELIGIBLE_LIMIT || character->hp_current == 0) {
        return;
    }
    stamina_max = character->stamina_max;
    if (character->stamina == stamina_max) {
        return;
    }

    character->stamina += amount;
    if (character->stamina > stamina_max) {
        character->stamina = stamina_max;
    }
    if (announce) {
        if (character->stamina == stamina_max) {
            PostCharacterNotice(party_slot, (const wchar_t*)gppStringList[0x970 / 4]);
        }
        else {
            PostCharacterNotice(party_slot, (const wchar_t*)gppStringList[0x978 / 4], amount);
        }
    }

    previous_band = character->fatigue_band;
    band = FatigueBandFromMissing(
        100 - (int)((character->stamina * 100) / (unsigned int)character->stamina_max));
    character->fatigue_band = band;
    if (band != previous_band) {
        CalcArmorClasses(character);
    }
    if (character->condition_turns[W8_CONDITION_EXHAUSTED] == W8_CONDITION_INDEFINITE &&
        character->stamina > W8_STAMINA_TO_SHAKE_OFF_EXHAUSTION) {
        RemoveCharacterCondition(party_slot, W8_CONDITION_EXHAUSTED, 1);
    }
}

/* Drain spell points from one character, taking them from randomly chosen
   realms until the whole amount is gone or fifty attempts have been spent.
   Each realm only gives up what it has, and each withdrawal is announced with
   that realm's name. */
// FUNCTION: WIZ8 0x0052b590
void DrainCharacterSpellPoints(int party_slot, unsigned int amount, char announce)
{
    W8Character* character = &g_party_characters[party_slot];
    unsigned int remaining = amount;
    unsigned int taken;
    int attempts;
    int realm;

    if (character->hp_current == 0) {
        return;
    }
    if (g_spell_points_free_00687500 != 0) {
        PostCharacterNotice(party_slot, (const wchar_t*)gppStringList[0x980 / 4], amount);
        return;
    }

    for (attempts = 0x32; remaining != 0 && attempts != 0; --attempts) {
        realm = Random(W8_SPELL_REALM_COUNT);
        if (character->sp_left[realm] > 0) {
            taken = remaining;
            if ((unsigned int)character->sp_left[realm] <= remaining) {
                taken = character->sp_left[realm];
            }
            SpendCharacterSpellPoints(party_slot, realm, taken);
            if (announce) {
                WriteGameLog(8, (const wchar_t*)gppStringList[0x98c / 4], amount,
                             gppStringList[g_spell_realm_notice[realm]]);
            }
            remaining = amount - taken;
            amount = remaining;
        }
    }
}

/* Spread spell points across a character's six realms, always topping up the
   realm that is furthest from full. The realms are sorted by how far short
   they are, then handed a point each in turn; ties are given a point together
   so the deficits stay level. */
// FUNCTION: WIZ8 0x0052b910
void RestoreCharacterSpellPointsEvenly(int party_slot, int amount)
{
    W8Character* character = &g_party_characters[party_slot];
    struct {
        unsigned int realm;
        unsigned int deficit;
    } order[W8_SPELL_REALM_COUNT];
    unsigned int index;
    int granted = 0;
    bool tied;

    for (index = 0; index < W8_SPELL_REALM_COUNT; ++index) {
        order[index].realm = index;
        order[index].deficit = character->sp_max[index] - character->sp_left[index];
    }
    qsort(order, W8_SPELL_REALM_COUNT, sizeof(order[0]), CompareSpellPointDeficits);

    for (;;) {
        if (amount == 0) {
            break;
        }
        for (index = 0; index < W8_SPELL_REALM_COUNT; ++index) {
            if (order[index].deficit == 0 ||
                (index != W8_SPELL_REALM_COUNT - 1 &&
                 order[index].deficit < order[index + 1].deficit)) {
                if (index == W8_SPELL_REALM_COUNT - 1) {
                    PostCharacterNotice(
                        party_slot, (const wchar_t*)gppStringList[0x68c / 4], granted);
                    return;
                }
                continue;
            }
            tied = index < W8_SPELL_REALM_COUNT &&
                   order[index].deficit == order[index + 1].deficit;
            ++character->sp_left[order[index].realm];
            --amount;
            ++granted;
            --order[index].deficit;
            if (!tied) {
                break;
            }
            if (amount == 0) {
                PostCharacterNotice(
                    party_slot, (const wchar_t*)gppStringList[0x68c / 4], granted);
                return;
            }
        }
    }
    PostCharacterNotice(party_slot, (const wchar_t*)gppStringList[0x68c / 4], granted);
}

/* Wound one character. Two thirds of the damage also tires them, the damage
   itself is booked against the hit-point adjustment and the pools rebuilt from
   it, and a character with no protection against it is put under condition
   one. */
// FUNCTION: WIZ8 0x0052b7e0
void DamageCharacter(int party_slot, int unused, int damage, char announce)
{
    W8Character* character = &g_party_characters[party_slot];

    if (g_party_slot_rows[party_slot].flag_00 == 0) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", HEALTH_STAMINA_MANA_CPP, 1186, 0);
    }

    if (character->hp_max != 0 && character->hp_current != 0) {
        FatigueCharacter(party_slot, (damage * 2) / 3, 0, 0, 0);
        if (announce) {
            WriteGameLog(8, (const wchar_t*)gppStringList[0x710 / 4], damage);
        }
        character->hp_adjustment -= damage;
        RecalculateCharacterHitPoints(character);
        if (character->condition_turns[1] == 0) {
            SetCharacterCondition(party_slot, 1, W8_CONDITION_INDEFINITE, 0, 0, 0);
        }
        if (character->hp_current != 0) {
            Function52F2C0(character);
        }
    }
}

/* Tire one monster. Running its pool down to nothing puts it under the
   exhausted condition indefinitely, and tells whoever asked for the fatigue
   that it landed. */
// FUNCTION: WIZ8 0x0052c070
void FatigueMonster(W8MonsterInfo* monster_info, unsigned int amount, int report_to)
{
    W8TargetSource target_block;

    if (monster_info->hp_current == 0) {
        return;
    }
    if (amount != 0) {
        GetMonsterDataForInfo(monster_info);
        if (amount < (unsigned int)monster_info->runtime_stat_current_33) {
            monster_info->runtime_stat_current_33 -= amount;
        }
        else {
            monster_info->runtime_stat_current_33 = 0;
        }
    }

    monster_info->runtime_value_242 = FatigueBandFromMissing(
        100 - (int)((monster_info->runtime_stat_current_33 * 100) /
                    (unsigned int)monster_info->runtime_stat_max_2f));

    if (monster_info->runtime_stat_current_33 == 0 &&
        (unsigned int)monster_info->condition_turns[W8_CONDITION_EXHAUSTED] < W8_CONDITION_INDEFINITE) {
        ResetTargetSource(&target_block);
        SetMonsterCondition(monster_info->location_id, W8_CONDITION_EXHAUSTED,
                            W8_CONDITION_INDEFINITE, 0, &target_block, report_to == 0);
        if (report_to != 0) {
            *(int*)((char*)report_to + 0x4c) += 1;
        }
    }
}

/* A completed spell costs its database level plus the cast result. Result
   eight is the one outcome that carries no stamina charge. MonsterCastsSpell
   deliberately returns this value to its caller. */
// FUNCTION: WIZ8 0x0052c320
unsigned int SpellCastFatigueCost(int spell_id, int result)
{
    if (result == 8) {
        return 0;
    }
    return g_spell_records[spell_id].spell_level + result;
}

/* Give one monster stamina back, the mirror of the character form down to the
   band ladder and the exhaustion it shakes off. */
// FUNCTION: WIZ8 0x0052c140
void RestoreMonsterStamina(W8MonsterInfo* monster_info, int amount, char announce)
{
    unsigned int stamina_max;

    if ((unsigned int)monster_info->value_107 >= W8_CHARACTER_ELIGIBLE_LIMIT ||
        monster_info->hp_current == 0) {
        return;
    }
    stamina_max = monster_info->runtime_stat_max_2f;
    if ((unsigned int)monster_info->runtime_stat_current_33 == stamina_max) {
        return;
    }

    monster_info->runtime_stat_current_33 += amount;
    if ((unsigned int)monster_info->runtime_stat_current_33 > stamina_max) {
        monster_info->runtime_stat_current_33 = stamina_max;
    }
    if (announce) {
        if ((unsigned int)monster_info->runtime_stat_current_33 == stamina_max) {
            WriteGameLog(9, (const wchar_t*)gppStringList[0x974 / 4],
                         GetMonsterName(monster_info, 0, 0));
        }
        else {
            WriteGameLog(9, (const wchar_t*)gppStringList[0x97c / 4],
                         GetMonsterName(monster_info, 0, 0), amount);
        }
    }

    monster_info->runtime_value_242 = FatigueBandFromMissing(
        100 - (int)((monster_info->runtime_stat_current_33 * 100) /
                    (unsigned int)monster_info->runtime_stat_max_2f));

    if (monster_info->condition_turns[W8_CONDITION_EXHAUSTED] == W8_CONDITION_INDEFINITE &&
        (unsigned int)monster_info->runtime_stat_current_33 >
            W8_STAMINA_TO_SHAKE_OFF_EXHAUSTION) {
        ClearMonsterCondition(monster_info->location_id, W8_CONDITION_EXHAUSTED);
    }
}

/* How a monster answers being struck. It plays the struck cycle; a monster
   with the right make-up may be knocked into another condition, its chance
   riding on half its fifth converted attribute; a monster the party had under
   control is handed back to itself; and if the attacker is not already an
   enemy, the disposition check decides whether being hit makes them one. */
// FUNCTION: WIZ8 0x0052beb0
void MonsterReactsToBeingStruck(W8MonsterInfo* monster_info, int attacker, char quiet)
{
    StartMonsterCycle(monster_info, 0x14, 1);

    if (monster_info->condition_turns[15] != 0 && quiet == 0 &&
        Random(100) < (unsigned int)((monster_info->converted_attributes_247[4] >> 1) + 0x32)) {
        ClearMonsterCondition(monster_info->location_id, 0xf);
    }
    if (monster_info->control_state == 1) {
        SetMonsterControlState(monster_info, 0);
    }

    if (!TargetSourceIsCharacter(attacker, 0) && !TargetSourceIsMonster(attacker, 0)) {
        return;
    }
    if (*(char*)(attacker + 0x1c) == 0 && *(char*)(attacker + 0x1b) == 0 &&
        *(char*)(attacker + 0x1e) == 0 && quiet == 0 && monster_info->condition_turns[W8_CONDITION_HOSTILE] != 0) {
        if (TargetSourceIsCharacter(attacker, 0)) {
            if (MonsterVsCharDisposition(*(int*)(attacker + 4), monster_info) == 2) {
                ApplyMonsterCondition(monster_info->location_id, 0xd, 1);
            }
        }
        else if (Function546F80(
                     MonsterInfoFromID(1570, HEALTH_STAMINA_MANA_CPP,
                                       *(int*)(attacker + 8), 1),
                     monster_info) == 2) {
            ApplyMonsterCondition(monster_info->location_id, 0xd, 1);
        }
    }
}

/* The five load categories and what each costs on top of an action's base
   fatigue, as a percentage. */
enum { W8_LOAD_CATEGORY_COUNT = 5 };
static const int kLoadFatiguePercent[W8_LOAD_CATEGORY_COUNT] = {0, 0x19, 0x32, 100, 200};

/* The band past which deep fatigue takes hold, and the band it lets go at. */
enum {
    W8_FATIGUE_BAND_DEEP = 2,
    W8_FATIGUE_BAND_RECOVERED = 2
};

extern void ApplyCharacterEffect(
    W8Character* character, void* effect, int arg_3, int arg_4, int arg_5);
/* 0x0052E690 */
extern void* g_effect_005ee598;
extern void ResetCombatSlot(W8CombatSlot* combat_slot);   /* 0x00536170 */
extern void RecordCharacterDeath(int party_slot);
extern void Function52F110(int party_slot);
extern int GetNpcState(int animation_id);
extern void DropCharacterFromRound(int party_slot);
extern void PlaySound(const char* path, int flags);

/* Tire one character. The load they are carrying scales the cost - eased or
   worsened by the two load modifiers - and the result is taken out of their
   stamina without going below nothing. Running out puts them under the
   exhausted condition; merely dropping into the deep band applies the
   deep-fatigue effect once. */
// FUNCTION: WIZ8 0x0052af50
void FatigueCharacter(int party_slot, int amount, char scale_by_load, int load_percent,
                      int report_to)
{
    W8Character* character = &g_party_characters[party_slot];
    int previous_band;
    int band;

    if (character->stamina <= 0 || character->hp_current == 0 || amount == 0) {
        return;
    }

    if (scale_by_load) {
        if ((unsigned int)character->load_category >= W8_LOAD_CATEGORY_COUNT) {
            srAssertFail("FALSE", HEALTH_STAMINA_MANA_CPP, 813,
                         "FatigueCharacter: ERROR - Invalid load category");
        }
        load_percent = kLoadFatiguePercent[character->load_category];
        if (character->condition_turns[W8_CONDITION_LOAD_EASED] == 0) {
            if (character->enchantments[5].value_08 != 0) {
                load_percent += 0x19;
            }
        }
        else {
            load_percent -= 0x19;
        }
        amount += (load_percent * amount) / 100;
    }

    if (amount < 0) {
        amount = 0;
    }
    else if (amount > character->stamina) {
        amount = character->stamina;
    }
    character->stamina -= amount;

    previous_band = character->fatigue_band;
    band = FatigueBandFromMissing(
        100 - (int)((character->stamina * 100) / (unsigned int)character->stamina_max));
    character->fatigue_band = band;
    if (band != previous_band) {
        CalcArmorClasses(character);
    }

    if (character->stamina < 1) {
        if ((unsigned int)character->condition_turns[W8_CONDITION_EXHAUSTED] < W8_CONDITION_INDEFINITE) {
            SetCharacterCondition(party_slot, W8_CONDITION_EXHAUSTED, W8_CONDITION_INDEFINITE,
                                  0, 0, report_to == 0);
            if (report_to != 0) {
                *(int*)((char*)report_to + 0x4c) += 1;
            }
        }
    }
    else if (band != previous_band && band > W8_FATIGUE_BAND_DEEP) {
        if (character->deep_fatigue_applied == 0) {
            ApplyCharacterEffect(character, g_effect_005ee598, 0,
                                 g_effect_argument_005ed8c8, g_effect_argument_005ed914);
            character->deep_fatigue_applied = 1;
        }
        if ((unsigned int)character->fatigue_band < W8_FATIGUE_BAND_RECOVERED) {
            character->deep_fatigue_applied = 0;
        }
    }
}

/* What one character action costs in fatigue before it is spent. An attack
   costs by the weight of what is swung - unarmed and the two special attack
   modes are flat rolls instead - a run costs a fifth of the character's whole
   stamina with a floor, and the rest are free. A character who tires twice as
   fast pays double. */
// FUNCTION: WIZ8 0x0052b2f0
unsigned int CharacterActionFatigueCost(int party_slot, int action_kind)
{
    unsigned int cost = 0;
    W8CombatCharacterRow* combat_row = &g_combat_character_rows[party_slot];
    int attack_mode;
    int item_id;
    int weight_bands;

    switch (action_kind) {
    case 0:
    case 1:
        attack_mode = g_party_slot_rows[party_slot].attack_mode[combat_row->current_hand];
        if (attack_mode == 5) {
            cost = Random(3) + 2;
        }
        else if (attack_mode == 6) {
            cost = Random(3) + 3;
        }
        else {
            item_id = g_party_characters[party_slot]
                          .equipment[combat_row->current_equip_slot]
                          .item_id;
            if (item_id == -1) {
                cost = Random(4) + 3;
            }
            else {
                weight_bands = g_item_records[item_id].weight / 0x28 + 1;
                cost = Random(weight_bands) + 1 + weight_bands;
            }
        }
        if (action_kind == 1) {
            cost *= 2;
        }
        break;
    case 2:
    case 4:
    case 5:
    case 7:
        break;
    case 3:
    case 6:
        cost = g_party_characters[party_slot].stamina_max / 5;
        if (cost < 0x14) {
            cost = 0x14;
        }
        break;
    case 8:
        cost = Random(2) + 1;
        break;
    case 9:
        cost = Random(4) + 3;
        break;
    default:
        cost = 0;
    }

    if (g_party_characters[party_slot].condition_turns[W8_CONDITION_FATIGUE_DOUBLED] != 0) {
        cost *= 2;
    }
    return cost;
}

/* Drain spell points from one named realm, taking no more than it holds.
   Announced with the realm's own name. */
// FUNCTION: WIZ8 0x0052b6d0
void DrainCharacterRealmSpellPoints(
    int party_slot, int realm, unsigned int amount, int unused, char announce)
{
    W8Character* character = &g_party_characters[party_slot];
    unsigned int available;

    if (character->hp_current == 0) {
        return;
    }
    available = character->sp_left[realm];
    if (available == 0) {
        return;
    }

    if (g_spell_points_free_00687500 != 0) {
        PostCharacterNotice(party_slot, (const wchar_t*)gppStringList[0x988 / 4], amount,
                            gppStringList[g_spell_realm_notice[realm]]);
        return;
    }

    if ((int)available < 0) {
        return;
    }
    if (amount >= available) {
        amount = available;
    }
    if (amount == 0) {
        return;
    }
    SpendCharacterSpellPoints(party_slot, realm, amount);
    if (announce) {
        WriteGameLog(8, (const wchar_t*)gppStringList[0x98c / 4], amount,
                     gppStringList[g_spell_realm_notice[realm]]);
    }
}

/* One character dies. Every condition but the tenth is lifted, both pools are
   emptied, the two targeting blocks on their slot row are cleared, combat
   forgets them as a participant, and whatever they were animating is told to
   stop. */
// FUNCTION: WIZ8 0x0052abf0
void CharacterDies(int party_slot)
{
    W8Character* character = &g_party_characters[party_slot];
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];
    unsigned int condition;
    int animation;

    if (row->flag_00 == 0) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", HEALTH_STAMINA_MANA_CPP, 561, 0);
    }

    ++character->death_count_09fd;
    for (condition = 0; condition < 0x12; ++condition) {
        if (condition != 10 && character->condition_turns[condition] != 0) {
            RemoveCharacterCondition(party_slot, condition, 0);
        }
    }
    character->hp_current = 0;
    character->stamina = 0;

    ResetCombatSlot(&row->target_out_of_combat);
    ResetCombatSlot(&row->target_in_combat);
    if (g_in_combat_00683f94 != 0) {
        RecordCharacterDeath(party_slot);
    }
    Function52F110(party_slot);
    PlaySound("Data\\Sound\\Misc\\CharacterDead.wav", 0);

    if (g_in_combat_00683f94 != 0) {
        if (g_combat_state->selected_character == party_slot) {
            g_combat_state->selected_slot = 0;
            g_combat_state->selected_character = -1;
        }
        row->pending_action = -1;
        g_combat_character_rows[party_slot].value_18 = 0;
        g_combat_character_rows[party_slot].flag_4c = 1;
        DropCharacterFromRound(party_slot);
    }

    animation = row->animation_0fa;
    if (animation != -1) {
        animation = GetNpcState(animation);
        if (animation != 0) {
            *(unsigned short*)(animation + 4) = 1;
        }
    }
}
