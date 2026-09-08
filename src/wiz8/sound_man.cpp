#include "wiz8/sound_man.h"

#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include "Mss.h"
#include "random.h"
#include "soundman.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern unsigned char g_flag_650e50;
extern unsigned int g_sound_memory_used_650e4c;
extern unsigned int g_sound_memory_limit_5ff648;
extern unsigned int guiSoundCacheThreshold;
extern unsigned int guiSoundDefaultVolume;

unsigned int g_sound_id_counter_650e64;
HDIGDRIVER g_sound_driver_6e4104;
/* Retail 0x005FF650: direct-sound attempt flag, set for the first driver
   round and cleared before the wave-out fallback. */
unsigned char g_direct_sound_5ff650;

// FUNCTION: WIZ8 0x0041a7f0
unsigned char Function41A7F0(void)
{
    atexit((void (__cdecl*)(void))AIL_shutdown);
    return 0;
}

/* AIL_digital_configuration leaves the queried status in eax, and retail
   stores that word as the driver handle and branches on it. The pinned
   header declares it void, so the call below reads eax through a
   compatible prototype; the import decoration is return-type independent. */
typedef unsigned int (__stdcall *DigitalConfigurationFn)(
    HDIGDRIVER dig, int rate, int format, char* string);
#define DigitalConfiguration \
    ((DigitalConfigurationFn)AIL_digital_configuration)

/* Inlined SoundPlayStreamed: samples at or above the streaming threshold
   never enter the cache. */
// FUNCTION: WIZ8 0x00408860
int PlaySound00408860(const char* path, int* options)
{
    char name[260];
    int sample;
    int channel;
    int index;

    if (g_flag_650e50 == 0) {
        return -1;
    }
    int handle = FileOpen((STR)path, 1, 0);
    if (handle != 0) {
        unsigned int size = FileGetSize(handle);
        CloseVirtualFile(handle);
        if (guiSoundCacheThreshold <= size) {
            return -1;
        }
    }
    strcpy(name, path);
    _strupr(name);
    if (FileExists(name) == 0) {
        if (strstr(name, ".WAV") != 0) {
            name[strlen(name) - 4] = 0;
            strcat(name, ".MP3");
        }
        else if (strstr(name, ".MP3") != 0) {
            name[strlen(name) - 4] = 0;
            strcat(name, ".WAV");
        }
    }

    /* Inlined SoundLoadSample/SoundGetCached: the cache search keys on the
       canonicalized name. */
    sample = -1;
    for (index = 0; index < 128; ++index) {
        if (_stricmp(g_sound_samples_6e4aa0[index].pName, name) == 0) {
            sample = index;
            break;
        }
    }
    if (sample == -1) {
        sample = SoundLoadDisk00409970(name);
    }

    /* Inlined SoundGetFreeChannel/SoundIsPlaying/SoundIndexIsPlaying. */
    if (sample != -1) {
        for (channel = 0; channel < 32; ++channel) {
            if (g_flag_650e50 != 0) {
                int found = -1;
                for (index = 0; index < 32; ++index) {
                    if (g_sound_channels_6e4120[index].uiSoundID ==
                        g_sound_channels_6e4120[channel].uiSoundID) {
                        found = index;
                        break;
                    }
                }
                if (found == -1) {
                    SoundStopIndex0040a5c0(channel);
                }
                else {
                    SOUNDTAG* occupant = &g_sound_channels_6e4120[found];
                    int status = SMP_DONE;
                    if (occupant->hMSS != 0) {
                        status = AIL_sample_status(occupant->hMSS);
                    }
                    if (occupant->hMSSStream != 0) {
                        status = AIL_stream_status(occupant->hMSSStream);
                    }
                    if (occupant->hM3D != 0) {
                        status = AIL_3D_sample_status(occupant->hM3D);
                    }
                    if (status == SMP_DONE || status == SMP_STOPPED) {
                        SoundStopIndex0040a5c0(channel);
                    }
                }
            }
            else {
                SoundStopIndex0040a5c0(channel);
            }
            if (g_sound_channels_6e4120[channel].hMSS == 0 &&
                g_sound_channels_6e4120[channel].hMSSStream == 0 &&
                g_sound_channels_6e4120[channel].hM3D == 0) {
                SoundResetChannel00409F30(channel);
                if (channel != -1) {
                    return SoundStartSample00409fe0(sample, channel, options);
                }
                return -1;
            }
        }
    }
    return -1;
}

