#pragma once

#include "soundman.h"

/* Storage owned by released SGP soundman.c and shared with Wizardry's
   product-specific sound bodies. The released public header does not expose
   these module globals. */
extern "C" {
extern UINT32 guiSoundMemoryLimit;
extern UINT32 guiSoundMemoryUsed;
extern HDIGDRIVER hSoundDriver;
extern BOOLEAN fDirectSound;
extern BOOLEAN fSoundSystemInit;
extern SAMPLETAG pSampleList[128];
extern SOUNDTAG pSoundList[32];
extern CHAR8* gpProviderName;
extern HPROVIDER gh3DProvider;
extern H3DPOBJECT gh3DListener;
extern BOOLEAN gfUsingEAX;
extern UINT32 guiRoomTypeIndex;

UINT32 SoundGetUniqueID(void);
}
