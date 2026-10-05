#pragma once

/* Animation mode is a byte in both the material record and texture object. */
typedef unsigned char W8TextureAnimationMode;

enum {
    W8_TEXTURE_ANIM_LOOP = 0,
    W8_TEXTURE_ANIM_PING_PONG = 1,
    W8_TEXTURE_ANIM_PLAY_ONCE = 2,
    W8_TEXTURE_ANIM_MANUAL = 3
};
