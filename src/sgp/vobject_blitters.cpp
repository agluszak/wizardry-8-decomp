/* Modified for the Wizardry 8 reconstruction: 2026-10-03, 2026-10-06, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "DirectDraw Calls.h"
#include <stdio.h>
#include "debug.h"
#include "video2.h" // Wiz8
#include "himage.h"
#include "vobject.h"
#include "vobject_private.h"
#include "video_private.h"
#include "wcheck.h"
#include "vobject.h"
#include "vobject_blitters.h"
#include "shading.h"

// GLOBAL: WIZ8 0x00600078
SGPRect ClippingRect = {0, 0, 640, 480};
//555      565
UINT32 guiTranslucentMask = 0x3def; //0x7bef;		// mask for halving 5,6,5

// GLOBALS for pre-calculating skip values
INT32 gLeftSkip, gRightSkip, gTopSkip, gBottomSkip;
BOOLEAN gfUsePreCalcSkips = FALSE;

//*Experimental**********************************************************************

/***********************************************************************************/

//** 8 Bit Blitters
//**

/**********************************************************************************************
 Blt8BPPDataTo8BPPBufferMonoShadowClip

	Uses a bitmap an 8BPP template for blitting. Anywhere a 1 appears in the bitmap, a shadow
	is blitted to the destination (a black pixel). Any other value above zero is considered a
	forground color, and zero is background. If the parameter for the background color is zero,
	transparency is used for the background.

	**********************************************************************************************/
// FUNCTION: WIZ8 0x00410750
BOOLEAN Blt8BPPDataTo8BPPBufferMonoShadowClip(UINT8* pBuffer, UINT32 uiDestPitchBYTES,
                                              HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                              UINT16 usIndex, SGPRect* clipregion,
                                              UINT8 ubForeground, UINT8 ubBackground)
{
    UINT32 uiOffset;
    UINT32 usHeight, usWidth, Unblitted;
    UINT8 *SrcPtr, *DestPtr;
    UINT32 LineSkip, LineSkipZ;
    ETRLEObject* pTrav;
    INT32 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
    INT32 ClipX1, ClipY1, ClipX2, ClipY2;
    UINT8* pPal8BPP;

    // Assertions
    Assert(hSrcVObject != NULL);
    Assert(pBuffer != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
    usHeight = (UINT32)pTrav->usHeight;
    usWidth = (UINT32)pTrav->usWidth;
    uiOffset = pTrav->uiDataOffset;

    // Add to start position of dest buffer
    iTempX = iX + pTrav->sOffsetX;
    iTempY = iY + pTrav->sOffsetY;

    if (clipregion == NULL) {
        ClipX1 = ClippingRect.iLeft;
        ClipY1 = ClippingRect.iTop;
        ClipX2 = ClippingRect.iRight;
        ClipY2 = ClippingRect.iBottom;
    } else {
        ClipX1 = clipregion->iLeft;
        ClipY1 = clipregion->iTop;
        ClipX2 = clipregion->iRight;
        ClipY2 = clipregion->iBottom;
    }

    // Calculate rows hanging off each side of the screen
    LeftSkip = __min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
    RightSkip = __min(max(ClipX2, (iTempX + (INT32)usWidth)) - ClipX2, (INT32)usWidth);
    TopSkip = __min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
    BottomSkip = __min(__max(ClipY2, (iTempY + (INT32)usHeight)) - ClipY2, (INT32)usHeight);

    // calculate the remaining rows and columns to blit
    BlitLength = ((INT32)usWidth - LeftSkip - RightSkip);
    BlitHeight = ((INT32)usHeight - TopSkip - BottomSkip);

    // check if whole thing is clipped
    if ((LeftSkip >= (INT32)usWidth) || (RightSkip >= (INT32)usWidth))
        return (TRUE);

    // check if whole thing is clipped
    if ((TopSkip >= (INT32)usHeight) || (BottomSkip >= (INT32)usHeight))
        return (TRUE);

    SrcPtr = (UINT8*)hSrcVObject->pPixData + uiOffset;
    DestPtr = (UINT8*)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip));
    LineSkip = (uiDestPitchBYTES - (BlitLength));
    LineSkipZ = LineSkip * 2;
    pPal8BPP = hSrcVObject->pShade8;

    __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		xor		eax, eax
		xor		ecx, ecx
		mov		edx, pPal8BPP

		cmp		TopSkip, 0 // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop: // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2 // if equal, skip whole, and start blit with new run
		jb		LSSkip1 // if less, skip whole thing

		add		esi, LSCount // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx // skip whole run, and start blit with new run
		jmp		BlitLineSetup

LSSkip1:
		add		esi, ecx // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop

LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup // if equal, skip whole, and start blit with new run
		jb		LSTrans1 // if less, skip whole thing

		sub		ecx, LSCount // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent

LSTrans1:
		sub		LSCount, ecx // skip whole run, continue skipping
		jmp		LeftSkipLoop

BlitLineSetup: // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0 // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop: // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:
		xor		eax, eax
		mov		al, [esi]
		cmp		al, 1
		jne		BlitNTL3

             // write shadow pixel
		xor		al, al
		mov		[edi], al
		jmp		BlitNTL2

BlitNTL3:
		or		al, al
		jz		BlitNTL4

             // write foreground pixel
		mov		al, ubForeground
		mov		[edi], al
		jmp		BlitNTL2

BlitNTL4:
		cmp		ubBackground, 0
		je		BlitNTL2

             //write background pixel
		mov		al, ubBackground
		mov		[edi], al

BlitNTL2:
		inc		esi
		inc		edi
		dec		cl
		jnz		BlitNTL1

             //BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent: // skip transparent pixels
		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:
		sub		LSCount, ecx

		mov		al, ubBackground
		or		al, al
		jz		BTrans2

		rep		stosb
		jmp		BlitDispatch

BTrans2:
		add		edi, ecx
		jmp		BlitDispatch

RightSkipLoop: // skip along until we hit and end-of-line marker

RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup

BlitDone:
    }

    return (TRUE);
}

/******************************************************************************
 Blt8BPPDataTo8BPPBufferTransparentClip

	Blits an image into the destination buffer, using an ETRLE brush as a source, and a 16-bit
	buffer as a destination. Clips the brush.

*******************************************************************************/
// FUNCTION: WIZ8 0x004109f0
BOOLEAN Blt8BPPDataTo8BPPBufferTransparentClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                               HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                               UINT16 usIndex, SGPRect* clipregion)
{
    UINT32 uiOffset;
    UINT32 usHeight, usWidth, Unblitted;
    UINT8 *SrcPtr, *DestPtr;
    UINT32 LineSkip;
    ETRLEObject* pTrav;
    INT32 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
    INT32 ClipX1, ClipY1, ClipX2, ClipY2;
    UINT8* pPal8BPP;

    // Assertions
    Assert(hSrcVObject != NULL);
    Assert(pBuffer != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
    usHeight = (UINT32)pTrav->usHeight;
    usWidth = (UINT32)pTrav->usWidth;
    uiOffset = pTrav->uiDataOffset;

    // Add to start position of dest buffer
    iTempX = iX + pTrav->sOffsetX;
    iTempY = iY + pTrav->sOffsetY;

    if (clipregion == NULL) {
        ClipX1 = ClippingRect.iLeft;
        ClipY1 = ClippingRect.iTop;
        ClipX2 = ClippingRect.iRight;
        ClipY2 = ClippingRect.iBottom;
    } else {
        ClipX1 = clipregion->iLeft;
        ClipY1 = clipregion->iTop;
        ClipX2 = clipregion->iRight;
        ClipY2 = clipregion->iBottom;
    }

    // Calculate rows hanging off each side of the screen
    LeftSkip = __min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
    RightSkip = __min(max(ClipX2, (iTempX + (INT32)usWidth)) - ClipX2, (INT32)usWidth);
    TopSkip = __min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
    BottomSkip = __min(__max(ClipY2, (iTempY + (INT32)usHeight)) - ClipY2, (INT32)usHeight);

    // calculate the remaining rows and columns to blit
    BlitLength = ((INT32)usWidth - LeftSkip - RightSkip);
    BlitHeight = ((INT32)usHeight - TopSkip - BottomSkip);

    // check if whole thing is clipped
    if ((LeftSkip >= (INT32)usWidth) || (RightSkip >= (INT32)usWidth))
        return (TRUE);

    // check if whole thing is clipped
    if ((TopSkip >= (INT32)usHeight) || (BottomSkip >= (INT32)usHeight))
        return (TRUE);

    SrcPtr = (UINT8*)hSrcVObject->pPixData + uiOffset;
    DestPtr = (UINT8*)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip));
    LineSkip = (uiDestPitchBYTES - (BlitLength));
    pPal8BPP = hSrcVObject->pShade8;

    __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, pPal8BPP
            //		mov		edx, pointer to shade table here
		xor		eax, eax
		mov		ebx, TopSkip
		xor		ecx, ecx

		or		ebx, ebx // check for nothing clipped on top
		jz		LeftSkipSetup

