#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <new>

#include "surrender/srCore.h"
#include "surrender/srExporter.h"
#include "surrender/srPlugin.h"

#include "codec_adapter.h"
#include "layout.h"

class srJPEGColorSurface : public srColorSurface {
public:
    static void* operator new(size_t size)
    {
        return srHeap.allocate(size);
    }

    static void operator delete(void* memory)
    {
        srHeap.free(memory);
    }

    srJPEGColorSurface(
        srPixelConvert::e_surfaceType type,
        unsigned long width,
        unsigned long height)
        : srColorSurface(type, width, height) {}

    virtual const char* getClassName() const;
    virtual unsigned long getClassID() const;
    virtual srRegistry::ClassNode* getClassNode() const;
    virtual srColorSurfaceIFace* clone();
};

// SYNTHETIC: SREXT_JPEGIMPORTER 0x100151D0
// srJPEGColorSurface::`scalar deleting destructor'

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
    void initializeCodecOptions();
    bool readHeader(void* input_cookie);

    JpegCodecState32 codec_;
    JpegExportOptions32 export_options_;
};

// SYNTHETIC: SREXT_JPEGIMPORTER 0x10014DB0
// srJPEGImporter::`scalar deleting destructor'
// SYNTHETIC: SREXT_JPEGIMPORTER 0x100155E0
// ?getTypeName@srJPEGImporter@@W3BEPBDXZ
// SYNTHETIC: SREXT_JPEGIMPORTER 0x100155F0
// ??_EsrJPEGImporter@@W3AEPAXI@Z

class srJPEGPlugin : public srPlugin {
public:
    virtual ~srJPEGPlugin();
    virtual const char* getDescription() const;

private:
    srJPEGImporter jpeg_importer_;
};

// SYNTHETIC: SREXT_JPEGIMPORTER 0x10014BB0
// srJPEGPlugin::`scalar deleting destructor'
// SYNTHETIC: SREXT_JPEGIMPORTER 0x10014BD0
// srJPEGPlugin::~srJPEGPlugin

