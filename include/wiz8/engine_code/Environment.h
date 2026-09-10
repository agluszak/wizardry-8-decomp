#pragma once

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

static_assert(sizeof(EnvironmentColour) == 0x0c,
              "EnvironmentColour_must_be_0x0c");

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

    float value_04;                      /* 0x04 */
    float value_08;                      /* 0x08 */
    unsigned char unknown_0c[8];         /* 0x0c */
    float offset_14;                     /* 0x14 */
    float offset_18;                     /* 0x18 */
};

static_assert(sizeof(W8MaterialMapper00482010) == 0x1c,
              "W8MaterialMapper00482010_must_be_0x1c");

extern W8MaterialMapper00482010 g_material_mapper_00659738;

extern "C" {
extern EnvironmentColour g_environment_colours_65a178[256];
extern EnvironmentColour g_environment_colours_65ad98[256];

class stLight;
void AddEnvironmentLight00483F30(stLight* light);
}

unsigned char ReadLightColourTable00482F90(int hFile);
unsigned char ReadEnvironmentColourTable004830D0(int hFile);
void BuildEnvironmentColourRamp00483210(void);
void BuildLightColourRamp00483360(void);
void UpdateEnvironmentLight004834B0(void);
void RefreshEnvironment00483560(void);
void SetWorldEnvironment00483BA0(
    W8World* world, float intensity, const EnvironmentColour* colour);
void SetWorldEnvironmentColour00483A60(
    W8World* world, EnvironmentColour colour);

void SetSkyEnabled(unsigned char enabled);
void EnableSky(void);
void SetFogEnabled(unsigned char enabled);
unsigned char IsFogEnabled(void);
void DisableSky(void);
unsigned char IsSkyEnabled(void);
void GetWorldLightValue(const void* world, int* light_value);
void SetLightDirection(const int* direction);
void GetLightDirection(int* direction);
extern unsigned char g_sky_enabled_0065b9ae;
void ResetEnvironment(void);
void InitializeLevelEnvironment00482410(void);
/* 0x00483BA0: apply one day-phase colour and intensity to the world. */
void ApplyEnvironmentColour00483BA0(W8World* world, float intensity, const EnvironmentColour* colour);
void ReleaseEnvironmentObjects(void);

void Function482990(unsigned char enabled);
float GetViewDistance(void);
void Function482720(int value);
/* The per-frame environment update, and the alternate lighting transition it
   hands off to when the bypass value is set. */
void UpdateEnvironment482770(void);
void UpdateEnvironmentLighting00484300(void);
float GetWorldValue24(const void* world);
void SetSkyNodeVisible(char visible);
void SetSkyNodeValue1D0(int value);
void Function4836A0(void);

void SetViewDistance(float distance);

extern int g_environment_value_0060a3a8;
extern unsigned char g_fog_enabled_0065b9ad;/* Zero unless the environment update is bypassed, in which case 0x00484300
   runs instead. */
extern float g_environment_value_0065b9b8;
/* Last day phase the light direction and the world colour came from. */
extern int g_environment_value_0060a3ac;
extern int g_environment_value_0060a3b0;
extern unsigned char g_flag_0060a395;
