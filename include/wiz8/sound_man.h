#ifndef WIZ8_SOUND_MAN_H
#define WIZ8_SOUND_MAN_H

/* Wizardry's modified SGP sound manager. The bodies retained byte-for-byte
   from the pinned SFI SGP soundman.c oracle keep their linked identities in
   sgp_runtime_adapters.cpp; the functions retail modified live here, next to
   the canonical channel and sample tables they share with the recovered
   InitializeSoundManager gate. */

#include "soundman.h"
#include "wiz8/wiz8_windows.h"

/* Retail 0x006E4120: the 32 output-channel slots InitializeSoundManager
   clears, shared with the modified playback bodies below. */
extern SOUNDTAG g_sound_channels_6e4120[32];
/* Retail 0x006E4AA0: the 128 cached-sample slots, shared the same way. */
extern SAMPLETAG g_sound_samples_6e4aa0[128];

/* Retail 0x006E4104: the Miles digital driver handle the channel startup
   path allocates samples from. */
extern HDIGDRIVER g_sound_driver_6e4104;

/* Wizardry's replacement for SoundGetUniqueID's static counter. */
extern unsigned int g_sound_id_counter_650e64;

int PlaySound00408860(const char* path, int* options);
int SoundLoadDisk00409970(const char* path);
unsigned char SoundInitHardware00409C50(void);
void SoundResetChannel00409F30(int channel);
int SoundStartSample00409fe0(int sample, int channel, int* options);
int SoundStopIndex0040a5c0(int channel);

#endif
