#pragma once

#include "surrender/srTexture.h"

/* The 0x005ECA04 table is the abstract Wizardry animated-texture base over
   srTexture. Its concrete implementation supplies vInstance, invalidate, and
   update while sharing this class identity and the playback state below. */
// VTABLE: WIZ8 0x005eca04
class stTextureAnim : public srTexture {
public:
    enum { CLASS_ID = 0x10000 };

    const char* getClassName() const override;             /* 0x00485800 */
    srRegistry::ClassNode* getClassNode() const override;  /* 0x00485810 */
    unsigned long getClassID() const override;             /* 0x004857F0 */
    virtual srClass* vInstance() override = 0;
    virtual srTextureIFace* clone() override;              /* 0x004858B0 */

    void SetFrame00485400(int frame);
    unsigned char Prepare004857B0();

protected:
    virtual ~stTextureAnim() override;                     /* 0x00485910 */

public:
    void* object_54;
    int frame_58;
    int value_5c;
    unsigned char flag_60;
    unsigned char unknown_61[3];
    int value_64;
    float frame_rate_68;
    unsigned long frame_tick_6c;
    int value_70;
    float value_74;
    unsigned char flag_78;
    unsigned char unknown_79[3];
};

static_assert(sizeof(stTextureAnim) == 0x7c, "stTextureAnim_size_must_be_0x7c");
