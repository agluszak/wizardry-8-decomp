#include "wiz8/engine_code/stTextureAnim.h"

#include "surrender/srCore.h"
#include "wiz8/wiz8_windows.h"

// SYNTHETIC: WIZ8 0x00485a10
// stTextureAnim::`scalar deleting destructor'

/* Restart playback from one frame and from the current wall-clock tick. */
// FUNCTION: WIZ8 0x00485400
void stTextureAnim::SetFrame00485400(int frame)
{
    frame_58 = frame;
    frame_tick_6c = GetTickCount();
}

// FUNCTION: WIZ8 0x004857f0
unsigned long stTextureAnim::getClassID() const
{
    return CLASS_ID;
}

// FUNCTION: WIZ8 0x00485800
const char* stTextureAnim::getClassName() const
{
    return "stTextureAnim";
}

// FUNCTION: WIZ8 0x00485810
srRegistry::ClassNode* stTextureAnim::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(CLASS_ID);

    if (node == 0) {
        srRegistry* texture_registry = srCore.getRegistry();
        srRegistry::ClassNode* texture = texture_registry->getClassNode(0x2110);

        if (texture == 0) {
            srRegistry* iface_registry = srCore.getRegistry();
            srRegistry::ClassNode* iface = iface_registry->getClassNode(0x2100);

            if (iface == 0) {
                iface = iface_registry->registerClass(
                    "srTextureIFace", srClass::sGetClassNode(), 0x2100, 1);
            }
            texture = texture_registry->registerClass(
                srTexture::sGetClassName(), iface, 0x2110, 0);
        }
        node = registry->registerClass(
            "stTextureAnim", texture, CLASS_ID, 0);
    }
    return node;
}

/* Clone through the concrete implementation's vInstance slot, then carry the
   Wizardry playback state that srTexture's assignment cannot know about. */
// FUNCTION: WIZ8 0x004858b0
srTextureIFace* stTextureAnim::clone()
{
    stTextureAnim* instance = static_cast<stTextureAnim*>(vInstance());

    *static_cast<srTexture*>(instance) = *this;
    instance->object_54 = object_54;
    instance->frame_58 = frame_58;
    instance->value_5c = value_5c;
    instance->flag_60 = flag_60;
    instance->value_64 = value_64;
    instance->frame_rate_68 = frame_rate_68;
    instance->frame_tick_6c = frame_tick_6c;
    instance->value_70 = value_70;
    instance->value_74 = value_74;
    instance->flag_78 = flag_78;
    return instance;
}

// FUNCTION: WIZ8 0x00485910
stTextureAnim::~stTextureAnim()
{
    srCore.getRegistry()->unregisterInstance(getClassNode(), this);
}
