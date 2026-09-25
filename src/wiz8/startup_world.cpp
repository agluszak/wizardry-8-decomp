#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_screens/Screens.h"
#include "surrender/srNode.h"
#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Quality.h"
#include "wiz8/startup_world.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Levels.h"

#include <new>
#include <stdlib.h>
#include <string.h>

// GLOBAL: WIZ8 0x00659c0c
W8Navigator* g_startup_world;
// GLOBAL: WIZ8 0x006081e8
float g_runtime_world_scale = 500.0f;
// GLOBAL: WIZ8 0x005EBC40
extern const float g_world_scale = 500.0f;
// GLOBAL: WIZ8 0x005EC000
extern const float g_startup_near_limit = 250.0f;

/* Builds the startup navigation state after the renderer graph is open.
   0x0044F060 sits in the attribution gap between Prop.cpp (ends 0x0044E1F0)
   and Engine Code\3dapi.cpp (0x0044F1C0); the two globals sit in the
   unbracketed .data tail. Unresolved fragment - no proven ownership. */
// FUNCTION: WIZ8 0x0044f060
unsigned char InitializeStartupNavigation(void)
{
    W8Navigator* navigator;

    NoOp();
    InitializeAniMeshCache(-1, -1);
    InitializeRenderQuality();
    InitializeEnvironmentColours();

    navigator = new W8Navigator;
    g_startup_world = navigator;
    navigator->configureStartupRange(500.0f);
    if (g_runtime_world_scale < g_world_scale) {
        g_runtime_world_scale = 500.0f;
    }
    navigator->configureStartupDepth(g_default_world_height < g_startup_near_limit
                                         ? g_startup_near_limit
                                         : g_default_world_height,
                                     g_default_world_height);
    return 1;
}
/* Tears down the startup navigation state InitializeStartupNavigation
   built: the ani-mesh cache, render quality, environment globals, and the
   startup navigator itself. */
// FUNCTION: WIZ8 0x0044F190
void ShutdownStartupNavigation(void)
{
    NoOp();
    FreeAniMeshCache();
    DestroyRenderQuality();
    ClearEnvironmentObjects();
    if (g_startup_world != 0) {
        delete g_startup_world;
    }
    g_startup_world = 0;
}
