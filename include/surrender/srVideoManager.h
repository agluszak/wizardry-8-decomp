#pragma once

#include "srColorSurfaceIFace.h"
#include "srIOManager.h"

class srVideoManager : public srIOManager {
public:
    class Stream {
    protected:
        SR_DLL_IMPORT Stream(const char* path);
    };

    class VStream {
    public:
        SR_DLL_IMPORT void decompress(srColorSurfaceIFace& surface);

    protected:
        SR_DLL_IMPORT void init(Stream* stream);
        unsigned char unknown_00_[0x80];
    };

    SR_DLL_IMPORT VStream* openVStream(const char* path);
};

static_assert((sizeof(srVideoManager::VStream) == 0x80), "srVideoManager_VStream_must_be_0x80");
