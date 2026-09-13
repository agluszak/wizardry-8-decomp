#pragma once

#include "Types.h"
#include "surrender/srVertexProcessor.h"

struct W8World;

struct EnvironmentColour {
    EnvironmentColour() {}
    EnvironmentColour(double red_value, double green_value, double blue_value);
    EnvironmentColour& operator=(double value)
    {
        Set(value, value, value);
        return *this;
    }
    void Set(double red_value, double green_value, double blue_value);
    float red;
    float green;
    float blue;
};

static_assert(sizeof(EnvironmentColour) == 0x0c, "EnvironmentColour_must_be_0x0c");

/* Environment.cpp. The vertex processor that scrolls the first texture
   coordinate set of the sky's AnimatedCloudMaterial by a fixed per-frame
   rate. No authored name survives, so the constructor address names the
   class; the global instance 0x00659738 is defined by the level loader. */
// VTABLE: WIZ8 0x005EC8F8
class W8MaterialMapper00482010 : public srVertexProcessor {
public:
    W8MaterialMapper00482010();
    virtual ~W8MaterialMapper00482010() override {}
    virtual int isActive(srVertexPipe& pipe) override;
    virtual void process(srVertexPipe& pipe) override;

    float value_04;              /* 0x04 */
    float value_08;              /* 0x08 */
    unsigned char unknown_0c[8]; /* 0x0c */
    float offset_14;             /* 0x14 */
    float offset_18;             /* 0x18 */
};

static_assert(sizeof(W8MaterialMapper00482010) == 0x1c, "W8MaterialMapper00482010_must_be_0x1c");

extern W8MaterialMapper00482010 g_material_mapper_00659738;

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
void SetSkyNodeVisible(bool visible);
void SetCameraLightIntensity00483E30(float value);
void RefreshFogRanges004836A0(void);

void SetViewDistance(float distance);

extern int g_environment_value_0060a3a8;
extern bool
    g_fog_enabled_0065b9ad; /* Zero unless the environment update is bypassed, in which case 0x00484300
   runs instead. */
extern float g_environment_value_0065b9b8;
/* Last day phase the light direction and the world colour came from. */
extern int g_environment_value_0060a3ac;
extern int g_environment_value_0060a3b0;
extern unsigned char g_flag_0060a395;

void SetGameTimeDays(int value);
