#pragma once

/* Reads g_environment_lighting_mode: 2 while the day/night cycle
   runs, 1 while a lighting transition is in progress, 0 once a transition has
   faded the world out. The only retail caller is in NPC Scripting, so the
   accessor keeps its address name until that owner can be renamed with it. */
int GetEnvironmentValue0060A3A8(void);
unsigned char GetEnvironmentFlag0060A394(void); /* 0x00482A10 */

#include "Types.h"
#include "surrender/srVertexProcessor.h"
#include "wiz8/layouts/world.h"
#include "wiz8/vector.h"

class stTextureAnim;

/* Environment.cpp. The vertex processor that scrolls the first texture
   coordinate set of the sky's AnimatedCloudMaterial by a fixed per-frame
   rate. No authored name survives, so the constructor address names the
   class; the global instance 0x00659738 is defined by the level loader. */
// VTABLE: WIZ8 0x005EC8F8
class W8MaterialMapper : public srVertexProcessor {
public:
    W8MaterialMapper();
    /* Retail ICF folds this onto W8NormalTexcoordMapper's deleting
       destructor at 0x004B8A50. */

    virtual ~W8MaterialMapper() override {}
    virtual int isActive(srVertexPipe& pipe) override;
    virtual void process(srVertexPipe& pipe) override;

    float scroll_rate_u_04;      /* 0x04 */
    float scroll_rate_v_08;      /* 0x08 */
    unsigned char padding_0c[8]; /* 0x0c */
    float offset_14;             /* 0x14 */
    float offset_18;             /* 0x18 */
};

static_assert(sizeof(W8MaterialMapper) == 0x1c, "W8MaterialMapper00482010_must_be_0x1c");

/* ABS 0x0065AD78: the three light-direction words as one colour triple.
   Produced from the day-phase colour table and consumed as fog-vector
   floats; the word copies below move it without reinterpreting it. */
extern EnvironmentColour g_light_direction;

extern EnvironmentColour g_environment_colours_65a178[256];
extern EnvironmentColour g_environment_colours_65ad98[256];

class stLight;
void AddEnvironmentLight(stLight* light);

BOOLEAN ReadLightColourTable00482F90(int hFile);
BOOLEAN ReadEnvironmentColourTable004830D0(int hFile);
void BuildEnvironmentColourRamp(void);
void BuildLightColourRamp(void);
void UpdateEnvironmentLight(void);
void RefreshEnvironment(void);
void SetWorldEnvironmentColour(W8World* world, EnvironmentColour colour);

void SetSkyEnabled(bool enabled);
void EnableSky(void);
void SetFogEnabled(bool enabled);
bool IsFogEnabled(void);
void DisableSky(void);
bool IsSkyEnabled(void);
/* The ambient light the world contributes as an EnvironmentColour. The
   Automap caller already stores the result in that type; Video2 converts
   to srVector3T<float> at its own boundary. */
void GetWorldLightValue(const W8World* world, EnvironmentColour* pLightValue);
void SetLightDirection(const EnvironmentColour* direction);
void GetLightDirection(EnvironmentColour* direction);
extern bool g_sky_enabled;
void ResetEnvironment(void);
void InitializeLevelEnvironment00482410(void);
/* Drop the sky gradients and celestial props, release both environment
   objects and empty the registered-light list; called by world teardown. */
void ClearEnvironmentObjects(void);
/* Apply one day-phase colour and intensity to the world, its environment
   lights, and the animated cloud material. */
void ApplyEnvironmentColour00483BA0(W8World* world, float intensity,
                                    const EnvironmentColour* colour);
void ReleaseEnvironmentObjects(void);

/* Turn the environment clock on or off; enabling resets its tick baseline. */
void SetEnvironmentTimeEnabled(bool enabled);
float GetViewDistance(void);
void SetGameTimeMilliseconds(int value);
/* The per-frame environment update, and the alternate lighting transition it
   hands off to when the bypass value is set. */
void UpdateEnvironment(void);
void UpdateEnvironmentLighting(void);
float GetWorldValue24(const W8World* world);
/* Show or hide the world's camera light: 0x00483E50 loads g_world->camera_light
   (offset 0x54) and clears srNode::FLAG_DISABLE when visible, sets it when not.
   Nothing here is a sky node; the exported name is still the placeholder and one
   caller lives in a TU owned elsewhere, so the rename waits for it. */
void SetSkyNodeVisible(bool visible);
void SetCameraLightIntensity(float value);
/* Mode 0 raises the camera light's intensity, 1 lowers it, 2 disables the
   node, 3 re-enables it. */
void SetCameraLightMode(int mode);
void RefreshFogRanges(void);

void SetViewDistance(float distance);
unsigned char InitializeEnvironmentColours(void);
extern unsigned int g_frame_tick;
extern float g_frame_elapsed;

/* The environment's lighting mode: 2 while the per-frame day/night cycle runs,
   1 while a colour transition is in progress and 0 once one has faded the world
   out. Retail writes it only from the transition bodies at 0x00483FD0 and
   0x00484300 and reads it from UpdateEnvironment; the exported accessor
   above still carries the address name. */
extern int g_environment_lighting_mode;
extern bool g_fog_enabled;
/* 1/duration while the lighting transition body at 0x00484300 runs, zero when
   idle: UpdateEnvironment hands off to that body while it is not zero. */
extern float g_environment_transition_rate;
extern unsigned long g_environment_transition_tick;
/* Last day phase the light direction was published from. */
extern int g_last_light_phase;
/* Last day phase the world's environment colour was refreshed from. */
extern int g_last_environment_colour_phase;
/* Gates the per-frame world-colour refresh. Retail writes it nowhere and
   initialises it to 1, so that refresh always runs; the only reference is the
   read inside UpdateEnvironment. */
extern unsigned char g_environment_colour_refresh;
/* The game clock's multiplier, not a distance: every environment clock advance
   scales the elapsed milliseconds by it (12.0 normally, 2880.0 while the party
   rests) and the environment update is skipped while it holds another value.
   Sight.cpp and GameplayTime.cpp consume it through SetViewDistance and
   GetViewDistance, so the rename waits for those owners. */
extern float g_view_distance;
extern bool g_environment_time_enabled;
/* Half the Sun-to-Moon distance, which is the radius of the circle the active
   celestial prop travels; -1 until InitializeLevelEnvironment measures it. */
extern float g_celestial_orbit_radius;
/* The level's "Moon" prop. */
extern W8Prop* g_moon_prop;
/* The level's "Sun" prop. */
extern W8Prop* g_sun_prop;
/* The three day-phase sky gradients the level setup registers:
   SkyGrad0000.ifl, Skytop0000.ifl and Horizon0000.ifl. The registration loop
   and the per-frame animation both walk one contiguous base, so they are one
   array rather than three globals. */
extern stTextureAnim* g_sky_gradient_animations[3];
/* Midpoint of the Sun and Moon, which the active prop orbits and the inactive
   one rests at. */
extern srVector3T<float> g_celestial_origin;

/* The registered environment lights, ambient-filled by ApplyEnvironmentColour.
   The vector count occupies 0x0065B99C. */
extern W8GrowableVector<stLight*> g_environment_lights;

void SetGameTimeDays(int value);
void AdvanceEnvironmentTime(int elapsed); /* 0x00482A20 */
void SetWorldEnvironmentValue(W8World* world, float value);
/* Arm the lighting transition. Duration is in milliseconds: negative fades the
   world out, positive fades it back in over abs(duration); zero applies the
   current base intensity immediately and returns to day/night mode. */
void BeginWorldLightingFade(float duration);
