#pragma once

#include "surrender/srTexture.h"
#include "wiz8/vector.h"

class stTextureAnim : public srClassSupport<stTextureAnim, srTexture, 0, 0x10000> {
public:
    enum { CLASS_ID = 0x10000 };

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
    int IsFinished00485730() const;
    unsigned char Prepare004857B0();
    virtual void setupDefaultValues() override; /* 0x00485760 */

protected:
    virtual ~stTextureAnim() override; /* 0x00485290 */

public:
    void UpdateFrame004854B0();

    W8GrowableVector<srTextureIFace*>* textures_54;
    int frame_58;
    int direction_5c;
    unsigned char animation_mode_60;
    int initial_frame_64;
    float frame_rate_68;
    unsigned long frame_tick_6c;
    int trigger_mode_70;
    float probability_74;
    bool running_78;
};

static_assert(sizeof(stTextureAnim) == 0x7c, "stTextureAnim_size_must_be_0x7c");
