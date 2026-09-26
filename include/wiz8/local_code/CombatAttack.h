#pragma once

#include "wiz8/dice.h"

struct W8CombatSlot;
struct W8CombatCharacterRow;
struct W8MonsterInfo;
struct W8SpellEffectDefinition;
struct W8SpellEffectResult;
struct W8TargetSource;
class W8Missile;
template <class T> class srVector3T;

/* The five character hit locations, in the order gubLocalACPercent and the
   per-location armour classes use them. */
enum { W8_PC_HIT_LOCATIONS = 5 };
/* The seven monster hit locations and the six body types that name them. */
enum { W8_MONSTER_HIT_LOCATIONS = 7, W8_MONSTER_BODY_TYPES = 6 };

/* 0x0061E7B0: gppStringList indices naming each character hit location,
   paired with a second form the missile hit does not use. */
extern const unsigned short g_pc_hit_location_labels[5][2];
/* 0x0061EA24: gppStringList indices naming each monster hit location for
   each body type. */
extern const unsigned short g_monster_hit_location_labels[W8_MONSTER_HIT_LOCATIONS]
                                                         [W8_MONSTER_BODY_TYPES];

/* 0x0061E9CC: gppStringList indices naming the sixteen damage channels the
   missile_values arrays on monster attacks and item records carry. */
extern const unsigned short g_damage_type_name_ids[0x10];
/* Paired label ids for the nine W8ItemDatabaseRecord::attack_flags_04e bits;
   AssayDialog reads the first of each pair, combat logging the second. */
extern const unsigned short g_attack_flag_name_ids[9][2];
/* Label ids for an item's special-category byte, also indexed by the MIPE
   editor's category selector. */
extern const unsigned short g_special_category_name_ids[42];

/* The missile attack paths hand FireMissileSourceToTarget the W8SpellEffectDefinition
   the fired missile stores: the weapon or monster attack's dice magnitude and
   per-condition chances ride the same record a spell effect definition does. */
void ClearAttackBlock(W8SpellEffectDefinition* block); /* 0x00543260 */
W8Missile* FireMissileSourceToTarget(int missile_type, W8TargetSource* source, W8CombatSlot* target,
                                     W8SpellEffectDefinition* attack,
                                     unsigned char use_default_accuracy,
                                     unsigned int range_category, int accuracy); /* 0x00544630 */
void ScatterMissileAimPoint(const srVector3T<float>* from, srVector3T<float>* to, int accuracy,
                            char blind);

/* 0x00545090: a physical missile reached its combat target - announce the
   hit, roll penetration against the target's armour and apply the damage.
   `deflected` reports the target's missile-deflection roll succeeded. */
void ResolveMissileHit(W8Missile* missile, bool deflected);
/* 0x00544D30: a spell missile reached its combat target - play its impact
   sound and apply the carried spell effect to the target or the area. */
void ResolveSpellMissileHit(W8Missile* missile);
/* 0x00542FC0: the armour class a hit at one location must beat, for the
   attack mode used. */
int TargetArmorClassAtLocation(W8CombatSlot* target, int attack_mode, int hit_location);
/* 0x00543270: roll the effect definition's condition chances against the
   target and apply the ones that take, reporting into `result` when one is
   given. */
void ApplyEffectConditions(W8TargetSource* source, W8CombatSlot* target,
                           W8SpellEffectDefinition* definition, unsigned char announce,
                           unsigned char verbose, W8SpellEffectResult* result);

struct W8MonsterRecord;
struct W8MonsterAttack;

/* Whether a character could attack what `target` names - a party member who
   is in play and not screened by the front rank, or a monster who is engaged,
   alive, targetable and within the character's reach. */
bool CharacterHasAttackOn(int party_slot, W8CombatSlot* target); /* 0x00545C20 */
/* Whether the character can berserk - has the fighter's ability (trait
   W8_TRAIT_BERSERK), a hand that can reach, and a primary hand that fights at
   short range or closer. The attack sub-menu entry it gates is "Berserk". */
bool CanCharacterBerserk(int party_slot); /* 0x005458A0 */
/* Whether a monster would press an attack on what `target` names: a hostile it
   can reach that outranks it, and that is either below forty percent health or
   out of formation. */
bool MonsterHasAttackOn(W8MonsterInfo* monster_info, W8CombatSlot* target); /* 0x00545CF0 */
/* Whether the monster is in a state to attack at all: in the world, in combat,
   alive, below the deactivation threshold, flagged as attacking by its record
   and carrying a first attack. */
bool CanMonsterAttack(W8MonsterInfo* monster_info); /* 0x00545BD0 */

/* What RateMonsterAttack reports for one of a monster's three attacks: zero
   when the attack can be made, otherwise why not. */
enum {
    W8_MONSTER_ATTACK_USABLE = 0,
    W8_MONSTER_ATTACK_NOT_USABLE = 1,
    W8_MONSTER_ATTACK_OUT_OF_REACH = 3
};

/* An attack the record does not carry, or carries with bad data, is not
   usable; otherwise the attack is usable when it can reach someone, judged as
   though the monster were idle. `hostile_only` narrows the sweep to enemies.
   RateMonsterBestAttack answers zero as soon as any attack is usable, one for a
   motionless monster, and otherwise the highest reason it saw. */
unsigned char RateMonsterAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                unsigned int attack, int unused, int hostile_only); /* 0x0053D4B0 */
unsigned char RateMonsterBestAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                    int hostile_only); /* 0x0053D450 */
bool CanAnyHandReachTarget(int party_slot);            /* 0x0053D310 */
/* 0x0053D2A0: whether `hand` is in play and has a range category at all. */
bool CanHandReachTarget(int party_slot, unsigned int hand);
bool CanCharacterAttack(int party_slot); /* 0x00545850 */
struct W8Character;

