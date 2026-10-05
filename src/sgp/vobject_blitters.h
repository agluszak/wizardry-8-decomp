/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Remove released functions that are neither retained in the Wizardry 8 retail image nor referenced by retained code.
   Formatting normalized for the Wizardry 8 reconstruction, 2026-10-06.
   Distributed under the accompanying SFI Source Code license agreement. */
#ifndef __VOBJECT_BLITTERS
#define __VOBJECT_BLITTERS

#include "shading.h"
#include "vsurface.h"
#include "vobject.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SGPRect ClippingRect;
extern UINT32 guiTranslucentMask;
extern UINT16 White16BPPPalette[256];

extern void SetClippingRect(SGPRect* clip);
void GetClippingRect(SGPRect* clip);

BOOLEAN BltIsClipped(HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect* clipregion);

// 8-Bit to 8-Bit Blitters

//BOOLEAN Blt8BPPDataTo8BPPBufferTransZIncClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion);

// pixelation blitters

BOOLEAN Blt8BPPDataTo8BPPBufferMonoShadowClip(UINT8* pBuffer, UINT32 uiDestPitchBYTES,
                                              HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                              UINT16 usIndex, SGPRect* clipregion,
                                              UINT8 ubForeground, UINT8 ubBackground);

BOOLEAN Blt8BPPDataTo8BPPBufferTransparent(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                           HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                           UINT16 usIndex);
BOOLEAN Blt8BPPDataTo8BPPBufferTransparentClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                               HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                               UINT16 usIndex, SGPRect* clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferTransMirror(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                            HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                            UINT16 usIndex);

BOOLEAN Blt8BPPDataTo8BPPBufferShadowClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                          HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex,
                                          SGPRect* clipregion);
BOOLEAN Blt8BPPDataTo8BPPBufferShadow(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                      HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex);

// 8-Bit to 16-Bit Blitters

// pixelation blitters

// translucency blitters
BOOLEAN Blt8BPPDataTo16BPPBufferMonoShadowClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                               HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                               UINT16 usIndex, SGPRect* clipregion,
                                               UINT16 usForeground, UINT16 usBackground,
                                               UINT16 usShadow);

// Next blitters are for blitting mask as intensity

BOOLEAN Blt8BPPDataTo16BPPBufferTransparentClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                                HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                                UINT16 usIndex, SGPRect* clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferTransparent(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                            HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                            UINT16 usIndex);

BOOLEAN Blt8BPPDataTo16BPPBufferShadowClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                           HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex,
                                           SGPRect* clipregion);
BOOLEAN DDBlt8BPPDataTo16BPPBufferShadow(HVOBJECT hDestVObject, HVOBJECT hSrcVObject, UINT8 level,
                                         COLORVAL maskrgb, UINT16 usX, UINT16 usY,
                                         SGPRect* srcRect);

BOOLEAN Blt8BPPTo8BPP(UINT8* pDest, UINT32 uiDestPitch, UINT8* pSrc, UINT32 uiSrcPitch,
                      INT32 iDestXPos, INT32 iDestYPos, INT32 iSrcXPos, INT32 iSrcYPos,
                      UINT32 uiWidth, UINT32 uiHeight);
BOOLEAN Blt16BPPTo16BPP(UINT16* pDest, UINT32 uiDestPitch, UINT16* pSrc, UINT32 uiSrcPitch,
                        INT32 iDestXPos, INT32 iDestYPos, INT32 iSrcXPos, INT32 iSrcYPos,
                        UINT32 uiWidth, UINT32 uiHeight);
BOOLEAN Blt16BPPTo16BPPTrans(UINT16* pDest, UINT32 uiDestPitch, UINT16* pSrc, UINT32 uiSrcPitch,
                             INT32 iDestXPos, INT32 iDestYPos, INT32 iSrcXPos, INT32 iSrcYPos,
                             UINT32 uiWidth, UINT32 uiHeight, UINT16 usTrans);
