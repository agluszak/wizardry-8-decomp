/* Modified for the Wizardry 8 reconstruction: 2026-10-03, 2026-10-06, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
// font.c
#include "types.h"
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <windows.h>
#include <stdarg.h>
#include <wchar.h>
#include "sgp.h"
#include "pcx.h"
#include "memman.h"
#include "fileman.h"
#include "Font.h"
#include "Debug.h"

#include "video2.h"

#include "himage.h"
#include "vobject.h"
#include "vobject_blitters.h"
//   Defines

#define PALETTE_SIZE 768
#define STRING_DELIMITER 0
#define ID_BLACK 0
#define MAX_FONTS 25
//   Typedefs

SGPPaletteEntry gSgpPalette[256];

typedef struct {
    UINT16 usDefaultPixelDepth;
    FontTranslationTable* pTranslationTable;
} FontManager;

// GLOBAL: WIZ8 0x006eb704
FontManager* pFManager;
// GLOBAL: WIZ8 0x006eb6a0
HVOBJECT FontObjs[MAX_FONTS];
INT32 FontsLoaded = 0;

// Destination printing parameters
// GLOBAL: WIZ8 0x005ff5f0
INT32 FontDefault = (-1);
// GLOBAL: WIZ8 0x005ff5f4
UINT32 FontDestBuffer = BACKBUFFER;
// GLOBAL: WIZ8 0x005ff5f8
UINT32 FontDestPitch = 640 * 2;
// GLOBAL: WIZ8 0x005ff5fc
UINT32 FontDestBPP = 16;
// GLOBAL: WIZ8 0x005ff600
SGPRect FontDestRegion = {0, 0, 640, 480};
// GLOBAL: WIZ8 0x00650e38
BOOLEAN FontDestWrap = FALSE;
// GLOBAL: WIZ8 0x00650e3a
UINT16 FontForeground16 = 0;
// GLOBAL: WIZ8 0x00650e3c
UINT16 FontBackground16 = 0;
// GLOBAL: WIZ8 0x005ff610
UINT16 FontShadow16 = DEFAULT_SHADOW;
// GLOBAL: WIZ8 0x00650e3e
UINT8 FontForeground8 = 0;
// GLOBAL: WIZ8 0x00650e3f
UINT8 FontBackground8 = 0;

// Temp, for saving printing parameters
// GLOBAL: WIZ8 0x005ff614
INT32 SaveFontDefault = (-1);
// GLOBAL: WIZ8 0x005ff618
UINT32 SaveFontDestBuffer = BACKBUFFER;
// GLOBAL: WIZ8 0x005ff61c
UINT32 SaveFontDestPitch = 640 * 2;
// GLOBAL: WIZ8 0x005ff620
UINT32 SaveFontDestBPP = 16;
// GLOBAL: WIZ8 0x005ff628
SGPRect SaveFontDestRegion = {0, 0, 640, 480};
// GLOBAL: WIZ8 0x00650e40
BOOLEAN SaveFontDestWrap = FALSE;
// GLOBAL: WIZ8 0x00650e42
UINT16 SaveFontForeground16 = 0;
// GLOBAL: WIZ8 0x00650e44
UINT16 SaveFontShadow16 = 0;
// GLOBAL: WIZ8 0x00650e46
UINT16 SaveFontBackground16 = 0;
// GLOBAL: WIZ8 0x00650e48
UINT8 SaveFontForeground8 = 0;
// GLOBAL: WIZ8 0x00650e49
UINT8 SaveFontBackground8 = 0;

// SetFontForeground
//	Sets the foreground color of the currently selected font. The parameter is
// the index into the 8-bit palette. In 8BPP mode, that index number is used
// for the pixel value to be drawn for nontransparent pixels. In 16BPP mode,
// the RGB values from the palette are used to create the pixel color. Note
// that if you change fonts, the selected foreground/background colors will
// stay at what they are currently set to.

// FUNCTION: WIZ8 0x00406c20
void SetFontForeground(UINT8 ubForeground)
{
    UINT32 uiRed, uiGreen, uiBlue;

    if ((FontDefault < 0) || (FontDefault > MAX_FONTS))
        return;

    FontForeground8 = ubForeground;

    uiRed = (UINT32)FontObjs[FontDefault]->pPaletteEntry[ubForeground].peRed;
    uiGreen = (UINT32)FontObjs[FontDefault]->pPaletteEntry[ubForeground].peGreen;
    uiBlue = (UINT32)FontObjs[FontDefault]->pPaletteEntry[ubForeground].peBlue;

    FontForeground16 = Get16BPPColor(FROMRGB(uiRed, uiGreen, uiBlue));
}

// FUNCTION: WIZ8 0x00406c90
void SetFontShadow(UINT8 ubShadow)
{
    UINT32 uiRed, uiGreen, uiBlue;

    if ((FontDefault < 0) || (FontDefault > MAX_FONTS))
        return;

    //FontForeground8=ubForeground;

    uiRed = (UINT32)FontObjs[FontDefault]->pPaletteEntry[ubShadow].peRed;
    uiGreen = (UINT32)FontObjs[FontDefault]->pPaletteEntry[ubShadow].peGreen;
    uiBlue = (UINT32)FontObjs[FontDefault]->pPaletteEntry[ubShadow].peBlue;

    FontShadow16 = Get16BPPColor(FROMRGB(uiRed, uiGreen, uiBlue));

    if (ubShadow != 0) {
        if (FontShadow16 == 0) {
            FontShadow16 = 1;
        }
    }
}

// SetFontBackground
//	Sets the Background color of the currently selected font. The parameter is
// the index into the 8-bit palette. In 8BPP mode, that index number is used
// for the pixel value to be drawn for nontransparent pixels. In 16BPP mode,
// the RGB values from the palette are used to create the pixel color. If the
// background value is zero, the background of the font will be transparent.
// Note that if you change fonts, the selected foreground/background colors will
// stay at what they are currently set to.

// FUNCTION: WIZ8 0x00406d10
void SetFontBackground(UINT8 ubBackground)
{
    UINT32 uiRed, uiGreen, uiBlue;

    if ((FontDefault < 0) || (FontDefault > MAX_FONTS))
        return;

    FontBackground8 = ubBackground;

    uiRed = (UINT32)FontObjs[FontDefault]->pPaletteEntry[ubBackground].peRed;
    uiGreen = (UINT32)FontObjs[FontDefault]->pPaletteEntry[ubBackground].peGreen;
    uiBlue = (UINT32)FontObjs[FontDefault]->pPaletteEntry[ubBackground].peBlue;

    FontBackground16 = Get16BPPColor(FROMRGB(uiRed, uiGreen, uiBlue));
}

// FUNCTION: WIZ8 0x00406d80
void SetRGBFontShadow(UINT32 uiRed, UINT32 uiGreen, UINT32 uiBlue)
{
    if ((FontDefault < 0) || (FontDefault > MAX_FONTS))
        return;
    FontShadow16 = Get16BPPColor(FROMRGB(uiRed, uiGreen, uiBlue));
}
//end Kris

// SetFontObjectPalette16BPP
//	Sets the palette of a font, using a 16 bit palette.

// FUNCTION: WIZ8 0x00406dc0
UINT16* SetFontObjectPalette16BPP(INT32 iFont, UINT16* pPal16)
{
    Assert(iFont >= 0);
    Assert(iFont <= MAX_FONTS);
    Assert(FontObjs[iFont] != NULL);

    FontObjs[iFont]->p16BPPPalette = pPal16;
    FontObjs[iFont]->pShadeCurrent = pPal16;

    return (pPal16);
}

// GetFontObjectPalette16BPP
//	Sets the palette of a font, using a 16 bit palette.

// FUNCTION: WIZ8 0x00406de0
UINT16* GetFontObjectPalette16BPP(INT32 iFont)
{
    Assert(iFont >= 0);
    Assert(iFont <= MAX_FONTS);
    Assert(FontObjs[iFont] != NULL);

    return (FontObjs[iFont]->p16BPPPalette);
}

// GetFontObject
//	Returns the VOBJECT pointer of a font.

// FUNCTION: WIZ8 0x00406df0
HVOBJECT GetFontObject(INT32 iFont)
{
    Assert(iFont >= 0);
    Assert(iFont <= MAX_FONTS);
    Assert(FontObjs[iFont] != NULL);

    return (FontObjs[iFont]);
}

// FindFreeFont
//	Locates an empty slot in the font table.

INT32 FindFreeFont(void)
{
    int count;

    for (count = 0; count < MAX_FONTS; count++)
        if (FontObjs[count] == NULL)
            return (count);

    return (-1);
}

// LoadFontFile
//	Loads a font from an ETRLE file, and inserts it into one of the font slots.
//  This function returns (-1) if it fails, and debug msgs for a reason.
//  Otherwise the font number is returned.

// FUNCTION: WIZ8 0x00406e00
INT32 LoadFontFile(UINT8* filename)
{
    VOBJECT_DESC vo_desc;
    UINT32 LoadIndex;

    Assert(filename != NULL);
    Assert(strlen(filename));

    if ((LoadIndex = FindFreeFont()) == (-1)) {
        DbgMessage(TOPIC_FONT_HANDLER, DBG_LEVEL_0, String("Out of font slots (%s)", filename));
        return (-1);
    }

    vo_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
    strcpy(vo_desc.ImageFile, (char*)filename);

    if ((FontObjs[LoadIndex] = CreateVideoObject(&vo_desc)) == NULL) {
        DbgMessage(TOPIC_FONT_HANDLER, DBG_LEVEL_0,
                   String("Error creating VOBJECT (%s)", filename));
        return (-1);
    }

    if (FontDefault == (-1))
        FontDefault = LoadIndex;

    return (LoadIndex);
}

// UnloadFont - Delete the font structure
//	Deletes the video object of a particular font. Frees up the memory and
// resources allocated for it.

void UnloadFont(UINT32 FontIndex)
{
    Assert(FontIndex >= 0);
    Assert(FontIndex <= MAX_FONTS);
    Assert(FontObjs[FontIndex] != NULL);

    DeleteVideoObject(FontObjs[FontIndex]);
    FontObjs[FontIndex] = NULL;
}

// GetWidth
//	Returns the width of a given character in the font.

UINT32 GetWidth(HVOBJECT hSrcVObject, INT16 ssIndex)
{
    ETRLEObject* pTrav;

    // Assertions
    Assert(hSrcVObject != NULL);

    if (ssIndex < 0 || ssIndex > 92) {
        int i = 0;
    }

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[ssIndex]);
    return ((UINT32)(pTrav->usWidth + pTrav->sOffsetX));
}

// StringPixLengthArg
//		Returns the length of a string with a variable number of arguments, in
// pixels, using the current font. Maximum length in characters the string can
// evaluate to is 512.
//    'uiCharCount' specifies how many characters of the string are counted.

// FUNCTION: WIZ8 0x00406ea0
INT16 StringPixLengthArg(INT32 usUseFont, UINT32 uiCharCount, UINT16* pFontString, ...)
{
    va_list argptr;
    wchar_t string[512];

    Assert(pFontString != NULL);

    va_start(argptr, pFontString);          // Set up variable argument pointer
    vswprintf(string, pFontString, argptr); // process gprintf string (get output str)
    va_end(argptr);

    // make sure the character count is legal
    if (uiCharCount > wcslen(string)) {
        uiCharCount = wcslen(string);
    } else {
        if (uiCharCount < wcslen(string)) {
            // less than the full string, so whack off the end of it (it's temporary anyway)
            string[uiCharCount] = '\0';
        }
    }

    return (StringPixLength(string, usUseFont));
}
//  StringNPixLength
//  Return the length of the of the string or count characters in the
//  string, which ever comes first.
//  Returns INT16
//  Created by:     Gilles Beauparlant
//  Created on:     12/1/99

// FUNCTION: WIZ8 0x00406f90
INT16 StringNPixLength(UINT16* string, UINT32 uiMaxCount, INT32 UseFont)
{
    UINT32 Cur, uiCharCount;
    UINT16 *curletter, transletter;

    Cur = 0;
    uiCharCount = 0;
    curletter = string;

    while ((*curletter) != L'\0' && uiCharCount < uiMaxCount) {
        transletter = GetIndex(*curletter++);
        Cur += GetWidth(FontObjs[UseFont], transletter);
        uiCharCount++;
    }
    return ((INT16)Cur);
}
// StringPixLength
//	Returns the length of a string in pixels, depending on the font given.

// FUNCTION: WIZ8 0x00407010
INT16 StringPixLength(UINT16* string, INT32 UseFont)
{
    UINT32 Cur;
    UINT16 *curletter, transletter;

    if (string == NULL) {
        return (0);
    }

    Cur = 0;
    curletter = string;

    while ((*curletter) != L'\0') {
        transletter = GetIndex(*curletter++);
        Cur += GetWidth(FontObjs[UseFont], transletter);
    }
    return ((INT16)Cur);
}
// SaveFontSettings
//	Saves the current font printing settings into temporary locations.

// FUNCTION: WIZ8 0x00407090
void SaveFontSettings(void)
{
    SaveFontDefault = FontDefault;
    SaveFontDestBuffer = FontDestBuffer;
    SaveFontDestPitch = FontDestPitch;
    SaveFontDestBPP = FontDestBPP;
    SaveFontDestRegion = FontDestRegion;
    SaveFontDestWrap = FontDestWrap;
    SaveFontForeground16 = FontForeground16;
    SaveFontShadow16 = FontShadow16;
    SaveFontBackground16 = FontBackground16;
    SaveFontForeground8 = FontForeground8;
    SaveFontBackground8 = FontBackground8;
}
// RestoreFontSettings
//	Restores the last saved font printing settings from the temporary lactions

// FUNCTION: WIZ8 0x00407140
void RestoreFontSettings(void)
{
    FontDefault = SaveFontDefault;
    FontDestBuffer = SaveFontDestBuffer;
    FontDestPitch = SaveFontDestPitch;
    FontDestBPP = SaveFontDestBPP;
    FontDestRegion = SaveFontDestRegion;
    FontDestWrap = SaveFontDestWrap;
    FontForeground16 = SaveFontForeground16;
    FontShadow16 = SaveFontShadow16;
    FontBackground16 = SaveFontBackground16;
    FontForeground8 = SaveFontForeground8;
    FontBackground8 = SaveFontBackground8;
}

// GetHeight
//	Returns the height of a given character in the font.

UINT32 GetHeight(HVOBJECT hSrcVObject, INT16 ssIndex)
{
    ETRLEObject* pTrav;

    // Assertions
    Assert(hSrcVObject != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[ssIndex]);
    return ((UINT32)(pTrav->usHeight + pTrav->sOffsetY));
}
// GetFontHeight
//	Returns the height of the first character in a font.

// FUNCTION: WIZ8 0x004071f0
UINT16 GetFontHeight(INT32 FontNum)
{
    Assert(FontNum >= 0);
    Assert(FontNum <= MAX_FONTS);
    Assert(FontObjs[FontNum] != NULL);

    return ((UINT16)GetHeight(FontObjs[FontNum], 0));
}

// GetIndex
//		Given a word-sized character, this function returns the index of the
//	cell in the font to print to the screen. The conversion table is built by
//	CreateEnglishTransTable()

INT16 GetIndex(UINT16 siChar)
{
    UINT16* pTrav;
    UINT16 ssCount = 0;
    UINT16 usNumberOfSymbols = pFManager->pTranslationTable->usNumberOfSymbols;

    // search the Translation Table and return the index for the font
    pTrav = pFManager->pTranslationTable->DynamicArrayOf16BitValues;
    while (ssCount < usNumberOfSymbols) {
        if (siChar == *pTrav) {
            return ssCount;
        }
        ssCount++;
        pTrav++;
    }

    // If here, present warning and give the first index
    DbgMessage(TOPIC_FONT_HANDLER, DBG_LEVEL_0,
               String("Error: Invalid character given %d", siChar));

    // Return 0 here, NOT -1 - we should see A's here now...
    return 0;
}

// SetFont
//	Sets the current font number.

// FUNCTION: WIZ8 0x00407210
BOOLEAN SetFont(INT32 iFontIndex)
{
    Assert(iFontIndex >= 0);
    Assert(iFontIndex <= MAX_FONTS);
    Assert(FontObjs[iFontIndex] != NULL);

    FontDefault = iFontIndex;
    return (TRUE);
}

// SetFontDestBuffer
//	Sets the destination buffer for printing to, the clipping rectangle, and
// sets the line wrap on/off. DestBuffer is a VOBJECT handle, not a pointer.

// FUNCTION: WIZ8 0x00407220
BOOLEAN SetFontDestBuffer(UINT32 DestBuffer, INT32 x1, INT32 y1, INT32 x2, INT32 y2, BOOLEAN wrap)
{
    Assert(x2 > x1);
    Assert(y2 > y1);

    FontDestBuffer = DestBuffer;

    FontDestRegion.iLeft = x1;
    FontDestRegion.iTop = y1;
    FontDestRegion.iRight = x2;
    FontDestRegion.iBottom = y2;
    FontDestWrap = wrap;

    return (TRUE);
}

// mprintf
//	Prints to the currently selected destination buffer, at the X/Y coordinates
// specified, using the currently selected font. Other than the X/Y coordinates,
// the parameters are identical to printf. The resulting string may be no longer
// than 512 word-characters. Uses monochrome font color settings

// FUNCTION: WIZ8 0x00407260
UINT32 mprintf(INT32 x, INT32 y, UINT16* pFontString, ...)
{
    INT32 destx, desty;
    UINT16 *curletter, transletter;
    va_list argptr;
    wchar_t string[512];
    UINT32 uiDestPitchBYTES;
    UINT8* pDestBuf;

    Assert(pFontString != NULL);

    va_start(argptr, pFontString);          // Set up variable argument pointer
    vswprintf(string, pFontString, argptr); // process gprintf string (get output str)
    va_end(argptr);

    curletter = string;

    destx = x;
    desty = y;

    // Lock the dest buffer
    pDestBuf = LockVideoSurface(FontDestBuffer, &uiDestPitchBYTES);

    while ((*curletter) != 0) {
        transletter = GetIndex(*curletter++);

        if (FontDestWrap &&
            BltIsClipped(FontObjs[FontDefault], destx, desty, transletter, &FontDestRegion)) {
            destx = x;
            desty += GetHeight(FontObjs[FontDefault], transletter);
        }

        // Blit directly
        if (gbPixelDepth == 8) {
            Blt8BPPDataTo8BPPBufferMonoShadowClip(pDestBuf, uiDestPitchBYTES, FontObjs[FontDefault],
                                                  destx, desty, transletter, &FontDestRegion,
                                                  FontForeground8, FontBackground8);
        } else {
            Blt8BPPDataTo16BPPBufferMonoShadowClip(
                (UINT16*)pDestBuf, uiDestPitchBYTES, FontObjs[FontDefault], destx, desty,
                transletter, &FontDestRegion, FontForeground16, FontBackground16, FontShadow16);
        }
        destx += GetWidth(FontObjs[FontDefault], transletter);
    }

    // Unlock buffer
    UnLockVideoSurface(FontDestBuffer);

    return (0);
}

// FUNCTION: WIZ8 0x00407420
void VarFindFontRightCoordinates(INT16 sLeft, INT16 sTop, INT16 sWidth, INT16 sHeight,
                                 INT32 iFontIndex, INT16* psNewX, INT16* psNewY,
                                 UINT16* pFontString, ...)
{
    wchar_t string[512];
    va_list argptr;

    va_start(argptr, pFontString);          // Set up variable argument pointer
    vswprintf(string, pFontString, argptr); // process gprintf string (get output str)
    va_end(argptr);

    FindFontRightCoordinates(sLeft, sTop, sWidth, sHeight, string, iFontIndex, psNewX, psNewY);
}

// FUNCTION: WIZ8 0x00407530
void VarFindFontCenterCoordinates(INT16 sLeft, INT16 sTop, INT16 sWidth, INT16 sHeight,
                                  INT32 iFontIndex, INT16* psNewX, INT16* psNewY,
                                  UINT16* pFontString, ...)
{
    wchar_t string[512];
    va_list argptr;

    va_start(argptr, pFontString);          // Set up variable argument pointer
    vswprintf(string, pFontString, argptr); // process gprintf string (get output str)
    va_end(argptr);

    FindFontCenterCoordinates(sLeft, sTop, sWidth, sHeight, string, iFontIndex, psNewX, psNewY);
}

void FindFontRightCoordinates(INT16 sLeft, INT16 sTop, INT16 sWidth, INT16 sHeight, UINT16* pStr,
                              INT32 iFontIndex, INT16* psNewX, INT16* psNewY)
{
    INT16 xp, yp;

    // Compute the coordinates to right justify the text
    xp = ((sWidth - StringPixLength(pStr, iFontIndex))) + sLeft;
    yp = ((sHeight - GetFontHeight(iFontIndex)) / 2) + sTop;

    *psNewX = xp;
    *psNewY = yp;
}

void FindFontCenterCoordinates(INT16 sLeft, INT16 sTop, INT16 sWidth, INT16 sHeight, UINT16* pStr,
                               INT32 iFontIndex, INT16* psNewX, INT16* psNewY)
{
    INT16 xp, yp;

    // Compute the coordinates to center the text
    xp = ((sWidth - StringPixLength(pStr, iFontIndex) + 1) / 2) + sLeft;
    yp = ((sHeight - GetFontHeight(iFontIndex)) / 2) + sTop;

    *psNewX = xp;
    *psNewY = yp;
}

// gprintf
//	Prints to the currently selected destination buffer, at the X/Y coordinates
// specified, using the currently selected font. Other than the X/Y coordinates,
// the parameters are identical to printf. The resulting string may be no longer
// than 512 word-characters.

// FUNCTION: WIZ8 0x00407650
UINT32 gprintf(INT32 x, INT32 y, UINT16* pFontString, ...)
{
    INT32 destx, desty;
    UINT16 *curletter, transletter;
    va_list argptr;
    wchar_t string[512];
    UINT32 uiDestPitchBYTES;
    UINT8* pDestBuf;

    Assert(pFontString != NULL);

    va_start(argptr, pFontString);          // Set up variable argument pointer
    vswprintf(string, pFontString, argptr); // process gprintf string (get output str)
    va_end(argptr);

    curletter = string;

    destx = x;
    desty = y;

    // Lock the dest buffer
    pDestBuf = LockVideoSurface(FontDestBuffer, &uiDestPitchBYTES);

    while ((*curletter) != 0) {
        transletter = GetIndex(*curletter++);

        if (FontDestWrap &&
            BltIsClipped(FontObjs[FontDefault], destx, desty, transletter, &FontDestRegion)) {
            destx = x;
            desty += GetHeight(FontObjs[FontDefault], transletter);
        }

        // Blit directly
        if (gbPixelDepth == 8) {
            Blt8BPPDataTo8BPPBufferTransparentClip((UINT16*)pDestBuf, uiDestPitchBYTES,
                                                   FontObjs[FontDefault], destx, desty, transletter,
                                                   &FontDestRegion);
        } else {
            Blt8BPPDataTo16BPPBufferTransparentClip((UINT16*)pDestBuf, uiDestPitchBYTES,
                                                    FontObjs[FontDefault], destx, desty,
                                                    transletter, &FontDestRegion);
        }
        destx += GetWidth(FontObjs[FontDefault], transletter);
    }

    // Unlock buffer
    UnLockVideoSurface(FontDestBuffer);

    return (0);
}

// FUNCTION: WIZ8 0x004077d0
UINT32 gprintfDirty(INT32 x, INT32 y, UINT16* pFontString, ...)
{
    INT32 destx, desty;
    UINT16 *curletter, transletter;
    va_list argptr;
    wchar_t string[512];
    UINT32 uiDestPitchBYTES;
    UINT8* pDestBuf;

    Assert(pFontString != NULL);

    va_start(argptr, pFontString);          // Set up variable argument pointer
    vswprintf(string, pFontString, argptr); // process gprintf string (get output str)
    va_end(argptr);

    curletter = string;

    destx = x;
    desty = y;

    // Lock the dest buffer
    pDestBuf = LockVideoSurface(FontDestBuffer, &uiDestPitchBYTES);

    while ((*curletter) != 0) {
        transletter = GetIndex(*curletter++);

        if (FontDestWrap &&
            BltIsClipped(FontObjs[FontDefault], destx, desty, transletter, &FontDestRegion)) {
            destx = x;
            desty += GetHeight(FontObjs[FontDefault], transletter);
        }

        // Blit directly
        if (gbPixelDepth == 8) {
            Blt8BPPDataTo8BPPBufferTransparentClip((UINT16*)pDestBuf, uiDestPitchBYTES,
                                                   FontObjs[FontDefault], destx, desty, transletter,
                                                   &FontDestRegion);
        } else {
            Blt8BPPDataTo16BPPBufferTransparentClip((UINT16*)pDestBuf, uiDestPitchBYTES,
                                                    FontObjs[FontDefault], destx, desty,
                                                    transletter, &FontDestRegion);
        }
        destx += GetWidth(FontObjs[FontDefault], transletter);
    }

    // Unlock buffer
    UnLockVideoSurface(FontDestBuffer);

    InvalidateRegion(x, y, x + StringPixLength(string, FontDefault), y + GetFontHeight(FontDefault),
                     INVAL_SRC_TRANS);

    return (0);
}

// gprintf_buffer
//	Prints to the currently selected destination buffer, at the X/Y coordinates
// specified, using the currently selected font. Other than the X/Y coordinates,
// the parameters are identical to printf. The resulting string may be no longer
// than 512 word-characters.

// FUNCTION: WIZ8 0x00407a10
UINT32 gprintf_buffer(UINT8* pDestBuf, UINT32 uiDestPitchBYTES, UINT32 FontType, INT32 x, INT32 y,
                      UINT16* pFontString, ...)
{
    INT32 destx, desty;
    UINT16 *curletter, transletter;
    va_list argptr;
    wchar_t string[512];

    Assert(pFontString != NULL);

    va_start(argptr, pFontString);          // Set up variable argument pointer
    vswprintf(string, pFontString, argptr); // process gprintf string (get output str)
    va_end(argptr);

    curletter = string;

    destx = x;
    desty = y;

    while ((*curletter) != 0) {
        transletter = GetIndex(*curletter++);

        if (FontDestWrap &&
            BltIsClipped(FontObjs[FontType], destx, desty, transletter, &FontDestRegion)) {
            destx = x;
            desty += GetHeight(FontObjs[FontType], transletter);
        }

        // Blit directly
        if (gbPixelDepth == 8) {
            Blt8BPPDataTo8BPPBufferTransparentClip((UINT16*)pDestBuf, uiDestPitchBYTES,
                                                   FontObjs[FontDefault], destx, desty, transletter,
                                                   &FontDestRegion);
        } else {
            Blt8BPPDataTo16BPPBufferTransparentClip((UINT16*)pDestBuf, uiDestPitchBYTES,
                                                    FontObjs[FontDefault], destx, desty,
                                                    transletter, &FontDestRegion);
        }

        destx += GetWidth(FontObjs[FontType], transletter);
    }

    return (0);
}

// FUNCTION: WIZ8 0x00407b80
UINT32 mprintf_buffer(UINT8* pDestBuf, UINT32 uiDestPitchBYTES, UINT32 FontType, INT32 x, INT32 y,
                      UINT16* pFontString, ...)
{
    INT32 destx, desty;
    UINT16 *curletter, transletter;
    va_list argptr;
    wchar_t string[512];

    Assert(pFontString != NULL);

    va_start(argptr, pFontString);          // Set up variable argument pointer
    vswprintf(string, pFontString, argptr); // process gprintf string (get output str)
    va_end(argptr);

    curletter = string;

    destx = x;
    desty = y;

    while ((*curletter) != 0) {
        transletter = GetIndex(*curletter++);

        if (FontDestWrap &&
            BltIsClipped(FontObjs[FontDefault], destx, desty, transletter, &FontDestRegion)) {
            destx = x;
            desty += GetHeight(FontObjs[FontDefault], transletter);
        }

        // Blit directly
        if (gbPixelDepth == 8) {
            Blt8BPPDataTo8BPPBufferMonoShadowClip(pDestBuf, uiDestPitchBYTES, FontObjs[FontDefault],
                                                  destx, desty, transletter, &FontDestRegion,
                                                  FontForeground8, FontBackground8);
        } else {
            Blt8BPPDataTo16BPPBufferMonoShadowClip(
                (UINT16*)pDestBuf, uiDestPitchBYTES, FontObjs[FontDefault], destx, desty,
                transletter, &FontDestRegion, FontForeground16, FontBackground16, FontShadow16);
        }
        destx += GetWidth(FontObjs[FontDefault], transletter);
    }

    return (0);
}

// InitializeFontManager
//	Starts up the font manager system with the appropriate translation table.

// FUNCTION: WIZ8 0x00407d30
BOOLEAN InitializeFontManager(UINT16 usDefaultPixelDepth, FontTranslationTable* pTransTable)
{
    FontTranslationTable* pTransTab;
    int count;
    UINT16 uiRight, uiBottom;
    UINT8 uiPixelDepth;

    FontDefault = (-1);
    FontDestBuffer = BACKBUFFER;
    FontDestPitch = 0;

    //	FontDestBPP=0;

    GetCurrentVideoSettings(&uiRight, &uiBottom, &uiPixelDepth);
    FontDestRegion.iLeft = 0;
    FontDestRegion.iTop = 0;
    FontDestRegion.iRight = (INT32)uiRight;
    FontDestRegion.iBottom = (INT32)uiBottom;
    FontDestBPP = (UINT32)uiPixelDepth;

    FontDestWrap = FALSE;

    // register the appropriate debug topics
    if (pTransTable == NULL) {
        return FALSE;
    }
    RegisterDebugTopic(TOPIC_FONT_HANDLER, "Font Manager");

    if ((pFManager = (FontManager*)MemAlloc(sizeof(FontManager))) == NULL) {
        return FALSE;
    }

    if ((pTransTab = (FontTranslationTable*)MemAlloc(sizeof(FontTranslationTable))) == NULL) {
        return FALSE;
    }

    pFManager->pTranslationTable = pTransTab;
    pFManager->usDefaultPixelDepth = usDefaultPixelDepth;
    pTransTab->usNumberOfSymbols = pTransTable->usNumberOfSymbols;
    pTransTab->DynamicArrayOf16BitValues = pTransTable->DynamicArrayOf16BitValues;

    // Mark all font slots as empty
    for (count = 0; count < MAX_FONTS; count++)
        FontObjs[count] = NULL;

    return TRUE;
}

// ShutdownFontManager
//	Shuts down, and deallocates all fonts.

// FUNCTION: WIZ8 0x00407e30
void ShutdownFontManager(void)
{
    INT32 count;

    UnRegisterDebugTopic(TOPIC_FONT_HANDLER, "Font Manager");
    if (pFManager)
        MemFree(pFManager);

    for (count = 0; count < MAX_FONTS; count++) {
        if (FontObjs[count] != NULL)
            UnloadFont(count);
    }
}

// DestroyEnglishTransTable
// Destroys the English text->font map table.

// FUNCTION: WIZ8 0x00407E70
void DestroyEnglishTransTable(void)
{
    if (pFManager) {
        if (pFManager->pTranslationTable != NULL) {
            if (pFManager->pTranslationTable->DynamicArrayOf16BitValues != NULL) {
                MemFree(pFManager->pTranslationTable->DynamicArrayOf16BitValues);
            }

            MemFree(pFManager->pTranslationTable);

            pFManager->pTranslationTable = NULL;
        }
    }
}

// CreateEnglishTransTable
// Creates the English text->font map table.

// FUNCTION: WIZ8 0x00407ec0
FontTranslationTable* CreateEnglishTransTable()
{
    FontTranslationTable* pTable = NULL;
    UINT16* temp;

    pTable = (FontTranslationTable*)MemAlloc(sizeof(FontTranslationTable));
    pTable->usNumberOfSymbols = 252;
    pTable->DynamicArrayOf16BitValues = (UINT16*)MemAlloc(pTable->usNumberOfSymbols * 2);
    temp = pTable->DynamicArrayOf16BitValues;

    *temp = 'A';
    temp++;
    *temp = 'B';
    temp++;
    *temp = 'C';
    temp++;
    *temp = 'D';
    temp++;
    *temp = 'E';
    temp++;
    *temp = 'F';
    temp++;
    *temp = 'G';
    temp++;
    *temp = 'H';
    temp++;
    *temp = 'I';
    temp++;
    *temp = 'J';
    temp++;
    *temp = 'K';
    temp++;
    *temp = 'L';
    temp++;
    *temp = 'M';
    temp++;
    *temp = 'N';
    temp++;
    *temp = 'O';
    temp++;
    *temp = 'P';
    temp++;
    *temp = 'Q';
    temp++;
    *temp = 'R';
    temp++;
    *temp = 'S';
    temp++;
    *temp = 'T';
    temp++;
    *temp = 'U';
    temp++;
    *temp = 'V';
    temp++;
    *temp = 'W';
    temp++;
    *temp = 'X';
    temp++;
    *temp = 'Y';
    temp++;
    *temp = 'Z';
    temp++;
    *temp = 'a';
    temp++;
    *temp = 'b';
    temp++;
    *temp = 'c';
    temp++;
    *temp = 'd';
    temp++;
    *temp = 'e';
    temp++;
    *temp = 'f';
    temp++;
    *temp = 'g';
    temp++;
    *temp = 'h';
    temp++;
    *temp = 'i';
    temp++;
    *temp = 'j';
    temp++;
    *temp = 'k';
    temp++;
    *temp = 'l';
    temp++;
    *temp = 'm';
    temp++;
    *temp = 'n';
    temp++;
    *temp = 'o';
    temp++;
    *temp = 'p';
    temp++;
    *temp = 'q';
    temp++;
    *temp = 'r';
    temp++;
    *temp = 's';
    temp++;
    *temp = 't';
    temp++;
    *temp = 'u';
    temp++;
    *temp = 'v';
    temp++;
    *temp = 'w';
    temp++;
    *temp = 'x';
    temp++;
    *temp = 'y';
    temp++;
    *temp = 'z';
    temp++;
    *temp = '0';
    temp++;
    *temp = '1';
    temp++;
    *temp = '2';
    temp++;
    *temp = '3';
    temp++;
    *temp = '4';
    temp++;
    *temp = '5';
    temp++;
    *temp = '6';
    temp++;
    *temp = '7';
    temp++;
    *temp = '8';
    temp++;
    *temp = '9';
    temp++;
    *temp = '!';
    temp++;
    *temp = '@';
    temp++;
    *temp = '#';
    temp++;
    *temp = '$';
    temp++;
    *temp = '%';
    temp++;
    *temp = '^';
    temp++;
    *temp = '&';
    temp++;
    *temp = '*';
    temp++;
    *temp = '(';
    temp++;
    *temp = ')';
    temp++;
    *temp = '-';
    temp++;
    *temp = '_';
    temp++;
    *temp = '+';
    temp++;
    *temp = '=';
    temp++;
    *temp = '|';
    temp++;
    *temp = '\\';
    temp++;
    *temp = '{';
    temp++;
    *temp = '}'; // 80
    temp++;
    *temp = '[';
    temp++;
    *temp = ']';
    temp++;
    *temp = ':';
    temp++;
    *temp = ';';
    temp++;
    *temp = '"';
    temp++;
    *temp = '\'';
    temp++;
    *temp = '<';
    temp++;
    *temp = '>';
    temp++;
    *temp = ',';
    temp++;
    *temp = '.';
    temp++;
    *temp = '?';
    temp++;
    *temp = '/';
    temp++;
    *temp = ' '; //93
    temp++;

    // Windows Code Page 1252 Western Standard Character Set

    *temp = 193; // "A" acute
    temp++;
    *temp = 192; // "A" grave
    temp++;
    *temp = 193; // "A" circumflex
    temp++;
    *temp = 196; // "A" umlaut
    temp++;
    *temp = 195; // "A" tilde
    temp++;
    *temp = 197; // "A" ring
    temp++;
    *temp = 199; // "C" cedile
    temp++;
    *temp = 201; // "E" acute
    temp++;
    *temp = 200; // "E" grave
    temp++;
    *temp = 202; // "E" circumflex
    temp++;
    *temp = 203; // "E" umlaut
    temp++;
    *temp = 205; // "I" acute
    temp++;
    *temp = 204; // "I" grave
    temp++;
    *temp = 206; // "I" circumflex
    temp++;
    *temp = 207; // "I" umlaut
    temp++;
    *temp = 209; // "N" tilde
    temp++;
    *temp = 211; // "O" acute
    temp++;
    *temp = 210; // "O" grave
    temp++;
    *temp = 212; // "O" circumflex
    temp++;
    *temp = 214; // "O" umlaut
    temp++;
    *temp = 213; // "O" tilde
    temp++;
    *temp = 216; // "0" O strike-through
    temp++;
    *temp = 218; // "U" acute
    temp++;
    *temp = 217; // "U" grave
    temp++;
    *temp = 219; // "U" circumflex
    temp++;
    *temp = 220; // "U" umlaut
    temp++;
    *temp = 221; // "Y" acute
    temp++;
    *temp = 225; // "a" acute
    temp++;
    *temp = 224; // "a" grave
    temp++;
    *temp = 226; // "a" circumflex
    temp++;
    *temp = 228; // "a" umlaut
    temp++;
    *temp = 227; // "a" tilde
    temp++;
    *temp = 229; // "a" ring
    temp++;
    *temp = 231; // "c" cedile
    temp++;
    *temp = 233; // "e" acute
    temp++;
    *temp = 232; // "e" grave
    temp++;
    *temp = 234; // "e" circumflex
    temp++;
    *temp = 235; // "e" umlaut
    temp++;
    *temp = 237; // "i" acute
    temp++;
    *temp = 236; // "i" grave
    temp++;
    *temp = 238; // "i" circumflex
    temp++;
    *temp = 239; // "i" umlaut
    temp++;
    *temp = 241; // "n" tilde
    temp++;
    *temp = 243; // "o" acute
    temp++;
    *temp = 242; // "o" grave
    temp++;
    *temp = 244; // "o" circumflex
    temp++;
    *temp = 246; // "o" umlaut
    temp++;
    *temp = 245; // "o" tilde
    temp++;
    *temp = 248; // "o" strike-through
    temp++;
    *temp = 250; // "u" acute
    temp++;
    *temp = 249; // "u" grave
    temp++;
    *temp = 251; // "u" circumflex
    temp++;
    *temp = 252; // "u" umlaut
    temp++;
    *temp = 254; // "y" acute
    temp++;
    *temp = 255; // "y" umlaut
    temp++;
    *temp = 223; // beta

    // Font glyphs for spell targeting icons
    //ATE: IMPORTANT! INcreate the array above if you add any new items here...
    temp++;
    *temp = FONT_GLYPH_TARGET_POINT;
    temp++;
    *temp = FONT_GLYPH_TARGET_CONE;
    temp++;
    *temp = FONT_GLYPH_TARGET_SINGLE;
    temp++;
    *temp = FONT_GLYPH_TARGET_GROUP;
    temp++;
    *temp = FONT_GLYPH_TARGET_NONE;

    // 154

    // Wizardry: entries 154-249 are unused
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;
    temp++;
    *temp = 0;

    temp++;
    *temp = 191; // inverted question mark
    temp++;
    *temp = 161; // inverted exclamation mark

    return pTable;
}
// LoadFontFile
// Parameter List : filename - File created by the utility tool to open
// Return Value  pointer to the base structure
// Modification History :
// Dec 15th 1996 -> modified for use by Wizardry
// GetMaxFontWidth - Gets the maximum font width
// Parameter List : pointer to the base structure
// Return Value  Maximum font width
// Modification History :
// Dec 15th 1996 -> modified for use by Wizardry
// ConvertToPaletteEntry
// Parameter List : Converts from RGB to SGPPaletteEntry
// Return Value  pointer to the SGPPaletteEntry
// Modification History :
// Dec 15th 1996 -> modified for use by Wizardry
// SetFontPalette - Sets the Palette
// Parameter List : pointer to the base structure
//                  new pixel depth
//                  new Palette size
//                  pointer to palette data
// Return Value  BOOLEAN
// Modification History :
// Dec 15th 1996 -> modified for use by Wizardry
// SetFont16BitData - Sets the font structure to hold 16 bit data
// Parameter List : pointer to the base structure
//                  pointer to new 16 bit data
// Return Value  BOOLEAN
// Modification History :
// Dec 15th 1996 -> modified for use by Wizardry
// Blt8Imageto16Dest
// Parameter List : Start offset
//                  End Offset
//                  Dest x, y
//                  Font Width
//                  Pointer to Base structure
//                  Pointer to destination buffer
//                  Destination Pitch
//                  Height of Each element
// Return Value  : BOOLEAN
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
// Blt8Imageto8Dest
// Parameter List : Start offset
//                  End Offset
//                  Dest x, y
//                  Font Width
//                  Pointer to Base structure
//                  Pointer to destination buffer
//                  Destination Pitch
//                  Height of Each element
// Return Value  : BOOLEAN
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
// Blt16Imageto16Dest
// Parameter List : Start offset
//                  End Offset
//                  Dest x, y
//                  Font Width
//                  Pointer to Base structure
//                  Pointer to destination buffer
//                  Destination Pitch
//                  Height of Each element
// Return Value  : BOOLEAN
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
// GetOffset
// Parameter List : Given the index, gets the corresponding offset
// Return Value  : offset
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
// GetOffLen
// Parameter List : Given the index, gets the corresponding offset
// length which is the number of compressed pixels
// Return Value  : offset
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
// PrintFontString
// Parameter List : pointer to \0 (NULL) terminated font string
//                  x,y,TotalWidth, TotalHeight is the bounding rectangle where
//                  the font is to be printed
//                  Multiline if true will print on multiple lines otherwise on 1 line
//                  Pointer to base structure
// Return Value  : BOOLEAN
// Modification History :
// Nov 26th 1996 -> modified for use by Wizardry
