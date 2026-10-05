#include "wiz8/engine_code/ReadMesh.h"
#include "wiz8/engine_code/stTextureFile.h"

#include "wiz8/engine_code/ReadLevel.h"
#include "FileMan.h"
#include "surrender/srCore.h"
#include "surrender/srHeap.h"
#include "surrender/srArray.h"
#include "surrender/srPalette.h"
#include "surrender/srTypeRegistry.h"
#include "wiz8/virtual_file.h"

#include <string.h>

/* Both TGA scratch buffers are srHeapBuffer objects: growth goes through the
   preserving two-argument setCapacity (retail copies the old contents before
   freeing) and the zero-size branch calls the emitted release() at
   0x004741B0. The second pair backs one decoded row / RLE packet. */
// GLOBAL: WIZ8 0x0065A138
static srHeapBuffer<unsigned char> g_tga_file_data;
// GLOBAL: WIZ8 0x0065A130
static srHeapBuffer<unsigned char> g_tga_row_data;

/* Decodes TGA pixel data into the surface. Destination writes are strided:
   the pixel step is the surface's bytes-per-pixel (negated when the
   descriptor's right-origin bit is set) and the row step only exists for
   bottom-up images. For RLE data, retail reloads the scratch source pointer
   at each row start and carries only the remaining packet count across a row
   boundary. */
// FUNCTION: WIZ8 0x0047BC80
void __stdcall LoadSurfacePixels(int handle, srColorSurface* surface, const W8TgaHeader* header)
{
    unsigned char* destination = static_cast<unsigned char*>(surface->getDataPtr());
    if (destination == 0) {
        return;
    }

    int rle = header->image_type == 9 || header->image_type == 10 || header->image_type == 11;
    long pixel_step = surface->pixel_format.bytes_per_pixel_minus_one + 1;
    long file_bpp = header->pixel_depth >> 3;
    if (file_bpp <= 0 || file_bpp > 4) {
        return;
    }

    long row_step = 0;
    if ((header->image_descriptor & 0x20) == 0) {
        row_step = header->width * pixel_step * -2;
        destination += (header->height - 1) * header->width * pixel_step;
    }
    if ((header->image_descriptor & 0x10) != 0) {
        destination += (header->width - 1) * pixel_step;
        pixel_step = -pixel_step;
    }

    unsigned int count = header->width;
    unsigned int carry = 0;
    unsigned int scratch_size = rle ? file_bpp << 7 : header->width * file_bpp;

    /* data_size is FileGetSize - FileGetPos, so it can be negative, and
       capacity is an unsigned long. The retail compares the two as signed
       (0x0047BD87 cmp eax,esi; jge) but then tests data_size > 0 unsigned
       (0x0047BD8B test esi,esi; jbe), so the two uses differ in class and the
       signed one carries a cast. Casting only data_size would leave the
       comparison unsigned, because capacity converts to unsigned to meet it. */
    unsigned int data_size = FileGetSize(handle) - FileGetPos(handle);
    if (static_cast<int>(g_tga_file_data.capacity) < static_cast<int>(data_size)) {
        g_tga_file_data.setCapacity(data_size, 1);
    }
    unsigned char* file_data = g_tga_file_data.data;
    if (!FileRead(handle, file_data, data_size, 0)) {
        return;
    }

    if (g_tga_row_data.capacity < scratch_size) {
        g_tga_row_data.setCapacity(scratch_size, 1);
    }
    unsigned char* scratch = g_tga_row_data.data;

    unsigned int file_offset = 0;
    unsigned int step = file_bpp;
    for (long row = 0; row < header->height; ++row) {
        if (!rle) {
            unsigned int row_bytes = header->width * file_bpp;
            memcpy(scratch, file_data + file_offset, row_bytes);
            file_offset += header->width * file_bpp;
        }

        long column = 0;
        unsigned char* source = scratch;
        while (column < header->width) {
            if (carry == 0) {
                if (rle) {
                    unsigned char packet = file_data[file_offset];
                    step = ((packet & 0x80) != 0) ? 0 : file_bpp;
                    count = (packet & 0x7f) + 1;
                    unsigned int copy_size = file_bpp;
                    if ((packet & 0x80) == 0) {
                        copy_size = count * file_bpp;
                    }
                    memcpy(scratch, file_data + file_offset + 1, copy_size);
                    file_offset += 1 + copy_size;
                    source = scratch;
                }
            } else {
                count = carry;
            }

            if (rle) {
                carry = column - header->width + count;
                if (static_cast<int>(carry) < 0) {
                    carry = 0;
                }
                count -= carry;
            }

            for (unsigned int index = count; index > 0; --index) {
                memcpy(destination, source, file_bpp);
                destination += pixel_step;
                source += step;
            }
            if (carry == 0 && step == 0) {
                source += file_bpp;
            }
            column += count;
        }
        destination += row_step;
    }
}

// FUNCTION: WIZ8 0x0047C090
srColorSurface* __stdcall LoadSurface(int handle, long* unused_out)
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
        palette = srPalette::findMatchingPalette(palette_colors, header.color_map_length);
        if (palette == 0) {
            palette = SR_NEW(W8Palette)(palette_colors, header.color_map_length);
            palette->autoRelease();
            palette->setName("TGA-importer generated palette");
        }
        break;
    case 1:
    case 9:
        if (header.color_map_type == 1) {
            palette = srPalette::findMatchingPalette(palette_colors, header.color_map_length);
            if (palette == 0) {
                palette = SR_NEW(W8Palette)(palette_colors, header.color_map_length);
                palette->autoRelease();
                palette->setName("TGA-importer generated palette");
            }
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
        LoadSurfacePixels(handle, surface, &header);
    }
    return surface;
}