BOOLEAN Blt16BPPTo16BPPFog(UINT16* pDest, UINT32 uiDestPitch, UINT16* pSrc, UINT32 uiSrcPitch,
                           INT32 iDestXPos, INT32 iDestYPos, INT32 iSrcXPos, INT32 iSrcYPos,
                           UINT32 uiWidth, UINT32 uiHeight, UINT8* pFog, UINT16 usFogPitch);
BOOLEAN Blt16BPPTo16BPPMirror(UINT16* pDest, UINT32 uiDestPitch, UINT16* pSrc, UINT32 uiSrcPitch,
                              INT32 iDestXPos, INT32 iDestYPos, INT32 iSrcXPos, INT32 iSrcYPos,
                              UINT32 uiWidth, UINT32 uiHeight);

BOOLEAN Blt8BPPDataTo16BPPBufferFogZ(UINT16* pBuffer, UINT32 uiDestPitchBYTES, UINT16* pZBuffer,
                                     UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                     UINT16 usIndex);
BOOLEAN Blt8BPPDataTo16BPPBufferFogZClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES, UINT16* pZBuffer,
                                         UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                         UINT16 usIndex, SGPRect* clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferFogZNB(UINT16* pBuffer, UINT32 uiDestPitchBYTES, UINT16* pZBuffer,
                                       UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                       UINT16 usIndex);
BOOLEAN Blt8BPPDataTo16BPPBufferFogZNBClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                           UINT16* pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject,
                                           INT32 iX, INT32 iY, UINT16 usIndex, SGPRect* clipregion);

BOOLEAN Blt16BPPBufferPixelateRectWithColor(UINT16* pBuffer, UINT32 uiDestPitchBYTES, SGPRect* area,
                                            UINT8 Pattern[8][8], UINT16 usColor);
//A wrapper for the Blt16BPPBufferPixelateRect that automatically passes a hatch pattern.
BOOLEAN Blt16BPPBufferHatchRect(UINT16* pBuffer, UINT32 uiDestPitchBYTES, SGPRect* area);
BOOLEAN Blt16BPPBufferShadowRect(UINT16* pBuffer, UINT32 uiDestPitchBYTES, SGPRect* area);

BOOLEAN Blt8BPPDataTo16BPPBufferShadow(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                       HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex);

BOOLEAN Blt8BPPDataSubTo16BPPBuffer(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                    HVSURFACE hSrcVSurface, UINT8* pSrcBuffer, UINT32 uiSrcPitch,
                                    INT32 iX, INT32 iY, SGPRect* pRect);

// Blits from flat 8bpp source, to 16bpp dest, divides in half
BOOLEAN DDBlt8BPPDataTo16BPPBuffer(HVOBJECT hDestVObject, HVOBJECT hSrcVObject, UINT16 usX,
                                   UINT16 usY, SGPRect* srcRect);
BOOLEAN DDBlt8BPPDataTo16BPPBufferFullTransparent(HVOBJECT hDestVObject, HVOBJECT hSrcVObject,
                                                  UINT16 usX, UINT16 usY, SGPRect* srcRect);
BOOLEAN DDFillSurface(HVOBJECT hDestVObject, blt_fx* pBltFx);
BOOLEAN DDFillSurfaceRect(HVOBJECT hDestVObject, blt_fx* pBltFx);
BOOLEAN BltVObjectUsingDD(HVOBJECT hDestVObject, HVOBJECT hSrcVObject, UINT32 fBltFlags,
                          INT32 iDestX, INT32 iDestY, RECT* SrcRect);

// New 16/16 blitters

BOOLEAN Blt16BPPDataTo16BPPBufferTransparent(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                             HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                             UINT16 usIndex);

// ATE: New blitters for showing an outline at color 254

// ATE: New blitter for included shadow, but pixellate if obscured by z
BOOLEAN FillRect16BPP(UINT16* pBuffer, UINT32 uiDestPitchBYTES, INT32 x1, INT32 y1, INT32 x2,
                      INT32 y2, UINT16 color);

#ifdef __cplusplus
}
#endif

#endif
