#pragma once

/* Region-set slots the state-5 party builder shares between its panels.
   A panel acquires a slot by address; an unused slot is zero, so the caller
   allocates from the static region catalog on first use. */
extern unsigned int g_state5_character_region_set_69c4f0;     /* 0x0069C4F0 */
extern unsigned int g_state5_party_slot_region_set_69c4f4;    /* 0x0069C4F4 */
extern unsigned int g_state5_six_text_region_set_69c4f8;      /* 0x0069C4F8 */
extern unsigned int g_state5_option_region_set_69c4fc;        /* 0x0069C4FC */
extern unsigned int g_state5_range_region_set_69c500;         /* 0x0069C500 */
extern unsigned int g_state5_left_action_region_set_69c504;   /* 0x0069C504 */
extern unsigned int g_state5_bottom_action_region_set_69c508; /* 0x0069C508 */
extern unsigned int g_state5_import_list_region_set_69c50c;   /* 0x0069C50C */

/* 0x0061CBC0: one byte per portrait, nonzero for the portraits that carry
   animation frames. */
extern unsigned char g_portrait_frame_flags_0061cbc0[0x50];

void RenderPartyPortrait0052EB00(int portrait, int left, int top, int flags, int value,
                                 int party_slot);
/* 0x0052EBE0: blit one animated portrait frame and its transition, returning
   whether a frame was drawn. */
char Function52EBE0(int portrait, int left, int top, int flags, int party_slot, char animate);

/* Refresh one party-selection list portrait after a slot change. */
void RefreshPartySelectionPortrait(unsigned int party_slot); /* 0x005C33C0 */
