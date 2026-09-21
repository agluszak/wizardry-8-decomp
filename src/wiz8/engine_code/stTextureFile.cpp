#include "wiz8/engine_code/ReadMesh.h"
#include "wiz8/engine_code/stTextureFile.h"

#include "wiz8/engine_code/ReadLevel.h"
#include "FileMan.h"
#include "surrender/srCore.h"
#include "surrender/srHeap.h"
#include "surrender/srPalette.h"
#include "surrender/srTypeRegistry.h"
#include "wiz8/virtual_file.h"

#include <string.h>

// GLOBAL: WIZ8 0x0065A138
unsigned char* g_tga_file_data_0065a138;
// GLOBAL: WIZ8 0x0065A13C
unsigned int g_tga_file_data_capacity_0065a13c;

// FUNCTION: WIZ8 0x0047BC80
void LoadSurfacePixels0047BC80(int handle, srColorSurface* surface, const W8TgaHeader* header)
{
    srPixelConvert::PixelFormat format;
    unsigned char* file_data;
    unsigned int file_size;
    unsigned int file_position;
    unsigned int data_size;
    unsigned int bytes_per_pixel;
    unsigned int width;
    unsigned int height;
    unsigned int row_size;
    unsigned char* destination;

    surface->getPixelFormat(format);
    bytes_per_pixel = format.bytes_per_pixel_minus_one + 1;
    width = header->width;
    height = header->height;
    row_size = width * bytes_per_pixel;
    file_size = FileGetSize(handle);
    file_position = FileGetPos(handle);
    data_size = file_size - file_position;

    if (g_tga_file_data_capacity_0065a13c < data_size) {
        if (g_tga_file_data_0065a138 != 0) {
            srHeap.free(g_tga_file_data_0065a138);
        }
        g_tga_file_data_0065a138 = static_cast<unsigned char*>(srHeap.allocate(data_size));
        g_tga_file_data_capacity_0065a13c = data_size;
    }
    file_data = g_tga_file_data_0065a138;
    if (data_size == 0 || file_data == 0 || !FileRead(handle, file_data, data_size, 0)) {
        return;
    }

    destination = static_cast<unsigned char*>(surface->getDataPtr());
    if (destination == 0) {
        return;
    }

    if (header->image_type == 9 || header->image_type == 10 || header->image_type == 11) {
        unsigned int source_offset = 0;
        unsigned int pixel_offset = 0;
        unsigned int pixel_count = width * height;

        while (pixel_offset < pixel_count && source_offset < data_size) {
            unsigned char packet = file_data[source_offset++];
            unsigned int packet_count = (packet & 0x7f) + 1;
            unsigned int copy_count = packet_count;

            if ((packet & 0x80) != 0) {
                const unsigned char* pixel = file_data + source_offset;
                for (unsigned int index = 0; index < copy_count; ++index) {
                    unsigned int x = pixel_offset % width;
                    unsigned int y = pixel_offset / width;
                    if ((header->image_descriptor & 0x20) == 0) {
                        y = height - 1 - y;
                    }
                    if ((header->image_descriptor & 0x10) != 0) {
                        x = width - 1 - x;
                    }
                    memcpy(destination + y * surface->getPitch() + x * bytes_per_pixel, pixel,
                           bytes_per_pixel);
                    ++pixel_offset;
                }
                source_offset += bytes_per_pixel;
            } else {
                if (source_offset + packet_count * bytes_per_pixel > data_size) {
                    packet_count = (data_size - source_offset) / bytes_per_pixel;
                }
                copy_count = packet_count;
                for (unsigned int index = 0; index < copy_count; ++index) {
                    unsigned int x = pixel_offset % width;
                    unsigned int y = pixel_offset / width;
                    if ((header->image_descriptor & 0x20) == 0) {
                        y = height - 1 - y;
                    }
                    if ((header->image_descriptor & 0x10) != 0) {
                        x = width - 1 - x;
                    }
                    memcpy(destination + y * surface->getPitch() + x * bytes_per_pixel,
                           file_data + source_offset, bytes_per_pixel);
                    source_offset += bytes_per_pixel;
                    ++pixel_offset;
                }
            }
        }
        return;
    }

    for (unsigned int row = 0; row < height; ++row) {
        unsigned int destination_row = row;
        if ((header->image_descriptor & 0x20) == 0) {
            destination_row = height - 1 - row;
        }
        if ((header->image_descriptor & 0x10) == 0) {
            memcpy(destination + destination_row * surface->getPitch(), file_data + row * row_size,
                   row_size);
        } else {
            for (unsigned int column = 0; column < width; ++column) {
                memcpy(destination + destination_row * surface->getPitch() +
                           (width - 1 - column) * bytes_per_pixel,
                       file_data + row * row_size + column * bytes_per_pixel, bytes_per_pixel);
            }
        }
    }
}

