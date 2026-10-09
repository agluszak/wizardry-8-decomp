#pragma once

#include "input.h"
#include "timer.h"
#include "wiz8/layouts/learned_spells.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/main_game_screen.h"
#include "wiz8/layouts/npc_dialogue.h"

class W8TextControl;
struct Controls;
struct W8Character;

enum {
    W8_SPELL_POWER_PIP_COUNT = 9,
    W8_SPELL_NAME_CONTROL = 9,
    W8_SPELL_CANCEL_CONTROL = 10,
    W8_SPELL_POWER_CONTROL_COUNT = 11
};

/* The spell-casting view state gpSCSV, malloc'd when the view opens. */
struct W8SpellCastingView {
    unsigned char unknown_000[0xf8];
    W8Character* caster;
    W8SpellRealm iSpellRealm; /* selected realm, -1 when none */
    int uiSpellToCast;
    int override_spell;                 /* detail/commit override spell */
    int iSpellPower;                    /* chosen power index, -1 when unset */
    W8SpellPowerClass iSpellPowerClass; /* the spell record's power class */
    unsigned int uiPowerLevels;         /* affordable power-level count */
    TIMER realm_anim_timer;
    unsigned int realm_anim_frame;
    W8LearnedSpellState learned;
    int uiSpellIndex; /* clicked list row */
    int selected_spell_index;
    int field_500;
    Controls* panels[3];
    W8TextControl* realm_buttons[6];
    W8TextControl* realm_icons[6];
    /* Indexed by region callback id. */
    W8TextControl* power_controls[W8_SPELL_POWER_CONTROL_COUNT];
    W8MainUiMode saved_game_mode;
    bool input_blocked;
    unsigned char pad_571[3];
    int field_574;
    W8NpcDialogueLayout interact_id;
    int location_id;
    unsigned int uiSpellsInList;
    int uiSpells[0x15e];
    signed char alt_colors[0x15e];
    bool closing; /* close already in progress */
    bool dialog_confirmed;
};

W8_ABI_ASSERT(sizeof(W8SpellCastingView) == 0xc5c, "W8SpellCastingView_must_be_0xc5c");

unsigned char OpenSpellCastingView(int party_slot);
void CloseSpellCastingView(void);
void RestoreSpellCastingRegions(void);
void SelectSpellCastingCharacter(int party_slot);
void BeginSpellCast(int spell_id, int location_id, W8NpcDialogueLayout interact_id);
void SetSpellCastingPanelsActive(bool active);
void InvalidateSpellCastingDescription(void);
void SelectSpellPowerLevel(int power_level);
void ResetSpellCastingSelection(void);
void CommitSpellCastingSelection(void);
void SetSpellCastingMode(W8MainUiMode value);
int GetSpellCastingSelection(void);

struct W8Region;
class W8DialogBase;
/* Destroy callback for the spell/use-item reason notices. */
void SpellCastingNoticeClosed(W8DialogBase* dialog);
unsigned char SpellRealmButtonRegionEvent(const InputAtom* event, W8Region* region);
unsigned char SpellPowerPipRegionEvent(const InputAtom* event, W8Region* region);
unsigned char IgnoreSpellCastingInput(const InputAtom* event, W8Region* region);
/* Text-box body callback while spell casting is active. */
unsigned char SpellCastTextBoxRegionEvent(const InputAtom* event, W8Region* region);

/* Live spell-casting view, or null when the panel is closed. */
extern W8SpellCastingView* gpSCSV;
/* gpSCSV->iSpellPower, or -1 when the view is closed. */
int GetSpellCastingPowerIndex(void);