void GetCharacterHandDamageDice(const W8Character* character, int hand, W8Dice* dice);
int GetCharacterHandDamageBonus(const W8Character* character, int hand);

int GetHandAttackValue(int party_slot, unsigned int hand);
int NormalizeAttackMode(int attack_mode);
unsigned int ChooseAttackMode(unsigned int attack_modes);

/* 0x005459B0: how much of a hit a monster actually takes - its own adjustment
   and the record's reduction add, the remainder is taken as a percentage
   rounding to nearest, and nothing goes below zero. */
int ApplyDamageReduction(const W8MonsterInfo* monster_info, const W8MonsterRecord* record,
                         int damage);
/* 0x00545950: the character-side counterpart - the character's own reduction
   taken the same way, and having the damage-reduction skill practises it. */
int ApplyCharacterDamageReduction(W8Character* character, int damage);

int ChooseCharacterAttackHand(int party_slot);                                   /* 0x0053D390 */
void PrepareCharacterAttacks(int party_slot);                                    /* 0x0053D680 */
int GetTargetArmorClassModifier(W8CombatSlot* target, unsigned int attack_mode); /* 0x005468D0 */
unsigned int CharChooseHandAttackMode(W8Character* character, int hand);         /* 0x00542CA0 */
wchar_t* SpellTargetString(W8TargetSource* source, W8CombatSlot* target);        /* 0x00546B40 */
int GetTargetArmorClass(W8CombatSlot* target, int attack_mode);                  /* 0x00542EE0 */
unsigned char BlockedForSpecialReason(int weapon_class, W8CombatSlot* target, int attack_value,
                                      int armor_value, unsigned int palette);  /* 0x00543110 */
unsigned int CapAttackDamageByTargetHealth(unsigned int damage);               /* 0x00545A00 */
void StartMonsterAttackCycle(W8MonsterInfo* monster_info, int action_detail);  /* 0x0053FFE0 */
void ReportCharacterAttackResult(int party_slot, W8SpellEffectResult* report); /* 0x0053FB00 */
void ReportMonsterAttackResult(W8MonsterInfo* monster_info,
                               W8SpellEffectResult* report); /* 0x005412B0 */
/* 0x00542720: the monster side of the attack-score pipeline - the attack's
   own score plus modifier, mode, surprise, armour and attribute terms; arg_4
   selects the fumble-redirect variant. */
int GetMonsterAttackScore(W8MonsterInfo* monster_info, W8MonsterAttack* attack, int attack_mode,
                          char arg_4);
/* 0x00542960: roll the running monster attack's damage dice and reduce them
   by the target's damage reduction, reporting the dice count. */
int ResolveMonsterAttackDamage(W8MonsterInfo* monster_info, W8MonsterAttack* attack,
                               unsigned int* out_dice_count);
struct W8PList;
/* 0x00544010: build the candidate list the monster's fumbled or repicked
   attack chooses a new target from. */
void BuildMonsterTargetList(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                            unsigned int attack, W8PList* out_list);
/* 0x00544250: announce a friendly-fire strike and highlight the name on the
   other side's notice palette when the two sides differ. */
void AnnounceAccidentalStrike(W8TargetSource* source, W8CombatSlot* target);
/* Whether a character catches the incoming attack in time to turn toward it. */
char CharacterNoticesAttacker(int party_slot); /* 0x0053D590 */
/* Begin one of the monster's attacks for the round. */
char StartMonsterAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record); /* 0x0053FEA0 */
/* 0x00545B20: the monster-side counterpart - the target still in play and in
   range, the monster able to act, and the attack itself able to come off. */
bool CanMonsterAttackItsTarget(W8MonsterInfo* monster_info);
/* 0x00545AA0: the character counterpart of the same six checks. */
bool CanCharacterAttackItsTarget(int party_slot);
/* Build the monster attack announcement message and aim it at the target. */
void AnnounceMonsterAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                           char arg_3); /* 0x00541630 */
/* 0x00540170: resolve one queued swing of the monster's attack - rolls the
   hit and fumble-redirect chances, resolves guardian interception, picks the
   hit location, rolls penetration, applies damage and the struck target's
   retaliation enchantment, then decides whether the monster keeps swinging. */
int ContinueMonsterAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record);
/* Start one of the character's attacks for the round: validates the hand and
   target, rolls the swings, announces the attack and dispatches the missile or
   melee event. */
char StartCharacterAttack(int party_slot, int attack_mode); /* 0x0053D870 */
/* 0x0053E250: resolve one of the character's queued swings - plays the
   attack sound on the first pass, rolls fumble redirection and guardian
   interception, picks the hit location, rolls penetration, applies damage
   and enchantments, consumes the item, and answers whether the attack
   continues. */
int ResolveCharacterAttack(int party_slot);
/* 0x00544530: a fumbled swing queues its reaction event - half the time the
   attacker himself, otherwise the fumbled victim or a random party member
   answers. */
void QueueFumbleReaction(int party_slot);
/* 0x00545E50: a defender guarding the attack's target may interpose and
   become the struck target instead; answers whether the target changed. */
int ResolveGuardianInterception(W8TargetSource* source, W8CombatSlot* target);
void FireCharacterItemMissile(int party_slot, W8Character* pc, W8CombatCharacterRow* row,
                              unsigned int range_category); /* 0x00544B60 */
