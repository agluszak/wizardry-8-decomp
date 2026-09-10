#ifndef WIZ8_SOUND_MAN_H
#define WIZ8_SOUND_MAN_H

/* Wizardry's modified SGP sound-manager bodies. Released storage and retained
   bodies remain owned by the pinned SFI SGP soundman.c. */

#include "soundman.h"
#include "wiz8/wiz8_windows.h"

int PlaySound00408860(const char* path, int* options);
void Function4098F0(void);
int SoundLoadDisk00409970(const char* path);
unsigned char SoundInitHardware00409C50(void);
void SoundResetChannel00409F30(int channel);
int SoundStartSample00409fe0(int sample, int channel, int* options);
int SoundStopIndex0040a5c0(int channel);
void ConfigureSoundCache(void);

#endif
