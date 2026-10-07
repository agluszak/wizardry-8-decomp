/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Add matching markers for retained SGP functions and globals.
   Collapse the released JA2, utility, and precompiled-header branches to the Wizardry build.
   Include the importer declaration to preserve its C linkage in C++ mode, 2026-10-04.
   Formatting normalized for the Wizardry 8 reconstruction, 2026-10-06.
   Remove inactive code and decorative comment banners, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
// Filename :	impTGA.c
//	Purpose :	.tga file importer
// Modification history :
//		20nov96:HJH				- Creation
//				Includes

#include "types.h"
#include "Fileman.h"
#include "memman.h"
#include "WCheck.h"
#include "himage.h"
#include "impTGA.h"
#include "string.h"
#include "debug.h"
#include "video2.h"
//				Defines
//				Typedefs
//				Function Prototypes

BOOLEAN ReadUncompColMapImage(HIMAGE hImage, HWFILE hFile, UINT8 uiImgID, UINT8 uiColMap,
                              UINT16 fContents);
BOOLEAN ReadUncompRGBImage(HIMAGE hImage, HWFILE hFile, UINT8 uiImgID, UINT8 uiColMap,
                           UINT16 fContents);
BOOLEAN ReadRLEColMapImage(HIMAGE hImage, HWFILE hFile, UINT8 uiImgID, UINT8 uiColMap,
                           UINT16 fContents);
BOOLEAN ReadRLERGBImage(HIMAGE hImage, HWFILE hFile, UINT8 uiImgID, UINT8 uiColMap,
                        UINT16 fContents);
//BOOLEAN	ConvertTGAToSystemBPPFormat( HIMAGE hImage );
//				Function Definitions

// FUNCTION: WIZ8 0x00414c60
BOOLEAN LoadTGAFileToImage(HIMAGE hImage, UINT16 fContents)
{
    HWFILE hFile;
    UINT8 uiImgID, uiColMap, uiType;
    UINT32 uiBytesRead;
    BOOLEAN fReturnVal = FALSE;

    Assert(hImage != NULL);

    CHECKF(FileExists(hImage->ImageFile));

    hFile = FileOpen(hImage->ImageFile, FILE_ACCESS_READ, FALSE);
    CHECKF(hFile);

    if (!FileRead(hFile, &uiImgID, sizeof(UINT8), &uiBytesRead))
        goto end;
    if (!FileRead(hFile, &uiColMap, sizeof(UINT8), &uiBytesRead))
        goto end;
    if (!FileRead(hFile, &uiType, sizeof(UINT8), &uiBytesRead))
        goto end;

    switch (uiType) {
    case 1:
        fReturnVal = ReadUncompColMapImage(hImage, hFile, uiImgID, uiColMap, fContents);
        break;
    case 2:
        fReturnVal = ReadUncompRGBImage(hImage, hFile, uiImgID, uiColMap, fContents);
        break;
    case 9:
        fReturnVal = ReadRLEColMapImage(hImage, hFile, uiImgID, uiColMap, fContents);
        break;
    case 10:
        fReturnVal = ReadRLERGBImage(hImage, hFile, uiImgID, uiColMap, fContents);
        break;
    default:
        break;
    }

    // Set remaining values

end:
    FileClose(hFile);
    return (fReturnVal);
}
// ReadUncompColMapImage
// Parameter List :
// Return Value :
// Modification history :
//		20nov96:HJH		-> creation

BOOLEAN ReadUncompColMapImage(HIMAGE hImage, HWFILE hFile, UINT8 uiImgID, UINT8 uiColMap,
                              UINT16 fContents)
{
    return (FALSE);
}
// ReadUncompRGBImage
// Parameter List :
// Return Value :
// Modification history :
//		20nov96:HJH		-> creation