srJPEGPlugin::~srJPEGPlugin()
{
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014D40
srJPEGImporter::srJPEGImporter()
{
    srSurfaceIOManager* manager = srCore.getSurfaceIOManager();
    if (manager != 0) {
        addToImporters(manager, "jpg");
        addToImporters(manager, "jpeg");
        addToExporters(manager, "jpg");
        addToExporters(manager, "jpeg");
    }
    initializeCodecOptions();
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014DD0
srJPEGImporter::~srJPEGImporter()
{
    srSurfaceIOManager* manager = srCore.getSurfaceIOManager();
    if (manager != 0) {
        removeFromImporters(manager);
        removeFromExporters(manager);
    }
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014E10
void srJPEGImporter::initializeCodecOptions()
{
    export_options_.limit_200 = 200;
    export_options_.quality = 75;
    export_options_.smoothing_factor = 0;
    export_options_.pointer_08 = 0;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10015420
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

    const int components = codec_.components;
    const unsigned long height = codec_.height;
    const unsigned long width = codec_.width;
    srJPEGColorSurface* surface;
    if (components == 1) {
        surface = new srJPEGColorSurface(
            srPixelConvert::SURFACE_L8, width, height);
    }
    else if (components == 3) {
        surface = new srJPEGColorSurface(
            srPixelConvert::SURFACE_BGR24, width, height);
    }
    else if (components == 4) {
        surface = new srJPEGColorSurface(
            srPixelConvert::SURFACE_BGRA32, width, height);
    }
    else {
        return 0;
    }

    const unsigned long data_size = components * height * width;
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

    unsigned long rgb_row_offset = 0;
    const unsigned char* source_row = decoded;
    for (unsigned long y = 0; y < height; ++y) {
        unsigned char* destination =
            static_cast<unsigned char*>(surface->asInterface()->getDataPtr())
            + surface->rowPitch() * y;
        if (components == 1) {
            memcpy(destination, source_row, width);
        }
        else if (components == 3) {
            const unsigned char* source = decoded + rgb_row_offset + 1;
            for (unsigned long x = 0; x < width; ++x) {
                destination[0] = source[1];
                destination[1] = source[0];
                destination[2] = source[-1];
                destination += 3;
                source += 3;
            }
        }
        else if (components == 4) {
            const unsigned long* source =
                reinterpret_cast<const unsigned long*>(source_row);
            unsigned long* destination_pixel =
                reinterpret_cast<unsigned long*>(destination);
            for (unsigned long x = 0; x < width; ++x) {
                const unsigned long pixel = *source++;
                *destination_pixel++ =
                    ((pixel & 0x00ff0000UL | pixel >> 16) >> 8)
                    | ((pixel << 16 | pixel & 0x0000ff00UL) << 8);
            }
        }
        rgb_row_offset += width * 3;
        source_row += components * width;
    }

    srJPEG_active_input_stream = 0;
    ::operator delete(decoded);
    return surface->asInterface();
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10015400
static void initializeExportOptions(JpegExportOptions32* options)
{
    options->limit_200 = 200;
    options->quality = 100;
    options->smoothing_factor = 0;
    options->pointer_08 = 0;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10015200
void srJPEGImporter::exportSurface(
    srBinOStream& stream,
    srColorSurfaceIFace& source,
    const srSurfaceIOManager::ExportInfo& options)
{
    srJPEG_active_output_stream = &stream;
    stream.seek(0, srBinStream::SR_SEEK_BEGIN);

    initializeExportOptions(&export_options_);
    export_options_.quality = 100;
    if (options.option_string != 0) {
        const unsigned long length = strlen(options.option_string) + 1;
        char* option_string = static_cast<char*>(::operator new(length));
        strcpy(option_string, options.option_string);
        for (char* cursor = option_string; *cursor != '\0'; ++cursor) {
            *cursor = static_cast<char>(toupper(*cursor));
        }
        char* quality = strstr(option_string, "QUALITY");
        if (quality != 0) {
            quality = strchr(quality, '=');
            if (quality != 0) {
                while (*quality == ' ' || *quality == '=') {
                    ++quality;
                }
                char* end = strchr(quality, ';');
                if (end != 0) {
                    *end = '\0';
                }
                export_options_.quality = static_cast<unsigned char>(atof(quality));
            }
        }
        ::operator delete(option_string);
    }

    srColorSurface* source_surface = srColorSurface::fromInterface(source);
    srJPEGColorSurface* copy = new srJPEGColorSurface(
        srPixelConvert::SURFACE_COPY,
        source_surface->width(),
        source_surface->height());
    copy->asInterface()->copy(source);

    memset(&codec_, 0, sizeof(codec_));
    codec_.pixels = static_cast<unsigned char*>(copy->asInterface()->getDataPtr());
    codec_.width = source_surface->width();
    codec_.height = source_surface->height();
    codec_.output_stdio_cookie = 0;
    codec_.arithmetic_coding = 0;
    codec_.ccir601_sampling = 0;
    codec_.smoothing_factor = export_options_.smoothing_factor;
    codec_.quality = export_options_.quality;
    srJPEG_encode_adapter(&codec_);
    copy->asInterface()->release();
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10015450
unsigned long srJPEGColorSurface::getClassID() const
{
    return 0x3110;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10015460
const char* srJPEGColorSurface::getClassName() const
{
    return sGetClassName();
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10015470
srRegistry::ClassNode* srJPEGColorSurface::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x3110);
    if (node == 0) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x3100);
        if (parent == 0) {
            parent = parent_registry->registerClass(
                srColorSurfaceIFace::sGetClassName(),
                srClass::sGetClassNode(),
                0x3100,
                1);
        }
        node = registry->registerClass(sGetClassName(), parent, 0x3110, 0);
    }
    return node;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x100154E0
srColorSurfaceIFace* srJPEGColorSurface::clone()
{
    srColorSurface* result = static_cast<srColorSurface*>(vInstance());
    *result = *this;
    return result;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014BA0
const char* srJPEGPlugin::getDescription() const
{
    return "SurRender JPEG-importer/exporter plug-in";
}

// FUNCTION: SREXT_JPEGIMPORTER 0x100155B0
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
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
typedef char srJPEGColorSurface_must_be_0x5c[
    (sizeof(srJPEGColorSurface) == 0x5c) ? 1 : -1];
