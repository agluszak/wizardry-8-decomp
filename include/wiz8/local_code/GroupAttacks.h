#pragma once

/* Local Code\GroupAttacks.cpp. The pair resolves a monster group's whole
   attack: one kind-table driven pass over the character targets and the
   monster targets, or - for kind six - a summon that calls a fresh group
   into existence near the attacker. */

struct W8TargetSource;
struct W8CombatSlot;
template <class T> class srVector3T;

/* Resolve one kind-table entry against every character and monster target.
   The kind's two attack slots each roll against a per-target miss chance and
   dispatch on the attack style; the hit/damage tallies are summarized to the
   text box unless combat announcements are verbose. */
void ResolveMonsterGroupAttack005560A0(int iAIKind, W8TargetSource* pSource,
                                       W8CombatSlot* pAttackerSlot, int arg_4, int iNumCharTargets,
                                       int arg_6, int* piCharTargets, int arg_8,
                                       int iNumMonsterTargets, int arg_10,
                                       int* piMonsterTargets); /* 0x005560A0 */

/* Kind six: look the kind's summoned species up, roll the record's group
   size and bring the new group in as close to the attacker's position as the
   camera-facing placement allows. */
void SpawnSummonedMonsterGroup00556B10(int iAIKind, W8TargetSource* pSource,
                                       W8CombatSlot* pAttackerSlot); /* 0x00556B10 */

/* 0x004BE5C0: the yaw that faces `position` back toward the camera; it is
   declared locally in MonsterManager.cpp and MonsterGroup.cpp today. */
float Function4BE5C0(srVector3T<float>* position);
