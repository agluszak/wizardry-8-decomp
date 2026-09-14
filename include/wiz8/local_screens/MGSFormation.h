#pragma once

#include "input.h"

/* Local Screens\MGSFormation.cpp: the party-formation screen - a board of
   fifteen cells in five rows of three drawn over the main-game view, with a
   rotating compass needle tracking the party's heading. */

struct W8Region;
struct Controls;
class W8TextControl;

extern Controls* g_formation_panel;
extern W8TextControl* g_formation_cell_controls[15];
extern W8TextControl* g_formation_cell_overlays[15];
extern W8TextControl* g_formation_action_buttons[3];

void ReleaseFormationBoard(void); /* 0x005B1C00 */
void DestroyFormationPanel(void); /* 0x005B2580 */
void CloseFormationPanel(void);   /* 0x005B2200 */

/* Rebuild the board, compass and overlay sprites and redraw the slot markers
   into the board image. */
void RefreshFormationBoard(void); /* 0x005B1C80 */
/* Rotate the compass needle to party_facing - party_heading. */
void UpdateFormationCompass(void); /* 0x005B1E70 */
/* Lazily create the overlay sprite above the board. */
void CreateFormationBoardOverlay(void); /* 0x005B1EA0 */
/* Board callback: opens the formation panel on release, hit-tests the cell
   markers for tooltips, and switches board art on hover transitions. */
unsigned char FormationBoardRegionEvent(const InputAtom* event, W8Region* region);
/* Open the formation panel: build the controls, snapshot the live formation
   into gXStatus.edited_formation and pause the world. */
void OpenFormationPanel(void); /* 0x005B2150 */
/* Region callback the fifteen formation cells share. */
unsigned char FormationCellRegionEvent(const InputAtom* event, W8Region* region);
unsigned char FormationActionRegionEvent(const InputAtom* event, W8Region* region);
unsigned char FormationBackgroundRegionEvent(const InputAtom* event, W8Region* region);
/* Highlight the cell holding one party slot and un-highlight the rest. */
void SelectFormationSlotCell(int party_slot); /* 0x005B3100 */
