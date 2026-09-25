#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
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
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/engine_code/stCube.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/notices.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"
#include "soundman.h"
#include "wiz8/local_code/character_events.h"

#include <math.h>
#include <string.h>
#include "wiz8/character_event_queue.h"
#include "wiz8/character_skills.h"
#include "wiz8/fact_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/regions.h"
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
    if (character->Hand[hand].uiHolds == HOLDS_NOTHING) {
        *dice = character->Hand[hand].damage_dice;
        return;
    }
    int slot;
    if (hand == 0) {
        slot = 6;
        if (ItemHasSingledOutGenericName(character->EquippedItem[6].iItemNo)) {
            int partner = GetPairedEquipSlot(6);
            if (partner != -1)
                slot = partner;
        }
    } else {
        slot = 7;
    }
    *dice = g_item_records[character->EquippedItem[slot].iItemNo].damage_dice;
}

// FUNCTION: WIZ8 0x00546b10
int GetCharacterHandDamageBonus(const W8Character* character, int hand)
{
    return character->Hand[hand].damage_percent_29 + character->bonus_1770.damage_percent_03;
}

/* Nine attack modes, one bit each, held in the low half of a word. */
enum { W8_ATTACK_MODE_COUNT = 9 };

/* Bit two of the monster record's flag word: the monster attacks at all. */
enum { W8_MONSTER_FLAG_ATTACKS = 4 };

/* Clear a forty-eight byte attack block. */
// FUNCTION: WIZ8 0x00543260
void ClearAttackBlock(W8SpellEffectDefinition* block)
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
    const W8Character* character = &g_status.buffers.Char[party_slot];

    if (!IsPartySlotEligible(party_slot)) {
        return false;
    }
    if (character->highest_condition > 0xb) {
        return false;
    }
    return character->Hand[0].in_play != 0;
}

/* Whether one monster can. It has to be in the world, in combat, alive, below
   the deactivation threshold, marked as attacking at all by its record, and
   actually have a first attack. */
// FUNCTION: WIZ8 0x00545bd0
bool CanMonsterAttack(W8MonsterInfo* monster_info)
{
    const W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);

    if (monster_info->fActive == 0 || monster_info->fInCombat == 0 ||
        monster_info->hp_current == 0 || monster_info->highest_condition >= 0xc ||
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
    if (g_status.buffers.Char[party_slot].Hand[hand].in_play == 0) {
        return false;
    }
    return GetCharAttackRange(&g_status.buffers.Char[party_slot], hand) != -1;
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
        if (g_status.buffers.Char[party_slot].Hand[hand].in_play != 0 &&
            GetCharAttackRange(&g_status.buffers.Char[party_slot], hand) != -1) {
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
    if (g_status.buffers.Char[party_slot].Hand[hand].in_play != 0 &&
        GetCharAttackRange(&g_status.buffers.Char[party_slot], hand) != -1) {
        return g_status.buffers.Char[party_slot].Hand[hand].attacks;
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
    if (character->skills[W8_SKILL_DAMAGE_REDUCTION].active_00 != 0) {
        PracticeCharacterSkill(character, W8_SKILL_DAMAGE_REDUCTION, 1, 0);
    }
    return damage;
}

/* Whether a character can berserk: the ability itself, a hand that can reach
   the target, and a primary hand fighting at short range or closer. */
// FUNCTION: WIZ8 0x005458a0
bool CanCharacterBerserk(int party_slot)
{
    W8Character* character = &g_status.buffers.Char[party_slot];
    unsigned int hand;

    if (CharacterHasTrait(character, W8_TRAIT_BERSERK) == 0) {
        return 0;
    }
    for (hand = 0; hand < W8_HAND_COUNT; ++hand) {
        if (hand >= W8_HAND_COUNT) {
            srAssertFail("uiHand < HAND_COUNT", COMBAT_ATTACK_CPP, 102, 0);
        }
        if (g_status.buffers.Char[party_slot].Hand[hand].in_play != 0 &&
            GetCharAttackRange(&g_status.buffers.Char[party_slot], hand) != -1) {
            return GetCharAttackRange(character, 0) <= W8_RANGE_SHORT;
        }
    }
    return 0;
}

/* Whether a character could attack what a combat slot names. A party member
   has to be in play and not screened off by the front rank; a monster has to be
   engaged, alive, targetable and within the character's reach. */
// FUNCTION: WIZ8 0x00545c20
bool CharacterHasAttackOn(int party_slot, W8CombatSlot* target)
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
        if (CanPartyMemberAimAtMonster(party_slot, 0, monster_info,
                                       g_combat_state->characters[party_slot].dead_34 == 0,
                                       0) == 0) {
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
bool MonsterHasAttackOn(W8MonsterInfo* monster_info, W8CombatSlot* target)
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
        character = &g_status.buffers.Char[target_slot];
        hp_percent = character->hp_current * 100 / character->uiHPMax;
        target_level = character->uiExpLevel;
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
        if (MonsterHostility(monster_info, target_info) != 2) {
            return 0;
        }
        if (MonsterAttackReachesMonster(monster_info, record, 0, target_info) == 0) {
            return 0;
        }
        target_level = GetMonsterDataForInfo(target_info)->effective_level_24f;
        hp_percent = target_info->hp_current * 100 / target_info->uiHPMax;
        out_of_formation = monster_info->modifiers_1db.out_of_formation;
    } else {
        return 0;
    }
    if (target_level > record->effective_level_24f && (hp_percent < 40 || out_of_formation != 0)) {
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
        monster_info->hp_current != 0 && monster_info->highest_condition < 0xc &&
        (record->flags_0d0 & W8_MONSTER_FLAG_ATTACKS) != 0 && record->attacks[0].fHasAttack != 0) {
        return MonsterHasAttackOn(monster_info, &monster_info->Target) != 0;
    }
    return false;
}

/* Attack verbs whose fatigue penalty halves before it lands on the monster's
   attack score - the unarmed and natural strikes. The score roll scans it for
   the attack's verb byte. */
// GLOBAL: WIZ8 0x0061D284
const unsigned char g_low_fatigue_attack_verbs[0x12] = {
    0x07, 0x0d, 0x0e, 0x0f, 0x15, 0x19, 0x21, 0x2b, 0x31,
    0x32, 0x34, 0x35, 0x36, 0x38, 0x3b, 0x55, 0x56, 0x57,
};

/* Guardian interception: every party member and combat monster protecting the
   struck target rolls its defense score against the attacker's, and a success
   replaces the target with the guardian. Answers whether the target was
   taken over. */
// FUNCTION: WIZ8 0x00545e50
int ResolveGuardianInterception(W8TargetSource* source, W8CombatSlot* target)
{
    W8GrowableVector<W8TargetSource> candidates;
    W8GrowableVector<W8TargetSource> interposers;
    W8TargetSource guardian;
    wchar_t target_name[80];
    int guardian_score;
    int attacker_score;
    int i;

    for (int slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status.buffers.Char[slot];
        W8CombatCharacterRow* row = &g_combat_state->characters[slot];
        W8PartySlotRow* party_row = &g_status.buffers.XChar[slot];
        if (IsPartySlotEligible(slot) && character->highest_condition < 0xc &&
            character->Hand[0].in_play != 0 &&
            TryCharacterAction(slot, W8_ACTION_PROTECT, 0) != 0 &&
            row->interception_count < character->Hand[0].attacks) {
            W8CombatSlot* guarded =
                row->dead_34 == 0 ? &party_row->target_in_combat : &party_row->target_out_of_combat;
            if (memcmp(target, guarded, sizeof(W8CombatSlot)) == 0 &&
                CharacterHasAttackOn(slot, target) != 0) {
                SetTargetSourceToCharacter(slot, &guardian);
                candidates.Add(guardian);
                if (row->dead_34 == 0) {
                    SwitchCharacterTo(slot, W8_ACTION_PROTECT);
                }
            }
        }
    }
    for (unsigned int monster_index = 0; monster_index < PLLength(gXStatus.plsMonsterList);
         ++monster_index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
        if (monster_info->fActive != 0 && monster_info->fInCombat != 0 &&
            monster_info->hp_current != 0 && monster_info->highest_condition < 0xc &&
            (record->flags_0d0 & 4) != 0 && record->attacks[0].fHasAttack != 0 &&
            monster_info->action_kind == 8 &&
            monster_info->pCombat->interception_count < record->attacks_per_round_0e5 &&
            memcmp(target, &monster_info->Target, sizeof(W8CombatSlot)) == 0 &&
            MonsterHasAttackOn(monster_info, target) != 0) {
            SetTargetSourceToMonster(monster_info, &guardian);
            candidates.Add(guardian);
        }
    }
    if (candidates.GetCount() == 0) {
        return 0;
    }
    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        wcscpy(target_name, g_status.buffers.Char[target->iChar].name);
    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
        W8MonsterInfo* target_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x1816, COMBAT_ATTACK_CPP, target->iMonsterID, 1));
        wcscpy(target_name, GetMonsterName(target_info, NULL, 0));
    } else {
        srAssertFail("FALSE", COMBAT_ATTACK_CPP, 0x181a, 0);
    }
    for (i = 0; i < candidates.GetCount(); ++i) {
        W8TargetSource* candidate = candidates.GetAt(i);
        if (candidate->iType == W8_TARGET_SOURCE_CHARACTER) {
            W8Character* defender = &g_status.buffers.Char[candidate->iChar];
            int armed = 0;
            if (defender->Hand[0].in_play != 0) {
                armed = defender->Hand[0].hit_bonus * 5 + defender->Hand[0].attack_score;
            }
            unsigned int penalty = FatigueArmorPenalty(defender->fatigue_band);
            if (defender->Hand[0].weapon_skill == W8_SKILL_MODERN_WEAPONS) {
                penalty >>= 1;
            }
            guardian_score = defender->attributes[W8_ATTRIBUTE_SPEED].effective - penalty + armed +
                             defender->bonus_1770.hit_bonus_01 * 5;
        } else if (candidate->iType == W8_TARGET_SOURCE_MONSTER) {
            W8MonsterInfo* defender = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x182b, COMBAT_ATTACK_CPP, candidate->iMonsterID, 1));
            W8MonsterRecord* record = GetMonsterDataForInfo(defender);
            int armed = 0;
            if (record->attacks[0].fHasAttack != 0) {
                armed = record->attacks[0].attack_score_04;
            }
            unsigned int penalty = FatigueArmorPenalty(defender->fatigue_band);
            for (int j = 0; j < 0x12; ++j) {
                if (record->attacks[0].ubWeaponNameIndex == g_low_fatigue_attack_verbs[j]) {
                    penalty >>= 1;
                    break;
                }
            }
            guardian_score = defender->attributes[3] - penalty + armed +
                             defender->modifiers_1db.hit_bonus_01 * 5;
        } else {
            srAssertFail("FALSE", COMBAT_ATTACK_CPP, 0x182f, 0);
        }
        if (source->iType == W8_TARGET_SOURCE_CHARACTER) {
            W8Character* attacker = &g_status.buffers.Char[source->iChar];
            int armed = 0;
            if (attacker->Hand[0].in_play != 0) {
                armed = attacker->Hand[0].hit_bonus * 5 + attacker->Hand[0].attack_score;
            }
            unsigned int penalty = FatigueArmorPenalty(attacker->fatigue_band);
            if (attacker->Hand[0].weapon_skill == W8_SKILL_MODERN_WEAPONS) {
                penalty >>= 1;
            }
            attacker_score = attacker->attributes[W8_ATTRIBUTE_SPEED].effective - penalty + armed +
                             attacker->bonus_1770.hit_bonus_01 * 5;
        } else if (source->iType == W8_TARGET_SOURCE_MONSTER) {
            W8MonsterInfo* attacker = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x1839, COMBAT_ATTACK_CPP, source->iMonsterID, 1));
            W8MonsterRecord* record = GetMonsterDataForInfo(attacker);
            int armed = 0;
            if (record->attacks[0].fHasAttack != 0) {
                armed = record->attacks[0].attack_score_04;
            }
            unsigned int penalty = FatigueArmorPenalty(attacker->fatigue_band);
            for (int j = 0; j < 0x12; ++j) {
                if (record->attacks[0].ubWeaponNameIndex == g_low_fatigue_attack_verbs[j]) {
                    penalty >>= 1;
                    break;
                }
            }
            attacker_score = attacker->attributes[3] - penalty + armed +
                             attacker->modifiers_1db.hit_bonus_01 * 5;
        } else {
            srAssertFail("FALSE", COMBAT_ATTACK_CPP, 0x183d, 0);
        }
        if (g_combat_state->unaware_9a4 != 0) {
            attacker_score += 10;
        }
        if (g_combat_state->natural_attack_9a5 != 0) {
            attacker_score += 10;
        }
        int chance = guardian_score - attacker_score + 50;
        if (chance < 0x60) {
            if (chance < 5) {
                chance = 5;
            }
        } else {
            chance = 0x5f;
        }
        if (Random(100) < static_cast<unsigned int>(chance)) {
            interposers.Add(*candidate);
        }
    }
    if (interposers.GetCount() == 0) {
        if (g_settings.verbose_combat_messages != 0 && candidates.GetCount() != 0) {
            for (i = 0; i < candidates.GetCount(); ++i) {
                W8TargetSource* candidate = candidates.GetAt(i);
                swprintf(g_combat_state->attack_message_6b8, gppStringList[0x219], target_name);
                if (candidate->iType == W8_TARGET_SOURCE_CHARACTER) {
                    PostCharacterNotice(candidate->iChar, g_combat_state->attack_message_6b8);
                } else if (candidate->iType == W8_TARGET_SOURCE_MONSTER) {
                    W8MonsterInfo* defender =
                        MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                            0x186f, COMBAT_ATTACK_CPP, candidate->iMonsterID, 1));
                    PostMonsterNotice(defender, g_combat_state->attack_message_6b8);
                }
            }
        }
        return 0;
    }
    unsigned int uiSuccessfulCnt = interposers.GetCount();
    if (uiSuccessfulCnt <= 0) {
        srAssertFail("uiSuccessfulCnt > 0", COMBAT_ATTACK_CPP, 0x187c, 0);
    }
    W8TargetSource* interposer = interposers.GetAt(Random(uiSuccessfulCnt));
    swprintf(g_combat_state->attack_message_6b8, gppStringList[0x21a], target_name);
    if (interposer->iType == W8_TARGET_SOURCE_CHARACTER) {
        int slot = interposer->iChar;
        PostCharacterNotice(slot, g_combat_state->attack_message_6b8);
        ResetCombatSlot(target);
        target->iType = W8_TARGET_KIND_CHARACTER;
        target->iChar = slot;
        ++g_combat_state->characters[slot].interception_count;
        unsigned int fatigue =
            g_item_records
                    [g_status.buffers.Char[slot].EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo]
                        .weight /
                0x28 +
            1;
        FatigueCharacter(slot, Random(fatigue) + 1 + fatigue, 1, NULL);
    } else if (interposer->iType == W8_TARGET_SOURCE_MONSTER) {
        W8MonsterInfo* defender = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x18a2, COMBAT_ATTACK_CPP, interposer->iMonsterID, 1));
        PostMonsterNotice(defender, g_combat_state->attack_message_6b8);
        ResetCombatSlot(target);
        target->iType = W8_TARGET_KIND_MONSTER;
        target->iMonsterID = interposer->iMonsterID;
        ++defender->pCombat->interception_count;
        FatigueMonster(defender, Random(3) + 2, NULL);
    }
    g_combat_state->unaware_9a4 = 0;
    g_combat_state->natural_attack_9a5 = 0;
    return 1;
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
    if (!IsPartySlotEligible(party_slot)) {
        return false;
    }
    character = &g_status.buffers.Char[party_slot];
    if (character->highest_condition >= 0xc || character->Hand[0].in_play == 0) {
        return false;
    }
    return CharacterHasAttackOn(party_slot,
                                &g_status.buffers.XChar[party_slot].target_out_of_combat) != 0;
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
const unsigned short g_pc_hit_location_labels[5][2] = {
    {0x429, 0x42a}, {0x42b, 0x42c}, {0x42d, 0x42e}, {0x42f, 0x430}, {0x431, 0x432},
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

// GLOBAL: WIZ8 0x0061e9a8
const unsigned short g_attack_flag_name_ids[9][2] = {
    {1311, 1312}, {1313, 1314}, {1315, 1316}, {1317, 1318}, {1319, 1320},
    {1321, 1322}, {1323, 1324}, {1325, 1326}, {1327, 1328},
};

// GLOBAL: WIZ8 0x0061e9cc
const unsigned short g_damage_type_name_ids[0x10] = {
    1330, 1331, 1332, 1333, 1334, 1335, 1336, 1337, 1338, 1339, 1340, 1341, 1342, 1343, 1344, 1345,
};

// GLOBAL: WIZ8 0x0061ea78
const unsigned short g_special_category_name_ids[42] = {
    1400, 1401, 1402, 1403, 1404, 1405, 1406, 1407, 1408, 1409, 1410, 1411, 1412, 1413,
    1414, 1415, 1416, 1417, 1418, 1419, 1420, 1421, 1422, 1423, 1424, 1425, 1426, 1427,
    1428, 1429, 1430, 1431, 1432, 0,    1433, 1434, 1435, 1436, 1437, 1438, 1439, 1440,
};

/* Land a spell missile on whatever it struck. The spell's own impact sound
   plays when it has one. A single-target effect (no radius) rolls its size,
   applies it to the struck monster or character and rolls the effect's
   conditions against the target. An area effect instead reaches the whole
   party when a character was struck, or every active monster within the
   radius of the struck monster's position. */
// FUNCTION: WIZ8 0x00544d30
void ResolveSpellMissileHit(W8Missile* missile)
{
    W8TargetSource* source = &missile->m_Source;
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
            DamageMonstersInRadius(monster_info->p3D->GetPosition(), definition->radius,
                                   &definition->magnitude, source, &missile->result_280);
            announce = g_settings.verbose_combat_messages;
            verbose = g_settings.verbose_combat_messages;
            srVector3T<float> center = monster_info->p3D->GetPosition();
            ResetCombatSlot(&struck);
            struck.iType = W8_TARGET_KIND_MONSTER;
            for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
                monster_info = MonsterGetScriptPartByLocationIndex(index);
                if (monster_info->fActive != 0) {
                    MonsterGetLocation(monster_info->p3D, &location);
                    srVector3T<float> offset = center - location;
                    if (offset.Length() <= definition->radius) {
                        struck.iMonsterID = monster_info->location_id;
                        ApplyEffectConditions(source, &struck, definition, announce, verbose, 0);
                    }
                }
            }
        } else {
            ApplyRolledHealthChangeToParty(&definition->magnitude, &missile->result_280, 1);
            announce = g_settings.verbose_combat_messages;
            verbose = g_settings.verbose_combat_messages;
            ResetCombatSlot(&struck);
            struck.iType = W8_TARGET_KIND_CHARACTER;
            for (index = 0; index < 8; ++index) {
                if (g_status.buffers.XChar[index].fOccupied != 0) {
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
                                     g_settings.verbose_combat_messages, 0, &missile->result_280,
                                     0);
            } else {
                ApplyDamageToCharacter(target->iChar, magnitude, 0,
                                       g_settings.verbose_combat_messages, 0, &missile->result_280,
                                       0);
            }
        }
    }
    ApplyEffectConditions(source, target, definition, g_settings.verbose_combat_messages,
                          g_settings.verbose_combat_messages, &missile->result_280);
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
    W8TargetSource* source = &missile->m_Source;
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
        wcscat(text, g_status.buffers.Char[target->iChar].name);
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
            wcscpy(location_name,
                   gppStringList[g_monster_hit_location_labels[hit_location]
                                                              [record->constitution_15e]]);
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
    if (g_settings.verbose_combat_messages != 0) {
        ShowNoticef(source_color, gppStringList[0x830 / 4], location_name);
    }

    attack_mode = g_missile_table[missile->missile_table_index_1d8].attack_mode_144;
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
        } else if (g_settings.verbose_combat_messages != 0) {
            ShowNoticef(source_color, gppStringList[0x838 / 4]);
        }
    } else {
        ShowNoticef(source_color, gppStringList[0x83c / 4]);
        MakePCHitSound(missile, target, hit_location, -1);
    }
}

/* Which hand a character attacks with: the hand carrying the most attacks
   wins, ties go to the bigger damage bonus, and a hand that cannot reach is
   not a candidate. */
// FUNCTION: WIZ8 0x0053d390
int ChooseCharacterAttackHand(int party_slot)
{
    int best_hand = -1;
    unsigned int best_attacks = 0;
    int best_damage_bonus = -999;
    unsigned int hand;

    for (hand = 0; hand < W8_HAND_COUNT; ++hand) {
        if (hand >= W8_HAND_COUNT) {
            srAssertFail("uiHand < HAND_COUNT", COMBAT_ATTACK_CPP, 102,
                         // reinterpret-ok: the assert message slot carries the failing hand index
                         reinterpret_cast<const char*>(hand));
        }
        if (g_status.buffers.Char[party_slot].Hand[hand].in_play != 0 &&
            GetCharAttackRange(&g_status.buffers.Char[party_slot], hand) != -1 &&
            (best_attacks < g_status.buffers.Char[party_slot].Hand[hand].attacks ||
             (g_status.buffers.Char[party_slot].Hand[hand].attacks == best_attacks &&
              g_status.buffers.Char[party_slot].Hand[hand].damage_bonus > best_damage_bonus))) {
            best_attacks = g_status.buffers.Char[party_slot].Hand[hand].attacks;
            best_hand = hand;
            best_damage_bonus = g_status.buffers.Char[party_slot].Hand[hand].damage_bonus;
        }
    }
    return best_hand;
}

/* Pick the attacking hand and record each hand's reach-adjusted attack count
   on the combat row, zero for a hand that cannot reach. A character with no
   usable hand cannot be attacking. */
