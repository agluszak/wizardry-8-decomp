#include "surrender/srVideoManager.h"

/* The retail Stream constructor ignores path; the importer subclasses open
   the file themselves. */
// FUNCTION: SURRENDER 0x1002DDD0
srVideoManager::Stream::Stream(const char* path)
{
    loaded_04 = 0;
    parameter_08 = 0;
    reset_pending_10 = 1;
    clamp_14 = 0;
    index_0c = 0;
    position_18 = 0;
}

// FUNCTION: SURRENDER 0x1002DE00
void srVideoManager::VStream::decompress(srColorSurfaceIFace& surface)
{
    Stream::Target target;
    target.flags_00 = 0;
    target.source_04.left = 0;
    target.source_04.top = 0;
    target.source_04.right = info_04.width_14;
    target.source_04.bottom = info_04.height_18;
    target.surface_14.left = 0;
    target.surface_14.top = 0;
    target.surface_14.right = surface.getWidth();
    target.surface_14.bottom = surface.getHeight();
    stream_00->decompress(surface, target, stream_00->getIndex() + 1);
}

// FUNCTION: SURRENDER 0x1002DE60
void srVideoManager::VStream::init(Stream* stream)
{
    info_04.width_14 = 1;
    info_04.height_18 = 1;
    info_04.frame_count_1c = 0;
    info_04.field_64 = 0;
    info_04.field_68 = 0;
    info_04.field_6c = 0;
    info_04.field_70 = 0;
    info_04.field_74 = 0;
    info_04.field_78 = 0;
    info_04.frames_per_second_20 = 15.0f;
    stream_00 = stream;
    stream_00->getInfo(&info_04);
}

// FUNCTION: SURRENDER 0x1002DEA0
srVideoManager::VStream* srVideoManager::openVStream(const char* path)
{
    if (path == 0 || *path == 0) {
        throw Error("srVideoManager::openVStream: Given filename is NULL or empty");
    }
    Importer* importer = findImporter(getExtension(path));
    if (importer == 0) {
        throw Error("srVideoManager::openVStream() - Importer for this file extension not found");
    }
    Stream* stream = static_cast<VideoImporter*>(importer)->openStream(path);
    if (stream == 0) {
        throw Error("srVideoManager::openVStream: Importer could not open video stream.");
    }
    VStream* vstream = new VStream;
    if (vstream != 0) {
        vstream->init(stream);
        return vstream;
    }
    return 0;
}

// SYNTHETIC: SURRENDER 0x1002DFF0
// srVideoManager::Stream scalar deleting destructor

// SYNTHETIC: SURRENDER 0x10016CB0
// srVideoManager::Stream default constructor closure
