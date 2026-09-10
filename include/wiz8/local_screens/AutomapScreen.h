#pragma once

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

class W8DialogButton;
extern W8DialogButton** g_automap_buttons;
extern int g_automap_zoom_mode;

unsigned char GetFlag68F105(void);
unsigned char GetFlag68F104(void);

unsigned char IsCursorInsideViewport(void);
unsigned char GetCursorPositionInViewport(srVector3T<float>* position);
unsigned char HasAutomapLayer(int layer);
void RestoreAutomapCameraPosition(void);
unsigned char Function57E490(void);
