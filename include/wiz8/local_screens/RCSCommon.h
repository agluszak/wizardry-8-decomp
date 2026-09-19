#pragma once

#include <stddef.h>

#include "wiz8/regions.h"

struct Controls;
class W8TextControl;

void DrawCampHeader005B4000(void);
void DrawCampVitals005B4790(void);
void DrawCampHands005B4BD0(void);
int CreateCampButtonPanel005B4EB0(void);
void DestroyCampButtonPanel005B55F0(void);
void RefreshCampItemActions005B5670(unsigned char invalidate);
void SetCampItemActionMode005B59B0(char mode);
void SelectCampCharacter005B6B30(int slot);
/* Right-click on a camp portrait while holding an item: refuse with a notice
   for dead/insane/stoned, else try AddItemToCharacter. */
void TryGiveHeldItemToCampPortrait005B6C10(int slot);

void RedrawRcsLevelUpPanel(void); /* 0x005B6590 */
void RedrawRcsDismissPanel(void); /* 0x005B68D0 */

extern Controls* g_level_up_panel_0069c3c4;
extern Controls* g_dismiss_panel_0069c3c8;
extern W8TextControl* g_level_up_button_0069c3c0;
extern W8TextControl* g_dismiss_button_0069c400;
/* Five bottom page buttons created with the item-action strip by
   CreateCampButtonPanel005B4EB0 (Items/Skills/...). */
extern W8TextControl* g_camp_page_buttons_0069c3ec[5];
extern W8TextControl* g_item_action_controls_69c3cc[8];
/* 0x0069C404: the bottom Controls panel CreateCampButtonPanel005B4EB0 parents
   the page and item-action strips to. */
extern Controls* g_item_actions_panel_0069c404;

unsigned char CampDismissPortraitRegionEvent(const InputAtom* event,
                                             W8Region* region); /* 0x005B5E90 */
unsigned char CampPortraitSlotRegionEvent(const InputAtom* event,
                                          W8Region* region); /* 0x005B5F10 */
unsigned char CampOpenCharacterScreenRegionEvent(const InputAtom* event,
                                                 W8Region* region);                /* 0x005B61A0 */
unsigned char CampNameEditRegionEvent(const InputAtom* event, W8Region* region);   /* 0x005B6220 */
unsigned char CampPageButtonRegionEvent(const InputAtom* event, W8Region* region); /* 0x005B62C0 */
unsigned char CampItemActionRegionEvent(const InputAtom* event, W8Region* region); /* 0x005B6360 */
unsigned char CampLevelUpButtonRegionEvent(const InputAtom* event,
                                           W8Region* region); /* 0x005B66B0 */
unsigned char CampDismissButtonRegionEvent(const InputAtom* event,
                                           W8Region* region); /* 0x005B6AA0 */

void CreateRcsLevelUpPanel(void);
void DestroyRcsLevelUpPanel(void);
void UpdateRcsLevelUpPanel(void);
void CreateRcsDismissPanel(void);
void DestroyRcsDismissPanel(void);
void UpdateRcsDismissPanel(void);
void DrawRcsText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode);
void DrawRcsBoldText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode);
void DrawTallRcsText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode);
/* 0x005B6FD0: like DrawRcsText but the box height is caller-provided and the
   text is rendered through mprintf with the current font. */
void DrawRcsTextJustified(const wchar_t* text, int left, int top, int width, int height,
                          unsigned int layout_mode);