/* Removes the least-used unlocked sample that no channel is playing. The
   eviction scan repeats SoundFreeSampleIndex's body inline; both resistance
   loops below are the compiler's rotation of SoundLoadDisk's cleanup
   sequence. */
// FUNCTION: WIZ8 0x00409970
int SoundLoadDisk00409970(const char* path)
{
    int handle = FileOpen((STR)path, 1, 0);
    int sample;
    int index;
    int probe;
    SAMPLETAG* slot;
    if (handle == 0) {
        return -1;
    }
    unsigned int size = FileGetSize(handle);
    unsigned char removed = 1;
    while (g_sound_memory_limit_5ff648 < g_sound_memory_used_650e4c + size &&
           removed != 0) {
        int best = -1;
        unsigned int best_hits = 0;
        for (index = 0; index < 128; ++index) {
            SAMPLETAG* entry = &g_sound_samples_6e4aa0[index];
            if ((entry->uiFlags & SAMPLE_ALLOCATED) != 0 &&
                (entry->uiFlags & SAMPLE_LOCKED) == 0 &&
                (best == -1 || best_hits < entry->uiCacheHits)) {
                int playing = 0;
                for (probe = 0; probe < 32; ++probe) {
                    if (g_sound_channels_6e4120[probe].uiSample ==
                        static_cast<unsigned int>(index)) {
                        playing = 1;
                        break;
                    }
                }
                if (!playing) {
                    best = index;
                    best_hits = entry->uiCacheHits;
                }
            }
        }
        if (best == -1) {
            removed = 0;
        }
        else {
            slot = &g_sound_samples_6e4aa0[best];
            if ((slot->uiFlags & SAMPLE_ALLOCATED) != 0) {
                if (slot->pData != 0) {
                    g_sound_memory_used_650e4c -= slot->uiSize;
                    AIL_mem_free_lock(slot->pData);
                }
                memset(slot, 0, sizeof(*slot));
            }
            removed = 1;
        }
    }
    if (g_sound_memory_used_650e4c + size > g_sound_memory_limit_5ff648) {
        CloseVirtualFile(handle);
        return -1;
    }
    sample = -1;
    for (index = 0; index < 128; ++index) {
        if ((g_sound_samples_6e4aa0[index].uiFlags & SAMPLE_ALLOCATED) ==
            0) {
            sample = index;
            break;
        }
    }
    if (sample == -1) {
        int best = -1;
        unsigned int best_hits = 0;
        for (index = 0; index < 128; ++index) {
            SAMPLETAG* entry = &g_sound_samples_6e4aa0[index];
            if ((entry->uiFlags & SAMPLE_ALLOCATED) != 0 &&
                (entry->uiFlags & SAMPLE_LOCKED) == 0 &&
                (best == -1 || best_hits < entry->uiCacheHits)) {
                int playing = 0;
                for (probe = 0; probe < 32; ++probe) {
                    if (g_sound_channels_6e4120[probe].uiSample ==
                        static_cast<unsigned int>(index)) {
                        playing = 1;
                        break;
                    }
                }
                if (!playing) {
                    best = index;
                    best_hits = entry->uiCacheHits;
                }
            }
        }
        sample = best;
        if (sample == -1) {
            CloseVirtualFile(handle);
            return -1;
        }
        slot = &g_sound_samples_6e4aa0[sample];
        if ((slot->uiFlags & SAMPLE_ALLOCATED) != 0) {
            if (slot->pData != 0) {
                g_sound_memory_used_650e4c -= slot->uiSize;
                AIL_mem_free_lock(slot->pData);
            }
            memset(slot, 0, sizeof(*slot));
        }
        sample = -1;
        for (index = 0; index < 128; ++index) {
            if ((g_sound_samples_6e4aa0[index].uiFlags & SAMPLE_ALLOCATED) ==
                0) {
                sample = index;
                break;
            }
        }
        if (sample == -1) {
            CloseVirtualFile(handle);
            return -1;
        }
    }

    slot = &g_sound_samples_6e4aa0[sample];
    memset(slot, 0, sizeof(*slot));
    slot->pData = AIL_mem_alloc_lock(size);
    if (slot->pData == 0) {
        CloseVirtualFile(handle);
        return -1;
    }
    g_sound_memory_used_650e4c += size;
    ReadVirtualFile(handle, slot->pData, size, 0);
    CloseVirtualFile(handle);
    strcpy(slot->pName, path);
    _strupr(slot->pName);
    slot->uiSize = size;
    slot->uiFlags |= SAMPLE_ALLOCATED;
    return sample;
}

