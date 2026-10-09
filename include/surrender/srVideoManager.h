#pragma once

#include "srColorSurfaceIFace.h"
#include "srIOManager.h"
#include "srPixelConvert.h"

// VTABLE: SURRENDER 0x100769A0 srVideoManager::Stream
class srVideoManager : public srIOManager {
public:
    class Stream {
    public:
        /* Per-frame target built by VStream::decompress; the leading dword is always 0. */
        struct Target {
            w8_long flags;
            srColorSurfaceIFace::Rectangle source;
            srColorSurfaceIFace::Rectangle surface;
        };

        /* Stream description the importer fills through getInfo. */
        struct Info {
            srPixelConvert::PixelFormat pixel_format;
            w8_ulong width;
            w8_ulong height;
            w8_ulong frame_count;
            float frames_per_second;
            char description[0x40];
            w8_ulong field_64;
            w8_ulong field_68;
            w8_ulong field_6c;
            w8_ulong field_70;
            w8_ulong field_74;
            w8_ulong field_78;

            Info()
            {
                pixel_format.fourcc = 0;
            }
        };

        virtual ~Stream() {}
        // FUNCTION: SURRENDER 0x1002DF90
        virtual int isLoaded()
        {
            return loaded;
        }
        /* Base reset only marks the rewind pending; srEXT_FLIC's override
           rewalks the chunk headers itself. */
        // FUNCTION: SURRENDER 0x1002DFA0
        virtual void reset()
        {
            reset_pending = 1;
        }
        // FUNCTION: SURRENDER 0x1002DFB0
        virtual w8_long getIndex()
        {
            return index;
        }
        // FUNCTION: SURRENDER 0x1002DFC0
        virtual void setParameter(w8_long parameter)
        {
            this->parameter = parameter;
        }
        /* Nonzero clamps an out-of-range frame request; zero wraps it and
           rewinds (srEXT_FLIC). */
        // FUNCTION: SURRENDER 0x1002DFD0
        virtual void setClamp(w8_long clamp)
        {
            this->clamp = clamp;
        }
        // FUNCTION: SURRENDER 0x1002DFE0
        virtual void setPosition(w8_long position)
        {
            this->position = position;
        }
        virtual void getInfo(Info* info) = 0;
        virtual void decompress(srColorSurfaceIFace& surface, const Target& target,
                                w8_long frame) = 0;

    protected:
        SR_DLL_EXPORT Stream(const char* path = 0);

        w8_long loaded;
        w8_long parameter;
        w8_long index;
        w8_long reset_pending;
        w8_long clamp;
        w8_long position;
    };

    /* Importer-side video entry: the video extensions register a subclass whose openStream
       allocates its Stream. */
    class VideoImporter : public srIOManager::Importer {
    public:
        virtual Stream* openStream(const char* path) = 0;
    };

    class VStream {
    public:
        void decompress(srColorSurfaceIFace& surface);

    private:
        friend class srVideoManager;

        void init(Stream* stream);

        Stream* stream;
        Stream::Info info;
    };

    VStream* openVStream(const char* path);
};

W8_ABI_ASSERT((sizeof(srVideoManager::VStream) == 0x80), "srVideoManager_VStream_must_be_0x80");
W8_ABI_ASSERT((sizeof(srVideoManager::Stream::Info) == 0x7c), "srStreamInfo_must_be_0x7c");
