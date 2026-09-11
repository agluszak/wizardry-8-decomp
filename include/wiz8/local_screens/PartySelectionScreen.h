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

/* 0x0068372D: one animated-portrait record per party slot. Two frame tracks
   each keep the frame last drawn and the frame to draw next; the two bytes
   force a track to redraw. */
struct W8PortraitAnimationState {
    int previous_a_00;                    /* 0x00 */
    int current_a_04;                     /* 0x04 */
    unsigned char unknown_08[8];
    int previous_b_10;                    /* 0x10 */
    int current_b_14;                     /* 0x14 */
    unsigned char unknown_18[0x0d];
    unsigned char dirty_b_25;             /* 0x25 */
    unsigned char dirty_a_26;             /* 0x26 */
    unsigned char unknown_27[0xf1];
};
static_assert(sizeof(W8PortraitAnimationState) == 0x118,
              "W8PortraitAnimationState_must_be_0x118");
extern W8PortraitAnimationState g_portrait_animation_states_68372d[8];

/* 0x0052EB00: draw one party member's portrait at a screen position, with
   the animated frame pass and the state overlay. */
void RenderPartyPortrait0052EB00(
    int portrait, int left, int top, int flags, int value, int party_slot);
/* 0x0052EBE0: blit one animated portrait frame and its transition, returning
   whether a frame was drawn. */
char Function52EBE0(
    int portrait, int left, int top, int flags, int party_slot, char animate);
