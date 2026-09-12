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
extern int g_value_68f2c4;

class W8DialogButton;
extern W8DialogButton** g_automap_buttons;
extern int g_automap_zoom_mode;

unsigned char GetFlag68F105(void);
unsigned char GetFlag68F104(void);

unsigned char HasAutomapLayer(int layer);
void RestoreAutomapCameraPosition(void);
unsigned char CanUseCurrentAutomapTool(void);

void ResetAutomapView005817D0(void);
void Function581CE0(int handle);
unsigned char Function582050(srVector3T<float>* position);
void Function5820F0(int tool);
W8AutomapNote* Function582180(void);
void Function5822C0(void);
void Function582930(void);
void Function583BC0(void);

unsigned char ReadAutomapNodes00584DD0(int hFile);
void RedrawTextBoxBody(void);
