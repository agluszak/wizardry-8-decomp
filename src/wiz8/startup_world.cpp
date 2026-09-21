#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_screens/Screens.h"
#include "surrender/srNode.h"
#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Quality.h"
#include "wiz8/startup_world.h"
#include "wiz8/engine_code/Environment.h"

#include <new>
#include <stdlib.h>
#include <string.h>

// GLOBAL: WIZ8 0x00659c0c
W8Navigator* g_startup_world_659c0c;
// GLOBAL: WIZ8 0x006081e8
float g_runtime_world_scale_6081e8 = 500.0f;
// GLOBAL: WIZ8 0x005EBC40
extern const float g_world_scale_005ebc40 = 500.0f;
float g_startup_depth_603ac8 = 1000.0f;
extern const float g_startup_near_limit_005ec000 = 250.0f;

/* Builds the startup navigation state after the renderer graph is open.
   0x0044F060 sits in the attribution gap between Prop.cpp (ends 0x0044E1F0)
   and Engine Code\3dapi.cpp (0x0044F1C0); the two globals sit in the
   unbracketed .data tail. Unresolved fragment - no proven ownership. */
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

/* Shutdown mirror of InitializeStartupNavigation0044F060; runs from
   ShutdownVideoManager. */
// FUNCTION: WIZ8 0x0044f190
void ShutdownStartupNavigation0044F190(void)
{
    NoOp();
    FreeAniMeshCache004B57D0();
    DestroyRenderQuality0047B570();
    ClearEnvironmentObjects004823B0();
    delete g_startup_world_659c0c;
    g_startup_world_659c0c = 0;
}