// FUNCTION: WIZ8 0x0047C090
srColorSurface* LoadSurface0047C090(int handle)
{
    W8TgaHeader header;
    unsigned short width;
    unsigned short height;
    srARGB palette_colors[1024];
    unsigned int palette_count;
    srPalette* palette;
    srColorSurface* surface;

    FileRead(handle, &header.id_length, 1, 0);
    FileRead(handle, &header.color_map_type, 1, 0);
    FileRead(handle, &header.image_type, 1, 0);
    FileRead(handle, &header.color_map_origin, 2, 0);
    FileRead(handle, &header.color_map_length, 2, 0);
    FileRead(handle, &header.color_map_entry_size, 1, 0);
    FileRead(handle, &header.x_origin, 2, 0);
    FileRead(handle, &header.y_origin, 2, 0);
    FileRead(handle, &width, 2, 0);
    FileRead(handle, &height, 2, 0);
    FileRead(handle, &header.pixel_depth, 1, 0);
    FileRead(handle, &header.image_descriptor, 1, 0);
    header.width = width;
    header.height = height;
    FileSeek(handle, header.id_length, FILE_SEEK_FROM_CURRENT);

    if (header.color_map_type == 0) {
        palette_count = header.color_map_length;
    } else if (header.image_type == 0 || header.image_type == 1 ||
               (header.image_type == 9 && header.color_map_type == 1)) {
        palette_count = header.color_map_length;
        for (unsigned int index = 0; index < palette_count; ++index) {
            palette_colors[index].alpha = 0xff;
            FileRead(handle, &palette_colors[index].blue, 1, 0);
            FileRead(handle, &palette_colors[index].green, 1, 0);
            FileRead(handle, &palette_colors[index].red, 1, 0);
        }
    } else {
        FileSeek(handle, (header.color_map_entry_size >> 3) * header.color_map_length,
                 FILE_SEEK_FROM_CURRENT);
        palette_count = header.color_map_length;
    }

    switch (header.image_type) {
    case 0:
        if (header.color_map_type != 1) {
            return 0;
        }
        width = 1;
        height = 1;
        palette = SR_NEW(W8Palette)(palette_colors, header.color_map_length);
        palette->setName("TGA importer generated palette");
        palette->autoRelease();
        break;
    case 1:
    case 9:
        if (header.color_map_type == 1) {
            palette = SR_NEW(W8Palette)(palette_colors, header.color_map_length);
            palette->setName("TGA importer generated palette");
            palette->autoRelease();
        } else {
            palette = srCore.getPalette();
        }
        break;
    case 2:
    case 10:
        if (header.pixel_depth == 16) {
            surface = SR_NEW(W8ColorSurface)(static_cast<srPixelConvert::e_surfaceType>(
                                                 (header.image_descriptor & 0xf) == 0 ? 8 : 9),
                                             width, height);
        } else if (header.pixel_depth == 24) {
            surface = SR_NEW(W8ColorSurface)(srPixelConvert::SURFACE_BGR24, width, height);
        } else if (header.pixel_depth == 32) {
            surface = SR_NEW(W8ColorSurface)(static_cast<srPixelConvert::e_surfaceType>(
                                                 (header.image_descriptor & 0xf) == 0 ? 0xd : 0xe),
                                             width, height);
        } else {
            return 0;
        }
        break;
    case 3:
    case 11:
        surface = SR_NEW(W8ColorSurface)(srPixelConvert::SURFACE_L8, width, height);
        break;
    default:
        return 0;
    }

    if (header.image_type == 0 || header.image_type == 1 || header.image_type == 9) {
        surface =
            SR_NEW(W8ColorSurface)(static_cast<srPixelConvert::e_surfaceType>(4), width, height);
        if (surface != 0) {
            surface->setPalette(palette);
        }
    }

    if (surface != 0) {
        LoadSurfacePixels0047BC80(handle, surface, &header);
    }
    return surface;
}

// VTABLE: WIZ8 0x005EC5F8
// class stTextureFile

// VTABLE: WIZ8 0x005EC63C
// class srClassSupport<stTextureFile,srTexture,0,65537>

// TEMPLATE: WIZ8 0x0047D6D0
// srClassSupport<stTextureFile,srTexture,0,65537>::getClassID

// TEMPLATE: WIZ8 0x0047D6E0
// srClassSupport<stTextureFile,srTexture,0,65537>::getClassName

// TEMPLATE: WIZ8 0x0047D6F0
// srClassSupport<stTextureFile,srTexture,0,65537>::getClassNode

// TEMPLATE: WIZ8 0x0047D790
// srClassSupport<stTextureFile,srTexture,0,65537>::clone

// TEMPLATE: WIZ8 0x0047D870
// srClassSupport<stTextureFile,srTexture,0,65537>::~srClassSupport<stTextureFile,srTexture,0,65537>

