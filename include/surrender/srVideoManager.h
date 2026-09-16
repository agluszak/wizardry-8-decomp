#pragma once

#include "srColorSurfaceIFace.h"
#include "srIOManager.h"

/* Provider-side video interface. Wizardry imports srCore::getVideoManager only
   as a getter returning this type; no known consumer imports a srVideoManager,
   Stream, or VStream-owned symbol. */
class srVideoManager : public srIOManager {
public:
    class Stream {
    protected:
        Stream(const char* path);
    };

    class VStream {
    public:
        void decompress(srColorSurfaceIFace& surface);

    protected:
        void init(Stream* stream);
        unsigned char unknown_00_[0x80];
    };

    VStream* openVStream(const char* path);
};

static_assert((sizeof(srVideoManager::VStream) == 0x80), "srVideoManager_VStream_must_be_0x80");
