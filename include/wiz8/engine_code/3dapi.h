#pragma once

void SetRendererReady(void);

#include "surrender/srMath.h"

class srCamera;

struct W8World;
class srScene;

extern bool g_renderer_ready;
extern bool g_world_cleanup_flag;
extern bool g_shift_held;
extern bool g_monster_combat_timer_enabled;
extern bool g_modifier_held;
extern bool g_navigator_vertical_enabled;
extern bool g_world_mesh_update_enabled;
extern float g_float_00609c88;
extern bool g_flag_00609c8c;

/* 0x00450780: the three-argument assert the main-game code paths use; it
   forwards to the four-argument SurRender export. */
void ReportAssertion(const char* expression, const char* source_path, long line);
void UpdateWorldCameraAndPaths0044FC20(W8World* world, unsigned int flags);
void ApplyWorldUpdateFlags(W8World* world, unsigned int flags);
/* Retail call sites push world/x/y; the body ignores them and reads the
   renderer's selected prop index. */
int ForwardSelectedPropIndex(W8World* world, int x, int y);
/* Retail call sites push world/mode/x/y; the body ignores them and runs the
   selected prop trigger when one is latched. */
bool ForwardActivateSelectedProp(W8World* world, int mode, int x, int y);
void SetCameraSwayMode(srCamera* camera, int mode);
void WorldSetCameraLocation(W8World* world, const float* location); /* 0x00450420 */
