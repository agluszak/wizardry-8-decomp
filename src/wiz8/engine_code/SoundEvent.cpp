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
#include "wiz8/engine_code/GameData.h"

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
    if (looping != 0 && sound_handle != -1) {
        SoundStop(sound_handle);
    }
}

/* The wave name is copied into storage the event owns and its destructor
   releases. */
// FUNCTION: WIZ8 0x004d57a0
W8SoundEvent* CreateSoundEvent(int kind, int cycle, int frame, int subcycle, const char* wave_name,
                               unsigned char looping)
{
    W8SoundEvent* pSndEvent = new W8SoundEvent();

    if (pSndEvent == 0) {
        srAssertFail("pSndEvent", SOUNDEVENT_CPP, 0x70, "SoundEvent: Out of memory");
    }
    pSndEvent->kind = kind;
    pSndEvent->cycle = cycle;
    pSndEvent->frame = frame;
    pSndEvent->subcycle = subcycle;
    pSndEvent->m_pacWaveName = new char[strlen(wave_name) + 1];
    pSndEvent->looping = looping;
    if (pSndEvent->m_pacWaveName == 0) {
        srAssertFail("pSndEvent->m_pacWaveName", SOUNDEVENT_CPP, 0x79, "SoundEvent: Out of memory");
    }
    strcpy(pSndEvent->m_pacWaveName, wave_name);
    return pSndEvent;
}

/* Candidate list, chosen index, and previous choice. The list is reused for
   every mask bit, so it is cleared before each scan instead of reallocated;
   the static initializer constructs it with capacity five. */
// GLOBAL: WIZ8 0x00683408
W8GrowableVector<W8SoundEvent*> g_sound_event_candidates_00683408(5);

/* The static initializer above emits this specialization's capacity ctor.
   0x005ED098 is its construction-phase table; the final table and both
   deleting destructors carry their markers in GrObject.cpp. */
// TEMPLATE: WIZ8 0x004D6030
// W8GrowableVector<W8SoundEvent*>::W8GrowableVector

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
unsigned char UpdateSoundEvents(W8GrowableVector<W8SoundEvent*>* events,
                                const srVector3T<float>* position, unsigned int event_mask,
                                int cycle, int frame, int subcycle)
{
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
                unsigned int kind = event->kind;

                if ((bit & kind) == 0 || (event_mask & kind) == 0) {
                    continue;
                }
                if (kind == W8_SOUND_EVENT_FRAME || kind == W8_SOUND_EVENT_FOOTSTEP) {
                    if ((event->cycle != -1 && event->cycle != cycle) || event->frame != frame ||
                        event->subcycle != subcycle) {
                        continue;
                    }
                } else if (kind == W8_SOUND_EVENT_CYCLE) {
                    if (event->cycle != cycle || event->subcycle != subcycle) {
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
                if (selected->Play(event_mask, position, cycle, frame, subcycle)) {
                    g_last_sound_event_0061095c = g_selected_sound_event_00683418;
                }
            }
        }
        bit <<= 1;
    } while (bit != 0);
    return 1;
}

/* Resolve one event into a positional 3D play: decide the base volume from
   the event's own routing, rotate the listener offset into the camera frame,
   and let the ground/step branch rebuild the wave name before the play. */
// FUNCTION: WIZ8 0x004d5a10
unsigned char W8SoundEvent::Play(unsigned int mask, const srVector3T<float>* position, int cycle,
                                 unsigned int frame, int subcycle)
{
    W8Monster* monster = 0;
    bool track_sound = false;

    if (IsSoundEffectsMuted() || !Chance(probability)) {
        return 0;
    }
    float angle = -GetCameraYawRadians();
    unsigned int base_volume = 0x78;
    srVector3T<float> camera_position;

    GetCameraPosition(&camera_position);
    srVector3T<float> camera_offset = camera_position - *position;
    if (camera_offset.Length() < falloff) {
        if (kind == W8_SOUND_EVENT_FOOTSTEP || strstr(m_pacWaveName, "step") != 0 ||
            (location_id != 0 && cycle == 4)) {
            base_volume = footstep_volume;
            track_sound = true;
            if (location_id != 0) {
                monster = GetMonsterByLocationID(location_id);
                if (gXStatus.fCombatMode != 0 || monster->nearest_to_party_218 != 0) {
                    base_volume = footstep_combat_volume;
                }
            }
        } else if (volume_min == 0 && volume_max == 0) {
            if (strchr(m_pacWaveName, '+') != 0) {
                base_volume = 0x87;
            }
        } else {
            base_volume =
                static_cast<unsigned int>(Random(volume_max - volume_min + 1)) + volume_min;
        }

        srMatrix3T<float> rotation;

        rotation.SetIdentity();
        if ((double)angle != g_zero_005ebb40) {
            rotation.RotateAboutY(sin(angle), cos(angle));
        }

        srVector3T<float> offset = *position - camera_position;
        srVector3T<float> transformed = rotation.Transform(offset);
        float x = transformed.x;
        float y = transformed.y;
        float z = transformed.z;
        SOUND3DPARMS options;

        memset(&options, 0xff, sizeof(options));
        unsigned int event_volume = static_cast<unsigned int>(
            (g_float_005ebb38 - camera_offset.Length() / falloff) * base_volume);
        event_volume = (GetSoundEffectsVolume() * event_volume) / 0x7f;
        if (event_volume != 0) {
            options.uiVolume = event_volume;
            options.uiLoop = (looping == 0);
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
            options.Pos.flFalloffMin = falloff;
            options.Pos.flFalloffMax = falloff;
            options.Pos.uiVolume = event_volume;

            if (kind == W8_SOUND_EVENT_FOOTSTEP) {
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
                GetGroundSurfaceInfo(position, &surface, &material);
                /* Retail hardcodes the surface to OutdoorsFlat here: the query
                   result that matters for the path is the material. */
                BuildFootstepPath0047A540(path, W8_FOOTSTEP_SURFACE_OUTDOORS_FLAT, material,
                                          W8_FOOTSTEP_KIND_STEP, variant);
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
            sound_handle = Sound3DPlay(m_pacWaveName, &options);
            if (sound_handle == -1) {
                return 0;
            }
            if (track_sound && monster != 0) {
                monster->TrackSoundHandle004CA6E0(sound_handle);
            }
        }
    }
    return 1;
}
