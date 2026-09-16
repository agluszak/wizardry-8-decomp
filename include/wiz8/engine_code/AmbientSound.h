#pragma once

#include "FileMan.h"

struct W8World;

#include "surrender/srMath.h"
#include "wiz8/engine_code/game_timer.h"

struct W8AmbientSoundConfig {
    char wave_name[0x80];
};

/* Engine Code\AmbientSound.cpp. The allocation and release pair establishes
   the complete 0x12c-byte object and four members used by its lifetime. */
class W8AmbientSound {
public:
    W8AmbientSound();  /* 0x00479040 */
    ~W8AmbientSound(); /* 0x0047A780 */

    /* 0x004790D0: true when listener is inside the rotated/scaled min-max region */
    unsigned char IsInsideRegion(const srVector3T<float>* listener);
    /* 0x00479350: listener-relative range test, group handoff, 3D re-aim, fade retarget */
    void UpdatePosition(const srVector3T<float>* listener);
    /* 0x00479970: per-frame service while in range; 'entered' forces an immediate roll */
    void Service(unsigned char entered);
    /* 0x0047A310: step the 1.5-second shared-group volume fade */
    void UpdateFade();

    W8AmbientSound* FindNextMatching0047A260(const char* match_name, W8AmbientSound* previous);

    char* pacSoundName;              /* 0x000: assertion-backed at 0x47A790 */
    W8AmbientSoundConfig config_004; /* 0x004: wave filename; shared sounds match on it */
    unsigned char stopped;           /* 0x084: script-stopped via the ByName commands */
    unsigned char unknown_085[3];
    srVector3T<float> position;  /* 0x088: emitter world position */
    unsigned int volume_min;     /* 0x094 */
    unsigned int volume_max;     /* 0x098 */
    unsigned int target_volume;  /* 0x09c: fade endpoint */
    unsigned int current_volume; /* 0x0a0: live fade value */
    int speed_min;               /* 0x0a4: playback speed low bound */
    int speed_max;               /* 0x0a8: playback speed high bound */
    int time_min;                /* 0x0ac: random retrigger interval, low ms */
    int time_max;                /* 0x0b0: random retrigger interval, high ms */
    float radius;                /* 0x0b4: audible range in world units */
    unsigned char in_range;      /* 0x0b8: listener currently inside radius/region */
    unsigned char looping;       /* 0x0b9: continuous loop vs random one-shot */
    unsigned char unknown_0ba[2];
    int sound_handle;      /* 0x0bc: live SGP voice id */
    int sample_handle;     /* 0x0c0: registered SGP random sample */
    unsigned char shared;  /* 0x0c4: shared 2D group sound; voices hand off by name */
    unsigned char bounded; /* 0x0c5: region test enabled (v2 trigger field) */
    unsigned char unknown_0c6[2];
    srVector3T<float> region_min;    /* 0x0c8 */
    srVector3T<float> region_max;    /* 0x0d4 */
    srVector3T<float> region_center; /* 0x0e0 */
    float region_angle;              /* 0x0ec: rotation about region_axis, radians */
    srVector3T<float> region_axis;   /* 0x0f0 */
    srVector3T<float> region_scale;  /* 0x0fc: per-axis region scale */
    W8GameTimer fade_timer;          /* 0x108 */
};

static_assert(sizeof(W8AmbientSound) == 0x12c, "W8AmbientSound_must_be_0x12c");

/* Reverb environment id indexing g_footstep_surfaces_609eb8; retail's range
   check admits 9, one past the table's last entry (the ids are byte-sized in
   the record, so the parameters stay `char`). */
enum W8FootstepSurface {
    W8_FOOTSTEP_SURFACE_NONE = 0,
    W8_FOOTSTEP_SURFACE_SMALL_CAVE = 1,
    W8_FOOTSTEP_SURFACE_MEDIUM_CAVE = 2,
    W8_FOOTSTEP_SURFACE_LARGE_CAVE = 3,
    W8_FOOTSTEP_SURFACE_SMALL_ROOM = 4,
    W8_FOOTSTEP_SURFACE_MEDIUM_ROOM = 5,
    W8_FOOTSTEP_SURFACE_LARGE_ROOM = 6,
    W8_FOOTSTEP_SURFACE_OUTDOORS_FLAT = 7,
    W8_FOOTSTEP_SURFACE_OUTDOORS_CANYON = 8,
    W8_FOOTSTEP_SURFACE_MAX = 9,
};

/* Step material id indexing g_footstep_names_609edc; ids at and above
   CLIMB_LADDER bypass the per-surface naming path and play a single
   "Step_<name>.WAV". */