// FUNCTION: WIZ8 0x0053d680
void PrepareCharacterAttacks(int party_slot)
{
    int best_hand = -1;
    unsigned int best_attacks = 0;
    int best_damage_bonus = -999;
    unsigned int hand;
    int value;

    for (hand = 0; hand < W8_HAND_COUNT; ++hand) {
        if (hand >= W8_HAND_COUNT) {
            srAssertFail("uiHand < HAND_COUNT", COMBAT_ATTACK_CPP, 102,
                         // reinterpret-ok: the assert message slot carries the failing hand index
                         reinterpret_cast<const char*>(hand));
        }
        if (g_status.buffers.Char[party_slot].Hand[hand].in_play != 0 &&
            GetCharAttackRange(&g_status.buffers.Char[party_slot], hand) != -1 &&
            (best_attacks < g_status.buffers.Char[party_slot].Hand[hand].attacks ||
             (g_status.buffers.Char[party_slot].Hand[hand].attacks == best_attacks &&
              g_status.buffers.Char[party_slot].Hand[hand].damage_bonus > best_damage_bonus))) {
            best_attacks = g_status.buffers.Char[party_slot].Hand[hand].attacks;
            best_hand = hand;
            best_damage_bonus = g_status.buffers.Char[party_slot].Hand[hand].damage_bonus;
        }
    }
    if (best_hand == -1) {
        FormatDebugMessage(1, "ERROR: %ls is preparing attacks with when it's not possible!",
                           g_status.buffers.Char[party_slot].name);
        return;
    }
    g_combat_state->characters[party_slot].current_hand = best_hand;
    for (hand = 0; hand < W8_HAND_COUNT; ++hand) {
        if (hand >= W8_HAND_COUNT) {
            srAssertFail("uiHand < HAND_COUNT", COMBAT_ATTACK_CPP, 102,
                         // reinterpret-ok: the assert message slot carries the failing hand index
                         reinterpret_cast<const char*>(hand));
        }
        if (g_status.buffers.Char[party_slot].Hand[hand].in_play == 0 ||
            GetCharAttackRange(&g_status.buffers.Char[party_slot], hand) == -1) {
            value = 0;
        } else {
            value = g_status.buffers.Char[party_slot].Hand[hand].attacks;
        }
        g_combat_state->characters[party_slot].saved_attack_value[hand] = value;
        g_combat_state->characters[party_slot].hand_attack_values_40[hand] = value;
    }
}

/* The situational armour-class adjustment a target gets against one attack
   mode: the character's conditional components, or the monster's bonus and
   fatigue share, plus the four points a distracted or flanked target loses
   against anything but the special modes. */
// FUNCTION: WIZ8 0x005468d0
int GetTargetArmorClassModifier(W8CombatSlot* target, unsigned int attack_mode)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    unsigned int monster_list_index;
    int modifier = 0;
    unsigned char distracted = 0;
    unsigned char out_of_formation;

    if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster_list_index =
            MonsterGetIndexByLocationID(0x1937, COMBAT_ATTACK_CPP, target->iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        record = GetMonsterDataForInfo(monster_info);
        modifier = monster_info->modifiers_1db.armor_class_adjustment_4b +
                   monster_info->modifiers_1db.armor_bonus_04 +
                   static_cast<int>(FatigueArmorPenalty(monster_info->fatigue_band) / 10);
        if (gXStatus.fCombatMode != 0) {
            if ((g_combat_state->unaware_9a4 != 0 || g_combat_state->natural_attack_9a5 != 0) &&
                (record->flags_0d0 & 2) != 0) {
                modifier -= 2;
            }
            if (monster_info->action_kind == 1) {
                modifier += 2;
            }
            distracted = monster_info->action_kind == 4;
        }
        out_of_formation = monster_info->modifiers_1db.out_of_formation;
    } else if (target->iType == W8_TARGET_KIND_CHARACTER) {
        W8Character* character = &g_status.buffers.Char[target->iChar];
        if (gXStatus.fCombatMode == 0) {
            distracted = GetLevelDataFlag8();
        } else {
            int component;
            if (g_combat_state->unaware_9a4 != 0 || g_combat_state->natural_attack_9a5 != 0) {
                component = character->armor_class_components[1];
                if (component > 0) {
                    modifier = -component;
                }
                component = character->armor_class_components[3];
                if (component > 0) {
                    modifier -= component;
                }
            }
            if (GetEngagementCount() == 0 && TryCharacterAction(target->iChar, 6, 0) != 0) {
                modifier -= 4;
            }
            distracted = GetEngagementCount() == 2;
        }
        out_of_formation = character->bonus_1770.out_of_formation;
    } else {
        srAssertFail("FALSE", COMBAT_ATTACK_CPP, 0x1992, 0);
        return 0;
    }
    if (distracted != 0 && out_of_formation == 0 &&
        (attack_mode != 4 && (attack_mode <= 6 || attack_mode > 8))) {
        modifier -= 4;
    }
    return modifier;
}

/* Which of the hand's attack modes this swing uses: a random one of the set
   mode bits, walked cyclically until the roll's-th set bit. */
// FUNCTION: WIZ8 0x00542ca0
unsigned int CharChooseHandAttackMode(W8Character* character, int hand)
{
    W8ItemInstance* item;
    unsigned short modes;
    int slot;

    if (character->Hand[hand].uiHolds == HOLDS_NOTHING) {
        modes = character->Hand[hand].attack_flags;
    } else {
        slot = (hand != 0) + 6;
        item = &character->EquippedItem[slot];
        if (item->iItemNo == -1) {
            srAssertFail(
                "pPC->EquippedItem[uiWeaponSlot].iItemNo != -1", COMBAT_ATTACK_CPP, 0xdb1,
                FormatString("CharChooseHandAttackMode: ERROR - %ls's weapon slot %d has no "
                             "item in it, but holds %d",
                             character->name, slot, character->Hand[hand].uiHolds));
        }
        modes = g_item_records[item->iItemNo].attack_flags_04e;
    }
    if (modes == 0) {
        srAssertFail("fsAttackModes > 0", COMBAT_ATTACK_CPP, 0xdb6,
                     FormatString("CharChooseHandAttackMode: ERROR - %ls's hand %d has no "
                                  "attack modes",
                                  character->name, hand));
    }
    return ChooseAttackMode(modes);
}

/* The display name of whatever a combat slot names, for the spell and attack
   notices. A target that is its own source answers the reflexive pronoun
   instead of the name. */
// FUNCTION: WIZ8 0x00546b40
wchar_t* SpellTargetString(W8TargetSource* source, W8CombatSlot* target)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    W8MonsterGroup* monster_group;
    unsigned int index;

    if (target == NULL) {
        srAssertFail("pTarget != NULL", COMBAT_ATTACK_CPP, 0x19ed, 0);
    }
    switch (target->iType) {
    case W8_TARGET_KIND_NONE:
    case W8_TARGET_KIND_PLACE:
        return &g_wchar_00689b34;
    case W8_TARGET_KIND_CHARACTER:
    case W8_TARGET_KIND_CHARACTER_INDIRECT:
        break;
    case W8_TARGET_KIND_PARTY:
        return gppStringList[0x1e3];
    case W8_TARGET_KIND_MONSTER:
        if (target->iMonsterID == -1) {
            srAssertFail("pTarget->iMonsterID != BAD_INDEX", COMBAT_ATTACK_CPP, 0x1a07, 0);
        }
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x1a0a, COMBAT_ATTACK_CPP, target->iMonsterID, 1));
        record = GetMonsterDataForInfo(monster_info);
        if (TargetSourceIsMonster(source, 0) != 0 && source->iMonsterID == target->iMonsterID) {
            return FormatWideString(
                gppStringList[g_gender_name_message_rows[record->name_group_0cc][3]]);
        }
        return FormatWideString(GetMonsterName(monster_info, record, 0));
    case W8_TARGET_KIND_GROUP:
        monster_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0x1a19, COMBAT_ATTACK_CPP, target->iGroupID, 1));
        return FormatWideString(GetMonsterGroupName(monster_group));
    case W8_TARGET_KIND_FIVE:
        return gppStringList[0x1e4];
    case W8_TARGET_KIND_ITEM:
        return FormatWideString(gppStringList[0x1e6], FormatItemDisplayName(target->pPCItem, 0));
    case W8_TARGET_KIND_EIGHT:
        return gppStringList[0x1e5];
    default:
        srAssertFail(
            "FALSE", COMBAT_ATTACK_CPP, 0x1a2b,
            FormatString("SpellTargetString: ERROR - Invalid target type %d", target->iType));
        return L" - TARGET INVALID";
    }
    if (target->iChar == -1) {
        srAssertFail("pTarget->iChar != BAD_INDEX", COMBAT_ATTACK_CPP, 0x19f6, 0);
    }
    if (TargetSourceIsCharacter(source, 0) != 0 && source->name_known_19 == 0 &&
        source->iChar == target->iChar) {
        return FormatWideString(
            gppStringList[g_gender_name_message_rows[g_status.buffers.Char[source->iChar].gender]
                                                    [3]]);
    }
    return FormatWideString(g_status.buffers.Char[target->iChar].name);
}

/* What the target's armour starts from before the situational modifier: the
   character's total, or ten minus the monster record's evasion - unless the
   monster is out of formation, which costs it a flat five. */
// FUNCTION: WIZ8 0x00542ee0
int GetTargetArmorClass(W8CombatSlot* target, int attack_mode)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    unsigned int monster_list_index;
    /* Retail continued with an uninitialized base after the assertion; a
       deterministic zero models that defect path. */
    int base = 0;

    if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster_list_index =
            MonsterGetIndexByLocationID(0xeb0, COMBAT_ATTACK_CPP, target->iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        record = GetMonsterDataForInfo(monster_info);
        base = 10 - record->evasion_ac_15d;
        if (monster_info->modifiers_1db.out_of_formation != 0 && base > -5) {
            return GetTargetArmorClassModifier(target, attack_mode) - 5;
        }
    } else if (target->iType == W8_TARGET_KIND_CHARACTER) {
        base = g_status.buffers.Char[target->iChar].armor_class_total;
    } else {
        srAssertFail("FALSE", COMBAT_ATTACK_CPP, 0xec8, 0);
    }
    return GetTargetArmorClassModifier(target, attack_mode) + base;
}

/* The armour class a hit at one location must beat: the base above plus the
   location and attack-mode terms each target kind carries, with the
   situational modifier applied a second time through the base. */
// FUNCTION: WIZ8 0x00542fc0
int TargetArmorClassAtLocation(W8CombatSlot* target, int attack_mode, int hit_location)
{
    W8Character* character;
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    unsigned int monster_list_index;
    int base = GetTargetArmorClass(target, attack_mode);

    if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster_list_index =
            MonsterGetIndexByLocationID(0xee0, COMBAT_ATTACK_CPP, target->iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        record = GetMonsterDataForInfo(monster_info);
        return GetTargetArmorClassModifier(target, attack_mode) +
               (monster_info->modifiers_1db.armor_bonus_05 -
                record->armor_class_by_attack_mode[attack_mode] -
                record->armor_class_by_location[hit_location]) +
               record->evasion_ac_15d + base;
    }
    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        character = &g_status.buffers.Char[target->iChar];
        return GetTargetArmorClassModifier(target, attack_mode) +
               (character->armor_class_by_location[hit_location] - character->armor_class_total) +
               base;
    }
    srAssertFail("FALSE", COMBAT_ATTACK_CPP, 0xefa, 0);
    return GetTargetArmorClassModifier(target, attack_mode) + hit_location + base;
}

/* Whether a swing the attacker landed is blocked outright: a character blocks
   with the off-hand item when their shield component covers the margin; a
   monster needs its record's blocking flag and a margin of ten or less. The
   block posts the notice and plays the material impact sound. */
// FUNCTION: WIZ8 0x00543110
unsigned char BlockedForSpecialReason(int weapon_class, W8CombatSlot* target, int attack_value,
                                      int armor_value, unsigned int palette)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    unsigned int monster_list_index;
    int material;
    int difference = attack_value - armor_value;

    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        W8Character* character = &g_status.buffers.Char[target->iChar];
        int shield = 0;
        if (g_combat_state->unaware_9a4 == 0 && g_combat_state->natural_attack_9a5 == 0) {
            shield = character->armor_class_components[3] * 5;
        }
        if (difference > shield) {
            return 0;
        }
        if (character->EquippedItem[7].iItemNo != -1) {
            material = g_item_records[character->EquippedItem[7].iItemNo].material_0c1;
        } else {
            material = 2;
        }
    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster_list_index =
            MonsterGetIndexByLocationID(0xf9b, COMBAT_ATTACK_CPP, target->iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        record = GetMonsterDataForInfo(monster_info);
        if ((record->flags_0d0 & 2) == 0) {
            return 0;
        }
        if (difference > 10) {
            return 0;
        }
        material = 2;
    } else {
        srAssertFail("FALSE", COMBAT_ATTACK_CPP, 0xfa6,
                     "BlockedForSpecialReason: Unknown target type");
        return 0;
    }
    if (g_settings.verbose_combat_messages != 0) {
        ShowNoticef(palette, gppStringList[0x213]);
    }
    PlayCombatSound(GetMaterialImpactSound(weapon_class, material), 1, 1, -1);
    return 1;
}

/* The most a single attack may take off the current target: a fifth of its
   maximum hit points, at least one. */
// FUNCTION: WIZ8 0x00545a00
unsigned int CapAttackDamageByTargetHealth(unsigned int damage)
{
    W8CombatSlot* target = &g_combat_state->TargetHit;
    W8MonsterInfo* monster_info;
    unsigned int monster_list_index;
    unsigned int health;
    unsigned int limit;

    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        health = g_status.buffers.Char[target->iChar].uiHPMax;
    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster_list_index =
            MonsterGetIndexByLocationID(0x1697, COMBAT_ATTACK_CPP, target->iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        health = monster_info->uiHPMax;
    } else {
        srAssertFail("FALSE", COMBAT_ATTACK_CPP, 0x169c, 0);
        health = 0;
    }
    limit = health / 5;
    if (limit <= 1) {
        limit = 1;
    }
    if (damage > limit) {
        damage = limit;
    }
    return damage;
}

/* Launch the animation cycle for the attack the monster just committed to.
   Action details zero through eight pick cycles nine through seventeen; a
   monster that lacks the cycle falls back to six or seven. A ranged cycle on
   an attack whose record only reaches short range is a data error, corrected
   to long and reported; the reverse is corrected to short the same way. */
// FUNCTION: WIZ8 0x0053ffe0
void StartMonsterAttackCycle(W8MonsterInfo* monster_info, int action_detail)
{
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    unsigned int attack = monster_info->pCombat->attack_index_11;
    W8RangeCategory range;
    signed char cycle;

    switch (action_detail) {
    case 0:
        cycle = 9;
        break;
    case 1:
        cycle = 10;
        break;
    case 2:
        cycle = 11;
        break;
    case 3:
        cycle = 12;
        break;
    case 4:
        cycle = 13;
        break;
    case 5:
        cycle = 14;
        break;
    case 6:
        cycle = 15;
        break;
    case 7:
        cycle = 16;
        break;
    case 8:
        cycle = 17;
        break;
    default:
        srAssertFail(
            "FALSE", COMBAT_ATTACK_CPP, 0x748,
            FormatString("StartMonsterAttack: ERROR - Invalid attack mode %d", action_detail));
        return;
    }
    if (MonsterIsCycleSupported(monster_info->p3D, cycle) == 0) {
        switch (cycle) {
        case 9:
        case 10:
        case 11:
        case 12:
        case 14:
        case 15:
        case 16:
            cycle = 6;
            break;
        case 13:
        case 17:
            cycle = 7;
            break;
        }
    }
    if (cycle == 0x11 || cycle == 0xd || cycle == 7) {
        if (record->attacks[attack].range_category <= W8_RANGE_SHORT) {
            record->attacks[attack].range_category = W8_RANGE_EXTREME;
            FormatDebugMessage(0, "DATA ERROR: %ls has mismatched range/attack modes for attack %d",
                               record->name_00, attack);
        }
    } else if (record->attacks[attack].range_category >= W8_RANGE_LONG) {
        record->attacks[attack].range_category = W8_RANGE_TOUCH;
        FormatDebugMessage(0, "DATA ERROR: %ls has mismatched range/attack modes for attack %d",
                           record->name_00, attack);
    }
    range = GetMonsterActionRangeCategory(monster_info, record, attack);
    if (range >= W8_RANGE_LONG) {
        monster_info->fMissileReleased = false;
    }
    ResetCombatSlot(&g_combat_state->TargetHit);
    StartMonsterCycle(monster_info, cycle, 1);
}

/* Announce a character's attack on the combat log: the swings line, the
   damage notices, what the target suffered unless it died, and the casualty
   list - then wipe the record clean for the next attack. */
// FUNCTION: WIZ8 0x0053fb00
void ReportCharacterAttackResult(int party_slot, W8SpellEffectResult* report)
{
    W8MonsterInfo* monster_info;
    unsigned int* condition_turns;
    W8SpellDamageReport* entry;
    unsigned short* notice;
    int index;
    unsigned int condition;

    if (report->condition_counts[W8_CONDITION_DEAD] > 0) {
        if (report->notice_values[3] > 0) {
            PostCharacterNotice(party_slot, gppStringList[0x267], report->notice_values[3]);
        }
        if (report->notice_values[4] > 0) {
            PostCharacterNotice(party_slot, gppStringList[0x268], report->notice_values[4]);
        }
    } else {
        if (report->count == 0) {
            if (report->missed != 0) {
                ShowNotice(8, gppStringList[0x20b], -1, -1, 0);
                return;
            }
            ShowNotice(8, gppStringList[0x209], -1, -1, 0);
            return;
        }
        if (report->amount == 0) {
            ShowNotice(8, gppStringList[0x20e], -1, -1, 0);
        } else if (report->count == 1) {
            PostCharacterNotice(party_slot, gppStringList[0x265], report->amount);
        } else if (report->count > 1) {
            PostCharacterNotice(party_slot, gppStringList[0x266], report->count, report->amount);
        }
        if (report->notice_values[3] > 0) {
            PostCharacterNotice(party_slot, gppStringList[0x267], report->notice_values[3]);
        }
        if (report->notice_values[4] > 0) {
            PostCharacterNotice(party_slot, gppStringList[0x268], report->notice_values[4]);
        }
        if (report->target.iType == W8_TARGET_KIND_MONSTER) {
            monster_info =
                MonsterInfoFromID(0x68c, COMBAT_ATTACK_CPP, report->target.iMonsterID, 1);
            condition_turns = monster_info->uiCondition;
        } else {
            condition_turns = g_status.buffers.Char[report->target.iChar].uiCondition;
        }
        if (condition_turns[W8_CONDITION_DEAD] == 0) {
            if (report->notice_values[0] > 0) {
                ShowNoticef(8, gppStringList[0x1a6], report->notice_values[0]);
            }
            if (report->notice_values[1] > 0) {
                ShowNoticef(8, gppStringList[0x264], report->notice_values[1]);
            }
            if (report->notice_values[2] > 0) {
                ShowNoticef(8, gppStringList[0x261], report->notice_values[2]);
            }
            if (report->notice_values[5] > 0) {
                if (g_status.buffers.Char[party_slot].hp_current ==
                    static_cast<unsigned int>(g_status.buffers.Char[party_slot].uiHPMax)) {
                    PostCharacterNotice(party_slot, gppStringList[0x258]);
                } else {
                    PostCharacterNotice(party_slot, gppStringList[0x25a], report->notice_values[5]);
                }
            }
            notice = g_condition_notices + 1;
            for (condition = 0; notice < g_condition_notices + 0x51; notice += 4, ++condition) {
                if (report->condition_counts[condition] > 0 && notice != g_condition_notices + 5 &&
                    notice != g_condition_notices + 0x4d) {
                    if (report->target.iType == W8_TARGET_KIND_MONSTER) {
                        ShowNoticef(9, L"%s %s!", GetMonsterName(monster_info, NULL, 0),
                                    gppStringList[*notice]);
                    } else if (report->target.iType == W8_TARGET_KIND_CHARACTER) {
                        PostCharacterNotice(report->target.iChar, L"%s!", gppStringList[*notice]);
                    }
                }
            }
        }
    }
    while (report->reports.count > 0) {
        entry = report->reports.data[0];
        if (report->reports.count > 0) {
            for (index = 0; index < report->reports.count - 1; ++index) {
                report->reports.data[index] = report->reports.data[index + 1];
            }
            --report->reports.count;
        }
        if (entry != NULL) {
            if (entry->kind == 1) {
                PostCharacterNotice(entry->value, L"%s!", gppStringList[g_condition_notices[0x49]]);
            } else if (entry->kind == 3) {
                ShowNoticef(9, L"%s %s!", entry->text, gppStringList[g_condition_notices[0x49]]);
            }
            free(entry);
        }
    }
    memset(static_cast<void*>(report), 0, sizeof(*report));
}

/* Resolve one queued swing of the monster's attack: rolls the hit chance and
   the fumble redirection, resolves guardian interception, picks the hit
   location, rolls penetration, applies damage and the struck target's
   retaliation enchantment, then decides whether the monster swings again,
   repicks a target or is done. */
