#pragma once

/* Local Screens\MGSFormation.cpp: the party-formation screen - a board of
   fifteen cells in five rows of three drawn over the main-game view, with a
   rotating compass needle tracking the party's heading. */

struct W8Region;
struct W8RegionEvent;

/* Rebuild the board, compass and overlay sprites and redraw the slot markers
   into the board image. */
void RefreshFormationBoard(void); /* 0x005B1C80 */
/* Rotate the compass needle to party_facing - party_heading. */
void UpdateFormationCompass(void); /* 0x005B1E70 */
/* Lazily create the overlay sprite above the board. */
void CreateFormationBoardOverlay(void); /* 0x005B1EA0 */
/* Region callback on the board itself: hit-tests the cell markers for the
   tooltip and swaps in the highlighted board art while hovered. */
unsigned char FormationBoardRegionEvent(const W8RegionEvent* event, W8Region* region);
/* Open the formation panel: build the controls, snapshot the live formation
   into gXStatus.edited_formation and pause the world. */
void OpenFormationPanel(void); /* 0x005B2150 */
/* Region callback the fifteen formation cells share. */
unsigned char FormationCellRegionEvent(const W8RegionEvent* event, W8Region* region);
/* Highlight the cell holding one party slot and un-highlight the rest. */
void SelectFormationSlotCell(int party_slot); /* 0x005B3100 */
