#pragma once

class W8Monster;
class W8SpellVisual;
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
unsigned char LoadSpellVisualResource004AB580(const W8GrCycleLoadContext* context,
                                              const char* name, int cycle_type,
                                              W8SpellVisual** visual, int unused);