TopSkipLoop: // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		ebx
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		ebx, LeftSkip // check for nothing clipped on the left
		or		ebx, ebx
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, ebx
		je		LSSkip2 // if equal, skip whole, and start blit with new run
		jb		LSSkip1 // if less, skip whole thing

		add		esi, ebx // skip partial run, jump into normal loop for rest
		sub		ecx, ebx
		mov		ebx, BlitLength
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx // skip whole run, and start blit with new run
		jmp		BlitLineSetup

LSSkip1:
		add		esi, ecx // skip whole run, continue skipping
		sub		ebx, ecx
		jmp		LeftSkipLoop

LSTrans:
		and		ecx, 07fH
		cmp		ecx, ebx
		je		BlitLineSetup // if equal, skip whole, and start blit with new run
		jb		LSTrans1 // if less, skip whole thing

		sub		ecx, ebx // skip partial run, jump into normal loop for rest
		mov		ebx, BlitLength
		mov		Unblitted, 0
		jmp		BlitTransparent

LSTrans1:
		sub		ebx, ecx // skip whole run, continue skipping
		jmp		LeftSkipLoop

BlitLineSetup: // Does any actual blitting (trans/non) for the line
		mov		ebx, BlitLength
		mov		Unblitted, 0

BlitDispatch:

		or		ebx, ebx // Check to see if we're done blitting
		jz		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop: // blit non-transparent pixels

		cmp		ecx, ebx
		jbe		BNTrans1

		sub		ecx, ebx
		mov		Unblitted, ecx
		mov		ecx, ebx

BNTrans1:
		sub		ebx, ecx

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		al, [esi]
		mov		al, [edx+eax]
		mov		[edi], al
		inc		esi
		inc		edi

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		al, [esi]
		mov		al, [edx+eax]
		mov		[edi], al

		mov		al, [esi+1]
		mov		al, [edx+eax]
		mov		[edi+1], al

		add		esi, 2
		add		edi, 2

BlitNTL3:

		or		cl, cl
		jz		BlitLineEnd

BlitNTL4:

		mov		al, [esi]
		mov		al, [edx+eax]
		mov		[edi], al

		mov		al, [esi+1]
		mov		al, [edx+eax]
		mov		[edi+1], al

		mov		al, [esi+2]
		mov		al, [edx+eax]
		mov		[edi+2], al

		mov		al, [esi+3]
		mov		al, [edx+eax]
		mov		[edi+3], al

		add		esi, 4
		add		edi, 4

		dec		cl
		jnz		BlitNTL4

BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent: // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, ebx
		jbe		BTrans1

		mov		ecx, ebx

BTrans1:

		sub		ebx, ecx
		add		edi, ecx
		jmp		BlitDispatch

RightSkipLoop: // skip along until we hit and end-of-line marker

RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup

BlitDone:
    }

    return (TRUE);
}

/**********************************************************************************************
 Blt8BPPDataTo8BPPBufferTransparent

	Blits an image into the destination buffer, using an ETRLE brush as a source, and a 16-bit
	buffer as a destination.

**********************************************************************************************/
// FUNCTION: WIZ8 0x00410ca0
BOOLEAN Blt8BPPDataTo8BPPBufferTransparent(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                           HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex)
{
    UINT32 uiOffset;
    UINT32 usHeight, usWidth;
    UINT8 *SrcPtr, *DestPtr, *pPal8BPP;
    UINT32 LineSkip;
    ETRLEObject* pTrav;
    INT32 iTempX, iTempY;

    // Assertions
    Assert(hSrcVObject != NULL);
    Assert(pBuffer != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
    usHeight = (UINT32)pTrav->usHeight;
    usWidth = (UINT32)pTrav->usWidth;
    uiOffset = pTrav->uiDataOffset;

    // Add to start position of dest buffer
    iTempX = iX + pTrav->sOffsetX;
    iTempY = iY + pTrav->sOffsetY;

    // Validations
    CHECKF(iTempX >= 0);
    CHECKF(iTempY >= 0);

    SrcPtr = (UINT8*)hSrcVObject->pPixData + uiOffset;
    DestPtr = (UINT8*)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX);
    LineSkip = (uiDestPitchBYTES - (usWidth));
    pPal8BPP = hSrcVObject->pShade8;

    __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		xor		eax, eax
		xor		ebx, ebx
		xor		ecx, ecx
		mov		edx, pPal8BPP

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

            //BlitNonTransLoop:

		clc
		rcr		cl, 1
		jnc		BlitNTL2

            //		movsb

		mov		al, [esi]
		mov		al, [edx+eax]
		mov		[edi], al
		inc		esi
		inc		edi

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

             //		movsw

		mov		al, [esi]
		mov		al, [edx+eax]
		mov		[edi], al

		mov		al, [esi+1]
		mov		al, [edx+eax]
		mov		[edi+1], al

		add		esi, 2
		add		edi, 2

BlitNTL3:

		or		cl, cl
		jz		BlitDispatch

BlitNTL4:

        //		rep		movsd

		mov		al, [esi]
		mov		al, [edx+eax]
		mov		[edi], al

		mov		al, [esi+1]
		mov		al, [edx+eax]
		mov		[edi+1], al

		mov		al, [esi+2]
		mov		al, [edx+eax]
		mov		[edi+2], al

		mov		al, [esi+3]
		mov		al, [edx+eax]
		mov		[edi+3], al

		add		esi, 4
		add		edi, 4

		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
		add		edi, ecx
		jmp		BlitDispatch

BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		jmp		BlitDispatch

BlitDone:
    }

    return (TRUE);
}

/**********************************************************************************************
 Blt8BPPDataTo8BPPBufferShadow

	Modifies the destination buffer. Darkens the destination pixels by 25%, using the source
	image as a mask. Any Non-zero index pixels are used to darken destination pixels.

**********************************************************************************************/
// FUNCTION: WIZ8 0x00410db0
BOOLEAN Blt8BPPDataTo8BPPBufferShadow(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                      HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex)
{
    UINT8* pPal8BPP;
    UINT32 uiOffset;
    UINT32 usHeight, usWidth;
    UINT8 *SrcPtr, *DestPtr;
    UINT32 LineSkip;
    ETRLEObject* pTrav;
    INT32 iTempX, iTempY;

    // Assertions
    Assert(hSrcVObject != NULL);
    Assert(pBuffer != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
    usHeight = (UINT32)pTrav->usHeight;
    usWidth = (UINT32)pTrav->usWidth;
    uiOffset = pTrav->uiDataOffset;

    // Add to start position of dest buffer
    iTempX = iX + pTrav->sOffsetX;
    iTempY = iY + pTrav->sOffsetY;

    // Validations
    CHECKF(iTempX >= 0);
    CHECKF(iTempY >= 0);

    SrcPtr = (UINT8*)hSrcVObject->pPixData + uiOffset;
    DestPtr = (UINT8*)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX);
    pPal8BPP = hSrcVObject->pShade8;
    LineSkip = (uiDestPitchBYTES - (usWidth));

    __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		xor		eax, eax
		mov		ebx, usHeight
		xor		ecx, ecx
		mov		edx, OFFSET ShadeTable

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

            //BlitNonTransLoop:

		xor		eax, eax

		add		esi, ecx

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitDispatch

BlitNTL4:

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		mov		ax, [edi+4]
		mov		ax, [edx+eax*2]
		mov		[edi+4], ax

		mov		ax, [edi+6]
		mov		ax, [edx+eax*2]
		mov		[edi+6], ax

		add		edi, 8
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch

BlitDoneLine:

		dec		ebx
		jz		BlitDone
		add		edi, LineSkip
		jmp		BlitDispatch

BlitDone:
    }

    return (TRUE);
}

