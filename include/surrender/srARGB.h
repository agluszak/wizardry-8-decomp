#pragma once

#include "srHeap.h"

/* Four-byte packed color. Palette getColor/setColor/setColors and
   Sampler::shiftDown index and copy it as a dword of bytes
   (srPalette::getColor 0x100048a0, setColor 0x100048d0, setColors
   0x10004850, shiftDown 0x100062a0). Memory order is B,G,R,A: the
   little-endian 0xAARRGGBB dword. Sampler::addColor writes 0xff into
   byte 3 (alpha) and hashes bytes 0-2; Quantizer::quantize and
   Optimizer::setupLUT likewise consume B,G,R and skip alpha.

   e_index is the logical ARGB channel, not a memory offset.
   getChannelStatistics (0x10059240) reads byte (3 - channel) of each
   packed pixel, so INDEX_ALPHA lands at +3 and INDEX_BLUE at +0. */
class srARGB {
public:
    enum e_index { INDEX_ALPHA = 0, INDEX_RED = 1, INDEX_GREEN = 2, INDEX_BLUE = 3 };

    /* User-provided trivial constructor like srVector2T's: the color-surface
       row buffers emit the new[] result select and element-count bound after
       srHeap::allocate (scaleHorizontal 0x10059AC0, composite 0x100589D0),
       which only appears when the element type is not POD. */
    srARGB() {}

    /* This is a class-wide allocation contract, not a local codegen device:
       every reviewed srARGB[] color-surface row-buffer site routes through
       srHeap and carries the same new[] lowering. */
    void* operator new[](unsigned int size)
    {
        return srHeap.allocate(size);
    }

    void operator delete[](void* allocation)
    {
        srHeap.free(allocation);
    }

    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char alpha;
};

static_assert(sizeof(srARGB) == 4, "srARGB_must_be_4");