/* Reinitializes one output-channel slot before SoundStartSample claims it. */
// FUNCTION: WIZ8 0x00409f30
void SoundResetChannel00409F30(int channel)
{
    g_sound_channels_6e4120[channel].pSample = 0;
    g_sound_channels_6e4120[channel].uiSample = NO_SAMPLE;
    g_sound_channels_6e4120[channel].hMSS = 0;
    g_sound_channels_6e4120[channel].hMSSStream = 0;
    g_sound_channels_6e4120[channel].hM3D = 0;
    g_sound_channels_6e4120[channel].uiFlags = 0;
    g_sound_channels_6e4120[channel].uiSoundID = NO_SAMPLE;
    g_sound_channels_6e4120[channel].uiPriority = PRIORITY_MAX;
    g_sound_channels_6e4120[channel].pCallback = 0;
    g_sound_channels_6e4120[channel].pData = 0;
    g_sound_channels_6e4120[channel].EOSCallback = 0;
    g_sound_channels_6e4120[channel].pCallbackData = 0;
    g_sound_channels_6e4120[channel].uiTimeStamp = GetTickCount();
    g_sound_channels_6e4120[channel].fLooping = 0;
    g_sound_channels_6e4120[channel].hFile = 0xffffffff;
    g_sound_channels_6e4120[channel].fMusic = 0;
    g_sound_channels_6e4120[channel].fStopAtZero = 1;
    g_sound_channels_6e4120[channel].uiFadeVolume = 0;
    g_sound_channels_6e4120[channel].uiFadeRate = 0;
    g_sound_channels_6e4120[channel].uiFadeTime = 0;
}

