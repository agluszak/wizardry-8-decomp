#pragma once

#include "wiz8/camera_motion.h"

void SetRendererReady(void);

#include "surrender/srMath.h"

class srCamera;

struct W8World;
class srScene;

extern bool g_renderer_ready;
extern bool g_world_cleanup_flag;
extern bool g_navigator_vertical_enabled;
extern bool g_world_mesh_update_enabled;
extern float g_mesh_view_half_angle_degrees;
extern bool g_world_update_requested;

/* 0x00450780: the three-argument assert the main-game code paths use; it
   forwards to the four-argument SurRender export. */
void ReportAssertion(const char* expression, const char* source_path, w8_long line);
void UpdateWorldCameraAndPaths(W8World* world, unsigned int flags);
/* Debug world controls combine these bits in unsigned int storage. */
enum {
    W8_WORLD_INCREASE_FAR_CLIP = 0x01u,
    W8_WORLD_DECREASE_FAR_CLIP = 0x02u,
    W8_WORLD_WIDEN_MESH_VIEW = 0x04u,
    W8_WORLD_NARROW_MESH_VIEW = 0x08u,
    W8_WORLD_TOGGLE_LOADED = 0x40u,
    W8_WORLD_INCREASE_ENVIRONMENT = 0x100u,
    W8_WORLD_DECREASE_ENVIRONMENT = 0x200u,
    W8_WORLD_CHANGE_FAR_CLIP = W8_WORLD_INCREASE_FAR_CLIP | W8_WORLD_DECREASE_FAR_CLIP
};
void ApplyWorldUpdateFlags(W8World* world, unsigned int flags);
/* Retail call sites push world/x/y; the body ignores them and reads the
   renderer's selected prop index. */
int ForwardSelectedPropIndex(W8World* world, int x, int y);
/* Retail call sites push world/mode/x/y; the body ignores them and runs the
   selected prop trigger when one is latched. */
bool ForwardActivateSelectedProp(W8World* world, int mode, int x, int y);
void SetCameraSwayMode(srCamera* camera, int mode);
void WorldSetCameraLocation(W8World* world, const srVector3T<float>* location); /* 0x00450420 */
