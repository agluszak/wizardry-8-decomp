#ifndef WIZ8_REGIONS_H
#define WIZ8_REGIONS_H

#include <stddef.h>

#include "input.h"

typedef struct W8RegionSet {
    unsigned int enabled;
    unsigned int first_region;
    unsigned int last_region;
} W8RegionSet;                           /* 0x0c */

typedef struct W8RegionEvent {
    unsigned int time;
    unsigned short modifiers;
    unsigned short reason;
} W8RegionEvent;

typedef struct W8RegionMouseEvent {
    W8RegionEvent event;
    unsigned int mouse_position;
} W8RegionMouseEvent;

struct W8Region;
typedef void (*W8RegionCallback)(const W8RegionEvent* event, struct W8Region* region);

typedef struct W8Region {
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
} W8Region;                              /* 0x1c */

extern "C" {

extern unsigned int g_region_set_count;  /* guiRegsetCount */
extern W8RegionSet g_region_sets[];
extern unsigned int g_region_count;      /* guiRegionCount */
extern W8Region g_regions[];
extern int g_region_help_delay;
extern int g_region_help_clock;
extern unsigned int g_hot_region_689b3c;
extern unsigned int g_hot_region_689b44;
extern unsigned int g_hot_region_689b4c;
extern unsigned short g_dword_689b48;
extern unsigned int g_dword_689b50;
extern wchar_t* g_default_help_text;

unsigned int GetForcedRegion(void);
void RegionSetEnable(unsigned int region_set_index);
void RegionSetDisable(unsigned int region_set_index);
void ClearRegionSetModeBits(unsigned int region_set_index);
void SetRegionSetMode4(unsigned int region_set_index);
void ClearRegionModeBits(unsigned int region_index);
void SetRegionMode4(unsigned int region_index);
void SetRegionBounds(
    unsigned int region_index,
    unsigned short x1,
    unsigned short y1,
    unsigned short x2,
    unsigned short y2);
bool RegionContainsPoint(
    unsigned int region_index, unsigned short x, unsigned short y);
bool RegionHasFlags(unsigned int region_index, unsigned int flags);
unsigned int CreateRegionSet(void);
void ResetRegionSet(unsigned int region_set_index);
unsigned int AddRegionToSet(unsigned int region_set_index);
void SetRegionCallback(
    unsigned int region_index,
    W8RegionCallback callback,
    unsigned short callback_id);
void SetRegionOwner(unsigned int region_index, void* owner);
void SetRegionHelp(
    unsigned int region_index, unsigned char enabled, int help_text_id);
void UpdateRegionHelp(void);
void ShowRegionHelp(unsigned int region_index);
void SetRegionHelpText(const wchar_t* text);
void ResetRegionHelp(unsigned char delayed);
void EnableRegionHelp(unsigned int region_index);
void DisableRegionHelp(unsigned int region_index);
unsigned char ClearActiveRegionIfMatches(unsigned int region_index);
void ActivateDialogRegion(unsigned int region_index); /* 0x004F2040 */
/* 0x004F1910 returns the byte produced by the selected region callback. */
unsigned char DispatchScreenInput004F1910(const InputAtom* event);

}

#endif