// FUNCTION: WIZ8 0x00540170
int ContinueMonsterAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record)
{
    W8SpellEffectResult* report = &g_combat_state->attack_report;
    W8SpellEffectResult local_report;
    W8MonsterCombatState* combat = monster_info->pCombat;
    unsigned int attack_index = combat->attack_index_11;
    int action_detail = monster_info->action_detail;
    W8MonsterAttack* attack = &record->attacks[attack_index];
    W8MonsterInfo* target_info = NULL;
    W8MonsterRecord* target_record;
    W8TargetSource source;
    W8TargetSource victim_source;
    W8CombatSlot entry_target;
    W8CombatSlot repick_target;
    W8SpellEffectDefinition effect;
    int event_ids[3];
    wchar_t location_name[20];
    char verbose = g_settings.verbose_combat_messages;
    bool swing_missed = false;
    char deflected = 0;
    char guaranteed_hit = 0;
    char guaranteed_penetration = 0;
    char fumbled = 0;
    char repicked = 0;
    unsigned int queued_fatigue = 1;
    unsigned int hit_location = 0;
    unsigned int damage = 0;
    unsigned int applied = 0;
    unsigned int dice_count = 0;
    int message_id = 0;
    int to_hit = 0;
    int roll = 0;
    W8RangeCategory range;
    W8PList* fumble_list;
    const wchar_t* location_text;

    SetTargetSourceToMonster(monster_info, &source);
    entry_target = monster_info->Target;
    if (entry_target.iType == W8_TARGET_KIND_CHARACTER) {
        W8Character* defender = &g_status.buffers.Char[entry_target.iChar];
        if (defender->skills[W8_SKILL_LOCKS_TRAPS].active_00 != 0) {
            g_combat_state->characters[entry_target.iChar].skill_use_flags[W8_SKILL_LOCKS_TRAPS] =
                1;
        }
        if (defender->skills[W8_SKILL_SHIELD].active_00 != 0 && g_combat_state->unaware_9a4 == 0 &&
            g_combat_state->natural_attack_9a5 == 0 && defender->armor_class_components[3] > 0) {
            g_combat_state->characters[entry_target.iChar].skill_use_flags[W8_SKILL_SHIELD] = 1;
        }
        if (defender->skills[W8_SKILL_REFLEXTION].active_00 != 0) {
            g_combat_state->characters[entry_target.iChar].skill_use_flags[W8_SKILL_REFLEXTION] = 1;
        }
    }
    range = GetMonsterActionRangeCategory(monster_info, record, attack_index);
    if (range < W8_RANGE_LONG) {
        to_hit = GetMonsterAttackScore(monster_info, attack, action_detail, 0);
        roll = Random(100) + 1;
        guaranteed_hit = roll <= 5;
        swing_missed = roll > 95;
        fumble_list = PLCreate();
        if (fumble_list == NULL) {
            srAssertFail("plsFumbleTargetList != NULL", COMBAT_ATTACK_CPP, 0x7dc, 0);
        }
        BuildMonsterTargetList(monster_info, record, attack_index, fumble_list);
        int redirect_chance = GetMonsterAttackScore(monster_info, attack, action_detail, 1);
        int fumble_chance;
        if (redirect_chance < 100) {
            fumble_chance = static_cast<int>(pow(100 - redirect_chance, 3.0) * 1e-5 + 0.5);
            unsigned int target_count = PLLength(fumble_list);
            if (target_count == 0) {
                srAssertFail("uiNumTargets > 0", COMBAT_ATTACK_CPP, 0x11d0, 0);
            }
            fumble_chance = target_count * fumble_chance * 10 / 100;
        } else {
            fumble_chance = 0;
        }
        ClampInteger(&fumble_chance, 0, 100);
        fumbled = roll > 100 - fumble_chance;
        CombatLog("TO HIT: Chance %d, Rolled %d (fumble %d%%)", to_hit, roll, fumble_chance);
        if (guaranteed_hit == 0 && fumbled != 0) {
            if (verbose == 0) {
                memset(static_cast<void*>(&local_report), 0, sizeof(local_report));
                report = &local_report;
            }
            unsigned int choices = PLLength(fumble_list);
            if (choices == 0) {
                srAssertFail("uiChoices > 0", COMBAT_ATTACK_CPP, 0x127e, 0);
            }
            W8CombatSlot* element = static_cast<W8CombatSlot*>(PLGet(fumble_list, Random(choices)));
            if (element == NULL) {
                srAssertFail("pListElement != NULL", COMBAT_ATTACK_CPP, 0x1282, 0);
            }
            g_combat_state->TargetHit = *element;
            AnnounceAccidentalStrike(&source, &g_combat_state->TargetHit);
            guaranteed_hit = 1;
            guaranteed_penetration = 0;
            swing_missed = false;
            roll = Random(100) + 1;
            g_combat_state->unaware_9a4 = 0;
            g_combat_state->natural_attack_9a5 = 0;
        } else {
            g_combat_state->TargetHit = monster_info->Target;
            guaranteed_penetration = guaranteed_hit;
        }
        PLDestroy(fumble_list);
    } else {
        if (monster_info->fMissileReleased == 0) {
            srAssertFail("pMonsterInfo->fMissileReleased", COMBAT_ATTACK_CPP, 0x80b,
                         FormatString("ContinueMonsterAttack: ERROR - Missile not released, ID %d, "
                                      "cycle %d, pending %d",
                                      monster_info->location_id, MonsterQuery(monster_info->p3D, 6),
                                      monster_info->p3D->m_pRep->pending_cycle));
        }
        if (g_combat_state->missile_hit_result == 1) {
            swing_missed = false;
            guaranteed_hit = 1;
            guaranteed_penetration = 0;
            roll = Random(100) + 1;
        } else if (g_combat_state->missile_hit_result == 2) {
            swing_missed = false;
            deflected = 1;
        } else {
            swing_missed = true;
        }
    }
    if (verbose == 0) {
        report->deferred_80 = 1;
    }
    if (swing_missed != 0) {
        if (g_settings.verbose_combat_messages != 0) {
            ShowNotice(9, gppStringList[0x209], -1, -1, 0);
        }
        ResetCombatSlot(&g_combat_state->TargetHit);
    } else if (deflected != 0) {
        if (g_settings.verbose_combat_messages == 0) {
            report->missed = true;
        } else {
            ShowNotice(9, gppStringList[0x20a], -1, -1, 0);
        }
        ResetCombatSlot(&g_combat_state->TargetHit);
    } else {
        source.target_diverted =
            memcmp(&g_combat_state->TargetHit, &monster_info->Target, sizeof(W8CombatSlot)) != 0;
        if (IsTargetStillPresent(&g_combat_state->TargetHit) == 0) {
            goto invalid_target;
        }
        if (verbose == 0 && source.target_diverted != 0) {
            memset(static_cast<void*>(&local_report), 0, sizeof(local_report));
            report = &local_report;
        }
        ResolveGuardianInterception(&source, &g_combat_state->TargetHit);
        PointCameraAtCombatTarget(&source, &g_combat_state->TargetHit);
        if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_CHARACTER) {
            unsigned int total = 0;
            unsigned int location_roll = Random(100);
            for (hit_location = 0;; ++hit_location) {
                if (hit_location >= W8_PC_HIT_LOCATIONS) {
                    if (monster_info == NULL) {
                        FormatDebugMessage(1, "ERROR: gubLocalACPercent total only %d%%");
                    } else {
                        FormatDebugMessage(
                            0, "ERROR: DBS Attack Locations total only %d%% for monster %ls", total,
                            GetMonsterName(monster_info, NULL, 0));
                    }
                    hit_location = 1;
                    break;
                }
                total += record->attack_body_part_chances_157[hit_location];
                if (location_roll < total) {
                    break;
                }
            }
            location_text = gppStringList[g_pc_hit_location_labels[hit_location][0]];
        } else {
            if (g_combat_state->TargetHit.iType != W8_TARGET_KIND_MONSTER) {
                goto invalid_target;
            }
            target_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x868, COMBAT_ATTACK_CPP, g_combat_state->TargetHit.iMonsterID, 1));
            target_record = GetMonsterDataForInfo(target_info);
            unsigned int total = 0;
            unsigned int location_roll = Random(100);
            for (hit_location = 0;; ++hit_location) {
                if (hit_location >= W8_MONSTER_HIT_LOCATIONS) {
                    FormatDebugMessage(0,
                                       "ERROR: DBS Hit Locations total only %d%% for monster %ls",
                                       total, target_record->name_00);
                    hit_location = 3;
                    break;
                }
                total += target_record->hit_location_chances_15f[hit_location];
                if (location_roll < total) {
                    break;
                }
            }
            if (target_record->hit_location_chances_15f[hit_location] < 100) {
                location_text =
                    gppStringList[g_monster_hit_location_labels[hit_location]
                                                               [target_record->constitution_15e]];
            } else {
                location_text = &g_wchar_00689b34;
            }
        }
        wcscpy(location_name, location_text);
        queued_fatigue = 1;
        if (guaranteed_hit == 0 && to_hit < roll) {
            if (BlockedForSpecialReason(attack->weapon_class_1c, &g_combat_state->TargetHit, roll,
                                        to_hit, 9) == 0) {
                if (verbose != 0) {
                    ShowNoticef(9, gppStringList[0x209]);
                }
                if (TryPanicWoundedCharacter(&g_combat_state->TargetHit) == 0 &&
                    g_combat_state->TargetHit.iType == W8_TARGET_KIND_CHARACTER &&
                    Random(100) < 0x32) {
                    event_ids[0] = g_special_event_0068c570;
                    event_ids[1] = g_special_event_0068c560;
                    event_ids[2] = g_special_event_0068c574;
                    QueueCharacterEvent(&g_status.buffers.Char[g_combat_state->TargetHit.iChar],
                                        event_ids[Random(3)], 0, g_effect_argument_005ed8c8,
                                        g_effect_argument_005ed914);
                }
                if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER &&
                    static_cast<char>(target_info->p3D->IsFacingMonster(monster_info->p3D)) != 0 &&
                    target_info->fMotionless == 0) {
                    StartMonsterCycle(target_info, 0x13, 1);
                }
            }
        } else {
            if (g_settings.verbose_combat_messages == 0) {
                report->target = g_combat_state->TargetHit;
            } else {
                ShowNoticef(9, gppStringList[0x20c], location_name);
            }
            queued_fatigue = 2;
            int penetration = (monster_info->modifiers_1db.attack_bonus_02 +
                               monster_info->modifiers_1db.hit_bonus_01) *
                                  5 +
                              attack->attack_score_04;
            penetration += AttackModeMod(1, action_detail);
            for (int i = 0; i < 0x12; ++i) {
                if (attack->ubWeaponNameIndex == g_low_fatigue_attack_verbs[i]) {
                    penetration -= FatigueArmorPenalty(monster_info->fatigue_band);
                    break;
                }
            }
            penetration -= TargetArmorClassAtLocation(&g_combat_state->TargetHit, action_detail,
                                                      hit_location) *
                           5;
            int target_sum;
            if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER) {
                unsigned int target_index = MonsterGetIndexByLocationID(
                    0xf2b, COMBAT_ATTACK_CPP, g_combat_state->TargetHit.iMonsterID, 1);
                W8MonsterInfo* target = MonsterGetScriptPartByLocationIndex(target_index);
                target_sum = target->attributes[4] + target->attributes[3] + target->attributes[2] +
                             target->attributes[1];
            } else if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_CHARACTER) {
                int target_char = g_combat_state->TargetHit.iChar;
                target_sum =
                    g_status.buffers.Char[target_char].attributes[W8_ATTRIBUTE_SENSES].effective +
                    g_status.buffers.Char[target_char].attributes[W8_ATTRIBUTE_SPEED].effective +
                    g_status.buffers.Char[target_char]
                        .attributes[W8_ATTRIBUTE_DEXTERITY]
                        .effective +
                    g_status.buffers.Char[target_char]
                        .attributes[W8_ATTRIBUTE_INTELLIGENCE]
                        .effective;
            } else {
                FormatDebugMessage(
                    1, "GetTargetAttackAttributes: Invalid Target Type %d(char %d, ID %d)",
                    g_combat_state->TargetHit.iType, g_combat_state->TargetHit.iChar,
                    g_combat_state->TargetHit.iMonsterID);
                target_sum = 0;
            }
            penetration +=
                (monster_info->attributes[4] + monster_info->attributes[3] +
                 monster_info->attributes[2] + monster_info->attributes[1] - target_sum) /
                10;
            ScaleValueForMonsterDifficulty(monster_info, &penetration);
            CombatLog("TO PENETRATE: Chance %d, Rolled was %d", penetration, roll);
            if (guaranteed_penetration == 0 && penetration < roll) {
                message_id = 0x20f;
            } else {
                queued_fatigue = 3;
                damage = ResolveMonsterAttackDamage(monster_info, attack, &dice_count);
                if (damage == 0) {
                    message_id = 0x20e;
                } else {
                    if (fumbled != 0) {
                        damage = CapAttackDamageByTargetHealth(damage);
                    } else if (dice_count > 1 && verbose != 0) {
                        ShowNoticef(9, gppStringList[0x20d], dice_count);
                    }
                    MakeMonsterHitSound(attack, &g_combat_state->TargetHit, hit_location, -1);
                    if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_CHARACTER) {
                        applied = ApplyDamageToCharacter(g_combat_state->TargetHit.iChar, damage, 0,
                                                         verbose != 0, verbose != 0,
                                                         verbose != 0 ? NULL : report, 0);
                    } else {
                        applied =
                            ApplyDamageToMonster(target_info, damage, &source, 0, verbose != 0,
                                                 verbose != 0, verbose != 0 ? NULL : report, 0);
                    }
                    if (applied != 0) {
                        memset(&effect, 0, sizeof(effect));
                        memcpy(effect.condition_chances, attack->missile_values_05, 0x10);
                        effect.power_level =
                            record->effective_level_24f +
                            (record->effective_level_24f > 0xe ? 0xf : record->effective_level_24f);
                        effect.magnitude_base_1c = attack->missile_magnitude_1b;
                        ApplyEffectConditions(&source, &g_combat_state->TargetHit, &effect, verbose,
                                              0, report);
                        if (range < W8_RANGE_LONG &&
                            g_combat_state->TargetHit.iType == W8_TARGET_KIND_CHARACTER &&
                            hit_location == 1 &&
                            g_status.buffers.Char[g_combat_state->TargetHit.iChar]
                                    .EquippedItem[W8_EQUIP_SLOT_TORSO]
                                    .iItemNo == 500) {
                            SetTargetSourceToCharacter(g_combat_state->TargetHit.iChar,
                                                       &victim_source);
                            ApplyDirectDamageToMonster(monster_info, &victim_source, damage);
                        }
                    }
                }
            }
            if (damage == 0) {
                MakeMonsterHitSound(attack, &g_combat_state->TargetHit, hit_location, 0x2a);
                if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER) {
                    MonsterReactsToBeingStruck(target_info, &source, 0);
                }
                if (verbose == 0) {
                    ++report->count;
                } else {
                    ShowNoticef(9, gppStringList[message_id]);
                }
            }
            if (range < W8_RANGE_LONG) {
                if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER) {
                    W8Enchantment* enchantment = &target_info->enchantments[3];
                    if (enchantment->turns_08 != 0) {
                        SetTargetSourceToMonster(target_info, &victim_source);
                        ApplyDiceDamageToMonster(monster_info, &victim_source, enchantment);
                        if (--enchantment->power_00 == 0) {
                            ClearMonsterEnchantmentSlot(target_info->location_id, 3);
                        }
                    }
                } else {
                    W8Enchantment* enchantment =
                        &g_status.buffers.Char[g_combat_state->TargetHit.iChar].enchantments[3];
                    if (enchantment->turns_08 != 0) {
                        SetTargetSourceToCharacter(g_combat_state->TargetHit.iChar, &victim_source);
                        ApplyDiceDamageToMonster(monster_info, &victim_source, enchantment);
                        if (--enchantment->power_00 == 0) {
                            ClearCharacterEnchantmentSlot(g_combat_state->TargetHit.iChar, 3);
                        }
                    }
                }
            }
        }
        ApplyQueuedFatigue(&g_combat_state->TargetHit, queued_fatigue, 1);
    }
    if (combat->uiSwingsRemaining == 0) {
        srAssertFail("pCmbt->uiSwingsRemaining > 0", COMBAT_ATTACK_CPP, 0x950, 0);
    }
    --combat->uiSwingsRemaining;
    if (monster_info->hp_current == 0 && monster_info->highest_condition > 0xe) {
        combat->uiSwingsRemaining = 0;
    }
    if (combat->uiSwingsRemaining != 0) {
        if (record->attack_multiple_targets_247 != 0 && range < W8_RANGE_LONG) {
            repick_target = monster_info->Target;
            if (ChooseRandomMonsterAction(monster_info, 0, 1, 0) == 0 ||
                monster_info->action_kind != 0) {
                combat->uiSwingsRemaining = 0;
            } else {
                repicked = 1;
                if (memcmp(&repick_target, &monster_info->Target, sizeof(W8CombatSlot)) != 0) {
                    ReportMonsterAttackResult(monster_info, report);
                    if (fumbled != 0 && g_combat_state->attack_report.deferred_80 != 0) {
                        ReportMonsterAttackResult(monster_info, &g_combat_state->attack_report);
                    }
                    wcscpy(g_combat_state->attack_message_6b8, gppStringList[0x210]);
                    AnnounceMonsterAttack(monster_info, record, 1);
                }
            }
        }
        if (combat->uiSwingsRemaining != 0) {
            if ((memcmp(&entry_target, &monster_info->Target, sizeof(W8CombatSlot)) == 0 ||
                 repicked != 0) &&
                TargetMatchesNeeded(&monster_info->Target, 2) != 0) {
                if (repicked == 0) {
                    StartMonsterAttackCycle(monster_info, action_detail);
                }
                if ((verbose == 0 && fumbled != 0) || source.target_diverted != 0) {
                    ReportMonsterAttackResult(monster_info, report);
                    PostMonsterNotice(monster_info, gppStringList[0x269],
                                      SpellTargetString(&source, &monster_info->Target));
                }
                return 2;
            }
        }
    }
    if (verbose == 0) {
        ReportMonsterAttackResult(monster_info, report);
        if (fumbled != 0 || source.target_diverted != 0) {
            PostMonsterNotice(monster_info, gppStringList[0x269],
                              SpellTargetString(&source, &monster_info->Target));
            if (g_combat_state->attack_report.deferred_80 != 0) {
                ReportMonsterAttackResult(monster_info, &g_combat_state->attack_report);
            }
        }
    }
    return 3;

invalid_target:
    FormatDebugMessage(g_dev_mode == 0,
                       "InvalidAttackTarget: Target Type %d(char %d,ID %d), Source Type "
                       "%d(char %d,ID %d)",
                       g_combat_state->TargetHit.iType, g_combat_state->TargetHit.iChar,
                       g_combat_state->TargetHit.iMonsterID, source.iType, source.iChar,
                       source.iMonsterID);
    return 3;
}

/* The monster-side counterpart: the same swing/damage/casualty walk reported
   against the monster's own notice stream. */
// FUNCTION: WIZ8 0x005412b0
void ReportMonsterAttackResult(W8MonsterInfo* monster_info, W8SpellEffectResult* report)
{
    unsigned int* condition_turns;
    W8SpellDamageReport* entry;
    int index;
    unsigned int condition;

    if (report->condition_counts[W8_CONDITION_DEAD] > 0) {
        if (report->notice_values[3] > 0) {
            PostMonsterNotice(monster_info, gppStringList[0x267], report->notice_values[3]);
        }
        if (report->notice_values[4] > 0) {
            PostMonsterNotice(monster_info, gppStringList[0x268], report->notice_values[4]);
        }
    } else {
        if (report->count == 0) {
            if (report->missed != 0) {
                ShowNotice(9, gppStringList[0x20b], -1, -1, 0);
                return;
            }
            ShowNotice(9, gppStringList[0x209], -1, -1, 0);
            return;
        }
        if (report->amount == 0) {
            ShowNotice(9, gppStringList[0x20e], -1, -1, 0);
        } else if (report->count == 1) {
            PostMonsterNotice(monster_info, gppStringList[0x265], report->amount);
        } else if (report->count > 1) {
            PostMonsterNotice(monster_info, gppStringList[0x266], report->count, report->amount);
        }
        if (report->notice_values[3] > 0) {
            PostMonsterNotice(monster_info, gppStringList[0x267], report->notice_values[3]);
        }
        if (report->notice_values[4] > 0) {
            PostMonsterNotice(monster_info, gppStringList[0x268], report->notice_values[4]);
        }
        if (report->target.iType == W8_TARGET_KIND_MONSTER) {
            monster_info =
                MonsterInfoFromID(0x9f5, COMBAT_ATTACK_CPP, report->target.iMonsterID, 1);
            condition_turns = monster_info->uiCondition;
        } else {
            condition_turns = g_status.buffers.Char[report->target.iChar].uiCondition;
        }
        if (condition_turns[W8_CONDITION_DEAD] == 0) {
            if (report->notice_values[0] > 0) {
                ShowNoticef(9, gppStringList[0x1a6], report->notice_values[0]);
            }
            if (report->notice_values[1] > 0) {
                ShowNoticef(9, gppStringList[0x264], report->notice_values[1]);
            }
            if (report->notice_values[2] > 0) {
                ShowNoticef(9, gppStringList[0x261], report->notice_values[2]);
            }
            for (condition = 0; condition < W8_CONDITION_COUNT; ++condition) {
                if (report->condition_counts[condition] > 0 && condition != 1) {
                    if (condition == 0x13) {
                        if (report->target.iType == W8_TARGET_KIND_CHARACTER &&
                            GetConditionRecordFlag(report->target.iChar, 1) != 0) {
                            PostCharacterNotice(report->target.iChar, g_format_s_006068e4,
                                                gppStringList[0x1d5]);
                        }
                    } else if (report->target.iType == W8_TARGET_KIND_MONSTER) {
                        ShowNoticef(9, L"%s %s!", GetMonsterName(monster_info, NULL, 0),
                                    gppStringList[g_condition_notices[condition * 4 + 1]]);
                    } else if (report->target.iType == W8_TARGET_KIND_CHARACTER) {
                        PostCharacterNotice(report->target.iChar, L"%s!",
                                            gppStringList[g_condition_notices[condition * 4 + 1]]);
                    }
                }
            }
        }
    }
    while (report->reports.count > 0) {
        entry = report->reports.data[0];
        if (report->reports.count > 0) {
            for (index = 0; index < report->reports.count - 1; ++index) {
                report->reports.data[index] = report->reports.data[index + 1];
            }
            --report->reports.count;
        }
        if (entry != NULL) {
            if (entry->kind == 1) {
                PostCharacterNotice(entry->value, L"%s!", gppStringList[g_condition_notices[0x49]]);
            } else if (entry->kind == 3) {
                ShowNoticef(9, L"%s %s!", entry->text, gppStringList[g_condition_notices[0x49]]);
            }
            free(entry);
        }
    }
    memset(static_cast<void*>(report), 0, sizeof(*report));
}

