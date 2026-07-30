#include <cstring>

#include "wiz8/combat_state.h"
#include "wiz8/engine_code/ClipPlane.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/game_state.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/monster_runtime.h"
#include "surrender/srCore.h"
#include "surrender/srNode.h"

extern void Function4EA310(int mode);
extern void Function426790(void);
extern void Function50DA00(void);
extern unsigned char ReleaseItemLists(void);
extern unsigned char ShutdownMonsterManager(void);
extern unsigned char Function5B1740(void);
extern void Function48DB30(void);
extern void ClearValue689FAC(void);
extern void Function4909C0(void);
extern void DisableSky(void);
extern void Function489920(void);
extern void ReleaseEnvironmentObjects(void);
extern void ClearValue6834D4(void);
extern unsigned char SaveLevelStatus(const char* path);

extern unsigned char g_world_cleanup_flag_00659757;
extern float g_runtime_world_scale_6081e8;

namespace {

srRegistry::ClassNode* GetClipPlaneClassNode()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1500);

    if (node == 0) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (parent == 0) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("srClipPlane", parent, 0x1500, 0);
    }
    return node;
}

} // namespace

// FUNCTION: WIZ8 0x0042ACE0
unsigned char UnloadLevel(const char* save_directory)
{
    if (g_in_combat_00683f94 != 0) {
        Function4EA310(1);
    }

    if (g_current_level < 47) {
        g_level_progress[g_current_level].sight_clock = g_world_clock_00686a48;
    }

    if (g_world_cleanup_flag_00659757 != 0) {
        Function426790();
    }

    Function50DA00();
    if (strcmp(save_directory, "") != 0) {
        SaveLevelStatus("Saves\\CurrentGame.SAV");
    }

    if (g_world_cleanup_flag_00659757 != 0) {
        Function426790();
    }

    if (g_active_monster_list_00683fad != 0) {
        if (ReleaseItemLists() == 0) {
            return 0;
        }
        if (g_world_cleanup_flag_00659757 != 0) {
            Function426790();
        }
        if (ShutdownMonsterManager() == 0) {
            return 0;
        }
        if (g_world_cleanup_flag_00659757 != 0) {
            Function426790();
        }
        if (Function5B1740() == 0) {
            return 0;
        }
        if (g_world_cleanup_flag_00659757 != 0) {
            Function426790();
        }
    }

    Function48DB30();
    ClearValue689FAC();
    if (g_world_cleanup_flag_00659757 != 0) {
        Function426790();
    }

    Function4909C0();
    DisableSky();
    W8World* world = GetWorld();
    if (world != 0) {
        Forward44FAF0(world);
        SetCurrentWorld(0);
    }
    Function489920();

    if (g_world_cleanup_flag_00659757 != 0) {
        Function426790();
    }

    DisableSky();
    g_current_level = -1;
    ReleaseEnvironmentObjects();
    g_runtime_world_scale_6081e8 = 500.0f;
    ClearValue6834D4();

    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = GetClipPlaneClassNode();
    srClass* clip_plane = static_cast<srClass*>(registry->find(node, 0, 0));

    while (clip_plane != 0) {
        srClass* next = static_cast<srClass*>(
            registry->find(GetClipPlaneClassNode(), 0, clip_plane));
        clip_plane->release();
        clip_plane = next;
    }
    return 1;
}
