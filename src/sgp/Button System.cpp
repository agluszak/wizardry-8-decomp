/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Annotate retail global identities verified against the Wizardry 8 binary.
   Collapse the released JA2, utility, and precompiled-header branches to the Wizardry build.
   Remove the unreferenced fast-help button global and its address, which lies inside ButtonList.
   Remove released functions that are neither retained in the Wizardry 8 retail image nor referenced by retained code.
   Distributed under the accompanying SFI Source Code license agreement. */
/***********************************************************************************************
	Button System.c

	Rewritten mostly by Kris Morness
***********************************************************************************************/

#include "types.h"
#include <windows.h>
#include <stdio.h>
#include <memory.h>
#include "debug.h"
#include "input.h"
#include "memman.h"
#include "english.h"
#include "vobject.h"
#include "vobject_blitters.h"
#include "soundman.h"
#include "Button System.h"
#include "line.h"
#include <stdarg.h>
#include "video2.h"


//ATE: Added to let Wiz default creating mouse regions with no cursor, JA2 default to a cursor ( first one )
#define		MSYS_STARTING_CURSORVAL		MSYS_NO_CURSOR
	// The following should be moved from here
#define GETPIXELDEPTH( )	( gbPixelDepth )		// From "utilities.h" in JA2
#define		COLOR_RED						162							// From "lighting.h" in JA2
#define		COLOR_BLUE					203
#define		COLOR_YELLOW				144
#define		COLOR_GREEN					184
#define		COLOR_LTGREY				134
#define		COLOR_BROWN					80
#define		COLOR_PURPLE				160
#define		COLOR_ORANGE				76
#define		COLOR_WHITE					208
#define		COLOR_BLACK					72
	// this doesn't exactly belong here either... (From "Font Control.h" in JA2)
#define		FONT_MCOLOR_BLACK				0
#define		COLOR_DKGREY				136


#define MAX_GENERIC_PICS		40
#define MAX_BUTTON_ICONS		40


#define GUI_BTN_NONE							0
#define GUI_BTN_DUPLICATE_VOBJ		1
#define GUI_BTN_EXTERNAL_VOBJ			2


// GLOBAL: WIZ8 0x006e1940
UINT8		str[128];

//Kris:  December 2, 1997
//Special internal debugging utilities that will ensure that you don't attempt to delete
//an already deleted button, or it's images, etc.  It will also ensure that you don't create
//the same button that already exists.
//TO REMOVE ALL DEBUG FUNCTIONALITY:  simply comment out BUTTONSYSTEM_DEBUGGING definition

#ifdef BUTTONSYSTEM_DEBUGGING
BOOLEAN gfIgnoreShutdownAssertions;
//Called immediately before assigning the button to the button list.
void AssertFailIfIdenticalButtonAttributesFound( GUI_BUTTON *b )
{
	INT32 x;
	GUI_BUTTON *c;
	for( x = 0; x < MAX_BUTTONS; x++ )
	{
		c = ButtonList[ x ];
		if( !c )																													continue;
		if( c->uiFlags									&  BUTTON_DELETION_PENDING		)   continue;
		if( c->UserData[3]							== 0xffffffff									)		continue;
		if( b->Area.PriorityLevel				!= c->Area.PriorityLevel			)		continue;
		if( b->Area.RegionTopLeftX			!= c->Area.RegionTopLeftX			)		continue;
		if( b->Area.RegionTopLeftY			!= c->Area.RegionTopLeftY			)		continue;
		if( b->Area.RegionBottomRightX	!= c->Area.RegionBottomRightX )		continue;
		if( b->Area.RegionBottomRightY	!= c->Area.RegionBottomRightY )		continue;
		if( b->ClickCallback						!= c->ClickCallback						)		continue;
		if( b->MoveCallback							!= c->MoveCallback						)		continue;
		if( b->XLoc											!= c->XLoc										)		continue;
		if( b->YLoc											!= c->YLoc										)		continue;
		//if we get this far, it is reasonably safe to assume that the newly created
		//button already exists.  Placing a break point on the following assert will
		//allow the coder to easily isolate the case!
		sprintf( str, "Attempting to create a button that has already been created (existing buttonID %d).", c->IDNum );
		AssertMsg( 0, str );
	}
}
#endif

//Kris:
//These are the variables used for the anchoring of a particular button.
//When you click on a button, it get's anchored, until you release the mouse button.
//When you move around, you don't want to select other buttons, even when you release
//it.  This follows the Windows 95 convention.
// GLOBAL: WIZ8 0x006e1198
GUI_BUTTON *gpAnchoredButton;
// GLOBAL: WIZ8 0x006E10C0
GUI_BUTTON *gpPrevAnchoredButton;
// GLOBAL: WIZ8 0x006e1880
BOOLEAN gfAnchoredState;
// GLOBAL: WIZ8 0x006e1190
INT8 gbDisabledButtonStyle;
void DrawHatchOnButton( GUI_BUTTON *b );
void DrawShadeOnButton( GUI_BUTTON *b );
void DrawDefaultOnButton( GUI_BUTTON *b );

// GLOBAL: WIZ8 0x005ff824
BOOLEAN gfRenderHilights = TRUE;

// GLOBAL: WIZ8 0x006e1bc0
BUTTON_PICS		ButtonPictures[MAX_BUTTON_PICS];
// GLOBAL: WIZ8 0x006e1194
INT32					ButtonPicsLoaded;

// GLOBAL: WIZ8 0x005ff828
UINT32 ButtonDestBuffer = BACKBUFFER;
// GLOBAL: WIZ8 0x005ff82c
UINT32 ButtonDestPitch = 640*2;
// GLOBAL: WIZ8 0x005ff830
UINT32 ButtonDestBPP = 16;

// GLOBAL: WIZ8 0x006e1240
GUI_BUTTON *ButtonList[MAX_BUTTONS];

// GLOBAL: WIZ8 0x00650ea4
INT32 ButtonsInList=0;

// GLOBAL: WIZ8 0x006e4060
HVOBJECT GenericButtonGrayed[MAX_GENERIC_PICS];
// GLOBAL: WIZ8 0x006e19c0
HVOBJECT GenericButtonOffNormal[MAX_GENERIC_PICS];
// GLOBAL: WIZ8 0x006e1b20
HVOBJECT GenericButtonOffHilite[MAX_GENERIC_PICS];
// GLOBAL: WIZ8 0x006e11a0
HVOBJECT GenericButtonOnNormal[MAX_GENERIC_PICS];
// GLOBAL: WIZ8 0x006e3fc0
HVOBJECT GenericButtonOnHilite[MAX_GENERIC_PICS];
// GLOBAL: WIZ8 0x006e18a0
HVOBJECT GenericButtonBackground[MAX_GENERIC_PICS];
// GLOBAL: WIZ8 0x006e1ac0
UINT16 GenericButtonFillColors[MAX_GENERIC_PICS];
// GLOBAL: WIZ8 0x006e1a60
UINT16 GenericButtonBackgroundIndex[MAX_GENERIC_PICS];
// GLOBAL: WIZ8 0x006e10e0
INT16 GenericButtonOffsetX[MAX_GENERIC_PICS];
// GLOBAL: WIZ8 0x006e1140
INT16 GenericButtonOffsetY[MAX_GENERIC_PICS];

// GLOBAL: WIZ8 0x006e1020
HVOBJECT GenericButtonIcons[MAX_BUTTON_ICONS];

// flag to state we wish to render buttons on the one after the next pass through render buttons
// GLOBAL: WIZ8 0x00650EA8
BOOLEAN fPausedMarkButtonsDirtyFlag = FALSE;
// GLOBAL: WIZ8 0x00650EA9
BOOLEAN fDisableHelpTextRestoreFlag = FALSE;

// GLOBAL: WIZ8 0x00650EAA
BOOLEAN gfDelayButtonDeletion = FALSE;
// GLOBAL: WIZ8 0x00650EAB
BOOLEAN gfPendingButtonDeletion = FALSE;
void RemoveButtonsMarkedForDeletion();

extern MOUSE_REGION *MSYS_PrevRegion;
extern MOUSE_REGION *MSYS_CurrRegion;

//=============================================================================
//	FindFreeButtonSlot
//
//	Finds an available slot for loading button pictures
//
INT32 FindFreeButtonSlot(void)
{
	int slot;

	// Are there any slots available?
	if(ButtonPicsLoaded >= MAX_BUTTON_PICS)
		return(BUTTON_NO_SLOT);

	// Search for a slot
	for(slot=0;slot<MAX_BUTTON_PICS;slot++)
	{
		if(ButtonPictures[slot].vobj==NULL)
			return(slot);
	}

	return(BUTTON_NO_SLOT);
}



//=============================================================================
//	LoadButtonImage
//
//	Load images for use with QuickButtons.
//
// FUNCTION: WIZ8 0x0040c230
INT32 LoadButtonImage(UINT8 *filename, INT32 Grayed, INT32 OffNormal, INT32 OffHilite, INT32 OnNormal, INT32 OnHilite)
{
	VOBJECT_DESC	vo_desc;
	UINT32				UseSlot;
	ETRLEObject		*pTrav;
	UINT32				MaxHeight,MaxWidth,ThisHeight,ThisWidth;
	UINT32 MemBefore,MemAfter,MemUsed;

	AssertMsg(filename!=BUTTON_NO_FILENAME, "Attempting to LoadButtonImage() with null filename." );
	AssertMsg(strlen(filename), "Attempting to LoadButtonImage() with empty filename string." );

	// is there ANY file to open?
	if((Grayed == BUTTON_NO_IMAGE) && (OffNormal == BUTTON_NO_IMAGE) && (OffHilite == BUTTON_NO_IMAGE) &&
		 (OnNormal == BUTTON_NO_IMAGE) && (OnHilite == BUTTON_NO_IMAGE))
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, String("No button pictures selected for %s",filename));
		return(-1);
	}

	// Get a button image slot
	if((UseSlot=FindFreeButtonSlot()) == BUTTON_NO_SLOT)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, String("Out of button image slots for %s",filename));
		return(-1);
	}

	// Load the image
	vo_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	strcpy(vo_desc.ImageFile, filename);

	MemBefore = MemGetFree();
	if((ButtonPictures[UseSlot].vobj = CreateVideoObject(&vo_desc)) == NULL)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, String("Couldn't create VOBJECT for %s",filename));
		return(-1);
	}
	MemAfter = MemGetFree();
	MemUsed = MemBefore-MemAfter;

	// Init the QuickButton image structure with indexes to use
	ButtonPictures[UseSlot].Grayed=Grayed;
	ButtonPictures[UseSlot].OffNormal=OffNormal;
	ButtonPictures[UseSlot].OffHilite=OffHilite;
	ButtonPictures[UseSlot].OnNormal=OnNormal;
	ButtonPictures[UseSlot].OnHilite=OnHilite;
	ButtonPictures[UseSlot].fFlags = GUI_BTN_NONE;

	// Fit the button size to the largest image in the set
	MaxWidth=MaxHeight=0;
	if(Grayed != BUTTON_NO_IMAGE)
	{
		pTrav = &(ButtonPictures[UseSlot].vobj->pETRLEObject[Grayed]);
		ThisHeight = (UINT32)(pTrav->usHeight+pTrav->sOffsetY);
		ThisWidth = (UINT32)(pTrav->usWidth+pTrav->sOffsetX);

		if(MaxWidth<ThisWidth)
			MaxWidth=ThisWidth;
		if(MaxHeight<ThisHeight)
			MaxHeight=ThisHeight;
	}

	if(OffNormal != BUTTON_NO_IMAGE)
	{
		pTrav = &(ButtonPictures[UseSlot].vobj->pETRLEObject[OffNormal]);
		ThisHeight = (UINT32)(pTrav->usHeight+pTrav->sOffsetY);
		ThisWidth = (UINT32)(pTrav->usWidth+pTrav->sOffsetX);

		if(MaxWidth<ThisWidth)
			MaxWidth=ThisWidth;
		if(MaxHeight<ThisHeight)
			MaxHeight=ThisHeight;
	}

	if(OffHilite != BUTTON_NO_IMAGE)
	{
		pTrav = &(ButtonPictures[UseSlot].vobj->pETRLEObject[OffHilite]);
		ThisHeight = (UINT32)(pTrav->usHeight+pTrav->sOffsetY);
		ThisWidth = (UINT32)(pTrav->usWidth+pTrav->sOffsetX);

		if(MaxWidth<ThisWidth)
			MaxWidth=ThisWidth;
		if(MaxHeight<ThisHeight)
			MaxHeight=ThisHeight;
	}

	if(OnNormal != BUTTON_NO_IMAGE)
	{
		pTrav = &(ButtonPictures[UseSlot].vobj->pETRLEObject[OnNormal]);
		ThisHeight = (UINT32)(pTrav->usHeight+pTrav->sOffsetY);
		ThisWidth = (UINT32)(pTrav->usWidth+pTrav->sOffsetX);

		if(MaxWidth<ThisWidth)
			MaxWidth=ThisWidth;
		if(MaxHeight<ThisHeight)
			MaxHeight=ThisHeight;
	}

	if(OnHilite != BUTTON_NO_IMAGE)
	{
		pTrav = &(ButtonPictures[UseSlot].vobj->pETRLEObject[OnHilite]);
		ThisHeight = (UINT32)(pTrav->usHeight+pTrav->sOffsetY);
		ThisWidth = (UINT32)(pTrav->usWidth+pTrav->sOffsetX);

		if(MaxWidth<ThisWidth)
			MaxWidth=ThisWidth;
		if(MaxHeight<ThisHeight)
			MaxHeight=ThisHeight;
	}

	// Set the width and height for this image set
	ButtonPictures[UseSlot].MaxHeight=MaxHeight;
	ButtonPictures[UseSlot].MaxWidth=MaxWidth;

	// return the image slot number
	ButtonPicsLoaded++;
	return(UseSlot);
}




