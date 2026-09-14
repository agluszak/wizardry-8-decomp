#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/sr_api.h"
#include "random.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/CombatSound.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/notices.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"
#include "soundman.h"

#include <string.h>
#include "wiz8/character_skills.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_screens/OptionsScreen.h"

/*
 * Local Code\Combat Attack.cpp.
 *
 * Choosing and resolving one swing: which attack mode is used, whether a
 * combatant can attack at all, and how much of the damage the target keeps.
 */

#define COMBAT_ATTACK_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Combat Attack.cpp"

// FUNCTION: WIZ8 0x00546a70
void GetCharacterHandDamageDice(const W8Character* character, int hand, W8Dice* dice)
{
    if (character->hand_attacks[hand].wield_kind == 0) {
        *dice = character->hand_attacks[hand].damage_dice;
        return;
    }
    int slot;
    if (hand == 0) {
        slot = 6;
        if (ItemHasSingledOutGenericName(character->equipment[6].item_id)) {
            int partner = GetPairedEquipSlot(6);
            if (partner != -1)
                slot = partner;
        }
    } else {
        slot = 7;
    }
    *dice = g_item_records[character->equipment[slot].item_id].damage_dice;
}

// FUNCTION: WIZ8 0x00546b10
int GetCharacterHandDamageBonus(const W8Character* character, int hand)
{
    return character->hand_attacks[hand].value_29 + character->bonus_1770.value_03;
}

/* Nine attack modes, one bit each, held in the low half of a word. */
enum { W8_ATTACK_MODE_COUNT = 9 };

/* Bit two of the monster record's flag word: the monster attacks at all. */
enum { W8_MONSTER_FLAG_ATTACKS = 4 };

/* Clear a forty-eight byte attack block. */
// FUNCTION: WIZ8 0x00543260
void ClearAttackBlock(W8MissileAttackBlock* block)
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
    const W8Character* character = &g_status_685170.buffers.characters[party_slot];

    if (!IsPartySlotEligible00524A10(party_slot)) {
        return false;
    }
    if (character->highest_condition > 0xb) {
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

    if (monster_info->fActive == 0 || monster_info->fInCombat == 0 ||
        monster_info->hp_current == 0 || (unsigned int)monster_info->highest_condition >= 0xc ||
        (record->flags_0d0 & W8_MONSTER_FLAG_ATTACKS) == 0) {
        return false;
    }
    return record->attacks[0].fHasAttack != 0;
}

/* How much of a hit the target actually takes. The two reductions add, the
   remainder is taken as a percentage rounding to nearest, and nothing goes
   below zero. */
// FUNCTION: WIZ8 0x005459b0
int ApplyDamageReduction(const W8MonsterInfo* monster_info, const W8MonsterRecord* record,
                         int damage)
{
    int reduction =
        monster_info->modifiers_1db.damage_reduction_adjustment + record->damage_reduction;

    if (reduction != 0) {
        damage = ((100 - reduction) * damage + 50) / 100;
    }
    if (damage < 0) {
        return 0;
    }
    return damage;
}

/* Why one of a monster's attacks cannot be made, or zero when it can. An
   attack the record lacks is simply not usable, and so is one whose data is
   broken, which is reported. Otherwise the attack is out of reach unless it
   reaches anyone, judged with the monster's current action set aside. */
// FUNCTION: WIZ8 0x0053d4b0
unsigned char RateMonsterAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                unsigned int attack, int unused, int hostile_only)
{
    int action_kind;
    unsigned char reaches;

    if (attack >= W8_MAX_MONSTER_ATTACKS) {
        srAssertFail("uiAttack < MAX_MONSTER_ATTACKS", COMBAT_ATTACK_CPP, 235, 0);
    }
    if (record->attacks[attack].fHasAttack == 0) {
        return W8_MONSTER_ATTACK_NOT_USABLE;
    }
    if (record->attacks[attack].attack_modes == 0) {
        FormatDebugMessage(0, "DATA ERROR: %ls has 0 attack modes for attack %d", record, attack);
        return W8_MONSTER_ATTACK_NOT_USABLE;
    }
    if (record->attacks_per_round_0e5 == 0) {
        FormatDebugMessage(0, "DATA ERROR: %ls has 0 ATTACKS/round", record);
        return W8_MONSTER_ATTACK_NOT_USABLE;
    }
    if (record->swings_per_round_0e6 == 0) {
        FormatDebugMessage(0, "DATA ERROR: %ls has 0 SWINGS/round", record);
        return W8_MONSTER_ATTACK_NOT_USABLE;
    }
    action_kind = monster_info->action_kind;
    monster_info->action_kind = 0;
    reaches = MonsterAttackReachesAnyone(monster_info, attack, hostile_only);
    monster_info->action_kind = action_kind;
    return !reaches ? W8_MONSTER_ATTACK_OUT_OF_REACH : W8_MONSTER_ATTACK_USABLE;
}