// FUNCTION: WIZ8 0x00414d70
BOOLEAN ReadUncompRGBImage(HIMAGE hImage, HWFILE hFile, UINT8 uiImgID, UINT8 uiColMap,
                           UINT16 fContents)
{
    UINT8* pBMData;
    UINT8* pBMPtr;

    UINT16 uiColMapOrigin;
    UINT16 uiColMapLength;
    UINT8 uiColMapEntrySize;
    UINT32 uiBytesRead;
    UINT16 uiXOrg;
    UINT16 uiYOrg;
    UINT16 uiWidth;
    UINT16 uiHeight;
    UINT8 uiImagePixelSize;
    UINT8 uiImageDescriptor;
    UINT32 iNumValues;
    UINT16 cnt;

    UINT32 i;
    UINT8 r;
    UINT8 g;
    UINT8 b;

    if (!FileRead(hFile, &uiColMapOrigin, sizeof(UINT16), &uiBytesRead))
        goto end;
    if (!FileRead(hFile, &uiColMapLength, sizeof(UINT16), &uiBytesRead))
        goto end;
    if (!FileRead(hFile, &uiColMapEntrySize, sizeof(UINT8), &uiBytesRead))
        goto end;

    if (!FileRead(hFile, &uiXOrg, sizeof(UINT16), &uiBytesRead))
        goto end;
    if (!FileRead(hFile, &uiYOrg, sizeof(UINT16), &uiBytesRead))
        goto end;
    if (!FileRead(hFile, &uiWidth, sizeof(UINT16), &uiBytesRead))
        goto end;
    if (!FileRead(hFile, &uiHeight, sizeof(UINT16), &uiBytesRead))
        goto end;
    if (!FileRead(hFile, &uiImagePixelSize, sizeof(UINT8), &uiBytesRead))
        goto end;
    if (!FileRead(hFile, &uiImageDescriptor, sizeof(UINT8), &uiBytesRead))
        goto end;

    // skip the id
    FileSeek(hFile, uiImgID, FILE_SEEK_FROM_CURRENT);

    // skip the colour map
    if (uiColMap != 0) {
        FileSeek(hFile, uiColMapLength * (uiImagePixelSize / 8), FILE_SEEK_FROM_CURRENT);
    }

    // Set some HIMAGE data values
    hImage->usWidth = uiWidth;
    hImage->usHeight = uiHeight;
    hImage->ubBitDepth = uiImagePixelSize;

    // Allocate memory based on bpp, height, width

    // Only do if contents flag is appropriate
    if (fContents & IMAGE_BITMAPDATA) {

        if (uiImagePixelSize == 16) {

            iNumValues = uiWidth * uiHeight;

            hImage->p16BPPData = (UINT16*)MemAlloc(iNumValues * (uiImagePixelSize / 8));

            if (hImage->p16BPPData == NULL)
                goto end;

            // Get data pointer
            pBMData = hImage->p8BPPData;

            // Start at end
            pBMData += uiWidth * (uiHeight - 1) * (uiImagePixelSize / 8);

            // Data is stored top-bottom - reverse for SGP HIMAGE format
            for (cnt = 0; cnt < uiHeight - 1; cnt++) {
                if (!FileRead(hFile, pBMData, uiWidth * 2, &uiBytesRead))
                    goto freeEnd;

                pBMData -= uiWidth * 2;
            }
            // Do first row
            if (!FileRead(hFile, pBMData, uiWidth * 2, &uiBytesRead))
                goto freeEnd;

            // Convert TGA 5,5,5 16 BPP data into current system 16 BPP Data
            //ConvertTGAToSystemBPPFormat( hImage );

            hImage->fFlags |= IMAGE_BITMAPDATA;
        }

        if (uiImagePixelSize == 24) {
            hImage->p8BPPData = (UINT8*)MemAlloc(uiWidth * uiHeight * (uiImagePixelSize / 8));

            if (hImage->p8BPPData == NULL)
                goto end;

            // Get data pointer
            pBMData = (UINT8*)hImage->p8BPPData;

            // Start at end
            pBMPtr = pBMData + uiWidth * (uiHeight - 1) * 3;

            iNumValues = uiWidth * uiHeight;

            for (cnt = 0; cnt < uiHeight; cnt++) {
                for (i = 0; i < uiWidth; i++) {
                    if (!FileRead(hFile, &b, sizeof(UINT8), &uiBytesRead))
                        goto freeEnd;
                    if (!FileRead(hFile, &g, sizeof(UINT8), &uiBytesRead))
                        goto freeEnd;
                    if (!FileRead(hFile, &r, sizeof(UINT8), &uiBytesRead))
                        goto freeEnd;

                    pBMPtr[i * 3] = r;
                    pBMPtr[i * 3 + 1] = g;
                    pBMPtr[i * 3 + 2] = b;
                }
                pBMPtr -= uiWidth * 3;
            }
            hImage->fFlags |= IMAGE_BITMAPDATA;
        }

#if 0
		// 32 bit not yet allowed in SGP
		else if ( uiImagePixelSize == 32 )
		{
			iNumValues = uiWidth * uiHeight;

			for ( i=0 ; i<iNumValues; i++ )
			{
				if ( !FileRead( hFile, &b, sizeof(UINT8), &uiBytesRead ) )
					goto freeEnd;
				if ( !FileRead( hFile, &g, sizeof(UINT8), &uiBytesRead ) )
					goto freeEnd;
				if ( !FileRead( hFile, &r, sizeof(UINT8), &uiBytesRead ) )
					goto freeEnd;
				if ( !FileRead( hFile, &a, sizeof(UINT8), &uiBytesRead ) )
					goto freeEnd;

				pBMData[ i*3   ] = r;
				pBMData[ i*3+1 ] = g;
				pBMData[ i*3+2 ] = b;
			}
		}
#endif
    }
    return (TRUE);

end:
    return (FALSE);

freeEnd:
    MemFree(pBMData);
    return (FALSE);
}
// ReadRLEColMapImage
// Parameter List :
// Return Value :
// Modification history :
//		20nov96:HJH		-> creation

BOOLEAN ReadRLEColMapImage(HIMAGE hImage, HWFILE hFile, UINT8 uiImgID, UINT8 uiColMap,
                           UINT16 fContents)
{
    return (FALSE);
}
// ReadRLERGBImage
// Parameter List :
// Return Value :
// Modification history :
//		20nov96:HJH		-> creation

BOOLEAN ReadRLERGBImage(HIMAGE hImage, HWFILE hFile, UINT8 uiImgID, UINT8 uiColMap,
                        UINT16 fContents)
{
    return (FALSE);
}
