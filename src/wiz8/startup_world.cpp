#include "wiz8/screen_state.h"
#include "surrender/srNode.h"
#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Quality.h"
#include "wiz8/startup_world.h"
#include "wiz8/render_state.h"

#include <new>
#include <stdlib.h>
#include <string.h>

// GLOBAL: WIZ8 0x00659c0c
W8Navigator* g_startup_world_659c0c;
// GLOBAL: WIZ8 0x006081e8
float g_runtime_world_scale_6081e8 = 500.0f;
extern const float g_world_scale_005ebc40 = 500.0f;
float g_startup_depth_603ac8 = 1000.0f;
extern const float g_startup_near_limit_005ec000 = 250.0f;

/* Builds the startup navigation state after the renderer graph is open.
   0x0044F060 sits in the attribution gap before Engine Code\3dapi.cpp. */
// FUNCTION: WIZ8 0x0044f060
unsigned char InitializeStartupNavigation0044F060(void)
{
    W8Navigator* navigator;

    NoOp();
    InitializeAniMeshCache(-1, -1);
    InitializeRenderQuality();
    InitializeEnvironmentColours();

    navigator = new W8Navigator;
    g_startup_world_659c0c = navigator;
    navigator->configureStartupRange(500.0f);
    if (g_runtime_world_scale_6081e8 < g_world_scale_005ebc40) {
        g_runtime_world_scale_6081e8 = 500.0f;
    }
    navigator->configureStartupDepth(g_startup_depth_603ac8 < g_startup_near_limit_005ec000
                                         ? g_startup_near_limit_005ec000
                                         : g_startup_depth_603ac8,
                                     g_startup_depth_603ac8);
    return 1;
}