/**********************************************************************************************
 Blt8BPPDataTo8BPPBufferShadowClip

	Modifies the destination buffer. Darkens the destination pixels by 25%, using the source
	image as a mask. Any Non-zero index pixels are used to darken destination pixels. Blitter
	clips brush if it doesn't fit on the viewport.

**********************************************************************************************/
// FUNCTION: WIZ8 0x00410ed0
BOOLEAN Blt8BPPDataTo8BPPBufferShadowClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                          HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex,
                                          SGPRect* clipregion)
{
    UINT8* pPal8BPP;
    UINT32 uiOffset;
    UINT32 usHeight, usWidth, Unblitted;
    UINT8 *SrcPtr, *DestPtr;
    UINT32 LineSkip;
    ETRLEObject* pTrav;
    INT32 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
    INT32 ClipX1, ClipY1, ClipX2, ClipY2;

    // Assertions
    Assert(hSrcVObject != NULL);
    Assert(pBuffer != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
    usHeight = (UINT32)pTrav->usHeight;
    usWidth = (UINT32)pTrav->usWidth;
    uiOffset = pTrav->uiDataOffset;

    // Add to start position of dest buffer
    iTempX = iX + pTrav->sOffsetX;
    iTempY = iY + pTrav->sOffsetY;

    if (clipregion == NULL) {
        ClipX1 = ClippingRect.iLeft;
        ClipY1 = ClippingRect.iTop;
        ClipX2 = ClippingRect.iRight;
        ClipY2 = ClippingRect.iBottom;
    } else {
        ClipX1 = clipregion->iLeft;
        ClipY1 = clipregion->iTop;
        ClipX2 = clipregion->iRight;
        ClipY2 = clipregion->iBottom;
    }

    // Calculate rows hanging off each side of the screen
    LeftSkip = __min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
    RightSkip = __min(max(ClipX2, (iTempX + (INT32)usWidth)) - ClipX2, (INT32)usWidth);
    TopSkip = __min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
    BottomSkip = __min(__max(ClipY2, (iTempY + (INT32)usHeight)) - ClipY2, (INT32)usHeight);

    // calculate the remaining rows and columns to blit
    BlitLength = ((INT32)usWidth - LeftSkip - RightSkip);
    BlitHeight = ((INT32)usHeight - TopSkip - BottomSkip);

    // whole thing is clipped
    if ((LeftSkip >= (INT32)usWidth) || (RightSkip >= (INT32)usWidth))
        return (TRUE);

    // whole thing is clipped
    if ((TopSkip >= (INT32)usHeight) || (BottomSkip >= (INT32)usHeight))
        return (TRUE);

    SrcPtr = (UINT8*)hSrcVObject->pPixData + uiOffset;
    DestPtr = (UINT8*)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip));
    pPal8BPP = hSrcVObject->pShade8;
    LineSkip = (uiDestPitchBYTES - (BlitLength));

    __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, pPal8BPP
		xor		eax, eax
		mov		ebx, TopSkip
		xor		ecx, ecx

		or		ebx, ebx // check for nothing clipped on top
		jz		LeftSkipSetup

TopSkipLoop: // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		ebx
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		ebx, LeftSkip // check for nothing clipped on the left
		or		ebx, ebx
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, ebx
		je		LSSkip2 // if equal, skip whole, and start blit with new run
		jb		LSSkip1 // if less, skip whole thing

		add		esi, ebx // skip partial run, jump into normal loop for rest
		sub		ecx, ebx
		mov		ebx, BlitLength
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx // skip whole run, and start blit with new run
		jmp		BlitLineSetup

LSSkip1:
		add		esi, ecx // skip whole run, continue skipping
		sub		ebx, ecx
		jmp		LeftSkipLoop

LSTrans:
		and		ecx, 07fH
		cmp		ecx, ebx
		je		BlitLineSetup // if equal, skip whole, and start blit with new run
		jb		LSTrans1 // if less, skip whole thing

		sub		ecx, ebx // skip partial run, jump into normal loop for rest
		mov		ebx, BlitLength
		jmp		BlitTransparent

LSTrans1:
		sub		ebx, ecx // skip whole run, continue skipping
		jmp		LeftSkipLoop

BlitLineSetup: // Does any actual blitting (trans/non) for the line
		mov		ebx, BlitLength
		mov		Unblitted, 0

BlitDispatch:

		or		ebx, ebx // Check to see if we're done blitting
		jz		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:

		cmp		ecx, ebx
		jbe		BNTrans1

		sub		ecx, ebx
		mov		Unblitted, ecx
		mov		ecx, ebx

BNTrans1:
		sub		ebx, ecx

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		ax, [edi]
		mov		ax, [edx+eax]
		mov		[edi], ax

		inc		esi
		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		ax, [edi]
		mov		ax, [edx+eax]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax]
		mov		[edi+2], ax

		add		esi, 2
		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitLineEnd

BlitNTL4:

		mov		ax, [edi]
		mov		ax, [edx+eax]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax]
		mov		[edi+2], ax

		mov		ax, [edi+4]
		mov		ax, [edx+eax]
		mov		[edi+4], ax

		mov		ax, [edi+6]
		mov		ax, [edx+eax]
		mov		[edi+6], ax

		add		esi, 4
		add		edi, 8
		dec		cl
		jnz		BlitNTL4

BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
		cmp		ecx, ebx
		jbe		BTrans1

		mov		ecx, ebx

BTrans1:

		sub		ebx, ecx
            //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch

RightSkipLoop:

RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup

BlitDone:
    }

    return (TRUE);
}

//** 16 Bit Blitters
//**

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferMonoShadowClip

	Uses a bitmap an 8BPP template for blitting. Anywhere a 1 appears in the bitmap, a shadow
	is blitted to the destination (a black pixel). Any other value above zero is considered a
	forground color, and zero is background. If the parameter for the background color is zero,
	transparency is used for the background.

**********************************************************************************************/
// FUNCTION: WIZ8 0x00411190
BOOLEAN Blt8BPPDataTo16BPPBufferMonoShadowClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                               HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                               UINT16 usIndex, SGPRect* clipregion,
                                               UINT16 usForeground, UINT16 usBackground,
                                               UINT16 usShadow)
{
    UINT32 uiOffset;
    UINT32 usHeight, usWidth, Unblitted;
    UINT8 *SrcPtr, *DestPtr;
    UINT32 LineSkip;
    ETRLEObject* pTrav;
    INT32 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
    INT32 ClipX1, ClipY1, ClipX2, ClipY2;

    // Assertions
    Assert(hSrcVObject != NULL);
    Assert(pBuffer != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
    usHeight = (UINT32)pTrav->usHeight;
    usWidth = (UINT32)pTrav->usWidth;
    uiOffset = pTrav->uiDataOffset;

    // Add to start position of dest buffer
    iTempX = iX + pTrav->sOffsetX;
    iTempY = iY + pTrav->sOffsetY;

    if (clipregion == NULL) {
        ClipX1 = ClippingRect.iLeft;
        ClipY1 = ClippingRect.iTop;
        ClipX2 = ClippingRect.iRight;
        ClipY2 = ClippingRect.iBottom;
    } else {
        ClipX1 = clipregion->iLeft;
        ClipY1 = clipregion->iTop;
        ClipX2 = clipregion->iRight;
        ClipY2 = clipregion->iBottom;
    }

    // Calculate rows hanging off each side of the screen
    LeftSkip = __min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
    RightSkip = __min(max(ClipX2, (iTempX + (INT32)usWidth)) - ClipX2, (INT32)usWidth);
    TopSkip = __min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
    BottomSkip = __min(__max(ClipY2, (iTempY + (INT32)usHeight)) - ClipY2, (INT32)usHeight);

    // calculate the remaining rows and columns to blit
    BlitLength = ((INT32)usWidth - LeftSkip - RightSkip);
    BlitHeight = ((INT32)usHeight - TopSkip - BottomSkip);

    // check if whole thing is clipped
    if ((LeftSkip >= (INT32)usWidth) || (RightSkip >= (INT32)usWidth))
        return (TRUE);

    // check if whole thing is clipped
    if ((TopSkip >= (INT32)usHeight) || (BottomSkip >= (INT32)usHeight))
        return (TRUE);

    SrcPtr = (UINT8*)hSrcVObject->pPixData + uiOffset;
    DestPtr = (UINT8*)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
    LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

    __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		xor		eax, eax
		xor		ecx, ecx

		cmp		TopSkip, 0 // check for nothing clipped on top
		je		LeftSkipSetup

TopSkipLoop: // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		eax, LeftSkip
		mov		LSCount, eax
		or		eax, eax
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2 // if equal, skip whole, and start blit with new run
		jb		LSSkip1 // if less, skip whole thing

		add		esi, LSCount // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx // skip whole run, and start blit with new run
		jmp		BlitLineSetup

LSSkip1:
		add		esi, ecx // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop

LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup // if equal, skip whole, and start blit with new run
		jb		LSTrans1 // if less, skip whole thing

		sub		ecx, LSCount // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent

LSTrans1:
		sub		LSCount, ecx // skip whole run, continue skipping
		jmp		LeftSkipLoop

BlitLineSetup: // Does any actual blitting (trans/non) for the line
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0 // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop: // blit non-transparent pixels

		cmp		ecx, LSCount
		jbe		BNTrans1

		sub		ecx, LSCount
		mov		Unblitted, ecx
		mov		ecx, LSCount

BNTrans1:
		sub		LSCount, ecx

BlitNTL1:
		xor		eax, eax
		mov		al, [esi]
		cmp		al, 1
		jne		BlitNTL3

             // write shadow pixel
		mov		ax, usShadow

             // only write if not zero
		cmp		ax, 0
		je		BlitNTL2

		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL3:
		or		al, al
		jz		BlitNTL4

             // write foreground pixel
		mov		ax, usForeground
		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL4:
		cmp		usBackground, 0
		je		BlitNTL2

		mov		ax, usBackground
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		dec		cl
		jnz		BlitNTL1

             //BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent: // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, LSCount
		jbe		BTrans1

		mov		ecx, LSCount

BTrans1:
		sub		LSCount, ecx

		mov		ax, usBackground
		or		ax, ax
		jz		BTrans2

		rep		stosw
		jmp		BlitDispatch

BTrans2:
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch

RightSkipLoop: // skip along until we hit and end-of-line marker

RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup

BlitDone:
    }

    return (TRUE);
}

