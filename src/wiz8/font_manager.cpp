#include "wiz8/font_manager.h"

#include "wiz8/render_state.h"
#include "wiz8/virtual_file.h"
#include "surrender/srColorSurface.h"
#include "wiz8/wiz8_windows.h"
#include "vsurface.h"
#include "vobject_blitters.h"
#include "DirectDraw Calls.h"
#include "FileMan.h"
#include "Font.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

void* LockPrimarySurface(unsigned int* pitch);
void UnlockPrimarySurface(void);
void NoOp(void);
unsigned char Function414700(void* record, unsigned int mode);
unsigned char Function414C60(void* record, unsigned int mode);

/* A video-surface list node as 0x00402B90 walks it. */
struct W8VideoSurfaceNode {
    HVSURFACE surface_00;
    unsigned int index_04;
    W8VideoSurfaceNode* next_08;
};

extern "C" {
extern W8VideoSurfaceNode* gpVSurfaceHead;
extern HVSURFACE ghMouseBuffer;
extern unsigned char gbPixelDepth;
/* Font.c keeps the table private to its TU; this matches its definition. */
extern HVOBJECT FontObjs[25];
}

void* Function402B90(int target, unsigned int* pitch);
void Function402C30(int target);

// GLOBAL: WIZ8 0x00600078
RECT g_clip_rect_600078;
/* Mono-print colors sampled by mprintf for its shadow blitters. */
// GLOBAL: WIZ8 0x00650E3E
unsigned char g_mono_fg_650e3e;
// GLOBAL: WIZ8 0x00650E3F
unsigned char g_mono_bg_650e3f;
// GLOBAL: WIZ8 0x00650E3A
unsigned short g_mono_fg16_650e3a;
// GLOBAL: WIZ8 0x00650E3C
unsigned short g_mono_bg16_650e3c;
// GLOBAL: WIZ8 0x005FF610
unsigned short g_mono_shadow_5ff610;
/* Retail 0x006BDAA0/0x006BDEA0: zero-initialized palettes the font parser
   publishes on objects whose file carries an 8-bpp marker. */
unsigned short g_font_palette_a_6bdaa0[256];
unsigned short g_font_palette_b_6bdea0[256];

// GLOBAL: WIZ8 0x006000B4
const unsigned int g_stci_magic_6000b4 = 0x49435453;

/* The file request 0x00406180 parses: a flag word selecting file-backed
   loading followed by the path. */
struct W8FontLoadRequest {
    int flags_00;
    char path_04[104];
};

/* The 0x90-byte image record the loaders fill. Only the fields the STI
   path touches are named. */
struct W8ImageRecord {
    unsigned char unknown_00[6];
    unsigned char flags_06;
    unsigned char unknown_07;
    char path_08[0x64];
    unsigned int kind_6c;
    void* ptr_70;
    void* ptr_74;
    void* ptr_78;
    unsigned int size_7c;
    void* ptr_80;
    unsigned int size_84;
    void* ptr_88;
    unsigned short count_8c;
    unsigned short unknown_8e;
};

/* The 0x40-byte STI header the loaders parse. Only the fields they touch
   are named. 0x004153F0 counts glyphs from the low word at 0x1c and
   0x00415250 compares the channel masks at 0x18/0x1c/0x20, so the words at
   0x1a and 0x22 stay unnamed. */
struct W8StiHeader {
    unsigned int magic_00;
    unsigned char unknown_04[4];
    unsigned int size_08;
    unsigned char unknown_0c[4];
    unsigned char flags_10;
    unsigned char unknown_11[3];
    unsigned short field_14;
    unsigned short field_16;
    unsigned short format_18;
    unsigned short unknown_1a;
    unsigned int format_1c;
    unsigned short format_20;
    unsigned char unknown_22[10];
    unsigned char type_2c;
    unsigned char unknown_2d[3];
    unsigned int size_30;
    unsigned char unknown_34[8];
    unsigned char field_3c;
    unsigned char unknown_3d[3];
};

/* The font table entries are the 0xFC-byte objects 0x00406180 builds. Their
   metrics array at +0x18 is an ETRLEObject array (stride 0x10), which is why
   the print path can hand these objects straight to the SGP blitters. */
