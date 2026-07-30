#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/game_timer.h"
#include "surrender/srCore.h"
#include "surrender/srNode.h"

#include <string.h>

/*
 * Engine Code\stParticle.cpp.
 *
 * Only the class registry slots are recovered so far.
 */

// FUNCTION: WIZ8 0x0049acd0
void stParticle::SetActive(unsigned char active)
{
    if (active != 0 && active_1a0 == 0) {
        unsigned int now = g_shared_timer_base->getMsTime(
            srTimer::TIMER_READ_DEFAULT);
        activated_at_258 = now;
        updated_at_25c = now;
    }
    active_1a0 = active;
}

// FUNCTION: WIZ8 0x0049ac30
unsigned char stParticle::ReplaceTexture0049AC30(
    const char* old_name, srTextureIFace* replacement)
{
    if (texture_154 != 0 &&
        (texture_154->getClassID() == 0x10001 ||
         texture_154->getClassID() == 0x10000) &&
        _stricmp(texture_154->getName(), old_name) == 0) {
        SetTexture0049AB00(replacement);
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0049b550
const char* stParticle::getClassName() const
{
    return "stParticle";
}

// FUNCTION: WIZ8 0x0049b540
unsigned long stParticle::getClassID() const
{
    return 0x10009;
}

/* Registry node builder in the proven two-level shape: the class hangs
   directly off srNode, whose name comes from its static getter while this
   class's own name is a literal. */
// FUNCTION: WIZ8 0x0049b560
srRegistry::ClassNode* stParticle::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10009);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("stParticle", parent, 0x10009, 0);
    }
    return node;
}
