#ifndef WIZ8_REGIONS_H
#define WIZ8_REGIONS_H

#include <stddef.h>

#include "input.h"

extern unsigned char g_flag_689b32;

struct W8RegionSet {
    unsigned int enabled;
    unsigned int first_region;
    unsigned int last_region;
}; /* 0x0c */

struct W8RegionEvent {
    unsigned int time;
    unsigned short modifiers;
    unsigned short reason;
};

struct W8RegionMouseEvent {
    W8RegionEvent event;
    unsigned int mouse_position;
};

enum W8RegionFlags {
    W8_REGION_RECTANGLE = 0x01,
    W8_REGION_CIRCLE = 0x02,
    W8_REGION_INPUT_DISABLED = 0x04,
    W8_REGION_INPUT_MODE_MASK = 0x0c,
    W8_REGION_MOUSE_ENTER = 0x10,
    W8_REGION_MOUSE_LEAVE = 0x20,
    W8_REGION_LEFT_BUTTON_HELD = 0x40,
    W8_REGION_RIGHT_BUTTON_HELD = 0x80,
    W8_REGION_MOUSE_TRANSITION_MASK = 0x30,
    W8_REGION_MOUSE_STATE_MASK = 0xf0,
    W8_REGION_HELP_SHOWN = 0x200
};

struct W8Region;
typedef unsigned char (*W8RegionCallback)(const W8RegionEvent* event, struct W8Region* region);

struct W8Region {
    unsigned int flags;
    short x1;
    short y1;
    short x2;
    short y2;
    W8RegionCallback callback;
    unsigned short callback_id;
    unsigned char help_enabled;
    unsigned char unknown_13;
    int help_text_id;
    void* owner;
}; /* 0x1c */

void InitializeRegionHelpState(void);

/* Callbacks of the static region catalog whose bodies are not yet decoded.
   The region table references these addresses directly, so they keep one
   canonical, address-qualified declaration here until their owning units are
   recovered; none of them is called by recovered code. */
unsigned char IntroScreenRegionEvent(const W8RegionEvent*, W8Region*);
unsigned char Function0052FD80(const W8RegionEvent*, W8Region*);
unsigned char Function0055E690(const W8RegionEvent*, W8Region*);
unsigned char Function00565990(const W8RegionEvent*, W8Region*);
unsigned char Function005667A0(const W8RegionEvent*, W8Region*);
unsigned char Function00566AE0(const W8RegionEvent*, W8Region*);
unsigned char Function00566E20(const W8RegionEvent*, W8Region*);
unsigned char Function005670C0(const W8RegionEvent*, W8Region*);
unsigned char Function005673B0(const W8RegionEvent*, W8Region*);
unsigned char Function00567600(const W8RegionEvent*, W8Region*);
unsigned char Function00567800(const W8RegionEvent*, W8Region*);
unsigned char Function00568100(const W8RegionEvent*, W8Region*);
unsigned char Function005699D0(const W8RegionEvent*, W8Region*);
unsigned char Function0056F020(const W8RegionEvent*, W8Region*);
unsigned char Function00576650(const W8RegionEvent*, W8Region*);
unsigned char Function00581790(const W8RegionEvent*, W8Region*);
unsigned char Function0058E2A0(const W8RegionEvent*, W8Region*);
unsigned char Function0058E650(const W8RegionEvent*, W8Region*);
unsigned char Function0058E9F0(const W8RegionEvent*, W8Region*);
unsigned char Function0058ED90(const W8RegionEvent*, W8Region*);
unsigned char Function0058EFD0(const W8RegionEvent*, W8Region*);
unsigned char Function0058F240(const W8RegionEvent*, W8Region*);
unsigned char Function00594760(const W8RegionEvent*, W8Region*);
unsigned char Function005949A0(const W8RegionEvent*, W8Region*);
unsigned char Function00598CD0(const W8RegionEvent*, W8Region*);
unsigned char Function00598DB0(const W8RegionEvent*, W8Region*);
unsigned char Function0059BD20(const W8RegionEvent*, W8Region*);
unsigned char Function0059C260(const W8RegionEvent*, W8Region*);
unsigned char Function0059D970(const W8RegionEvent*, W8Region*);
unsigned char Function0059DA30(const W8RegionEvent*, W8Region*);
unsigned char Function005A0C80(const W8RegionEvent*, W8Region*);
unsigned char Function005A0E50(const W8RegionEvent*, W8Region*);
unsigned char Function005A1DE0(const W8RegionEvent*, W8Region*);
unsigned char Function005AEEA0(const W8RegionEvent*, W8Region*);
unsigned char Function005AF530(const W8RegionEvent*, W8Region*);
unsigned char Function005AF5E0(const W8RegionEvent*, W8Region*);
unsigned char Function005B2020(const W8RegionEvent*, W8Region*);
unsigned char Function005B29D0(const W8RegionEvent*, W8Region*);
unsigned char Function005B2CB0(const W8RegionEvent*, W8Region*);
unsigned char Function005B2D70(const W8RegionEvent*, W8Region*);
unsigned char Function005B5E90(const W8RegionEvent*, W8Region*);
unsigned char Function005B5F10(const W8RegionEvent*, W8Region*);
unsigned char Function005B61A0(const W8RegionEvent*, W8Region*);
unsigned char Function005B6220(const W8RegionEvent*, W8Region*);
unsigned char Function005B62C0(const W8RegionEvent*, W8Region*);
unsigned char Function005B6360(const W8RegionEvent*, W8Region*);
unsigned char Function005B66B0(const W8RegionEvent*, W8Region*);
unsigned char Function005B6AA0(const W8RegionEvent*, W8Region*);
unsigned char Function005B79F0(const W8RegionEvent*, W8Region*);
unsigned char Function005BB350(const W8RegionEvent*, W8Region*);
unsigned char Function005BB560(const W8RegionEvent*, W8Region*);
unsigned char Function005BB900(const W8RegionEvent*, W8Region*);
unsigned char Function005BBBB0(const W8RegionEvent*, W8Region*);
unsigned char Function005BBC70(const W8RegionEvent*, W8Region*);
unsigned char Function005BC7A0(const W8RegionEvent*, W8Region*);

