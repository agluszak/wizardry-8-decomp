#pragma once

#include <stddef.h>

#include "wiz8/regions.h"

struct Controls;
class W8TextControl;

void DrawCampHeader(void);
void DrawCampVitals(void);
void DrawCampHands(void);
int CreateCampButtonPanel(void);
void DestroyCampButtonPanel(void);
void RefreshCampItemActions(unsigned char invalidate);
void SetCampItemActionMode005B59B0(char mode);
void SelectCampCharacter(int slot);
/* Right-click on a camp portrait while holding an item: refuse with a notice
   for dead/insane/stoned, else try AddItemToCharacter. */
void TryGiveHeldItemToCampPortrait(int slot);

void RedrawRcsLevelUpPanel(void); /* 0x005B6590 */
void RedrawRcsDismissPanel(void); /* 0x005B68D0 */

extern Controls* g_level_up_panel;
extern Controls* g_dismiss_panel;
extern W8TextControl* g_level_up_button;
extern W8TextControl* g_dismiss_button;
/* Five bottom page buttons created with the item-action strip by
   CreateCampButtonPanel (Items/Skills/...). */
extern W8TextControl* g_camp_page_buttons[5];
extern W8TextControl* g_item_action_controls[8];
/* 0x0069C404: the bottom Controls panel CreateCampButtonPanel parents
   the page and item-action strips to. */
extern Controls* g_item_actions_panel;

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
