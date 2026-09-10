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
