#pragma once

/* Region-set slots the party-selection party builder shares between its panels.
   A panel acquires a slot by address; an unused slot is zero, so the caller
   allocates from the static region catalog on first use. */
extern unsigned int g_party_selection_character_region_set;      /* 0x0069C4F0 */
extern unsigned int g_party_selection_party_slot_region_set;     /* 0x0069C4F4 */
extern unsigned int g_party_selection_character_grid_region_set; /* 0x0069C4F8 */
extern unsigned int g_party_selection_option_region_set;         /* 0x0069C4FC */
extern unsigned int g_party_selection_range_region_set;          /* 0x0069C500 */
extern unsigned int g_party_selection_left_action_region_set;    /* 0x0069C504 */
extern unsigned int g_party_selection_bottom_action_region_set;  /* 0x0069C508 */
extern unsigned int g_party_selection_import_list_region_set;    /* 0x0069C50C */

/* 0x0061CBC0: one byte per portrait, nonzero for the portraits that carry
   animation frames. */
extern unsigned char g_portrait_frame_flags[0x50];

/* Refresh one party-selection list portrait after a slot change. */
void RefreshPartySelectionPortrait(unsigned int party_slot); /* 0x005C33C0 */
/* 0x005C3470: nonzero while the selector is reviewing an existing character. */
bool PartySelectionInReviewMode(void);
unsigned char PartySelectionScreenEnter(void);
void PartySelectionScreenFrame(void);
unsigned char PartySelectionScreenLeave(int leaving);
void GameStartRouterFrame(void);
// bool-byte-ok: W8ScreenStateHandlers leave slot
unsigned char GameStartRouterLeave(int leaving);