// VTABLE: WIZ8 0x005EC5F8
// class stTextureFile

// VTABLE: WIZ8 0x005EC63C
// class srClassSupport<stTextureFile,srTexture,0,65537>

/* The TGA loader instantiates srClassSupport for the imported srPalette
   (class id 0x2900); its registry and clone slots are emitted in this TU. */
// VTABLE: WIZ8 0x005EC5D8
// class srClientSupport<srPalette,10496>

// FUNCTION: WIZ8 0x0047BBD0
void stTextureFile::releaseSurface()
{
    if (surface != 0) {
        surface->release();
        surface = 0;
    }
    texture_flags_ |= DEFAULTS_PENDING;
}

// FUNCTION: WIZ8 0x0047C630
stTextureFile::stTextureFile(const char* file_name, int cached)
    : cached(0), file_name(0), surface(0), frame_handle(getNewFrameHandle()),
      has_alpha(0)
{
    /* Retail stores 0 then conditionally stores 1: the authored value is the
       normalized predicate, not the raw parameter. */
    this->cached = (cached != 0);
    invalidate();
    setFileName(file_name);
    if (file_name != 0) {
        setName(file_name);
    }
    if (cached != 0 && file_name != 0) {
        setupDefaultValues();
    }
}

stTextureFile& stTextureFile::operator=(const stTextureFile& other)
{
    if (this != &other) {
        srTexture::operator=(other);
        setFileName(other.file_name);
        cached = other.cached;
    }
    return *this;
}

// FUNCTION: WIZ8 0x0047C5F0
unsigned long stTextureFile::getTextureFrameHandle()
{
    if ((texture_flags_ & LOAD_FAILED) != 0) {
        return 0;
    }
    return frame_handle;
}

// FUNCTION: WIZ8 0x0047C600
void stTextureFile::setupDefaultValues()
{
    if ((texture_flags_ & DEFAULTS_PENDING) == 0) {
        return;
    }

    texture_flags_ &= ~DEFAULTS_PENDING;
    if (surface == 0) {
        loadSurface();
    }
    setupDefaultValuesFromSurface(surface);
}

// FUNCTION: WIZ8 0x0047C7A0
srClass* stTextureFile::vInstance()
{
    return new stTextureFile(0, 0);
}

// FUNCTION: WIZ8 0x0047C830
void stTextureFile::setFileName(const char* file_name)
{
    invalidate();
    delete[] this->file_name;
    this->file_name = 0;

    if (file_name != 0 && file_name[0] != 0) {
        this->file_name = new char[strlen(file_name) + 1];
        strcpy(this->file_name, file_name);
    }

    texture_flags_ &= ~LOAD_FAILED;
    texture_flags_ |= DEFAULTS_PENDING;
}

// FUNCTION: WIZ8 0x0047C8B0
void stTextureFile::invalidate()
{
    releaseSurface();
    invalidateFrameHandle(frame_handle);
    texture_flags_ &= ~LOAD_FAILED;
}

/* Retail runs the invalidate sequence twice: this call is expanded inline,
   and the setFileName(0) invalidation stays a virtual dispatch. */
// FUNCTION: WIZ8 0x0047C8E0
stTextureFile::~stTextureFile()
{
    if (IsTextureInReadMeshScratch(this) != 0) {
        ReleaseReadMeshScratch();
    }
    invalidate();
    setFileName(0);
}

// FUNCTION: WIZ8 0x0047BBF0
void stTextureFile::loadSurface()
{
    /* The second LoadSurface argument is an out-pointer the callee ignores;
       the caller still initializes the dword it passes. */
    long unused = 0;

    if (surface != 0) {
        invalidate();
    }
    /* A missing file name lands on the same LOAD_FAILED tail as a failed
       load; retail has no silent early return here. */
    if (file_name == 0) {
        texture_flags_ |= LOAD_FAILED;
        return;
    }

    surface = 0;
    int handle = FileOpen(file_name, 0x41, 0);
    if (handle != 0) {
        surface = LoadSurface(handle, &unused);
        FileClose(handle);
    }

    if (surface == 0) {
        texture_flags_ |= LOAD_FAILED;
        return;
    }

    setupDefaultValues();
    surface->setFilter(getFilter());
    /* Retail reads the alpha channel count straight out of the surface's
       pixel format (unsigned SETA): the authored comparison is `> 0`. */
    has_alpha = (surface->pixel_format.alpha_bits > 0);
}

// FUNCTION: WIZ8 0x0047CA50
void stTextureFile::getMipmapData(MultiRequest& request)
{
    if (surface == 0) {
        loadSurface();
    }
    if (surface == 0) {
        return;
    }

    /* 0x0047CA85 and 0x0047CAAB compare the level against last_level
       unsigned, and last_level is already unsigned in the request record. */
    unsigned long level = static_cast<unsigned long>(request.mipmap_level);
    if (request.destinations[level] != 0) {
        request.destinations[level]->copy(*surface);
    }
    for (++level; level <= request.last_level; ++level) {
        if (request.destinations[level] != 0 && request.destinations[level - 1] != 0) {
            request.destinations[level]->copy(*request.destinations[level - 1]);
        }
    }

    /* Retail tests the flag with TEST byte ptr [+0x54],0x1 even though the
       constructor stores the field dword-wide: the authored predicate is a
       bit test, not a zero compare. */
    if ((cached & 1) == 0) {
        releaseSurface();
    }
}

void stTextureFile::getMipmapLevelPartial(PartialRequest&) {}

void stTextureFile::dump(std::ostream&) {}
