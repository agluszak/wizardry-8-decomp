#pragma once

#include "surrender/srMath.h"
#include "wiz8/engine_code/SpellVisual.h"
#include "wiz8/layouts/gameplay_databases.h"

class W8Monster;
struct W8Item;
struct W8GrCycleLoadContext;
struct W8World;

/* Spells.cpp's assertion names for the monster spell-icon domain: the icon
   index into g_monster_spell_icon_names, with SPELL_ICON_NONE marking no
   icon. The two literal indices cast code uses are named from the same
   table. */
enum {
    SPELL_ICON_NONE = -1,
    SPELL_ICON_CHARMED = 0x26,
    SPELL_ICON_SUMMONED = 0x27,
    SPELL_NUM_ICONS = 40
};

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
void SetTargetConeEnabled(char enabled);
W8SpellVisual* SpawnCameraSpellEffect(const char* name, int power_level, int value,
                                      int flags); /* 0x004AD080 */
/* Load one named visual from the spell bitmap directory; the out pointer is
   set only on success. */
bool LoadSpellVisualResource(const W8GrCycleLoadContext* context, const char* name,
                             W8SpellVisualMode group, W8SpellVisual** visual, int unused);
/* Create one spell visual from the spell's own record resource. Engine
   Code\Spells.cpp's factory, whose result the queued effect owns. */
W8SpellVisual* SpawnSpellEffect(const srVector3T<float>* position, const char* resource_name,
                                int power_level, int value, int flags); /* 0x004AD430 */

void ReleaseSpellDatabase(void);
unsigned char InitializeSpellDatabase(void);
W8SpellTargetType GetSpellTargetType(int spell_id, unsigned char normalize_single_target);
bool IsCombatEffectSlotSpell(int spell_id);
int MinimumCasterLevelForSpellLevel(int spell_level);
int GetMinimumCasterLevelForSpell(int spell_id);
bool CanSpellBackfire(int spell_id);
void ClearMonsterSpellIcons(W8Monster* monster);                  /* 0x004ACF90 */
void SetMonsterSpellIcon(W8Monster* monster, int icon, char add); /* 0x004ACD80 */
void UpdateWorldSpellVisuals004AAB80(W8World* world);
