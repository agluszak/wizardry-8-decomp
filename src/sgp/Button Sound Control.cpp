/* Modified for the Wizardry 8 reconstruction: 2026-10-03, 2026-10-06, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "Types.h"
#include "Button System.h"
#include "Button Sound Control.h"

// FUNCTION: WIZ8 0x00413fb0
void SpecifyButtonSoundScheme(INT32 iButtonID, INT8 bSoundScheme)
{
    ButtonList[iButtonID]->ubSoundSchemeID = (UINT8)bSoundScheme;
    if (bSoundScheme == BUTTON_SOUND_SCHEME_GENERIC) {
        if (bSoundScheme == BUTTON_SOUND_SCHEME_GENERIC)
            bSoundScheme = BUTTON_SOUND_SCHEME_NONE;
    }
}

void PlayButtonSound(INT32 iButtonID, INT32 iSoundType)
{
    if (ButtonList[iButtonID] == NULL) {
        return;
    }

    switch (ButtonList[iButtonID]->ubSoundSchemeID) {
    case BUTTON_SOUND_SCHEME_NONE:
    case BUTTON_SOUND_SCHEME_GENERIC:
        break;
    }
}
