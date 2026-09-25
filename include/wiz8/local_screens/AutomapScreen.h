#pragma once

extern float g_float_64b914;

#include "input.h"
#include "wiz8/geometry.h"

#include "surrender/srMath.h"
#include "wiz8/vector.h"

/* Map-note allocation at 0x00581360 is 0x10 bytes. The text is separately
   malloc-owned and contains at most 39 UTF-16 characters plus the terminator. */
struct W8AutomapNote {
    srVector2T<float> position;
    int layer;
    wchar_t* text;
};
static_assert(sizeof(W8AutomapNote) == 0x10, "W8AutomapNote_size");

extern W8GrowableVector<W8AutomapNote*>* g_automap_notes;
extern int g_ui_mode_current;
extern int g_ui_mode_saved;
void SetValue68F2B0(int value); /* 0x00587C10 */
void SetValue68F2C4(int value); /* 0x0058A870 */

class W8DialogButton;
extern W8DialogButton** g_automap_buttons;
extern int g_automap_zoom_mode;

extern bool g_mipe_menu_active;
extern bool g_mipe_active;

bool HasAutomapLayer(int layer);
void RestoreAutomapCameraPosition(void);
bool CanUseCurrentAutomapTool(void);

/* 0x00580380: recompute the automap's visible world bounds from lit cells. */
void UpdateAutomapBounds(void);
/* 0x00580760: when the automap dirty flag is set, light a batch of pending
   visited cells through the table-1 vertex lights, then clear the flag once
   the batch finds nothing left. */
void RefreshDirtyAutomap(void);
/* 0x00581B30: pack `position` into a cell key, mark it visited if known, and
   return 1 only when that mark was newly set (retry one cell higher on miss). */
bool AutomapHasCellAt(const srVector3T<float>* position);

void ResetAutomapView(void);
bool SaveAutomapNotes(int handle); /* 0x00581CE0 */
bool LoadAutomapNotes(int handle); /* 0x00581E60 */
unsigned char GetAutomapPositionUnderCursor(srVector3T<float>* position);
void SetAutomapToolCursor(int tool);
W8AutomapNote* FindAutomapNoteUnderCursor(void);
void CreateAutomapMarkerSprites(void);
void RenderAutomapMarkers(void);
void CreateAutomapButtons(void);

unsigned char ReadAutomapNodes(int hFile);
unsigned char AutomapScreenInitialize(void);
unsigned char AutomapScreenEnter(void);
void AutomapScreenFrame(void);
unsigned char AutomapScreenLeave(int leaving);
unsigned char AutomapScreenFinalize(void);
/* Full-screen dismiss: left-up after a held press leaves the automap. */
unsigned char AutomapBackgroundRegionEvent(const InputAtom* event,
                                           struct W8Region* region); /* 0x00581790 */
float GetFloat64B914(void);
void SetFloat64B914(float value); /* 0x00585300 */
/* Packs a world position into an automap cell key. */
unsigned int AutomapNodeKey(const srVector3T<float>* position);
bool AutomapLevelIsLarge(void);
