#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "surrender/srCore.h"
#include "surrender/srHeap.h"
#include "surrender/srNode.h"

#include <string.h>

// VTABLE: WIZ8 0x005ECBD0
// class stParticle

// VTABLE: WIZ8 0x005ECC04
// class srClassSupport<stParticle,srNode,0,65545>

// TEMPLATE: WIZ8 0x0049B540
// srClassSupport<stParticle,srNode,0,65545>::getClassID

// TEMPLATE: WIZ8 0x0049B550
// srClassSupport<stParticle,srNode,0,65545>::getClassName

// TEMPLATE: WIZ8 0x0049B560
// srClassSupport<stParticle,srNode,0,65545>::getClassNode

// TEMPLATE: WIZ8 0x0049B5D0
// srClassSupport<stParticle,srNode,0,65545>::clone

// TEMPLATE: WIZ8 0x0049B990
// srClassSupport<stParticle,srNode,0,65545>::~srClassSupport<stParticle,srNode,0,65545>

// SYNTHETIC: WIZ8 0x0049BA50
// srClassSupport<stParticle,srNode,0,65545>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00498150
// stParticle::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004980E0
srClass* stParticle::vInstance()
{
    return new stParticle(0, 0);
}

// FUNCTION: WIZ8 0x00498A20
stParticle::~stParticle()
{
    if (allocation_148 != 0) {
        srHeap.free(allocation_148);
    }
    if (allocation_170 != 0) {
        srHeap.free(allocation_170);
    }
    if (allocation_164 != 0) {
        srHeap.free(allocation_164);
    }
    if (allocation_160 != 0) {
        srHeap.free(allocation_160);
    }
    if (allocation_168 != 0) {
        srHeap.free(allocation_168);
    }
    if (allocation_16c != 0) {
        srHeap.free(allocation_16c);
    }
    if (allocation_174 != 0) {
        ::operator delete(allocation_174);
    }
    if (retained_14c != 0) {
        retained_14c->release();
    }
    if (allocation_198 != 0) {
        srHeap.free(allocation_198);
    }
    if (allocation_19c != 0) {
        ::operator delete(allocation_19c);
    }
    if (allocation_254 != 0) {
        ::operator delete(allocation_254);
    }
    if (allocation_194 != 0) {
        ::operator delete(allocation_194);
    }
    if (texture_frames_178 != 0) {
        for (unsigned int i = 0; i < texture_frame_count_15c; i += 2) {
            texture_frames_178[i]->release();
        }
        ::operator delete(texture_frames_178);
    }
    if (allocation_17c != 0) {
        ::operator delete(allocation_17c);
    }
    texture_154->release();
    setParent(0, 1);
}

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
