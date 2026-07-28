#include "wiz8/gameplay_boundaries.h"
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
// FUNCTION: WIZ8 0x005430C0
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
// FUNCTION: WIZ8 0x00542DB0
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
// FUNCTION: WIZ8 0x00545B80
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
// FUNCTION: WIZ8 0x00545BD0
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
// FUNCTION: WIZ8 0x005459B0
int ApplyDamageReduction(
    const W8MonsterInfo* monster_info, const W8MonsterRecord* record, int damage)
{
    int reduction = monster_info->damage_reduction + record->damage_reduction;

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
// FUNCTION: WIZ8 0x0053D450
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
