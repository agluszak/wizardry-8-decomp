/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Restrict JA2-only includes to their product branch.
   Collapse the released JA2, utility, and precompiled-header branches to the Wizardry build.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "types.h"
#include "Button System.h"
#include "Button Sound Control.h"

// FUNCTION: WIZ8 0x00413fb0
void SpecifyButtonSoundScheme( INT32 iButtonID, INT8 bSoundScheme )
{
	ButtonList[ iButtonID ]->ubSoundSchemeID = (UINT8)bSoundScheme;
	if( bSoundScheme == BUTTON_SOUND_SCHEME_GENERIC )
	{
		if( bSoundScheme == BUTTON_SOUND_SCHEME_GENERIC )
			bSoundScheme = BUTTON_SOUND_SCHEME_NONE;
	}
}

void PlayButtonSound( INT32 iButtonID, INT32 iSoundType )
{
	if ( ButtonList[ iButtonID ] == NULL )
	{
		return;
	}

	switch( ButtonList[ iButtonID ]->ubSoundSchemeID )
	{
		case BUTTON_SOUND_SCHEME_NONE:
		case BUTTON_SOUND_SCHEME_GENERIC:
			break;


	}

}
