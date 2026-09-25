#pragma once

#include "input.h"

struct Controls;
struct W8Region;

/* Parent strip for the top-row spell icons (0x19d,0)-(0x280,0x12). */
extern Controls* g_spell_icon_strip;
/* Combat-effect HUD panels flanking the center strip. */
extern Controls* g_combat_effect_left_panel;
extern Controls* g_combat_effect_right_panel;
/* Live icon counts for the three panels above. */
extern unsigned int g_spell_icon_count;
extern unsigned int g_combat_effect_left_count;
extern unsigned int g_combat_effect_right_count;

/* Allocate the three HUD Controls panels MainGameScreenEnter needs before the
   spell/effect icon rows are filled in. */
unsigned char CreateSpellIconHudControls(void); /* 0x005AE9D0 */
/* Tear down the icon rows and destroy the three HUD panels. */
void DestroySpellIconHudControls(void); /* 0x005AEB20 */
/* Drop and rebuild the spell-icon rows on the existing top strip. */
void RefreshSpellIconHudRows(void); /* 0x005AEBE0 */
/* Rebuild the top-strip spell-icon text controls from active party effects. */
void RebuildSpellIconHudRows(void); /* 0x005AEC70 */
/* Drop the combat-effect icon rows without destroying their parent panels. */
void DestroyCombatEffectHudRows(void); /* 0x005AF210 */
/* Tear down combat-effect rows, rebuild them in combat, and enable/redraw the
   flanking panels (honoring portrait-refresh pending on slots 0/1). */
void RefreshCombatEffectHud(void); /* 0x005AEF30 */
/* Clear and invalidate the main-game effect strip while it is up. */
void InvalidateMainGameEffectHud(void); /* 0x005AF2D0 */

void ShowPartyEffectIconHelp(int slot_index);       /* 0x005AED90 */
void ShowCombatLeftEffectIconHelp(int slot_index);  /* 0x005AF300 */
void ShowCombatRightEffectIconHelp(int slot_index); /* 0x005AF410 */
unsigned char PartyEffectIconRegionEvent(const InputAtom* event,
                                         struct W8Region* region); /* 0x005AEEA0 */
unsigned char CombatLeftEffectIconRegionEvent(const InputAtom* event,
                                              struct W8Region* region); /* 0x005AF530 */
unsigned char CombatRightEffectIconRegionEvent(const InputAtom* event,
                                               struct W8Region* region); /* 0x005AF5E0 */