/**********************************************************************************************
	Blt16BPPTo16BPP

	Copies a rect of 16 bit data from a video buffer to a buffer position of the brush
	in the data area, for later blitting. Used to copy background information for mercs
	etc. to their unblit buffer, for later reblitting. Does NOT clip.

**********************************************************************************************/
// FUNCTION: WIZ8 0x00411430
BOOLEAN Blt16BPPTo16BPP(UINT16* pDest, UINT32 uiDestPitch, UINT16* pSrc, UINT32 uiSrcPitch,
                        INT32 iDestXPos, INT32 iDestYPos, INT32 iSrcXPos, INT32 iSrcYPos,
                        UINT32 uiWidth, UINT32 uiHeight)
{
    UINT16 *pSrcPtr, *pDestPtr;
    UINT32 uiLineSkipDest, uiLineSkipSrc;

    Assert(pDest != NULL);
    Assert(pSrc != NULL);

    pSrcPtr = (UINT16*)((UINT8*)pSrc + (iSrcYPos * uiSrcPitch) + (iSrcXPos * 2));
    pDestPtr = (UINT16*)((UINT8*)pDest + (iDestYPos * uiDestPitch) + (iDestXPos * 2));
    uiLineSkipDest = uiDestPitch - (uiWidth * 2);
    uiLineSkipSrc = uiSrcPitch - (uiWidth * 2);

    __asm {
	mov		esi, pSrcPtr
	mov		edi, pDestPtr
	mov		ebx, uiHeight
	cld

	mov		ecx, uiWidth
	test	ecx, 1
	jz		BlitDwords

BlitNewLine:

	mov		ecx, uiWidth
	shr		ecx, 1
	movsw

            //BlitNL2:

	rep		movsd

	add		edi, uiLineSkipDest
	add		esi, uiLineSkipSrc
	dec		ebx
	jnz		BlitNewLine

	jmp		BlitDone

BlitDwords:
	mov		ecx, uiWidth
	shr		ecx, 1
	rep		movsd

	add		edi, uiLineSkipDest
	add		esi, uiLineSkipSrc
	dec		ebx
	jnz		BlitDwords

BlitDone:

    }

    return (TRUE);
}

/**********************************************************************************************
	Blt16BPPTo16BPPTrans

	Copies a rect of 16 bit data from a video buffer to a buffer position of the brush
	in the data area, for later blitting. Used to copy background information for mercs
	etc. to their unblit buffer, for later reblitting. Does NOT clip. Transparent color is
	not copied.

**********************************************************************************************/
// FUNCTION: WIZ8 0x004114c0
BOOLEAN Blt16BPPTo16BPPTrans(UINT16* pDest, UINT32 uiDestPitch, UINT16* pSrc, UINT32 uiSrcPitch,
                             INT32 iDestXPos, INT32 iDestYPos, INT32 iSrcXPos, INT32 iSrcYPos,
                             UINT32 uiWidth, UINT32 uiHeight, UINT16 usTrans)
{
    UINT16 *pSrcPtr, *pDestPtr;
    UINT32 uiLineSkipDest, uiLineSkipSrc;

    Assert(pDest != NULL);
    Assert(pSrc != NULL);

    pSrcPtr = (UINT16*)((UINT8*)pSrc + (iSrcYPos * uiSrcPitch) + (iSrcXPos * 2));
    pDestPtr = (UINT16*)((UINT8*)pDest + (iDestYPos * uiDestPitch) + (iDestXPos * 2));
    uiLineSkipDest = uiDestPitch - (uiWidth * 2);
    uiLineSkipSrc = uiSrcPitch - (uiWidth * 2);

    __asm {
	mov		esi, pSrcPtr
	mov		edi, pDestPtr
	mov		ebx, uiHeight
	mov		dx, usTrans

BlitNewLine:
	mov		ecx, uiWidth

Blit2:
	mov		ax, [esi]
	cmp		ax, dx
	je		Blit3

	mov		[edi], ax

Blit3:
	add		esi, 2
	add		edi, 2
	dec		ecx
	jnz		Blit2

	add		edi, uiLineSkipDest
	add		esi, uiLineSkipSrc
	dec		ebx
	jnz		BlitNewLine

    }

    return (TRUE);
}

/**********************************************************************************************
	Blt16BPPTo16BPPMirror

	Copies a rect of 16 bit data from a video buffer to a buffer position of the brush
	in the data area, for later blitting. Used to copy background information for mercs
	etc. to their unblit buffer, for later reblitting. Does NOT clip.

**********************************************************************************************/
// FUNCTION: WIZ8 0x00411540
BOOLEAN Blt16BPPTo16BPPMirror(UINT16* pDest, UINT32 uiDestPitch, UINT16* pSrc, UINT32 uiSrcPitch,
                              INT32 iDestXPos, INT32 iDestYPos, INT32 iSrcXPos, INT32 iSrcYPos,
                              UINT32 uiWidth, UINT32 uiHeight)
{
    UINT16 *pSrcPtr, *pDestPtr;
    UINT32 uiLineSkipDest, uiLineSkipSrc;
    INT32 RightSkip, LeftSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
    INT32 iTempX, iTempY, ClipX1, ClipY1, ClipX2, ClipY2;
    SGPRect* clipregion = NULL;

    Assert(pDest != NULL);
    Assert(pSrc != NULL);

    // Add to start position of dest buffer
    iTempX = iDestXPos;
    iTempY = iDestYPos;

    if (clipregion == NULL) {
        ClipX1 = 0;   //ClippingRect.iLeft;
        ClipY1 = 0;   //ClippingRect.iTop;
        ClipX2 = 640; //ClippingRect.iRight;
        ClipY2 = 480; //ClippingRect.iBottom;
    } else {
        ClipX1 = clipregion->iLeft;
        ClipY1 = clipregion->iTop;
        ClipX2 = clipregion->iRight;
        ClipY2 = clipregion->iBottom;
    }

    // Calculate rows hanging off each side of the screen
    LeftSkip = __min(ClipX1 - __min(ClipX1, iTempX), (INT32)uiWidth);
    RightSkip = __min(__max(ClipX2, (iTempX + (INT32)uiWidth)) - ClipX2, (INT32)uiWidth);
    TopSkip = __min(ClipY1 - __min(ClipY1, iTempY), (INT32)uiHeight);
    BottomSkip = __min(__max(ClipY2, (iTempY + (INT32)uiHeight)) - ClipY2, (INT32)uiHeight);

    iTempX = __max(ClipX1, iDestXPos);
    iTempY = __max(ClipY1, iDestYPos);

    // calculate the remaining rows and columns to blit
    BlitLength = ((INT32)uiWidth - LeftSkip - RightSkip);
    BlitHeight = ((INT32)uiHeight - TopSkip - BottomSkip);

    // check if whole thing is clipped
    if ((LeftSkip >= (INT32)uiWidth) || (RightSkip >= (INT32)uiWidth))
        return (TRUE);

    // check if whole thing is clipped
    if ((TopSkip >= (INT32)uiHeight) || (BottomSkip >= (INT32)uiHeight))
        return (TRUE);

    pSrcPtr = (UINT16*)((UINT8*)pSrc + (TopSkip * uiSrcPitch) + (RightSkip * 2));
    pDestPtr =
        (UINT16*)((UINT8*)pDest + (iTempY * uiDestPitch) + (iTempX * 2) + ((BlitLength - 1) * 2));
    uiLineSkipDest = uiDestPitch; //+((BlitLength-1)*2);
    uiLineSkipSrc = uiSrcPitch - (BlitLength * 2);

    __asm {
	mov		esi, pSrcPtr
	mov		edi, pDestPtr
	mov		ebx, BlitHeight

BlitNewLine:

	mov		ecx, BlitLength
            //add   edi, ecx
            //add   edi, ecx

BlitNTL2:

	mov		ax, [esi]
	mov		[edi], ax
	inc		esi
	dec		edi
	inc		esi
	dec		edi
	dec		ecx
	jnz		BlitNTL2

	add		edi, BlitLength
	add		esi, uiLineSkipSrc
	add		edi, BlitLength
	add		edi, uiLineSkipDest
	dec		ebx
	jnz		BlitNewLine

    }

    return (TRUE);
}

