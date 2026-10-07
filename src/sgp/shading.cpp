/* Modified for the Wizardry 8 reconstruction: 2026-10-03, 2026-10-06, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "DirectDraw Calls.h"
#include <stdio.h>
#include "debug.h"
#include "video2.h"
#include "himage.h"
#include "vobject.h"
#include "vobject_private.h"
#include "video_private.h"
#include "wcheck.h"
#include "vobject_blitters.h"
#include "shading.h"

void FindMaskIndecies(UINT8*, UINT8*, UINT8*);

SGPPaletteEntry Shaded8BPPPalettes[HVOBJECT_SHADE_TABLES + 3][256];
// GLOBAL: WIZ8 0x006bdaa0
UINT8 ubColorTables[HVOBJECT_SHADE_TABLES + 3][256];

// GLOBAL: WIZ8 0x0069da80
UINT16 IntensityTable[65536];
// GLOBAL: WIZ8 0x006c0da0
UINT16 ShadeTable[65536];
// GLOBAL: WIZ8 0x006e0da0
UINT16 White16BPPPalette[256];
// GLOBAL: WIZ8 0x006000ac
FLOAT guiShadePercent = (FLOAT)0.48;
FLOAT guiBrightPercent = (FLOAT)1.1;

/**********************************************************************************************
 BuildShadeTable

	Builds a 16-bit color shading table. This function should be called only after the current
	video adapter's pixel format is known (IE: GetRgbDistribution() has been called, and the
	globals for masks and shifts have been initialized by that function), and before any
	blitting is done.

	Using the table is a straight lookup. The pixel to be shaded down is used as the index into
	the table and the entry at that point will be a pixel that is 25% darker.

**********************************************************************************************/
// FUNCTION: WIZ8 0x00413d60
void BuildShadeTable(void)
{
    UINT16 red, green, blue;
    UINT16 index;

    for (red = 0; red < 256; red += 4)
        for (green = 0; green < 256; green += 4)
            for (blue = 0; blue < 256; blue += 4) {
                index = Get16BPPColor(FROMRGB(red, green, blue));
                ShadeTable[index] = Get16BPPColor(FROMRGB(
                    red * guiShadePercent, green * guiShadePercent, blue * guiShadePercent));
            }

    memset(White16BPPPalette, 65535, sizeof(White16BPPPalette));
}

// FUNCTION: WIZ8 0x00413e80
void SetShadeTablePercent(FLOAT uiShadePercent)
{
    guiShadePercent = uiShadePercent;
    BuildShadeTable();
}
