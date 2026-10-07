#include "surrender/srVideoManager.h"

/* path is ignored; the importer subclasses open the file themselves. */
// FUNCTION: SURRENDER 0x1002DDD0
srVideoManager::Stream::Stream(const char* path)
{
    loaded = 0;
    parameter = 0;
    reset_pending = 1;
    clamp = 0;
    index = 0;
    position = 0;
}

// FUNCTION: SURRENDER 0x1002DE00
void srVideoManager::VStream::decompress(srColorSurfaceIFace& surface)
{
    Stream::Target target;
    target.flags = 0;
    target.source.left = 0;
    target.source.top = 0;
    target.source.right = info.width;
    target.source.bottom = info.height;
    target.surface.left = 0;
    target.surface.top = 0;
    target.surface.right = surface.getWidth();
    target.surface.bottom = surface.getHeight();
    stream->decompress(surface, target, stream->getIndex() + 1);
}

// FUNCTION: SURRENDER 0x1002DE60
void srVideoManager::VStream::init(Stream* stream)
{
    info.width = 1;
    info.height = 1;
    info.frame_count = 0;
    info.field_64 = 0;
    info.field_68 = 0;
    info.field_6c = 0;
    info.field_70 = 0;
    info.field_74 = 0;
    info.field_78 = 0;
    info.frames_per_second = 15.0f;
    this->stream = stream;
    this->stream->getInfo(&info);
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