/***********************************************************************************************
	Blt8BPPTo8BPP

	Copies a rect of an 8 bit data from a video buffer to a buffer position of the brush
	in the data area, for later blitting. Used to copy background information for mercs
	etc. to their unblit buffer, for later reblitting. Does NOT clip.

**********************************************************************************************/
// FUNCTION: WIZ8 0x004116c0
BOOLEAN Blt8BPPTo8BPP(UINT8* pDest, UINT32 uiDestPitch, UINT8* pSrc, UINT32 uiSrcPitch,
                      INT32 iDestXPos, INT32 iDestYPos, INT32 iSrcXPos, INT32 iSrcYPos,
                      UINT32 uiWidth, UINT32 uiHeight)
{
    UINT8 *pSrcPtr, *pDestPtr;
    UINT32 uiLineSkipDest, uiLineSkipSrc;

    Assert(pDest != NULL);
    Assert(pSrc != NULL);

    pSrcPtr = pSrc + (iSrcYPos * uiSrcPitch) + (iSrcXPos);
    pDestPtr = pDest + (iDestYPos * uiDestPitch) + (iDestXPos);
    uiLineSkipDest = uiDestPitch - (uiWidth);
    uiLineSkipSrc = uiSrcPitch - (uiWidth);

    __asm {
	mov		esi, pSrcPtr
	mov		edi, pDestPtr
	mov		ebx, uiHeight
	cld

BlitNewLine:
	mov		ecx, uiWidth

	clc
	rcr		ecx, 1
	jnc		Blit2
	movsb

Blit2:
	clc
	rcr		ecx, 1
	jnc		Blit3

	movsw

Blit3:
	or		ecx, ecx
	jz		BlitLineDone

	rep		movsd

BlitLineDone:

	add		edi, uiLineSkipDest
	add		esi, uiLineSkipSrc
	dec		ebx
	jnz		BlitNewLine

    }

    return (TRUE);
}

#if 0

BlitNTL4:

		// TEST FOR Z FIRST!
		mov		ax, [ebx]
		cmp		ax, usZValue
		ja		BlitNTL8

		// Write it NOW!
		jmp		BlitNTL7

BlitNTL8:

		test	uiLineFlag, 1
		jz		BlitNTL6

		test	edi, 2
		jz		BlitNTL5
		jmp		BlitNTL9

BlitNTL6:
		test	edi, 2
		jnz		BlitNTL5

BlitNTL7:

		// Write normal z value
		mov		ax, usZValue
		mov		[ebx], ax
		jmp   BlitNTL10

BlitNTL9:

		// Write high z
		mov		ax, 32767
		mov		[ebx], ax

BlitNTL10:

		xor		eax, eax
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax
#endif

/**********************************************************************************************
 Blt8BPPDataSubTo16BPPBuffer

	Blits a subrect from a flat 8 bit surface to a 16-bit buffer.

**********************************************************************************************/
// FUNCTION: WIZ8 0x00411730
BOOLEAN Blt8BPPDataSubTo16BPPBuffer(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                    HVSURFACE hSrcVSurface, UINT8* pSrcBuffer, UINT32 uiSrcPitch,
                                    INT32 iX, INT32 iY, SGPRect* pRect)
{
    UINT16* p16BPPPalette;
    UINT32 usHeight, usWidth;
    UINT8 *SrcPtr, *DestPtr;
    UINT32 LineSkip, LeftSkip, RightSkip, TopSkip, BlitLength, SrcSkip, BlitHeight;
    INT32 iTempX, iTempY;

    // Assertions
    Assert(hSrcVSurface != NULL);
    Assert(pSrcBuffer != NULL);
    Assert(pBuffer != NULL);

    // Get Offsets from Index into structure
    usHeight = (UINT32)hSrcVSurface->usHeight;
    usWidth = (UINT32)hSrcVSurface->usWidth;

    // Add to start position of dest buffer
    iTempX = iX;
    iTempY = iY;

    // Validations
    CHECKF(iTempX >= 0);
    CHECKF(iTempY >= 0);

    LeftSkip = pRect->iLeft;
    RightSkip = usWidth - pRect->iRight;
    TopSkip = pRect->iTop * uiSrcPitch;
    BlitLength = pRect->iRight - pRect->iLeft;
    BlitHeight = pRect->iBottom - pRect->iTop;
    SrcSkip = uiSrcPitch - BlitLength;

    SrcPtr = (UINT8*)(pSrcBuffer + TopSkip + LeftSkip);
    DestPtr = ((UINT8*)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2));
    p16BPPPalette = hSrcVSurface->p16BPPPalette;
    LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

    __asm {

		mov		esi, SrcPtr // pointer to current line start address in source
		mov		edi, DestPtr // pointer to current line start address in destination
		mov		ebx, BlitHeight // line counter (goes top to bottom)
		mov		edx, p16BPPPalette // conversion table

		sub		eax, eax
		sub		ecx, ecx

NewRow:
		mov		ecx, BlitLength // pixels to blit count

BlitLoop:
		mov		al, [esi]
		xor		ah, ah

		shl		eax, 1 // make it into a word index
		mov		ax, [edx+eax] // get 16-bit version of 8-bit pixel
		mov		[edi], ax // store it in destination buffer

		inc		edi
		inc		esi
		inc		edi
		dec		ecx
		jnz		BlitLoop

		add		esi, SrcSkip // move line pointers down one line
		add		edi, LineSkip

		dec		ebx // check line counter
		jnz		NewRow // done blitting, exit

        //DoneBlit:											// finished blit
    }

    return (TRUE);
}

/****************************INCOMPLETE***********************************************/

// FUNCTION: WIZ8 0x004117f0
void SetClippingRect(SGPRect* clip)
{
    Assert(clip != NULL);
    Assert(clip->iLeft < clip->iRight);
    Assert(clip->iTop < clip->iBottom);

    memcpy(&ClippingRect, clip, sizeof(SGPRect));
}

// FUNCTION: WIZ8 0x00411820
void GetClippingRect(SGPRect* clip)
{
    Assert(clip != NULL);

    memcpy(clip, &ClippingRect, sizeof(SGPRect));
}

/**********************************************************************************************
	Blt16BPPBufferPixelateRectWithColor

		Given an 8x8 pattern and a color, pixelates an area by repeatedly "applying the color" to pixels whereever there
		is a non-zero value in the pattern.

		KM:  Added Nov. 23, 1998
		This is all the code that I moved from Blt16BPPBufferPixelateRect().
		This function now takes a color field (which previously was
		always black.  The 3rd assembler line in this function:

				mov		ax, usColor				// color of pixel

		used to be:

				xor   eax, eax					// color of pixel (black or 0)

	  This was the only internal modification I made other than adding the usColor argument.

*********************************************************************************************/
// FUNCTION: WIZ8 0x00411850
BOOLEAN Blt16BPPBufferPixelateRectWithColor(UINT16* pBuffer, UINT32 uiDestPitchBYTES, SGPRect* area,
                                            UINT8 Pattern[8][8], UINT16 usColor)
{
    INT32 width, height;
    UINT32 LineSkip;
    UINT16* DestPtr;
    INT32 iLeft, iTop, iRight, iBottom;

    // Assertions
    Assert(pBuffer != NULL);
    Assert(Pattern != NULL);

    iLeft = __max(ClippingRect.iLeft, area->iLeft);
    iTop = __max(ClippingRect.iTop, area->iTop);
    iRight = __min(ClippingRect.iRight - 1, area->iRight);
    iBottom = __min(ClippingRect.iBottom - 1, area->iBottom);

    DestPtr = (pBuffer + (iTop * (uiDestPitchBYTES / 2)) + iLeft);
    width = iRight - iLeft + 1;
    height = iBottom - iTop + 1;
    LineSkip = (uiDestPitchBYTES - (width * 2));

    CHECKF(width >= 1);
    CHECKF(height >= 1);

    __asm {
		mov		esi, Pattern // Pointer to pixel pattern
		mov		edi, DestPtr // Pointer to top left of rect area
		mov		ax, usColor // color of pixel
		xor		ebx, ebx // pattern column index
		xor		edx, edx // pattern row index

BlitNewLine:
		mov		ecx, width

BlitLine:
		cmp	byte ptr [esi+ebx], 0
		je	BlitLine2

		mov		[edi], ax

BlitLine2:
		add		edi, 2
		inc		ebx
		and		ebx, 07H
		or		ebx, edx
		dec		ecx
		jnz		BlitLine

		add		edi, LineSkip
		xor		ebx, ebx
		add		edx, 08H
		and		edx, 38H
		dec		height
		jnz		BlitNewLine
    }

    return (TRUE);
}

