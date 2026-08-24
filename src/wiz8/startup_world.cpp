#include "surrender/srNode.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Navigator.h"

#include <new>
#include <stdlib.h>
#include <string.h>

extern "C" {

extern void NoOp(void);
extern void InitializeRenderQuality(void);
extern unsigned char InitializeEnvironmentColours(void);

W8Navigator* g_startup_world_659c0c;
float g_runtime_world_scale_6081e8 = 500.0f;
extern const float g_world_scale_005ebc40 = 500.0f;
float g_startup_depth_603ac8 = 1000.0f;
extern const float g_startup_near_limit_005ec000 = 250.0f;

int g_storage_state_65be80;
int g_storage_state_65be84;
int g_storage_limit_65be88;
int g_storage_limit_65be8c;
W8PList g_storage_list_65be90;

/* The two caller-provided values override the original 16 MiB and 1 MiB
   defaults only when positive.  Startup deliberately passes -1 for both. */
// FUNCTION: WIZ8 0x004b5780
void Function4B5780(int primary_limit, int secondary_limit)
{
    g_storage_state_65be80 = 0;
    g_storage_state_65be84 = 0;
    g_storage_limit_65be88 = 0x1000000;
    if (primary_limit > 0) {
        g_storage_limit_65be88 = primary_limit;
    }
    g_storage_limit_65be8c = 0x100000;
    if (secondary_limit > 0) {
        g_storage_limit_65be8c = secondary_limit;
    }
    PListInit(&g_storage_list_65be90);
}

/* Builds the startup navigation state after the renderer graph is open. */
// FUNCTION: WIZ8 0x0044f060
unsigned char Function44F060(void)
{
    W8Navigator* navigator;

    NoOp();
    Function4B5780(-1, -1);
    InitializeRenderQuality();
    InitializeEnvironmentColours();

    navigator = new W8Navigator;
    g_startup_world_659c0c = navigator;
    navigator->configureStartupRange(500.0f);
    if (g_runtime_world_scale_6081e8 < g_world_scale_005ebc40) {
        g_runtime_world_scale_6081e8 = 500.0f;
    }
    navigator->configureStartupDepth(
        g_startup_depth_603ac8 < g_startup_near_limit_005ec000
            ? g_startup_near_limit_005ec000
            : g_startup_depth_603ac8,
        g_startup_depth_603ac8);
    return 1;
}

}