// SYNTHETIC: WIZ8 0x0047D970
// srClassSupport<stTextureFile,srTexture,0,65537>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0047D9A0
// srClassSupport<stTextureFile,srTexture,0,65537>::sGetClassNode

// FUNCTION: WIZ8 0x0047C630
stTextureFile::stTextureFile(const char* file_name, int cached)
    : cached_54(cached), file_name_58(0), surface_5c(0), frame_handle_60(getNewFrameHandle()),
      has_alpha_64(0)
{
    invalidate();
    setFileName(file_name);
    if (file_name != 0) {
        setName(file_name);
    }
    if (cached_54 != 0 && file_name_58 != 0) {
        setupDefaultValues();
    }
}

stTextureFile& stTextureFile::operator=(const stTextureFile& other)
{
    if (this != &other) {
        srTexture::operator=(other);
        setFileName(other.file_name_58);
        cached_54 = other.cached_54;
    }
    return *this;
}

// FUNCTION: WIZ8 0x0047C5F0
unsigned long stTextureFile::getTextureFrameHandle()
{
    if ((texture_flags_ & LOAD_FAILED) != 0) {
        return 0;
    }
    return frame_handle_60;
}

// FUNCTION: WIZ8 0x0047C600
void stTextureFile::setupDefaultValues()
{
    if ((texture_flags_ & DEFAULTS_PENDING) == 0) {
        return;
    }

    texture_flags_ &= ~DEFAULTS_PENDING;
    if (surface_5c == 0) {
        loadSurface();
    }
    setupDefaultValuesFromSurface(surface_5c);
}

// FUNCTION: WIZ8 0x0047C7A0
srClass* stTextureFile::vInstance()
{
    return new stTextureFile(0, 0);
}

// SYNTHETIC: WIZ8 0x0047C800
// stTextureFile::`scalar deleting destructor'

// FUNCTION: WIZ8 0x0047C830
void stTextureFile::setFileName(const char* file_name)
{
    invalidate();
    delete[] file_name_58;
    file_name_58 = 0;

    if (file_name != 0 && file_name[0] != 0) {
        file_name_58 = new char[strlen(file_name) + 1];
        strcpy(file_name_58, file_name);
    }

    texture_flags_ &= ~LOAD_FAILED;
    texture_flags_ |= DEFAULTS_PENDING;
}

// FUNCTION: WIZ8 0x0047C8B0
void stTextureFile::invalidate()
{
    releaseSurface();
    texture_flags_ |= DEFAULTS_PENDING;
    invalidateFrameHandle(frame_handle_60);
    texture_flags_ &= ~LOAD_FAILED;
}

// FUNCTION: WIZ8 0x0047C8E0
stTextureFile::~stTextureFile()
{
    if (IsTextureInReadMeshScratch(this) != 0) {
        ReleaseReadMeshScratch004881D0();
    }
    invalidate();
    delete[] file_name_58;
    file_name_58 = 0;
    texture_flags_ &= ~LOAD_FAILED;
    texture_flags_ |= DEFAULTS_PENDING;
}

// FUNCTION: WIZ8 0x0047BBD0
void stTextureFile::releaseSurface()
{
    if (surface_5c != 0) {
        surface_5c->release();
        surface_5c = 0;
    }
    texture_flags_ |= DEFAULTS_PENDING;
}

// FUNCTION: WIZ8 0x0047BBF0
void stTextureFile::loadSurface()
{
    if (surface_5c != 0) {
        invalidate();
    }
    if (file_name_58 == 0) {
        return;
    }

    int handle = FileOpen(file_name_58, 0x41, 0);
    if (handle != 0) {
        surface_5c = LoadSurface0047C090(handle);
        FileClose(handle);
    }

    if (surface_5c == 0) {
        texture_flags_ |= LOAD_FAILED;
        return;
    }

    setupDefaultValues();
    surface_5c->setFilter(getFilter());
}

// FUNCTION: WIZ8 0x0047CA50
void stTextureFile::getMipmapData(MultiRequest& request)
{
    if (surface_5c == 0) {
        loadSurface();
    }
    if (surface_5c == 0) {
        return;
    }

    long level = request.mipmap_level;
    if (request.destinations[level] != 0) {
        request.destinations[level]->copy(*surface_5c);
    }
    for (++level; level <= static_cast<long>(request.unknown_04); ++level) {
        if (request.destinations[level] != 0 && request.destinations[level - 1] != 0) {
            request.destinations[level]->copy(*request.destinations[level - 1]);
        }
    }

    if (cached_54 == 0) {
        releaseSurface();
        texture_flags_ |= DEFAULTS_PENDING;
    }
}

void stTextureFile::getMipmapLevelPartial(PartialRequest&) {}

void stTextureFile::dump(std::ostream&) {}
