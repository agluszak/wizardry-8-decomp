#include "wiz8/gameplay_boundaries.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sr_api.h"

#include <string.h>

/*
 * Local Code\Combat Attack.cpp.
 *
 * Choosing and resolving one swing: which attack mode is used, whether a
 * combatant can attack at all, and how much of the damage the target keeps.
 */

#define COMBAT_ATTACK_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Combat Attack.cpp"

/* Nine attack modes, one bit each, held in the low half of a word. */
enum { W8_ATTACK_MODE_COUNT = 9 };

/* Bit two of the monster record's flag word: the monster attacks at all. */
enum { W8_MONSTER_FLAG_ATTACKS = 4 };

extern unsigned char CharacterIsEngaged(unsigned int party_slot);        /* 0x00524A10 */
extern unsigned char RateMonsterAttack(
    W8MonsterInfo* monster_info, int target, unsigned int attack, int arg_4, int arg_5);
/* 0x0053D4B0 */

/* Clear a forty-eight byte attack block. */
// FUNCTION: WIZ8 0x00543260
void ClearAttackBlock(void* block)
{
    memset(block, 0, 0x30);
}

/* Fold the attack modes that share a resolution onto the mode that resolves
   them. Seven of the eighteen collapse onto one mode and two onto another;
   the rest answer for themselves. */
// FUNCTION: WIZ8 0x005430c0
int NormalizeAttackMode(int attack_mode)
{
    switch (attack_mode) {
    case 9:
    case 10:
    case 11:
    case 12:
    case 14:
    case 15:
    case 16:
        return 6;
    case 13:
    case 17:
        return 7;
    }
    return attack_mode;
}

/* Pick one of the attack modes a mask allows, at random. The walk wraps round
   the nine bits, counting only the ones that are set, until it has passed as
   many as the roll asked for - so a mask with one bit always answers that bit
   however the roll came out. */
// FUNCTION: WIZ8 0x00542db0
unsigned int ChooseAttackMode(unsigned int attack_modes)
{
    unsigned int wanted;
    unsigned int seen = 0;
    unsigned int mode = 0;

    if ((short)attack_modes == 0) {
        srAssertFail("fsAttackModes != 0", COMBAT_ATTACK_CPP, 3557, 0);
    }

    wanted = Random(W8_ATTACK_MODE_COUNT);
    while (((attack_modes & 0xffff) & (1 << mode)) == 0 || seen++ < wanted) {
        ++mode;
        if (mode > 8) {
            mode = 0;
        }
    }
    return mode;
}

/* Whether one character can swing this round: engaged, in better shape than
   the attack threshold, and with the first hand in play. */
// FUNCTION: WIZ8 0x00545b80
bool CanCharacterAttack(int party_slot)
{
    const W8Character* character = &g_party_characters[party_slot];

    if (!CharacterIsEngaged(party_slot)) {
        return false;
    }
    if (character->unknown_0b01 > 0xb) {
        return false;
    }
    return character->hand_attacks[0].in_play != 0;
}

/* Whether one monster can. It has to be in the world, in combat, alive, below
   the deactivation threshold, marked as attacking at all by its record, and
   actually have a first attack. */
// FUNCTION: WIZ8 0x00545bd0
bool CanMonsterAttack(W8MonsterInfo* monster_info)
{
    const W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);

    if (monster_info->flag_14 == 0 || monster_info->fInCombat == 0 ||
        monster_info->hp_current == 0 || (unsigned int)monster_info->value_107 >= 0xc ||
        (record->flags_0d0 & W8_MONSTER_FLAG_ATTACKS) == 0) {
        return false;
    }
    return record->attacks[0].fHasAttack != 0;
}

/* How much of a hit the target actually takes. The two reductions add, the
   remainder is taken as a percentage rounding to nearest, and nothing goes
   below zero. */
// FUNCTION: WIZ8 0x005459b0
int ApplyDamageReduction(
    const W8MonsterInfo* monster_info, const W8MonsterRecord* record, int damage)
{
    int reduction = monster_info->runtime_block_1db.damage_reduction +
                    record->damage_reduction;

    if (reduction != 0) {
        damage = ((100 - reduction) * damage + 50) / 100;
    }
    if (damage < 0) {
        return 0;
    }
    return damage;
}

/* How good this monster's best attack on a target is. A motionless monster is
   always rated at one; otherwise every one of its three attacks is rated and
   the best kept, with any attack rating zero stopping the walk outright. */
// FUNCTION: WIZ8 0x0053d450
unsigned char RateMonsterBestAttack(W8MonsterInfo* monster_info, int target, int arg_3)
{
    unsigned char best = 0;
    unsigned char rating;
    unsigned int attack;

    if (monster_info->motionless != 0) {
        return 1;
    }
    for (attack = 0; attack < W8_MAX_MONSTER_ATTACKS; ++attack) {
        rating = RateMonsterAttack(monster_info, target, attack, 1, arg_3);
        if (rating == 0) {
            return 0;
        }
        if (rating > best) {
            best = rating;
        }
    }
    return best;
}

/* HAND_COUNT, named by the assertion that bounds every hand argument here. */
enum { W8_HAND_COUNT = 2 };

/* The skill practised whenever the character's own damage reduction is used. */
enum { W8_SKILL_DAMAGE_REDUCTION = 0x25 };

extern int CalcRangeCategoryToTarget(const W8Character* character, int hand);
/* 0x00519AC0 */
extern void PracticeCharacterSkill(W8Character* character, int skill, int amount, int arg_4);
extern unsigned char MonsterHasAttackOn(W8MonsterInfo* monster_info, W8CombatSlot* target);
/* 0x00545CF0 */
extern unsigned char CharacterHasAttackOn(int party_slot, W8CombatSlot* target);
/* 0x00545C20 */
extern unsigned char TargetIsInPlay(int party_slot, int arg_2, int arg_3);   /* 0x00536F60 */

