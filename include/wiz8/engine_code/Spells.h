#pragma once

#include "surrender/srMath.h"

class W8Monster;
class W8SpellVisual;
struct W8Item;
struct W8GrCycleLoadContext;
struct W8World;

void AudioUpdateFinish004AEFD0();
int CountSpellsOfKind(int kind); /* 0x004AC8F0 */
W8SpellVisual* CreateSpellEffect004AD8A0(const char* mls_name, int frame, W8Monster* parent,
                                         int value, int flags);
void SetTargetConeEnabled004ADD30(char enabled);
W8SpellVisual* SpawnSpellEffect004AD080(const char* name, int animation, int value_1, int value_2);
/* Load one named visual from the spell bitmap directory; the out pointer is
   set only on success. */
unsigned char LoadSpellVisualResource004AB580(const W8GrCycleLoadContext* context, const char* name,
                                              int cycle_type, W8SpellVisual** visual, int unused);
/* Create one spell visual from the spell's own record resource. Engine
   Code\Spells.cpp's factory, whose result the queued effect owns. */
W8SpellVisual* SpawnSpellEffect(const srVector3T<float>* position, const char* resource_name,
                                int argument_3, int argument_4, int argument_5); /* 0x004AD430 */

void ReleaseSpellDatabase(void);
unsigned char InitializeSpellDatabase(void);
int GetSpellTargetType(int spell_id, unsigned char normalize_single_target);
bool IsSpellInSingledOutSet(int spell_id);
int MinimumCasterLevelForSpellLevel(int spell_level);
int GetMinimumCasterLevelForSpell(int spell_id);
bool CanSpellBackfire(int spell_id);
void PrepareMonsterCycleForDestruction004ACF90(W8Monster* cycle);
void DropMonsterVisual(W8Monster* monster, int visual, char add);
void UpdateWorldSpellVisuals004AAB80(W8World* world);