/* Begin one of the monster's attacks for the round: validate the state, rate
   the pending attack, confirm the target is still reachable, roll the swing
   count, announce it and start the cycle. */
// FUNCTION: WIZ8 0x0053fea0
char StartMonsterAttack0053FEA0(W8MonsterInfo* monster_info, W8MonsterRecord* record)
{
    W8MonsterCombatState* combat;
    unsigned int attack;

    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", COMBAT_ATTACK_CPP, 0x6f4, 0);
    }
    if (record == 0) {
        srAssertFail("pMonsterDB != NULL", COMBAT_ATTACK_CPP, 0x6f5, 0);
    }
    combat = monster_info->pCombat;
    if (combat == 0) {
        srAssertFail("pCmbt != NULL", COMBAT_ATTACK_CPP, 0x6f8, 0);
    }
    memset(static_cast<void*>(&g_combat_state->attack_report), 0, sizeof(W8SpellEffectResult));
    if (combat->attacks_per_round == 0) {
        FormatDebugMessage(
            1, "ERROR: Monster ID %d is starting attack with 0 of %d attacks remaining!",
            monster_info->location_id, combat->attacks_per_round_005);
        return 0;
    }
    attack = combat->attack_index_11;
    --combat->attacks_per_round;
    int action_detail = monster_info->action_detail;
    if (RateMonsterAttack(monster_info, record, attack, 0, 0) != 0) {
        return 0;
    }
    if (TargetMatchesNeeded(&monster_info->Target, 2) == 0 ||
        MonsterActionReachesTarget(monster_info, record, attack, &monster_info->Target) == 0) {
        if (ClearMonsterCombatSlot(monster_info) == 0) {
            return 0;
        }
    }
    combat->uiSwingsRemaining = Random(record->swings_per_round_0e6) + 1;
    AnnounceMonsterAttack(monster_info, record, 0);
    StartMonsterAttackCycle(monster_info, action_detail);
    return 1;
}

/* Attack weapon-type name string ids; the monster attack announcement indexes
   it two words per type. */
// GLOBAL: WIZ8 0x0061EB02
const unsigned short g_monster_attack_name_ids[0x25] = {
    0x619, 0x61a, 0x61b, 0x61c, 0x61d, 0x61e, 0x61f, 0x620, 0x621, 0x622, 0x623, 0x624, 0x625,
    0x626, 0x627, 0x628, 0x629, 0x62a, 0x62b, 0x62c, 0x62d, 0x62e, 0x62f, 0x630, 0x631, 0x632,
    0x633, 0x634, 0x635, 0x636, 0x637, 0x638, 0x639, 0x63a, 0x63b, 0x63c, 0x63d,
};

/* Attack verb-class string ids; the monster attack announcement appends the
   entry the attack's verb byte selects. */
// GLOBAL: WIZ8 0x0061EB4C
const unsigned short g_monster_attack_verb_ids[0x60] = {
    0x5b8, 0x5b9, 0x5ba, 0x5bb, 0x5bc, 0x5bd, 0x5be, 0x5bf, 0x5c0, 0x5c1, 0x5c2, 0x5c3,
    0x5c4, 0x5c5, 0x5c6, 0x5c7, 0x5c8, 0x5c9, 0x5ca, 0x5cb, 0x5cc, 0x5cd, 0x5ce, 0x5cf,
    0x5d0, 0x5d1, 0x5d2, 0x5d3, 0x5d4, 0x5d5, 0x5d6, 0x5d7, 0x5d8, 0x5d9, 0x5da, 0x5db,
    0x5dc, 0x5dd, 0x5de, 0x5df, 0x5e0, 0x5e1, 0x5e2, 0x5e3, 0x5e4, 0x5e5, 0x5e6, 0x5e7,
    0x5e8, 0x5e9, 0x5ea, 0x5eb, 0x5ec, 0x5ed, 0x5ee, 0x5ef, 0x5f0, 0x5f1, 0x5f2, 0x5f3,
    0x5f4, 0x5f5, 0x5f6, 0x5f7, 0x5f8, 0x5f9, 0x5fa, 0x5fb, 0x5fc, 0x5fd, 0x5fe, 0x5ff,
    0x600, 0x601, 0x602, 0x603, 0x604, 0x605, 0x606, 0x607, 0x608, 0x609, 0x60a, 0x60b,
    0x60c, 0x60d, 0x60e, 0x60f, 0x610, 0x611, 0x612, 0x613, 0x614, 0x615, 0x616, 0x617,
};

/* Whether a character catches the incoming attack in time to turn toward it.
   Only a living, alert defender or protector gets the roll; the first try
   always succeeds and each retry is 25 points harder on the senses check. */
// FUNCTION: WIZ8 0x0053d590
char CharacterNoticesAttacker(int party_slot)
{
    W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
    bool noticed = false;

    if (g_status.buffers.Char[party_slot].hp_current == 0 ||
        g_status.buffers.Char[party_slot].stamina == 0 ||
        g_status.buffers.Char[party_slot].highest_condition >= 0xe ||
        g_status.buffers.Char[party_slot].uiCondition[0xc] != 0) {
        return 0;
    }
    if (TryCharacterAction(party_slot, W8_ACTION_DEFEND, 1) == 0 &&
        TryCharacterAction(party_slot, W8_ACTION_PROTECT, 1) == 0) {
        return 0;
    }
    if (row->spot_attempts_90 == 0) {
        row->spot_attempts_90 = 1;
        return 1;
    }
    int senses = g_status.buffers.Char[party_slot].attributes[W8_ATTRIBUTE_SENSES].effective -
                 row->spot_attempts_90 * 0x19;
    noticed = Random(100) < static_cast<unsigned int>(senses);
    ++row->spot_attempts_90;
    return noticed;
}

/* Build the "<monster> <attack> <target>" announcement for the monster's
   committed attack, flag a target that never saw it coming, highlight the
   target's name and turn the monster toward what it is striking. */
// FUNCTION: WIZ8 0x00541630
void AnnounceMonsterAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record, char arg_3)
{
    W8MonsterCombatState* combat = monster_info->pCombat;
    int action_detail = monster_info->action_detail;
    unsigned int attack = combat->attack_index_11;
    W8MonsterAttack* pAttack = &record->attacks[attack];
    W8RangeCategory range = GetMonsterActionRangeCategory(monster_info, record, attack);
    W8MonsterCombatState* second_combat;
    W8MonsterInfo* second;
    wchar_t* text;
    unsigned char palette;
    unsigned char highlight_start;
    unsigned char highlight_end;
    unsigned short name_id;
    int attempts;
    bool notices;

    g_combat_state->unaware_9a4 = 0;
    if (record->kind_0cb == 3 && range <= W8_RANGE_TOUCH) {
        g_combat_state->natural_attack_9a5 = 1;
    } else {
        g_combat_state->natural_attack_9a5 = 0;
    }
    swprintf(g_combat_state->attack_message_6b8, L"%s ", GetMonsterName(monster_info, record, 0));
    if (arg_3 != 0) {
        wcscat(g_combat_state->attack_message_6b8, gppStringList[0x211]);
        wcscat(g_combat_state->attack_message_6b8, L" ");
    }
    if (g_combat_state->natural_attack_9a5 == 0) {
        if (pAttack->weapon_type_02 == 1) {
            name_id = g_attack_flag_name_ids[action_detail][1];
        } else {
            if (pAttack->weapon_type_02 == 0) {
                FormatDebugMessage(
                    0, "DATA ERROR: Attack %d weapon type is NONE for monster %ls -> Charles",
                    attack, GetMonsterName(monster_info, 0, 0));
            }
            name_id = g_monster_attack_name_ids[pAttack->weapon_type_02 * 2];
        }
        wcscat(g_combat_state->attack_message_6b8, gppStringList[name_id]);
        wcscat(g_combat_state->attack_message_6b8, L" ");
        if (pAttack->ubWeaponNameIndex != 0) {
            text = gppStringList[g_monster_attack_verb_ids[pAttack->ubWeaponNameIndex]];
            if (g_settings.verbose_combat_messages != 0) {
                if (g_combat_state->natural_attack_9a5 != 0) {
                    wcscat(g_combat_state->attack_message_6b8, L" ");
                    wcscat(g_combat_state->attack_message_6b8, gppStringList[0x215]);
                } else if (action_detail == 3) {
                    wcscat(g_combat_state->attack_message_6b8, gppStringList[0x215]);
                }
                wcscat(g_combat_state->attack_message_6b8, text);
            }
        }
    } else {
        wcscat(g_combat_state->attack_message_6b8, gppStringList[0x204]);
        wcscat(g_combat_state->attack_message_6b8, L" ");
    }
    if (g_combat_state->natural_attack_9a5 == 0 && pAttack->ubWeaponNameIndex != 0) {
        if (g_settings.verbose_combat_messages == 0) {
            switch (action_detail) {
            case 0:
            case 1:
            case 4:
                wcscat(g_combat_state->attack_message_6b8, gppStringList[0x217]);
                break;
            case 3:
                wcscat(g_combat_state->attack_message_6b8, gppStringList[0x218]);
                break;
            }
        } else {
            wcscat(g_combat_state->attack_message_6b8, gppStringList[0x216]);
        }
    }
    highlight_start = wcslen(g_combat_state->attack_message_6b8);
    if (monster_info->Target.iType == W8_TARGET_KIND_CHARACTER) {
        int party_slot = monster_info->Target.iChar;
        MonsterVsCharDisposition(party_slot, monster_info);
        wcscat(g_combat_state->attack_message_6b8, g_status.buffers.Char[party_slot].name);
        palette = g_status.buffers.XChar[party_slot].party_order_index;
        if (range <= W8_RANGE_SHORT && IsCharacterFacingMonster(party_slot, monster_info) == 0) {
            if (CharacterNoticesAttacker(party_slot) == 0) {
                g_combat_state->unaware_9a4 = IsMonsterBehindCharacter(monster_info, party_slot);
            } else {
                TurnCharacterTowardMonster(party_slot, monster_info);
            }
        }
    } else {
        if (monster_info->Target.iType != W8_TARGET_KIND_MONSTER) {
            srAssertFail("pMonsterInfo->Target.iType == TARGET_TYPE_MONSTER", COMBAT_ATTACK_CPP,
                         0xad1, 0);
        }
        unsigned int index = MonsterGetIndexByLocationID(0xad4, COMBAT_ATTACK_CPP,
                                                         monster_info->Target.iMonsterID, 1);
        second = MonsterGetScriptPartByLocationIndex(index);
        MonsterHostility(monster_info, second);
        wcscat(g_combat_state->attack_message_6b8, GetMonsterName(second, 0, 0));
        palette = 9;
        if (range <= W8_RANGE_SHORT && IsMonsterFacingMonster(second, monster_info) == 0) {
            second_combat = second->pCombat;
            notices = false;
            if (second->hp_current != 0 && second->stamina != 0 &&
                second->highest_condition < 0xe && second->uiCondition[0xc] == 0 &&
                second->action_kind == 1) {
                attempts = second_combat->spot_attempts_13d;
                if (attempts == 0) {
                    notices = true;
                } else {
                    notices = Random(100) <
                              static_cast<unsigned int>(second->attributes[4] + attempts * -0x19);
                }
                ++second_combat->spot_attempts_13d;
                if (notices) {
                    MonsterAimAtMonster(second->p3D, monster_info->p3D, 0);
                }
            }
            if (!notices) {
                g_combat_state->unaware_9a4 = IsMonsterLookingAwayFrom(monster_info, second);
            }
        }
    }
    highlight_end = wcslen(g_combat_state->attack_message_6b8);
    if (g_combat_state->natural_attack_9a5 == 0) {
        if (g_combat_state->unaware_9a4 != 0) {
            text = gppStringList[0x207];
            wcscat(g_combat_state->attack_message_6b8, text);
        }
    } else {
        wcscat(g_combat_state->attack_message_6b8, gppStringList[0x212]);
        if (pAttack->ubWeaponNameIndex == 0) {
            srAssertFail("pAttack->ubWeaponNameIndex != MON_WEAPON_NAME_UNARMED", COMBAT_ATTACK_CPP,
                         0xafd, 0);
        }
        if (pAttack->ubWeaponNameIndex != 0) {
            text = gppStringList[g_monster_attack_verb_ids[pAttack->ubWeaponNameIndex]];
            wcscat(g_combat_state->attack_message_6b8, text);
        }
    }
    if (combat->uiSwingsRemaining > 1 &&
        (record->attack_multiple_targets_247 == 0 || range > W8_RANGE_SHORT)) {
        wcscat(g_combat_state->attack_message_6b8,
               FormatWideString(L" %dx", combat->uiSwingsRemaining));
    }
    ShowNotice(9, g_combat_state->attack_message_6b8, -1, 0xffffffff, 0);
    if (palette != 9) {
        HighlightTextBoxRange(palette, highlight_start, highlight_end, -1);
    }
    OrientMonsterTowardTarget(monster_info, 0);
}

/* The character-side attack score against the pending action's target: the
   hand attack's own score plus five per point of hit and modifier-block
   bonus, the attack-mode term, the caught-unaware and natural-attack bonuses,
   the fatigue penalty - halved for modern weapons - and a repick-count
   surprise term. When the flag is clear the target's armour class and the
   INT/DEX/SPEED/SENSES differential count too; a blinded attacker caps at ten
   unless a trait lets it fight blind. Finally the difficulty scaler runs. */
// FUNCTION: WIZ8 0x00541c00
int GetTargetAttackAttributes(int party_slot, int hand, int attack_mode, char arg_4)
{
    W8Character* character = &g_status.buffers.Char[party_slot];
    W8HandAttack* attack = &character->Hand[hand];
    W8CombatSlot* target;
    int score;
    int surprise;
    int penalty;
    int target_sum;
    int attacker_sum;
    unsigned int fatigue;
    unsigned int monster_list_index;
    W8MonsterInfo* monster_info;

    score = attack->attack_score + attack->hit_bonus * 5;
    score += character->bonus_1770.hit_bonus_01 * 5;
    score += AttackModeMod(0, attack_mode);
    if (g_combat_state->unaware_9a4 != 0) {
        score += 10;
    }
    if (g_combat_state->natural_attack_9a5 != 0) {
        score += 10;
    }
    fatigue = FatigueArmorPenalty(character->fatigue_band);
    if (attack->weapon_skill == W8_SKILL_MODERN_WEAPONS) {
        fatigue >>= 1;
    }
    score -= fatigue;
    if (g_combat_state->characters[party_slot].pending_action_repick_count > 0) {
        surprise = character->attributes[W8_ATTRIBUTE_SPEED].effective -
                   g_combat_state->characters[party_slot].pending_action_repick_count * 25 - 50 +
                   attack->attack_score;
        if (surprise < 0) {
            surprise = 0;
        }
        penalty = static_cast<int>(50.0 - pow(static_cast<double>(surprise), g_double_005ebf40));
        if (penalty < 0) {
            penalty = 0;
        }
        score -= penalty;
    }
    if (arg_4 == 0) {
        target = &g_status.buffers.XChar[party_slot].target_out_of_combat;
        score -= GetTargetArmorClass(target, attack_mode) * 5;
        if (target->iType == W8_TARGET_KIND_MONSTER) {
            monster_list_index =
                MonsterGetIndexByLocationID(0xf2b, COMBAT_ATTACK_CPP, target->iMonsterID, 1);
            monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
            target_sum = monster_info->attributes[4] + monster_info->attributes[3] +
                         monster_info->attributes[2] + monster_info->attributes[1];
        } else if (target->iType == W8_TARGET_KIND_CHARACTER) {
            target_sum =
                g_status.buffers.Char[target->iChar].attributes[W8_ATTRIBUTE_SENSES].effective +
                g_status.buffers.Char[target->iChar].attributes[W8_ATTRIBUTE_SPEED].effective +
                g_status.buffers.Char[target->iChar].attributes[W8_ATTRIBUTE_DEXTERITY].effective +
                g_status.buffers.Char[target->iChar]
                    .attributes[W8_ATTRIBUTE_INTELLIGENCE]
                    .effective;
        } else {
            FormatDebugMessage(1,
                               "GetTargetAttackAttributes: Invalid Target Type %d(char %d, ID %d)",
                               target->iType, target->iChar, target->iMonsterID);
            target_sum = 0;
        }
        attacker_sum = character->attributes[W8_ATTRIBUTE_SENSES].effective +
                       character->attributes[W8_ATTRIBUTE_SPEED].effective +
                       character->attributes[W8_ATTRIBUTE_DEXTERITY].effective +
                       character->attributes[W8_ATTRIBUTE_INTELLIGENCE].effective;
        score += (attacker_sum - target_sum) / 10;
    }
    if (character->uiCondition[W8_CONDITION_BLIND] != 0 && score > 10) {
        if (CharacterHasTrait(character, W8_TRAIT_EFFECTIVE_WHILE_BLIND) == 0) {
            score = 10;
        } else {
            score =
                static_cast<int>(ScaleValueByProfessionLevel(
                    character, W8_TRAIT_EFFECTIVE_WHILE_BLIND, static_cast<float>(score - 10))) +
                10;
        }
    }
    ScaleValueForCharacterDifficulty(party_slot, &score);
    return score;
}

/* The same score against the in-combat TargetHit slot: the hand attack's
   score with the hit, value_25 and both modifier-block bonuses, the
   character-side attack-mode term, the fatigue penalty skipped entirely for
   modern weapons, then the target's armour class at the hit location and the
   same attribute differential. */
// FUNCTION: WIZ8 0x00541ec0
int GetTargetHitAttackAttributes(int party_slot, int hand, int attack_mode, int hit_location)
{
    W8Character* character = &g_status.buffers.Char[party_slot];
    W8HandAttack* attack = &character->Hand[hand];
    int score;
    int target_sum;
    int attacker_sum;
    unsigned int monster_list_index;
    W8MonsterInfo* monster_info;
    W8CombatSlot* target = &g_combat_state->TargetHit;

    score = attack->attack_score + attack->hit_bonus * 5;
    score += attack->attack_bonus_25 * 5;
    score += character->bonus_1770.hit_bonus_01 * 5;
    score += character->bonus_1770.attack_bonus_02 * 5;
    score += AttackModeMod(1, attack_mode);
    if (attack->weapon_skill != W8_SKILL_MODERN_WEAPONS) {
        score -= FatigueArmorPenalty(character->fatigue_band);
    }
    score -= TargetArmorClassAtLocation(target, attack_mode, hit_location) * 5;
    if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster_list_index =
            MonsterGetIndexByLocationID(0xf2b, COMBAT_ATTACK_CPP, target->iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        target_sum = monster_info->attributes[3] + monster_info->attributes[4] +
                     monster_info->attributes[2] + monster_info->attributes[1];
    } else if (target->iType == W8_TARGET_KIND_CHARACTER) {
        target_sum =
            g_status.buffers.Char[target->iChar].attributes[W8_ATTRIBUTE_SENSES].effective +
            g_status.buffers.Char[target->iChar].attributes[W8_ATTRIBUTE_SPEED].effective +
            g_status.buffers.Char[target->iChar].attributes[W8_ATTRIBUTE_DEXTERITY].effective +
            g_status.buffers.Char[target->iChar].attributes[W8_ATTRIBUTE_INTELLIGENCE].effective;
    } else {
        FormatDebugMessage(1, "GetTargetAttackAttributes: Invalid Target Type %d(char %d, ID %d)",
                           target->iType, target->iChar, target->iMonsterID);
        target_sum = 0;
    }
    attacker_sum = character->attributes[W8_ATTRIBUTE_SENSES].effective +
                   character->attributes[W8_ATTRIBUTE_SPEED].effective +
                   character->attributes[W8_ATTRIBUTE_DEXTERITY].effective +
                   character->attributes[W8_ATTRIBUTE_INTELLIGENCE].effective;
    score += (attacker_sum - target_sum) / 10;
    ScaleValueForCharacterDifficulty(party_slot, &score);
    return score;
}

/* Resolves one character swing: picks the dice source for the attack mode,
   rolls the effective dice count, reports hit or miss and the dice count
   through the out params, and returns the damage after bonuses and the
   target's reduction. */