// FUNCTION: WIZ8 0x00409fe0
int SoundStartSample00409fe0(int sample, int channel, int* options)
{
    char error[200];
    SOUNDTAG* slot;
    SAMPLETAG* entry;
    int volume;

    if (g_flag_650e50 == 0) {
        return -1;
    }
    slot = &g_sound_channels_6e4120[channel];
    slot->hMSS = AIL_allocate_sample_handle(g_sound_driver_6e4104);
    if (slot->hMSS == 0) {
        sprintf(error, "Sample Error: %s", AIL_last_error());
        return -1;
    }
    AIL_init_sample(slot->hMSS);
    entry = &g_sound_samples_6e4aa0[sample];
    if (AIL_set_named_sample_file(
            slot->hMSS, entry->pName, entry->pData, entry->uiSize,
            0) == 0) {
        AIL_release_sample_handle(slot->hMSS);
        slot->hMSS = 0;
        sprintf(error, "AIL Set Sample Error: %s", AIL_last_error());
        return -1;
    }
    entry->uiSpeed = AIL_sample_playback_rate(slot->hMSS);
    if ((entry->uiFlags & SAMPLE_RANDOM) == 0) {
        if (options != 0 && options[0] != -1) {
            AIL_set_sample_playback_rate(slot->hMSS, options[0]);
        }
    }
    else if (entry->uiSpeedMin != NO_SAMPLE) {
        AIL_set_sample_playback_rate(
            slot->hMSS,
            entry->uiSpeedMin +
                Random(entry->uiSpeedMax - entry->uiSpeedMin));
    }
    if (options == 0) {
        volume = guiSoundDefaultVolume;
    }
    else {
        if (options[1] != -1) {
            unsigned int natural = AIL_sample_playback_rate(slot->hMSS);
            unsigned int bend =
                static_cast<unsigned int>(options[1]) * natural / 100;
            AIL_set_sample_playback_rate(
                slot->hMSS, Random(bend * 2) + natural - bend);
        }
        volume = options[2];
        if (volume == -1) {
            volume = guiSoundDefaultVolume;
        }
    }
    AIL_set_sample_volume(slot->hMSS, volume);
    slot->uiFadeVolume = volume;
    if (options == 0) {
        slot->uiPriority = PRIORITY_MAX;
    }
    else {
        if (options[4] != -1) {
            AIL_set_sample_loop_count(slot->hMSS, options[4]);
            if (options[4] == 0) {
                slot->fLooping = 1;
                entry->uiFlags |= SAMPLE_LOCKED;
            }
        }
        if (options[3] != -1) {
            AIL_set_sample_pan(slot->hMSS, options[3]);
        }
        if (options[5] == -1) {
            slot->uiPriority = PRIORITY_MAX;
        }
        else {
            slot->uiPriority = options[5];
        }
    }
    if (options == 0 || options[6] == -1) {
        slot->EOSCallback = 0;
        slot->pCallbackData = 0;
    }
    else {
        slot->EOSCallback =
            reinterpret_cast<void (*)(void*)>(options[6]);
        slot->pCallbackData = reinterpret_cast<void*>(options[7]);
    }
    int sound_id = g_sound_id_counter_650e64;
    if (sound_id == -1) {
        sound_id = 0;
    }
    g_sound_id_counter_650e64 = sound_id + 1;
    slot->uiSoundID = sound_id;
    slot->uiSample = sample;
    slot->uiTimeStamp = GetTickCount();
    unsigned int fade;
    if (g_flag_650e50 != 0) {
        if (slot->hMSS != 0) {
            fade = AIL_sample_volume(slot->hMSS);
        }
        else if (slot->hMSSStream != 0) {
            fade = AIL_stream_volume(slot->hMSSStream);
        }
        else if (slot->hM3D != 0) {
            fade = AIL_3D_sample_volume(slot->hM3D);
        }
        else {
            fade = SOUND_ERROR;
        }
    }
    else {
        fade = SOUND_ERROR;
    }
    slot->uiFadeVolume = fade;
    slot->fMusic = 0;
    entry->uiCacheHits += 1;
    AIL_start_sample(slot->hMSS);
    return sound_id;
}