extern unsigned int g_region_set_count; /* guiRegsetCount */
extern W8RegionSet g_region_sets[];
extern unsigned int g_region_count; /* guiRegionCount */
extern W8Region g_regions[];
extern int g_region_help_delay;
extern int g_region_help_clock;
extern unsigned int g_current_region_index;
extern unsigned int g_captured_region_index;
extern unsigned int g_hover_region_index;
extern unsigned int g_region_help_force_enabled;
extern wchar_t* g_default_help_text;

unsigned int GetForcedRegion(void);
void ReleasePointer689B40(void);
unsigned int UpdateRegionMousePosition(int x, int y);
unsigned int FindRegionAtPoint(unsigned short x, unsigned short y);
void RegionSetEnable(unsigned int region_set_index);
void RegionSetDisable(unsigned int region_set_index);
void EnableRegionSetInput(unsigned int region_set_index);
void DisableRegionSetInput(unsigned int region_set_index);
void EnableRegionInput(unsigned int region_index);
void DisableRegionInput(unsigned int region_index);
void SetRegionBounds(unsigned int region_index, unsigned short x1, unsigned short y1,
                     unsigned short x2, unsigned short y2);
bool RegionContainsPoint(unsigned int region_index, unsigned short x, unsigned short y);
bool RegionHasFlags(unsigned int region_index, unsigned int flags);
unsigned int CreateRegionSet(void);
void ResetRegionSet(unsigned int region_set_index);
unsigned int AddRegionToSet(unsigned int region_set_index);
void SetRegionCallback(unsigned int region_index, W8RegionCallback callback,
                       unsigned short callback_id);
void SetRegionOwner(unsigned int region_index, void* owner);
void SetRegionHelp(unsigned int region_index, unsigned char enabled, int help_text_id);
void ClearHotRegion004F2A80(void);
void UpdateRegionHelp(void);
void ShowRegionHelp(unsigned int region_index);
void SetRegionHelpText(const wchar_t* text);
void ResetRegionHelp(unsigned char delayed);
void SetRegionHelpDelay(int delay_ms);
void EnableRegionHelp(unsigned int region_index);
void DisableRegionHelp(unsigned int region_index);
unsigned char ClearActiveRegionIfMatches(unsigned int region_index);
void ActivateDialogRegion(unsigned int region_index); /* 0x004F2040 */
/* 0x004F1910 returns the byte produced by the selected region callback. */
unsigned char DispatchRegionInput(const InputAtom* event);

/* Unresolved region-manager gap helpers used by RCSItemsPage.cpp: the first
   records that region help is armed, and the other two set and clear a
   W8Region's help-enabled byte at +0x12. */
void Function4F27C0(char armed);
void Function4F27D0(W8Region* region);
void Function4F27E0(W8Region* region);

unsigned int DispatchMainGameMouseButtons(const InputAtom* input);

void ResetRegions(void);

#endif
