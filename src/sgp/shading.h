/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Collapse the released JA2, utility, and precompiled-header branches to the Wizardry build.
   Remove released functions that are neither retained in the Wizardry 8 retail image nor referenced by retained code.
   Formatting normalized for the Wizardry 8 reconstruction, 2026-10-06.
   Distributed under the accompanying SFI Source Code license agreement. */
#ifndef _SHADING_H_
#define _SHADING_H_

#include "himage.h"   // For SGPPaletteEntry
#include "vobject.h"  // For HVOBJECT_SHADE_TABLES
#include "vsurface.h" // For

#ifdef __cplusplus
extern "C" {
#endif

void BuildShadeTable(void);
void SetShadeTablePercent(FLOAT uiShadePercent);

extern SGPPaletteEntry Shaded8BPPPalettes[HVOBJECT_SHADE_TABLES + 3][256];
extern UINT8 ubColorTables[HVOBJECT_SHADE_TABLES + 3][256];

extern UINT16 IntensityTable[65536];
extern UINT16 ShadeTable[65536];
extern UINT16 White16BPPPalette[256];
extern FLOAT guiShadePercent;
extern FLOAT guiBrightPercent;

#ifdef __cplusplus
}
#endif

#define DEFAULT_SHADE_LEVEL 4

#endif