// FUNCTION: WIZ8 0x005420b0
int ResolveCharacterAttackDamage(int party_slot, int hand, unsigned int attack_mode,
                                 unsigned int* out_dice_count, unsigned char* out_hit)
{
    W8Character* pPC = &g_status.buffers.Char[party_slot];
    W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
    W8MonsterInfo* monster_info = NULL;
    W8MonsterRecord* record = NULL;
    W8Character* target = NULL;
    unsigned char out_of_formation;
    unsigned char target_exposed;
    bool bVar10;

    if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER) {
        if (g_combat_state->TargetHit.iMonsterID == -1) {
            srAssertFail("gpCombat->TargetHit.iMonsterID != BAD_INDEX", COMBAT_ATTACK_CPP, 0xbf0,
                         0);
        }
        unsigned int monster_list_index = MonsterGetIndexByLocationID(
            0xbf2, COMBAT_ATTACK_CPP, g_combat_state->TargetHit.iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        record = GetMonsterDataForInfo(monster_info);
        out_of_formation = monster_info->modifiers_1db.out_of_formation;
        int action_kind = monster_info->action_kind;
        bVar10 = action_kind == 4;
        target_exposed = action_kind == 1 || action_kind == 8;
    } else {
        if (g_combat_state->TargetHit.iChar == -1) {
            srAssertFail("gpCombat->TargetHit.iChar != BAD_INDEX", COMBAT_ATTACK_CPP, 0xbff, 0);
        }
        target = &g_status.buffers.Char[g_combat_state->TargetHit.iChar];
        int engaged = GetEngagementCount();
        out_of_formation = target->bonus_1770.out_of_formation;
        bVar10 = engaged == 2;
        target_exposed = TryCharacterAction(g_combat_state->TargetHit.iChar, 4, 1) != 0 ||
                         TryCharacterAction(g_combat_state->TargetHit.iChar, 5, 1) != 0;
    }

    unsigned int dice_count = 1;
    if (attack_mode != 4 && (attack_mode < 7 || attack_mode > 8)) {
        if (bVar10 || out_of_formation != 0 || g_combat_state->unaware_9a4 != 0) {
            dice_count = 2;
        }
        if (g_combat_state->natural_attack_9a5 != 0) {
            int chance = static_cast<int>(ScaleValueByProfessionLevel(pPC, 9, 100.0f));
            unsigned int roll = Random(100);
            int extra = 0;
            if (roll + 15 < static_cast<unsigned int>(chance)) {
                extra = 1;
                if (roll + 75 < static_cast<unsigned int>(chance)) {
                    extra = 2;
                    if (roll + 95 < static_cast<unsigned int>(chance)) {
                        extra = 3;
                    }
                }
            }
            dice_count += extra;
        }
    }
    if (attack_mode == 3) {
        dice_count += 1;
    }
    if (dice_count > 1 && target_exposed != 0 && g_combat_state->unaware_9a4 == 0) {
        dice_count -= 1;
    }
    if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER &&
        pPC->Hand[hand].uiHolds != HOLDS_NOTHING) {
        if (g_item_records[pPC->EquippedItem[row->current_equip_slot].iItemNo].slays_kind_061 !=
                0xff &&
            g_item_records[pPC->EquippedItem[row->current_equip_slot].iItemNo].slays_kind_061 ==
                record->kind_0cb) {
            dice_count += 1;
        }
        if (row->paired_equip_slot != -1 &&
            g_item_records[pPC->EquippedItem[row->paired_equip_slot].iItemNo].slays_kind_061 !=
                0xff &&
            g_item_records[pPC->EquippedItem[row->paired_equip_slot].iItemNo].slays_kind_061 ==
                record->kind_0cb) {
            dice_count += 1;
        }
    }

    W8Dice dice;
    switch (attack_mode) {
    case 5:
        if (pPC->Hand[hand].uiHolds != HOLDS_NOTHING) {
            srAssertFail("pPC->Hand[uiHand].uiHolds == HOLDS_NOTHING", COMBAT_ATTACK_CPP, 0xc55, 0);
        }
        if (pPC->Hand[1].uiHolds != HOLDS_NOTHING) {
            dice = g_item_records[pPC->EquippedItem[7].iItemNo].damage_dice;
        } else {
            dice = pPC->Hand[1].damage_dice;
        }
        break;
    case 6:
        if (pPC->Hand[hand].uiHolds != HOLDS_NOTHING) {
            srAssertFail("pPC->Hand[uiHand].uiHolds == HOLDS_NOTHING", COMBAT_ATTACK_CPP, 0xc50, 0);
        }
        if (pPC->Hand[0].uiHolds != HOLDS_NOTHING) {
            int paired = -1;
            if (ItemHasSingledOutGenericName(pPC->EquippedItem[6].iItemNo) != 0 &&
                (paired = GetPairedEquipSlot(6)) != -1) {
                dice = g_item_records[pPC->EquippedItem[paired].iItemNo].damage_dice;
            } else {
                dice = g_item_records[pPC->EquippedItem[6].iItemNo].damage_dice;
            }
        } else {
            dice = pPC->Hand[0].damage_dice;
        }
        break;
    default:
        if (attack_mode != 3 && pPC->Hand[hand].uiHolds == HOLDS_NOTHING) {
            srAssertFail("pPC->Hand[uiHand].uiHolds != HOLDS_NOTHING", COMBAT_ATTACK_CPP, 0xc5c, 0);
        }
        if (pPC->Hand[hand].uiHolds == HOLDS_NOTHING) {
            dice = pPC->Hand[hand].damage_dice;
        } else {
            int item_id;
            if (hand == 0) {
                int paired = -1;
                if (ItemHasSingledOutGenericName(pPC->EquippedItem[6].iItemNo) != 0 &&
                    (paired = GetPairedEquipSlot(6)) != -1) {
                    item_id = pPC->EquippedItem[paired].iItemNo;
                } else {
                    item_id = pPC->EquippedItem[6].iItemNo;
                }
            } else {
                item_id = pPC->EquippedItem[7].iItemNo;
            }
            dice = g_item_records[item_id].damage_dice;
        }
    }

    int rolled = 0;
    for (unsigned int i = dice_count; i != 0; i = i - 1) {
        rolled += RollDice(&dice);
    }
    if (dice.count == 0) {
        *out_hit = 0;
    } else if (rolled < (g_float_005ebb38 - dice.count * g_facing_tolerance_005ebcf4) *
                            (dice.sides * dice.count + dice.base) * dice_count) {
        *out_hit = 0;
    } else {
        *out_hit = 1;
    }
    if (dice_count > 1 && Random(100) < dice_count * 20 - 20) {
        QueueCharacterEvent(pPC, g_learn_sound, 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
    }

    int damage =
        (pPC->bonus_1770.damage_percent_03 + 100 + pPC->Hand[hand].damage_percent_29) * rolled + 50;
    damage /= 100;
    if (damage < 1) {
        damage = 1;
    }
    if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER) {
        int reduction = static_cast<int>(monster_info->modifiers_1db.damage_reduction_adjustment) +
                        record->damage_reduction;
        if (reduction != 0) {
            damage = ((100 - reduction) * damage + 50) / 100;
        }
        if (damage < 0) {
            damage = 0;
        }
    } else {
        if (target->damage_reduction != 0) {
            damage = ((100 - target->damage_reduction) * damage + 50) / 100;
        }
        if (damage < 0) {
            damage = 0;
        }
        if (target->skills[W8_SKILL_IRON_SKIN].active_00 != 0) {
            PracticeCharacterSkill(target, W8_SKILL_IRON_SKIN, 1, 0);
        }
    }
    if (damage < 0) {
        *out_dice_count = dice_count;
        return 0;
    }
    *out_dice_count = dice_count;
    return damage;
}

/* The monster side of the attack-score pipeline: the attack's own score plus
   modifier and mode terms, unaware/natural bonuses, a fatigue penalty that
   halves for natural verbs, a repick surprise penalty, an optional armor and
   attribute comparison against the target, the blind cap, and difficulty. */
// FUNCTION: WIZ8 0x00542720
int GetMonsterAttackScore(W8MonsterInfo* monster_info, W8MonsterAttack* attack, int attack_mode,
                          char arg_4)
{
    W8MonsterCombatState* combat = monster_info->pCombat;
    int score = monster_info->modifiers_1db.hit_bonus_01 * 5 + attack->attack_score_04;
    score += AttackModeMod(0, attack_mode);
    if (g_combat_state != NULL) {
        if (g_combat_state->unaware_9a4 != 0) {
            score += 10;
        }
        if (g_combat_state->natural_attack_9a5 != 0) {
            score += 10;
        }
    }
    int penalty = FatigueArmorPenalty(monster_info->fatigue_band);
    for (unsigned int i = 0; i < 0x12; i = i + 1) {
        if (attack->ubWeaponNameIndex == g_low_fatigue_attack_verbs[i]) {
            penalty >>= 1;
            break;
        }
    }
    score -= penalty;
    if (combat->pending_action_repick_count > 0) {
        int surprise = monster_info->attributes[3] -
                       (combat->pending_action_repick_count * 5 + 10) * 5 + attack->attack_score_04;
        if (surprise < 0) {
            surprise = 0;
        }
        int penalty_roll =
            static_cast<int>(50.0 - pow(static_cast<double>(surprise), g_double_005ebf40));
        if (penalty_roll < 0) {
            penalty_roll = 0;
        }
        score -= penalty_roll;
    }
    if (arg_4 == 0) {
        score -= GetTargetArmorClass(&monster_info->Target, attack_mode) * 5;
        int target_sum;
        if (monster_info->Target.iType == W8_TARGET_KIND_MONSTER) {
            unsigned int target_index = MonsterGetIndexByLocationID(
                0xf2b, COMBAT_ATTACK_CPP, monster_info->Target.iMonsterID, 1);
            W8MonsterInfo* target_info = MonsterGetScriptPartByLocationIndex(target_index);
            target_sum = target_info->attributes[4] + target_info->attributes[3] +
                         target_info->attributes[2] + target_info->attributes[1];
        } else if (monster_info->Target.iType == W8_TARGET_KIND_CHARACTER) {
            int target_char = monster_info->Target.iChar;
            target_sum =
                g_status.buffers.Char[target_char].attributes[W8_ATTRIBUTE_SENSES].effective +
                g_status.buffers.Char[target_char].attributes[W8_ATTRIBUTE_SPEED].effective +
                g_status.buffers.Char[target_char].attributes[W8_ATTRIBUTE_DEXTERITY].effective +
                g_status.buffers.Char[target_char].attributes[W8_ATTRIBUTE_INTELLIGENCE].effective;
        } else {
            FormatDebugMessage(1,
                               "GetTargetAttackAttributes: Invalid Target Type %d(char %d, ID %d)",
                               monster_info->Target.iType, monster_info->Target.iChar,
                               monster_info->Target.iMonsterID);
            target_sum = 0;
        }
        score += (monster_info->attributes[4] + monster_info->attributes[3] +
                  monster_info->attributes[2] + monster_info->attributes[1] - target_sum) /
                 10;
    }
    if (monster_info->uiCondition[W8_CONDITION_BLIND] != 0 && 10 < score) {
        score = 10;
    }
    ScaleValueForMonsterDifficulty(monster_info, &score);
    return score;
}

/* The monster twin of ResolveCharacterAttackDamage: works out the effective
   dice count for the running attack, rolls the attack's dice, floors the
   result at one, applies the target's damage reduction, and reports the dice
   count through the out pointer. */
// FUNCTION: WIZ8 0x00542960
int ResolveMonsterAttackDamage(W8MonsterInfo* monster_info, W8MonsterAttack* attack,
                               unsigned int* out_dice_count)
{
    unsigned int attack_mode = monster_info->action_detail;
    W8MonsterInfo* target_info;
    W8MonsterRecord* record;
    W8Character* target;
    unsigned char out_of_formation;
    unsigned char target_exposed;
    bool bVar10;

    if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_CHARACTER) {
        if (g_combat_state->TargetHit.iChar == -1) {
            srAssertFail("gpCombat->TargetHit.iChar != BAD_INDEX", COMBAT_ATTACK_CPP, 0xd3b, 0);
        }
        target = &g_status.buffers.Char[g_combat_state->TargetHit.iChar];
        target_info = NULL;
        record = NULL;
        int engaged = GetEngagementCount();
        out_of_formation = target->bonus_1770.out_of_formation;
        bVar10 = engaged == 2;
        target_exposed = TryCharacterAction(g_combat_state->TargetHit.iChar, 4, 1) != 0 ||
                         TryCharacterAction(g_combat_state->TargetHit.iChar, 5, 1) != 0;
    } else {
        if (g_combat_state->TargetHit.iMonsterID == -1) {
            srAssertFail("gpCombat->TargetHit.iMonsterID != BAD_INDEX", COMBAT_ATTACK_CPP, 0xd48,
                         0);
        }
        unsigned int monster_list_index = MonsterGetIndexByLocationID(
            0xd4a, COMBAT_ATTACK_CPP, g_combat_state->TargetHit.iMonsterID, 1);
        target_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        record = GetMonsterDataForInfo(target_info);
        GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0xd4c, COMBAT_ATTACK_CPP, target_info->monster_group_id, 1));
        int action_kind = target_info->action_kind;
        out_of_formation = target_info->modifiers_1db.out_of_formation;
        bVar10 = action_kind == 4;
        target_exposed = action_kind == 1 || action_kind == 8;
        target = NULL;
    }

    unsigned int dice_count = 1;
    if (attack_mode != 4 && (attack_mode < 7 || attack_mode > 8)) {
        if (bVar10 || out_of_formation != 0 || g_combat_state->unaware_9a4 != 0) {
            dice_count = 2;
        }
        if (g_combat_state->natural_attack_9a5 != 0) {
            int chance = static_cast<int>(
                ScaleValueByMonsterLevel(GetMonsterDataForInfo(monster_info), 9, 100.0f));
            unsigned int roll = Random(100);
            int extra = 0;
            if (roll + 15 < static_cast<unsigned int>(chance)) {
                extra = 1;
                if (roll + 75 < static_cast<unsigned int>(chance)) {
                    extra = 2;
                    if (roll + 95 < static_cast<unsigned int>(chance)) {
                        extra = 3;
                    }
                }
            }
            dice_count += extra;
        }
    }
    if (attack_mode == 3) {
        dice_count += 1;
    }
    if (dice_count > 1 && target_exposed != 0 && g_combat_state->unaware_9a4 == 0) {
        dice_count -= 1;
    }

    int rolled = 0;
    for (unsigned int i = dice_count; i != 0; i = i - 1) {
        rolled += RollDice(&attack->damage_dice);
    }
    if (rolled <= 0) {
        rolled = 1;
    }
    if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER) {
        int reduction = static_cast<int>(target_info->modifiers_1db.damage_reduction_adjustment) +
                        record->damage_reduction;
        if (reduction != 0) {
            rolled = ((100 - reduction) * rolled + 50) / 100;
        }
        if (rolled < 0) {
            rolled = 0;
        }
    } else {
        if (target->damage_reduction != 0) {
            rolled = ((100 - target->damage_reduction) * rolled + 50) / 100;
        }
        if (rolled < 0) {
            rolled = 0;
        }
        if (target->skills[W8_SKILL_IRON_SKIN].active_00 != 0) {
            PracticeCharacterSkill(target, W8_SKILL_IRON_SKIN, 1, 0);
        }
    }
    if (rolled < 0) {
        *out_dice_count = dice_count;
        return 0;
    }
    *out_dice_count = dice_count;
    return rolled;
}

/* The condition pass behind ApplyEffectConditions: for each of the effect
   definition's sixteen condition chances, roll the percentage and, when it
   lands, resolve the condition or drain the case handles. A report passed in
   is folded into a local accumulator per condition, then merged back - unless
   a killing blow landed, in which case the report keeps only the kill. */
// FUNCTION: WIZ8 0x00543270
void ApplyEffectConditions(W8TargetSource* source, W8CombatSlot* target,
                           W8SpellEffectDefinition* definition, unsigned char announce,
                           unsigned char verbose, W8SpellEffectResult* result)
{
    W8SpellEffectResult local;
    W8SpellEffectResult* accumulator;
    W8MonsterInfo* monster_info;
    W8CharacterEvent* event;
    unsigned int highest_condition;
    unsigned int magnitude;
    int count_base;
    char resisted;
    int i;
    bool drained = false;

    if (result == NULL) {
        accumulator = NULL;
    } else {
        memset(static_cast<void*>(&local), 0, sizeof(local));
        accumulator = &local;
    }
    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        if (target->iChar == -1) {
            srAssertFail("pTarget->iChar != BAD_INDEX", COMBAT_ATTACK_CPP, 0x1015, 0);
        }
        if (g_status.buffers.XChar[target->iChar].fOccupied == 0) {
            srAssertFail("fCHAR_OCCUPIED(pTarget->iChar)", COMBAT_ATTACK_CPP, 0x1016, 0);
        }
        highest_condition = g_status.buffers.Char[target->iChar].highest_condition;
    } else {
        if (target->iType != W8_TARGET_KIND_MONSTER) {
            srAssertFail("pTarget->iType == TARGET_TYPE_MONSTER", COMBAT_ATTACK_CPP, 0x101c, 0);
        }
        if (target->iMonsterID == -1) {
            srAssertFail("pTarget->iMonsterID != BAD_INDEX", COMBAT_ATTACK_CPP, 0x101d, 0);
        }
        highest_condition =
            MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x101f, COMBAT_ATTACK_CPP, target->iMonsterID, 1))
                ->highest_condition;
    }
    for (i = 0; i < 0x10; ++i) {
        if (highest_condition > 0x11) {
            break;
        }
        if (definition->condition_chances[i] != 0) {
            if (Random(100) < definition->condition_chances[i]) {
                count_base = definition->power_level / 3 + 1;
                magnitude = RollEffectDuration(definition);
                if (magnitude < 2) {
                    SetDice(&definition->magnitude, count_base, 2, 1);
                    magnitude = RollEffectMagnitude(definition) + 1;
                }
                if (static_cast<int>(magnitude) < 1) {
                    magnitude = 1;
                }
                if (magnitude > 0xc) {
                    magnitude = 0xc;
                }
                switch (i) {
                case 0:
                    resisted = ResolveAttackOnTarget(source, target, W8_CONDITION_ASLEEP, 2,
                                                     definition->power_level, 0, magnitude, verbose,
                                                     announce, 0);
                    if (accumulator != NULL) {
                        accumulator->condition_counts[W8_CONDITION_ASLEEP] += (resisted == 0);
                    }
                    break;
                case 1:
                    resisted = ResolveAttackOnTarget(source, target, W8_CONDITION_PARALYZED, 1,
                                                     definition->power_level, 0, magnitude, verbose,
                                                     announce, 0);
                    if (accumulator != NULL) {
                        accumulator->condition_counts[W8_CONDITION_PARALYZED] += (resisted == 0);
                    }
                    break;
                case 2:
                    magnitude = RollEffectDuration(definition);
                    if (magnitude < 2) {
                        SetDice(&definition->magnitude, count_base * 3, 2,
                                definition->magnitude_base_1c);
                        magnitude = RollEffectMagnitude(definition) + 1;
                    }
                    resisted = ResolveAttackOnTarget(
                        source, target, W8_CONDITION_POISONED, 1, definition->power_level,
                        definition->magnitude_base_1c, magnitude, verbose, announce, 0);
                    if (accumulator != NULL) {
                        accumulator->condition_counts[W8_CONDITION_POISONED] += (resisted == 0);
                    }
                    break;
                case 3:
                    resisted = ResolveAttackOnTarget(source, target, W8_CONDITION_HEXED, 5,
                                                     definition->power_level, 0, magnitude, verbose,
                                                     announce, 0);
                    if (accumulator != NULL) {
                        accumulator->condition_counts[W8_CONDITION_HEXED] += (resisted == 0);
                    }
                    break;
                case 4:
                    resisted = ResolveAttackOnTarget(source, target, W8_CONDITION_DISEASED, 1,
                                                     definition->power_level, 0,
                                                     W8_CONDITION_INDEFINITE, verbose, announce, 0);
                    if (accumulator != NULL) {
                        accumulator->condition_counts[W8_CONDITION_DISEASED] += (resisted == 0);
                    }
                    break;
                case 5:
                    resisted = ResolveAttackOnTarget(source, target, W8_CONDITION_DEAD, 5,
                                                     definition->power_level, 0,
                                                     W8_CONDITION_INDEFINITE, verbose, 0, 0);
                    if (resisted == 0) {
                        if (source->iType == W8_TARGET_SOURCE_CHARACTER) {
                            ShowNoticef(8, gppStringList[0x17f]);
                            if (Random(100) < 0x32) {
                                QueueCharacterEvent(&g_status.buffers.Char[source->iChar],
                                                    g_learn_sound, 0, g_effect_argument_005ed8c8,
                                                    g_effect_argument_005ed914);
                            } else if (gXStatus.hostile_monster_count > 1 &&
                                       (event = QueueCharacterEvent(
                                            &g_status.buffers.Char[source->iChar],
                                            g_item_message_005ee668, 0, g_effect_argument_005ed8c8,
                                            g_effect_argument_005ed914)) != NULL) {
                                event->dispatch_delay_ms = 800;
                                event->dispatch_delay_start = GetTickCount();
                            }
                        } else {
                            ShowNoticef(9, gppStringList[0x17f]);
                        }
                        if (target->iType == W8_TARGET_KIND_CHARACTER) {
                            PostCharacterNotice(
                                target->iChar, L"%s!",
                                gppStringList[g_condition_notices[W8_CONDITION_DEAD * 4 + 1]]);
                        }
                        if (accumulator != NULL) {
                            accumulator->condition_counts[W8_CONDITION_DEAD]++;
                        }
                    }
                    break;
                case 6:
                    resisted = ResolveAttackOnTarget(source, target, W8_CONDITION_UNCONSCIOUS, 3,
                                                     definition->power_level, 0, magnitude, verbose,
                                                     announce, 0);
                    if (accumulator != NULL) {
                        accumulator->condition_counts[W8_CONDITION_UNCONSCIOUS] += (resisted == 0);
                    }
                    break;
                case 7:
                    resisted = ResolveAttackOnTarget(source, target, W8_CONDITION_BLIND, 0,
                                                     definition->power_level, 0, magnitude, verbose,
                                                     announce, 0);
                    if (accumulator != NULL) {
                        accumulator->condition_counts[W8_CONDITION_BLIND] += (resisted == 0);
                    }
                    break;
                case 8:
                    resisted = ResolveAttackOnTarget(source, target, W8_CONDITION_AFRAID, 4,
                                                     definition->power_level, 0, magnitude, verbose,
                                                     announce, 0);
                    if (accumulator != NULL) {
                        accumulator->condition_counts[W8_CONDITION_AFRAID] += (resisted == 0);
                    }
                    break;
                case 9:
                    if (TargetSourceIsMonster(source, 0)) {
                        if (target->iType == W8_TARGET_KIND_CHARACTER) {
                            resisted = ResolveAttackOnTarget(
                                source, target, W8_CONDITION_MISSING, 5, definition->power_level, 0,
                                W8_CONDITION_INDEFINITE, verbose, announce, 1);
                            if (resisted == 0) {
                                if (accumulator != NULL) {
                                    accumulator->condition_counts[W8_CONDITION_MISSING]++;
                                }
                                BindMonsterToCharacterDependence(target->iChar, 1,
                                                                 source->iMonsterID);
                            }
                        } else if (target->iType == W8_TARGET_KIND_MONSTER) {
                            ResolveAttackOnTarget(source, target, W8_CONDITION_DEAD, 5,
                                                  definition->power_level, 0,
                                                  W8_CONDITION_INDEFINITE, verbose, announce, 0);
                        }
                    }
                    break;
                case 10:
                    if (TargetSourceIsMonster(source, 0) &&
                        target->iType == W8_TARGET_KIND_CHARACTER) {
                        resisted = ResolveAttackOnTarget(source, target, W8_CONDITION_HOSTILE, 5,
                                                         definition->power_level, 0, magnitude,
                                                         verbose, announce, 0);
                        if (accumulator != NULL) {
                            accumulator->condition_counts[W8_CONDITION_HOSTILE] += (resisted == 0);
                        }
                    }
                    break;
                case 0xb:
                    if (target->iType == W8_TARGET_KIND_CHARACTER) {
                        resisted = TargetResistsCondition(target, 3, definition->power_level, 0);
                        if (resisted == 0) {
                            magnitude = RollEffectDuration(definition);
                            if (magnitude < 2) {
                                SetDice(&definition->magnitude, count_base, 2, 0);
                                magnitude = RollEffectMagnitude(definition) + 1;
                            }
                            ReduceMagnitudeByResistance(&magnitude, target, 3,
                                                        definition->power_level);
                            if (magnitude != 0) {
                                DamageCharacter(target->iChar, magnitude, 1);
                                if (accumulator != NULL) {
                                    accumulator->notice_values[0] += magnitude;
                                    drained = true;
                                }
                            }
                        }
                    }
                    break;
                case 0xc:
                    resisted = TargetResistsCondition(target, 1, definition->power_level, 0);
                    if (resisted == 0) {
                        magnitude = RollEffectDuration(definition);
                        if (magnitude < 2) {
                            SetDice(&definition->magnitude, count_base * 2, 5, count_base * 3);
                            magnitude = RollEffectMagnitude(definition) + 1;
                        }
                        ReduceMagnitudeByResistance(&magnitude, target, 1, definition->power_level);
                        if (magnitude != 0) {
                            if (target->iType == W8_TARGET_KIND_MONSTER) {
                                monster_info =
                                    MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                                        0x1137, COMBAT_ATTACK_CPP, target->iMonsterID, 1));
                                FatigueMonster(monster_info, magnitude, NULL);
                            } else {
                                FatigueCharacter(target->iChar, magnitude, 0, NULL);
                            }
                            if (accumulator != NULL) {
                                accumulator->notice_values[1] += magnitude;
                                drained = true;
                            }
                        }
                    }
                    break;
                case 0xd:
                    if (target->iType == W8_TARGET_KIND_CHARACTER) {
                        resisted = TargetResistsCondition(target, 5, definition->power_level, 0);
                        if (resisted == 0) {
                            magnitude = RollEffectDuration(definition);
                            if (magnitude < 2) {
                                SetDice(&definition->magnitude, count_base, 6, count_base);
                                magnitude = RollEffectMagnitude(definition) + 1;
                            }
                            ReduceMagnitudeByResistance(&magnitude, target, 5,
                                                        definition->power_level);
                            if (magnitude != 0) {
                                DrainCharacterSpellPoints(target->iChar, magnitude, 1);
                                if (target->iChar == g_status.selected_character) {
                                    RefreshFlaggedMainGameState();
                                }
                                if (accumulator != NULL) {
                                    accumulator->notice_values[2] += magnitude;
                                    drained = true;
                                }
                            }
                        }
                    }
                    break;
                case 0xe:
                    resisted = ResolveAttackOnTarget(source, target, W8_CONDITION_NAUSEATED, 2,
                                                     definition->power_level, 0, magnitude, verbose,
                                                     announce, 0);
                    if (accumulator != NULL) {
                        accumulator->condition_counts[W8_CONDITION_NAUSEATED] += (resisted == 0);
                    }
                    break;
                case 0xf:
                    resisted = ResolveAttackOnTarget(source, target, W8_CONDITION_INSANE, 4,
                                                     definition->power_level, 0, magnitude, verbose,
                                                     announce, 0);
                    if (accumulator != NULL) {
                        accumulator->condition_counts[W8_CONDITION_INSANE] += (resisted == 0);
                    }
                    break;
                }
            }
            if (target->iType == W8_TARGET_KIND_CHARACTER) {
                highest_condition = g_status.buffers.Char[target->iChar].highest_condition;
            } else {
                highest_condition = MonsterGetScriptPartByLocationIndex(
                                        MonsterGetIndexByLocationID(0x1172, COMBAT_ATTACK_CPP,
                                                                    target->iMonsterID, 1))
                                        ->highest_condition;
            }
        }
    }
    if (accumulator != NULL) {
        if (drained) {
            accumulator->condition_counts[W8_CONDITION_DRAINED]++;
        }
        if (accumulator->condition_counts[W8_CONDITION_DEAD] == 0) {
            result->notice_values[0] += accumulator->notice_values[0];
            result->notice_values[1] += accumulator->notice_values[1];
            result->notice_values[2] += accumulator->notice_values[2];
            for (i = 1; i < W8_CONDITION_COUNT; ++i) {
                result->condition_counts[i] += accumulator->condition_counts[i];
            }
        } else {
            result->notice_values[0] = 0;
            result->notice_values[1] = 0;
            result->notice_values[2] = 0;
            for (i = 1; i < W8_CONDITION_COUNT; ++i) {
                if (i == W8_CONDITION_DEAD) {
                    result->condition_counts[i] += accumulator->condition_counts[i];
                } else {
                    result->condition_counts[i] = 0;
                }
            }
        }
    }
}

