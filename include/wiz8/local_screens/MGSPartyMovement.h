#pragma once

#include "timer.h"
#include "input.h"

struct Controls;
struct W8Region;
class W8TextBuffer;
class W8TextControl;

extern W8TextControl* g_free_turn_button;
extern W8TextControl* g_cancel_party_movement_button;
extern TIMER g_party_movement_animation_clock;
extern Controls* g_party_movement_panel;
extern W8TextBuffer* g_party_movement_caption;
extern unsigned int g_party_movement_animation_frame;

/* Creates the combat party-movement panel and its two buttons; returns zero
   when an allocation fails. */
unsigned char CreatePartyMovementPanel(void);
/* Releases the party-movement panels and clears the
   combat-UI teardown flag; called when the party regains movement. */
void ReleasePartyMovement(void);
void UpdatePartyMovementPanel(void);
void DrawPartyMovementPanel(void);
void DisablePartyMovementRegions(void);
/* Per-frame movement/fatigue processing; the callers reuse unrelated storage
   for the two elapsed-time outputs. */
unsigned char HandlePartyMovement(float* real_elapsed, float* frame_elapsed);

void InvalidatePartyMovementPanel(void);
/* Free-turn / cancel-party-movement button region callback (ids 0 and 1). */
unsigned char FreeTurnButtonRegionEvent(const InputAtom* event, W8Region* region);
void DisableFreeTurnButton(void);
void EnableFreeTurnButton(void);
