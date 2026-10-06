#pragma once

#include <stddef.h>

#include "wiz8/regions.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/video_object_catalog.h"
#include "vobject_blitters.h"

struct Controls;
class W8TextControl;

/* Button order differs from W8CampItemAction's byte values. */
enum W8CampItemActionButton {
    W8_CAMP_ACTION_BUTTON_NONE = -1,
    W8_CAMP_ACTION_BUTTON_IDENTIFY = 0,
    W8_CAMP_ACTION_BUTTON_MOVE = 1,
    W8_CAMP_ACTION_BUTTON_SPLIT_STACK = 2,
    W8_CAMP_ACTION_BUTTON_USE = 3,
    W8_CAMP_ACTION_BUTTON_DROP = 4,
    W8_CAMP_ACTION_BUTTON_CAST_SPELL = 5,
    W8_CAMP_ACTION_BUTTON_USE_ON_ITEM = 6,
    W8_CAMP_ACTION_BUTTON_USE_ON_CHARACTER = 7,
    W8_CAMP_ACTION_BUTTON_COUNT = 8
};

/* Attribute and resistance bars use the same base/bonus/penalty strips and
   invalidate every drawn segment. Skill bars have a different invalidation policy. */
inline void DrawCampValueBar(unsigned int value, unsigned int base, int left, int top)
{
    unsigned int filled;
    unsigned int extra;
    unsigned int missing;
    if (value < base) {
        filled = value;
        extra = 0;
        missing = base - value;
    } else {
        filled = base;
        extra = value - base;
        missing = 0;
    }
    SGPRect saved_clip;
    GetClippingRect(&saved_clip);
    SGPRect clip;
    clip.iTop = 0;
    clip.iBottom = 0x1e0;
    if (filled != 0) {
        clip.iLeft = left;
        clip.iRight = left + filled;
        SetClippingRect(&clip);
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x143, 0, 0, left, top, VO_BLT_SRCTRANSPARENCY,
                                      0);
    }
    if (extra != 0) {
        clip.iLeft = left + filled;
        clip.iRight = left + filled + extra;
        SetClippingRect(&clip);
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x143, 0, 1, left, top, VO_BLT_SRCTRANSPARENCY,
                                      0);
    } else if (missing != 0) {
        clip.iLeft = left + filled;
        clip.iRight = left + filled + missing;
        SetClippingRect(&clip);
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x143, 0, 2, left, top, VO_BLT_SRCTRANSPARENCY,
                                      0);
    }
    SetClippingRect(&saved_clip);
}

void DrawCampHeader(void);
void DrawCampVitals(void);
void DrawCampHands(void);
int CreateCampButtonPanel(void);
void DestroyCampButtonPanel(void);
void RefreshCampItemActions(bool invalidate);
void SetCampItemActionMode(char mode);
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
extern W8TextControl* g_item_action_controls[W8_CAMP_ACTION_BUTTON_COUNT];
/* 0x0069C404: the bottom Controls panel CreateCampButtonPanel parents
   the page and item-action strips to. */
extern Controls* g_item_actions_panel;

unsigned char CampDismissPortraitRegionEvent(const InputAtom* event,
                                             W8Region* region); /* 0x005B5E90 */
unsigned char CampPortraitSlotRegionEvent(const InputAtom* event,
                                          W8Region* region); /* 0x005B5F10 */
unsigned char CampOpenCharacterScreenRegionEvent(const InputAtom* event,
                                                 W8Region* region); /* 0x005B61A0 */
unsigned char CampProfessionHistoryRegionEvent(const InputAtom* event,
                                               W8Region* region);                  /* 0x005B6220 */
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