void* Function406180(W8FontLoadRequest* request);
void* Function40F850(char* path, unsigned int mode);
unsigned char Function415130(void* record, unsigned int mode);
unsigned char Function415250(
    W8ImageRecord* record, unsigned int mode, int file, W8StiHeader* header);
unsigned char Function4153F0(
    W8ImageRecord* record, unsigned int mode, int file, W8StiHeader* header);
unsigned char Function410580(void* image, void* metrics);
unsigned short* Function410190(int table);
unsigned char Function40F9F0(void* resource);
unsigned char Function40FA10(void* resource, unsigned char mask);
void Function410620(unsigned short* pixels, int count);
void Function410670(unsigned short* pixels, int count);
void Function4106C0(unsigned short* pixels, int count);
void Function410700(unsigned short* pixels, int count);
unsigned int Function4104B0(int value);
extern unsigned short g_alpha_mask_650f48;
extern unsigned short g_red_mask_650f4a;
extern unsigned short g_green_mask_650f4c;
extern unsigned short g_blue_mask_650f4e;
extern short g_red_shift_650f50;
extern short g_blue_shift_650f52;
extern short g_green_shift_650f54;

// FUNCTION: WIZ8 0x00406dc0
unsigned short* SetFontObjectPalette16BPP(int font, unsigned short* palette)
{
    void* object = FontObjs[font];
    *(unsigned short**)((char*)object + 0x10) = palette;
    *(unsigned short**)((char*)object + 0xe0) = palette;
    return palette;
}

// FUNCTION: WIZ8 0x00406e00
int LoadFontFile(unsigned char* path)
{
    W8FontLoadRequest request;
    int index;
    void* font;

    for (index = 0; index < 25; ++index) {
        if (FontObjs[index] == 0) {
            break;
        }
    }
    if (index == 25) {
        return -1;
    }
    request.flags_00 = 0x40;
    strcpy(request.path_04, (char*)path);
    font = Function406180(&request);
    FontObjs[index] = (HVOBJECT)font;
    if (font == 0) {
        return -1;
    }
    if (FontDefault == -1) {
        FontDefault = index;
    }
    return index;
}

// FUNCTION: WIZ8 0x00407260
unsigned int mprintf(int x, int y, unsigned short* format, ...)
{
    va_list args;
    unsigned short buffer[512];
    unsigned short* text;
    void* surface;
    unsigned int pitch;
    int cur_x;

    va_start(args, format);
    vswprintf((wchar_t*)buffer, (wchar_t*)format, args);
    va_end(args);
    text = buffer;
    surface = Function402B90(FontDestBuffer, &pitch);
    cur_x = x;
    for (;;) {
        unsigned short ch;
        unsigned int index = 0;
        if (buffer[0] == 0) {
            Function402C30(FontDestBuffer);
            return 0;
        }
        ch = *text++;
        /* Retail inlines this translation-table walk; the oracle factors it
           as GetIndex, and pFManager's record stays private to Font.c, so
           the printers call in with identical results. */
        index = (unsigned int)GetIndex(ch);
        if (FontDestWrap != 0 &&
            BltIsClipped(FontObjs[FontDefault], cur_x, y, (UINT16)index,
                         &FontDestRegion) != 0) {
            ETRLEObject* glyph = &FontObjs[FontDefault]->pETRLEObject[index];
            y = glyph->sOffsetY + y + glyph->usHeight;
            cur_x = x;
        }
        if (gbPixelDepth == 8) {
            Blt8BPPDataTo8BPPBufferMonoShadowClip(
                (unsigned char*)surface, pitch,
                FontObjs[FontDefault],
                cur_x, y, index, &FontDestRegion,
                g_mono_fg_650e3e, g_mono_bg_650e3f);
        }
        else {
            Blt8BPPDataTo16BPPBufferMonoShadowClip(
                (unsigned short*)surface, pitch,
                FontObjs[FontDefault],
                cur_x, y, index, &FontDestRegion,
                g_mono_fg16_650e3a, g_mono_bg16_650e3c, g_mono_shadow_5ff610);
        }
        {
            ETRLEObject* glyph = &FontObjs[FontDefault]->pETRLEObject[index];
            buffer[0] = *text;
            cur_x = glyph->sOffsetX + cur_x + glyph->usWidth;
        }
    }
}