/* Whether one of a character's hands can reach the target it is aimed at: the
   hand has to be in play and to have a range category at all. */
// FUNCTION: WIZ8 0x0053d2a0
bool CanHandReachTarget(int party_slot, unsigned int hand)
{
    if (hand >= W8_HAND_COUNT) {
        srAssertFail("uiHand < HAND_COUNT", COMBAT_ATTACK_CPP, 102, 0);
    }
    if (g_party_characters[party_slot].hand_attacks[hand].in_play == 0) {
        return false;
    }
    return CalcRangeCategoryToTarget(&g_party_characters[party_slot], hand) != -1;
}

/* Whether either hand can. */
// FUNCTION: WIZ8 0x0053d310
bool CanAnyHandReachTarget(int party_slot)
{
    unsigned int hand;

    for (hand = 0; hand < W8_HAND_COUNT; ++hand) {
        if (hand >= W8_HAND_COUNT) {
            srAssertFail("uiHand < HAND_COUNT", COMBAT_ATTACK_CPP, 102, 0);
        }
        if (g_party_characters[party_slot].hand_attacks[hand].in_play != 0 &&
            CalcRangeCategoryToTarget(&g_party_characters[party_slot], hand) != -1) {
            return true;
        }
    }
    return false;
}

/* What one hand's attack is worth, or nothing when it cannot reach. */
// FUNCTION: WIZ8 0x0053d7f0
int GetHandAttackValue(int party_slot, unsigned int hand)
{
    if (hand >= W8_HAND_COUNT) {
        srAssertFail("uiHand < HAND_COUNT", COMBAT_ATTACK_CPP, 102, 0);
    }
    if (g_party_characters[party_slot].hand_attacks[hand].in_play != 0 &&
        CalcRangeCategoryToTarget(&g_party_characters[party_slot], hand) != -1) {
        return g_party_characters[party_slot].hand_attacks[hand].attack_value;
    }
    return 0;
}

/* How much of a hit a character keeps. Their own reduction is taken as a
   percentage rounding to nearest, and using it practises the skill it comes
   from - but only for a character who has that skill at all. */
// FUNCTION: WIZ8 0x00545950
int ApplyCharacterDamageReduction(W8Character* character, int damage)
{
    if (character->damage_reduction != 0) {
        damage = ((100 - character->damage_reduction) * damage + 50) / 100;
    }
    if (damage < 0) {
        damage = 0;
    }
    if (character->skills[W8_SKILL_DAMAGE_REDUCTION].flag_00 != 0) {
        PracticeCharacterSkill(character, W8_SKILL_DAMAGE_REDUCTION, 1, 0);
    }
    return damage;
}

/* Whether one monster has an attack it could make on what it is aimed at. The
   same six checks CanMonsterAttack makes, and then the attack itself. */
// FUNCTION: WIZ8 0x00545b20
bool CanMonsterAttackItsTarget(W8MonsterInfo* monster_info)
{
    const W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);

    if (monster_info->flag_14 != 0 && monster_info->fInCombat != 0 &&
        monster_info->hp_current != 0 && (unsigned int)monster_info->value_107 < 0xc &&
        (record->flags_0d0 & W8_MONSTER_FLAG_ATTACKS) != 0 &&
        record->attacks[0].fHasAttack != 0) {
        return MonsterHasAttackOn(monster_info, &monster_info->combat_slot_2ba) != 0;
    }
    return false;
}

/* The character counterpart: the target has to still be in play, the character
   engaged and in shape, the first hand in play, and the attack itself has to
   come off. */
// FUNCTION: WIZ8 0x00545aa0
bool CanCharacterAttackItsTarget(int party_slot)
{
    W8Character* character;

    if (!TargetIsInPlay(party_slot, 0, 0)) {
        return false;
    }
    if (!CharacterIsEngaged(party_slot)) {
        return false;
    }
    character = &g_party_characters[party_slot];
    if (character->unknown_0b01 >= 0xc || character->hand_attacks[0].in_play == 0) {
        return false;
    }
    return CharacterHasAttackOn(
               party_slot, &g_party_slot_rows[party_slot].target_out_of_combat) != 0;
}

/* What an attack mode is worth to hit with, which depends on whether the
   attacker is a monster or a character - the same nine modes score differently
   for each. Its error text carries the function's own name. */
// FUNCTION: WIZ8 0x00542e10
int AttackModeMod(int is_character, int attack_mode)
{
    if (is_character == 0) {
        switch (attack_mode) {
        case 0:
        case 2:
        case 8:
            return 0;
        case 1:
        case 6:
            return -10;
        case 3:
            return -30;
        case 4:
            return -5;
        case 5:
            return 5;
        case 7:
            return 10;
        default:
            srAssertFail("FALSE", COMBAT_ATTACK_CPP, 3626,
                         "AttackModeMod: ERROR - Invalid attack mode");
            return 0;
        }
    }
    switch (attack_mode) {
    case 0:
    case 6:
        return 0;
    case 1:
    case 8:
        return 10;
    case 2:
    case 5:
        return -5;
    case 3:
        return -30;
    case 4:
        return 5;
    case 7:
        return -10;
    default:
        srAssertFail("FALSE", COMBAT_ATTACK_CPP, 3663,
                     "AttackModeMod: ERROR - Invalid attack mode");
        return 0;
    }
}
