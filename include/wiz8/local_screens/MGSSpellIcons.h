#pragma once

#include "input.h"

struct Controls;
struct W8Region;

/* Parent strip for the top-row spell icons (0x19d,0)-(0x280,0x12). */
extern Controls* g_spell_icon_strip_69c2b0;
/* Combat-effect HUD panels flanking the center strip. */
extern Controls* g_combat_effect_left_panel_69c2bc;
extern Controls* g_combat_effect_right_panel_69c2b8;
/* Live icon counts for the three panels above. */
extern unsigned int g_spell_icon_count_69c25c;
extern unsigned int g_combat_effect_left_count_69c2b4;
extern unsigned int g_combat_effect_right_count_69c2ac;

/* Allocate the three HUD Controls panels MainGameScreenEnter needs before the
   spell/effect icon rows are filled in. */
unsigned char CreateSpellIconHudControls(void); /* 0x005AE9D0 */
/* Tear down the icon rows and destroy the three HUD panels. */
void DestroySpellIconHudControls(void); /* 0x005AEB20 */
/* Drop the combat-effect icon rows without destroying their parent panels. */
void DestroyCombatEffectHudRows(void); /* 0x005AF210 */
/* Clear and invalidate the main-game effect strip while it is up. */
void InvalidateMainGameEffectHud(void); /* 0x005AF2D0 */

/* Party / combat effect-icon region help + hover handlers (catalog rows for
   the three HUD strips CreateSpellIconHudControls allocates). */
void ShowPartyEffectIconHelp(int slot_index);       /* 0x005AED90 */
void ShowCombatLeftEffectIconHelp(int slot_index);  /* 0x005AF300 */
void ShowCombatRightEffectIconHelp(int slot_index); /* 0x005AF410 */
unsigned char PartyEffectIconRegionEvent(const InputAtom* event,
                                         struct W8Region* region); /* 0x005AEEA0 */
unsigned char CombatLeftEffectIconRegionEvent(const InputAtom* event,
                                              struct W8Region* region); /* 0x005AF530 */
unsigned char CombatRightEffectIconRegionEvent(const InputAtom* event,
                                               struct W8Region* region); /* 0x005AF5E0 */