// FUNCTION: WIZ8 0x00402c30
void Function402C30(int target)
{
    if (target == -14) {
        UnlockPrimarySurface();
        return;
    }
    if (target == -13) {
        NoOp();
        return;
    }
    W8VideoSurfaceNode* node = gpVSurfaceHead;
    if (node == 0) {
        return;
    }
    while (node->index_04 != (unsigned int)target) {
        node = node->next_08;
        if (node == 0) {
            return;
        }
    }
    if (node == 0) {
        return;
    }
    HVSURFACE surface = node->surface_00;
    if (surface == ghMouseBuffer) {
        UnlockPrimarySurface();
        return;
    }
    DDUnlockSurface(*(IDirectDrawSurface2**)((char*)surface + 8), 0);
    if ((*(int*)((char*)surface + 0x18) & 2) == 0) {
        return;
    }
    /* The composite-to-primary tail below is unreachable in retail: the
       flag test above always falls through to the return. It is not
       reproduced. */
}

/* Locks a print destination: the primary surface, the mouse surface, or a
   video surface from the manager list. */
// FUNCTION: WIZ8 0x00402b90
void* Function402B90(int target, unsigned int* pitch)
{
    if (target == -14) {
        return LockPrimarySurface(pitch);
    }
    if (target == -13) {
        srColorSurface* surface = g_mouse_surface_659688;
        *pitch = *(unsigned int*)((char*)surface + 0x24);
        return surface->getDataPtr();
    }
    W8VideoSurfaceNode* node = gpVSurfaceHead;
    if (node != 0) {
        while (node->index_04 != (unsigned int)target) {
            node = node->next_08;
            if (node == 0) {
                return 0;
            }
        }
        if (node != 0) {
            if (node->surface_00 == ghMouseBuffer) {
                return LockPrimarySurface(pitch);
            }
            DDSURFACEDESC description;
            DDLockSurface(*(IDirectDrawSurface2**)((char*)node->surface_00 + 8),
                          0, &description, 0, 0);
            *pitch = description.lPitch;
            return description.lpSurface;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00407650
unsigned int gprintf(int x, int y, unsigned short* format, ...)
{
    va_list args;
    unsigned short buffer[512];
    unsigned short* text;
    void* surface;
    unsigned int pitch;
    int cur_x;

    va_start(args, format);
    vswprintf((wchar_t*)buffer, (wchar_t*)format, args);
    va_end(args);
    text = buffer;
    surface = Function402B90(FontDestBuffer, &pitch);
    cur_x = x;
    for (;;) {
        unsigned short ch;
        unsigned int index = 0;
        if (buffer[0] == 0) {
            Function402C30(FontDestBuffer);
            return 0;
        }
        ch = *text++;
        /* Retail inlines this translation-table walk; the oracle factors it
           as GetIndex, and pFManager's record stays private to Font.c, so
           the printers call in with identical results. */
        index = (unsigned int)GetIndex(ch);
        if (FontDestWrap != 0 &&
            BltIsClipped(FontObjs[FontDefault], cur_x, y, (UINT16)index,
                         &FontDestRegion) != 0) {
            ETRLEObject* glyph = &FontObjs[FontDefault]->pETRLEObject[index];
            y = glyph->sOffsetY + y + glyph->usHeight;
            cur_x = x;
        }
        if (gbPixelDepth == 8) {
            Blt8BPPDataTo8BPPBufferTransparentClip(
                (unsigned short*)surface, pitch,
                FontObjs[FontDefault],
                cur_x, y, index, &FontDestRegion);
        }
        else {
            Blt8BPPDataTo16BPPBufferTransparentClip(
                (unsigned short*)surface, pitch,
                FontObjs[FontDefault],
                cur_x, y, index, &FontDestRegion);
        }
        {
            ETRLEObject* glyph = &FontObjs[FontDefault]->pETRLEObject[index];
            buffer[0] = *text;
            cur_x = glyph->sOffsetX + cur_x + glyph->usWidth;
        }
    }
}

// FUNCTION: WIZ8 0x00415250
unsigned char Function415250(
    W8ImageRecord* record, unsigned int mode, int file, W8StiHeader* header)
{
    void* buffer;
    unsigned char ok;
    unsigned int done;

    if ((((mode & 4) == 0) || ((mode & 0xc) != 0)) && ((mode & 8) != 0)) {
        buffer = malloc(header->size_08);
        record->ptr_80 = buffer;
        if (buffer != 0) {
            ok = ReadVirtualFile(file, buffer, header->size_08, &done);
            if (ok != 0 && done == header->size_08) {
                record->flags_06 |= 8;
                if (header->type_2c == 0x10) {
                    if ((g_red_mask_650f4a != header->format_18) ||
                        (g_green_mask_650f4c != (unsigned short)header->format_1c) ||
                        (g_blue_mask_650f4e != header->format_20)) {
                        if (g_green_mask_650f4c < g_red_mask_650f4a &&
                            g_blue_mask_650f4e < g_green_mask_650f4c) {
                            if (g_red_mask_650f4a == 0x7c00) {
                                if (g_green_mask_650f4c == 0x3e0 &&
                                    g_blue_mask_650f4e == 0x1f) {
                                    Function410620(
                                        (unsigned short*)buffer,
                                        (unsigned int)header->field_16 *
                                        (unsigned int)header->field_14);
                                    return 1;
                                }
                            }
                            else if (g_red_mask_650f4a == 0xfc00) {
                                if (g_green_mask_650f4c == 0x3e0 &&
                                    g_blue_mask_650f4e == 0x1f) {
                                    Function410670(
                                        (unsigned short*)buffer,
                                        (unsigned int)header->field_16 *
                                        (unsigned int)header->field_14);
                                    return 1;
                                }
                            }
                            else if (g_red_mask_650f4a == 0xf800 &&
                                     g_green_mask_650f4c == 0x7c0 &&
                                     g_blue_mask_650f4e == 0x3f) {
                                Function4106C0(
                                    (unsigned short*)buffer,
                                    (unsigned int)header->field_16 *
                                    (unsigned int)header->field_14);
                                return 1;
                            }
                        }
                        Function410700(
                            (unsigned short*)buffer,
                            (unsigned int)header->field_16 *
                            (unsigned int)header->field_14);
                        return 1;
                    }
                }
            }
            else {
                free(buffer);
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004153f0
unsigned char Function4153F0(
    W8ImageRecord* record, unsigned int mode, int file, W8StiHeader* header)
{
    int file_handle = file;
    unsigned int flag4 = mode & 4;
    unsigned int done;
    unsigned char ok;
    int index;

    if ((mode & 4) == 0) {
        if (((mode & 0x18) != 0) &&
            FileSeek(file, *(int*)((char*)header + 0x18) * 3, 4) == 0) {
            CloseVirtualFile(file_handle);
            return 0;
        }
    }
    else {
        if (*(int*)((char*)header + 0x18) != 0x100) {
            return 0;
        }
        {
            unsigned int* pal_buffer = (unsigned int*)malloc(0x300);
            unsigned int* pal_cursor;
            if (pal_buffer == 0) {
                CloseVirtualFile(file_handle);
                return 0;
            }
            pal_cursor = pal_buffer;
            for (index = 0xc0; index != 0; --index) {
                *pal_cursor++ = 0;
            }
            ok = ReadVirtualFile(file_handle, pal_buffer, 0x300, &done);
            if (ok != 0 && done == 0x300) {
                unsigned int* palette = (unsigned int*)malloc(0x400);
                record->ptr_70 = palette;
                for (index = 0x100; index != 0; --index) {
                    *palette++ = 0;
                }
                if (record->ptr_70 != 0) {
                    int offset = 0;
                    int remaining_entries = 0x100;
                    unsigned char* triplet =
                        (unsigned char*)pal_buffer + 2;
                    do {
                        ((unsigned char*)record->ptr_70)[offset] =
                            triplet[-2];
                        ((unsigned char*)record->ptr_70)[offset + 1] =
                            triplet[-1];
                        ((unsigned char*)record->ptr_70)[offset + 2] =
                            triplet[0];
                        ((unsigned char*)record->ptr_70)[offset + 3] = 0;
                        offset += 4;
                        --remaining_entries;
                        triplet += 3;
                    } while (remaining_entries != 0);
                    record->flags_06 |= 4;
                    free(pal_buffer);
                }
                else {
                    CloseVirtualFile(file_handle);
                    free(pal_buffer);
                    return 0;
                }
            }
            else {
                CloseVirtualFile(file_handle);
                free(pal_buffer);
                return 0;
            }
        }
    }
    if ((mode & 8) == 0) {
        if (((mode & 0x10) != 0) &&
            FileSeek(file, *(int*)((char*)header + 8), 4) == 0) {
            CloseVirtualFile(file_handle);
            return 0;
        }
    }
    else {
        if ((header->flags_10 & 0x20) != 0) {
            unsigned short count = (unsigned short)header->format_1c;
            unsigned int table_size;
            void* pixels;
            record->count_8c = count;
            table_size = (unsigned int)count * 0x10;
            pixels = malloc(table_size);
            record->ptr_88 = pixels;
            if (pixels == 0) {
                CloseVirtualFile(file_handle);
                if (flag4 == 0) {
                    return 0;
                }
                free(record->ptr_70);
                return 0;
            }
            ok = ReadVirtualFile(file_handle, pixels, table_size, &done);
            if (ok == 0 || done != table_size) {
                CloseVirtualFile(file_handle);
                if (flag4 != 0) {
                    free(record->ptr_70);
                }
                free(record->ptr_88);
                return 0;
            }
            record->flags_06 |= 2;
            record->size_84 = header->size_08;
        }
        {
            void* pixels = malloc(header->size_08);
            record->ptr_80 = pixels;
            if (pixels == 0) {
                CloseVirtualFile(file_handle);
                if (flag4 != 0) {
                    free(record->ptr_70);
                }
                if (record->count_8c == 0) {
                    return 0;
                }
                free(record->ptr_88);
                return 0;
            }
            ok = ReadVirtualFile(file_handle, pixels, header->size_08,
                                 &done);
            if (ok == 0 || done != header->size_08) {
                CloseVirtualFile(file_handle);
                free(record->ptr_80);
                if (flag4 != 0) {
                    free(record->ptr_70);
                }
                if (record->count_8c == 0) {
                    return 0;
                }
                free(record->ptr_88);
                return 0;
            }
            record->flags_06 |= 8;
        }
    }
    if (((mode & 0x10) == 0) || (header->size_30 == 0)) {
        record->ptr_78 = 0;
        record->size_7c = 0;
        return 1;
    }
    {
        void* extra = malloc(header->size_30);
        record->ptr_78 = extra;
        if (extra == 0) {
            CloseVirtualFile(file_handle);
            free(record->ptr_78);
            if (flag4 != 0) {
                free(record->ptr_70);
            }
            if ((mode & 8) != 0) {
                free(record->ptr_80);
            }
            if (record->count_8c != 0) {
                free(record->ptr_88);
            }
            return 0;
        }
        ok = ReadVirtualFile(file_handle, extra, header->size_30, &done);
        if (ok != 0 && done == header->size_30) {
            record->flags_06 |= 0x10;
            record->size_7c = header->size_30;
            return 1;
        }
        CloseVirtualFile(file_handle);
        free(record->ptr_78);
        if (flag4 != 0) {
            free(record->ptr_70);
        }
        if ((mode & 8) != 0) {
            free(record->ptr_80);
        }
        if (record->count_8c != 0) {
            free(record->ptr_88);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x0040f850
void* Function40F850(char* path, unsigned int mode)
{
    char ext[8];
    int kind = 0x200;
    void* record;
    unsigned int* cursor;
    int index;
    unsigned char ok;

    if (strstr(path, ".") == 0) {
        strcat(path, ".PCX");
        strcpy(ext, ".PCX");
    }
    else {
        strcpy(ext, strstr(path, ".") + 1);
    }
    if (_stricmp(ext, "PCX") == 0) {
        kind = 1;
    }
    else if (_stricmp(ext, "TGA") == 0) {
        kind = 2;
    }
    else if (_stricmp(ext, "STI") == 0) {
        kind = 4;
    }
    if (FileExists(path) == 0) {
        return 0;
    }
    record = malloc(0x90);
    cursor = (unsigned int*)record;
    for (index = 0x24; index != 0; --index) {
        *cursor++ = 0;
    }
    strcpy((char*)record + 8, path);
    ((unsigned int*)record)[0x1b] = kind;
    if (kind == 1) {
        ok = Function414700(record, mode);
    }
    else if (kind == 2) {
        ok = Function414C60(record, mode);
    }
    else {
        if (kind != 4) {
            return 0;
        }
        ok = Function415130(record, mode);
    }
    if (ok == 0) {
        return 0;
    }
    return record;
}

// FUNCTION: WIZ8 0x00415130
unsigned char Function415130(void* record, unsigned int mode)
{
    unsigned int saved[0x24];
    W8StiHeader header;
    unsigned int done;
    unsigned char ok;
    int file;
    unsigned int* cursor;
    unsigned int* src;
    int index;

    cursor = saved;
    src = (unsigned int*)record;
    for (index = 0x24; index != 0; --index) {
        *cursor++ = *src++;
    }
    if (FileExists((char*)record + 8) == 0) {
        return 0;
    }
    file = FileOpen((char*)record + 8, 1, 0);
    if (file == 0) {
        return 0;
    }
    ok = ReadVirtualFile(file, &header, 0x40, &done);
    {
        int bi;
            for (bi = 0; bi < 0x40; ++bi) {
                }
        }
    if (ok == 0 || done != 0x40 || header.magic_00 != g_stci_magic_6000b4) {
        goto fail;
    }
    if ((header.flags_10 & 4) == 0) {
        if ((header.flags_10 & 8) == 0) {
            goto fail;
        }
        ok = Function4153F0((W8ImageRecord*)saved, mode, file, &header);
    }
    else {
        ok = Function415250((W8ImageRecord*)saved, mode, file, &header);
    }
    if (ok == 0) {
        goto fail;
    }
    CloseVirtualFile(file);
    {
        W8ImageRecord* rec = (W8ImageRecord*)saved;
        }
    if ((header.flags_10 & 0x10) != 0) {
        ((unsigned char*)saved)[6] |= 1;
    }
    ((unsigned short*)saved)[0] = header.field_16;
    ((unsigned short*)saved)[1] = header.field_14;
    ((unsigned char*)saved)[4] = header.type_2c;
    cursor = (unsigned int*)record;
    src = saved;
    for (index = 0x24; index != 0; --index) {
        *cursor++ = *src++;
    }
    return 1;
fail:
    CloseVirtualFile(file);
    return 0;
}

// FUNCTION: WIZ8 0x00410580
unsigned char Function410580(void* image, void* metrics)
{
    unsigned short count = *(unsigned short*)((char*)image + 0x8c);
    unsigned int* pixels;
    unsigned int* src;
    int n;
    unsigned int size;
    unsigned int* pixels2;
    unsigned int* src2;
    unsigned int tail;
    unsigned char* d;
    unsigned char* s;
    unsigned int* out = (unsigned int*)metrics;

    *(unsigned short*)(out + 3) = count;
    pixels = (unsigned int*)malloc((unsigned int)count << 4);
    out[2] = (unsigned int)pixels;
    if (pixels == 0) {
        return 0;
    }
    src = *(unsigned int**)((char*)image + 0x88);
    for (n = (unsigned int)count << 2; n != 0; --n) {
        *pixels++ = *src++;
    }
    pixels2 = (unsigned int*)malloc(*(unsigned int*)((char*)image + 0x84));
    *out = (unsigned int)pixels2;
    if (pixels2 == 0) {
        return 0;
    }
    size = *(unsigned int*)((char*)image + 0x84);
    out[1] = size;
    src2 = *(unsigned int**)((char*)image + 0x80);
    for (n = size >> 2; n != 0; --n) {
        *pixels2++ = *src2++;
    }
    d = (unsigned char*)pixels2;
    s = (unsigned char*)src2;
    for (tail = size & 3; tail != 0; --tail) {
        *d++ = *s++;
    }
    return 1;
}

// FUNCTION: WIZ8 0x00410190
unsigned short* Function410190(int table)
{
    unsigned short* out = (unsigned short*)malloc(0x200);
    unsigned char* src = (unsigned char*)(table + 2);
    unsigned short* dst = out;
    int remaining = 0x100;

    /* Packs each RGB triple to the display format selected by the channel
       masks. Zero triples stay zero; any other triple that packs to zero
       keeps the alpha bit so transparent black stays distinct. */
    for (; remaining != 0; --remaining) {
        unsigned int red = src[-2];
        unsigned int green = src[-1];
        unsigned int blue = src[0];
        unsigned int packed;

        red <<= (g_red_shift_650f50 & 31);
        green <<= (g_green_shift_650f54 & 31);
        blue <<= (((unsigned int)g_red_shift_650f50 >> 16) & 31);
        packed = (red & ((unsigned int)g_red_mask_650f4a << 16)) |
                 (green & (unsigned int)g_green_mask_650f4c) |
                 (blue & (unsigned int)g_blue_mask_650f4e);
        packed |= (unsigned int)g_alpha_mask_650f48;
        if ((unsigned short)packed == 0) {
            packed = 0;
            if (red + green + blue != 0) {
                packed = (unsigned int)g_alpha_mask_650f48 | 1;
            }
        }
        *dst++ = (unsigned short)packed;
        src += 4;
    }
    return out;
}

// FUNCTION: WIZ8 0x0040f9f0
unsigned char Function40F9F0(void* resource)
{
    Function40FA10(resource, 0x1c);
    free(resource);
    return 1;
}

// FUNCTION: WIZ8 0x0040fa10
unsigned char Function40FA10(void* resource, unsigned char mask)
{
    char* block = (char*)resource;
    if ((mask & 4) != 0 && (*(block + 6) & 4) != 0) {
        if (*(void**)(block + 0x70) != 0) {
            free(*(void**)(block + 0x70));
            *(void**)(block + 0x70) = 0;
        }
        if (*(void**)(block + 0x74) != 0) {
            free(*(void**)(block + 0x74));
            *(void**)(block + 0x74) = 0;
        }
        *(unsigned short*)(block + 6) ^= 4;
    }
    if ((mask & 8) != 0 && (*(block + 6) & 8) != 0) {
        free(*(void**)(block + 0x80));
        *(void**)(block + 0x80) = 0;
        if (*(short*)(block + 0x8c) != 0) {
            free(*(void**)(block + 0x88));
        }
        *(unsigned short*)(block + 6) ^= 8;
    }
    if ((mask & 0x10) != 0 && (*(block + 6) & 0x10) != 0 &&
        *(void**)(block + 0x78) != 0) {
        free(*(void**)(block + 0x78));
        *(unsigned short*)(block + 6) &= 0xffef;
    }
    return 1;
}

/* Pixel-format converters for loaded image data, driven by the current
   16-bit channel masks and shifts. */
// FUNCTION: WIZ8 0x00410620
void Function410620(unsigned short* pixels, int count)
{
    for (; count != 0; --count) {
        unsigned short pixel = *pixels;
        if (pixel != 0) {
            unsigned int combined =
                ((unsigned int)(pixel >> 6) << 16) |
                (unsigned short)(pixel << 11);
            unsigned short high = (unsigned short)((combined << 5) >> 16);
            *pixels = g_alpha_mask_650f48 | high;
        }
        ++pixels;
    }
}

// FUNCTION: WIZ8 0x00410670
void Function410670(unsigned short* pixels, int count)
{
    unsigned short* cursor = pixels;
    for (; count != 0; --count) {
        unsigned short pixel = *cursor;
        unsigned int low = (unsigned int)(unsigned short)(pixel << 11);
        unsigned int combined =
            ((unsigned int)(pixel >> 6) << 16) | low;
        unsigned int shifted = combined >> 5;
        unsigned int high = (unsigned int)(pixel >> 11);
        unsigned int merged = (high << 1) | (shifted & 0xffff);
        *cursor = (unsigned short)(((merged << 10) >> 16) & 0xffff);
        ++cursor;
    }
}

// FUNCTION: WIZ8 0x004106c0
void Function4106C0(unsigned short* pixels, int count)
{
    unsigned short* cursor = pixels;
    for (; count != 0; --count) {
        unsigned int combined =
            ((unsigned int)(*cursor >> 6) << 16) |
            (unsigned short)(*cursor << 11);
        unsigned int shifted = combined >> 5;
        *cursor = (unsigned short)(((shifted << 5) >> 16) << 1);
        ++cursor;
    }
}

// FUNCTION: WIZ8 0x00410700
void Function410700(unsigned short* pixels, int count)
{
    for (; count != 0; --count) {
        unsigned short pixel = *pixels;
        unsigned int expanded =
            ((unsigned int)(pixel >> 11) |
             ((((pixel & 0x1f) << 13) | (pixel & 0x7e0)) << 3));
        *pixels = (unsigned short)Function4104B0(expanded);
        ++pixels;
    }
}

// FUNCTION: WIZ8 0x004104b0
unsigned int Function4104B0(int value)
{
    unsigned char byte0 = (unsigned char)value;
    unsigned char byte1 = (unsigned char)((unsigned int)value >> 8);
    unsigned char byte2 = (unsigned char)((unsigned int)value >> 16);
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    unsigned int packed;

    if (g_red_shift_650f50 >= 0) {
        red = (unsigned int)byte0 << (g_red_shift_650f50 & 31);
    }
    else {
        red = (unsigned int)(byte0 >> (-g_red_shift_650f50 & 31));
    }
    if (g_green_shift_650f54 >= 0) {
        green = (unsigned int)byte1 << (g_green_shift_650f54 & 31);
    }
    else {
        green = (unsigned int)(byte1 >> (-g_green_shift_650f54 & 31));
    }
    if (g_blue_shift_650f52 >= 0) {
        blue = (unsigned int)byte2 << (g_blue_shift_650f52 & 31);
    }
    else {
        blue = (unsigned int)(byte2 >> (-g_blue_shift_650f52 & 31));
    }
    packed = (red & g_red_mask_650f4a) | (green & g_green_mask_650f4c) |
             (blue & g_blue_mask_650f4e);
    if ((unsigned short)packed == 0) {
        if (value == 0) {
            return 0;
        }
        return ((unsigned int)g_red_mask_650f4a << 16) |
               (unsigned int)g_alpha_mask_650f48 | 1;
    }
    return packed | ((unsigned int)g_red_mask_650f4a << 16) |
           (unsigned int)g_alpha_mask_650f48;
}

/* Parses a font resource into its 0xfc-byte runtime object. */
// FUNCTION: WIZ8 0x00406180
void* Function406180(W8FontLoadRequest* request)
{
    unsigned int* font;
    unsigned int* cursor;
    unsigned char* source;
    unsigned char ok;
    int index;
    struct {
        void* pixels_00;
        unsigned int size_04;
        void* metrics_08;
        unsigned short count_0c;
    } parsed;

    font = (unsigned int*)malloc(0xfc);
    if (font == 0) {
        return 0;
    }
    cursor = font;
    for (index = 0x3f; index != 0; --index) {
        *cursor++ = 0;
    }
    if ((request->flags_00 & 0x40) == 0 &&
        (request->flags_00 & 0x80) == 0) {
        free(font);
        return 0;
    }
    if ((request->flags_00 & 0x40) == 0) {
        source = *(unsigned char**)request->path_04;
        if (source == 0) {
            free(font);
            return 0;
        }
    }
    else {
        source = (unsigned char*)Function40F850(request->path_04, 0xc);
        if (source == 0) {
            free(font);
            return 0;
        }
    }
    if ((source[6] & 2) == 0) {
        free(font);
        Function40F9F0(source);
        return 0;
    }
    *((unsigned char*)font + 0xf8) = source[4];
    ok = Function410580(source, &parsed);
    if (ok == 0) {
        return 0;
    }
    *(unsigned short*)((char*)font + 0xf6) = parsed.count_0c;
    font[6] = (unsigned int)parsed.metrics_08;
    font[5] = (unsigned int)parsed.pixels_00;
    font[1] = parsed.size_04;
    if (source[4] == 8) {
        unsigned int* glyphs = (unsigned int*)font[2];
        unsigned int* table = *(unsigned int**)(source + 0x70);
        unsigned int* src = table;
        font[0x3a] = (unsigned int)&g_font_palette_b_6bdea0;
        font[0x3b] = (unsigned int)&g_font_palette_a_6bdaa0;
        if (glyphs == 0) {
            glyphs = (unsigned int*)malloc(0x400);
            font[2] = (unsigned int)glyphs;
            if (glyphs == 0) {
                goto done;
            }
        }
        index = 0x100;
        for (; index != 0; --index) {
            *glyphs++ = *src++;
        }
        if (font[4] != 0) {
            free((void*)font[4]);
            font[4] = 0;
        }
        font[4] = (unsigned int)Function410190((int)table);
        font[0x38] = font[4];
    }
done:
    if ((request->flags_00 & 0x40) != 0) {
        Function40F9F0(source);
    }
    return font;
}
