#pragma once

/* Reads g_environment_lighting_mode_0060a3a8: 2 while the day/night cycle
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
class W8MaterialMapper00482010 : public srVertexProcessor {
public:
    W8MaterialMapper00482010();
    /* Retail ICF folds this onto W8NormalTexcoordMapper004B89A0's deleting
       destructor at 0x004B8A50. */

    virtual ~W8MaterialMapper00482010() override {}
    virtual int isActive(srVertexPipe& pipe) override;
    virtual void process(srVertexPipe& pipe) override;

    float scroll_rate_u_04;      /* 0x04 */
    float scroll_rate_v_08;      /* 0x08 */
    unsigned char padding_0c[8]; /* 0x0c */
    float offset_14;             /* 0x14 */
    float offset_18;             /* 0x18 */
};

static_assert(sizeof(W8MaterialMapper00482010) == 0x1c, "W8MaterialMapper00482010_must_be_0x1c");

/* ABS 0x0065AD78: the three light-direction words as one colour triple.
   Produced from the day-phase colour table and consumed as fog-vector
   floats; the word copies below move it without reinterpreting it. */
extern EnvironmentColour g_light_direction_0065ad78;

extern EnvironmentColour g_environment_colours_65a178[256];
extern EnvironmentColour g_environment_colours_65ad98[256];

class stLight;
void AddEnvironmentLight00483F30(stLight* light);

BOOLEAN ReadLightColourTable00482F90(int hFile);
BOOLEAN ReadEnvironmentColourTable004830D0(int hFile);
void BuildEnvironmentColourRamp00483210(void);
void BuildLightColourRamp00483360(void);
void UpdateEnvironmentLight004834B0(void);
void RefreshEnvironment00483560(void);
void SetWorldEnvironmentColour00483A60(W8World* world, EnvironmentColour colour);

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
extern bool g_sky_enabled_0065b9ae;
void ResetEnvironment(void);
void InitializeLevelEnvironment00482410(void);
/* Drop the sky gradients and celestial props, release both environment
   objects and empty the registered-light list; called by world teardown. */
void ClearEnvironmentObjects004823B0(void);
/* Apply one day-phase colour and intensity to the world, its environment
   lights, and the animated cloud material. */
void ApplyEnvironmentColour00483BA0(W8World* world, float intensity,
                                    const EnvironmentColour* colour);
void ReleaseEnvironmentObjects(void);

/* Turn the environment clock on or off; enabling resets its tick baseline. */
void SetEnvironmentTimeEnabled00482990(bool enabled);
float GetViewDistance(void);
void SetGameTimeMilliseconds(int value);
/* The per-frame environment update, and the alternate lighting transition it
   hands off to when the bypass value is set. */
void UpdateEnvironment482770(void);
void UpdateEnvironmentLighting00484300(void);
float GetWorldValue24(const W8World* world);
/* Show or hide the world's camera light: 0x00483E50 loads g_world->camera_light
   (offset 0x54) and clears srNode::FLAG_DISABLE when visible, sets it when not.
   Nothing here is a sky node; the exported name is still the placeholder and one
   caller lives in a TU owned elsewhere, so the rename waits for it. */
void SetSkyNodeVisible(bool visible);
void SetCameraLightIntensity00483E30(float value);
/* Mode 0 raises the camera light's intensity, 1 lowers it, 2 disables the
   node, 3 re-enables it. */
void SetCameraLightMode00483E80(int mode);
void RefreshFogRanges004836A0(void);

void SetViewDistance(float distance);
unsigned char InitializeEnvironmentColours(void);
extern unsigned int g_frame_tick_65a154;
extern float g_frame_elapsed_65a158;

/* The environment's lighting mode: 2 while the per-frame day/night cycle runs,
   1 while a colour transition is in progress and 0 once one has faded the world
   out. Retail writes it only from the transition bodies at 0x00483FD0 and
   0x00484300 and reads it from UpdateEnvironment482770; the exported accessor
   above still carries the address name. */
extern int g_environment_lighting_mode_0060a3a8;
extern bool g_fog_enabled_0065b9ad;
/* 1/duration while the lighting transition body at 0x00484300 runs, zero when
   idle: UpdateEnvironment482770 hands off to that body while it is not zero. */
extern float g_environment_transition_rate_0065b9b8;
extern unsigned long g_environment_transition_tick_0065b9bc;
/* Last day phase the light direction was published from. */
extern int g_last_light_phase_0060a3ac;
/* Last day phase the world's environment colour was refreshed from. */
extern int g_last_environment_colour_phase_0060a3b0;
/* Gates the per-frame world-colour refresh. Retail writes it nowhere and
   initialises it to 1, so that refresh always runs; the only reference is the
   read inside UpdateEnvironment482770. */
extern unsigned char g_environment_colour_refresh_0060a395;
/* The game clock's multiplier, not a distance: every environment clock advance
   scales the elapsed milliseconds by it (12.0 normally, 2880.0 while the party
   rests) and the environment update is skipped while it holds another value.
   Sight.cpp and GameplayTime.cpp consume it through SetViewDistance and
   GetViewDistance, so the rename waits for those owners. */
extern float g_view_distance_0060a390;
extern unsigned char g_environment_flag_0060a394;
/* Half the Sun-to-Moon distance, which is the radius of the circle the active
   celestial prop travels; -1 until InitializeLevelEnvironment measures it. */
extern float g_celestial_orbit_radius_0060a3a4;
/* The level's "Moon" prop. */
extern W8Prop* g_moon_prop_0065ad84;
/* The level's "Sun" prop. */
extern W8Prop* g_sun_prop_0065a160;
/* The three day-phase sky gradients the level setup registers:
   SkyGrad0000.ifl, Skytop0000.ifl and Horizon0000.ifl. The registration loop
   and the per-frame animation both walk one contiguous base, so they are one
   array rather than three globals. */
extern stTextureAnim* g_sky_gradient_animations_0065a168[3];
/* Midpoint of the Sun and Moon, which the active prop orbits and the inactive
   one rests at. */
extern srVector3T<float> g_celestial_origin_65ad88;

/* The registered environment lights, ambient-filled by ApplyEnvironmentColour.
   Its count is the 0x0065B99C word the module reset previously carried as an
   address-named global. */
extern W8GrowableVector<stLight*> g_environment_lights_0065b998;

void SetGameTimeDays(int value);
void AdvanceEnvironmentTime00482A20(int elapsed); /* 0x00482A20 */
void SetWorldEnvironmentValue00483AE0(W8World* world, float value);
/* Arm the lighting transition. Duration is in milliseconds: negative fades the
   world out, positive fades it back in over abs(duration); zero applies the
   current base intensity immediately and returns to day/night mode. */
void BeginWorldLightingFade(float duration);