//=============================================================================
//	UseVObjAsButtonImage
//
//	Uses a previously loaded VObject for use with QuickButtons.
//	The function simply duplicates the vobj pointer and uses that.
//
//		**** NOTE ****
//			The image isn't unloaded with a call to UnloadButtonImage. The internal
//			structures are simply removed from the button image list. It's up to
//			the user to actually unload the image.
//
// FUNCTION: WIZ8 0x0040c4f0
INT32 UseVObjAsButtonImage(HVOBJECT hVObject, INT32 Grayed, INT32 OffNormal, INT32 OffHilite, INT32 OnNormal, INT32 OnHilite)
{
	UINT32				UseSlot;
	ETRLEObject		*pTrav;
	UINT32				MaxHeight,MaxWidth,ThisHeight,ThisWidth;


	// Is button image index given valid?
	if( hVObject == NULL )
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, String("UseVObjAsButtonImage: Invalid VObject image given"));
		return(-1);
	}

	// is there ANY file to open?
	if((Grayed == BUTTON_NO_IMAGE) && (OffNormal == BUTTON_NO_IMAGE) && (OffHilite == BUTTON_NO_IMAGE) &&
		 (OnNormal == BUTTON_NO_IMAGE) && (OnHilite == BUTTON_NO_IMAGE))
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, String("UseVObjAsButtonImage: No button pictures indexes selected for VObject"));
		return(-1);
	}

	// Get a button image slot
	if((UseSlot=FindFreeButtonSlot()) == BUTTON_NO_SLOT)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, String("UseVObjAsButtonImage: Out of button image slots for VObject"));
		return(-1);
	}

	// Init the QuickButton image structure with indexes to use
	ButtonPictures[UseSlot].vobj = hVObject;
	ButtonPictures[UseSlot].Grayed=Grayed;
	ButtonPictures[UseSlot].OffNormal=OffNormal;
	ButtonPictures[UseSlot].OffHilite=OffHilite;
	ButtonPictures[UseSlot].OnNormal=OnNormal;
	ButtonPictures[UseSlot].OnHilite=OnHilite;
	ButtonPictures[UseSlot].fFlags = GUI_BTN_EXTERNAL_VOBJ;

	// Fit the button size to the largest image in the set
	MaxWidth=MaxHeight=0;
	if(Grayed != BUTTON_NO_IMAGE)
	{
		pTrav = &(ButtonPictures[UseSlot].vobj->pETRLEObject[Grayed]);
		ThisHeight = (UINT32)(pTrav->usHeight+pTrav->sOffsetY);
		ThisWidth = (UINT32)(pTrav->usWidth+pTrav->sOffsetX);

		if(MaxWidth<ThisWidth)
			MaxWidth=ThisWidth;
		if(MaxHeight<ThisHeight)
			MaxHeight=ThisHeight;
	}

	if(OffNormal != BUTTON_NO_IMAGE)
	{
		pTrav = &(ButtonPictures[UseSlot].vobj->pETRLEObject[OffNormal]);
		ThisHeight = (UINT32)(pTrav->usHeight+pTrav->sOffsetY);
		ThisWidth = (UINT32)(pTrav->usWidth+pTrav->sOffsetX);

		if(MaxWidth<ThisWidth)
			MaxWidth=ThisWidth;
		if(MaxHeight<ThisHeight)
			MaxHeight=ThisHeight;
	}

	if(OffHilite != BUTTON_NO_IMAGE)
	{
		pTrav = &(ButtonPictures[UseSlot].vobj->pETRLEObject[OffHilite]);
		ThisHeight = (UINT32)(pTrav->usHeight+pTrav->sOffsetY);
		ThisWidth = (UINT32)(pTrav->usWidth+pTrav->sOffsetX);

		if(MaxWidth<ThisWidth)
			MaxWidth=ThisWidth;
		if(MaxHeight<ThisHeight)
			MaxHeight=ThisHeight;
	}

	if(OnNormal != BUTTON_NO_IMAGE)
	{
		pTrav = &(ButtonPictures[UseSlot].vobj->pETRLEObject[OnNormal]);
		ThisHeight = (UINT32)(pTrav->usHeight+pTrav->sOffsetY);
		ThisWidth = (UINT32)(pTrav->usWidth+pTrav->sOffsetX);

		if(MaxWidth<ThisWidth)
			MaxWidth=ThisWidth;
		if(MaxHeight<ThisHeight)
			MaxHeight=ThisHeight;
	}

	if(OnHilite != BUTTON_NO_IMAGE)
	{
		pTrav = &(ButtonPictures[UseSlot].vobj->pETRLEObject[OnHilite]);
		ThisHeight = (UINT32)(pTrav->usHeight+pTrav->sOffsetY);
		ThisWidth = (UINT32)(pTrav->usWidth+pTrav->sOffsetX);

		if(MaxWidth<ThisWidth)
			MaxWidth=ThisWidth;
		if(MaxHeight<ThisHeight)
			MaxHeight=ThisHeight;
	}

	// Set the width and height for this image set
	ButtonPictures[UseSlot].MaxHeight=MaxHeight;
	ButtonPictures[UseSlot].MaxWidth=MaxWidth;

	// return the image slot number
	ButtonPicsLoaded++;
	return(UseSlot);
}





//Removes a QuickButton image from the system.
// FUNCTION: WIZ8 0x0040c710
void UnloadButtonImage(INT32 Index)
{
	INT32 x;
	BOOLEAN fDone;

	if( Index < 0 || Index >= MAX_BUTTON_PICS )
	{
		sprintf( str, "Attempting to UnloadButtonImage with out of range index %d.", Index );
		AssertMsg( 0, str );
	}

	if( !ButtonPictures[ Index ].vobj )
	{
#ifdef BUTTONSYSTEM_DEBUGGING
		if( gfIgnoreShutdownAssertions )
#endif
			return;
		AssertMsg( 0, "Attempting to UnloadButtonImage that has a null vobj (already deleted).");
	}

	// If this is a duplicated button image, then don't trash the vobject
	if(ButtonPictures[Index].fFlags & GUI_BTN_DUPLICATE_VOBJ || ButtonPictures[Index].fFlags & GUI_BTN_EXTERNAL_VOBJ)
	{
		ButtonPictures[Index].vobj = NULL;
		ButtonPicsLoaded--;
	}
	else
	{
		// Deleting a non-duplicate, so see if any dups present. if so, then
		// convert one of them to an original!

		fDone = FALSE;
		for( x = 0; x < MAX_BUTTON_PICS && !fDone; x++ )
		{
			if( (x != Index) && (ButtonPictures[x].vobj == ButtonPictures[Index].vobj) )
			{
				if ( ButtonPictures[x].fFlags & GUI_BTN_DUPLICATE_VOBJ )
				{
					// If we got here, then we got a duplicate object of the one we
					// want to delete, so convert it to an original!
					ButtonPictures[x].fFlags &= (~GUI_BTN_DUPLICATE_VOBJ);

					// Now remove this button, but not it's vobject
					ButtonPictures[Index].vobj = NULL;

					fDone = TRUE;
					ButtonPicsLoaded--;
				}
			}
		}
	}

	// If image slot isn't empty, delete the image
	if(ButtonPictures[Index].vobj != NULL)
	{
		DeleteVideoObject(ButtonPictures[Index].vobj);
		ButtonPictures[Index].vobj = NULL;
		ButtonPicsLoaded--;
	}
}





//=============================================================================
//	DisableButton
//
//	Disables a button. The button remains in the system list, and can be
//	reactivated by calling EnableButton.
//
//	Diabled buttons will appear "grayed out" on the screen (unless the
//	graphics for such are not available).
//
// FUNCTION: WIZ8 0x0040c7e0
BOOLEAN DisableButton(INT32 iButtonID )
{
	GUI_BUTTON *b;
	UINT32 OldState;

	if( iButtonID < 0 || iButtonID >= MAX_BUTTONS )
	{
		sprintf( str, "Attempting to DisableButton with out of range buttonID %d.", iButtonID );
		AssertMsg( 0, str );
	}

	b = ButtonList[ iButtonID ];

	// If button exists, reset the ENABLED flag
	if( b )
	{
		OldState = b->uiFlags & BUTTON_ENABLED;
		b->uiFlags &= (~BUTTON_ENABLED);
		b->uiFlags |= BUTTON_DIRTY;
	}
	else
		OldState = 0;

	// Return previous ENABLED state of button
	return((OldState==BUTTON_ENABLED)?TRUE:FALSE);
}



//=============================================================================
//	InitializeButtonImageManager
//
//	Initializes the button image sub-system. This function is called by
//	InitButtonSystem.
//
// FUNCTION: WIZ8 0x0040c840
BOOLEAN InitializeButtonImageManager(INT32 DefaultBuffer, INT32 DefaultPitch, INT32 DefaultBPP)
{
	VOBJECT_DESC	vo_desc;
	UINT8 Pix;
	int x;

	// Set up the default settings
	if(DefaultBuffer != BUTTON_USE_DEFAULT)
		ButtonDestBuffer = (UINT32)DefaultBuffer;
	else
		ButtonDestBuffer = FRAME_BUFFER;

	if(DefaultPitch != BUTTON_USE_DEFAULT)
		ButtonDestPitch = (UINT32)DefaultPitch;
	else
		ButtonDestPitch = 640*2;

	if(DefaultBPP != BUTTON_USE_DEFAULT)
		ButtonDestBPP = (UINT32)DefaultBPP;
	else
		ButtonDestBPP = 16;

	// Blank out all QuickButton images
	for(x=0;x<MAX_BUTTON_PICS;x++)
	{
		ButtonPictures[x].vobj=NULL;
		ButtonPictures[x].Grayed = -1;
		ButtonPictures[x].OffNormal = -1;
		ButtonPictures[x].OffHilite = -1;
		ButtonPictures[x].OnNormal = -1;
		ButtonPictures[x].OnHilite = -1;
	}
	ButtonPicsLoaded = 0;

	// Blank out all Generic button data
	for(x=0;x<MAX_GENERIC_PICS;x++)
	{
		GenericButtonGrayed[x]=NULL;
		GenericButtonOffNormal[x]=NULL;
		GenericButtonOffHilite[x]=NULL;
		GenericButtonOnNormal[x]=NULL;
		GenericButtonOnHilite[x]=NULL;
		GenericButtonBackground[x]=NULL;
		GenericButtonBackgroundIndex[x]=0;
		GenericButtonFillColors[x]=0;
		GenericButtonBackgroundIndex[x]=0;
		GenericButtonOffsetX[x]=0;
		GenericButtonOffsetY[x]=0;
	}

	// Blank out all icon images
	for(x=0;x<MAX_BUTTON_ICONS;x++)
		GenericButtonIcons[x]=NULL;

	// Load the default generic button images
	vo_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	strcpy(vo_desc.ImageFile, DEFAULT_GENERIC_BUTTON_OFF);

	if((GenericButtonOffNormal[0] = CreateVideoObject(&vo_desc)) == NULL)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, "Couldn't create VOBJECT for "DEFAULT_GENERIC_BUTTON_OFF);
		return(FALSE);
	}

	vo_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	strcpy(vo_desc.ImageFile, DEFAULT_GENERIC_BUTTON_ON);

	if((GenericButtonOnNormal[0] = CreateVideoObject(&vo_desc)) == NULL)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, "Couldn't create VOBJECT for "DEFAULT_GENERIC_BUTTON_ON);
		return(FALSE);
	}

	// Load up the off hilite and on hilite images. We won't check for errors because if the file
	// doesn't exists, the system simply ignores that file. These are only here as extra images, they
	// aren't required for operation (only OFF Normal and ON Normal are required).
	vo_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	strcpy(vo_desc.ImageFile, DEFAULT_GENERIC_BUTTON_OFF_HI);
	GenericButtonOffHilite[0] = CreateVideoObject(&vo_desc);

	vo_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	strcpy(vo_desc.ImageFile, DEFAULT_GENERIC_BUTTON_ON_HI);
	GenericButtonOnHilite[0] = CreateVideoObject(&vo_desc);

	Pix=0;
	if(!GetETRLEPixelValue(&Pix,GenericButtonOffNormal[0],8,0,0))
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, "Couldn't get generic button's background pixel value");
		return(FALSE);
	}

	if(GETPIXELDEPTH()==16)
		GenericButtonFillColors[0]=GenericButtonOffNormal[0]->p16BPPPalette[Pix];
	else if(GETPIXELDEPTH()==8)
		GenericButtonFillColors[0]=COLOR_DKGREY;

	return(TRUE);
}



//=============================================================================
//	FindFreeGenericSlot
//
//	Finds the next available slot for generic (TEXT and/or ICONIC) buttons.
//
INT16 FindFreeGenericSlot(void)
{
	INT16 slot,x;

	slot=BUTTON_NO_SLOT;
	for(x=0;x<MAX_GENERIC_PICS && slot<0;x++)
	{
		if(GenericButtonOffNormal[x]==NULL)
			slot=x;
	}

	return(slot);
}







