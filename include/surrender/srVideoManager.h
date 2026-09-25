#pragma once

#include "srColorSurfaceIFace.h"
#include "srIOManager.h"
#include "srPixelConvert.h"

/* Provider-side video interface. Wizardry imports srCore::getVideoManager only
   as a getter returning this type; no known consumer imports a srVideoManager,
   Stream, or VStream-owned symbol. */
// VTABLE: SURRENDER 0x100769A0 srVideoManager::Stream
class srVideoManager : public srIOManager {
public:
    class Stream {
    public:
        /* Per-frame target built by VStream::decompress: the frame extent
           pairs with the destination surface extent behind a leading dword
           that sr.dll always writes 0. */
        struct Target {
            long flags_00;
            srColorSurfaceIFace::Rectangle source_04;
            srColorSurfaceIFace::Rectangle surface_14;
        };

        /* Stream description the importer fills through getInfo. The video
           extensions memset the block, mapPixelFormat the leading format,
           then write the extent/count/rate fields and a description string. */
        struct Info {
            srPixelConvert::PixelFormat pixel_format_00;
            unsigned long width_14;
            unsigned long height_18;
            unsigned long frame_count_1c;
            float frames_per_second_20;
            char description_24[0x40];
            unsigned long field_64;
            unsigned long field_68;
            unsigned long field_6c;
            unsigned long field_70;
            unsigned long field_74;
            unsigned long field_78;

            /* VStream's implicit constructor emits a single store at +0x10,
               the flags dword of the embedded pixel format. */
            Info() { pixel_format_00.flags = 0; }
        };

        virtual ~Stream() {}
// FUNCTION: SURRENDER 0x1002DF90
        virtual int isLoaded() { return loaded_04; }
        /* Base reset only marks the rewind pending; srEXT_FLIC's override
           rewalks the chunk headers itself. */
// FUNCTION: SURRENDER 0x1002DFA0
        virtual void reset() { reset_pending_10 = 1; }
// FUNCTION: SURRENDER 0x1002DFB0
        virtual long getIndex() { return index_0c; }
        /* The +0x08 payload's semantics are unresolved; no shipping importer
           reads it. */
// FUNCTION: SURRENDER 0x1002DFC0
        virtual void setParameter(long parameter) { parameter_08 = parameter; }
        /* Nonzero clamps an out-of-range frame request; zero wraps it and
           rewinds (srEXT_FLIC). */
// FUNCTION: SURRENDER 0x1002DFD0
        virtual void setClamp(long clamp) { clamp_14 = clamp; }
// FUNCTION: SURRENDER 0x1002DFE0
        virtual void setPosition(long position) { position_18 = position; }
        virtual void getInfo(Info* info) = 0;
        virtual void decompress(srColorSurfaceIFace& surface, const Target& target,
                                long frame) = 0;

    protected:
        Stream(const char* path = 0);

        long loaded_04;
        long parameter_08;
        long index_0c;
        long reset_pending_10;
        long clamp_14;
        long position_18;
    };

    /* Importer-side video entry: the video extensions register a subclass
       whose openStream allocates its Stream (srEXT_FLIC drops the object when
       the file fails to load). */
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

        Stream* stream_00;
        Stream::Info info_04;
    };

    VStream* openVStream(const char* path);
};

static_assert((sizeof(srVideoManager::VStream) == 0x80), "srVideoManager_VStream_must_be_0x80");
static_assert((sizeof(srVideoManager::Stream::Info) == 0x7c), "srStreamInfo_must_be_0x7c");