/* Why a monster cannot attack at all, or zero when some attack is usable. A
   motionless monster is always rated at one; otherwise each of its three
   attacks is rated, a usable one answering at once and the highest reason
   otherwise kept. */
// FUNCTION: WIZ8 0x0053d450
unsigned char RateMonsterBestAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                    int hostile_only)
{
    unsigned char best;
    unsigned char rating;
    unsigned int attack;

    if (monster_info->fMotionless != 0) {
        return 1;
    }
    best = 0;
    for (attack = 0; attack < W8_MAX_MONSTER_ATTACKS; ++attack) {
        rating = RateMonsterAttack(monster_info, record, attack, 1, hostile_only);
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

/* The ability that lets a character choose the knock-out action. */
enum { W8_TRAIT_KNOCK_OUT = 0x14 };

/* The skill practised whenever the character's own damage reduction is used. */
enum { W8_SKILL_DAMAGE_REDUCTION = 0x25 };

/* Whether one of a character's hands can reach the target it is aimed at: the
   hand has to be in play and to have a range category at all. */
// FUNCTION: WIZ8 0x0053d2a0
bool CanHandReachTarget(int party_slot, unsigned int hand)
{
    if (hand >= W8_HAND_COUNT) {
        srAssertFail("uiHand < HAND_COUNT", COMBAT_ATTACK_CPP, 102, 0);
    }
    if (g_status_685170.buffers.characters[party_slot].hand_attacks[hand].in_play == 0) {
        return false;
    }
    return CalcRangeCategoryToTarget(&g_status_685170.buffers.characters[party_slot], hand) != -1;
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
        if (g_status_685170.buffers.characters[party_slot].hand_attacks[hand].in_play != 0 &&
            CalcRangeCategoryToTarget(&g_status_685170.buffers.characters[party_slot], hand) !=
                -1) {
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
    if (g_status_685170.buffers.characters[party_slot].hand_attacks[hand].in_play != 0 &&
        CalcRangeCategoryToTarget(&g_status_685170.buffers.characters[party_slot], hand) != -1) {
        return g_status_685170.buffers.characters[party_slot].hand_attacks[hand].attacks;
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

/* Whether a character can knock out: the ability itself, a hand that can reach
   the target, and a primary hand fighting at short range or closer. */
// FUNCTION: WIZ8 0x005458a0
unsigned char CanCharacterKnockOut(int party_slot)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    unsigned int hand;

    if (CharacterHasTrait00547940(character, W8_TRAIT_KNOCK_OUT) == 0) {
        return 0;
    }
    for (hand = 0; hand < W8_HAND_COUNT; ++hand) {
        if (hand >= W8_HAND_COUNT) {
            srAssertFail("uiHand < HAND_COUNT", COMBAT_ATTACK_CPP, 102, 0);
        }
        if (g_status_685170.buffers.characters[party_slot].hand_attacks[hand].in_play != 0 &&
            CalcRangeCategoryToTarget(&g_status_685170.buffers.characters[party_slot], hand) !=
                -1) {
            return CalcRangeCategoryToTarget(character, 0) <= W8_RANGE_SHORT;
        }
    }
    return 0;
}

/* Whether a character could attack what a combat slot names. A party member
   has to be in play and not screened off by the front rank; a monster has to be
   engaged, alive, targetable and within the character's reach. */
// FUNCTION: WIZ8 0x00545c20
unsigned char CharacterHasAttackOn(int party_slot, W8CombatSlot* target)
{
    W8MonsterInfo* monster_info;
    int target_slot;

    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        target_slot = target->iChar;
        if (!CanPartySlotParticipate(target_slot)) {
            return 0;
        }
        if (FrontRankScreens(party_slot, target_slot) > 0) {
            return 0;
        }
    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(5927, COMBAT_ATTACK_CPP, target->iMonsterID, 1));
        if (monster_info->fActive == 0 || monster_info->hp_current == 0 ||
            monster_info->fInCombat == 0) {
            return 0;
        }
        if (GetMonsterDataForInfo(monster_info)->untargetable_24a != 0) {
            return 0;
        }
        if (Function5194E0(party_slot, 0, monster_info,
                           g_combat_state->characters[party_slot].flag_34 == 0, 0) == 0) {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

/* Whether a monster would press an attack on what a combat slot names. The
   target has to be a hostile the monster's first attack reaches; beyond that
   the monster only takes on something that outranks it when it is below forty
   percent health or out of formation. */
// FUNCTION: WIZ8 0x00545cf0
unsigned char MonsterHasAttackOn(W8MonsterInfo* monster_info, W8CombatSlot* target)
{
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    W8Character* character;
    W8MonsterInfo* target_info;
    int target_slot;
    unsigned int target_level;
    unsigned int hp_percent;
    unsigned char out_of_formation;

    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        target_slot = target->iChar;
        if (!CanPartySlotParticipate(target_slot)) {
            return 0;
        }
        if (MonsterVsCharDisposition(target_slot, monster_info) != 2) {
            return 0;
        }
        if (MonsterAttackReachesCharacter(monster_info, record, 0, target_slot) == 0) {
            return 0;
        }
        character = &g_status_685170.buffers.characters[target_slot];
        hp_percent = character->hp_current * 100 / character->hp_max;
        target_level = character->level;
        out_of_formation = character->bonus_1770.out_of_formation;
    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
        target_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(5997, COMBAT_ATTACK_CPP, target->iMonsterID, 1));
        if (target_info->fActive == 0 || target_info->hp_current == 0 ||
            target_info->fInCombat == 0) {
            return 0;
        }
        if (GetMonsterDataForInfo(target_info)->untargetable_24a != 0) {
            return 0;
        }
        if (MonsterHostility00546F80(monster_info, target_info) != 2) {
            return 0;
        }
        if (MonsterAttackReachesMonster(monster_info, record, 0, target_info) == 0) {
            return 0;
        }
        target_level = GetMonsterDataForInfo(target_info)->missile_value_24f;
        hp_percent = target_info->hp_current * 100 / (unsigned int)target_info->hp_max;
        out_of_formation = monster_info->modifiers_1db.out_of_formation;
    } else {
        return 0;
    }
    if (target_level > record->missile_value_24f && (hp_percent < 40 || out_of_formation != 0)) {
        return 1;
    }
    return 0;
}

/* Whether one monster has an attack it could make on what it is aimed at. The
   same six checks CanMonsterAttack makes, and then the attack itself. */
// FUNCTION: WIZ8 0x00545b20
bool CanMonsterAttackItsTarget(W8MonsterInfo* monster_info)
{
    const W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);

    if (monster_info->fActive != 0 && monster_info->fInCombat != 0 &&
        monster_info->hp_current != 0 && (unsigned int)monster_info->highest_condition < 0xc &&
        (record->flags_0d0 & W8_MONSTER_FLAG_ATTACKS) != 0 && record->attacks[0].fHasAttack != 0) {
        return MonsterHasAttackOn(monster_info, &monster_info->Target) != 0;
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

    if (!TargetIsInPlay(party_slot, 0, W8_TARGETING_CONTEXT_OUT_OF_COMBAT)) {
        return false;
    }
    if (!IsPartySlotEligible00524A10(party_slot)) {
        return false;
    }
    character = &g_status_685170.buffers.characters[party_slot];
    if (character->highest_condition >= 0xc || character->hand_attacks[0].in_play == 0) {
        return false;
    }
    return CharacterHasAttackOn(
               party_slot, &g_status_685170.buffers.party_rows[party_slot].target_out_of_combat) !=
           0;
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

// GLOBAL: WIZ8 0x0061e7b0
const unsigned short g_pc_hit_location_labels[W8_PC_HIT_LOCATIONS + 1][2] = {
    {0x429, 0x42a}, {0x42b, 0x42c}, {0x42d, 0x42e}, {0x42f, 0x430}, {0x431, 0x432}, {0x433, 0x434},
};

// clang-format off
// GLOBAL: WIZ8 0x0061ea24
const unsigned short g_monster_hit_location_labels[W8_MONSTER_HIT_LOCATIONS][W8_MONSTER_BODY_TYPES] = {
    {0x55c, 0x563, 0x56a, 0x55c, 0x571, 0x572}, {0x55d, 0x564, 0x56b, 0x55d, 0x572, 0x575},
    {0x55e, 0x565, 0x56c, 0x55e, 0x55e, 0x55e}, {0x55f, 0x566, 0x56d, 0x55f, 0x573, 0x576},
    {0x560, 0x567, 0x56e, 0x560, 0x574, 0x577}, {0x561, 0x568, 0x56f, 0x568, 0x560, 0x560},
    {0x562, 0x569, 0x570, 0x562, 0x55c, 0x55d},
};
// clang-format on

/* Land a spell missile on whatever it struck. The spell's own impact sound
   plays when it has one. A single-target effect (no radius) rolls its size,
   applies it to the struck monster or character and rolls the effect's
   conditions against the target. An area effect instead reaches the whole
   party when a character was struck, or every active monster within the
   radius of the struck monster's position. */
// FUNCTION: WIZ8 0x00544d30
void ResolveSpellMissileHit(W8Missile* missile)
{
    W8TargetSource* source = &missile->source_22c;
    W8CombatSlot* target = &missile->combat_slot_260;
    W8SpellEffectDefinition* definition = &missile->definition_1fc;
    W8SpellRuntimeRecord* spell;
    W8CombatSlot struck;
    unsigned int magnitude;
    unsigned int monster_list_index;
    W8MonsterInfo* monster_info;
    srVector3T<float> location;
    unsigned char announce;
    unsigned char verbose;
    unsigned int index;

    TargetSourceIsCharacter(source, 0);
    spell = &g_spell_records[MissileSpellId(missile->missile_table_index_1d8)];
    if (strlen(spell->sound_name) != 0) {
        SoundPlay((STR)FormatString("Data\\Missiles\\Sounds\\%s.wav", spell->sound_name), 0);
    }

    if (definition->radius > g_float_005ebb34) {
        if (target->iType == W8_TARGET_KIND_MONSTER) {
            monster_list_index =
                MonsterGetIndexByLocationID(0x1460, COMBAT_ATTACK_CPP, target->iMonsterID, 1);
            monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
            DamageMonstersInRadius(monster_info->monster->GetPosition(), definition->radius,
                                   &definition->magnitude, source, &missile->result_280);
            announce = g_settings_6850c8.verbose_combat_messages;
            verbose = g_settings_6850c8.verbose_combat_messages;
            srVector3T<float> center = monster_info->monster->GetPosition();
            ResetCombatSlot(&struck);
            struck.iType = W8_TARGET_KIND_MONSTER;
            for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
                monster_info = MonsterGetScriptPartByLocationIndex(index);
                if (monster_info->fActive != 0) {
                    MonsterGetLocation(monster_info->monster, &location);
                    srVector3T<float> offset(center.x - location.x, center.y - location.y,
                                             center.z - location.z);
                    if (offset.Length() <= definition->radius) {
                        struck.iMonsterID = monster_info->location_id;
                        ApplyEffectConditions(source, &struck, definition, announce, verbose, 0);
                    }
                }
            }
        } else {
            ApplyRolledHealthChangeToParty(&definition->magnitude, &missile->result_280, 1);
            announce = g_settings_6850c8.verbose_combat_messages;
            verbose = g_settings_6850c8.verbose_combat_messages;
            ResetCombatSlot(&struck);
            struck.iType = W8_TARGET_KIND_CHARACTER;
            for (index = 0; index < 8; ++index) {
                if (g_status_685170.buffers.party_rows[index].occupied != 0) {
                    struck.iChar = index;
                    ApplyEffectConditions(source, &struck, definition, announce, verbose, 0);
                }
            }
        }
        return;
    }

    magnitude = RollEffectMagnitude(definition);
    if (magnitude > 0) {
        ApplyEffectAndAnnounce(&magnitude, target, spell->realm, definition->power_level);
        if (magnitude > 0) {
            if (target->iType == W8_TARGET_KIND_MONSTER) {
                monster_list_index =
                    MonsterGetIndexByLocationID(0x147f, COMBAT_ATTACK_CPP, target->iMonsterID, 1);
                monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
                ApplyDamageToMonster(monster_info, magnitude, source, 0,
                                     g_settings_6850c8.verbose_combat_messages, 0,
                                     &missile->result_280, 0);
            } else {
                ApplyDamageToCharacter(target->iChar, magnitude, 0,
                                       g_settings_6850c8.verbose_combat_messages, 0,
                                       &missile->result_280, 0);
            }
        }
    }
    ApplyEffectConditions(source, target, definition, g_settings_6850c8.verbose_combat_messages,
                          g_settings_6850c8.verbose_combat_messages, &missile->result_280);
}

/* Land a physical missile on its target. The notice names the target, with
   the retargeted suffix when the shot was turned aside onto it, and colours
   the name by side. A deflected shot only reports the deflection. Otherwise
   the hit location is rolled from the monster body's own chances or the
   party's gubLocalACPercent, named in verbose mode, and the shot must still
   penetrate the armour there: a 50 percent base, plus the attack mode's
   modifier and 5 per point of armour class under 10. A penetrating shot
   rolls the effect size and, when it is not nothing, plays the hit sound,
   applies the damage and rolls the effect's conditions. */
// FUNCTION: WIZ8 0x00545090
void ResolveMissileHit(W8Missile* missile, bool deflected)
{
    W8TargetSource* source = &missile->source_22c;
    W8CombatSlot* target = &missile->combat_slot_260;
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    wchar_t text[120];
    wchar_t location_name[20];
    unsigned int monster_list_index;
    unsigned char target_start;
    unsigned char target_stop;
    char source_color;
    char target_color;
    unsigned int hit_location;
    unsigned int total;
    unsigned int roll;
    int attack_mode;
    int chance;
    int penetration_roll;
    unsigned int magnitude;

    if (!IsTargetStillPresent(target)) {
        return;
    }
    TargetSourceIsCharacter(source, 0);
    swprintf(text, L"%s ", gppStringList[0x6fc / 4]);
    target_start = wcslen(text);
    if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster_list_index =
            MonsterGetIndexByLocationID(0x14b0, COMBAT_ATTACK_CPP, target->iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        wcscat(text, GetMonsterName(monster_info, 0, 0));
    } else {
        wcscat(text, g_status_685170.buffers.characters[target->iChar].name);
    }
    if (missile->retargeted_322) {
        wcscat(text, L" ");
        wcscat(text, gppStringList[0x700 / 4]);
    }
    target_stop = wcslen(text);
    source_color = GetSourceNoticeColor(source);
    target_color = GetTargetNoticeColor(source, target);
    ShowNotice(source_color, text, -1, -1, 0);
    if (target_color != source_color) {
        HighlightTextBoxRange(target_color, target_start, target_stop, -1);
    }
    if (deflected) {
        ShowNotice(source_color, gppStringList[0x850 / 4], -1, -1, 0);
        return;
    }

    if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster_list_index =
            MonsterGetIndexByLocationID(0x14d9, COMBAT_ATTACK_CPP, target->iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        record = GetMonsterDataForInfo(monster_info);
        total = 0;
        roll = Random(100);
        for (hit_location = 0;; ++hit_location) {
            if (hit_location >= W8_MONSTER_HIT_LOCATIONS) {
                FormatDebugMessage(0, "ERROR: DBS Hit Locations total only %d%% for monster %ls",
                                   total, record->name_00);
                hit_location = 3;
                break;
            }
            total += record->hit_location_chances_15f[hit_location];
            if (roll < total) {
                break;
            }
        }
        if (record->hit_location_chances_15f[hit_location] < 100) {
            wcscpy(
                location_name,
                gppStringList[g_monster_hit_location_labels[hit_location][record->body_type_15e]]);
        } else {
            wcscpy(location_name, &g_wchar_00689b34);
        }
    } else {
        total = 0;
        roll = Random(100);
        for (hit_location = 0;; ++hit_location) {
            if (hit_location >= W8_PC_HIT_LOCATIONS) {
                FormatDebugMessage(1, "ERROR: gubLocalACPercent total only %d%%");
                hit_location = 1;
                break;
            }
            total += gubLocalACPercent[hit_location];
            if (roll < total) {
                break;
            }
        }
        wcscpy(location_name, gppStringList[g_pc_hit_location_labels[hit_location][0]]);
    }
    if (g_settings_6850c8.verbose_combat_messages != 0) {
        WriteGameLog(source_color, gppStringList[0x830 / 4], location_name);
    }

    attack_mode = g_missile_table_65bde0[missile->missile_table_index_1d8].attack_mode_144;
    chance = AttackModeMod(1, attack_mode) + 50 +
             (10 - TargetArmorClassAtLocation(target, attack_mode, hit_location)) * 5;
    penetration_roll = Random(100) + 1;
    CombatLog("TO PENETRATE: Chance %d, Rolled %d", chance, penetration_roll);
    if (penetration_roll <= chance) {
        magnitude = RollEffectMagnitude(&missile->definition_1fc);
        if (magnitude > 0) {
            MakePCHitSound(missile, target, hit_location, -1);
            if (target->iType == W8_TARGET_KIND_MONSTER) {
                ApplyDamageToMonster(monster_info, magnitude, source, 0, 1, 1, 0, 0);
            } else {
                ApplyDamageToCharacter(target->iChar, magnitude, 0, 1, 1, 0, 0);
            }
            ApplyEffectConditions(source, target, &missile->definition_1fc, 1, 0, 0);
        } else if (g_settings_6850c8.verbose_combat_messages != 0) {
            WriteGameLog(source_color, gppStringList[0x838 / 4]);
        }
    } else {
        WriteGameLog(source_color, gppStringList[0x83c / 4]);
        MakePCHitSound(missile, target, hit_location, -1);
    }
}