//=============================================================================
//	UnloadGenericButtonImage
//
//	Removes the images associated with a generic button. Except the icon
//	image of iconic buttons. See above.
//
// FUNCTION: WIZ8 0x0040ca70
BOOLEAN UnloadGenericButtonImage(INT16 GenImg)
{
	BOOLEAN fDeletedSomething = FALSE;
	if( GenImg < 0 || GenImg >= MAX_GENERIC_PICS  )
	{
		sprintf( str, "Attempting to UnloadGenericButtonImage with out of range index %d.", GenImg );
		AssertMsg( 0, str );
	}

	// For each possible image type in a generic button, check if it's
	// present, and if so, remove it.
	if(GenericButtonGrayed[GenImg]!=NULL)
	{
		DeleteVideoObject(GenericButtonGrayed[GenImg]);
		GenericButtonGrayed[GenImg]=NULL;
		fDeletedSomething = TRUE;
	}

	if(GenericButtonOffNormal[GenImg]!=NULL)
	{
		DeleteVideoObject(GenericButtonOffNormal[GenImg]);
		GenericButtonOffNormal[GenImg]=NULL;
		fDeletedSomething = TRUE;
	}

	if(GenericButtonOffHilite[GenImg]!=NULL)
	{
		DeleteVideoObject(GenericButtonOffHilite[GenImg]);
		GenericButtonOffHilite[GenImg]=NULL;
		fDeletedSomething = TRUE;
	}

	if(GenericButtonOnNormal[GenImg]!=NULL)
	{
		DeleteVideoObject(GenericButtonOnNormal[GenImg]);
		GenericButtonOnNormal[GenImg]=NULL;
		fDeletedSomething = TRUE;
	}

	if(GenericButtonOnHilite[GenImg]!=NULL)
	{
		DeleteVideoObject(GenericButtonOnHilite[GenImg]);
		GenericButtonOnHilite[GenImg]=NULL;
		fDeletedSomething = TRUE;
	}

	if(GenericButtonBackground[GenImg]!=NULL)
	{
		DeleteVideoObject(GenericButtonBackground[GenImg]);
		GenericButtonBackground[GenImg]=NULL;
		fDeletedSomething = TRUE;
	}

#ifdef BUTTONSYSTEM_DEBUGGING
	if( !gfIgnoreShutdownAssertions && !fDeletedSomething )
		AssertMsg( 0, "Attempting to UnloadGenericButtonImage that has no images (already deleted)." );
#endif

	// Reset the remaining variables
	GenericButtonFillColors[GenImg]=0;
	GenericButtonBackgroundIndex[GenImg]=0;
	GenericButtonOffsetX[GenImg]=0;
	GenericButtonOffsetY[GenImg]=0;

	return(TRUE);
}



//=============================================================================
//	LoadGenericButtonImages
//
//	Loads the image files required for displaying a generic button.
//
// FUNCTION: WIZ8 0x0040cb70
INT16 LoadGenericButtonImages(UINT8 *GrayName,UINT8 *OffNormName,UINT8 *OffHiliteName,UINT8 *OnNormName,UINT8 *OnHiliteName,UINT8 *BkGrndName,INT16 Index,INT16 OffsetX, INT16 OffsetY)
{
	INT16 ImgSlot;
	VOBJECT_DESC	vo_desc;
	UINT8 Pix;

	// if the images for Off-Normal and On-Normal don't exist, abort call
	if((OffNormName == BUTTON_NO_FILENAME) || (OnNormName == BUTTON_NO_FILENAME))
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, "LoadGenericButtonImages: No filenames for OFFNORMAL and/or ONNORMAL images");
		return(-1);
	}

	// Get a slot number for these images
	if((ImgSlot=FindFreeGenericSlot()) == -1)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, "LoadGenericButtonImages: Out of generic button slots");
		return(-1);
	}

	// Load the image for the Off-Normal button state (required)
	vo_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	strcpy(vo_desc.ImageFile, OffNormName);

	if((GenericButtonOffNormal[ImgSlot] = CreateVideoObject(&vo_desc)) == NULL)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, String("LoadGenericButtonImages: Couldn't create VOBJECT for %s",OffNormName));
		return(-1);
	}

	// Load the image for the On-Normal button state (required)
	vo_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	strcpy(vo_desc.ImageFile, OnNormName);

	if((GenericButtonOnNormal[ImgSlot] = CreateVideoObject(&vo_desc)) == NULL)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, String("LoadGenericButtonImages: Couldn't create VOBJECT for %s",OnNormName));
		return(-1);
	}

	// For the optional button state images, see if a filename was given, and
	// if so, load it.

	if(GrayName != BUTTON_NO_FILENAME)
	{
		vo_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
		strcpy(vo_desc.ImageFile, GrayName);

		if((GenericButtonGrayed[ImgSlot] = CreateVideoObject(&vo_desc)) == NULL)
		{
			DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, String("LoadGenericButtonImages: Couldn't create VOBJECT for %s",GrayName));
			return(-1);
		}
	}
	else
		GenericButtonGrayed[ImgSlot] = NULL;

	if(OffHiliteName != BUTTON_NO_FILENAME)
	{
		vo_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
		strcpy(vo_desc.ImageFile, OffHiliteName);

		if((GenericButtonOffHilite[ImgSlot] = CreateVideoObject(&vo_desc)) == NULL)
		{
			DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, String("LoadGenericButtonImages: Couldn't create VOBJECT for %s",OffHiliteName));
			return(-1);
		}
	}
	else
		GenericButtonOffHilite[ImgSlot] = NULL;

	if(OnHiliteName != BUTTON_NO_FILENAME)
	{
		vo_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
		strcpy(vo_desc.ImageFile, OnHiliteName);

		if((GenericButtonOnHilite[ImgSlot] = CreateVideoObject(&vo_desc)) == NULL)
		{
			DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, String("LoadGenericButtonImages: Couldn't create VOBJECT for %s",OnHiliteName));
			return(-1);
		}
	}
	else
		GenericButtonOnHilite[ImgSlot] = NULL;

	if(BkGrndName != BUTTON_NO_FILENAME)
	{
		vo_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
		strcpy(vo_desc.ImageFile, BkGrndName);

		if((GenericButtonBackground[ImgSlot] = CreateVideoObject(&vo_desc)) == NULL)
		{
			DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, String("LoadGenericButtonImages: Couldn't create VOBJECT for %s",BkGrndName));
			return(-1);
		}
	}
	else
		GenericButtonBackground[ImgSlot] = NULL;

	GenericButtonBackgroundIndex[ImgSlot]=Index;

	// Get the background fill color from the last (9th) sub-image in the
	// Off-Normal image.
	Pix=0;
	if(!GetETRLEPixelValue(&Pix,GenericButtonOffNormal[ImgSlot],8,0,0))
	{
		DbgMessage(TOPIC_BUTTON_HANDLER, DBG_LEVEL_0, "LoadGenericButtonImages: Couldn't get generic button's background pixel value");
		return(-1);
	}

	GenericButtonFillColors[ImgSlot]=GenericButtonOffNormal[ImgSlot]->p16BPPPalette[Pix];

	// Set the button's background image adjustement offsets
	GenericButtonOffsetX[ImgSlot]=OffsetX;
	GenericButtonOffsetY[ImgSlot]=OffsetY;

	// Return the slot number used.
	return(ImgSlot);
}


//=============================================================================
//	ShutdownButtonImageManager
//
//	Cleans up, and shuts down the button image manager sub-system.
//
//	This function is called by ShutdownButtonSystem.
//
// FUNCTION: WIZ8 0x0040ce60
void ShutdownButtonImageManager(void)
{
	int x;

#ifdef BUTTONSYSTEM_DEBUGGING
	gfIgnoreShutdownAssertions = TRUE;
#endif

	// Remove all QuickButton images
	for(x=0;x<MAX_BUTTON_PICS;x++)
		UnloadButtonImage(x);

	// Remove all GenericButton images
	for(x=0;x<MAX_GENERIC_PICS;x++)
	{
		if(GenericButtonGrayed[x]!=NULL)
		{
			DeleteVideoObject(GenericButtonGrayed[x]);
			GenericButtonGrayed[x]=NULL;
		}

		if(GenericButtonOffNormal[x]!=NULL)
		{
			DeleteVideoObject(GenericButtonOffNormal[x]);
			GenericButtonOffNormal[x]=NULL;
		}

		if(GenericButtonOffHilite[x]!=NULL)
		{
			DeleteVideoObject(GenericButtonOffHilite[x]);
			GenericButtonOffHilite[x]=NULL;
		}

		if(GenericButtonOnNormal[x]!=NULL)
		{
			DeleteVideoObject(GenericButtonOnNormal[x]);
			GenericButtonOnNormal[x]=NULL;
		}

		if(GenericButtonOnHilite[x]!=NULL)
		{
			DeleteVideoObject(GenericButtonOnHilite[x]);
			GenericButtonOnHilite[x]=NULL;
		}

		if(GenericButtonBackground[x]!=NULL)
		{
			DeleteVideoObject(GenericButtonBackground[x]);
			GenericButtonBackground[x]=NULL;
		}

		GenericButtonFillColors[x]=0;
		GenericButtonBackgroundIndex[x]=0;
		GenericButtonOffsetX[x]=0;
		GenericButtonOffsetY[x]=0;
	}

	// Remove all button icons
	for(x=0;x<MAX_BUTTON_ICONS;x++)
	{
		if(GenericButtonIcons[x]!=NULL)
			GenericButtonIcons[x]=NULL;
	}
}



//=============================================================================
//	InitButtonSystem
//
//	Initializes the GUI button system for use. Must be called before using
//	any other button functions.
//
// FUNCTION: WIZ8 0x0040cf60
BOOLEAN InitButtonSystem(void)
{
	INT32 x;

#ifdef BUTTONSYSTEM_DEBUGGING
	gfIgnoreShutdownAssertions = FALSE;
#endif

	RegisterDebugTopic(TOPIC_BUTTON_HANDLER,"Button System & Button Image Manager");

	// Clear out button list
	for(x=0;x<MAX_BUTTONS;x++)
	{
		ButtonList[x]=NULL;
	}

	// Initialize the button image manager sub-system
	if(InitializeButtonImageManager(-1,-1,-1) == FALSE)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER,DBG_LEVEL_0,"Failed button image manager init\n");
		return(FALSE);
	}

	ButtonsInList = 0;

	return(TRUE);
}



//=============================================================================
//	ShutdownButtonSystem
//
//	Shuts down and cleans up the GUI button system. Must be called before
//	exiting the program. Button functions should not be used after calling
//	this function.
//
// FUNCTION: WIZ8 0x0040cf90
void ShutdownButtonSystem(void)
{
	int x;

	// Kill off all buttons in the system
	for(x=0;x<MAX_BUTTONS;x++)
	{
		if(ButtonList[x]!=NULL)
			RemoveButton(x);
	}
	// Shutdown the button image manager sub-system
	ShutdownButtonImageManager();

	UnRegisterDebugTopic(TOPIC_BUTTON_HANDLER,"Button System & Button Image Manager");
}

// FUNCTION: WIZ8 0x0040d070
void RemoveButtonsMarkedForDeletion()
{
	INT32 i;
	for( i = 0; i < MAX_BUTTONS; i++ )
	{
		if( ButtonList[ i ] && ButtonList[ i ]->uiFlags & BUTTON_DELETION_PENDING )
		{
			RemoveButton( i );
		}
	}
}

//=============================================================================
//	RemoveButton
//
//	Removes a button from the system's list. All memory associated with the
//	button is released.
//
// FUNCTION: WIZ8 0x0040d150
void RemoveButton(INT32 iButtonID)
{
	GUI_BUTTON *b;

	if( iButtonID < 0 || iButtonID >= MAX_BUTTONS )
	{
		sprintf( str, "Attempting to RemoveButton with out of range buttonID %d.", iButtonID );
		AssertMsg( 0, str );
	}

	b = ButtonList[ iButtonID ];

	// If button exists...
	if( !b )
	{
#ifdef BUTTONSYSTEM_DEBUGGING
		if( gfIgnoreShutdownAssertions )
#endif
			return;
		AssertMsg( 0, "Attempting to remove a button that has already been deleted." );
	}

	//If we happen to be in the middle of a callback, and attempt to delete a button,
	//like deleting a node during list processing, then we delay it till after the callback
	//is completed.
	if( gfDelayButtonDeletion )
	{
		b->uiFlags |= BUTTON_DELETION_PENDING;
		gfPendingButtonDeletion = TRUE;
		return;
	}

	//Kris:
	if( b->uiFlags & BUTTON_SELFDELETE_IMAGE )
	{ //checkboxes and simple create buttons have their own graphics associated with them,
		//and it is handled internally.  We delete it here.  This provides the advantage of less
		//micromanagement, but with the disadvantage of wasting more memory if you have lots of
		//buttons using the same graphics.
		UnloadButtonImage( b->ImageNum );
	}

	// ...kill it!!!
	MSYS_RemoveRegion(&b->Area);


	// Get rid of the text string
	if (b->string != NULL)
		MemFree( b->string );

	if( b == gpAnchoredButton )
		gpAnchoredButton = NULL;
	if( b == gpPrevAnchoredButton )
		gpPrevAnchoredButton = NULL;

	MemFree(b);
	b=NULL;
	ButtonList[ iButtonID ] = NULL;
}