// FUNCTION: WIZ8 0x0040a5c0
int SoundStopIndex0040a5c0(int channel)
{
    SOUNDTAG* slot;
    int sample;
    int index;
    int probe;
    int found;
    int status;
    int in_use;

    if (g_flag_650e50 == 0 || channel == -1) {
        return 0;
    }
    slot = &g_sound_channels_6e4120[channel];
    if (slot->hMSS != 0) {
        AIL_stop_sample(slot->hMSS);
        AIL_release_sample_handle(slot->hMSS);
        slot->hMSS = 0;
        sample = slot->uiSample;
        if ((g_sound_samples_6e4aa0[sample].uiFlags & SAMPLE_RANDOM) != 0) {
            g_sound_samples_6e4aa0[sample].uiInstances -= 1;
        }
        if (slot->EOSCallback != 0) {
            slot->EOSCallback(slot->pCallbackData);
        }
        /* Inlined SoundSampleIsInUse/SoundIsPlaying/SoundIndexIsPlaying:
           another channel still rendering this sample keeps the lock. */
        if (slot->fLooping != 0) {
            in_use = 0;
            for (index = 0; index < 32 && in_use == 0; ++index) {
                if (g_sound_channels_6e4120[index].uiSample ==
                        static_cast<unsigned int>(channel) &&
                    g_flag_650e50 != 0) {
                    found = -1;
                    for (probe = 0; probe < 32; ++probe) {
                        if (g_sound_channels_6e4120[probe].uiSoundID ==
                            static_cast<unsigned int>(index)) {
                            found = probe;
                            break;
                        }
                    }
                    if (found != -1) {
                        SOUNDTAG* occupant =
                            &g_sound_channels_6e4120[found];
                        status = SMP_DONE;
                        if (occupant->hMSS != 0) {
                            status = AIL_sample_status(occupant->hMSS);
                        }
                        if (occupant->hMSSStream != 0) {
                            status = AIL_stream_status(occupant->hMSSStream);
                        }
                        if (occupant->hM3D != 0) {
                            status =
                                AIL_3D_sample_status(occupant->hM3D);
                        }
                        if (status != SMP_DONE && status != SMP_STOPPED) {
                            in_use = 1;
                        }
                    }
                }
            }
            if (in_use == 0 &&
                (g_sound_samples_6e4aa0[sample].uiFlags &
                 SAMPLE_ALLOCATED) != 0) {
                g_sound_samples_6e4aa0[sample].uiFlags &= ~SAMPLE_LOCKED;
            }
        }
        slot->uiSample = NO_SAMPLE;
    }
    if (slot->hMSSStream != 0) {
        AIL_close_stream(slot->hMSSStream);
        slot->hMSSStream = 0;
        if (slot->EOSCallback != 0) {
            slot->EOSCallback(slot->pCallbackData);
        }
        slot->uiSample = NO_SAMPLE;
    }
    if (slot->hM3D != 0) {
        AIL_stop_3D_sample(slot->hM3D);
        AIL_release_3D_sample_handle(slot->hM3D);
        slot->hM3D = 0;
        sample = slot->uiSample;
        if ((g_sound_samples_6e4aa0[sample].uiFlags & SAMPLE_RANDOM) != 0) {
            g_sound_samples_6e4aa0[sample].uiInstances -= 1;
        }
        if (slot->EOSCallback != 0) {
            slot->EOSCallback(slot->pCallbackData);
        }
        if (slot->fLooping != 0) {
            in_use = 0;
            for (index = 0; index < 32 && in_use == 0; ++index) {
                if (g_sound_channels_6e4120[index].uiSample ==
                        static_cast<unsigned int>(channel) &&
                    g_flag_650e50 != 0) {
                    found = -1;
                    for (probe = 0; probe < 32; ++probe) {
                        if (g_sound_channels_6e4120[probe].uiSoundID ==
                            static_cast<unsigned int>(index)) {
                            found = probe;
                            break;
                        }
                    }
                    if (found != -1) {
                        SOUNDTAG* occupant =
                            &g_sound_channels_6e4120[found];
                        status = SMP_DONE;
                        if (occupant->hMSS != 0) {
                            status = AIL_sample_status(occupant->hMSS);
                        }
                        if (occupant->hMSSStream != 0) {
                            status = AIL_stream_status(occupant->hMSSStream);
                        }
                        if (occupant->hM3D != 0) {
                            status =
                                AIL_3D_sample_status(occupant->hM3D);
                        }
                        if (status != SMP_DONE && status != SMP_STOPPED) {
                            in_use = 1;
                        }
                    }
                }
            }
            if (in_use == 0 &&
                (g_sound_samples_6e4aa0[sample].uiFlags &
                 SAMPLE_ALLOCATED) != 0) {
                g_sound_samples_6e4aa0[sample].uiFlags &= ~SAMPLE_LOCKED;
            }
        }
        slot->uiSample = NO_SAMPLE;
    }
    if (slot->hFile != 0xffffffff) {
        CloseHandle((void*)slot->hFile);
        slot->hFile = 0xffffffff;
        slot->uiSample = NO_SAMPLE;
    }
    slot->fMusic = 0;
    return 1;
}

/* Brings up the Miles driver, preferring 44 kHz 16-bit stereo DirectSound
   and falling back through three cheaper formats, then to wave-out. An
   emulated driver is rejected outright. */
