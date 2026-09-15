#pragma once

struct Controls;
class W8TextBuffer;
class W8TextControl;

extern W8TextControl* g_panel_69bf40;
extern W8TextControl* g_panel_69bf44;
extern unsigned int g_movement_timer_69bf48;
extern Controls* g_panel_69bf4c;
extern W8TextBuffer* g_text_buffer_69bf50;
extern unsigned int g_movement_frame_69bf54;

/* Creates the combat party-movement panel and its two buttons; returns zero
   when an allocation fails. */
unsigned char CreatePartyMovementPanel005A1640(void); /* 0x005A1640 */
/* Releases the party-movement panels at 0x0069BF40/0x0069BF4C and clears the
   combat-UI teardown flag; called when the party regains movement. */
void ReleasePartyMovement(void);             /* 0x005A1890 */
void UpdatePartyMovementPanel005A1950(void); /* 0x005A1950 */
void DrawPartyMovementPanel005A19B0(void);   /* 0x005A19B0 */
void DisableRegionSet1C(void);               /* 0x005A19A0 */
/* Per-frame movement/fatigue processing; the callers reuse unrelated storage
   for the two elapsed-time outputs. */
unsigned char HandlePartyMovement005A1EB0(float* real_elapsed,
                                          float* frame_elapsed); /* 0x005A1EB0 */

void RedrawPanel69BF4C(void);          /* 0x005A1DD0 */
void DisablePanel69BF40005A1E90(void); /* 0x005A1E90 */
void EnablePanel69BF40005A1EA0(void);  /* 0x005A1EA0 */