//=============================================================================
//	GetNextButtonNumber
//
//	Finds the next available button slot.
//
INT32 GetNextButtonNumber(void)
{
	INT32 x;

	for(x=0;x<MAX_BUTTONS;x++)
	{
		if(ButtonList[x] == NULL)
			return(x);
	}

	return(BUTTON_NO_SLOT);
}



//=============================================================================
//	ResizeButton
//
//	Changes the size of a generic button.
//
//	QuickButtons cannot be resized, therefore this function ignores the
//	call if a QuickButton is given.
//
// FUNCTION: WIZ8 0x0040d210
void ResizeButton(INT32 iButtonID,INT16 w, INT16 h)
{
	GUI_BUTTON *b;
	INT32	xloc,yloc;

	if( iButtonID < 0 || iButtonID >= MAX_BUTTONS )
	{
		sprintf( str, "Attempting to resize button with out of range buttonID %d.", iButtonID );
		AssertMsg( 0, str );
	}

	// if button size is too small, adjust it.
	if(w<4)
		w=4;
	if(h<3)
		h=3;

	b = ButtonList[ iButtonID ];

	if( !b )
	{
		sprintf( str, "Attempting to resize deleted button with buttonID %d", iButtonID );
		AssertMsg( 0, str );
	}

	// If this is a QuickButton, ignore this call
	if((b->uiFlags & BUTTON_TYPES) == BUTTON_QUICK)
		return;

	// Get current button screen location
	xloc=b->XLoc;
	yloc=b->YLoc;

	// Set the new MOUSE_REGION area values to reflect change in size.
	b->Area.RegionTopLeftX=(UINT16)xloc;
	b->Area.RegionTopLeftY=(UINT16)yloc;
	b->Area.RegionBottomRightX=(UINT16)(xloc+w);
	b->Area.RegionBottomRightY=(UINT16)(yloc+h);
	b->uiFlags |= BUTTON_DIRTY;


}



//=============================================================================
//	SetButtonPosition
//
//	Sets the position of a button on the screen. The position is relative
//	to the top left corner of the button.
//
// FUNCTION: WIZ8 0x0040d2b0
void SetButtonPosition( INT32 iButtonID ,INT16 x, INT16 y)
{
	GUI_BUTTON *b;
	INT32	xloc,yloc,w,h;

	if( iButtonID < 0 || iButtonID >= MAX_BUTTONS )
	{
		sprintf( str, "Attempting to set button position with out of range buttonID %d.", iButtonID );
		AssertMsg( 0, str );
	}

	b=ButtonList[ iButtonID ];

	if( !b )
	{
		sprintf( str, "Attempting to set button position with buttonID %d", iButtonID );
		AssertMsg( 0, str );
	}

	// Get new screen position
	xloc=(INT16)x;
	yloc=(INT16)y;
	// Compute current width and height of this button
	w = b->Area.RegionBottomRightX - b->Area.RegionTopLeftX;
	h = b->Area.RegionBottomRightY - b->Area.RegionTopLeftY;

	// Set button to new location
	b->XLoc=x;
	b->YLoc=y;
	// Set the buttons MOUSE_REGION to appropriate area
	b->Area.RegionTopLeftX=(UINT16)xloc;
	b->Area.RegionTopLeftY=(UINT16)yloc;
	b->Area.RegionBottomRightX=(UINT16)(xloc+w);
	b->Area.RegionBottomRightY=(UINT16)(yloc+h);
	b->uiFlags |= BUTTON_DIRTY;


}



//Creates a generic button with text on it.
// FUNCTION: WIZ8 0x0040d350
INT32 CreateTextButton(UINT16 *string, UINT32 uiFont, INT16 sForeColor, INT16 sShadowColor, INT16 GenImg, INT16 xloc, INT16 yloc, INT16 w, INT16 h, INT32 Type, INT16 Priority,GUI_CALLBACK MoveCallback, GUI_CALLBACK ClickCallback)
{
	GUI_BUTTON *b;
	INT32	ButtonNum;
	INT32 BType,x;

	if( xloc < 0 || yloc < 0 )
	{
		sprintf( str, "Attempting to CreateTextButton with invalid position of %d,%d", xloc, yloc );
		AssertMsg( 0, str );
	}
	if( GenImg < -1 || GenImg >= MAX_GENERIC_PICS )
	{
		sprintf( str, "Attempting to CreateTextButton with out of range iconID %d.", GenImg );
		AssertMsg( 0, str );
	}

	// if button size is too small, adjust it.
	if(w<4)
		w=4;
	if(h<3)
		h=3;

	// Strip off any extraneous bits from button type
	BType = Type & ( BUTTON_TYPE_MASK | BUTTON_NEWTOGGLE );

	// Get a button number for this new button
	if((ButtonNum = GetNextButtonNumber()) == BUTTON_NO_SLOT)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER,DBG_LEVEL_0,"CreateTextButton: No more button slots");
		return(-1);
	}

	// Allocate memory for a GUI_BUTTON structure
	if((b=(GUI_BUTTON *)MemAlloc(sizeof(GUI_BUTTON))) == NULL)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER,DBG_LEVEL_0,"CreateTextButton: Can't alloc mem for button struct");
		return(-1);
	}

	// Allocate memory for the button's text string...
	b->string = NULL;
	if ( string && wcslen( string ) )
	{
		b->string = (UINT16*)MemAlloc( (wcslen(string)+1)*sizeof(UINT16) );
		AssertMsg( b->string, "Out of memory error:  Couldn't allocate string in CreateTextButton." );
		wcscpy( b->string, string );
	}

	// Init the button structure variables
	b->uiFlags = BUTTON_DIRTY;
	b->uiOldFlags = 0;
	b->IDNum = ButtonNum;
	b->XLoc = xloc;
	b->YLoc = yloc;

	if(GenImg<0)
		b->ImageNum = 0;
	else
		b->ImageNum = GenImg;

	for(x=0;x<4;x++)
		b->UserData[x] = 0;
	b->Group = -1;
	b->bDefaultStatus = DEFAULT_STATUS_NONE;
	b->bDisabledStyle = DISABLED_STYLE_DEFAULT;
	//Init string
	b->usFont = (UINT16)uiFont;
	b->fMultiColor=FALSE;
	b->sForeColor = sForeColor;
	b->sWrappedWidth = -1;
	b->sShadowColor = sShadowColor;
	b->sForeColorDown = -1;
	b->sShadowColorDown = -1;
	b->sForeColorHilited = -1;
	b->sShadowColorHilited = -1;
	b->bJustification = BUTTON_TEXT_CENTER;
	b->bTextXOffset = -1;
	b->bTextYOffset = -1;
	b->bTextXSubOffSet = 0;
	b->bTextYSubOffSet = 0;
	b->fShiftText = TRUE;
	//Init icon
	b->iIconID = -1;
	b->usIconIndex = -1;
	b->bIconXOffset = -1;
	b->bIconYOffset = -1;
	b->fShiftImage = TRUE;

	// Set the button click callback function (if any)
	if(ClickCallback != BUTTON_NO_CALLBACK)
	{
		b->ClickCallback = ClickCallback;
		BType |= BUTTON_CLICK_CALLBACK;
	}
	else
		b->ClickCallback = BUTTON_NO_CALLBACK;

	// Set the button's mouse movement callback function (if any)
	if(MoveCallback != BUTTON_NO_CALLBACK)
	{
		b->MoveCallback = MoveCallback;
		BType |= BUTTON_MOVE_CALLBACK;
	}
	else
		b->MoveCallback = BUTTON_NO_CALLBACK;

	// Define a MOUSE_REGION for this button
	MSYS_DefineRegion(&b->Area, (UINT16)xloc, (UINT16)yloc, (UINT16)(xloc+w), (UINT16)(yloc+h),
				(INT8)Priority, MSYS_STARTING_CURSORVAL, (MOUSE_CALLBACK)QuickButtonCallbackMMove, (MOUSE_CALLBACK)QuickButtonCallbackMButn);

	// Link the MOUSE_REGION to this button
	MSYS_SetRegionUserData(&b->Area,0,ButtonNum);

	// Set the flags for this button
	b->uiFlags |= ( BUTTON_ENABLED | BType | BUTTON_GENERIC);


	// Add this button to the button list
#ifdef BUTTONSYSTEM_DEBUGGING
	AssertFailIfIdenticalButtonAttributesFound( b );
#endif
	ButtonList[ButtonNum]=b;

	SpecifyButtonSoundScheme( b->IDNum, BUTTON_SOUND_SCHEME_GENERIC );

	// return the slot number
	return(ButtonNum);
}





//=============================================================================
//	QuickCreateButton
//
//	Creates a QuickButton. QuickButtons only have graphics associated with
//	them. They cannot be re-sized, nor can the graphic be changed.
//
// FUNCTION: WIZ8 0x0040d5e0
INT32 QuickCreateButton(UINT32 Image,INT16 xloc,INT16 yloc,INT32 Type,INT16 Priority,GUI_CALLBACK MoveCallback,GUI_CALLBACK ClickCallback)
{
	GUI_BUTTON *b;
	INT32	ButtonNum;
	INT32 BType,x;

	if( xloc < 0 || yloc < 0 )
	{
		sprintf( str, "Attempting to QuickCreateButton with invalid position of %d,%d", xloc, yloc );
		AssertMsg( 0, str );
	}
	if( Image < 0 || Image >= MAX_BUTTON_PICS )
	{
		sprintf( str, "Attempting to QuickCreateButton with out of range ImageID %d.", Image );
		AssertMsg( 0, str );
	}

	// Strip off any extraneous bits from button type
	BType = Type & ( BUTTON_TYPE_MASK | BUTTON_NEWTOGGLE );

	// Is there a QuickButton image in the given image slot?
	if(ButtonPictures[Image].vobj == NULL)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER,DBG_LEVEL_0,"QuickCreateButton: Invalid button image number");
		return(-1);
	}

	// Get a new button number
	if((ButtonNum = GetNextButtonNumber()) == BUTTON_NO_SLOT)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER,DBG_LEVEL_0,"QuickCreateButton: No more button slots");
		return(-1);
	}

	// Allocate memory for a GUI_BUTTON structure
	if((b=(GUI_BUTTON *)MemAlloc(sizeof(GUI_BUTTON))) == NULL)
	{
		DbgMessage(TOPIC_BUTTON_HANDLER,DBG_LEVEL_0,"QuickCreateButton: Can't alloc mem for button struct");
		return(-1);
	}

	// Set the values for this buttn
	b->uiFlags = BUTTON_DIRTY;
	b->uiOldFlags = 0;

	// Set someflags if of s certain type....
	if ( Type & BUTTON_NEWTOGGLE )
	{
		b->uiFlags |= BUTTON_NEWTOGGLE;
	}

	// shadow style
	b->bDefaultStatus = DEFAULT_STATUS_NONE;
	b->bDisabledStyle = DISABLED_STYLE_DEFAULT;

	b->Group = -1;
	//Init string
	b->string = NULL;
	b->usFont = 0;
	b->fMultiColor=FALSE;
	b->sForeColor = 0;
	b->sWrappedWidth = -1;
	b->sShadowColor = -1;
	b->sForeColorDown = -1;
	b->sShadowColorDown = -1;
	b->sForeColorHilited = -1;
	b->sShadowColorHilited = -1;
	b->bJustification = BUTTON_TEXT_CENTER;
	b->bTextXOffset = -1;
	b->bTextYOffset = -1;
	b->bTextXSubOffSet = 0;
	b->bTextYSubOffSet = 0;
	b->fShiftText = TRUE;
	//Init icon
	b->iIconID = -1;
	b->usIconIndex = -1;
	b->bIconXOffset = -1;
	b->bIconYOffset = -1;
	b->fShiftImage = TRUE;
	//Init quickbutton
	b->IDNum = ButtonNum;
	b->ImageNum = Image;
	for(x=0;x<4;x++)
		b->UserData[x] = 0;

	b->XLoc = xloc;
	b->YLoc = yloc;

	b->ubToggleButtonOldState = 0;
	b->ubToggleButtonActivated = FALSE;

	// Set the button click callback function (if any)
	if(ClickCallback != BUTTON_NO_CALLBACK)
	{
		b->ClickCallback = ClickCallback;
		BType |= BUTTON_CLICK_CALLBACK;
	}
	else
		b->ClickCallback = BUTTON_NO_CALLBACK;

	// Set the button's mouse movement callback function (if any)
	if(MoveCallback != BUTTON_NO_CALLBACK)
	{
		b->MoveCallback = MoveCallback;
		BType |= BUTTON_MOVE_CALLBACK;
	}
	else
		b->MoveCallback = BUTTON_NO_CALLBACK;

	memset( &b->Area, 0, sizeof( MOUSE_REGION ) );
	// Define a MOUSE_REGION for this QuickButton
	MSYS_DefineRegion(&b->Area,(UINT16)xloc,(UINT16)yloc,
			  (UINT16)(xloc+(INT16)ButtonPictures[Image].MaxWidth),
				(UINT16)(yloc+(INT16)ButtonPictures[Image].MaxHeight),
				(INT8)Priority, MSYS_STARTING_CURSORVAL,
				(MOUSE_CALLBACK)QuickButtonCallbackMMove,
				(MOUSE_CALLBACK)QuickButtonCallbackMButn);

	// Link the MOUSE_REGION with this QuickButton
	MSYS_SetRegionUserData(&b->Area,0,ButtonNum);

	// Set the flags for this button
	b->uiFlags |= BUTTON_ENABLED | BType | BUTTON_QUICK;

	// Add this QuickButton to the button list
