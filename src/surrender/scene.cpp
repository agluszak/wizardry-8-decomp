#include "surrender/srScene.h"

#include <string.h>

#pragma intrinsic(memset)

// FUNCTION: SURRENDER 0x10056550
void srScene::resetStatistics()
{
    memset(&statistics_140, 0, sizeof(statistics_140));
}

// FUNCTION: SURRENDER 0x100565D0
srScene::srScene(srNode* parent)
    : srClassSupport<srScene, srNode, 0, 0x1010>(static_cast<srNode*>(0))
{
    setAmbientLight(0.2f, 0.2f, 0.2f);
    setFogColor(0.1f, 0.2f, 0.4f);
    traversal_158.entry_count = 0;
    traversal_158.node_count = 0;
    process_info_170.renderer = 0;
    enabled_138.value = 0;
    if (parent != 0) {
        setParent(parent, 0);
    }
    resetStatistics();
}
