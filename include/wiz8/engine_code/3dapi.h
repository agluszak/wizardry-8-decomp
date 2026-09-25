#pragma once

void SetRendererReady(void);

#include "surrender/srMath.h"

class srCamera;

struct W8World;
class srScene;

extern bool g_renderer_ready_00607d7c;
extern bool g_world_cleanup_flag_00659757;
extern bool g_shift_held_006f0530;
extern bool g_monster_combat_timer_enabled_006f0531;
extern bool g_modifier_held_006f0534;
extern bool g_navigator_vertical_enabled_006081f8;
extern bool g_world_mesh_update_enabled_00607d7d;
extern float g_float_00609c88;
extern bool g_flag_00609c8c;

/* 0x00450780: the three-argument assert the main-game code paths use; it
   forwards to the four-argument SurRender export. */
void ReportAssertion(const char* expression, const char* source_path, long line);
void UpdateWorldCameraAndPaths0044FC20(W8World* world, unsigned int flags);
void ApplyWorldUpdateFlags(W8World* world, unsigned int flags);
/* Retail call sites push world/x/y; the body ignores them and reads the
   renderer's selected prop index. */
int ForwardSelectedPropIndex004503B0(W8World* world, int x, int y);
/* Retail call sites push world/mode/x/y; the body ignores them and runs the
   selected prop trigger when one is latched. */
bool ForwardActivateSelectedProp00451150(W8World* world, int mode, int x, int y);
void SetCameraSwayMode(srCamera* camera, int mode);
void WorldSetCameraLocation(W8World* world, const float* location); /* 0x00450420 */