// FUNCTION: WIZ8 0x00409c50
unsigned char SoundInitHardware00409C50(void)
{
    HDIGDRIVER driver;
    unsigned int config[32];
    char name[128];
    int index;
    unsigned int* slot;

    Function41A7F0();
    if (AIL_startup() == 0) {
        return 0;
    }
    g_sound_driver_6e4104 = 0;
    AIL_set_preference(DIG_MIXER_CHANNELS, 32);
    g_direct_sound_5ff650 = 1;
    AIL_set_preference(DIG_USE_WAVEOUT, 0);
    driver = AIL_open_digital_driver(44100, 16, 2, 0);
    if (driver == 0) {
        driver = 0;
    }
    else {
        slot = config;
        for (index = 0x20; index != 0; --index) {
            *slot++ = 0;
        }
        driver = (HDIGDRIVER)DigitalConfiguration(
            driver, 0, 0, (char*)config);
    }
    g_sound_driver_6e4104 = driver;
    if (driver != 0) {
        goto fetch_config;
    }
    driver = AIL_open_digital_driver(44100, 8, 2, 0);
    if (driver == 0) {
        driver = 0;
    }
    else {
        slot = config;
        for (index = 0x20; index != 0; --index) {
            *slot++ = 0;
        }
        driver = (HDIGDRIVER)DigitalConfiguration(
            driver, 0, 0, (char*)config);
    }
    g_sound_driver_6e4104 = driver;
    if (driver != 0) {
        goto fetch_config;
    }
    driver = AIL_open_digital_driver(22050, 8, 2, 0);
    if (driver == 0) {
        driver = 0;
    }
    else {
        slot = config;
        for (index = 0x20; index != 0; --index) {
            *slot++ = 0;
        }
        driver = (HDIGDRIVER)DigitalConfiguration(
            driver, 0, 0, (char*)config);
    }
    g_sound_driver_6e4104 = driver;
    if (driver != 0) {
        goto fetch_config;
    }
    driver = AIL_open_digital_driver(11025, 8, 1, 0);
    if (driver == 0) {
        driver = 0;
    }
    else {
        slot = config;
        for (index = 0x20; index != 0; --index) {
            *slot++ = 0;
        }
        driver = (HDIGDRIVER)DigitalConfiguration(
            driver, 0, 0, (char*)config);
    }
    g_sound_driver_6e4104 = driver;
    if (driver == 0) {
        goto wave_out;
    }
fetch_config:
    name[0] = 0;
    AIL_digital_configuration(g_sound_driver_6e4104, 0, 0, name);
    _strlwr(name);
    if (strstr(name, "emulated") == 0) {
        goto started;
    }
    AIL_close_digital_driver(g_sound_driver_6e4104);
    g_sound_driver_6e4104 = 0;
wave_out:
    g_direct_sound_5ff650 = 0;
    AIL_set_preference(DIG_USE_WAVEOUT, 1);
    if (g_sound_driver_6e4104 == 0) {
        driver = AIL_open_digital_driver(44100, 16, 2, 0);
        if (driver == 0) {
            driver = 0;
        }
        else {
            slot = config;
            for (index = 0x20; index != 0; --index) {
                *slot++ = 0;
            }
            AIL_digital_configuration(driver, 0, 0, (char*)config);
        }
        g_sound_driver_6e4104 = driver;
        if (driver == 0) {
            driver = AIL_open_digital_driver(44100, 8, 2, 0);
            if (driver == 0) {
                driver = 0;
            }
            else {
                slot = config;
                for (index = 0x20; index != 0; --index) {
                    *slot++ = 0;
                }
                AIL_digital_configuration(driver, 0, 0, (char*)config);
            }
            g_sound_driver_6e4104 = driver;
            if (driver == 0) {
                driver = AIL_open_digital_driver(22050, 8, 2, 0);
                if (driver == 0) {
                    driver = 0;
                }
                else {
                    slot = config;
                    for (index = 0x20; index != 0; --index) {
                        *slot++ = 0;
                    }
                    AIL_digital_configuration(driver, 0, 0, (char*)config);
                }
                g_sound_driver_6e4104 = driver;
                if (driver == 0) {
                    driver = AIL_open_digital_driver(11025, 8, 1, 0);
                    if (driver == 0) {
                        driver = 0;
                    }
                    else {
                        slot = config;
                        for (index = 0x20; index != 0; --index) {
                            *slot++ = 0;
                        }
                        AIL_digital_configuration(
                            driver, 0, 0, (char*)config);
                    }
                    g_sound_driver_6e4104 = driver;
                    if (driver == 0) {
                        return 0;
                    }
                }
            }
        }
    }
started:
    memset(g_sound_channels_6e4120, 0, sizeof(g_sound_channels_6e4120));
    return 1;
}
