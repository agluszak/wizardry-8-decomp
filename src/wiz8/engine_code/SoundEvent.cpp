#include "wiz8/engine_code/SoundEvent.h"
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"
#include "random.h"
#include "soundman.h"

#include <math.h>
#include <new>
#include <string.h>

#define SOUNDEVENT_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\SoundEvent.cpp"

/* Releases the two things a sound event owns: the copied wave name, and the
   playing handle, but only when the event was created with the flag that says
   it started one. */
// FUNCTION: WIZ8 0x004d5770
W8SoundEvent::~W8SoundEvent()
{
    if (m_pacWaveName != 0) {
        delete[] m_pacWaveName;
    }
    if (flag_025 != 0 && handle_020 != -1) {
        SoundStop(handle_020);
    }
}

/* The wave name is copied into storage the event owns and its destructor
   releases. */
// FUNCTION: WIZ8 0x004d57a0
W8SoundEvent* CreateSoundEvent004D57A0(int value_000, int value_004, int value_008, int value_00c,
                                       const char* wave_name, unsigned char flag_025)
{
    W8SoundEvent* pSndEvent = new W8SoundEvent();

    if (pSndEvent == 0) {
        srAssertFail("pSndEvent", SOUNDEVENT_CPP, 0x70, "SoundEvent: Out of memory");
    }
    pSndEvent->value_000 = value_000;
    pSndEvent->value_004 = value_004;
    pSndEvent->value_008 = value_008;
    pSndEvent->value_00c = value_00c;
    pSndEvent->m_pacWaveName = new char[strlen(wave_name) + 1];
    pSndEvent->flag_025 = flag_025;
    if (pSndEvent->m_pacWaveName == 0) {
        srAssertFail("pSndEvent->m_pacWaveName", SOUNDEVENT_CPP, 0x79, "SoundEvent: Out of memory");
    }
    strcpy(pSndEvent->m_pacWaveName, wave_name);
    return pSndEvent;
}

/* Candidate list, chosen index, and previous choice. The list is reused for
   every mask bit, so it is cleared before each scan instead of reallocated. */
// GLOBAL: WIZ8 0x00683408
W8GrowableVector<W8SoundEvent*> g_sound_event_candidates_00683408;

// GLOBAL: WIZ8 0x00683418
int g_selected_sound_event_00683418;

// GLOBAL: WIZ8 0x0061095c
int g_last_sound_event_0061095c = 0x1869f;

// GLOBAL: WIZ8 0x00683420
static int g_previous_footstep_variant_00683420;

/* For every bit of the caller's mask, append the events of that kind whose
   own classification matches, then play one of them at random without
   repeating the previous choice for as long as a second candidate exists. */
// FUNCTION: WIZ8 0x004d5890
unsigned char UpdateSoundEvents004D5890(W8GrowableVector<W8SoundEvent*>* events,
                                        const srVector3T<float>* position, unsigned int event_mask,
                                        int cycle, unsigned int frame, int subcycle)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    unsigned int bit = 1;

    if (events == 0) {
        return 0;
    }
    int count = events->GetCount();
    if (count == 0) {
        return 0;
    }
    do {
        if ((bit & event_mask) != 0) {
            int candidate_count = 0;
            int index;

            g_sound_event_candidates_00683408.Clear();
            for (index = 0; index < count; ++index) {
                W8SoundEvent* event = *events->GetAt(index);
                unsigned int kind = event->value_000;

                if ((bit & kind) == 0 || (event_mask & kind) == 0) {
                    continue;
                }
                if (kind == 1 || kind == 0x100) {
                    if ((event->value_004 != -1 && event->value_004 != cycle) ||
                        event->value_008 != frame || event->value_00c != subcycle) {
                        continue;
                    }
                } else if (kind == 2) {
                    if (event->value_004 != cycle || event->value_00c != subcycle) {
                        continue;
                    }
                }
                g_sound_event_candidates_00683408.Add(event);
                candidate_count = g_sound_event_candidates_00683408.GetCount();
            }
            if (candidate_count != 0) {
                W8SoundEvent* selected;

                do {
                    g_selected_sound_event_00683418 = (int)Random(candidate_count);
                    if (g_sound_event_candidates_00683408.GetCount() < 2) {
                        break;
                    }
                    candidate_count = g_sound_event_candidates_00683408.GetCount();
                } while (g_selected_sound_event_00683418 == g_last_sound_event_0061095c);
                selected =
                    *g_sound_event_candidates_00683408.GetAt(g_selected_sound_event_00683418);
                if (selected->Play004D5A10(event_mask, position, cycle, frame, subcycle)) {
                    g_last_sound_event_0061095c = g_selected_sound_event_00683418;
                }
            }
        }
        bit <<= 1;
    } while (bit != 0);
    return 1;
