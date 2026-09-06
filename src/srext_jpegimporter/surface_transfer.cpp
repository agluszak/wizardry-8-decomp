#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#include "codec_adapter.h"
#include "plugin_classes.h"

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
    srColorSurface* surface;
    switch (components) {
    case 1:
        surface = new srClassSupport<srColorSurface, srColorSurface, false, 0x3110>(
            srPixelConvert::SURFACE_L8, width, height);
        break;
    case 3:
        surface = new srClassSupport<srColorSurface, srColorSurface, false, 0x3110>(
            srPixelConvert::SURFACE_BGR24, width, height);
        break;
    case 4:
        surface = new srClassSupport<srColorSurface, srColorSurface, false, 0x3110>(
            srPixelConvert::SURFACE_BGRA32, width, height);
        break;
    default:
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
            surface->release();
        }
        return 0;
    }

    unsigned long rgb_row_offset = 0;
    const unsigned char* source_row = decoded;
    for (unsigned long y = 0; y < height; ++y) {
        unsigned char* destination =
            static_cast<unsigned char*>(surface->getDataPtr())
            + surface->getPitch() * y;
        switch (components) {
        case 1:
            memcpy(destination, source_row, width);
            break;
        case 3: {
            const unsigned char* source = decoded + rgb_row_offset + 1;
            for (unsigned long x = 0; x < width; ++x) {
                destination[0] = source[1];
                destination[1] = source[0];
                destination[2] = source[-1];
                destination += 3;
                source += 3;
            }
            break;
        }
        case 4: {
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
            break;
        }
        }
        rgb_row_offset += width * 3;
        source_row += components * width;
    }

    srJPEG_active_input_stream = 0;
    ::operator delete(decoded);
    return surface;
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
                double normalized_quality = atof(quality);
                if (normalized_quality < 0.0) {
                    normalized_quality = 0.0;
                }
                else if (normalized_quality > 1.0) {
                    normalized_quality = 1.0;
                }
                export_options_.quality = static_cast<unsigned char>(
                    normalized_quality * 100.0);
            }
        }
        ::operator delete(option_string);
    }

    srColorSurface* copy =
        new srClassSupport<srColorSurface, srColorSurface, false, 0x3110>(
            srPixelConvert::SURFACE_COPY,
            source.getWidth(),
            source.getHeight());
    copy->copy(source);

    memset(&codec_, 0, sizeof(codec_));
    codec_.pixels = static_cast<unsigned char*>(copy->getDataPtr());
    codec_.width = source.getWidth();
    codec_.height = source.getHeight();
    codec_.output_stdio_cookie = 0;
    codec_.arithmetic_coding = 0;
    codec_.ccir601_sampling = 0;
    codec_.smoothing_factor = export_options_.smoothing_factor;
    codec_.quality = export_options_.quality;
    srJPEG_encode_adapter(&codec_);
    copy->release();
}
