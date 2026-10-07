#pragma once

enum W8PartySelectionMode {
    W8_PARTY_SELECT_CHARACTERS = 0,
    W8_PARTY_SELECT_IMPORT = 1,
    W8_PARTY_SELECT_OPTIONS = 2,
    W8_PARTY_SELECT_CREATION_NOTICE = 3,
    W8_PARTY_SELECT_SAVE_NAME = 4
};

enum W8PartyCreationPage {
    W8_PARTY_CREATION_OPTIONS = 0,
    W8_PARTY_CREATION_NOTICE = 1,
    W8_PARTY_CREATION_SAVE_NAME = 2
};

enum W8PartyConfirmationAction {
    W8_PARTY_CONFIRM_NONE = 0,
    W8_PARTY_CONFIRM_DELETE_CHARACTER = 1,
    W8_PARTY_CONFIRM_START_WITH_SAVE_NAME = 2,
    W8_PARTY_CONFIRM_PROCEED_TO_OPTIONS = 3,
    W8_PARTY_CONFIRM_IMPORT = 4,
    W8_PARTY_CONFIRM_LEAVE = 5
};

/* Region-set slots the party-selection party builder shares between its panels.
   A panel acquires a slot by address; an unused slot is zero, so the caller
   allocates from the static region catalog on first use. */
extern unsigned int g_party_selection_character_region_set;
extern unsigned int g_party_selection_party_slot_region_set;
extern unsigned int g_party_selection_character_grid_region_set;
extern unsigned int g_party_selection_option_region_set;
extern unsigned int g_party_selection_range_region_set;
extern unsigned int g_party_selection_left_action_region_set;
extern unsigned int g_party_selection_bottom_action_region_set;
extern unsigned int g_party_selection_import_list_region_set;

/* One byte per portrait, nonzero for the portraits that carry
   animation frames. */
extern unsigned char g_portrait_frame_flags[0x50];

/* Refresh one party-selection list portrait after a slot change. */
void RefreshPartySelectionPortrait(unsigned int party_slot);
/* Nonzero while the selector is reviewing an existing character. */
bool PartySelectionInReviewMode(void);
unsigned char PartySelectionScreenEnter(void);
void PartySelectionScreenFrame(void);
unsigned char PartySelectionScreenLeave(int leaving);
void GameStartRouterFrame(void);
// bool-byte-ok: W8ScreenStateHandlers leave slot
unsigned char GameStartRouterLeave(int leaving);