#pragma clang diagnostic pop
}

/* Resolve one event into a positional 3D play: decide the base volume from
   the event's own routing, rotate the listener offset into the camera frame,
   and let the ground/step branch rebuild the wave name before the play. */
// FUNCTION: WIZ8 0x004d5a10
unsigned char W8SoundEvent::Play004D5A10(unsigned int mask, const srVector3T<float>* position,
                                         int cycle, unsigned int frame, int subcycle)
{
    W8Monster* monster = 0;
    bool track_sound = false;

    if (IsAmbientSoundMuted() || !Chance(value_024)) {
        return 0;
    }
    float angle = -GetCameraYawRadians();
    unsigned int base_volume = 0x78;
    srVector3T<float> camera_position;

    GetCameraPosition(&camera_position);
    srVector3T<float> camera_offset(camera_position.x - position->x,
                                    camera_position.y - position->y,
                                    camera_position.z - position->z);
    if (camera_offset.Length() < value_02c) {
        if (value_000 == 0x100 || strstr(m_pacWaveName, "step") != 0 ||
            (value_028 != 0 && cycle == 4)) {
            base_volume = value_030;
            track_sound = true;
            if (value_028 != 0) {
                monster = GetMonsterByLocationID(value_028);
                if (gXStatus.fCombatMode != 0 || monster->flag_218 != 0) {
                    base_volume = value_034;
                }
            }
        } else if (value_018 == 0 && value_01c == 0) {
            if (strchr(m_pacWaveName, '+') != 0) {
                base_volume = 0x87;
            }
        } else {
            base_volume = (unsigned int)Random(value_01c - value_018 + 1) + value_018;
        }

        srMatrix3T<float> rotation;

        rotation.SetIdentity();
        if ((double)angle != g_zero_005ebb40) {
            rotation.RotateAboutY(sin(angle), cos(angle));
        }

        srVector3T<float> offset(position->x - camera_position.x, position->y - camera_position.y,
                                 position->z - camera_position.z);
        srVector3T<float> transformed = rotation.Transform(offset);
        float x = transformed.x;
        float y = transformed.y;
        float z = transformed.z;
        SOUND3DPARMS options;

        memset(&options, 0xff, sizeof(options));
        unsigned int event_volume =
            (unsigned int)((g_float_005ebb38 - camera_offset.Length() / value_02c) * base_volume);
        event_volume = (GetFlag6850F6() * event_volume) / 0x7f;
        if (event_volume != 0) {
            options.uiVolume = event_volume;
            options.uiLoop = (flag_025 == 0);
            options.Pos.flX = x;
            options.Pos.flY = y;
            options.Pos.flZ = z;
            options.Pos.flVelX = 0.0f;
            options.Pos.flVelY = 0.0f;
            options.Pos.flVelZ = 0.0f;
            options.Pos.flFaceX = -x;
            options.Pos.flFaceY = -y;
            options.Pos.flFaceZ = -z;
            options.Pos.flUpX = 0.0f;
            options.Pos.flUpY = g_float_005ebb38;
            options.Pos.flUpZ = 0.0f;
            options.Pos.flFalloffMin = value_02c;
            options.Pos.flFalloffMax = value_02c;
            options.Pos.uiVolume = event_volume;

            if (value_000 == 0x100) {
                if (position == 0) {
                    srAssertFail("vPos", SOUNDEVENT_CPP, 0x121, 0);
                }
                int variant;
                int attempts = 0;
                do {
                    variant = (int)Random(4) + 1;
                    ++attempts;
                    if (variant != g_previous_footstep_variant_00683420) {
                        break;
                    }
                } while (attempts < 100);
                g_previous_footstep_variant_00683420 = variant;

                char path[260];
                char surface;
                char material;
                Function420CA0(position, &surface, &material);
                BuildFootstepPath0047A540(path, 7, material, 0, variant);
                delete[] m_pacWaveName;
                m_pacWaveName = new char[strlen(path) + 1];
                strcpy(m_pacWaveName, path);
            }
            char* marker = strchr(m_pacWaveName, '!');
            options.uiPitchBend = marker != 0 ? 0 : 20;
            marker = strchr(m_pacWaveName, '+');
            if (marker != 0) {
                options.Pos.flFalloffMax += options.Pos.flFalloffMax;
                options.Pos.flFalloffMin += options.Pos.flFalloffMin;
            }
            handle_020 = Sound3DPlay(m_pacWaveName, &options);
            if (handle_020 == -1) {
                return 0;
            }
            if (track_sound && monster != 0) {
                monster->TrackSoundHandle004CA6E0(handle_020);
            }
        }
    }
    return 1;
}
