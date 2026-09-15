#pragma once

struct Controls;
class W8TextBuffer;
class W8TextControl;

extern W8TextControl* g_free_turn_button;
extern W8TextControl* g_cancel_party_movement_button;
extern unsigned int g_party_movement_animation_clock;
extern Controls* g_party_movement_panel;
extern W8TextBuffer* g_party_movement_caption;
extern unsigned int g_party_movement_animation_frame;

/* Creates the combat party-movement panel and its two buttons; returns zero
   when an allocation fails. */
unsigned char CreatePartyMovementPanel(void); /* 0x005A1640 */
/* Releases the party-movement panels at 0x0069BF40/0x0069BF4C and clears the
   combat-UI teardown flag; called when the party regains movement. */
void ReleasePartyMovement(void);        /* 0x005A1890 */
void UpdatePartyMovementPanel(void);    /* 0x005A1950 */
void DrawPartyMovementPanel(void);      /* 0x005A19B0 */
void DisablePartyMovementRegions(void); /* 0x005A19A0 */
/* Per-frame movement/fatigue processing; the callers reuse unrelated storage
   for the two elapsed-time outputs. */
unsigned char HandlePartyMovement(float* real_elapsed, float* frame_elapsed); /* 0x005A1EB0 */

void InvalidatePartyMovementPanel(void); /* 0x005A1DD0 */
void DisableFreeTurnButton(void);        /* 0x005A1E90 */
void EnableFreeTurnButton(void);         /* 0x005A1EA0 */