/* Appends one slot-headed target record to a candidate list; both retargeting
   builders share it, so its asserts report these fixed source lines. */
static void AppendCombatTargetEntry(W8PList* out_list, W8TargetKind kind, int iChar, int iMonsterID)
{
    W8CombatSlot* pTarget = static_cast<W8CombatSlot*>(malloc(0x20));
    if (pTarget == NULL) {
        srAssertFail("pTarget", COMBAT_ATTACK_CPP, 0x1262, 0);
    }
    pTarget->iType = kind;
    pTarget->iChar = iChar;
    pTarget->iMonsterID = iMonsterID;
    int iListIndex = PLAdoptAppend(out_list, pTarget);
    if (iListIndex == -1) {
        srAssertFail("iListIndex != BAD_INDEX", COMBAT_ATTACK_CPP, 0x1273, 0);
    }
}

/* Builds the retargeting candidate list for a character: every live party
   member except the one already aimed at, plus self, then every active
   in-combat monster except the aimed one. */
// FUNCTION: WIZ8 0x00543dc0
void BuildCharacterTargetList(int party_slot, int action, W8PList* out_list)
{
    for (int i = 0; i < 8; i = i + 1) {
        W8PartySlotRow* row = &g_status.buffers.XChar[i];
        W8Character* pc = &g_status.buffers.Char[i];
        if (row->fOccupied != 0 && pc->hp_current != 0 && pc->highest_condition < 0x12) {
            if (i == party_slot) {
                AppendCombatTargetEntry(out_list, W8_TARGET_KIND_CHARACTER, i, -1);
            } else {
                if ((row->target_out_of_combat.iType == W8_TARGET_KIND_CHARACTER &&
                     row->target_out_of_combat.iChar == i) ||
                    CharacterActionReachesSlot(party_slot, action, i, 0) == 0) {
                    continue;
                }
                AppendCombatTargetEntry(out_list, W8_TARGET_KIND_CHARACTER, i, -1);
            }
        }
    }
    unsigned int monster_list_index = 0;
    while (monster_list_index < PLLength(gXStatus.plsMonsterList)) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        if (monster_info->fActive != 0 && monster_info->hp_current != 0 &&
            monster_info->fInCombat != 0 &&
            (g_status.buffers.XChar[party_slot].target_out_of_combat.iType !=
                 W8_TARGET_KIND_MONSTER ||
             g_status.buffers.XChar[party_slot].target_out_of_combat.iMonsterID !=
                 monster_info->location_id) &&
            CanPartyMemberAimAtMonster(party_slot, action, monster_info, 0, 0) != 0) {
            AppendCombatTargetEntry(out_list, W8_TARGET_KIND_MONSTER, -1,
                                    monster_info->location_id);
        }
        ++monster_list_index;
    }
}

/* The monster twin: candidates for the monster's next target. Every live
   party member other than the aimed character, then the monster itself and
   every other reachable in-combat monster other than the aimed one. */
// FUNCTION: WIZ8 0x00544010
void BuildMonsterTargetList(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                            unsigned int attack, W8PList* out_list)
{
    for (int i = 0; i < 8; i = i + 1) {
        W8PartySlotRow* row = &g_status.buffers.XChar[i];
        W8Character* pc = &g_status.buffers.Char[i];
        if (row->fOccupied != 0 && pc->hp_current != 0 && pc->highest_condition < 0x12 &&
            (monster_info->Target.iType != W8_TARGET_KIND_CHARACTER ||
             monster_info->Target.iChar != i) &&
            MonsterAttackReachesCharacter(monster_info, record, attack, i) != 0) {
            AppendCombatTargetEntry(out_list, W8_TARGET_KIND_CHARACTER, i, -1);
        }
    }
    unsigned int monster_list_index = 0;
    while (monster_list_index < PLLength(gXStatus.plsMonsterList)) {
        W8MonsterInfo* candidate = MonsterGetScriptPartByLocationIndex(monster_list_index);
        if (candidate->fActive != 0 && candidate->hp_current != 0 && candidate->fInCombat != 0) {
            if (candidate == monster_info) {
                AppendCombatTargetEntry(out_list, W8_TARGET_KIND_MONSTER, -1,
                                        candidate->location_id);
            } else {
                if ((monster_info->Target.iType == W8_TARGET_KIND_MONSTER &&
                     monster_info->Target.iMonsterID == candidate->location_id) ||
                    MonsterAttackReachesMonster(monster_info, record, attack, candidate) == 0) {
                    ++monster_list_index;
                    continue;
                }
                AppendCombatTargetEntry(out_list, W8_TARGET_KIND_MONSTER, -1,
                                        candidate->location_id);
            }
        }
        ++monster_list_index;
    }
}

/* Announces a friendly-fire strike: "<attacker> accidentally strikes
   <target>", then highlights the name that belongs to the other side's
   notice palette when the two sides differ. */
// FUNCTION: WIZ8 0x00544250
void AnnounceAccidentalStrike(W8TargetSource* source, W8CombatSlot* target)
{
    char source_color = GetSourceNoticeColor(source);
    wchar_t* message = g_combat_state->attack_message_6b8;
    unsigned char color;
    unsigned char highlight_start;
    unsigned char highlight_stop;
    if (TargetSourceIsCharacter(source, 0)) {
        color = static_cast<unsigned char>(g_status.buffers.XChar[source->iChar].party_order_index);
        wchar_t* name = g_status.buffers.Char[source->iChar].name;
        highlight_start = 0;
        highlight_stop = static_cast<unsigned char>(wcslen(name));
        swprintf(message, L"%s %s ", name, L"accidentally strikes");
        if (target->iType == W8_TARGET_KIND_CHARACTER) {
            if (source->iChar == target->iChar) {
                W8Character* pc = &g_status.buffers.Char[source->iChar];
                wcscat(message, gppStringList[g_gender_name_message_rows[pc->gender][3]]);
            } else {
                wcscat(message, g_status.buffers.Char[target->iChar].name);
            }
        } else {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x12c7, COMBAT_ATTACK_CPP, target->iMonsterID, 1));
            wcscat(message, GetMonsterName(monster_info, NULL, 0));
        }
    } else {
        if (TargetSourceIsMonster(source, 0) == 0) {
            srAssertFail("SourceIsMonster(pSource)", COMBAT_ATTACK_CPP, 0x12a7, 0);
        }
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x12a9, COMBAT_ATTACK_CPP, source->iMonsterID, 1));
        W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
        swprintf(message, L"%s %s ", GetMonsterName(monster_info, record, 0),
                 L"accidentally strikes");
        color = GetTargetNoticeColor(source, target);
        highlight_start = static_cast<unsigned char>(wcslen(message));
        if (target->iType == W8_TARGET_KIND_MONSTER && source->iMonsterID == target->iMonsterID) {
            wcscat(message, gppStringList[g_gender_name_message_rows[record->name_group_0cc][3]]);
        } else if (target->iType == W8_TARGET_KIND_CHARACTER) {
            wcscat(message, g_status.buffers.Char[target->iChar].name);
        } else {
            W8MonsterInfo* target_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x12c7, COMBAT_ATTACK_CPP, target->iMonsterID, 1));
            wcscat(message, GetMonsterName(target_info, NULL, 0));
        }
    }
    if (!TargetSourceIsCharacter(source, 0)) {
        highlight_stop = static_cast<unsigned char>(wcslen(message));
    }
    ShowNotice(source_color, message, -1, -1, 0);
    if (source_color != color) {
        HighlightTextBoxRange(color, highlight_start, highlight_stop, -1);
    }
}

/* Fumble banter after a redirected hit: half the time the attacker reacts to
   its own fumble, otherwise the struck victim speaks, or a random other party
   member when the victim is gone or incapacitated. */
// FUNCTION: WIZ8 0x00544530
void QueueFumbleReaction(int party_slot)
{
    int slot;
    if (Random(2) != 0) {
        QueueCharacterEvent(&g_status.buffers.Char[party_slot], g_item_message_005ee5c8, 0,
                            g_effect_argument_005ed8cc, g_effect_argument_005ed914);
        return;
    }
    if (g_combat_state->TargetHit.iType != W8_TARGET_KIND_CHARACTER ||
        party_slot == g_combat_state->TargetHit.iChar ||
        IsTargetStillPresent(&g_combat_state->TargetHit) == 0 ||
        (slot = g_combat_state->TargetHit.iChar,
         g_status.buffers.Char[slot].highest_condition > 0xe)) {
        slot = GetRandomCharacter(0, 0, party_slot, -1);
    }
    if (slot == -1) {
        return;
    }
    QueueCharacterEvent(&g_status.buffers.Char[slot], g_item_message_005ee5cc, 0,
                        g_effect_argument_005ed8cc, g_effect_argument_005ed914);
}

/* When GetMonsterByLocationID returns NULL the sight-flag call is skipped and
   retail read `secondary` uninitialized; deterministic zero flags model that
   defect path. */
/* The missile dispatcher shared by the character and monster attack paths:
   resolves both endpoints' world positions, defers the target when a combat-
   ending missile is already engaged, folds the range category's base speed
   into the shot and fires through FireMissile. */
