#include "wiz8/engine_code/ReadMesh.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/engine_code/GrCycle.h"

#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/stTextureFile.h"
#include "wiz8/wiz8_windows.h"

#include <stdlib.h>

// VTABLE: WIZ8 0x005EC9C0
// class stTextureAnim

// VTABLE: WIZ8 0x005ECA04
// class srClassSupport<stTextureAnim,srTexture,0,65536>

// TEMPLATE: WIZ8 0x004857F0
// srClassSupport<stTextureAnim,srTexture,0,65536>::getClassID

// TEMPLATE: WIZ8 0x00485800
// srClassSupport<stTextureAnim,srTexture,0,65536>::getClassName

// TEMPLATE: WIZ8 0x00485810
// srClassSupport<stTextureAnim,srTexture,0,65536>::getClassNode

// TEMPLATE: WIZ8 0x004858B0
// srClassSupport<stTextureAnim,srTexture,0,65536>::clone

// TEMPLATE: WIZ8 0x00485910
// srClassSupport<stTextureAnim,srTexture,0,65536>::~srClassSupport<stTextureAnim,srTexture,0,65536>

// SYNTHETIC: WIZ8 0x00485A10
// srClassSupport<stTextureAnim,srTexture,0,65536>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00485040
// stTextureAnim::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00485A40
// srClassSupport<srTextureIFace,srClass,1,8448>::sGetClassNode

// SYNTHETIC: WIZ8 0x00485A80
// W8GrowableVector<srTextureIFace*>::`vector deleting destructor'

// SYNTHETIC: WIZ8 0x00485AB0
// W8GrowableVector<srTextureIFace*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00485AD0
// W8GrowableVector<srTextureIFace*>::~W8GrowableVector

// FUNCTION: WIZ8 0x00484BE0
stTextureAnim::stTextureAnim()
{
    textures_54 = 0;
    frame_58 = 0;
    direction_5c = 1;
    animation_mode_60 = 0;
    initial_frame_64 = 0;
    frame_rate_68 = 15.0f;
    frame_tick_6c = GetTickCount();
    trigger_mode_70 = 0;
    probability_74 = -1.0f;
    running_78 = 0;
    textures_54 = new W8GrowableVector<srTextureIFace*>;
}

// FUNCTION: WIZ8 0x00484E60
srClass* stTextureAnim::vInstance()
{
    return new stTextureAnim;
}

// FUNCTION: WIZ8 0x00485070
stTextureAnim::stTextureAnim(const stTextureAnim& other)
{
    int i;

    textures_54 = 0;
    frame_58 = 0;
    direction_5c = 1;
    animation_mode_60 = other.animation_mode_60;
    initial_frame_64 = other.initial_frame_64;
    frame_rate_68 = other.frame_rate_68;
    frame_tick_6c = GetTickCount();
    trigger_mode_70 = other.trigger_mode_70;
    probability_74 = other.probability_74;
    running_78 = other.running_78;
    textures_54 = new W8GrowableVector<srTextureIFace*>;

    for (i = 0; i < other.textures_54->GetCount(); ++i) {
        srTextureIFace* texture = *other.textures_54->GetAt(i);
        textures_54->Add(texture);
        texture->addReference();
    }
}

// FUNCTION: WIZ8 0x00485290
stTextureAnim::~stTextureAnim()
{
    if (IsTextureInReadMeshScratch(this) != 0) {
        ReleaseReadMeshScratch();
    }

    while (textures_54->GetCount() != 0) {
        (*textures_54->GetAt(0))->release();
        textures_54->RemoveAt(0);
    }
    delete textures_54;
}

// FUNCTION: WIZ8 0x00485400
void stTextureAnim::SetFrame(int frame)
{
    frame_58 = frame;
    frame_tick_6c = GetTickCount();
}

// FUNCTION: WIZ8 0x00485420
void stTextureAnim::AddTexture(srTextureIFace* texture)
{
    textures_54->Add(texture);
    texture->addReference();
}