//Uses black hatch color
// FUNCTION: WIZ8 0x00411930
BOOLEAN Blt16BPPBufferHatchRect(UINT16* pBuffer, UINT32 uiDestPitchBYTES, SGPRect* area)
{
    UINT8 Pattern[8][8] = {1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0,
                           1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1,
                           0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1};
    return Blt16BPPBufferPixelateRectWithColor(pBuffer, uiDestPitchBYTES, area, Pattern, 0);
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferShadow

	Modifies the destination buffer. Darkens the destination pixels by 25%, using the source
	image as a mask. Any Non-zero index pixels are used to darken destination pixels.

**********************************************************************************************/
// FUNCTION: WIZ8 0x00411a60
BOOLEAN Blt8BPPDataTo16BPPBufferShadow(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                       HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex)
{
    UINT16* p16BPPPalette;
    UINT32 uiOffset;
    UINT32 usHeight, usWidth;
    UINT8 *SrcPtr, *DestPtr;
    UINT32 LineSkip;
    ETRLEObject* pTrav;
    INT32 iTempX, iTempY;

    // Assertions
    Assert(hSrcVObject != NULL);
    Assert(pBuffer != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
    usHeight = (UINT32)pTrav->usHeight;
    usWidth = (UINT32)pTrav->usWidth;
    uiOffset = pTrav->uiDataOffset;

    // Add to start position of dest buffer
    iTempX = iX + pTrav->sOffsetX;
    iTempY = iY + pTrav->sOffsetY;

    // Validations
    CHECKF(iTempX >= 0);
    CHECKF(iTempY >= 0);

    SrcPtr = (UINT8*)hSrcVObject->pPixData + uiOffset;
    DestPtr = (UINT8*)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
    p16BPPPalette = hSrcVObject->pShadeCurrent;
    LineSkip = (uiDestPitchBYTES - (usWidth * 2));

    __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		xor		eax, eax
		mov		ebx, usHeight
		xor		ecx, ecx
		mov		edx, OFFSET ShadeTable

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

            //BlitNonTransLoop:

		xor		eax, eax

		add		esi, ecx

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitDispatch

BlitNTL4:

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		mov		ax, [edi+4]
		mov		ax, [edx+eax*2]
		mov		[edi+4], ax

		mov		ax, [edi+6]
		mov		ax, [edx+eax*2]
		mov		[edi+6], ax

		add		edi, 8
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch

BlitDoneLine:

		dec		ebx
		jz		BlitDone
		add		edi, LineSkip
		jmp		BlitDispatch

BlitDone:
    }

    return (TRUE);
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransparent

	Blits an image into the destination buffer, using an ETRLE brush as a source, and a 16-bit
	buffer as a destination.

**********************************************************************************************/

// FUNCTION: WIZ8 0x00411b80
BOOLEAN Blt8BPPDataTo16BPPBufferTransparent(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                            HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                            UINT16 usIndex)
{
    UINT16* p16BPPPalette;
    UINT32 uiOffset;
    UINT32 usHeight, usWidth;
    UINT8 *SrcPtr, *DestPtr;
    UINT32 LineSkip;
    ETRLEObject* pTrav;
    INT32 iTempX, iTempY;

    // Assertions
    Assert(hSrcVObject != NULL);
    Assert(pBuffer != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
    usHeight = (UINT32)pTrav->usHeight;
    usWidth = (UINT32)pTrav->usWidth;
    uiOffset = pTrav->uiDataOffset;

    // Add to start position of dest buffer
    iTempX = iX + pTrav->sOffsetX;
    iTempY = iY + pTrav->sOffsetY;

    // Validations
    CHECKF(iTempX >= 0);
    CHECKF(iTempY >= 0);

    SrcPtr = (UINT8*)hSrcVObject->pPixData + uiOffset;
    DestPtr = (UINT8*)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
    p16BPPPalette = hSrcVObject->pShadeCurrent;
    LineSkip = (uiDestPitchBYTES - (usWidth * 2));

    __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		xor		ebx, ebx
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

            //BlitNonTransLoop:

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		bl, [esi]
		mov		ax, [edx+ebx*2]
		mov		[edi], ax

		inc		esi
		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		bl, [esi]
		mov		ax, [edx+ebx*2]
		mov		[edi], ax

		mov		bl, [esi+1]
		mov		ax, [edx+ebx*2]
		mov		[edi+2], ax

		add		esi, 2
		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitDispatch

		xor		ebx, ebx

BlitNTL4:

		mov		bl, [esi]
		mov		ax, [edx+ebx*2]
		mov		[edi], ax

		mov		bl, [esi+1]
		mov		ax, [edx+ebx*2]
		mov		[edi+2], ax

		mov		bl, [esi+2]
		mov		ax, [edx+ebx*2]
		mov		[edi+4], ax

		mov		bl, [esi+3]
		mov		ax, [edx+ebx*2]
		mov		[edi+6], ax

		add		esi, 4
		add		edi, 8
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
        //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch

BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		jmp		BlitDispatch

BlitDone:
    }

    return (TRUE);
}

// Blt8BPPDataTo16BPPBufferTransMirror
// Blits an 8bpp ETRLE to a 16-bit buffer, mirroring the image, with transparency.
// Returns BOOLEAN            - TRUE if successful
//  UINT16 *pBuffer           - 16bpp Destination buffer
// UINT32 uiDestPitchBYTES    - Destination pitch in bytes
// HVOBJECT hSrcVObject       - Source VOBJECT handle
// INT32 iX                   - X-location of blit
// INT32 iY                   - Y-location of blit
// UINT16 usIndex             - VOBJECT image index to blit from
// Created:  7/28/99 Derek Beland

// FUNCTION: WIZ8 0x00411cb0
BOOLEAN Blt8BPPDataTo16BPPBufferTransMirror(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                            HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                            UINT16 usIndex)
{
    UINT16* p16BPPPalette;
    UINT32 uiOffset;
    UINT32 usHeight, usWidth;
    UINT8 *SrcPtr, *DestPtr;
    UINT32 uiDestSkip;
    ETRLEObject* pTrav;
    INT32 iTempX, iTempY;

    // Assertions
    Assert(hSrcVObject != NULL);
    Assert(pBuffer != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
    usHeight = (UINT32)pTrav->usHeight;
    usWidth = (UINT32)pTrav->usWidth;
    uiOffset = pTrav->uiDataOffset;

    // Add to start position of dest buffer
    //	iTempX = iX + pTrav->sOffsetX;
    iTempX = iX + usWidth - pTrav->sOffsetX - 1;
    iTempY = iY + pTrav->sOffsetY;

    // Validations
    CHECKF(iTempX >= 0);
    CHECKF(iTempY >= 0);

    SrcPtr = (UINT8*)hSrcVObject->pPixData + uiOffset;
    DestPtr = (UINT8*)pBuffer + (uiDestPitchBYTES * iTempY) + (iTempX * 2);
    p16BPPPalette = hSrcVObject->pShadeCurrent;
    uiDestSkip = (uiDestPitchBYTES + (usWidth * 2));

    __asm {
        // esi = pointer to source data
        // edi = pointer to destination buffer
        // eax = 16bpp pixel
        // ebx = 8bpp pixel
        // ecx = repeat count
        // edx = pointer to 8->16bpp conversion table

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		xor		ebx, ebx
		xor		ecx, ecx

BlitDispatch:

        // pick up a new byte
		mov		cl, [esi]
		inc		esi
		or		cl, cl
            // if bit 7 is set, the run is transparent
		js		BlitTransparent
                // if the byte is zero, it marks the end of current line
		jz		BlitDoneLine

                    //BlitNonTransLoop:

                    // else we have a normal run of non-transparent bytes
                    // blit one byte of the count
		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		bl, [esi]
		mov		ax, [edx+ebx*2]
		mov		[edi], ax

		inc		esi
		sub		edi, 2

        // blit one word of the count
BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		bl, [esi]
		mov		ax, [edx+ebx*2]
		mov		[edi], ax

		mov		bl, [esi+1]
		mov		ax, [edx+ebx*2]
		mov		[edi-2], ax

		add		esi, 2
		sub		edi, 4

         // blit the rest four at a time (unrolled loop)
BlitNTL3:

		or		cl, cl
		jz		BlitDispatch

		xor		ebx, ebx

BlitNTL4:

		mov		bl, [esi]
		mov		ax, [edx+ebx*2]
		mov		[edi], ax

		mov		bl, [esi+1]
		mov		ax, [edx+ebx*2]
		mov		[edi-2], ax

		mov		bl, [esi+2]
		mov		ax, [edx+ebx*2]
		mov		[edi-4], ax

		mov		bl, [esi+3]
		mov		ax, [edx+ebx*2]
		mov		[edi-6], ax

		add		esi, 4
		sub		edi, 8
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
        //		shl		ecx, 1
		add   ecx, ecx
		sub		edi, ecx
		jmp		BlitDispatch

BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, uiDestSkip
		jmp		BlitDispatch

BlitDone:
    }

    return (TRUE);
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransparentClip

	Blits an image into the destination buffer, using an ETRLE brush as a source, and a 16-bit
	buffer as a destination. Clips the brush.

**********************************************************************************************/
// FUNCTION: WIZ8 0x00411de0
BOOLEAN Blt8BPPDataTo16BPPBufferTransparentClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                                HVOBJECT hSrcVObject, INT32 iX, INT32 iY,
                                                UINT16 usIndex, SGPRect* clipregion)
{
    UINT16* p16BPPPalette;
    UINT32 uiOffset;
    UINT32 usHeight, usWidth, Unblitted;
    UINT8 *SrcPtr, *DestPtr;
    UINT32 LineSkip;
    ETRLEObject* pTrav;
    INT32 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
    INT32 ClipX1, ClipY1, ClipX2, ClipY2;

    // Assertions
    Assert(hSrcVObject != NULL);
    Assert(pBuffer != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
    usHeight = (UINT32)pTrav->usHeight;
    usWidth = (UINT32)pTrav->usWidth;
    uiOffset = pTrav->uiDataOffset;

    // Add to start position of dest buffer
    iTempX = iX + pTrav->sOffsetX;
    iTempY = iY + pTrav->sOffsetY;

    if (clipregion == NULL) {
        ClipX1 = ClippingRect.iLeft;
        ClipY1 = ClippingRect.iTop;
        ClipX2 = ClippingRect.iRight;
        ClipY2 = ClippingRect.iBottom;
    } else {
        ClipX1 = clipregion->iLeft;
        ClipY1 = clipregion->iTop;
        ClipX2 = clipregion->iRight;
        ClipY2 = clipregion->iBottom;
    }

    // Calculate rows hanging off each side of the screen
    LeftSkip = __min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
    RightSkip = __min(max(ClipX2, (iTempX + (INT32)usWidth)) - ClipX2, (INT32)usWidth);
    TopSkip = __min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
    BottomSkip = __min(__max(ClipY2, (iTempY + (INT32)usHeight)) - ClipY2, (INT32)usHeight);

    // calculate the remaining rows and columns to blit
    BlitLength = ((INT32)usWidth - LeftSkip - RightSkip);
    BlitHeight = ((INT32)usHeight - TopSkip - BottomSkip);

    // check if whole thing is clipped
    if ((LeftSkip >= (INT32)usWidth) || (RightSkip >= (INT32)usWidth))
        return (TRUE);

    // check if whole thing is clipped
    if ((TopSkip >= (INT32)usHeight) || (BottomSkip >= (INT32)usHeight))
        return (TRUE);

    SrcPtr = (UINT8*)hSrcVObject->pPixData + uiOffset;
    DestPtr = (UINT8*)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
    p16BPPPalette = hSrcVObject->pShadeCurrent;
    LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

    __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, TopSkip
		xor		ecx, ecx

		or		ebx, ebx // check for nothing clipped on top
		jz		LeftSkipSetup

TopSkipLoop: // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		ebx
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		ebx, LeftSkip // check for nothing clipped on the left
		or		ebx, ebx
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, ebx
		je		LSSkip2 // if equal, skip whole, and start blit with new run
		jb		LSSkip1 // if less, skip whole thing

		add		esi, ebx // skip partial run, jump into normal loop for rest
		sub		ecx, ebx
		mov		ebx, BlitLength
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx // skip whole run, and start blit with new run
		jmp		BlitLineSetup

LSSkip1:
		add		esi, ecx // skip whole run, continue skipping
		sub		ebx, ecx
		jmp		LeftSkipLoop

LSTrans:
		and		ecx, 07fH
		cmp		ecx, ebx
		je		BlitLineSetup // if equal, skip whole, and start blit with new run
		jb		LSTrans1 // if less, skip whole thing

		sub		ecx, ebx // skip partial run, jump into normal loop for rest
		mov		ebx, BlitLength
		mov		Unblitted, 0
		jmp		BlitTransparent

LSTrans1:
		sub		ebx, ecx // skip whole run, continue skipping
		jmp		LeftSkipLoop

BlitLineSetup: // Does any actual blitting (trans/non) for the line
		mov		ebx, BlitLength
		mov		Unblitted, 0

BlitDispatch:

		or		ebx, ebx // Check to see if we're done blitting
		jz		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop: // blit non-transparent pixels

		cmp		ecx, ebx
		jbe		BNTrans1

		sub		ecx, ebx
		mov		Unblitted, ecx
		mov		ecx, ebx

BNTrans1:
		sub		ebx, ecx

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		xor		eax, eax
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		inc		esi
		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		xor		eax, eax
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		xor		eax, eax
		mov		al, [esi+1]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		add		esi, 2
		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitLineEnd

BlitNTL4:

		xor		eax, eax
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		xor		eax, eax
		mov		al, [esi+1]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		xor		eax, eax
		mov		al, [esi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+4], ax

		xor		eax, eax
		mov		al, [esi+3]
		mov		ax, [edx+eax*2]
		mov		[edi+6], ax

		add		esi, 4
		add		edi, 8
		dec		cl
		jnz		BlitNTL4

BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent: // skip transparent pixels

		and		ecx, 07fH
		cmp		ecx, ebx
		jbe		BTrans1

		mov		ecx, ebx

BTrans1:

		sub		ebx, ecx
            //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch

RightSkipLoop: // skip along until we hit and end-of-line marker

RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup

BlitDone:
    }

    return (TRUE);
}

/**********************************************************************************************
 BltIsClipped

	Determines whether a given blit will need clipping or not. Returns TRUE/FALSE.

**********************************************************************************************/
// FUNCTION: WIZ8 0x004120b0
BOOLEAN BltIsClipped(HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect* clipregion)
{
    UINT32 usHeight, usWidth;
    ETRLEObject* pTrav;
    INT32 iTempX, iTempY;
    INT32 ClipX1, ClipY1, ClipX2, ClipY2;

    // Assertions
    Assert(hSrcVObject != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
    usHeight = (UINT32)pTrav->usHeight;
    usWidth = (UINT32)pTrav->usWidth;

    // Add to start position of dest buffer
    iTempX = iX + pTrav->sOffsetX;
    iTempY = iY + pTrav->sOffsetY;

    if (clipregion == NULL) {
        ClipX1 = ClippingRect.iLeft;
        ClipY1 = ClippingRect.iTop;
        ClipX2 = ClippingRect.iRight;
        ClipY2 = ClippingRect.iBottom;
    } else {
        ClipX1 = clipregion->iLeft;
        ClipY1 = clipregion->iTop;
        ClipX2 = clipregion->iRight;
        ClipY2 = clipregion->iBottom;
    }

    // Calculate rows hanging off each side of the screen
    if (__min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth))
        return (TRUE);

    if (__min(max(ClipX2, (iTempX + (INT32)usWidth)) - ClipX2, (INT32)usWidth))
        return (TRUE);

    if (__min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight))
        return (TRUE);

    if (__min(__max(ClipY2, (iTempY + (INT32)usHeight)) - ClipY2, (INT32)usHeight))
        return (TRUE);

    return (FALSE);
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferShadowClip

	Modifies the destination buffer. Darkens the destination pixels by 25%, using the source
	image as a mask. Any Non-zero index pixels are used to darken destination pixels. Blitter
	clips brush if it doesn't fit on the viewport.

**********************************************************************************************/
// FUNCTION: WIZ8 0x004121e0
BOOLEAN Blt8BPPDataTo16BPPBufferShadowClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                           HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex,
                                           SGPRect* clipregion)
{
    UINT16* p16BPPPalette;
    UINT32 uiOffset;
    UINT32 usHeight, usWidth, Unblitted;
    UINT8 *SrcPtr, *DestPtr;
    UINT32 LineSkip;
    ETRLEObject* pTrav;
    INT32 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
    INT32 ClipX1, ClipY1, ClipX2, ClipY2;

    // Assertions
    Assert(hSrcVObject != NULL);
    Assert(pBuffer != NULL);

    // Get Offsets from Index into structure
    pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
    usHeight = (UINT32)pTrav->usHeight;
    usWidth = (UINT32)pTrav->usWidth;
    uiOffset = pTrav->uiDataOffset;

    // Add to start position of dest buffer
    iTempX = iX + pTrav->sOffsetX;
    iTempY = iY + pTrav->sOffsetY;

    if (clipregion == NULL) {
        ClipX1 = ClippingRect.iLeft;
        ClipY1 = ClippingRect.iTop;
        ClipX2 = ClippingRect.iRight;
        ClipY2 = ClippingRect.iBottom;
    } else {
        ClipX1 = clipregion->iLeft;
        ClipY1 = clipregion->iTop;
        ClipX2 = clipregion->iRight;
        ClipY2 = clipregion->iBottom;
    }

    // Calculate rows hanging off each side of the screen
    LeftSkip = __min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
    RightSkip = __min(max(ClipX2, (iTempX + (INT32)usWidth)) - ClipX2, (INT32)usWidth);
    TopSkip = __min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
    BottomSkip = __min(__max(ClipY2, (iTempY + (INT32)usHeight)) - ClipY2, (INT32)usHeight);

    // calculate the remaining rows and columns to blit
    BlitLength = ((INT32)usWidth - LeftSkip - RightSkip);
    BlitHeight = ((INT32)usHeight - TopSkip - BottomSkip);

    // whole thing is clipped
    if ((LeftSkip >= (INT32)usWidth) || (RightSkip >= (INT32)usWidth))
        return (TRUE);

    // whole thing is clipped
    if ((TopSkip >= (INT32)usHeight) || (BottomSkip >= (INT32)usHeight))
        return (TRUE);

    SrcPtr = (UINT8*)hSrcVObject->pPixData + uiOffset;
    DestPtr = (UINT8*)pBuffer + (uiDestPitchBYTES * (iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
    p16BPPPalette = hSrcVObject->pShadeCurrent;
    LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

    __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, OFFSET ShadeTable
		xor		eax, eax
		mov		ebx, TopSkip
		xor		ecx, ecx

		or		ebx, ebx // check for nothing clipped on top
		jz		LeftSkipSetup

TopSkipLoop: // Skips the number of lines clipped at the top

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		ebx
		jnz		TopSkipLoop

LeftSkipSetup:

		mov		Unblitted, 0
		mov		ebx, LeftSkip // check for nothing clipped on the left
		or		ebx, ebx
		jz		BlitLineSetup

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, ebx
		je		LSSkip2 // if equal, skip whole, and start blit with new run
		jb		LSSkip1 // if less, skip whole thing

		add		esi, ebx // skip partial run, jump into normal loop for rest
		sub		ecx, ebx
		mov		ebx, BlitLength
		mov		Unblitted, 0
		jmp		BlitNonTransLoop

LSSkip2:
		add		esi, ecx // skip whole run, and start blit with new run
		jmp		BlitLineSetup

LSSkip1:
		add		esi, ecx // skip whole run, continue skipping
		sub		ebx, ecx
		jmp		LeftSkipLoop

LSTrans:
		and		ecx, 07fH
		cmp		ecx, ebx
		je		BlitLineSetup // if equal, skip whole, and start blit with new run
		jb		LSTrans1 // if less, skip whole thing

		sub		ecx, ebx // skip partial run, jump into normal loop for rest
		mov		ebx, BlitLength
		jmp		BlitTransparent

LSTrans1:
		sub		ebx, ecx // skip whole run, continue skipping
		jmp		LeftSkipLoop

BlitLineSetup: // Does any actual blitting (trans/non) for the line
		mov		ebx, BlitLength
		mov		Unblitted, 0

BlitDispatch:

		or		ebx, ebx // Check to see if we're done blitting
		jz		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent

BlitNonTransLoop:

		cmp		ecx, ebx
		jbe		BNTrans1

		sub		ecx, ebx
		mov		Unblitted, ecx
		mov		ecx, ebx

BNTrans1:
		sub		ebx, ecx

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		inc		esi
		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		add		esi, 2
		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitLineEnd

BlitNTL4:

		mov		ax, [edi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

		mov		ax, [edi+2]
		mov		ax, [edx+eax*2]
		mov		[edi+2], ax

		mov		ax, [edi+4]
		mov		ax, [edx+eax*2]
		mov		[edi+4], ax

		mov		ax, [edi+6]
		mov		ax, [edx+eax*2]
		mov		[edi+6], ax

		add		esi, 4
		add		edi, 8
		dec		cl
		jnz		BlitNTL4

BlitLineEnd:
		add		esi, Unblitted
		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
		cmp		ecx, ebx
		jbe		BTrans1

		mov		ecx, ebx

BTrans1:

		sub		ebx, ecx
            //		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch

RightSkipLoop:

RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip

		jmp		LeftSkipSetup

BlitDone:
    }

    return (TRUE);
}

/**********************************************************************************************
	Blt16BPPBufferShadowRect

		Darkens a rectangular area by 25%. This blitter is used by ShadowVideoObjectRect.

	pBuffer						Pointer to a 16BPP buffer
	uiDestPitchBytes	Pitch of the destination surface
	area							An SGPRect, the area to darken

*********************************************************************************************/
// FUNCTION: WIZ8 0x004124a0
BOOLEAN Blt16BPPBufferShadowRect(UINT16* pBuffer, UINT32 uiDestPitchBYTES, SGPRect* area)
{
    INT32 width, height;
    UINT32 LineSkip;
    UINT16* DestPtr;

    // Assertions
    Assert(pBuffer != NULL);

    // Clipping
    if (area->iLeft < ClippingRect.iLeft)
        area->iLeft = ClippingRect.iLeft;
    if (area->iTop < ClippingRect.iTop)
        area->iTop = ClippingRect.iTop;
    if (area->iRight >= ClippingRect.iRight)
        area->iRight = ClippingRect.iRight - 1;
    if (area->iBottom >= ClippingRect.iBottom)
        area->iBottom = ClippingRect.iBottom - 1;
    //CHECKF(area->iLeft >= ClippingRect.iLeft );
    //CHECKF(area->iTop >= ClippingRect.iTop );
    //CHECKF(area->iRight <= ClippingRect.iRight );
    //CHECKF(area->iBottom <= ClippingRect.iBottom );

    DestPtr = (pBuffer + (area->iTop * (uiDestPitchBYTES / 2)) + area->iLeft);
    width = area->iRight - area->iLeft + 1;
    height = area->iBottom - area->iTop + 1;
    LineSkip = (uiDestPitchBYTES - (width * 2));

    CHECKF(width >= 1);
    CHECKF(height >= 1);

    __asm {
		mov		esi, OFFSET ShadeTable
		mov		edi, DestPtr
		xor		eax, eax
		mov		ebx, LineSkip
		mov		edx, height

BlitNewLine:
		mov		ecx, width

BlitLine:
		mov		ax, [edi]
		mov		ax, [esi+eax*2]
		mov		[edi], ax
		add		edi, 2
		dec		ecx
		jnz		BlitLine

		add		edi, ebx
		dec		edx
		jnz		BlitNewLine
    }

    return (TRUE);
}

/**********************************************************************************************
	Blt16BPPBufferShadowRect

		Darkens a rectangular area by 25%. This blitter is used by ShadowVideoObjectRect.

	pBuffer						Pointer to a 16BPP buffer
	uiDestPitchBytes	Pitch of the destination surface
	area							An SGPRect, the area to darken

*********************************************************************************************/
// FUNCTION: WIZ8 0x00412570
BOOLEAN Blt16BPPBufferShadowRectAlternateTable(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
                                               SGPRect* area)
{
    INT32 width, height;
    UINT32 LineSkip;
    UINT16* DestPtr;

    // Assertions
    Assert(pBuffer != NULL);

    // Clipping
    if (area->iLeft < ClippingRect.iLeft)
        area->iLeft = ClippingRect.iLeft;
    if (area->iTop < ClippingRect.iTop)
        area->iTop = ClippingRect.iTop;
    if (area->iRight >= ClippingRect.iRight)
        area->iRight = ClippingRect.iRight - 1;
    if (area->iBottom >= ClippingRect.iBottom)
        area->iBottom = ClippingRect.iBottom - 1;
    //CHECKF(area->iLeft >= ClippingRect.iLeft );
    //CHECKF(area->iTop >= ClippingRect.iTop );
    //CHECKF(area->iRight <= ClippingRect.iRight );
    //CHECKF(area->iBottom <= ClippingRect.iBottom );

    DestPtr = (pBuffer + (area->iTop * (uiDestPitchBYTES / 2)) + area->iLeft);
    width = area->iRight - area->iLeft + 1;
    height = area->iBottom - area->iTop + 1;
    LineSkip = (uiDestPitchBYTES - (width * 2));

    CHECKF(width >= 1);
    CHECKF(height >= 1);

    __asm {
		mov		esi, OFFSET IntensityTable
		mov		edi, DestPtr
		xor		eax, eax
		mov		ebx, LineSkip
		mov		edx, height

BlitNewLine:
		mov		ecx, width

BlitLine:
		mov		ax, [edi]
		mov		ax, [esi+eax*2]
		mov		[edi], ax
		add		edi, 2
		dec		ecx
		jnz		BlitLine

		add		edi, ebx
		dec		edx
		jnz		BlitNewLine
    }

    return (TRUE);
}

// UTILITY FUNCTIONS FOR BLITTING

// FUNCTION: WIZ8 0x00412640
BOOLEAN FillRect16BPP(UINT16* pBuffer, UINT32 uiDestPitchBYTES, INT32 x1, INT32 y1, INT32 x2,
                      INT32 y2, UINT16 color)
{
    INT32 x1real, y1real, x2real, y2real;
    UINT32 linelength, lines, lineskip;
    UINT16* startoffset;

    // check parameters
    Assert(pBuffer != NULL);
    Assert(uiDestPitchBYTES > 0);
    Assert(x2 > x1);
    Assert(y2 > y1);

    // clip edges of rect if hanging off screen

    x1real = __max(0, x1);
    x2real = __min(639, x2);
    y1real = __max(0, y1);
    y2real = __min(479, y2);

    startoffset = pBuffer + (y1real * uiDestPitchBYTES / 2) + x1real;
    lines = y2real - y1real + 1;
    linelength = x2real - x1real + 1;
    lineskip = uiDestPitchBYTES - (linelength * 2);

    __asm {
		mov		edi, startoffset
		mov		ax, color
		shl		eax, 16
		mov		ax, color
		mov		edx, lines
		mov		ebx, linelength

            // edi = destination pointer
            // eax = dword of color value
            // ebx = line length
            // ecx = column counter
            // edx = row counter

LineLoop:
		mov		ecx, ebx

		clc
		rcr		ecx, 1
		jnc		FL2

		mov		[edi], ax
		add		edi, 2

FL2:
		or		ecx, ecx
		jz		FillLineEnd

		rep		stosd

FillLineEnd:
		add		edi, lineskip
		dec		edx
		jnz		LineLoop

    }
    return (TRUE);
}