#ifdef BUTTONSYSTEM_DEBUGGING
	AssertFailIfIdenticalButtonAttributesFound( b );
#endif
	ButtonList[ButtonNum]=b;

	SpecifyButtonSoundScheme( b->IDNum, BUTTON_SOUND_SCHEME_GENERIC );

	// return the button number (slot)
	return(ButtonNum);
}


//New functions
// FUNCTION: WIZ8 0x0040d850
void SpecifyButtonText( INT32 iButtonID, UINT16 *string )
{
	GUI_BUTTON *b;

	Assert( iButtonID >= 0 );
	Assert( iButtonID < MAX_BUTTONS );

	b = ButtonList[ iButtonID ];

	//free the previous strings memory if applicable
	if( b->string )
		MemFree( b->string );
	b->string = NULL;

	if( string && wcslen( string ) )
	{
		//allocate memory for the new string
		b->string = (UINT16*)MemAlloc( (wcslen(string)+1)*sizeof(UINT16) );
		Assert( b->string );
		//copy the string to the button
		wcscpy( b->string, string );
		b->uiFlags |= BUTTON_DIRTY;
	}
}

// FUNCTION: WIZ8 0x0040d8c0
void SpecifyButtonMultiColorFont(INT32 iButtonID, BOOLEAN fMultiColor)
{
	GUI_BUTTON *b;
	Assert( iButtonID >= 0 );
	Assert( iButtonID < MAX_BUTTONS );
	b = ButtonList[ iButtonID ];
	Assert( b );
	b->fMultiColor = fMultiColor;
	b->uiFlags |= BUTTON_DIRTY ;
}

// FUNCTION: WIZ8 0x0040d8e0
void SpecifyButtonTextOffsets( INT32 iButtonID, INT8 bTextXOffset, INT8 bTextYOffset, BOOLEAN fShiftText )
{
	GUI_BUTTON *b;
	Assert( iButtonID >= 0 );
	Assert( iButtonID < MAX_BUTTONS );
	b = ButtonList[ iButtonID ];
	Assert( b );
	//Copy over information
	b->bTextXOffset = bTextXOffset;
	b->bTextYOffset = bTextYOffset;
	b->fShiftText = fShiftText;
}



//=============================================================================
//	SetButtonFastHelpText
//
//	Set the text that will be displayed as the FastHelp
//
// FUNCTION: WIZ8 0x0040d910
void SetButtonFastHelpText(INT32 iButton, UINT16 *Text)
{
	GUI_BUTTON *b;
	if(iButton<0 || iButton>MAX_BUTTONS)
		return;
	b = ButtonList[iButton];
	AssertMsg( b, "Called SetButtonFastHelpText() with a non-existant button." );
	SetRegionFastHelpText( &b->Area, Text );
}

//=============================================================================
//	QuickButtonCallbackMMove
//
//	Dispatches all button callbacks for mouse movement. This function gets
//	called by the Mouse System. *DO NOT CALL DIRECTLY*
//
// FUNCTION: WIZ8 0x0040d940
void QuickButtonCallbackMMove(MOUSE_REGION *reg,INT32 reason)
{
	GUI_BUTTON *b;
	INT32 iButtonID;


	Assert(reg != NULL);

	iButtonID = MSYS_GetRegionUserData(reg,0);

	sprintf( str, "QuickButtonCallbackMMove: Mouse Region #%d (%d,%d to %d,%d) has invalid buttonID %d",
						reg->IDNumber, reg->RegionTopLeftX, reg->RegionTopLeftY, reg->RegionBottomRightX, reg->RegionBottomRightY, iButtonID );

	AssertMsg( iButtonID >= 0, str );
	AssertMsg( iButtonID < MAX_BUTTONS, str );

	b = ButtonList[ iButtonID ];

	AssertMsg( b != NULL, str );

	if( !b )
		return;  //This is getting called when Adding new regions...


	if( b->uiFlags & BUTTON_ENABLED &&
		  reason & (MSYS_CALLBACK_REASON_LOST_MOUSE | MSYS_CALLBACK_REASON_GAIN_MOUSE) )
	{
		b->uiFlags |= BUTTON_DIRTY;
	}

	// Mouse moved on the button, so reset it's timer to maximum.
	if( b->Area.uiFlags & MSYS_CALLBACK_REASON_GAIN_MOUSE )
	{
		//check for sound playing stuff
		if( b->ubSoundSchemeID )
		{
			if( &b->Area == MSYS_PrevRegion && !gpAnchoredButton )
			{
				if( b->uiFlags & BUTTON_ENABLED )
				{
					PlayButtonSound( iButtonID, BUTTON_SOUND_MOVED_ONTO );
				}
				else
				{
					PlayButtonSound( iButtonID, BUTTON_SOUND_DISABLED_MOVED_ONTO );
				}
			}
		}
	}
	else
	{
		//Check if we should play a sound
		if( b->ubSoundSchemeID )
		{
			if( b->uiFlags & BUTTON_ENABLED )
			{
				if( &b->Area == MSYS_PrevRegion && !gpAnchoredButton )
				{
					PlayButtonSound( iButtonID, BUTTON_SOUND_MOVED_OFF_OF );
				}
			}
			else
			{
				PlayButtonSound( iButtonID, BUTTON_SOUND_DISABLED_MOVED_OFF_OF );
			}
		}
	}

	// ATE: New stuff for toggle buttons that work with new Win95 paridigm
	if ( ( b->uiFlags & BUTTON_NEWTOGGLE ) )
	{
		if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
		{
			if ( b->ubToggleButtonActivated )
			{
				b->ubToggleButtonActivated = FALSE;

				if ( !b->ubToggleButtonOldState )
				{
					b->uiFlags &= (~BUTTON_CLICKED_ON );
				}
				else
				{
					b->uiFlags |= BUTTON_CLICKED_ON;
				}
			}
		}
	}

	// If this button is enabled and there is a callback function associated with it,
	// call the callback function.
	if((b->uiFlags & BUTTON_ENABLED) && (b->uiFlags & BUTTON_MOVE_CALLBACK))
		(b->MoveCallback)(b,reason);
}