// FUNCTION: WIZ8 0x004854B0
void stTextureAnim::UpdateFrame004854B0()
{
    int elapsed_frames;

    if (animation_mode_60 == 3) {
        return;
    }

    /* Retail scales rand() by the folded constant 1/32768 (0x005ec1e4),
       not a runtime division by RAND_MAX. */
    if (trigger_mode_70 == 1) {
        if (rand() * (1.0f / 32768.0f) < probability_74) {
            frame_58 = static_cast<int>(rand() * (1.0f / 32768.0f) * textures_54->GetCount());
        }
        return;
    }

    if (trigger_mode_70 == 2) {
        if (running_78 == 0 && rand() * (1.0f / 32768.0f) < probability_74) {
            running_78 = 1;
            direction_5c = 0;
            frame_58 = 0;
            frame_tick_6c = GetTickCount();
        }
        if (running_78 == 0 || textures_54->GetCount() == 0) {
            return;
        }
    } else if (textures_54->GetCount() == 0) {
        return;
    }

    elapsed_frames = (int)((GetTickCount() - frame_tick_6c) * frame_rate_68 * g_float_005ec128);
    if (animation_mode_60 == 0) {
        int frame = (direction_5c * elapsed_frames) % textures_54->GetCount();
        if (frame < frame_58) {
            frame_58 = 0;
            running_78 = 0;
            return;
        }
        frame_58 = frame;
    } else if (animation_mode_60 == 1) {
        if ((elapsed_frames / textures_54->GetCount() & 1) != 0) {
            direction_5c = -1;
            frame_58 = textures_54->GetCount() - elapsed_frames % textures_54->GetCount() - 1;
        } else {
            if (direction_5c == -1) {
                running_78 = 0;
                return;
            }
            direction_5c = 1;
            frame_58 = elapsed_frames % textures_54->GetCount();
        }
    } else if (animation_mode_60 == 2) {
        if (elapsed_frames >= textures_54->GetCount()) {
            running_78 = 0;
            frame_58 = textures_54->GetCount() - 1 < 0 ? 0 : textures_54->GetCount() - 1;
        } else {
            frame_58 = elapsed_frames;
        }
    }
}

// FUNCTION: WIZ8 0x00485730
int stTextureAnim::IsFinished00485730() const
{
    if (animation_mode_60 != 2) {
        return 0;
    }

    int final_frame = textures_54->GetCount() - 1;
    final_frame = final_frame < 0 ? 0 : final_frame;
    if (frame_58 != final_frame) {
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x004856F0
unsigned long stTextureAnim::getTextureFrameHandle()
{
    UpdateFrame004854B0();
    if ((texture_flags_ & (1UL << FLAG_DIRTY_DEFAULTS)) != 0) {
        setupDefaultValues();
    }
    return (*textures_54->GetAt(frame_58))->getTextureFrameHandle();
}

// FUNCTION: WIZ8 0x00484D60
float stTextureAnim::getPriority()
{
    return (*textures_54->GetAt(frame_58))->getPriority();
}

// FUNCTION: WIZ8 0x00484D80
void stTextureAnim::getDimensions(Dimensions& dimensions)
{
    (*textures_54->GetAt(frame_58))->getDimensions(dimensions);
}

// FUNCTION: WIZ8 0x00484DB0
void stTextureAnim::getMipmapData(MultiRequest& request)
{
    (*textures_54->GetAt(frame_58))->getMipmapData(request);
}

// FUNCTION: WIZ8 0x00484DE0
void stTextureAnim::getMipmapLevelPartial(PartialRequest& request)
{
    (*textures_54->GetAt(frame_58))->getMipmapLevelPartial(request);
}

// FUNCTION: WIZ8 0x00484E10
void stTextureAnim::getTextureParms(Parameters& parameters)
{
    (*textures_54->GetAt(frame_58))->getTextureParms(parameters);
}

// FUNCTION: WIZ8 0x00484E40
const char* stTextureAnim::getTextureName()
{
    return (*textures_54->GetAt(frame_58))->getTextureName();
}

void stTextureAnim::invalidate() {}

// FUNCTION: WIZ8 0x00485760
void stTextureAnim::setupDefaultValues()
{
    srTextureIFace* texture = *textures_54->GetAt(0);

    if (texture != 0) {
        texture->getDimensions(texture_dimensions_);
        texture_flags_ &= ~2U; /* FLAG_DIRTY_DEFAULTS */
    } else {
        texture_dimensions_.width = 1;
        texture_dimensions_.height = 1;
        texture_dimensions_.palette = 0;
    }
}

// FUNCTION: WIZ8 0x004857B0
unsigned char stTextureAnim::Prepare004857B0()
{
    srTextureIFace* texture = *textures_54->GetAt(0);

    if (texture != 0 && texture->getClassID() == 0x10001) {
        texture->getTextureFrameHandle();
        return static_cast<stTextureFile*>(texture)->hasAlpha();
    }
    return texture_dimensions_.format.alpha_bits != 0;
}
