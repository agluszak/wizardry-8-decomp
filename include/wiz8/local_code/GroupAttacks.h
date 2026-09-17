#pragma once

/* Local Code\GroupAttacks.cpp. The pair resolves a monster group's special
   attack: one kind-table driven pass over the character targets and monster
   targets, or - for summoning kinds - a fresh group near the attacker. */

struct W8TargetSource;
struct W8CombatSlot;
template <class T> class srVector3T;

/* Resolve one special-attack entry against every character and monster target.
   Its two effect slots each roll against a per-target miss chance and dispatch
   on the effect style; the hit/damage tallies are summarized to the text box
   unless combat announcements are verbose. */
void ResolveMonsterGroupAttack005560A0(int special_attack_kind, W8TargetSource* pSource,
                                       W8CombatSlot* pAttackerSlot, int arg_4, int iNumCharTargets,
                                       int arg_6, int* piCharTargets, int arg_8,
                                       int iNumMonsterTargets, int arg_10,
                                       int* piMonsterTargets); /* 0x005560A0 */
/* Whether the special-attack kind is one of the seven the casting-blocked
   condition keeps from fleeing. */
unsigned char MonsterSpecialAttackHonorsCastingBlock(int special_attack_kind); /* 0x00556050 */

/* Translate a summoning special-attack kind to its monster species, roll that
   record's group size and bring the new group in as close to the attacker's
   position as camera-facing placement allows. */
void SpawnSummonedMonsterGroup00556B10(int special_attack_kind, W8TargetSource* pSource,
                                       W8CombatSlot* pAttackerSlot); /* 0x00556B10 */
