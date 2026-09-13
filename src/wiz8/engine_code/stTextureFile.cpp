#include "wiz8/engine_code/ReadMesh.h"
#include "wiz8/engine_code/stTextureFile.h"

#include "wiz8/engine_code/ReadLevel.h"
#include "FileMan.h"
#include "surrender/srPalette.h"
#include "surrender/srTypeRegistry.h"
#include "wiz8/virtual_file.h"

#include <string.h>

// FUNCTION: WIZ8 0x0047C090
srColorSurface* stTextureFile::LoadSurface0047C090(int handle, int* image_type)
{
    unsigned char id_length;
    unsigned char color_map_type;
    unsigned char type;
    unsigned short color_map_origin;
    unsigned short color_map_length;
    unsigned char color_map_depth;
    unsigned short origin_x;
    unsigned short origin_y;
    unsigned short width;
    unsigned short height;
    unsigned char pixel_depth;
    unsigned char descriptor;

    FileRead(handle, &id_length, 1, 0);
    FileRead(handle, &color_map_type, 1, 0);
    FileRead(handle, &type, 1, 0);
    FileRead(handle, &color_map_origin, 2, 0);
    FileRead(handle, &color_map_length, 2, 0);
    FileRead(handle, &color_map_depth, 1, 0);
    FileRead(handle, &origin_x, 2, 0);
    FileRead(handle, &origin_y, 2, 0);
    FileRead(handle, &width, 2, 0);
    FileRead(handle, &height, 2, 0);
    FileRead(handle, &pixel_depth, 1, 0);
    FileRead(handle, &descriptor, 1, 0);
    (void)color_map_origin;
    (void)color_map_type;
    (void)origin_x;
    (void)origin_y;

    FileSeek(handle, id_length, FILE_SEEK_FROM_CURRENT);
    if (image_type != 0) {
        *image_type = type;
    }

    srPalette* palette = 0;
    srARGB colors[1024];
    if ((type == 1 || type == 9) && color_map_length != 0) {
        if (color_map_depth != 24 && color_map_depth != 32) {
            return 0;
        }
        for (unsigned short index = 0; index < color_map_length; ++index) {
            colors[index].alpha = 0xff;
            FileRead(handle, &colors[index].blue, 1, 0);
            FileRead(handle, &colors[index].green, 1, 0);
            FileRead(handle, &colors[index].red, 1, 0);
            if (color_map_depth == 32) {
                FileRead(handle, &colors[index].alpha, 1, 0);
            }
        }
        palette = SR_NEW(srPalette)(colors, color_map_length);
        if (palette != 0) {
            palette->setName("TGA importer generated palette");
            palette->autoRelease();
        }
    } else if (type == 2 || type == 10) {
        palette = srCore.getPalette();
    }

    srPixelConvert::e_surfaceType surface_type;
    switch (type) {
    case 1:
    case 9:
        surface_type = srPixelConvert::SURFACE_L8;
        break;
    case 2:
    case 10:
        if (pixel_depth == 16) {
            surface_type = srPixelConvert::SURFACE_ARGB1555;
        } else if (pixel_depth == 24) {
            surface_type = srPixelConvert::SURFACE_BGR24;
        } else if (pixel_depth == 32) {
            surface_type = srPixelConvert::SURFACE_BGRA32;
        } else {
            return 0;
        }
        break;
    case 3:
    case 11:
        surface_type = srPixelConvert::SURFACE_L8;
        break;
    default:
        return 0;
    }

    srColorSurface* surface = SR_NEW(W8ColorSurface)(surface_type, width, height);
    if (surface == 0) {
        return 0;
    }
    if (palette != 0) {
        surface->setPalette(palette);
    }
    const long data_size = surface->getDataSize();
    if (data_size > 0) {
        FileRead(handle, surface->getDataPtr(), data_size, 0);
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

void stTextureFile::releaseSurface()
{
    if (surface_5c != 0) {
        surface_5c->release();
        surface_5c = 0;
    }
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

// FUNCTION: WIZ8 0x0047BBF0
void stTextureFile::loadSurface()
{
    if (surface_5c != 0) {
        invalidate();
    }
    if (file_name_58 == 0) {
        return;
    }

    int image_type = 0;
    int handle = FileOpen(file_name_58, 0x41, 0);
    if (handle != 0) {
        surface_5c = LoadSurface0047C090(handle, &image_type);
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
