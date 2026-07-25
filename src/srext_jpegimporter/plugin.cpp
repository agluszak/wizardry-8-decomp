#include <string.h>
#include <new>

#include "surrender/srCore.h"
#include "surrender/srExporter.h"
#include "surrender/srPlugin.h"

#include "codec_adapter.h"
#include "layout.h"

class srJPEGImporter :
    public srSurfaceIOManager::SurfaceImporter,
    public srSurfaceIOManager::SurfaceExporter {
public:
    srJPEGImporter();
    virtual ~srJPEGImporter();

    virtual const char* getTypeName() const;
    virtual int getSurfaceDesc(
        srColorSurfaceIFace::SurfaceDesc& description,
        srBinIStream& stream,
        const srSurfaceIOManager::ImportInfo& options);
    virtual srColorSurfaceIFace* importSurface(
        srBinIStream& stream,
        const srSurfaceIOManager::ImportInfo& options);
    virtual void exportSurface(
        srBinOStream& stream,
        srColorSurfaceIFace& surface,
        const srSurfaceIOManager::ExportInfo& options);

private:
    bool readHeader(void* input_cookie);

    JpegCodecState32 codec_;
    JpegExportOptions32 export_options_;
};

class srJPEGPlugin : public srPlugin {
public:
    virtual ~srJPEGPlugin() {}
    virtual const char* getDescription() const;

private:
    srJPEGImporter jpeg_importer_;
};

srJPEGImporter::srJPEGImporter()
{
    srSurfaceIOManager* manager = srCore.getSurfaceIOManager();
    addToImporters(manager, "jpeg");
    addToImporters(manager, "jpg");
    addToExporters(manager, "jpeg");
    addToExporters(manager, "jpg");
}

srJPEGImporter::~srJPEGImporter()
{
    srSurfaceIOManager* manager = srCore.getSurfaceIOManager();
    removeFromImporters(manager);
    removeFromExporters(manager);
}

const char* srJPEGImporter::getTypeName() const
{
    return "JPEG";
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014E30
bool srJPEGImporter::readHeader(void* input_cookie)
{
    memset(&codec_, 0, sizeof(codec_));
    codec_.input_stdio_cookie = input_cookie;
    srJPEG_read_header_adapter(&codec_);
    return codec_.failed == 0;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014E60
int srJPEGImporter::getSurfaceDesc(
    srColorSurfaceIFace::SurfaceDesc& description,
    srBinIStream& stream,
    const srSurfaceIOManager::ImportInfo&)
{
    srJPEG_active_input_stream = &stream;
    if (!readHeader(0)) {
        return 0;
    }

    memset(&description, 0, sizeof(description));
    switch (codec_.components) {
    case 1:
        srPixelConvert::mapPixelFormat(
            srPixelConvert::SURFACE_L8, description.pixel_format);
        break;
    case 3:
        srPixelConvert::mapPixelFormat(
            srPixelConvert::SURFACE_BGR24, description.pixel_format);
        break;
    case 4:
        srPixelConvert::mapPixelFormat(
            srPixelConvert::SURFACE_BGRA32, description.pixel_format);
        break;
    default:
        return 0;
    }
    description.width = codec_.width;
    description.height = codec_.height;
    description.pitch = codec_.width * codec_.components;
    return 1;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014F30
srColorSurfaceIFace* srJPEGImporter::importSurface(
    srBinIStream& stream,
    const srSurfaceIOManager::ImportInfo&)
{
    srJPEG_active_input_stream = &stream;
    if (!readHeader(0)) {
        return 0;
    }

    srPixelConvert::e_surfaceType surface_type;
    switch (codec_.components) {
    case 1:
        surface_type = srPixelConvert::SURFACE_L8;
        break;
    case 3:
        surface_type = srPixelConvert::SURFACE_BGR24;
        break;
    case 4:
        surface_type = srPixelConvert::SURFACE_BGRA32;
        break;
    default:
        return 0;
    }

    void* storage = srHeap.allocate(sizeof(srColorSurface));
    srColorSurface* surface = storage == 0
        ? 0
        : new (storage) srColorSurface(surface_type, codec_.width, codec_.height);

    const unsigned long data_size = codec_.components * codec_.width * codec_.height;
    unsigned char* decoded = static_cast<unsigned char*>(::operator new(data_size));
    codec_.pixels = decoded;
    stream.seek(0, srBinStream::SR_SEEK_BEGIN);
    srJPEG_decode_adapter(&codec_);
    if (codec_.failed != 0) {
        ::operator delete(decoded);
        if (surface != 0) {
            surface->asInterface()->release();
        }
        return 0;
    }

    const unsigned char* source = decoded;
    for (unsigned long y = 0; y < codec_.height; ++y) {
        unsigned char* destination =
            static_cast<unsigned char*>(surface->asInterface()->getDataPtr())
            + surface->rowPitch() * y;
        if (codec_.components == 1) {
            memcpy(destination, source, codec_.width);
        }
        else if (codec_.components == 3) {
            for (unsigned long x = 0; x < codec_.width; ++x) {
                destination[0] = source[2];
                destination[1] = source[1];
                destination[2] = source[0];
                destination += 3;
                source += 3;
            }
        }
        else {
            for (unsigned long x = 0; x < codec_.width; ++x) {
                destination[0] = source[2];
                destination[1] = source[1];
                destination[2] = source[0];
                destination[3] = source[3];
                destination += 4;
                source += 4;
            }
        }
        if (codec_.components != 3 && codec_.components != 4) {
            source += codec_.width * codec_.components;
        }
    }

    srJPEG_active_input_stream = 0;
    ::operator delete(decoded);
    return surface->asInterface();
}

void srJPEGImporter::exportSurface(
    srBinOStream&,
    srColorSurfaceIFace&,
    const srSurfaceIOManager::ExportInfo&)
{
}

const char* srJPEGPlugin::getDescription() const
{
    return "SurRender JPEG-importer/exporter plug-in";
}

// FUNCTION: SREXT_JPEGIMPORTER 0x100155D0
extern "C" unsigned long __cdecl srGetLibraryVersion()
{
    return 0x012A0209UL;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014B70
extern "C" srPlugin* __cdecl srInitPlugin()
{
    return new srJPEGPlugin;
}

typedef char srJPEGImporter_must_be_0x44[(sizeof(srJPEGImporter) == 0x44) ? 1 : -1];
typedef char srJPEGPlugin_must_be_0x48[(sizeof(srJPEGPlugin) == 0x48) ? 1 : -1];