//=============================================================================
//	QuickButtonCallbackMButn
//
//	Dispatches all button callbacks for button presses. This function is
//	called by the Mouse System. *DO NOT CALL DIRECTLY*
//
// FUNCTION: WIZ8 0x0040da50
void QuickButtonCallbackMButn( MOUSE_REGION *reg, INT32 reason )
{
	GUI_BUTTON *b;
	INT32		iButtonID;
	BOOLEAN MouseBtnDown;
	BOOLEAN StateBefore,StateAfter;


	Assert(reg != NULL);

	iButtonID = MSYS_GetRegionUserData(reg,0);

	sprintf( str, "QuickButtonCallbackMButn: Mouse Region #%d (%d,%d to %d,%d) has invalid buttonID %d",
						reg->IDNumber, reg->RegionTopLeftX, reg->RegionTopLeftY, reg->RegionBottomRightX, reg->RegionBottomRightY, iButtonID );

	AssertMsg( iButtonID >= 0, str );
	AssertMsg( iButtonID < MAX_BUTTONS, str );

	b = ButtonList[ iButtonID ];

	AssertMsg( b != NULL, str );

	if( !b )
		return;


	if( reason & (MSYS_CALLBACK_REASON_LBUTTON_DWN | MSYS_CALLBACK_REASON_RBUTTON_DWN) )
		MouseBtnDown = TRUE;
	else
		MouseBtnDown = FALSE;

	StateBefore = (b->uiFlags & BUTTON_CLICKED_ON) ? (TRUE) : (FALSE);

	// ATE: New stuff for toggle buttons that work with new Win95 paridigm
	if( b->uiFlags & BUTTON_NEWTOGGLE && b->uiFlags & BUTTON_ENABLED )
	{
		if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
		{
			if ( !b->ubToggleButtonActivated )
			{
				if ( !(b->uiFlags & BUTTON_CLICKED_ON ) )
				{
					b->ubToggleButtonOldState = FALSE;
					b->uiFlags |= BUTTON_CLICKED_ON;
				}
				else
				{
					b->ubToggleButtonOldState = TRUE;
					b->uiFlags &= (~BUTTON_CLICKED_ON);
				}
				b->ubToggleButtonActivated = TRUE;
			}
		}
		else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
		{
			b->ubToggleButtonActivated = FALSE;
		}
	}


	//Kris:
	//Set the anchored button incase the user moves mouse off region while still holding
	//down the button, but only if the button is up.  In Win95, buttons that are already
	//down, and anchored never change state, unless you release the mouse in the button area.

	if( b->MoveCallback == DEFAULT_MOVE_CALLBACK && b->uiFlags & BUTTON_ENABLED )
	{
		if( reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
		{
			gpAnchoredButton =  b;
			gfAnchoredState = StateBefore;
			b->uiFlags |= BUTTON_CLICKED_ON;
		}
		else if( reason & MSYS_CALLBACK_REASON_LBUTTON_UP && b->uiFlags & BUTTON_NO_TOGGLE )
		{
			b->uiFlags &= (~BUTTON_CLICKED_ON);
		}
	}
	else if( b->uiFlags & BUTTON_CHECKBOX )
	{
		if( reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
		{	//the check box button gets anchored, though it doesn't actually use the anchoring move callback.
			//The effect is different, we don't want to toggle the button state, but we do want to anchor this
			//button so that we don't effect any other buttons while we move the mouse around in anchor mode.
			gpAnchoredButton = b;
			gfAnchoredState = StateBefore;

			//Trick the before state of the button to be different so the sound will play properly as checkbox buttons
			//are processed differently.
			StateBefore = (b->uiFlags & BUTTON_CLICKED_ON) ? FALSE : TRUE;
			StateAfter = !StateBefore;
		}
		else if( reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
		{
			b->uiFlags ^= BUTTON_CLICKED_ON; //toggle the checkbox state upon release inside button area.
			//Trick the before state of the button to be different so the sound will play properly as checkbox buttons
			//are processed differently.
			StateBefore = (b->uiFlags & BUTTON_CLICKED_ON) ? FALSE : TRUE;
			StateAfter = !StateBefore;
		}
	}

	// Should we play a sound if clicked on while disabled?
	if( b->ubSoundSchemeID && !(b->uiFlags & BUTTON_ENABLED) && MouseBtnDown )
	{
		PlayButtonSound( iButtonID, BUTTON_SOUND_DISABLED_CLICK );
	}

	// If this button is disabled, and no callbacks allowed when disabled
	// callback
	if(!(b->uiFlags & BUTTON_ENABLED) && !(b->uiFlags & BUTTON_ALLOW_DISABLED_CALLBACK))
		return;

	// Button not enabled but allowed to use callback, then do that!
	if(!(b->uiFlags & BUTTON_ENABLED) && (b->uiFlags & BUTTON_ALLOW_DISABLED_CALLBACK))
	{
		if( b->uiFlags & BUTTON_CLICK_CALLBACK )
		{
			(b->ClickCallback)(b, reason | BUTTON_DISABLED_CALLBACK );
		}
		return;
	}

	// If there is a callback function with this button, call it
	if(b->uiFlags & BUTTON_CLICK_CALLBACK)
	{
		//Kris:  January 6, 1998
		//Added these checks to avoid a case where it was possible to process a leftbuttonup message when
		//the button wasn't anchored, and should have been.
		gfDelayButtonDeletion = TRUE;
		if( !(reason & MSYS_CALLBACK_REASON_LBUTTON_UP) || b->MoveCallback != DEFAULT_MOVE_CALLBACK ||
				b->MoveCallback == DEFAULT_MOVE_CALLBACK && gpPrevAnchoredButton == b )
			(b->ClickCallback)(b,reason);
		gfDelayButtonDeletion = FALSE;
	}
	else if((reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) && !(b->uiFlags & BUTTON_IGNORE_CLICKS))
	{
		// Otherwise, do default action with this button.
		b->uiFlags^=BUTTON_CLICKED_ON;
	}

	if( b->uiFlags & BUTTON_CHECKBOX )
	{
		StateAfter = (b->uiFlags & BUTTON_CLICKED_ON) ? (TRUE) : (FALSE);
	}

	// Play sounds for this enabled button (disabled sounds have already been done)
	if( b->ubSoundSchemeID && b->uiFlags & BUTTON_ENABLED )
	{
		if( reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
		{
			if( b->ubSoundSchemeID && StateBefore && !StateAfter )
			{
				PlayButtonSound( iButtonID, BUTTON_SOUND_CLICKED_OFF );
			}
		}
		else if( reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
		{
			if( b->ubSoundSchemeID && !StateBefore && StateAfter)
			{
				PlayButtonSound( iButtonID, BUTTON_SOUND_CLICKED_ON );
			}
		}
	}

	if( StateBefore != StateAfter )
	{
	}

	if( gfPendingButtonDeletion )
	{
		RemoveButtonsMarkedForDeletion();
	}
}

// FUNCTION: WIZ8 0x0040dc80
void RenderButtons(void)
{
	INT32			iButtonID;
	BOOLEAN		fOldButtonDown, fOldEnabled;
	GUI_BUTTON *b;

	SaveFontSettings();
	for(iButtonID=0;iButtonID<MAX_BUTTONS;iButtonID++)
	{
		// If the button exists, and it's not owned by another object, draw it
		//Kris:  and make sure that the button isn't hidden.
		b = ButtonList[iButtonID];
		if( b && b->Area.uiFlags & MSYS_REGION_ENABLED )
		{
			// Check for buttonchanged status
			fOldButtonDown = (BOOLEAN)(b->uiFlags & BUTTON_CLICKED_ON);

			if ( fOldButtonDown != ( b->uiOldFlags & BUTTON_CLICKED_ON ) )
			{
				//Something is different, set dirty!
				b->uiFlags |= BUTTON_DIRTY;
			}

			// Check for button dirty flags
			fOldEnabled = (BOOLEAN)(b->uiFlags & BUTTON_ENABLED);

			if ( fOldEnabled != ( b->uiOldFlags & BUTTON_ENABLED ) )
			{
				//Something is different, set dirty!
				b->uiFlags |= BUTTON_DIRTY;
			}

			// If we ABSOLUTELY want to render every frame....
			if ( b->uiFlags & BUTTON_SAVEBACKGROUND )
			{
				b->uiFlags |= BUTTON_DIRTY;
			}

			// Set old flags
			b->uiOldFlags = b->uiFlags;

			if ( b->uiFlags & BUTTON_FORCE_UNDIRTY )
			{
				b->uiFlags &= ~( BUTTON_DIRTY );
				b->uiFlags &= ~( BUTTON_FORCE_UNDIRTY );
			}

			// Check if we need to update!
			if ( b->uiFlags & BUTTON_DIRTY )
			{
				// Turn off dirty flag
				b->uiFlags &= (~BUTTON_DIRTY);
				DrawButtonFromPtr(b);


			}
		}
	}

	// check if we want to render 1 frame later?
	if( ( fPausedMarkButtonsDirtyFlag == TRUE ) && ( fDisableHelpTextRestoreFlag == FALSE ) )
	{
		fPausedMarkButtonsDirtyFlag = FALSE;
		MarkButtonsDirty( );
	}

	RestoreFontSettings();
}

//=============================================================================
//	MarkButtonsDirty
//
// FUNCTION: WIZ8 0x0040de90
void MarkButtonsDirty( void )
{
	INT32 x;
	for(x=0;x<MAX_BUTTONS;x++)
	{
		// If the button exists, and it's not owned by another object, draw it
		if( ButtonList[x] )
		{
			// Turn on dirty flag
			ButtonList[x]->uiFlags |= BUTTON_DIRTY;
		}
	}
}


//=============================================================================
// PauseMarkButtonsDirty
//

//=============================================================================
//	DrawButton
//
//	Draws a single button on the screen.
//
// FUNCTION: WIZ8 0x0040deb0
BOOLEAN DrawButton(INT32 iButtonID )
{
	// Fail if button handle out of range
	if( iButtonID < 0 || iButtonID > MAX_BUTTONS )
		return FALSE;

	// Fail if button handle is invalid
	if( !ButtonList[ iButtonID ] )
		return FALSE;

	if( ButtonList[ iButtonID ]->string )
		SaveFontSettings();
	// Draw this button
  if( ButtonList[ iButtonID ]->Area.uiFlags & MSYS_REGION_ENABLED )
  {
	  DrawButtonFromPtr( ButtonList[ iButtonID ] );
  }

	if( ButtonList[ iButtonID ]->string )
		RestoreFontSettings();
	return TRUE;
}



//=============================================================================
//	DrawButtonFromPtr
//
//	Given a pointer to a GUI_BUTTON structure, draws the button on the
//	screen.
//
void DrawButtonFromPtr(GUI_BUTTON *b)
{
	Assert( b );
	// Draw the appropriate button according to button type
	gbDisabledButtonStyle = DISABLED_STYLE_NONE;
	switch(b->uiFlags & BUTTON_TYPES)
	{
		case BUTTON_QUICK:
			DrawQuickButton(b);
			break;
		case BUTTON_GENERIC:
			DrawGenericButton(b);
			break;
		case BUTTON_HOT_SPOT:
			if(b->uiFlags & BUTTON_NO_TOGGLE)
				b->uiFlags &= (~BUTTON_CLICKED_ON);
			return;  //hotspots don't have text, but if you want to, change this to a break!
		case BUTTON_CHECKBOX:
			DrawCheckBoxButton(b);
			break;
	}
	//If button has an icon, overlay it on current button.
	if( b->iIconID != -1 )
		DrawIconOnButton( b );
	//If button has text, draw it now
	if( b->string )
		DrawTextOnButton( b );
	//If the button is disabled, and a style has been calculated, then
	//draw the style last.
	switch( gbDisabledButtonStyle )
	{
		case DISABLED_STYLE_HATCHED:
			DrawHatchOnButton( b );
			break;
		case DISABLED_STYLE_SHADED:
			DrawShadeOnButton( b );
			break;
	}
	if( b->bDefaultStatus )
	{
		DrawDefaultOnButton( b );
	}
}

//=============================================================================
//	DrawQuickButton
//
//	Draws a QuickButton type button on the screen.
//
// FUNCTION: WIZ8 0x0040e060
void DrawQuickButton(GUI_BUTTON *b)
{
	INT32 UseImage;
	UseImage=0;
	// Is button Enabled, or diabled but no "Grayed" image associated with this QuickButton?
	if(b->uiFlags & BUTTON_ENABLED )
	{
		// Is the button's state ON?
		if(b->uiFlags & BUTTON_CLICKED_ON)
		{
			// Is the mouse over this area, and we have a hilite image?
			if((b->Area.uiFlags & MSYS_MOUSE_IN_AREA) && gfRenderHilights &&
					(ButtonPictures[b->ImageNum].OnHilite != -1))
				UseImage = ButtonPictures[b->ImageNum].OnHilite;			// Use On-Hilite image
			else if(ButtonPictures[b->ImageNum].OnNormal != -1)
				UseImage = ButtonPictures[b->ImageNum].OnNormal;			// Use On-Normal image
		}
		else
		{
			// Is the mouse over the button, and do we have hilite image?
			if((b->Area.uiFlags & MSYS_MOUSE_IN_AREA) && gfRenderHilights &&
					(ButtonPictures[b->ImageNum].OffHilite != -1))
				UseImage = ButtonPictures[b->ImageNum].OffHilite;			// Use Off-Hilite image
			else if(ButtonPictures[b->ImageNum].OffNormal != -1)
				UseImage = ButtonPictures[b->ImageNum].OffNormal;			// Use Off-Normal image
		}
	}
	else if( ButtonPictures[b->ImageNum].Grayed != -1)
	{	// Button is diabled so use the "Grayed-out" image
		UseImage = ButtonPictures[b->ImageNum].Grayed;
	}
	else
	{
		UseImage = ButtonPictures[b->ImageNum].OffNormal;
		switch( b->bDisabledStyle )
		{
			case DISABLED_STYLE_DEFAULT:
				gbDisabledButtonStyle = b->string ? DISABLED_STYLE_SHADED : DISABLED_STYLE_HATCHED;
				break;
			case DISABLED_STYLE_HATCHED:
			case DISABLED_STYLE_SHADED:
				gbDisabledButtonStyle = b->bDisabledStyle;
				break;
		}
	}

	// Display the button image
	BltVideoObject(ButtonDestBuffer, ButtonPictures[b->ImageNum].vobj,
								 (UINT16)UseImage, b->XLoc, b->YLoc,
								 VO_BLT_SRCTRANSPARENCY, NULL);
}

void DrawHatchOnButton( GUI_BUTTON *b )
{
	UINT8	 *pDestBuf;
	UINT32 uiDestPitchBYTES;
	SGPRect ClipRect;
	ClipRect.iLeft = b->Area.RegionTopLeftX;
	ClipRect.iRight = b->Area.RegionBottomRightX - 1;
	ClipRect.iTop = b->Area.RegionTopLeftY;
	ClipRect.iBottom = b->Area.RegionBottomRightY - 1;
	pDestBuf = LockVideoSurface( ButtonDestBuffer, &uiDestPitchBYTES );
	Blt16BPPBufferHatchRect( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect );
	UnLockVideoSurface( ButtonDestBuffer );
}

void DrawShadeOnButton( GUI_BUTTON *b )
{
	UINT8 *pDestBuf;
	UINT32 uiDestPitchBYTES;
	SGPRect ClipRect;
	ClipRect.iLeft = b->Area.RegionTopLeftX;
	ClipRect.iRight = b->Area.RegionBottomRightX-1;
	ClipRect.iTop = b->Area.RegionTopLeftY;
	ClipRect.iBottom = b->Area.RegionBottomRightY-1;
	pDestBuf = LockVideoSurface( ButtonDestBuffer, &uiDestPitchBYTES );
	Blt16BPPBufferShadowRect( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect );
	UnLockVideoSurface( ButtonDestBuffer );
}

// FUNCTION: WIZ8 0x0040e170
void DrawDefaultOnButton( GUI_BUTTON *b )
{
	UINT8 *pDestBuf;
	UINT32 uiDestPitchBYTES;
	pDestBuf = LockVideoSurface( ButtonDestBuffer, &uiDestPitchBYTES );
	SetClippingRegionAndImageWidth( uiDestPitchBYTES, 0, 0, 640, 480 );
	if( b->bDefaultStatus == DEFAULT_STATUS_DARKBORDER || b->bDefaultStatus == DEFAULT_STATUS_WINDOWS95 )
	{
		//left (one thick)
		LineDraw( TRUE, b->Area.RegionTopLeftX-1, b->Area.RegionTopLeftY-1, b->Area.RegionTopLeftX-1, b->Area.RegionBottomRightY+1, 0, pDestBuf );
		//top (one thick)
		LineDraw( TRUE, b->Area.RegionTopLeftX-1, b->Area.RegionTopLeftY-1, b->Area.RegionBottomRightX+1, b->Area.RegionTopLeftY-1, 0, pDestBuf );
		//right (two thick)
		LineDraw( TRUE, b->Area.RegionBottomRightX, b->Area.RegionTopLeftY-1, b->Area.RegionBottomRightX, b->Area.RegionBottomRightY+1, 0, pDestBuf );
		LineDraw( TRUE, b->Area.RegionBottomRightX+1, b->Area.RegionTopLeftY-1, b->Area.RegionBottomRightX+1, b->Area.RegionBottomRightY+1, 0, pDestBuf );
		//bottom (two thick)
		LineDraw( TRUE, b->Area.RegionTopLeftX-1, b->Area.RegionBottomRightY, b->Area.RegionBottomRightX+1, b->Area.RegionBottomRightY, 0, pDestBuf );
		LineDraw( TRUE, b->Area.RegionTopLeftX-1, b->Area.RegionBottomRightY+1, b->Area.RegionBottomRightX+1, b->Area.RegionBottomRightY+1, 0, pDestBuf );
	}
	if( b->bDefaultStatus == DEFAULT_STATUS_DOTTEDINTERIOR || b->bDefaultStatus == DEFAULT_STATUS_WINDOWS95 )
	{ //Draw an internal dotted rectangle.

	}
	UnLockVideoSurface( ButtonDestBuffer );
}


// FUNCTION: WIZ8 0x0040e280
void DrawCheckBoxButton( GUI_BUTTON *b )
{
	INT32 UseImage;

	UseImage=0;
	// Is button Enabled, or diabled but no "Grayed" image associated with this QuickButton?
	if( b->uiFlags & BUTTON_ENABLED )
	{
		// Is the button's state ON?
		if(b->uiFlags & BUTTON_CLICKED_ON)
		{
			// Is the mouse over this area, and we have a hilite image?
			if( b->Area.uiFlags & MSYS_MOUSE_IN_AREA && gfRenderHilights &&
					gfLeftButtonState &&
					ButtonPictures[b->ImageNum].OnHilite != -1 )
				UseImage = ButtonPictures[b->ImageNum].OnHilite;			// Use On-Hilite image
			else if(ButtonPictures[b->ImageNum].OnNormal != -1)
				UseImage = ButtonPictures[b->ImageNum].OnNormal;			// Use On-Normal image
		}
		else
		{
			// Is the mouse over the button, and do we have hilite image?
			if( b->Area.uiFlags & MSYS_MOUSE_IN_AREA && gfRenderHilights &&
				  gfLeftButtonState &&
					ButtonPictures[b->ImageNum].OffHilite != -1 )
				UseImage = ButtonPictures[b->ImageNum].OffHilite;			// Use Off-Hilite image
			else if(ButtonPictures[b->ImageNum].OffNormal != -1)
				UseImage = ButtonPictures[b->ImageNum].OffNormal;			// Use Off-Normal image
		}
	}
	else if( ButtonPictures[b->ImageNum].Grayed != -1 )
	{	// Button is disabled so use the "Grayed-out" image
		UseImage = ButtonPictures[b->ImageNum].Grayed;
	}
	else //use the disabled style
	{
		if( b->uiFlags & BUTTON_CLICKED_ON )
			UseImage = ButtonPictures[b->ImageNum].OnHilite;
		else
			UseImage = ButtonPictures[b->ImageNum].OffHilite;
		switch( b->bDisabledStyle )
		{
			case DISABLED_STYLE_DEFAULT:
				gbDisabledButtonStyle = DISABLED_STYLE_HATCHED;
				break;
			case DISABLED_STYLE_HATCHED:
			case DISABLED_STYLE_SHADED:
				gbDisabledButtonStyle = b->bDisabledStyle;
				break;
		}
	}

	// Display the button image
	BltVideoObject(ButtonDestBuffer, ButtonPictures[b->ImageNum].vobj,
								 (UINT16)UseImage, b->XLoc, b->YLoc,
								 VO_BLT_SRCTRANSPARENCY, NULL);
}

// FUNCTION: WIZ8 0x0040e3b0
void DrawIconOnButton(GUI_BUTTON *b)
{
	INT32 xp,yp,width,height,IconX,IconY;
	INT32 IconW,IconH;
	SGPRect NewClip,OldClip;
	ETRLEObject		*pTrav;
	HVOBJECT	hvObject;

	// If there's an actual icon on this button, try to show it.
	if(b->iIconID >= 0)
	{
		// Get width and height of button area
		width = b->Area.RegionBottomRightX - b->Area.RegionTopLeftX;
		height = b->Area.RegionBottomRightY - b->Area.RegionTopLeftY;

		// Compute viewable area (inside borders)
		NewClip.iLeft = b->XLoc + 3;
		NewClip.iRight = b->XLoc + width - 3;
		NewClip.iTop = b->YLoc + 2;
		NewClip.iBottom = b->YLoc + height - 2;

		// Get Icon's blit start coordinates
		IconX = NewClip.iLeft;
		IconY = NewClip.iTop;

		// Get current clip area
		GetClippingRect(&OldClip);

		// Clip button's viewable area coords to screen
		if(NewClip.iLeft < OldClip.iLeft)
			NewClip.iLeft = OldClip.iLeft;

		// Is button right off the right side of the screen?
		if(NewClip.iLeft > OldClip.iRight)
			return;

		if(NewClip.iRight > OldClip.iRight)
			NewClip.iRight = OldClip.iRight;

		// Is button completely off the left side of the screen?
		if(NewClip.iRight < OldClip.iLeft)
			return;

		if(NewClip.iTop < OldClip.iTop)
			NewClip.iTop = OldClip.iTop;

		// Are we right off the bottom of the screen?
		if(NewClip.iTop > OldClip.iBottom)
			return;

		if(NewClip.iBottom > OldClip.iBottom)
			NewClip.iBottom = OldClip.iBottom;

		// Are we off the top?
		if(NewClip.iBottom < OldClip.iTop)
			return;

		// Did we clip the viewable area out of existance?
		if((NewClip.iRight <= NewClip.iLeft) || (NewClip.iBottom <= NewClip.iTop))
			return;

		// Get the width and height of the icon itself
		if( b->uiFlags & BUTTON_GENERIC )
			pTrav = &(GenericButtonIcons[b->iIconID]->pETRLEObject[b->usIconIndex]);
		else
		{
			GetVideoObject( &hvObject, b->iIconID );
			pTrav = &(hvObject->pETRLEObject[b->usIconIndex] );
		}
		IconH = (UINT32)(pTrav->usHeight+pTrav->sOffsetY);
		IconW = (UINT32)(pTrav->usWidth+pTrav->sOffsetX);

		// Compute coordinates for centering the icon on the button or
		// use the offset system.
		if( b->bIconXOffset == -1 )
			xp = (((width-6)-IconW) / 2) + IconX;
		else
			xp = b->Area.RegionTopLeftX + b->bIconXOffset;
		if( b->bIconYOffset == -1 )
			yp = (((height-4)-IconH) / 2) + IconY;
		else
			yp = b->Area.RegionTopLeftY + b->bIconYOffset;

		// Was the button clicked on? if so, move the image slightly for the illusion
		// that the image moved into the screen.
		if( b->uiFlags & BUTTON_CLICKED_ON && b->fShiftImage )
		{
			xp++;
			yp++;
		}

		// Set the clipping rectangle to the viewable area of the button
		SetClippingRect(&NewClip);
		// Blit the icon
		if( b->uiFlags & BUTTON_GENERIC )
			BltVideoObject( ButtonDestBuffer,GenericButtonIcons[b->iIconID], b->usIconIndex, (INT16)xp, (INT16)yp,
										  VO_BLT_SRCTRANSPARENCY, NULL);
		else
			BltVideoObject( ButtonDestBuffer, hvObject, b->usIconIndex, (INT16)xp, (INT16)yp,
				              VO_BLT_SRCTRANSPARENCY, NULL );
		// Restore previous clip region
		SetClippingRect(&OldClip);
	}
}


//If a button has text attached to it, then it'll draw it last.
// FUNCTION: WIZ8 0x0040e5d0
void DrawTextOnButton(GUI_BUTTON *b)
{
	INT32 xp,yp,width,height,TextX,TextY;
	SGPRect NewClip,OldClip;
	INT16	sForeColor;

	// If this button actually has a string to print
	if( b->string )
	{
		// Get the width and height of this button
		width = b->Area.RegionBottomRightX - b->Area.RegionTopLeftX;
		height = b->Area.RegionBottomRightY - b->Area.RegionTopLeftY;

		// Compute the viewable area on this button
		NewClip.iLeft = b->XLoc + 3;
		NewClip.iRight = b->XLoc + width - 3;
		NewClip.iTop = b->YLoc + 2;
		NewClip.iBottom = b->YLoc + height - 2;

		// Get the starting coordinates to print
		TextX = NewClip.iLeft;
		TextY = NewClip.iTop;

		// Get the current clipping area
		GetClippingRect(&OldClip);

		// Clip the button's viewable area to the screen
		if(NewClip.iLeft < OldClip.iLeft)
			NewClip.iLeft = OldClip.iLeft;

		// Are we off hte right side?
		if(NewClip.iLeft > OldClip.iRight)
			return;

		if(NewClip.iRight > OldClip.iRight)
			NewClip.iRight = OldClip.iRight;

		// Are we off the left side?
		if(NewClip.iRight < OldClip.iLeft)
			return;

		if(NewClip.iTop < OldClip.iTop)
			NewClip.iTop = OldClip.iTop;

		// Are we off the bottom of the screen?
		if(NewClip.iTop > OldClip.iBottom)
			return;

		if(NewClip.iBottom > OldClip.iBottom)
			NewClip.iBottom = OldClip.iBottom;

		// Are we off the top?
		if(NewClip.iBottom < OldClip.iTop)
			return;

		// Did we clip the viewable area out of existance?
		if((NewClip.iRight <= NewClip.iLeft) || (NewClip.iBottom <= NewClip.iTop))
			return;

		// Set the font printing settings to the buttons viewable area
		SetFontDestBuffer(ButtonDestBuffer, NewClip.iLeft,
											NewClip.iTop, NewClip.iRight,
											NewClip.iBottom, FALSE);

		// Compute the coordinates to center the text
		if( b->bTextYOffset == -1 )
			yp = (((height) - GetFontHeight(b->usFont)) / 2) + TextY - 1;
		else
			yp = b->Area.RegionTopLeftY + b->bTextYOffset;
		if( b->bTextXOffset == -1 )
		{
			switch( b->bJustification )
			{
				case BUTTON_TEXT_LEFT:
					xp = TextX + 3;
					break;
				case BUTTON_TEXT_RIGHT:
					xp = NewClip.iRight - StringPixLength(b->string, b->usFont) - 3;
					break;
				case BUTTON_TEXT_CENTER:
				default:
					xp = (((width-6)-StringPixLength(b->string,b->usFont)) / 2) + TextX;
					break;
			}
		}
		else
			xp = b->Area.RegionTopLeftX + b->bTextXOffset;

		// Set the printing font to the button text font
		SetFont(b->usFont);

		// print the text
		SetFontBackground( FONT_MCOLOR_BLACK );
		SetFontForeground( (UINT8)b->sForeColor );
		sForeColor = b->sForeColor;
		if( b->sShadowColor != -1 )
			SetFontShadow( (UINT8)b->sShadowColor );
		//Override the colors if necessary.
		if( b->uiFlags & BUTTON_ENABLED && b->Area.uiFlags & MSYS_MOUSE_IN_AREA && b->sForeColorHilited != -1 )
		{
			SetFontForeground( (UINT8)b->sForeColorHilited );
			sForeColor = b->sForeColorHilited;
		}
		else if( b->uiFlags & BUTTON_CLICKED_ON && b->sForeColorDown != -1 )
		{
			SetFontForeground( (UINT8)b->sForeColorDown );
			sForeColor = b->sForeColorDown;
		}
		if( b->uiFlags & BUTTON_ENABLED && b->Area.uiFlags & MSYS_MOUSE_IN_AREA && b->sShadowColorHilited != -1 )
		{
			SetFontShadow( (UINT8)b->sShadowColorHilited );
		}
		else if( b->uiFlags & BUTTON_CLICKED_ON && b->sShadowColorDown != -1 )
		{
			SetFontShadow( (UINT8)b->sShadowColorDown );
		}
		if( b->uiFlags & BUTTON_CLICKED_ON && b->fShiftText )
		{	// Was the button clicked on? if so, move the text slightly for the illusion
			// that the text moved into the screen.
			xp++;
			yp++;
		}
		if(b->fMultiColor)
			gprintf(xp, yp, b->string);
		else
			mprintf(xp, yp, b->string);
		// Restore the old text printing settings
	}
}



//=============================================================================
//	DrawGenericButton
//
//	This function is called by the DrawIconicButton and DrawTextButton
//	routines to draw the borders and background of the buttons.
//
// FUNCTION: WIZ8 0x0040e890
void DrawGenericButton(GUI_BUTTON *b)
{
	INT32 NumChunksWide,NumChunksHigh,cx,cy,width,height,hremain,wremain;
	INT32 q,ImgNum,ox,oy;
	INT32 iBorderHeight, iBorderWidth;
	HVOBJECT BPic;
	UINT32			uiDestPitchBYTES;
	UINT8				*pDestBuf;
	SGPRect			ClipRect;
	ETRLEObject *pTrav;

	// Select the graphics to use depending on the current state of the button
	if( b->uiFlags & BUTTON_ENABLED )
	{
		if ( !(b->uiFlags & BUTTON_ENABLED) && (GenericButtonGrayed[b->ImageNum]==NULL) )
			BPic = GenericButtonOffNormal[b->ImageNum];
		else if(b->uiFlags & BUTTON_CLICKED_ON)
		{
			if((b->Area.uiFlags & MSYS_MOUSE_IN_AREA) && (GenericButtonOnHilite[b->ImageNum]!=NULL) && gfRenderHilights )
				BPic = GenericButtonOnHilite[b->ImageNum];
			else
				BPic = GenericButtonOnNormal[b->ImageNum];
		}
		else
		{
			if((b->Area.uiFlags & MSYS_MOUSE_IN_AREA) && (GenericButtonOffHilite[b->ImageNum]!=NULL) && gfRenderHilights )
				BPic = GenericButtonOffHilite[b->ImageNum];
			else
				BPic = GenericButtonOffNormal[b->ImageNum];
		}
	}
	else if( GenericButtonGrayed[ b->ImageNum ] )
		BPic = GenericButtonGrayed[b->ImageNum];
	else
	{
		BPic = GenericButtonOffNormal[ b->ImageNum ];
		switch( b->bDisabledStyle )
		{
			case DISABLED_STYLE_DEFAULT:
				gbDisabledButtonStyle = b->string ? DISABLED_STYLE_SHADED : DISABLED_STYLE_HATCHED;
				break;
			case DISABLED_STYLE_HATCHED:
			case DISABLED_STYLE_SHADED:
				gbDisabledButtonStyle = b->bDisabledStyle;
				break;
		}
	}

	iBorderWidth=3;
	iBorderHeight=2;
	pTrav=NULL;

// DB - Added this to support more flexible sizing of border images
// The 3x2 size was a bit limiting. JA2 should default to the original
// size, unchanged

	pTrav = &(BPic->pETRLEObject[0] );
	iBorderHeight = (INT32)pTrav->usHeight;
	iBorderWidth = (INT32)pTrav->usWidth;

	// Compute the number of button "chunks" needed to be blitted
	width = b->Area.RegionBottomRightX - b->Area.RegionTopLeftX;
	height = b->Area.RegionBottomRightY - b->Area.RegionTopLeftY;
	NumChunksWide = width / iBorderWidth;
	NumChunksHigh = height / iBorderHeight;
	hremain = height % iBorderHeight;
	wremain = width % iBorderWidth;

	cx = (b->XLoc + ((NumChunksWide-1)*iBorderWidth) + wremain);
	cy = (b->YLoc + ((NumChunksHigh-1)*iBorderHeight) + hremain);

	// Fill the button's area with the button's background color
	ColorFillVideoSurfaceArea(ButtonDestBuffer,b->Area.RegionTopLeftX,
																						 b->Area.RegionTopLeftY,
																						 b->Area.RegionBottomRightX,
																						 b->Area.RegionBottomRightY,
																						 GenericButtonFillColors[b->ImageNum]);

	// If there is a background image, fill the button's area with it
	if(GenericButtonBackground[b->ImageNum]!=NULL)
	{
		ox=oy=0;
		// if the button was clicked on, adjust the background image so that we get
		// the illusion that it is sunk into the screen.
		if(b->uiFlags & BUTTON_CLICKED_ON)
			ox=oy=1;

		// Fill the area with the image, tilling it if need be.
		ImageFillVideoSurfaceArea(ButtonDestBuffer,b->Area.RegionTopLeftX+ox,
																							 b->Area.RegionTopLeftY+oy,
																							 b->Area.RegionBottomRightX,
																							 b->Area.RegionBottomRightY,
																							 GenericButtonBackground[b->ImageNum],
																							 GenericButtonBackgroundIndex[b->ImageNum],
																							 GenericButtonOffsetX[b->ImageNum],
																							 GenericButtonOffsetY[b->ImageNum]);
	}

	// Lock the dest buffer
	pDestBuf = LockVideoSurface( ButtonDestBuffer, &uiDestPitchBYTES );

	GetClippingRect(&ClipRect);

	// Draw the button's borders and corners (horizontally)
	for(q=0;q<NumChunksWide;q++)
	{
		if(q==0)
			ImgNum=0;
		else
			ImgNum=1;

		if(gbPixelDepth==16)
		{
			Blt8BPPDataTo16BPPBufferTransparentClip( (UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											(INT32)(b->XLoc + (q*iBorderWidth)),
											(INT32)b->YLoc,
											(UINT16)ImgNum, &ClipRect );
		}
		else if(gbPixelDepth==8)
		{
			Blt8BPPDataTo8BPPBufferTransparentClip( (UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											(INT32)(b->XLoc + (q*iBorderWidth)),
											(INT32)b->YLoc,
											(UINT16)ImgNum, &ClipRect );
		}

		if(q==0)
			ImgNum=5;
		else
			ImgNum=6;

		if(gbPixelDepth==16)
		{
			Blt8BPPDataTo16BPPBufferTransparentClip( (UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											(INT32)(b->XLoc + (q*iBorderWidth)),
											cy, (UINT16)ImgNum, &ClipRect );
		}
		else if(gbPixelDepth==8)
		{
			Blt8BPPDataTo8BPPBufferTransparentClip((UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											(INT32)(b->XLoc + (q*iBorderWidth)),
											cy, (UINT16)ImgNum, &ClipRect );
		}

	}
	// Blit the right side corners
		if(gbPixelDepth==16)
		{
			Blt8BPPDataTo16BPPBufferTransparentClip( (UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											cx, (INT32)b->YLoc,
											2, &ClipRect );
		}
		else if(gbPixelDepth==8)
		{
			Blt8BPPDataTo8BPPBufferTransparentClip( (UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											cx, (INT32)b->YLoc,
											2, &ClipRect );
		}


		if(gbPixelDepth==16)
		{
			Blt8BPPDataTo16BPPBufferTransparentClip( (UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											cx, cy, 7, &ClipRect );
		}
		else if(gbPixelDepth==8)
		{
			Blt8BPPDataTo8BPPBufferTransparentClip( (UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											cx, cy, 7, &ClipRect );
		}
	// Draw the vertical members of the button's borders
	NumChunksHigh--;

	if(hremain!=0)
	{
		q=NumChunksHigh;
		if(gbPixelDepth==16)
		{
			Blt8BPPDataTo16BPPBufferTransparentClip( (UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											(INT32)b->XLoc,
											(INT32)(b->YLoc + (q*iBorderHeight) - (iBorderHeight-hremain)),
											3, &ClipRect );
		}
		else if(gbPixelDepth==8)
		{
			Blt8BPPDataTo8BPPBufferTransparentClip( (UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											(INT32)b->XLoc,
											(INT32)(b->YLoc + (q*iBorderHeight) - (iBorderHeight-hremain)),
											3, &ClipRect );
		}

		if(gbPixelDepth==16)
		{
			Blt8BPPDataTo16BPPBufferTransparentClip( (UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											cx,	(INT32)(b->YLoc + (q*iBorderHeight) - (iBorderHeight-hremain)),
											4, &ClipRect );
		}
		else if(gbPixelDepth==8)
		{
			Blt8BPPDataTo8BPPBufferTransparentClip((UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											cx,	(INT32)(b->YLoc + (q*iBorderHeight) - (iBorderHeight-hremain)),
											4, &ClipRect );
		}
	}

	for(q=1;q<NumChunksHigh;q++)
	{
		if(gbPixelDepth==16)
		{
			Blt8BPPDataTo16BPPBufferTransparentClip( (UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											(INT32)b->XLoc,
											(INT32)(b->YLoc + (q*iBorderHeight)),
											3, &ClipRect );
		}
		else if(gbPixelDepth==8)
		{
			Blt8BPPDataTo8BPPBufferTransparentClip((UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											(INT32)b->XLoc,
											(INT32)(b->YLoc + (q*iBorderHeight)),
											3, &ClipRect );
		}

		if(gbPixelDepth==16)
		{
			Blt8BPPDataTo16BPPBufferTransparentClip( (UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											cx,	(INT32)(b->YLoc + (q*iBorderHeight)),
											4, &ClipRect );
		}
		else if(gbPixelDepth==8)
		{
			Blt8BPPDataTo8BPPBufferTransparentClip((UINT16*)pDestBuf,
											uiDestPitchBYTES, BPic,
											cx,	(INT32)(b->YLoc + (q*iBorderHeight)),
											4, &ClipRect );
		}
	}

	// Unlock buffer
	UnLockVideoSurface( ButtonDestBuffer );

}





//=======================================================================================================
// Dialog box code
//


//=======================================================================================================
//=======================================================================================================
//
//	Very preliminary stuff follows
//
//=======================================================================================================
//=======================================================================================================



typedef struct _CreateDlgInfo {
	INT32			iFlags;						// Holds the creation flags

	INT32			iPosX;						// Screen position of dialog box
	INT32			iPosY;
	INT32			iWidth;						// Dimensions of dialog box (if needed)
	INT32			iHeight;

	INT32			iAreaWidth;				// Dimensions of area the dialog box will be
	INT32			iAreaHeight;			// placed (for auto-sizing and auto-placement
	INT32			iAreaOffsetX;			// only)
	INT32			iAreaOffsetY;

	UINT16		*zDlgText;				// Text to be displayed (if any)
	INT32			iTextFont;				// Font to be used for text (if any)
	UINT16		usTextCols;				// Font colors (for mono fonts only)

	INT32			iTextAreaX;				// Area in dialog box where text is to be
	INT32			iTextAreaY;				// put (for non-auto placed text)
	INT32			iTextAreaWidth;
	INT32			iTextAreaHeight;

	HVOBJECT	hBackImg;					// Background pic for dialog box (if any)
	INT32			iBackImgIndex;		// Sub-image index to use for image
	INT32			iBackOffsetX;			// Offset on dialog box where to put image
	INT32			iBackOffsetY;

	HVOBJECT	hIconImg;					// Icon image pic and index.
	INT32			iIconImgIndex;
	INT32			iIconPosX;
	INT32			iIconPosY;

	INT32			iBtnTypes;

	INT32			iOkPosX;					// Ok button info
	INT32			iOkPosY;
	INT32			iOkWidth;
	INT32			iOkHeight;
	INT32			iOkImg;

	INT32			iCnclPosX;				// Cancel button info
	INT32			iCnclPosY;
	INT32			iCnclWidth;
	INT32			iCnclHeight;
	INT32			iCnclImg;
} CreateDlgInfo;

#define DLG_RESTRICT_MOUSE				1
#define DLG_OK_BUTTON							2
#define DLG_CANCEL_BUTTON					4
#define DLG_AUTOSIZE							8
#define DLG_RECREATE							16
#define DLG_AUTOPOSITION					32
#define DLG_TEXT_IN_AREA					64
#define DLG_USE_BKGRND_IMAGE			128
#define DLG_USE_BORDERS						256
#define DLG_USE_BTN_HOTSPOTS			512
#define DLG_USE_MONO_FONTS				1024
#define DLG_IS_ACTIVE							2048

#define DLG_MANUAL_RENDER					0
#define DLG_START_RENDER					1
#define DLG_STOP_RENDER						2
#define DLG_AUTO_RENDER						3

#define DLG_GET_STATUS						0
#define DLG_WAIT_FOR_RESPONSE			1

#define DLG_STATUS_NONE						0
#define DLG_STATUS_OK							1
#define DLG_STATUS_CANCEL					2
#define DLG_STATUS_PENDING				3

#define DLG_CLEARALL							0
#define DLG_POSITION							1
#define DLG_AREA									2
#define DLG_TEXT									3
#define DLG_TEXTAREA							4
#define DLG_BACKPIC								5
#define DLG_ICON									6
#define DLG_OKBUTTON							7
#define DLG_CANCELBUTTON					8
#define DLG_OPTIONS								9
#define DLG_SIZE									10





//------------------------------------------------------------------------------------------------------



// Added Oct17, 97 Carter - kind of mindless, but might as well have it
// FUNCTION: WIZ8 0x0040edc0
void MSYS_SetBtnUserData(INT32 iButtonNum,INT32 index,INT32 userdata)
{
  GUI_BUTTON *b;
	b=ButtonList[iButtonNum];
	if(index < 0 || index > 3)
		return;
	b->UserData[index]=userdata;
}

// FUNCTION: WIZ8 0x0040edf0
INT32 MSYS_GetBtnUserData(GUI_BUTTON *b,INT32 index)
{


	if(index < 0 || index > 3)
		return(0);

	return(b->UserData[index]);
}


//Generic Button Movement Callback to reset the mouse button if the mouse is no longer
//in the button region.
// FUNCTION: WIZ8 0x0040ee10
void BtnGenericMouseMoveButtonCallback(GUI_BUTTON *btn,INT32 reason)
{
	//If the button isn't the anchored button, then we don't want to modify the button state.
	if( btn != gpAnchoredButton )
		return;
	if( reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		if( !gfAnchoredState )
		{
			btn->uiFlags &= (~BUTTON_CLICKED_ON );
			if(	btn->ubSoundSchemeID )
			{
				PlayButtonSound( btn->IDNum, BUTTON_SOUND_CLICKED_OFF );
			}
		}
	}
	else if( reason & MSYS_CALLBACK_REASON_GAIN_MOUSE )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON ;
		if( btn->ubSoundSchemeID )
		{
			PlayButtonSound( btn->IDNum, BUTTON_SOUND_CLICKED_ON );
		}
	}
}


//Kris:
//Yet new logical additions to the winbart library.
// FUNCTION: WIZ8 0x0040ee80
void HideButton( INT32 iButtonNum )
{
	GUI_BUTTON *b;

	Assert( iButtonNum >= 0 );
	Assert( iButtonNum < MAX_BUTTONS);

	b = ButtonList[ iButtonNum ];

	Assert( b );

	b->Area.uiFlags &= (~MSYS_REGION_ENABLED);
	b->uiFlags |= BUTTON_DIRTY;
}

// FUNCTION: WIZ8 0x0040eea0
void ShowButton( INT32 iButtonNum )
{
	GUI_BUTTON *b;

	Assert( iButtonNum >= 0 );
	Assert( iButtonNum < MAX_BUTTONS);

	b = ButtonList[ iButtonNum ];

	Assert( b );

	b->Area.uiFlags |= MSYS_REGION_ENABLED;
	b->uiFlags |= BUTTON_DIRTY;
}

// FUNCTION: WIZ8 0x0040eec0
BOOLEAN GetButtonArea(INT32 iButtonID, SGPRect *pRect)
{
GUI_BUTTON *b;

	Assert( iButtonID >= 0 );
	Assert( iButtonID < MAX_BUTTONS);
	Assert(pRect);

	b = ButtonList[ iButtonID ];
	Assert( b );

	if((pRect==NULL) || (b==NULL))
		return(FALSE);

	pRect->iLeft=b->Area.RegionTopLeftX;
	pRect->iTop=b->Area.RegionTopLeftY;
	pRect->iRight=b->Area.RegionBottomRightX;
	pRect->iBottom=b->Area.RegionBottomRightY;

	return(TRUE);
}

// FUNCTION: WIZ8 0x0040ef00
INT32 GetButtonWidth(INT32 iButtonID)
{
GUI_BUTTON *b;

	Assert( iButtonID >= 0 );
	Assert( iButtonID < MAX_BUTTONS);

	b = ButtonList[ iButtonID ];
	Assert( b );

	if(b==NULL)
		return(-1);

	return(b->Area.RegionBottomRightX - b->Area.RegionTopLeftX);
}

// FUNCTION: WIZ8 0x0040ef20
INT32 GetButtonHeight(INT32 iButtonID)
{
GUI_BUTTON *b;

	Assert( iButtonID >= 0 );
	Assert( iButtonID < MAX_BUTTONS);

	b = ButtonList[ iButtonID ];
	Assert( b );

	if(b==NULL)
		return(-1);

	return(b->Area.RegionBottomRightY - b->Area.RegionTopLeftY);
}

// FUNCTION: WIZ8 0x0040ef40
INT32 GetButtonX(INT32 iButtonID)
{
GUI_BUTTON *b;

	Assert( iButtonID >= 0 );
	Assert( iButtonID < MAX_BUTTONS);

	b = ButtonList[ iButtonID ];
	Assert( b );

	if(b==NULL)
		return(0);

	return(b->Area.RegionTopLeftX);
}

// FUNCTION: WIZ8 0x0040ef60
INT32 GetButtonY(INT32 iButtonID)
{
GUI_BUTTON *b;

	Assert( iButtonID >= 0 );
	Assert( iButtonID < MAX_BUTTONS);

	b = ButtonList[ iButtonID ];
	Assert( b );

	if(b==NULL)
		return(0);

	return(b->Area.RegionTopLeftY);
}
