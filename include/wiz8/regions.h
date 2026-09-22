#ifndef WIZ8_REGIONS_H
#define WIZ8_REGIONS_H

#include <stddef.h>

#include "input.h"

struct Controls;

extern unsigned char g_dev_mode_689b32;

struct W8RegionSet {
    unsigned int enabled;
    unsigned int first_region;
    unsigned int last_region;
}; /* 0x0c */

/* Region callbacks receive SGP's input atom unchanged. Wheel rotation uses
   usParam; uiParam carries the packed cursor position. */
static_assert(sizeof(InputAtom) == 0x10, "InputAtom_size");
static_assert(offsetof(InputAtom, usParam) == 0x08, "InputAtom_wheel_payload_offset");
static_assert(offsetof(InputAtom, uiParam) == 0x0c, "InputAtom_mouse_position_offset");

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
typedef unsigned char (*W8RegionCallback)(const InputAtom* event, struct W8Region* region);

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
    Controls* owner;
}; /* 0x1c */

unsigned char InitializeRegionHelpState(void);

/* Unrecovered static-catalog callbacks are stored as absolute retail VAs in
   RegionManager.cpp so matching WIZ8 does not need runtime_stubs /
   /FORCE:UNRESOLVED (which would collapse them to image base). */
unsigned char TextBoxScrollUpRegionEvent(const InputAtom*, W8Region*);    /* 0x0058E2A0 */
unsigned char TextBoxScrollDownRegionEvent(const InputAtom*, W8Region*);  /* 0x0058E650 */
unsigned char TextBoxScrollThumbRegionEvent(const InputAtom*, W8Region*); /* 0x0058E9F0 */
unsigned char TextBoxBodyRegionEvent(const InputAtom*, W8Region*);        /* 0x0058ED90 */
unsigned char TextBoxChannelTabRegionEvent(const InputAtom*, W8Region*);  /* 0x0058EFD0 */
unsigned char TextBoxMuteRegionEvent(const InputAtom*, W8Region*);        /* 0x0058F240 */

extern unsigned int g_region_set_count; /* guiRegsetCount */
extern W8RegionSet g_region_sets[];
extern unsigned int g_region_count; /* guiRegionCount */
extern W8Region g_regions[];
extern int g_region_help_delay;
extern int g_region_help_clock;
extern unsigned int g_current_region_index;
extern unsigned int g_captured_region_index;
extern unsigned int g_hover_region_index;
extern unsigned char g_region_help_force_enabled;
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
void SetRegionOwner(unsigned int region_index, Controls* owner);
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

void SetRegionHelpForceEnabled004F27C0(unsigned char enabled);
void EnableRegionHelpFlag004F27D0(W8Region* region);
void DisableRegionHelpFlag004F27E0(W8Region* region);

unsigned int DispatchMainGameMouseButtons(const InputAtom* input);

void ResetRegions(void);

#endif
