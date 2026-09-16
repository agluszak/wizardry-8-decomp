#pragma once

void SetRendererReady(void);

#include "surrender/srMath.h"

class srCamera;

struct W8World;
class srScene;

extern unsigned char g_renderer_ready_00607d7c;
extern unsigned char g_world_cleanup_flag_00659757;
extern unsigned char g_navigator_vertical_enabled_006081f8;
extern unsigned char g_world_mesh_update_enabled_00607d7d;

void Function44FC20(W8World* world, unsigned int flags);
void Function450210(W8World* world, unsigned int flags);
void SetCameraSwayMode(srCamera* camera, int mode);
