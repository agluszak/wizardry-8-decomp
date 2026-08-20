#ifndef WIZ8_REGIONS_H
#define WIZ8_REGIONS_H

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

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int g_region_set_count;  /* guiRegsetCount */
extern W8RegionSet g_region_sets[];
extern unsigned int g_region_count;      /* guiRegionCount */
extern W8Region g_regions[];

unsigned int GetForcedRegion(void);
void UpdateRegionHelp(void);
void ShowRegionHelp(unsigned int region_index);
void SetRegionHelpText(const wchar_t* text);
void ResetRegionHelp(unsigned char delayed);
void EnableRegionHelp(unsigned int region_index);
void DisableRegionHelp(unsigned int region_index);
unsigned char ClearActiveRegionIfMatches(unsigned int region_index);
void ActivateDialogRegion(unsigned int region_index); /* 0x004F2040 */

#ifdef __cplusplus
}
#endif

#endif
