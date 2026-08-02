#pragma once

struct W8World;

struct EnvironmentColour {
    float red;
    float green;
    float blue;
};

static_assert(sizeof(EnvironmentColour) == 0x0c,
              "EnvironmentColour_must_be_0x0c");

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
void SetWorldEnvironment00483BA0(
    W8World* world, float intensity, const EnvironmentColour* colour);
void SetWorldEnvironmentColour00483A60(
    W8World* world, EnvironmentColour colour);

void SetSkyEnabled(unsigned char enabled);
void SetFogEnabled(unsigned char enabled);
unsigned char IsFogEnabled(void);
void DisableSky(void);
unsigned char IsSkyEnabled(void);
void ResetEnvironment(void);
void ReleaseEnvironmentObjects(void);