enum W8FootstepMaterial {
    W8_FOOTSTEP_MATERIAL_NONE = 0,
    W8_FOOTSTEP_MATERIAL_GRITTY = 1,
    W8_FOOTSTEP_MATERIAL_GRASS = 2,
    W8_FOOTSTEP_MATERIAL_STONE = 3,
    W8_FOOTSTEP_MATERIAL_SHALLOW_WATER = 4,
    W8_FOOTSTEP_MATERIAL_CREAKY_WOOD = 5,
    W8_FOOTSTEP_MATERIAL_SOLID_WOOD = 6,
    W8_FOOTSTEP_MATERIAL_HOLLOW_WOOD = 7,
    W8_FOOTSTEP_MATERIAL_METAL = 8,
    W8_FOOTSTEP_MATERIAL_GRAVEL = 9,
    W8_FOOTSTEP_MATERIAL_ROUGH_STONE = 10,
    W8_FOOTSTEP_MATERIAL_MARBLE = 11,
    W8_FOOTSTEP_MATERIAL_MUD = 12,
    W8_FOOTSTEP_MATERIAL_SAND = 13,
    W8_FOOTSTEP_MATERIAL_LEAVES = 14,
    W8_FOOTSTEP_MATERIAL_SNOW = 15,
    W8_FOOTSTEP_MATERIAL_CARPET = 16,
    W8_FOOTSTEP_MATERIAL_MAGIC = 17,
    W8_FOOTSTEP_MATERIAL_CLIMB_LADDER = 18,
    W8_FOOTSTEP_MATERIAL_CLIMB_ROCK = 19,
    W8_FOOTSTEP_MATERIAL_CLIMB_ROPE = 20,
    W8_FOOTSTEP_MATERIAL_SWIM_SURFACE = 21,
    W8_FOOTSTEP_MATERIAL_SWIM_UNDERWATER = 22,
    W8_FOOTSTEP_MATERIAL_CRAWL = 23,
    W8_FOOTSTEP_MATERIAL_FLY = 24,
    W8_FOOTSTEP_MATERIAL_MAX = 25,
};

/* Which footstep wave shape PlayFootstep builds: an ordinary numbered step, a
   single jump, or a numbered scuff. */
enum W8FootstepKind {
    W8_FOOTSTEP_KIND_STEP = 0,
    W8_FOOTSTEP_KIND_JUMP = 1,
    W8_FOOTSTEP_KIND_SCUFF = 2,
};

/* 0x00479030: stop all random samples and the shared ambient priority group */
void StopAllAmbientSounds();

void BuildFootstepPath0047A540(char* path, char surface, char material, char kind, int variant);
int PlayFootstep0047A440(char surface, char material, int kind);
void UpdateAmbientSounds0047A3E0(W8World* world);
void RepositionAmbientSounds0047A600(W8World* world);
unsigned char LoadAmbientSoundList0047AB40(char* filename);

void PositionAmbientSoundByName0047A950(int /* unused */, const char* name);
void StopAmbientSoundByName0047A9E0(int /* unused */, const char* name);
void ToggleAmbientSoundByName0047AA70(int /* unused */, const char* name);
void DestroyAmbientSound0047A700(W8AmbientSound* ambient);
unsigned char AddAmbientSound0047A790(W8World* world, const char* name,
                                      const W8AmbientSoundConfig* config,
                                      const srVector3T<float>* position,
                                      const srVector3T<float>* region_min,
                                      const srVector3T<float>* region_max, int volume_min,
                                      int volume_max, int time_min, int time_max, int speed_min,
                                      int speed_max, float radius, unsigned char looping,
                                      unsigned char bounded, const srVector3T<float>* region_center,
                                      float region_angle, const srVector3T<float>* region_axis,
                                      const srVector3T<float>* region_scale, unsigned char shared);

void SaveAmbientSoundList0047B140(HWFILE handle);
void LoadAmbientSoundList0047B270(HWFILE handle);

bool IsSoundEffectsMuted(void);
unsigned char GetSoundEffectsVolume(void);
void SetSoundEffectsVolume0047AD00(unsigned char volume);
void SetSoundEffectsMuted(unsigned char muted);

extern unsigned char g_default_footstep_surface_65a108;
extern unsigned char g_default_footstep_material_65a109;
extern unsigned char g_footstep_alternate_65a10a;
extern int g_previous_footstep_variant_65a10c;
extern const char* g_footstep_names_609edc[];
extern const char* g_footstep_surfaces_609eb8[];
/* Zeroed 128-byte ambient name image; only its first word is ever read — a
   `memcpy` of it followed by a memset seeds an empty fixed-size name. */
extern unsigned short g_empty_ambient_name_65a110;
