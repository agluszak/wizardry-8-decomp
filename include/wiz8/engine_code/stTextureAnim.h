#pragma once

#include "surrender/srTexture.h"
#include "wiz8/vector.h"
#include "wiz8/texture_animation.h"

enum W8TextureTriggerMode {
    W8_TEXTURE_TRIGGER_CONTINUOUS = 0,
    W8_TEXTURE_TRIGGER_RANDOM_FRAME = 1,
    W8_TEXTURE_TRIGGER_RANDOM_START = 2
};

class stTextureAnim : public srClassSupport<stTextureAnim, srTexture, 0, 0x10000> {
public:
    static const char* sGetClassName()
    {
        return "stTextureAnim";
    }

    stTextureAnim();                           /* 0x00484BE0 */
    stTextureAnim(const stTextureAnim& other); /* 0x00485070 */

    virtual srClass* vInstance() override;                                /* 0x00484E60 */
    virtual unsigned long getTextureFrameHandle() override;               /* 0x004856F0 */
    virtual float getPriority() override;                                 /* 0x00484D60 */
    virtual void getDimensions(Dimensions& dimensions) override;          /* 0x00484D80 */
    virtual void getMipmapData(MultiRequest& request) override;           /* 0x00484DB0 */
    virtual void getMipmapLevelPartial(PartialRequest& request) override; /* 0x00484DE0 */
    virtual void getTextureParms(Parameters& parameters) override;        /* 0x00484E10 */
    virtual const char* getTextureName() override;                        /* 0x00484E40 */
    virtual void invalidate() override;                                   /* 0x004023A0 */

    void SetFrame(int frame);
    void AddTexture(srTextureIFace* texture);
    int IsFinished() const;
    unsigned char Prepare();
    virtual void setupDefaultValues() override; /* 0x00485760 */

protected:
    virtual ~stTextureAnim() override; /* 0x00485290 */

public:
    void UpdateFrame();

    W8Vector<srTextureIFace*>* textures;
    int frame;
    int direction;
    W8TextureAnimationMode animation_mode;
    int initial_frame;
    float frame_rate;
    unsigned long frame_tick;
    W8TextureTriggerMode trigger_mode;
    float probability;
    bool running;
};

static_assert(sizeof(stTextureAnim) == 0x7c, "stTextureAnim_size_must_be_0x7c");
