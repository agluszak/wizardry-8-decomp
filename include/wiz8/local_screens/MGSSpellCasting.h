#pragma once

#include "input.h"
#include "wiz8/layouts/main_game_screen.h"

unsigned char IgnoreSpellCastingInput(const InputAtom* input);               /* 0x005A1140 */
unsigned char OpenSpellCastingView(int party_slot);                          /* 0x0059F0E0 */
void CloseSpellCastingView(void);                                            /* 0x0059F2B0 */
void RestoreSpellCastingRegions(void);                                       /* 0x0059F440 */
void SelectSpellCastingCharacter(int party_slot);                            /* 0x0059F490 */
void BeginSpellCast005A0110(int spell_id, int location_id, int interact_id); /* 0x005A0110 */
void SetSpellCastingPanelsActive005A0270(unsigned char active);              /* 0x005A0270 */
void InvalidateSpellCastingDescription005A0300(void);                        /* 0x005A0300 */
void SelectSpellPowerLevel005A06F0(int power_level);                         /* 0x005A06F0 */
void ResetSpellCastingSelection005A0B90(void);                               /* 0x005A0B90 */
void CommitSpellCastingSelection005A0BC0(void);                              /* 0x005A0BC0 */
void SetSpellCastingMode005A1330(W8MainUiMode value);                        /* 0x005A1330 */
int GetSpellCastingSelection005A1350(void);                                  /* 0x005A1350 */

struct W8Region;
class W8DialogBase;
/* 0x005A02F0: destroy callback for the spell/use-item reason notices. */
void SpellCastingNoticeClosed005A02F0(W8DialogBase* dialog);
unsigned char SpellRealmButtonRegionEvent(const InputAtom* event,
                                          W8Region* region);                      /* 0x005A0C80 */
unsigned char SpellPowerPipRegionEvent(const InputAtom* event, W8Region* region); /* 0x005A0E50 */

/* 0x0069BF3C: live spell-casting view, or null when the panel is closed. */
struct W8SpellCastingView;
extern W8SpellCastingView* gpSCSV;
/* gpSCSV->iSpellPower, or -1 when the view is closed. */
int GetSpellCastingPowerIndex(void);
