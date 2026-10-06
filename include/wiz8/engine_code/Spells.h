#pragma once

#include "wiz8/monster_spell_icons.h"
#include "surrender/srMath.h"
#include "wiz8/engine_code/SpellVisual.h"
#include "wiz8/layouts/gameplay_databases.h"

class W8Monster;
struct W8Item;
struct W8GrCycleLoadContext;
struct W8World;

/* Walks every registered stSound3D each audio update: releases finished
   auto-release nodes and re-aims/re-volumes playing voices relative to the
   camera. */
void Update3DSounds(); /* 0x004AEFD0 */
W8SpellVisual* CreateMonsterSpellEffect(const char* mls_name, int power_level, W8Monster* monster,
                                        int value, int flags); /* 0x004AD630 */
W8SpellVisual* CreateAttachedSpellEffect(const char* mls_name, int power_level, W8Monster* parent,
                                         int value, int flags); /* 0x004AD8A0 */
W8SpellVisual* CreateAimedSpellEffect(const char* mls_name, int power_level,
                                      srVector3T<float>* position, srMatrix3T<float>* rotation,
                                      int value, int flags); /* 0x004ADB20 */
void SetTargetConeEnabled(bool enabled);
W8SpellVisual* SpawnCameraSpellEffect(const char* name, int power_level, int value,
                                      int flags); /* 0x004AD080 */
/* Load one named visual from the spell bitmap directory; the out pointer is
   set only on success. */
bool LoadSpellVisualResource(const W8GrCycleLoadContext* context, const char* name,
                             W8SpellVisualMode group, W8SpellVisual** visual, int);
/* Create one spell visual from the spell's own record resource. Engine
   Code\Spells.cpp's factory, whose result the queued effect owns. */
W8SpellVisual* SpawnSpellEffect(const srVector3T<float>* position, const char* resource_name,
                                int power_level, int value, int flags); /* 0x004AD430 */

void ReleaseSpellDatabase(void);
unsigned char InitializeSpellDatabase(void);
W8SpellTargetType GetSpellTargetType(int spell_id, bool normalize_single_target);
bool IsCombatEffectSlotSpell(int spell_id);
int MinimumCasterLevelForSpellLevel(int spell_level);
int GetMinimumCasterLevelForSpell(int spell_id);
bool CanSpellBackfire(int spell_id);
void ClearMonsterSpellIcons(W8Monster* monster);                                   /* 0x004ACF90 */
void SetMonsterSpellIcon(W8Monster* monster, W8MonsterSpellIconId icon, bool add); /* 0x004ACD80 */
void UpdateWorldSpellVisuals(W8World* world);