// FUNCTION: WIZ8 0x00544630
W8Missile* FireMissileSourceToTarget(int missile_type, W8TargetSource* source, W8CombatSlot* target,
                                     W8SpellEffectDefinition* attack,
                                     unsigned char use_default_accuracy,
                                     unsigned int range_category, int accuracy)
{
    unsigned int target_flag;
    unsigned char blind = 0;
    unsigned char primary = 0;
    unsigned char secondary = 0;
    unsigned char position_ok;
    srVector3T<float> source_position;
    srVector3T<float> target_position;
    W8MonsterInfo* monster_info;
    W8Monster* monster;
    W8Missile* missile;
    float speed;
    float base_speed;
    float radius;
    float yaw;
    float pitch;
    unsigned int index;

    if (!use_default_accuracy && g_combat_state->engaged_missile != NULL) {
        srAssertFail("FALSE", COMBAT_ATTACK_CPP, 0x132c, 0);
        return NULL;
    }
    if (TargetSourceIsCharacter(source, 0) && target->iType == W8_TARGET_KIND_CHARACTER &&
        g_missile_table[missile_type].spell_missile_154 == 0) {
        if (use_default_accuracy) {
            return NULL;
        }
        g_combat_state->missile_hit_result = 1;
        g_combat_state->TargetHit = *target;
        return NULL;
    }
    if (target->iType == W8_TARGET_KIND_CHARACTER || target->iType == W8_TARGET_KIND_PARTY) {
        GetCameraPosition(&target_position);
    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
        index = MonsterGetIndexByLocationID(0x134f, COMBAT_ATTACK_CPP, target->iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        monster = monster_info->p3D;
        target_position.x = monster->movement_0c0.position_040.x;
        target_position.z = monster->movement_0c0.position_040.z;
        target_position.y =
            monster->movement_0c0.position_040.y + monster->movement_0c0.height_offset_0b8;
    } else if (target->iType == W8_TARGET_KIND_PLACE) {
        target_position.x = target->point.x;
        target_position.y = target->point.y;
        target_position.z = target->point.z;
    } else {
        srAssertFail("FALSE", COMBAT_ATTACK_CPP, 0x1358,
                     "FireMissileSourceToTarget: Unknown target type");
    }
    if (TargetSourceIsCharacter(source, 0)) {
        GetCharacterProjectilePosition(source->iChar, &source_position);
        if (target->iType == W8_TARGET_KIND_PLACE) {
            if (g_octree->TraceLineOfSight(&source_position, &target_position, 1, -3, -3, 1, 0) ==
                1) {
                GetCameraPosition(&source_position);
            }
        } else if (target->iType == W8_TARGET_KIND_MONSTER) {
            monster = GetMonsterByLocationID(target->iMonsterID);
            if (monster != NULL) {
                monster->GetPlayerToMonsterSightFlags(&primary, &secondary, &source_position);
            }
            if (secondary == 0 ||
                (g_missile_table[missile_type].spell_missile_154 == 0 && primary == 0)) {
                GetCameraPosition(&source_position);
            }
        }
        if (target->iType == W8_TARGET_KIND_CHARACTER || target->iType == W8_TARGET_KIND_PARTY) {
            target_flag = 0;
        } else {
            target_flag = 1;
        }
        blind = g_status.buffers.Char[source->iChar].uiCondition[W8_CONDITION_BLIND] != 0;
    } else if (TargetSourceIsMonster(source, 0)) {
        if (source->iMonsterID == -1) {
            srAssertFail("pSource->iMonsterID != -1", COMBAT_ATTACK_CPP, 0x1386, 0);
        }
        index = MonsterGetIndexByLocationID(0x1387, COMBAT_ATTACK_CPP, source->iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        monster = monster_info->p3D;
        if (g_missile_table[missile_type].spell_missile_154 == 0) {
            position_ok = monster->GetProjectilePosition(&source_position);
        } else if (source->point_source_1d == 0) {
            position_ok = monster->GetSpellPosition(&source_position);
        } else {
            position_ok = missile_type;
        }
        if (position_ok == 0) {
            source_position.x = monster->movement_0c0.position_040.x;
            source_position.z = monster->movement_0c0.position_040.z;
            source_position.y =
                monster->movement_0c0.position_040.y + monster->movement_0c0.height_offset_0b8;
            MonsterGetAnimationRadius(monster, &radius);
            pitch = GetElevationAngle(&source_position, &target_position);
            yaw = GetHeadingAngle(&source_position, &target_position);
            OffsetPositionByYawPitch(radius + radius, &source_position, yaw, pitch);
        }
        target_flag = 0;
        blind = monster_info->uiCondition[W8_CONDITION_BLIND] != 0;
    } else {
        if (source->iType != W8_TARGET_SOURCE_INDIRECT) {
            srAssertFail("FALSE", COMBAT_ATTACK_CPP, 0x13ba, 0);
            return NULL;
        }
        source_position = source->point;
        if (source->iChar == -1) {
            target_flag = source->iMonsterID != -1;
        } else {
            target_flag = 0;
        }
    }
    speed = sqrt((source_position.x - target_position.x) * (source_position.x - target_position.x) +
                 (source_position.y - target_position.y) * (source_position.y - target_position.y) +
                 (source_position.z - target_position.z) * (source_position.z - target_position.z));
    if (range_category != 0xffffffff) {
        CalcRangeDistance(range_category, source);
        base_speed = CalcRangeDistance(range_category, source);
        if (base_speed <= speed) {
            base_speed = speed;
        }
        speed = base_speed * g_monster_motion_push;
    }
    if (accuracy != 9999) {
        ScatterMissileAimPoint(&source_position, &target_position, accuracy, blind);
    }
    missile = FireMissile(missile_type, &source_position, &target_position, 0, target_flag,
                          use_default_accuracy, speed);
    if (missile != NULL) {
        missile->m_Source = *source;
        missile->combat_slot_260 = *target;
        missile->SetEffectDefinition(attack);
        if (!use_default_accuracy) {
            g_combat_state->engaged_missile = missile;
            g_combat_state->missile_hit_result = 0;
        }
        PointCameraAtCombatTarget(source, target);
        return missile;
    }
    srAssertFail("pMissileFired", COMBAT_ATTACK_CPP, 0x13e9, 0);
    return NULL;
}

/* The character-side missile fire: builds the attack block from the wielded
   (and paired) weapon's item records, scores the shot for the current hand's
   attack mode and fires at the slot's out-of-combat target. */
// FUNCTION: WIZ8 0x00544b60
void FireCharacterItemMissile(int party_slot, W8Character* pc, W8CombatCharacterRow* row,
                              unsigned int range_category)
{
    W8TargetSource source;
    W8SpellEffectDefinition attack_block;
    unsigned char item_modifiers[0x10];
    const unsigned char* modifiers;
    W8ItemDatabaseRecord* weapon;
    W8ItemDatabaseRecord* paired;
    unsigned int level;
    unsigned int capped_level;
    unsigned int missile_value;
    int accuracy;
    int i;
    int missile_type = g_item_records[row->paired_item_id_7c].missile_type;
    if (g_missile_table_count <= static_cast<unsigned int>(missile_type)) {
        FormatDebugMessage(0, "DATA ERROR: Item %ls has an invalid missile type (%d)",
                           &g_item_records[row->paired_item_id_7c], missile_type);
        missile_type = 0;
    }
    SetTargetSourceToCharacter(party_slot, &source);
    ClearAttackBlock(&attack_block);
    level = pc->uiExpLevel;
    capped_level = level;
    if (0xe < level) {
        capped_level = 0xf;
    }
    attack_block.power_level = level + capped_level;
    attack_block.magnitude = g_item_records[row->paired_item_id_7c].damage_dice;
    weapon = &g_item_records[pc->EquippedItem[row->current_equip_slot].iItemNo];
    missile_value = weapon->missile_magnitude_060;
    modifiers = weapon->missile_values_050;
    if (row->paired_equip_slot != -1) {
        paired = &g_item_records[pc->EquippedItem[row->paired_equip_slot].iItemNo];
        for (i = 0x10; i != 0; i = i - 1) {
            item_modifiers[0x10 - i] =
                weapon->missile_values_050[0x10 - i] + paired->missile_values_050[0x10 - i];
        }
        modifiers = item_modifiers;
        missile_value += paired->missile_magnitude_060;
    }
    memcpy(attack_block.condition_chances, modifiers, 0x10);
    attack_block.magnitude_base_1c = missile_value;
    accuracy = GetTargetAttackAttributes(
        party_slot, row->current_hand,
        g_status.buffers.XChar[party_slot].attack_mode[row->current_hand], 0);
    CombatLog("TO HIT (MISSILE ACCURACY): Chance %d", accuracy);
    FireMissileSourceToTarget(missile_type, &source,
                              &g_status.buffers.XChar[party_slot].target_out_of_combat,
                              &attack_block, 0, range_category, accuracy);
}

/* Displaces the aim point `to` perpendicular to the from->to line by a
   randomized cone scatter. `accuracy` shrinks the cone; `blind` swaps the
   flat 8% profile for the coarse distance-scaled one. */
// FUNCTION: WIZ8 0x005454c0
void ScatterMissileAimPoint(const srVector3T<float>* from, srVector3T<float>* to, int accuracy,
                            char blind)
{
    float dx = to->x - from->x;
    float dy = to->y - from->y;
    float dz = to->z - from->z;
    float px = dy * -dx;
    float py = dz * dz - dx * -dx;
    float pz = -(dz * dy);
    float sx = py * dz - pz * dy;
    float sy = pz * dx - dz * px;
    float sz = px * dy - py * dx;
    double angle = static_cast<double>(Random(0x168));
    float radians = static_cast<float>(3.141592653589793 * g_camera_view_factor_005ec300 * angle);
    float mag = sx * sx + sz * sz + sy * sy;
    if (mag != 0.0f) {
        float scale = static_cast<float>(sin(radians));
        scale = scale / static_cast<float>(sqrt(mag));
        sx *= scale;
        sy *= scale;
        sz *= scale;
    }
    mag = px * px + py * py + pz * pz;
    if (mag != 0.0f) {
        float scale = static_cast<float>(cos(radians));
        scale = scale / static_cast<float>(sqrt(mag));
        px *= scale;
        py *= scale;
        pz *= scale;
    }
    sx = px + sx;
    sy = py + sy;
    sz = pz + sz;
    float distance = static_cast<float>(sqrt(dy * dy + dx * dx + dz * dz));
    if (distance < static_cast<float>(g_double_005ebc30)) {
        return;
    }
    double probability;
    double factor;
    if (blind == 0) {
        factor = 0.08;
        probability = 0.2;
    } else {
        factor = g_double_005ec150 / distance;
        probability = 1.75;
        if (1.75 < factor) {
            factor = 1.75;
        }
    }
    double raw_radius = factor * distance;
    int spread = static_cast<int>(distance * g_world_cursor_scale);
    int effective = ((100 - spread) * accuracy) / 100;
    double magnitude;
    if (effective <= 0) {
        magnitude = probability * distance;
    } else if (effective >= 300) {
        magnitude = raw_radius;
    } else {
        magnitude =
            raw_radius + (probability - factor) * (300.0 - effective) * (1.0 / 300.0) * distance;
    }
    int roll = Random(100) + 1;
    int cap = effective;
    if (cap >= 0x60) {
        cap = 0x5f;
    }
    if (cap >= roll) {
        magnitude = (roll / static_cast<double>(effective)) * raw_radius;
    } else if (effective >= 1 || roll + effective >= 1) {
        double weight;
        if (effective < 1) {
            weight = (100 - (roll + effective)) * 0.01;
        } else {
            weight = (roll - effective) / static_cast<double>(roll);
        }
        magnitude = (magnitude - raw_radius) * weight + raw_radius;
    }
    mag = sx * sx + sz * sz + sy * sy;
    if (mag != 0.0f) {
        magnitude = magnitude / sqrt(mag);
        sx = sx * static_cast<float>(magnitude);
        sy = sy * static_cast<float>(magnitude);
        sz = sz * static_cast<float>(magnitude);
    }
    to->x = dx + sx + from->x;
    to->y = dy + sy + from->y;
    to->z = sz + dz + from->z;
}

/* Starts a party member's attack action: validates the hand's attack count and
   target, picks the attack mode, rolls this round's swings, builds the combat
   log line, resolves the facing and surprise checks, then either fires the
   wielded missile weapon or queues the melee swing event. */
// FUNCTION: WIZ8 0x0053d870
char StartCharacterAttack(int party_slot, int attack_mode)
{
    W8Character* character;
    W8CombatCharacterRow* row;
    W8PartySlotRow* party_row;
    W8ItemInstance* found;
    W8MonsterInfo* monster_info;
    wchar_t* verb;
    wchar_t* name;
    unsigned int mode;
    unsigned int hand;
    int range;
    bool noticed;
    unsigned int event_ids[3];

    if (static_cast<unsigned int>(party_slot) >= 8) {
        srAssertFail("uiChar < MAX_CHARS", COMBAT_ATTACK_CPP, 0x1b4, 0);
    }
    found = NULL;
    character = &g_status.buffers.Char[party_slot];
    row = &g_combat_state->characters[party_slot];
    party_row = &g_status.buffers.XChar[party_slot];
    memset(static_cast<void*>(&g_combat_state->attack_report), 0, sizeof(W8SpellEffectResult));
    if (static_cast<unsigned int>(row->current_hand) >= 2) {
        srAssertFail("uiHand < HAND_COUNT", COMBAT_ATTACK_CPP, 0x66, 0);
    }
    hand = row->current_hand;
    if (character->Hand[hand].in_play == 0) {
        if (GetCharAttackRange(character, hand) == -1) {
            row->hand_attack_values_40[hand] = 0;
            return 0;
        }
    }
    if (attack_mode == -1) {
        mode = CharChooseHandAttackMode(character, hand);
    } else {
        mode = attack_mode;
    }
    party_row->attack_mode[hand] = mode;
    if (row->hand_attack_values_40[hand] == 0) {
        FormatDebugMessage(
            1, "ERROR: %ls is starting attack with 0 of %d attacks remaining (hand %d)!",
            character->name, row->saved_attack_value[hand], hand);
        return 0;
    }
    row->hand_attack_values_40[hand]--;
    g_combat_state->unaware_9a4 = 0;
    g_combat_state->natural_attack_9a5 = 0;
    if (!TargetIsInPlay(party_slot, hand, W8_TARGETING_CONTEXT_OUT_OF_COMBAT)) {
        FormatDebugMessage(1, "ERROR: %ls is starting attack with invalid target!",
                           character->name);
        return 0;
    }
    if (hand == 0) {
        row->current_equip_slot = 6;
        if (ItemHasSingledOutGenericName(character->EquippedItem[6].iItemNo)) {
            row->paired_equip_slot = GetPairedEquipSlot(row->current_equip_slot);
        } else {
            row->paired_equip_slot = -1;
        }
    } else {
        row->current_equip_slot = 7;
        row->paired_equip_slot = -1;
    }
    row->uiSwingsRemaining = Random(character->Hand[hand].swings) + 1;
    range = GetCharAttackRange(character, hand);
    if (CharacterHasTrait(character, 9) != 0 && range <= W8_RANGE_TOUCH &&
        (character->Hand[hand].weapon_skill == 0 || character->Hand[hand].weapon_skill == 4)) {
        g_combat_state->natural_attack_9a5 = 1;
    }
    if (character->EquippedItem[row->current_equip_slot].iItemNo != -1) {
        row->uiSwingsRemaining +=
            g_item_records[character->EquippedItem[row->current_equip_slot].iItemNo]
                .swings_bonus_0ad;
    }
    if (row->extra_swings_82[0] != 0 && hand == 0) {
        if (character->Hand[0].weapon_skill == 0 &&
            character->uiCondition[W8_CONDITION_SLOWED] == 0) {
            PostCharacterNotice(party_slot, gppStringList[0x203]);
            if (Random(0xb) + character->profession_levels[character->iProfession] < 0x14) {
                row->uiSwingsRemaining += 3;
            } else {
                row->uiSwingsRemaining += 4;
            }
        }
        row->extra_swings_82[0] = 0;
    }
    row->weapon_item_id_78 = character->EquippedItem[row->current_equip_slot].iItemNo;
    if (row->paired_equip_slot == -1) {
        row->paired_item_id_7c = row->weapon_item_id_78;
    } else {
        row->paired_item_id_7c = character->EquippedItem[row->paired_equip_slot].iItemNo;
    }
    FindItemOnCharacter(character, row->weapon_item_id_78, &found, 0, NULL);
    if (found == NULL) {
        FormatDebugMessage(1, "ERROR: %ls does not have a %ls!", character->name,
                           g_item_records[row->weapon_item_id_78].display_name);
        return 0;
    }
    if (g_combat_state->natural_attack_9a5 != 0) {
        wcscpy(g_combat_state->attack_message_6b8, gppStringList[0x204]);
        wcscat(g_combat_state->attack_message_6b8, L" ");
    } else {
        if (hand == 1 && character->EquippedItem[6].iItemNo != -1 &&
            g_item_records[character->EquippedItem[6].iItemNo].equip_class == 3) {
            verb = gppStringList[g_attack_flag_name_ids[8][1]];
        } else {
            verb = gppStringList[g_attack_flag_name_ids[mode][1]];
        }
        wcscpy(g_combat_state->attack_message_6b8, verb);
        wcscat(g_combat_state->attack_message_6b8, L" ");
        if (character->Hand[hand].uiHolds != HOLDS_NOTHING) {
            name = GetItemDisplayName(found);
            if (g_settings.verbose_combat_messages != 0) {
                if (g_combat_state->natural_attack_9a5 != 0) {
                    wcscat(g_combat_state->attack_message_6b8, L" ");
                    wcscat(g_combat_state->attack_message_6b8, gppStringList[0x215]);
                } else if (mode == 3) {
                    wcscat(g_combat_state->attack_message_6b8, gppStringList[0x215]);
                }
                wcscat(g_combat_state->attack_message_6b8, name);
            }
            if (g_settings.verbose_combat_messages != 0 &&
                g_combat_state->natural_attack_9a5 == 0) {
                wcscat(g_combat_state->attack_message_6b8, gppStringList[0x216]);
            } else if (g_combat_state->natural_attack_9a5 == 0) {
                switch (mode) {
                case 0:
                case 1:
                case 4:
                    wcscat(g_combat_state->attack_message_6b8, gppStringList[0x217]);
                    break;
                case 3:
                    wcscat(g_combat_state->attack_message_6b8, gppStringList[0x218]);
                    break;
                }
            }
        } else {
            if (mode == 3) {
                wcscat(g_combat_state->attack_message_6b8, gppStringList[0x205]);
                wcscat(g_combat_state->attack_message_6b8, L" ");
            }
        }
    }
    if (party_row->target_out_of_combat.iType == W8_TARGET_KIND_MONSTER) {
        monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0x260, COMBAT_ATTACK_CPP, party_row->target_out_of_combat.iMonsterID, 1));
        wcscat(g_combat_state->attack_message_6b8, GetMonsterName(monster_info, 0, 0));
        PointCameraAtMonster(monster_info, 0, 1);
        if (range <= W8_RANGE_SHORT && !IsMonsterFacingParty(monster_info)) {
            if (monster_info->hp_current != 0 && monster_info->stamina != 0 &&
                monster_info->highest_condition < 0xe &&
                monster_info->uiCondition[W8_CONDITION_BLIND] == 0 &&
                monster_info->action_kind == 1) {
                if (monster_info->pCombat->spot_attempts_13d == 0) {
                    noticed = true;
                } else {
                    noticed = Random(100) < static_cast<unsigned int>(
                                                monster_info->attributes[4] -
                                                monster_info->pCombat->spot_attempts_13d * 25);
                }
                monster_info->pCombat->spot_attempts_13d++;
                if (noticed != 0) {
                    MonsterForwardReferencePosition(monster_info->p3D, 0);
                } else {
                    g_combat_state->unaware_9a4 = IsPartyLookingAwayFrom(party_slot, monster_info);
                }
            } else {
                g_combat_state->unaware_9a4 = IsPartyLookingAwayFrom(party_slot, monster_info);
            }
        }
    } else {
        if (party_row->target_out_of_combat.iType != W8_TARGET_KIND_CHARACTER) {
            srAssertFail("CHAR_CURRENT_TARGET(uiChar).iType == TARGET_TYPE_CHAR", COMBAT_ATTACK_CPP,
                         0x280, 0);
        }
        if (party_row->target_out_of_combat.iChar == -1) {
            srAssertFail("CHAR_CURRENT_TARGET(uiChar).iChar != BAD_INDEX", COMBAT_ATTACK_CPP, 0x281,
                         0);
        }
        wcscat(g_combat_state->attack_message_6b8,
               g_status.buffers.Char[party_row->target_out_of_combat.iChar].name);
        if (range <= W8_RANGE_SHORT &&
            !PositionFacesAsDecided(party_row->target_out_of_combat.iChar, party_slot)) {
            if (CharacterNoticesAttacker(party_row->target_out_of_combat.iChar) != 0) {
                FacePositionAsDecided(party_row->target_out_of_combat.iChar, party_slot);
            } else {
                g_combat_state->unaware_9a4 = PositionFacesOppositeToDecided(
                    party_slot, party_row->target_out_of_combat.iChar);
            }
        }
    }
    if (g_combat_state->natural_attack_9a5 != 0) {
        if (character->Hand[hand].uiHolds == HOLDS_NOTHING) {
            srAssertFail("pPC->Hand[uiHand].uiHolds != HOLDS_NOTHING", COMBAT_ATTACK_CPP, 0x29f, 0);
        }
        if (character->Hand[hand].uiHolds != HOLDS_NOTHING) {
            name = GetItemDisplayName(found);
            if (g_settings.verbose_combat_messages != 0) {
                if (g_combat_state->natural_attack_9a5 != 0) {
                    wcscat(g_combat_state->attack_message_6b8, L" ");
                    wcscat(g_combat_state->attack_message_6b8, gppStringList[0x215]);
                } else if (mode == 3) {
                    wcscat(g_combat_state->attack_message_6b8, gppStringList[0x215]);
                }
                wcscat(g_combat_state->attack_message_6b8, name);
            }
            if (g_settings.verbose_combat_messages != 0 &&
                g_combat_state->natural_attack_9a5 == 0) {
                wcscat(g_combat_state->attack_message_6b8, gppStringList[0x216]);
            } else if (g_combat_state->natural_attack_9a5 == 0) {
                switch (mode) {
                case 0:
                case 1:
                case 4:
                    wcscat(g_combat_state->attack_message_6b8, gppStringList[0x217]);
                    break;
                case 3:
                    wcscat(g_combat_state->attack_message_6b8, gppStringList[0x218]);
                    break;
                }
            }
        }
    } else {
        if (g_combat_state->unaware_9a4 != 0) {
            wcscat(g_combat_state->attack_message_6b8, gppStringList[0x207]);
        }
    }
    if (row->uiSwingsRemaining > 1) {
        wcscat(g_combat_state->attack_message_6b8,
               FormatWideString(L" %dx", row->uiSwingsRemaining));
    }
    PostCharacterNotice(party_slot, g_combat_state->attack_message_6b8);
    ResetCombatSlot(&g_combat_state->TargetHit);
    row->attack_sound_played_a5 = false;
    if (range >= W8_RANGE_LONG) {
        FireCharacterItemMissile(party_slot, character, row, range);
        MakePCAttackSound(row, &character->Hand[hand], mode, 0, -1);
        row->attack_sound_played_a5 = true;
    } else {
        event_ids[0] = g_event_range_min;
        event_ids[1] = g_special_event_0068c56c;
        event_ids[2] = g_event_range_max;
        QueueCharacterEvent(character, event_ids[Random(3)], 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
    }
    return 1;
}

/* Resolves one queued swing of the character's attack: plays the swing sound
   on the first pass, rolls fumble redirection and guardian interception,
   picks the hit location, rolls to hit and to penetrate, applies damage and
   the struck target's retaliation enchantment, consumes the thrown item or
   charge, and answers whether the attack continues. */
// FUNCTION: WIZ8 0x0053e250
int ResolveCharacterAttack(int party_slot)
{
    W8SpellEffectResult* report = &g_combat_state->attack_report;
    W8SpellEffectResult local_report;
    W8Character* character = &g_status.buffers.Char[party_slot];
    unsigned int hand = g_combat_state->characters[party_slot].current_hand;
    W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
    W8PartySlotRow* party_row = &g_status.buffers.XChar[party_slot];
    W8MonsterInfo* monster_info = NULL;
    W8TargetSource source;
    W8TargetSource target_source;
    W8CombatSlot target;
    W8SpellEffectDefinition effect;
    wchar_t location_name[20];
    char verbose = g_settings.verbose_combat_messages;
    char special_item = 0;
    char staged_miss = 0;
    bool swing_missed = false;
    char guaranteed_hit = 0;
    char guaranteed_penetration = 0;
    char fumbled = 0;
    unsigned int outcome = 1;
    unsigned int queued_fatigue = 1;
    unsigned int hit_location = 0;
    unsigned int damage = 0;
    unsigned int applied = 0;
    int message_id = 0;
    int to_hit = 0;
    int roll = 0;
    int range = 0;
    int attack_mode;

    SetTargetSourceToCharacter(party_slot, &source);
    if (character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo == 0x272) {
        special_item = 1;
        if (g_combat_state->missile_hit_result == 2) {
            if (verbose == 0) {
                report->missed = true;
            } else {
                ShowNotice(8, gppStringList[0x20a], -1, -1, 0);
            }
            ResetCombatSlot(&g_combat_state->TargetHit);
            outcome = 1;
        } else {
            outcome = 2;
        }
    } else if (TargetIsInPlay(party_slot, hand, W8_TARGETING_CONTEXT_OUT_OF_COMBAT) == 0) {
        FormatDebugMessage(g_dev_mode == 0,
                           "InvalidAttackTarget: Target Type %d(char %d,ID %d), Source Type "
                           "%d(char %d,ID %d)",
                           party_row->target_out_of_combat.iType,
                           party_row->target_out_of_combat.iChar,
                           party_row->target_out_of_combat.iMonsterID, source.iType, source.iChar,
                           source.iMonsterID);
        return 3;
    } else {
        attack_mode = party_row->attack_mode[hand];
        if (row->attack_sound_played_a5 == 0) {
            MakePCAttackSound(row, &character->Hand[hand], attack_mode, 1, -1);
            row->attack_sound_played_a5 = true;
            return 2;
        }
        target = party_row->target_out_of_combat;
        range = GetCharAttackRange(character, hand);
        if (target.iType == W8_TARGET_KIND_CHARACTER) {
            W8Character* defender = &g_status.buffers.Char[target.iChar];
            if (defender->skills[W8_SKILL_LOCKS_TRAPS].active_00 != 0) {
                g_combat_state->characters[target.iChar].skill_use_flags[W8_SKILL_LOCKS_TRAPS] = 1;
            }
            if (defender->skills[W8_SKILL_SHIELD].active_00 != 0 &&
                g_combat_state->unaware_9a4 == 0 && g_combat_state->natural_attack_9a5 == 0 &&
                defender->armor_class_components[3] > 0) {
                g_combat_state->characters[target.iChar].skill_use_flags[W8_SKILL_SHIELD] = 1;
            }
            if (defender->skills[W8_SKILL_REFLEXTION].active_00 != 0) {
                g_combat_state->characters[target.iChar].skill_use_flags[W8_SKILL_REFLEXTION] = 1;
            }
        }
        if (range < W8_RANGE_LONG) {
            to_hit = GetTargetAttackAttributes(party_slot, hand, attack_mode, 0);
            roll = Random(100) + 1;
            guaranteed_hit = roll <= 5;
            swing_missed = roll > 95;
            W8PList* fumble_list = PLCreate();
            if (fumble_list == NULL) {
                srAssertFail("plsFumbleTargetList != NULL", COMBAT_ATTACK_CPP, 0x359, 0);
            }
            BuildCharacterTargetList(party_slot, hand, fumble_list);
            int redirect_chance = GetTargetAttackAttributes(party_slot, hand, attack_mode, 1);
            int fumble_chance;
            if (redirect_chance < 100) {
                fumble_chance = static_cast<int>(pow(100 - redirect_chance, 3.0) * 1e-5 + 0.5);
                unsigned int target_count = PLLength(fumble_list);
                if (target_count == 0) {
                    srAssertFail("uiNumTargets > 0", COMBAT_ATTACK_CPP, 0x11d0, 0);
                }
                fumble_chance = target_count * fumble_chance * 10 / 100;
            } else {
                fumble_chance = 0;
            }
            ClampInteger(&fumble_chance, 0, 100);
            if (character->iRace == 0xf) {
                W8NpcState* npc_state = GetNpcState(
                    g_status.buffers.XChar[CharacterPointerToPartySlot(character)].npc_index);
                if (npc_state != NULL && npc_state->name_style == ' ' && GetFact(0x44) == 0) {
                    fumble_chance += 5;
                }
            }
            fumbled = roll > 100 - fumble_chance;
            CombatLog("TO HIT: Chance %d, Rolled %d (fumble %d%%)", to_hit, roll, fumble_chance);
            if (guaranteed_hit == 0 && fumbled != 0) {
                if (verbose == 0) {
                    memset(static_cast<void*>(&local_report), 0, sizeof(local_report));
                    report = &local_report;
                }
                unsigned int choices = PLLength(fumble_list);
                if (choices == 0) {
                    srAssertFail("uiChoices > 0", COMBAT_ATTACK_CPP, 0x127e, 0);
                }
                W8CombatSlot* element =
                    static_cast<W8CombatSlot*>(PLGet(fumble_list, Random(choices)));
                if (element == NULL) {
                    srAssertFail("pListElement != NULL", COMBAT_ATTACK_CPP, 0x1282, 0);
                }
                g_combat_state->TargetHit = *element;
                AnnounceAccidentalStrike(&source, &g_combat_state->TargetHit);
                guaranteed_hit = 1;
                guaranteed_penetration = 0;
                swing_missed = false;
                roll = Random(100) + 1;
                g_combat_state->unaware_9a4 = 0;
                g_combat_state->natural_attack_9a5 = 0;
                PLDestroy(fumble_list);
            } else {
                g_combat_state->TargetHit = party_row->target_out_of_combat;
                PLDestroy(fumble_list);
                guaranteed_penetration = guaranteed_hit;
            }
        } else if (g_combat_state->missile_hit_result == 1) {
            swing_missed = false;
            guaranteed_hit = 1;
            guaranteed_penetration = 0;
            if (CharacterHasTrait(character, W8_TRAIT_THROWN_CRITICALS) != 0 &&
                character->Hand[hand].combat_skill == W8_SKILL_RANGED_COMBAT &&
                character->Hand[hand].weapon_skill == W8_SKILL_THROWING_SLING &&
                g_item_records[character->EquippedItem[row->current_equip_slot].iItemNo]
                        .unidentified_name_index != 0xd) {
                guaranteed_penetration = 1;
            }
            roll = Random(100) + 1;
        } else {
            if (g_combat_state->missile_hit_result == 2) {
                swing_missed = false;
                staged_miss = 1;
            } else {
                swing_missed = true;
            }
        }
        if (verbose == 0) {
            report->deferred_80 = 1;
        }
        if (swing_missed != 0) {
            if (g_settings.verbose_combat_messages != 0) {
                ShowNotice(8, gppStringList[0x209], -1, -1, 0);
            }
            ResetCombatSlot(&g_combat_state->TargetHit);
        } else if (staged_miss == 0) {
            source.target_diverted =
                memcmp(&target, &g_combat_state->TargetHit, sizeof(W8CombatSlot)) != 0;
            if (IsTargetStillPresent(&g_combat_state->TargetHit) == 0) {
                FormatDebugMessage(g_dev_mode == 0,
                                   "InvalidAttackTarget: Target Type %d(char %d,ID %d), Source "
                                   "Type %d(char %d,ID %d)",
                                   g_combat_state->TargetHit.iType, g_combat_state->TargetHit.iChar,
                                   g_combat_state->TargetHit.iMonsterID, source.iType, source.iChar,
                                   source.iMonsterID);
                return 3;
            }
            if (verbose == 0 && source.target_diverted != 0) {
                memset(static_cast<void*>(&local_report), 0, sizeof(local_report));
                report = &local_report;
            }
            ResolveGuardianInterception(&source, &g_combat_state->TargetHit);
            PointCameraAtCombatTarget(&source, &g_combat_state->TargetHit);
            if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER) {
                unsigned int location_index = MonsterGetIndexByLocationID(
                    1000, COMBAT_ATTACK_CPP, g_combat_state->TargetHit.iMonsterID, 1);
                monster_info = MonsterGetScriptPartByLocationIndex(location_index);
                W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
                unsigned int total = 0;
                unsigned int location_roll = Random(100);
                for (hit_location = 0;; ++hit_location) {
                    if (hit_location >= W8_MONSTER_HIT_LOCATIONS) {
                        FormatDebugMessage(
                            0, "ERROR: DBS Hit Locations total only %d%% for monster %ls", total,
                            record->name_00);
                        hit_location = 3;
                        break;
                    }
                    total += record->hit_location_chances_15f[hit_location];
                    if (location_roll < total) {
                        break;
                    }
                }
                if (record->hit_location_chances_15f[hit_location] < 100) {
                    wcscpy(location_name,
                           gppStringList[g_monster_hit_location_labels[hit_location]
                                                                      [record->constitution_15e]]);
                } else {
                    wcscpy(location_name, &g_wchar_00689b34);
                }
            } else {
                if (g_combat_state->TargetHit.iType != W8_TARGET_KIND_CHARACTER) {
                    FormatDebugMessage(g_dev_mode == 0,
                                       "InvalidAttackTarget: Target Type %d(char %d,ID %d), Source "
                                       "Type %d(char %d,ID %d)",
                                       g_combat_state->TargetHit.iType,
                                       g_combat_state->TargetHit.iChar,
                                       g_combat_state->TargetHit.iMonsterID, source.iType,
                                       source.iChar, source.iMonsterID);
                    return 3;
                }
                unsigned int total = 0;
                unsigned int location_roll = Random(100);
                for (hit_location = 0;; ++hit_location) {
                    if (hit_location >= W8_PC_HIT_LOCATIONS) {
                        FormatDebugMessage(1, "ERROR: gubLocalACPercent total only %d%%");
                        hit_location = 1;
                        break;
                    }
                    total += gubLocalACPercent[hit_location];
                    if (location_roll < total) {
                        break;
                    }
                }
                wcscpy(location_name, gppStringList[g_pc_hit_location_labels[hit_location][0]]);
            }
            if (guaranteed_hit == 0 && to_hit < roll) {
                int weapon_class;
                if (character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo == -1) {
                    weapon_class = 9;
                } else {
                    weapon_class =
                        g_item_records[character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON]
                                           .iItemNo]
                            .weapon_sound_class_0c5;
                }
                if (BlockedForSpecialReason(weapon_class, &g_combat_state->TargetHit, roll, to_hit,
                                            8) == 0) {
                    if (g_settings.verbose_combat_messages != 0) {
                        ShowNoticef(8, gppStringList[0x209]);
                    }
                    if (character->hp_current / static_cast<float>(character->uiHPMax) <
                        g_navigator_mode3_scale) {
                        if (Random(100) < 0x1e) {
                            QueueCharacterEvent(character, g_special_event_0068c558, 0,
                                                g_effect_argument_005ed8c8,
                                                g_effect_argument_005ed914);
                        }
                    } else if (Random(100) < 0x19) {
                        QueueCharacterEvent(character, g_special_event_0068c558, 0,
                                            g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                    }
                    if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER &&
                        static_cast<char>(monster_info->p3D->IsFacingPlayer()) != 0 &&
                        monster_info->fMotionless == 0) {
                        StartMonsterCycle(monster_info, 0x13, 1);
                    }
                }
            } else {
                if (g_settings.verbose_combat_messages == 0) {
                    report->target = g_combat_state->TargetHit;
                } else {
                    ShowNoticef(8, gppStringList[0x20c], location_name);
                }
                outcome = 2;
                queued_fatigue = 2;
                unsigned int mode = attack_mode;
                if (row->paired_equip_slot != -1) {
                    mode = CharChooseHandAttackMode(character, 1);
                }
                int penetrate = GetTargetHitAttackAttributes(party_slot, hand, mode, hit_location);
                CombatLog("TO PENETRATE: Chance %d, Rolled %d", penetrate, roll);
                if (guaranteed_penetration == 0 && penetrate < roll) {
                    message_id = 0x20f;
                } else {
                    outcome = 3;
                    queued_fatigue = 3;
                    unsigned int dice_count = 0;
                    unsigned char hit_flag = 0;
                    damage = ResolveCharacterAttackDamage(party_slot, hand, attack_mode,
                                                          &dice_count, &hit_flag);
                    if (damage == 0) {
                        message_id = 0x20e;
                    } else {
                        outcome = 4;
                        if (fumbled != 0) {
                            damage = CapAttackDamageByTargetHealth(damage);
                        } else if (dice_count > 1) {
                            if (verbose != 0) {
                                ShowNoticef(8, gppStringList[0x20d], dice_count);
                            }
                            outcome = 5;
                        }
                        W8HandAttack* hand_attack = &character->Hand[hand];
                        MakePCMeleeHitSound(party_slot, hand_attack, &g_combat_state->TargetHit,
                                            hit_location, -1);
                        IsTargetStillPresent(&g_combat_state->TargetHit);
                        if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER) {
                            applied =
                                ApplyDamageToMonster(monster_info, damage, &source, 0, verbose != 0,
                                                     verbose != 0, verbose != 0 ? NULL : report, 0);
                            if (monster_info->hp_current == 0 && hit_flag != 0 && Random(2) == 0 &&
                                gXStatus.hostile_monster_count > 1) {
                                W8CharacterEvent* event = QueueCharacterEvent(
                                    character, g_item_message_005ee668, 0,
                                    g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                                if (event != NULL) {
                                    event->dispatch_delay_ms = 800;
                                    event->dispatch_delay_start = GetTickCount();
                                }
                            }
                        } else {
                            applied = ApplyDamageToCharacter(g_combat_state->TargetHit.iChar,
                                                             damage, 0, verbose != 0, verbose != 0,
                                                             verbose != 0 ? NULL : report, 0);
                        }
                        if (fumbled != 0) {
                            QueueFumbleReaction(party_slot);
                        }
                        memset(&effect, 0, sizeof(effect));
                        effect.power_level =
                            character->uiExpLevel +
                            (character->uiExpLevel > 0xe ? 0xf : character->uiExpLevel);
                        if (applied != 0) {
                            if (hand_attack->uiHolds == HOLDS_NOTHING) {
                                memcpy(effect.condition_chances, &hand_attack->condition_chance_33,
                                       0x10);
                                effect.magnitude_base_1c = 0;
                            } else {
                                memcpy(
                                    effect.condition_chances,
                                    g_item_records[character->EquippedItem[row->current_equip_slot]
                                                       .iItemNo]
                                        .missile_values_050,
                                    0x10);
                                effect.magnitude_base_1c =
                                    g_item_records[character->EquippedItem[row->current_equip_slot]
                                                       .iItemNo]
                                        .missile_magnitude_060;
                                if (row->paired_equip_slot != -1) {
                                    const unsigned char* paired_values =
                                        g_item_records[character
                                                           ->EquippedItem[row->paired_equip_slot]
                                                           .iItemNo]
                                            .missile_values_050;
                                    for (int i = 0; i < 0x10; ++i) {
                                        effect.condition_chances[i] += paired_values[i];
                                    }
                                    effect.magnitude_base_1c +=
                                        g_item_records[character
                                                           ->EquippedItem[row->paired_equip_slot]
                                                           .iItemNo]
                                            .missile_magnitude_060;
                                }
                            }
                            if (fumbled == 0) {
                                unsigned int skill_level = 0;
                                if (hand_attack->combat_skill == W8_SKILL_CLOSE_COMBAT) {
                                    skill_level = character->skills[W8_SKILL_CRITICAL_STRIKE].level;
                                } else if (hand_attack->combat_skill == W8_SKILL_RANGED_COMBAT &&
                                           hand_attack->weapon_skill > W8_SKILL_SHIELD) {
                                    if (hand_attack->weapon_skill <= W8_SKILL_BOW) {
                                        if (CharacterHasTrait(character,
                                                              W8_TRAIT_RANGED_CRITICALS) != 0) {
                                            skill_level =
                                                character->skills[W8_SKILL_RANGED_COMBAT].level;
                                        }
                                    } else if (hand_attack->weapon_skill ==
                                               W8_SKILL_THROWING_SLING) {
                                        if (g_item_records
                                                [character->EquippedItem[row->current_equip_slot]
                                                     .iItemNo]
                                                    .unidentified_name_index == 0xd) {
                                            if (CharacterHasTrait(character,
                                                                  W8_TRAIT_RANGED_CRITICALS) != 0) {
                                                skill_level =
                                                    character->skills[W8_SKILL_RANGED_COMBAT].level;
                                            }
                                        } else if (CharacterHasTrait(
                                                       character, W8_TRAIT_THROWN_CRITICALS) != 0) {
                                            skill_level =
                                                character->skills[W8_SKILL_CRITICAL_STRIKE].level;
                                        }
                                    }
                                }
                                if (skill_level != 0) {
                                    char bonus = static_cast<char>(skill_level / 0x19);
                                    if (skill_level % 0x19 != 0 &&
                                        Random(0x19) < skill_level % 0x19) {
                                        ++bonus;
                                    }
                                    effect.condition_chances[5] += bonus;
                                }
                                if (CharacterHasTrait(character, W8_TRAIT_KNOCKOUT) != 0) {
                                    effect.condition_chances[6] +=
                                        static_cast<char>(ScaleValueByProfessionLevel(
                                            character, W8_TRAIT_KNOCKOUT, 5.0));
                                }
                            }
                            ApplyEffectConditions(&source, &g_combat_state->TargetHit, &effect,
                                                  verbose, 0, report);
                            if (character->EquippedItem[row->current_equip_slot].iItemNo == 0x1f8) {
                                unsigned int heal = damage / 3;
                                unsigned int missing = character->uiHPMax - character->hp_current;
                                if (missing < heal) {
                                    heal = missing;
                                }
                                if (heal != 0) {
                                    HealCharacter(party_slot, heal, verbose);
                                    if (verbose == 0) {
                                        report->notice_values[5] += heal;
                                    }
                                }
                            }
                            if (range < W8_RANGE_LONG &&
                                g_combat_state->TargetHit.iType == W8_TARGET_KIND_CHARACTER &&
                                hit_location == 1 &&
                                g_status.buffers.Char[g_combat_state->TargetHit.iChar]
                                        .EquippedItem[W8_EQUIP_SLOT_TORSO]
                                        .iItemNo == 500) {
                                SetTargetSourceToCharacter(g_combat_state->TargetHit.iChar,
                                                           &target_source);
                                ApplyDirectDamageToCharacter(party_slot, &target_source, damage);
                            }
                        }
                    }
                }
            }
            if (damage == 0) {
                MakePCMeleeHitSound(party_slot, &character->Hand[hand], &g_combat_state->TargetHit,
                                    hit_location, 0x2a);
                if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER) {
                    MonsterReactsToBeingStruck(monster_info, &source, 0);
                }
                if (verbose == 0) {
                    ++report->count;
                } else {
                    ShowNoticef(8, gppStringList[message_id]);
                }
            }
            if (range < W8_RANGE_LONG) {
                if (g_combat_state->TargetHit.iType == W8_TARGET_KIND_MONSTER) {
                    W8Enchantment* enchantment = &monster_info->enchantments[3];
                    if (enchantment->turns_08 != 0) {
                        SetTargetSourceToMonster(monster_info, &target_source);
                        ApplyDiceDamageToCharacter(party_slot, &target_source, enchantment);
                        if (--enchantment->power_00 == 0) {
                            ClearMonsterEnchantmentSlot(monster_info->location_id, 3);
                        }
                    }
                } else {
                    W8Enchantment* enchantment =
                        &g_status.buffers.Char[g_combat_state->TargetHit.iChar].enchantments[3];
                    if (enchantment->turns_08 != 0) {
                        SetTargetSourceToCharacter(g_combat_state->TargetHit.iChar, &target_source);
                        ApplyDiceDamageToCharacter(party_slot, &target_source, enchantment);
                        if (--enchantment->power_00 == 0) {
                            ClearCharacterEnchantmentSlot(g_combat_state->TargetHit.iChar, 3);
                        }
                    }
                }
            }
            ApplyQueuedFatigue(&g_combat_state->TargetHit, queued_fatigue, 1);
        } else {
            if (verbose == 0) {
                report->missed = true;
            } else {
                ShowNotice(8, gppStringList[0x20a], -1, -1, 0);
            }
            ResetCombatSlot(&g_combat_state->TargetHit);
        }
        if (fumbled != 0) {
            outcome = 1;
        } else if (outcome > 3) {
            if (character->Hand[hand].combat_skill == W8_SKILL_CLOSE_COMBAT) {
                if (character->skills[W8_SKILL_CRITICAL_STRIKE].active_00 != 0) {
                    PracticeCharacterSkill(character, W8_SKILL_CRITICAL_STRIKE, 1, 0);
                }
                if (character->skills[W8_SKILL_POWER_STRIKE].active_00 != 0) {
                    PracticeCharacterSkill(character, W8_SKILL_POWER_STRIKE, 1, 0);
                }
            } else if (character->Hand[hand].combat_skill == W8_SKILL_RANGED_COMBAT) {
                if (character->skills[W8_SKILL_EAGLE_EYE].active_00 != 0) {
                    PracticeCharacterSkill(character, W8_SKILL_EAGLE_EYE, 1, 0);
                }
                if (CharacterHasTrait(character, W8_TRAIT_THROWN_CRITICALS) != 0 &&
                    character->skills[W8_SKILL_CRITICAL_STRIKE].active_00 != 0) {
                    PracticeCharacterSkill(character, W8_SKILL_CRITICAL_STRIKE, 1, 0);
                }
            }
        }
    }
    if (row->hand_records[hand].score < static_cast<int>(outcome)) {
        row->hand_records[hand].score = outcome;
        row->hand_records[hand].weapon_skill = character->Hand[hand].weapon_skill;
        row->hand_records[hand].combat_skill = character->Hand[hand].combat_skill;
        row->hand_records[hand].dual_wielding = character->dual_wielding;
    }
    FatigueCharacter(party_slot, CharacterActionFatigueCost(party_slot, party_row->pending_action),
                     1, 0);
    if (row->paired_equip_slot != -1) {
        int item_id = character->EquippedItem[row->paired_equip_slot].iItemNo;
        RemoveCharacterItem(character, &character->EquippedItem[row->paired_equip_slot], 1);
        if (character->EquippedItem[row->paired_equip_slot].stack_count == 0) {
            EquipMatchingPartnerItem(character, &character->EquippedItem[row->current_equip_slot],
                                     item_id, row->paired_equip_slot);
        }
    } else {
        W8ItemInstance* item = &character->EquippedItem[row->current_equip_slot];
        if (character->Hand[hand].combat_skill == W8_SKILL_RANGED_COMBAT &&
            g_item_records[item->iItemNo].spell_id == 0) {
            if (g_item_records[item->iItemNo].equip_class == 2) {
                if ((g_item_records[item->iItemNo].flags_041 & 0x80) == 0) {
                    if (g_item_records[item->iItemNo].quantity_kind == 1) {
                        --item->stack_count;
                        if (character->EquippedItem[row->current_equip_slot].stack_count == 0) {
                            EmptyItemRecord(&character->EquippedItem[row->current_equip_slot],
                                            character, 0);
                            SplitThrowableStackBetweenHands(character, row->current_equip_slot);
                            if (character->EquippedItem[row->current_equip_slot].stack_count == 0) {
                                EmptyItemRecord(&character->EquippedItem[row->current_equip_slot],
                                                character, 1);
                            }
                        }
                    } else {
                        RemoveCharacterItem(character, item, 1);
                        SplitThrowableStackBetweenHands(character, row->current_equip_slot);
                    }
                    if (character->EquippedItem[row->current_equip_slot].iItemNo == -1 ||
                        character->EquippedItem[row->current_equip_slot].stack_count == 0) {
                        row->uiSwingsRemaining = 1;
                    }
                }
            } else {
                RemoveCharacterItem(character, item, 0);
                if (character->EquippedItem[row->current_equip_slot].uses_or_charges == 0) {
                    MergeMatchingPartnerItem(character,
                                             &character->EquippedItem[row->current_equip_slot]);
                    RefreshAfterItemRecordChange(&character->EquippedItem[row->current_equip_slot],
                                                 character, 1);
                }
            }
        }
    }
    if (row->uiSwingsRemaining == 0) {
        srAssertFail("pCmbt->uiSwingsRemaining > 0", COMBAT_ATTACK_CPP, 0x611, 0);
    }
    --row->uiSwingsRemaining;
    if (character->hp_current == 0 || character->highest_condition >= 0xf) {
        row->uiSwingsRemaining = 0;
    }
    if (row->uiSwingsRemaining != 0) {
        if (hand >= 2) {
            srAssertFail("uiHand < HAND_COUNT", COMBAT_ATTACK_CPP, 0x66, 0);
        }
        if (character->Hand[hand].in_play != 0 && GetCharAttackRange(character, hand) != -1 &&
            memcmp(&target, &party_row->target_out_of_combat, sizeof(W8CombatSlot)) == 0 &&
            TargetIsInPlay(party_slot, hand, W8_TARGETING_CONTEXT_OUT_OF_COMBAT) != 0) {
            if (verbose == 0 && (fumbled != 0 || source.target_diverted != 0)) {
                ReportCharacterAttackResult(party_slot, report);
                PostCharacterNotice(party_slot, gppStringList[0x269],
                                    SpellTargetString(&source, &party_row->target_out_of_combat));
            }
            ResetCombatSlot(&g_combat_state->TargetHit);
            row->attack_sound_played_a5 = false;
            if (range >= W8_RANGE_LONG) {
                FireCharacterItemMissile(party_slot, character, row, range);
                MakePCAttackSound(row, &character->Hand[hand], attack_mode, 0, -1);
                row->attack_sound_played_a5 = true;
            }
            return 2;
        }
    }
    if (verbose == 0 && special_item == 0) {
        ReportCharacterAttackResult(party_slot, report);
        if (fumbled != 0 || source.target_diverted != 0) {
            PostCharacterNotice(party_slot, gppStringList[0x269],
                                SpellTargetString(&source, &party_row->target_out_of_combat));
            if (g_combat_state->attack_report.deferred_80 != 0) {
                ReportCharacterAttackResult(party_slot, &g_combat_state->attack_report);
            }
        }
    }
    return 3;
}
