#pragma once

#include "input.h"
#include "timer.h"
#include "wiz8/layouts/learned_spells.h"
#include "wiz8/layouts/main_game_screen.h"

class W8TextControl;
struct Controls;
struct W8Character;

/* The spell-casting view state gpSCSV, a 0xc5c-byte block malloc'd when the
   view opens. The member spellings come from this file's assertion strings;
   the rest are unresolved. */
struct W8SpellCastingView {
    unsigned char unknown_000[0xf8];
    W8Character* caster;             /* 0x0f8 */
    int iSpellRealm;                 /* 0x0fc: selected realm, -1 when none */
    int uiSpellToCast;               /* 0x100 */
    int override_spell_104;          /* 0x104: detail/commit override spell */
    int iSpellPower;                 /* 0x108: chosen power index, -1 when unset */
    int iSpellPowerClass;            /* 0x10c: the spell record's power class */
    unsigned int uiPowerLevels;      /* 0x110: affordable power-level count */
    TIMER realm_anim_timer;          /* 0x114 */
    unsigned int realm_anim_frame;   /* 0x118 */
    W8LearnedSpellState learned;     /* 0x11c */
    int uiSpellIndex;                /* 0x4f8: clicked list row */
    int selected_spell_index;        /* 0x4fc */
    int field_500;                   /* 0x500 */
    Controls* panels[3];             /* 0x504 */
    W8TextControl* realm_buttons[6]; /* 0x510 */
    W8TextControl* realm_icons[6];   /* 0x528 */
    W8TextControl* power_pips[9];    /* 0x540 */
    W8TextControl* spell_name;       /* 0x564 */
    W8TextControl* cancel_button;    /* 0x568 */
    W8MainUiMode saved_game_mode;    /* 0x56c */
    bool input_blocked_570;          /* 0x570 */
    unsigned char pad_571[3];
    int field_574;                  /* 0x574 */
    int interact_id;                /* 0x578 */
    int location_id;                /* 0x57c */
    unsigned int uiSpellsInList;    /* 0x580 */
    int uiSpells[0x15e];            /* 0x584 */
    signed char alt_colors[0x15e];  /* 0xafc */
    unsigned char closing;          /* 0xc5a: close already in progress */
    unsigned char dialog_confirmed; /* 0xc5b */
};

static_assert(sizeof(W8SpellCastingView) == 0xc5c, "W8SpellCastingView_must_be_0xc5c");

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
/* Text-box body callback while spell casting is active. */
unsigned char SpellCastTextBoxRegionEvent(const InputAtom* event,
                                          W8Region* region); /* 0x005A0F70 */

/* 0x0069BF3C: live spell-casting view, or null when the panel is closed. */
extern W8SpellCastingView* gpSCSV;
/* gpSCSV->iSpellPower, or -1 when the view is closed. */
int GetSpellCastingPowerIndex(void);
