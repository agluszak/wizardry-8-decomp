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

// FUNCTION: WIZ8 0x00484BE0
stTextureAnim::stTextureAnim()
{
    textures = 0;
    frame = 0;
    direction = 1;
    animation_mode = 0;
    initial_frame = 0;
    frame_rate = 15.0f;
    frame_tick = GetTickCount();
    trigger_mode = 0;
    probability = -1.0f;
    running = false;
    textures = new W8Vector<srTextureIFace*>;
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

    textures = 0;
    frame = 0;
    direction = 1;
    animation_mode = other.animation_mode;
    initial_frame = other.initial_frame;
    frame_rate = other.frame_rate;
    frame_tick = GetTickCount();
    trigger_mode = other.trigger_mode;
    probability = other.probability;
    running = other.running;
    textures = new W8Vector<srTextureIFace*>;

    for (i = 0; i < other.textures->GetCount(); ++i) {
        srTextureIFace* texture = *other.textures->GetAt(i);
        textures->Add(texture);
        texture->addReference();
    }
}

// FUNCTION: WIZ8 0x00485290
stTextureAnim::~stTextureAnim()
{
    if (IsTextureInReadMeshScratch(this)) {
        ReleaseReadMeshScratch();
    }

    while (textures->GetCount() != 0) {
        (*textures->GetAt(0))->release();
        textures->RemoveAt(0);
    }
    delete textures;
}

// FUNCTION: WIZ8 0x00485400
void stTextureAnim::SetFrame(int frame)
{
    this->frame = frame;
    frame_tick = GetTickCount();
}

// FUNCTION: WIZ8 0x00485420
void stTextureAnim::AddTexture(srTextureIFace* texture)
{
    textures->Add(texture);
    texture->addReference();
}

// FUNCTION: WIZ8 0x004854B0
void stTextureAnim::UpdateFrame()
{
    int elapsed_frames;

    if (animation_mode == 3) {
        return;
    }

    if (trigger_mode == 1) {
        if (rand() / static_cast<float>(RAND_MAX) < probability) {
            this->frame =
                static_cast<int>(rand() / static_cast<float>(RAND_MAX) * textures->GetCount());
        }
        return;
    }

    if (trigger_mode == 2) {
        if (!running && rand() / static_cast<float>(RAND_MAX) < probability) {
            running = true;
            direction = 0;
            this->frame = 0;
            frame_tick = GetTickCount();
        }
        if (!running || textures->GetCount() == 0) {
            return;
        }
    } else if (textures->GetCount() == 0) {
        return;
    }

    elapsed_frames =
        static_cast<int>((GetTickCount() - frame_tick) * frame_rate * g_float_005ec128);
    if (animation_mode == 0) {
        int frame = (direction * elapsed_frames) % textures->GetCount();
        if (frame < this->frame) {
            this->frame = 0;
            running = false;
            return;
        }
        this->frame = frame;
    } else if (animation_mode == 1) {
        if ((elapsed_frames / textures->GetCount() & 1) != 0) {
            direction = -1;
            this->frame = textures->GetCount() - elapsed_frames % textures->GetCount() - 1;
        } else {
            if (direction == -1) {
                running = false;
                return;
            }
            direction = 1;
            this->frame = elapsed_frames % textures->GetCount();
        }
    } else if (animation_mode == 2) {
        if (elapsed_frames >= textures->GetCount()) {
            running = false;
            this->frame = textures->GetCount() - 1 < 0 ? 0 : textures->GetCount() - 1;
        } else {
            this->frame = elapsed_frames;
        }
    }
}

// FUNCTION: WIZ8 0x00485730
int stTextureAnim::IsFinished() const
{
    if (animation_mode != 2) {
        return 0;
    }

    int final_frame = textures->GetCount() - 1;
    final_frame = final_frame < 0 ? 0 : final_frame;
    if (frame != final_frame) {
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x004856F0
unsigned long stTextureAnim::getTextureFrameHandle()
{
    UpdateFrame();
    if ((texture_flags_ & (1UL << FLAG_DIRTY_DEFAULTS)) != 0) {
        setupDefaultValues();
    }
    return (*textures->GetAt(frame))->getTextureFrameHandle();
}

// FUNCTION: WIZ8 0x00484D60
float stTextureAnim::getPriority()
{
    return (*textures->GetAt(frame))->getPriority();
}

// FUNCTION: WIZ8 0x00484D80
void stTextureAnim::getDimensions(Dimensions& dimensions)
{
    (*textures->GetAt(frame))->getDimensions(dimensions);
}

// FUNCTION: WIZ8 0x00484DB0
void stTextureAnim::getMipmapData(MultiRequest& request)
{
    (*textures->GetAt(frame))->getMipmapData(request);
}

// FUNCTION: WIZ8 0x00484DE0
void stTextureAnim::getMipmapLevelPartial(PartialRequest& request)
{
    (*textures->GetAt(frame))->getMipmapLevelPartial(request);
}

// FUNCTION: WIZ8 0x00484E10
void stTextureAnim::getTextureParms(Parameters& parameters)
{
    (*textures->GetAt(frame))->getTextureParms(parameters);
}

// FUNCTION: WIZ8 0x00484E40
const char* stTextureAnim::getTextureName()
{
    return (*textures->GetAt(frame))->getTextureName();
}

void stTextureAnim::invalidate() {}

// FUNCTION: WIZ8 0x00485760
void stTextureAnim::setupDefaultValues()
{
    srTextureIFace* texture = *textures->GetAt(0);

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
unsigned char stTextureAnim::Prepare()
{
    srTextureIFace* texture = *textures->GetAt(0);

    if (texture != 0 && texture->getClassID() == stTextureFile::CLASS_ID) {
        texture->getTextureFrameHandle();
        return static_cast<stTextureFile*>(texture)->hasAlpha();
    }
    return texture_dimensions_.format.alpha_bits != 0;
}
