#include "wiz8/unattributed/quarantine_common.h"

#include "Font.h"
#include "himage.h"
#include "input.h"
#include "mousesystem.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* Address quarantine 005d2a51-005d730f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

typedef void (*W8TextInputFieldCallback)(unsigned char index, int active);

/* The allocation at 0x005D39B0 is 0x6c bytes. Repeated traversal fixes the
   scalar fields and links, while calls into the pinned SGP oracle fix
   0x10-0x5b as one ordinary MOUSE_REGION. */
struct W8TextInputField005D39B0 {
    unsigned char index;
    unsigned char value_01;
    short input_type;
    unsigned char capacity;
    unsigned char value_05[3];
    wchar_t* text;
    unsigned char length;
    unsigned char region_enabled;
    unsigned char value_0e[2];
    MOUSE_REGION region;
    W8TextInputFieldCallback callback;
    unsigned char flag_60;
    unsigned char blocks_mouse_callback;
    unsigned char value_62[2];
    W8TextInputField005D39B0* next;
    W8TextInputField005D39B0* previous;
};

/* 0x005D3520 allocates this record once per active input session, and
   0x005D35E0 fills the same fixed layout for its three presentation modes. */
struct W8TextInputStyle005D35E0 {
    short font;
    unsigned short foreground;
    unsigned char value_04;
    unsigned char value_05;
    unsigned char value_06;
    unsigned char value_07;
    unsigned char value_08;
    unsigned char flag_09;
    unsigned short colour_0a;
    unsigned short colour_0c;
    unsigned short colour_0e;
    unsigned char flag_10;
    unsigned char disabled_foreground;
    unsigned char disabled_background;
    unsigned char value_13;
    unsigned short disabled_colour;
    unsigned short colour_16;
};

struct W8TextInputSession005D3520 {
    W8TextInputField005D39B0* first;
    W8TextInputStyle005D35E0* style;
    W8TextInputSession005D3520* previous;
};

static_assert(sizeof(W8TextInputField005D39B0) == 0x6c,
              "text input field must match the retail allocation");
static_assert(sizeof(W8TextInputStyle005D35E0) == 0x18,
              "text input style must match the retail allocation");
static_assert(sizeof(W8TextInputSession005D3520) == 0x0c,
              "text input session must match the retail allocation");

extern "C" {
extern int g_wiz_text_mono_font_683630;
extern int g_font12point1_683648;
extern unsigned char g_flag_6f04ed;
unsigned char g_flag_69c808;
}
extern wchar_t g_no_target_text[];
extern unsigned char SetValue5FF5F0(int font);
extern unsigned char FillSurfaceRect(int surface_id, int left, int top,
                                     int right, int bottom, int colour);
extern void MarkScreenRectDirty(int left, int top, int right, int bottom,
                                int flags);
extern void* Function402B90(int surface_id, unsigned int* pitch);
extern void Function402C30(int surface_id);
extern int Function4124A0(void* pixels, unsigned int pitch, int* rectangle);
extern int Function40BFC0(MOUSE_REGION* region);
extern bool IsModalOpen(void);
extern int Function55EF80(void);
extern void Function55EE70(int value);
void Function5D4CB0(MOUSE_REGION* region, int reason);
extern void Function5D4F10(MOUSE_REGION* region, int reason);

static W8TextInputStyle005D35E0* g_text_input_style_0069c7ec;
static W8TextInputSession005D3520* g_text_input_sessions_0069c7f0;
static W8TextInputField005D39B0* g_text_input_first_0069c7f4;
static W8TextInputField005D39B0* g_text_input_last_0069c7f8;
static W8TextInputField005D39B0* g_text_input_current_0069c7fc;
static unsigned char g_text_input_active_0069c809;
static unsigned char g_text_input_cursor_visible_0069c80a;
static unsigned char g_text_input_cursor_0069c80b;
static unsigned char g_text_input_anchor_0069c80c;
static unsigned char g_text_input_end_0069c80d;
static unsigned char g_text_input_visible_end_0069c80e;
static unsigned char g_text_input_visible_start_0069c80f;
static unsigned char g_text_input_horizontal_key_0069c7e8;
static unsigned char g_text_input_click_start_0069c7e9;
static int g_text_input_cursor_width_0069c5d8;
static size_t g_text_input_visible_count_0069c7e4;

void Function5D35E0(int mode);
unsigned int Function5D5A10(int width, int cursor, const wchar_t* text,
                           int* cursor_width, size_t* visible_count);
void Function5D52C0(W8TextInputField005D39B0* field);
void Function5D5390(void);
void Function5D5770(W8TextInputField005D39B0* field);
void Function5D5C40(void);
void Function5D4970(unsigned short character);
void Function5D4B70(unsigned short character);
void Function5D5BF0(unsigned char cursor);
unsigned short Function402780(unsigned short key, unsigned char modifiers);
unsigned short Function402800(unsigned short character);
unsigned short Function402820(unsigned short character);
unsigned short Function402840(unsigned short character);
int Function402880(int character);
int Function4028A0(int character);

// FUNCTION: WIZ8 0x005D3520
void Function5D3520(int mode)
{
    if (g_text_input_first_0069c7f4 != 0) {
        W8TextInputSession005D3520* session =
            (W8TextInputSession005D3520*)malloc(sizeof(W8TextInputSession005D3520));
        session->first = g_text_input_first_0069c7f4;
        session->style = g_text_input_style_0069c7ec;
        session->previous = g_text_input_sessions_0069c7f0;
        g_text_input_sessions_0069c7f0 = session;
        for (W8TextInputField005D39B0* field = g_text_input_first_0069c7f4;
             field != 0; field = field->next) {
            if (field->region_enabled != 0) {
                MSYS_DisableRegion(&field->region);
                field->region_enabled = 0;
            }
        }
        g_text_input_current_0069c7fc = 0;
    }
    g_text_input_first_0069c7f4 = 0;
    g_text_input_style_0069c7ec =
        (W8TextInputStyle005D35E0*)malloc(sizeof(W8TextInputStyle005D35E0));
    g_text_input_active_0069c809 = 1;
    g_flag_69c808 = 0;
    g_text_input_style_0069c7ec->flag_09 = 0;
    g_text_input_style_0069c7ec->flag_10 = 1;
    g_text_input_style_0069c7ec->colour_0e = Get16BPPColor(0x0a0a0a);
    g_text_input_visible_start_0069c80f = 0;
    Function5D35E0(mode);
}

// FUNCTION: WIZ8 0x005D35E0
void Function5D35E0(int mode)
{
    if (mode == 0) {
        g_text_input_style_0069c7ec->font = (short)g_font12point1_683648;
        g_text_input_style_0069c7ec->foreground = Get16BPPColor(0x00c8c8);
        g_text_input_style_0069c7ec->colour_16 = Get16BPPColor(0xffffff);
        g_text_input_style_0069c7ec->colour_0c = Get16BPPColor(0x513d18);
        g_text_input_style_0069c7ec->colour_0a = Get16BPPColor(0x878a88);
        g_text_input_style_0069c7ec->flag_09 = 1;
        g_text_input_style_0069c7ec->value_04 = 0x8d;
        g_text_input_style_0069c7ec->value_05 = 0;
        g_text_input_style_0069c7ec->value_06 = 0xd0;
    }
    else if (mode == 1) {
        g_text_input_style_0069c7ec->font = (short)g_wiz_text_mono_font_683630;
        g_text_input_style_0069c7ec->foreground = Get16BPPColor(0x632a1e);
        g_text_input_style_0069c7ec->colour_16 = Get16BPPColor(0x0a0a0a);
        g_text_input_style_0069c7ec->colour_0c = Get16BPPColor(0);
        g_text_input_style_0069c7ec->colour_0a = Get16BPPColor(0);
        g_text_input_style_0069c7ec->flag_09 = 1;
        g_text_input_style_0069c7ec->value_04 = 4;
        g_text_input_style_0069c7ec->value_05 = 5;
        g_text_input_style_0069c7ec->value_06 = 4;
        g_text_input_style_0069c7ec->value_07 = 5;
        g_text_input_style_0069c7ec->value_08 = 3;
        g_text_input_style_0069c7ec->colour_0e = Get16BPPColor(0xffffff);
        return;
    }
    else if (mode == 2) {
        g_text_input_style_0069c7ec->font = (short)g_wiz_text_mono_font_683630;
        g_text_input_style_0069c7ec->foreground = Get16BPPColor(0xffffff);
        g_text_input_style_0069c7ec->colour_16 = Get16BPPColor(0xffffff);
        g_text_input_style_0069c7ec->colour_0c = Get16BPPColor(0);
        g_text_input_style_0069c7ec->colour_0a = Get16BPPColor(0);
        g_text_input_style_0069c7ec->flag_09 = 1;
        g_text_input_style_0069c7ec->value_04 = 0x8d;
        g_text_input_style_0069c7ec->value_05 = 0;
        g_text_input_style_0069c7ec->value_06 = 0xd0;
    }
    else {
        return;
    }
    g_text_input_style_0069c7ec->value_07 = 0xcd;
    g_text_input_style_0069c7ec->value_08 = 0xcd;
    g_text_input_style_0069c7ec->colour_0e = Get16BPPColor(0x0a0a0a);
}

// FUNCTION: WIZ8 0x005D3800
void Function5D3800(void)
{
    W8TextInputField005D39B0* field = g_text_input_first_0069c7f4;
    if (field == 0) return;
    do {
        g_text_input_first_0069c7f4 = field->next;
        if (field->text != 0) {
            free(field->text);
            field->text = 0;
            MSYS_RemoveRegion(&field->region);
        }
        free(field);
        field = g_text_input_first_0069c7f4;
    } while (field != 0);

    free(g_text_input_style_0069c7ec);
    W8TextInputSession005D3520* session = g_text_input_sessions_0069c7f0;
    g_text_input_style_0069c7ec = 0;
    g_text_input_first_0069c7f4 = 0;
    if (session == 0) {
        g_text_input_active_0069c809 = 0;
        g_flag_69c808 = 0;
        g_text_input_current_0069c7fc = 0;
        return;
    }
    g_text_input_first_0069c7f4 = session->first;
    g_text_input_style_0069c7ec = session->style;
    g_text_input_sessions_0069c7f0 = session->previous;
    free(session);
    for (field = g_text_input_first_0069c7f4; field != 0; field = field->next) {
        if (field->region_enabled == 0) {
            MSYS_EnableRegion(&field->region);
            field->region_enabled = 1;
        }
    }
    field = g_text_input_first_0069c7f4;
    if (g_text_input_current_0069c7fc == 0)
        g_text_input_current_0069c7fc = g_text_input_first_0069c7f4;
    for (; field != 0; field = field->next) {
        if (field != g_text_input_current_0069c7fc || field->index != 0 ||
            field->region_enabled == 0)
            continue;
        g_text_input_current_0069c7fc = field;
        if (field->text == 0) {
            g_text_input_cursor_visible_0069c80a = 0;
            g_flag_69c808 = 0;
            if (field->callback != 0) field->callback(field->index, 1);
        }
        else {
            g_text_input_anchor_0069c80c = 0;
            g_text_input_end_0069c80d = field->length;
            g_text_input_cursor_0069c80b = field->length;
            g_text_input_visible_end_0069c80e = Function5D5A10(
                field->region.RegionBottomRightX - field->region.RegionTopLeftX - 10,
                g_text_input_cursor_0069c80b, field->text,
                &g_text_input_cursor_width_0069c5d8,
                &g_text_input_visible_count_0069c7e4);
            g_text_input_cursor_0069c80b = g_text_input_current_0069c7fc->length;
            g_text_input_cursor_visible_0069c80a = 1;
            g_flag_69c808 = 1;
        }
        break;
    }
    if (g_text_input_first_0069c7f4 == 0) g_text_input_current_0069c7fc = 0;
}

// FUNCTION: WIZ8 0x005D39B0
char Function5D39B0(int left, int top, int width, int height, int priority,
                    const wchar_t* text, unsigned char capacity,
                    short input_type, unsigned char flag)
{
    W8TextInputField005D39B0* field =
        (W8TextInputField005D39B0*)malloc(sizeof(W8TextInputField005D39B0));
    memset(field, 0, sizeof(W8TextInputField005D39B0));
    if (g_text_input_first_0069c7f4 == 0) {
        g_text_input_first_0069c7f4 = field;
        g_text_input_last_0069c7f8 = field;
        field->index = 0;
    }
    else {
        g_text_input_last_0069c7f8->next = field;
        field->previous = g_text_input_last_0069c7f8;
        field->index = g_text_input_last_0069c7f8->index + 1;
        g_text_input_last_0069c7f8 = field;
    }
    field->input_type = input_type;
    if (input_type == 0x1002) capacity = 6;
    field->text = (wchar_t*)malloc((capacity + 1) * sizeof(wchar_t));
    if (text == 0) {
        field->length = 0;
        swprintf(field->text, g_no_target_text);
    }
    else {
        field->length = (unsigned char)wcslen(text);
        swprintf(field->text, text);
    }
    field->capacity = capacity;
    if (g_text_input_first_0069c7f4 == field) {
        g_text_input_anchor_0069c80c = 0;
        g_text_input_end_0069c80d = field->length;
        g_text_input_cursor_0069c80b = field->length;
        if (g_text_input_current_0069c7fc != 0) {
            g_text_input_visible_end_0069c80e = Function5D5A10(
                g_text_input_current_0069c7fc->region.RegionBottomRightX -
                    g_text_input_current_0069c7fc->region.RegionTopLeftX - 10,
                g_text_input_cursor_0069c80b, g_text_input_current_0069c7fc->text,
                &g_text_input_cursor_width_0069c5d8,
                &g_text_input_visible_count_0069c7e4);
        }
        g_text_input_cursor_visible_0069c80a = 1;
    }
    field->region_enabled = 1;
    MSYS_DefineRegion(&field->region, (unsigned short)left, (unsigned short)top,
                      (unsigned short)(left + width), (unsigned short)(top + height),
                      (signed char)priority, MSYS_NO_CURSOR,
                      Function5D4CB0, Function5D4F10);
    MSYS_SetRegionUserData(&field->region, 0, field->index);
    field->flag_60 = flag;
    return field->index;
}

// FUNCTION: WIZ8 0x005D3B40
void Function5D3B40(int index)
{
    W8TextInputField005D39B0* field = g_text_input_first_0069c7f4;
    while (field != 0 && field->index != index) field = field->next;
    if (field == 0) return;
    if (field == g_text_input_first_0069c7f4) g_text_input_first_0069c7f4 = field->next;
    if (field == g_text_input_last_0069c7f8) g_text_input_last_0069c7f8 = field->previous;
    if (field->next != 0) field->next->previous = field->previous;
    if (field->previous != 0) field->previous->next = field->next;
    if (field->text != 0) {
        free(field->text);
        field->text = 0;
        MSYS_RemoveRegion(&field->region);
    }
    if (field == g_text_input_current_0069c7fc) g_text_input_current_0069c7fc = 0;
    free(field);
    if (g_text_input_first_0069c7f4 == 0) {
        g_text_input_active_0069c809 = 0;
        g_flag_69c808 = 0;
    }
}

// FUNCTION: WIZ8 0x005D3D00
unsigned char Function5D3D00(int index)
{
    for (W8TextInputField005D39B0* field = g_text_input_first_0069c7f4;
         field != 0; field = field->next) {
        if (field->index == index) return field->length;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005D3D20
void Function5D3D20(char index)
{
    W8TextInputField005D39B0* field = g_text_input_first_0069c7f4;
    while (field != 0 &&
           (field == g_text_input_current_0069c7fc || field->index != index ||
            field->region_enabled == 0)) {
        field = field->next;
    }
    if (field == 0) return;
    g_text_input_current_0069c7fc = field;
    if (field->text != 0) {
        g_text_input_anchor_0069c80c = 0;
        g_text_input_end_0069c80d = field->length;
        g_text_input_cursor_0069c80b = field->length;
        g_text_input_visible_end_0069c80e = Function5D5A10(
            field->region.RegionBottomRightX - field->region.RegionTopLeftX - 10,
            g_text_input_cursor_0069c80b, field->text,
            &g_text_input_cursor_width_0069c5d8,
            &g_text_input_visible_count_0069c7e4);
        g_text_input_cursor_0069c80b = g_text_input_current_0069c7fc->length;
        g_text_input_cursor_visible_0069c80a = 1;
        g_flag_69c808 = 1;
        return;
    }
    g_text_input_cursor_visible_0069c80a = 0;
    g_flag_69c808 = 0;
    if (field->callback != 0) field->callback(field->index, 1);
}

// FUNCTION: WIZ8 0x005D3DF0
void Function5D3DF0(void)
{
    if (g_text_input_current_0069c7fc == 0) return;
    W8TextInputField005D39B0* previous = g_text_input_current_0069c7fc;
    if (previous->text == 0) {
        if (previous->callback != 0) previous->callback(previous->index, 0);
    }
    else {
        Function5D5770(previous);
    }

    bool found = false;
    do {
        g_text_input_current_0069c7fc = g_text_input_current_0069c7fc->next;
        if (g_text_input_current_0069c7fc == 0)
            g_text_input_current_0069c7fc = g_text_input_first_0069c7f4;
        if (g_text_input_current_0069c7fc->region_enabled != 0) {
            found = true;
            if (g_text_input_current_0069c7fc->text == 0) {
                g_text_input_cursor_visible_0069c80a = 0;
                g_flag_69c808 = 0;
                if (g_text_input_current_0069c7fc->callback != 0)
                    g_text_input_current_0069c7fc->callback(
                        g_text_input_current_0069c7fc->index, 1);
            }
            else {
                g_text_input_anchor_0069c80c = 0;
                g_text_input_end_0069c80d = g_text_input_current_0069c7fc->length;
                g_text_input_cursor_0069c80b = g_text_input_current_0069c7fc->length;
                g_text_input_visible_end_0069c80e = Function5D5A10(
                    g_text_input_current_0069c7fc->region.RegionBottomRightX -
                        g_text_input_current_0069c7fc->region.RegionTopLeftX - 10,
                    g_text_input_cursor_0069c80b, g_text_input_current_0069c7fc->text,
                    &g_text_input_cursor_width_0069c5d8,
                    &g_text_input_visible_count_0069c7e4);
                g_text_input_cursor_visible_0069c80a = 1;
                g_flag_69c808 = 1;
            }
        }
        if (g_text_input_current_0069c7fc == previous) break;
        if (found) return;
    } while (true);
    g_flag_69c808 = 0;
}

// FUNCTION: WIZ8 0x005D3F00
void Function5D3F00(void)
{
    if (g_text_input_current_0069c7fc == 0) return;
    if (g_text_input_current_0069c7fc->text != 0) {
        Function5D5770(g_text_input_current_0069c7fc);
    }
    else if (g_text_input_current_0069c7fc->callback != 0) {
        g_text_input_current_0069c7fc->callback(
            g_text_input_current_0069c7fc->index, 0);
    }
    g_flag_69c808 = 0;
    g_text_input_current_0069c7fc = 0;
}

// FUNCTION: WIZ8 0x005D3F50
unsigned int Function5D3F50(const InputAtom* input)
{
    g_text_input_horizontal_key_0069c7e8 = 0;
    if (g_text_input_active_0069c809 == 0 || g_flag_69c808 == 0 ||
        g_text_input_current_0069c7fc == 0 ||
        (input->usEvent != KEY_DOWN && input->usEvent != KEY_REPEAT) ||
        input->usParam == 0x1b || input->usParam == 0x0d || input->usParam == 9 ||
        (input->usKeyState & ALT_DOWN) != 0 ||
        ((input->usKeyState & CTRL_DOWN) != 0 && input->usParam != 0x2e &&
         input->usParam != 0x27 && input->usParam != 0x25) ||
        (input->usParam > 0x6f && input->usParam < 0x7c)) {
        return 0;
    }

    unsigned char selection_end = g_text_input_end_0069c80d;
    switch (input->usParam) {
    case 8:
        if (g_text_input_cursor_visible_0069c80a == 0) {
            if (g_text_input_cursor_0069c80b != 0) {
                --g_text_input_cursor_0069c80b;
                g_text_input_visible_end_0069c80e = Function5D5A10(
                    g_text_input_current_0069c7fc->region.RegionBottomRightX -
                        g_text_input_current_0069c7fc->region.RegionTopLeftX - 10,
                    g_text_input_cursor_0069c80b, g_text_input_current_0069c7fc->text,
                    &g_text_input_cursor_width_0069c5d8,
                    &g_text_input_visible_count_0069c7e4);
                memmove(g_text_input_current_0069c7fc->text + g_text_input_cursor_0069c80b,
                        g_text_input_current_0069c7fc->text + g_text_input_cursor_0069c80b + 1,
                        (g_text_input_current_0069c7fc->length -
                         g_text_input_cursor_0069c80b) * sizeof(wchar_t));
                --g_text_input_current_0069c7fc->length;
                return 1;
            }
        }
        else if (g_text_input_anchor_0069c80c != selection_end) {
            unsigned char first = g_text_input_anchor_0069c80c;
            unsigned char last = selection_end;
            if (last < first) {
                unsigned char swap = first;
                first = last;
                last = swap;
            }
            memmove(g_text_input_current_0069c7fc->text + first,
                    g_text_input_current_0069c7fc->text + last,
                    (g_text_input_current_0069c7fc->length - last + 1) * sizeof(wchar_t));
            g_text_input_current_0069c7fc->length -= last - first;
            g_text_input_cursor_0069c80b = first;
            g_text_input_anchor_0069c80c = 0;
            g_text_input_visible_end_0069c80e = Function5D5A10(
                g_text_input_current_0069c7fc->region.RegionBottomRightX -
                    g_text_input_current_0069c7fc->region.RegionTopLeftX - 10,
                first, g_text_input_current_0069c7fc->text,
                &g_text_input_cursor_width_0069c5d8,
                &g_text_input_visible_count_0069c7e4);
            g_text_input_cursor_visible_0069c80a = 0;
            return 1;
        }
        break;

    case 0x23: /* End */
        if ((input->usKeyState & SHIFT_DOWN) == 0) {
            g_text_input_cursor_visible_0069c80a = 0;
        }
        else if (g_text_input_cursor_visible_0069c80a == 0) {
            g_text_input_cursor_visible_0069c80a = 1;
            g_text_input_anchor_0069c80c = g_text_input_cursor_0069c80b;
        }
        g_text_input_cursor_0069c80b = g_text_input_current_0069c7fc->length;
        g_text_input_end_0069c80d = g_text_input_cursor_0069c80b;
        g_text_input_visible_end_0069c80e = Function5D5A10(
            g_text_input_current_0069c7fc->region.RegionBottomRightX -
                g_text_input_current_0069c7fc->region.RegionTopLeftX - 10,
            g_text_input_cursor_0069c80b, g_text_input_current_0069c7fc->text,
            &g_text_input_cursor_width_0069c5d8,
            &g_text_input_visible_count_0069c7e4);
        return 1;

    case 0x24: /* Home */
        if ((input->usKeyState & SHIFT_DOWN) == 0) {
            g_text_input_cursor_visible_0069c80a = 0;
        }
        else if (g_text_input_cursor_visible_0069c80a == 0) {
            g_text_input_cursor_visible_0069c80a = 1;
            g_text_input_anchor_0069c80c = g_text_input_cursor_0069c80b;
        }
        g_text_input_cursor_0069c80b = 0;
        g_text_input_end_0069c80d = 0;
        g_text_input_visible_end_0069c80e = Function5D5A10(
            g_text_input_current_0069c7fc->region.RegionBottomRightX -
                g_text_input_current_0069c7fc->region.RegionTopLeftX - 10,
            0, g_text_input_current_0069c7fc->text,
            &g_text_input_cursor_width_0069c5d8,
            &g_text_input_visible_count_0069c7e4);
        return 1;

    case 0x25: /* Left */
        g_text_input_horizontal_key_0069c7e8 = 1;
        if ((input->usKeyState & SHIFT_DOWN) != 0) {
            if (g_text_input_cursor_visible_0069c80a == 0) {
                g_text_input_cursor_visible_0069c80a = 1;
                g_text_input_anchor_0069c80c = g_text_input_cursor_0069c80b;
            }
            if (g_text_input_cursor_0069c80b != 0) --g_text_input_cursor_0069c80b;
            g_text_input_end_0069c80d = g_text_input_cursor_0069c80b;
        }
        else if (g_text_input_cursor_visible_0069c80a != 0) {
            g_text_input_cursor_0069c80b = g_text_input_anchor_0069c80c;
            g_text_input_cursor_visible_0069c80a = 0;
        }
        else if (g_text_input_cursor_0069c80b != 0) {
            --g_text_input_cursor_0069c80b;
        }
        g_text_input_visible_end_0069c80e = Function5D5A10(
            g_text_input_current_0069c7fc->region.RegionBottomRightX -
                g_text_input_current_0069c7fc->region.RegionTopLeftX - 10,
            g_text_input_cursor_0069c80b, g_text_input_current_0069c7fc->text,
            &g_text_input_cursor_width_0069c5d8,
            &g_text_input_visible_count_0069c7e4);
        return 1;

    case 0x27: /* Right */
        g_text_input_horizontal_key_0069c7e8 = 1;
        if ((input->usKeyState & SHIFT_DOWN) != 0) {
            if (g_text_input_cursor_visible_0069c80a == 0) {
                g_text_input_cursor_visible_0069c80a = 1;
                g_text_input_anchor_0069c80c = g_text_input_cursor_0069c80b;
            }
            if (g_text_input_cursor_0069c80b < g_text_input_current_0069c7fc->length)
                ++g_text_input_cursor_0069c80b;
            g_text_input_end_0069c80d = g_text_input_cursor_0069c80b;
        }
        else if (g_text_input_cursor_visible_0069c80a != 0) {
            g_text_input_cursor_0069c80b = selection_end;
            g_text_input_cursor_visible_0069c80a = 0;
        }
        else if (g_text_input_cursor_0069c80b < g_text_input_current_0069c7fc->length) {
            ++g_text_input_cursor_0069c80b;
        }
        g_text_input_visible_end_0069c80e = Function5D5A10(
            g_text_input_current_0069c7fc->region.RegionBottomRightX -
                g_text_input_current_0069c7fc->region.RegionTopLeftX - 10,
            g_text_input_cursor_0069c80b, g_text_input_current_0069c7fc->text,
            &g_text_input_cursor_width_0069c5d8,
            &g_text_input_visible_count_0069c7e4);
        return 1;

    case 0x2e: /* Delete */
        if ((input->usKeyState & CTRL_DOWN) != 0) {
            g_text_input_current_0069c7fc->text[0] = L'\0';
            g_text_input_current_0069c7fc->length = 0;
            g_text_input_cursor_0069c80b = 0;
            g_text_input_anchor_0069c80c = 0;
            g_text_input_end_0069c80d = 0;
            g_text_input_cursor_visible_0069c80a = 0;
            Function5D5BF0(0);
            return 1;
        }
        if (g_text_input_cursor_visible_0069c80a != 0 &&
            g_text_input_anchor_0069c80c != selection_end) {
            unsigned char first = g_text_input_anchor_0069c80c;
            unsigned char last = selection_end;
            if (last < first) {
                unsigned char swap = first;
                first = last;
                last = swap;
            }
            memmove(g_text_input_current_0069c7fc->text + first,
                    g_text_input_current_0069c7fc->text + last,
                    (g_text_input_current_0069c7fc->length - last + 1) * sizeof(wchar_t));
            g_text_input_current_0069c7fc->length -= last - first;
            g_text_input_cursor_0069c80b = first;
            g_text_input_cursor_visible_0069c80a = 0;
            Function5D5BF0(first);
            return 1;
        }
        if (g_text_input_cursor_0069c80b < g_text_input_current_0069c7fc->length) {
            memmove(g_text_input_current_0069c7fc->text + g_text_input_cursor_0069c80b,
                    g_text_input_current_0069c7fc->text + g_text_input_cursor_0069c80b + 1,
                    (g_text_input_current_0069c7fc->length -
                     g_text_input_cursor_0069c80b) * sizeof(wchar_t));
            --g_text_input_current_0069c7fc->length;
        }
        Function5D5BF0(g_text_input_cursor_0069c80b);
        return 1;

    default:
        break;
    }

    unsigned int character = Function402780((unsigned short)input->usParam,
                                            input->usKeyState);
    if (character == 0) return 1;
    if (character == 0x25 || character == 0x5c) return 0;

    if (g_text_input_cursor_visible_0069c80a != 0 &&
        g_text_input_anchor_0069c80c != g_text_input_end_0069c80d) {
        unsigned char first = g_text_input_anchor_0069c80c;
        unsigned char last = g_text_input_end_0069c80d;
        if (last < first) {
            unsigned char swap = first;
            first = last;
            last = swap;
        }
        memmove(g_text_input_current_0069c7fc->text + first,
                g_text_input_current_0069c7fc->text + last,
                (g_text_input_current_0069c7fc->length - last + 1) * sizeof(wchar_t));
        g_text_input_current_0069c7fc->length -= last - first;
        g_text_input_cursor_0069c80b = first;
        g_text_input_cursor_visible_0069c80a = 0;
    }

    unsigned short input_type = (unsigned short)g_text_input_current_0069c7fc->input_type;
    if (input_type > 0x0fff) {
        Function5D4970((unsigned short)character);
        return 1;
    }
    if (character == L' ' && (input_type & 4) != 0) {
        Function5D4B70(L' ');
        return 1;
    }
    if (character == L'-' && (input_type & 2) != 0 && g_text_input_cursor_0069c80b == 0) {
        Function5D4B70(L'-');
        return 1;
    }
    if (character >= L'0' && character <= L'9' && (input_type & 1) != 0) {
        Function5D4B70((unsigned short)character);
        return 1;
    }
    if ((input_type & 2) != 0) {
        if (Function402800((unsigned short)character) != 0) {
            if ((input_type & 0x20) != 0) character = Function4028A0(character);
            Function5D4B70((unsigned short)character);
            return 1;
        }
        if (Function402820((unsigned short)character) != 0) {
            if ((input_type & 0x10) != 0) character = Function402880(character);
            Function5D4B70((unsigned short)character);
            return 1;
        }
    }
    if ((input_type & 8) != 0 && Function402840((unsigned short)character) != 0) {
        Function5D4B70((unsigned short)character);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005D4970
void Function5D4970(unsigned short character)
{
    short input_type = g_text_input_current_0069c7fc->input_type;
    if (input_type == 0x1000) {
        if (Function402800(character) == 0 && Function402820(character) == 0 &&
            (character < L'0' || character > L'9') && character != L'_' &&
            character != L'.') {
            return;
        }
        if (g_text_input_cursor_0069c80b == 0 && character >= L'0' && character <= L'9')
            return;
        Function5D4B70(character);
        return;
    }
    if (input_type == 0x1001) {
        if (g_text_input_cursor_0069c80b == 0) {
            if (Function402820(character) != 0) {
                Function5D4B70(character);
                return;
            }
            if (Function402800(character) == 0) return;
            Function5D4B70((unsigned short)Function4028A0(character));
            return;
        }
        if (character >= L'0' && character <= L'9') Function5D4B70(character);
        return;
    }
    if (input_type != 0x1002) return;
    if (g_text_input_cursor_0069c80b == 0) {
        if (character >= L'0' && character <= L'2') Function5D4B70(character);
        return;
    }
    if (g_text_input_cursor_0069c80b == 1) {
        if (character >= L'0' && character <= L'9') {
            if (g_text_input_current_0069c7fc->text[0] != L'2' || character <= L'3')
                Function5D4B70(character);
        }
        if (g_text_input_current_0069c7fc->text[2] == L'\0') {
            Function5D4B70(L':');
            return;
        }
        ++g_text_input_cursor_0069c80b;
        Function5D5BF0(g_text_input_cursor_0069c80b);
        return;
    }
    if (g_text_input_cursor_0069c80b == 2) {
        if (character == L':') {
            Function5D4B70(L':');
            return;
        }
        if (character < L'0' || character > L'9') return;
        Function5D4B70(L':');
        Function5D4B70(character);
        return;
    }
    if (g_text_input_cursor_0069c80b == 3) {
        if (character >= L'0' && character <= L'5') Function5D4B70(character);
        return;
    }
    if (g_text_input_cursor_0069c80b == 4 &&
        character >= L'0' && character <= L'9') {
        Function5D4B70(character);
    }
}

// FUNCTION: WIZ8 0x005D4B70
void Function5D4B70(unsigned short character)
{
    unsigned char length = g_text_input_current_0069c7fc->length;
    if (g_text_input_current_0069c7fc->capacity <= length) {
        g_text_input_current_0069c7fc->length = g_text_input_current_0069c7fc->capacity;
        g_text_input_current_0069c7fc->text[
            g_text_input_current_0069c7fc->length - 1] = character;
        g_text_input_current_0069c7fc->text[
            g_text_input_current_0069c7fc->length] = L'\0';
        return;
    }
    if (g_text_input_cursor_0069c80b == length) {
        g_text_input_current_0069c7fc->text[length] = character;
        g_text_input_current_0069c7fc->text[length + 1] = L'\0';
        ++g_text_input_current_0069c7fc->length;
        g_text_input_cursor_0069c80b = g_text_input_current_0069c7fc->length;
    }
    else {
        for (int position = length;
             position >= g_text_input_cursor_0069c80b; --position) {
            g_text_input_current_0069c7fc->text[position + 1] =
                g_text_input_current_0069c7fc->text[position];
        }
        g_text_input_current_0069c7fc->text[g_text_input_cursor_0069c80b] = character;
        ++g_text_input_current_0069c7fc->length;
        ++g_text_input_cursor_0069c80b;
    }
    g_text_input_visible_end_0069c80e = Function5D5A10(
        g_text_input_current_0069c7fc->region.RegionBottomRightX -
            g_text_input_current_0069c7fc->region.RegionTopLeftX - 10,
        g_text_input_cursor_0069c80b, g_text_input_current_0069c7fc->text,
        &g_text_input_cursor_width_0069c5d8,
        &g_text_input_visible_count_0069c7e4);
}

// FUNCTION: WIZ8 0x005D4CB0
void Function5D4CB0(MOUSE_REGION* region, int reason)
{
    if (IsModalOpen()) return;

    int field_index = MSYS_GetRegionUserData(region, 0);
    for (W8TextInputField005D39B0* field = g_text_input_first_0069c7f4;
         field != 0; field = field->next) {
        if (field->index == field_index && field->blocks_mouse_callback != 0)
            return;
    }

    if ((reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) != 0)
        Function55EE70(Function55EF80());
    if ((reason & MSYS_CALLBACK_REASON_LOST_MOUSE) != 0)
        Function55EE70(-1);

    if (g_flag_6f04ed == 0 || g_text_input_current_0069c7fc == 0 ||
        (reason & MSYS_CALLBACK_REASON_MOVE) == 0) {
        return;
    }

    field_index = MSYS_GetRegionUserData(region, 0);
    if (field_index != g_text_input_current_0069c7fc->index) {
        Function5D5770(g_text_input_current_0069c7fc);
        for (W8TextInputField005D39B0* field = g_text_input_first_0069c7f4;
             field != 0; field = field->next) {
            if (field->index == field_index) {
                g_text_input_click_start_0069c7e9 = 0;
                g_text_input_cursor_0069c80b = 0;
                g_text_input_current_0069c7fc = field;
                g_text_input_visible_end_0069c80e = Function5D5A10(
                    field->region.RegionBottomRightX -
                        field->region.RegionTopLeftX - 10,
                    0, field->text, &g_text_input_cursor_width_0069c5d8,
                    &g_text_input_visible_count_0069c7e4);
                g_text_input_cursor_visible_0069c80a = 0;
                g_text_input_anchor_0069c80c = 0;
                g_text_input_end_0069c80d = 0;
                break;
            }
        }
    }

    W8TextInputField005D39B0* current_field = g_text_input_current_0069c7fc;
    if (current_field->text == 0 || g_flag_6f04ed == 0) return;

    unsigned char position = g_text_input_visible_end_0069c80e;
    int mouse_offset = gusMouseXPos - current_field->region.RegionTopLeftX;
    unsigned int start = g_text_input_visible_end_0069c80e;
    short width = StringPixLengthArg(
        g_text_input_style_0069c7ec->font, 1,
        (unsigned short*)(current_field->text + start));
    if ((width / 2) / 2 < mouse_offset) {
        int count = 1;
        int previous_width = width / 2;
        do {
            if (current_field->length <= position) break;
            ++position;
            ++count;
            width = StringPixLengthArg(
                g_text_input_style_0069c7ec->font, count,
                (unsigned short*)(current_field->text + start));
            int midpoint = (width - previous_width) / 2 + previous_width;
            previous_width = width;
            if (mouse_offset <= midpoint) break;
        } while (true);
    }

    if (position == g_text_input_click_start_0069c7e9) {
        g_text_input_cursor_visible_0069c80a = 0;
        return;
    }
    if (g_text_input_click_start_0069c7e9 < position) {
        g_text_input_anchor_0069c80c = g_text_input_click_start_0069c7e9;
        g_text_input_end_0069c80d = position;
    }
    else {
        g_text_input_end_0069c80d = g_text_input_click_start_0069c7e9;
        g_text_input_anchor_0069c80c = position;
    }
    g_text_input_cursor_visible_0069c80a = 1;
    g_text_input_cursor_0069c80b = position;
    g_text_input_visible_end_0069c80e = Function5D5A10(
        current_field->region.RegionBottomRightX -
            current_field->region.RegionTopLeftX - 10,
        position, current_field->text, &g_text_input_cursor_width_0069c5d8,
        &g_text_input_visible_count_0069c7e4);
}

// FUNCTION: WIZ8 0x005D4F10
void Function5D4F10(MOUSE_REGION* region, int reason)
{
    int field_index = MSYS_GetRegionUserData(region, 0);
    if (IsModalOpen()) return;

    for (W8TextInputField005D39B0* field = g_text_input_first_0069c7f4;
         field != 0; field = field->next) {
        if (field->index == field_index && field->blocks_mouse_callback != 0)
            return;
    }

    if ((reason & MSYS_CALLBACK_REASON_LBUTTON_DOUBLECLICK) != 0) {
        if (g_text_input_current_0069c7fc != 0) Function5D5C40();
        return;
    }

    if ((reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) != 0) {
        W8TextInputField005D39B0* field = g_text_input_current_0069c7fc;
        if (field == 0 || field_index != field->index) return;

        unsigned char position = g_text_input_visible_end_0069c80e;
        if (field->text == 0) {
            position = 0;
        }
        else {
            int mouse_offset = gusMouseXPos - field->region.RegionTopLeftX;
            unsigned int start = g_text_input_visible_end_0069c80e;
            short width = StringPixLengthArg(
                g_text_input_style_0069c7ec->font, 1,
                (unsigned short*)(field->text + start));
            if ((width / 2) / 2 < mouse_offset) {
                int count = 1;
                int previous_width = width / 2;
                do {
                    position = (unsigned char)(position + 1);
                    ++count;
                    width = StringPixLengthArg(
                        g_text_input_style_0069c7ec->font, count,
                        (unsigned short*)(field->text + start));
                    int midpoint = (width - previous_width) / 2 + previous_width;
                    previous_width = width;
                    if (field->length <= position || mouse_offset <= midpoint)
                        break;
                } while (true);
            }
        }
        Function5D5BF0(position);
        g_text_input_click_start_0069c7e9 = g_text_input_cursor_0069c80b;
        Function40BFC0(region);
        return;
    }

    if ((reason & MSYS_CALLBACK_REASON_LBUTTON_UP) == 0) return;
    MSYS_ReleaseMouse(region);

    W8TextInputField005D39B0* clicked = g_text_input_first_0069c7f4;
    if (g_text_input_current_0069c7fc != 0) {
        if (field_index != g_text_input_current_0069c7fc->index)
            Function5D5770(g_text_input_current_0069c7fc);
        clicked = g_text_input_first_0069c7f4;
        if (field_index == g_text_input_current_0069c7fc->index)
            clicked = g_text_input_current_0069c7fc;
    }

    if (clicked != g_text_input_current_0069c7fc) {
        while (clicked != 0 && clicked->index != field_index)
            clicked = clicked->next;
        if (clicked == 0) return;

        W8TextInputField005D39B0* candidate = g_text_input_first_0069c7f4;
        while (candidate != 0 &&
               (candidate == g_text_input_current_0069c7fc ||
                candidate->index != clicked->index ||
                candidate->region_enabled == 0)) {
            candidate = candidate->next;
        }
        if (candidate == 0) return;

        g_text_input_current_0069c7fc = candidate;
        if (candidate->text == 0) {
            g_text_input_cursor_visible_0069c80a = 0;
            g_flag_69c808 = 0;
            if (candidate->callback != 0)
                candidate->callback(candidate->index, 1);
            return;
        }
        g_text_input_anchor_0069c80c = 0;
        g_text_input_end_0069c80d = candidate->length;
        g_text_input_cursor_0069c80b = candidate->length;
        Function5D5BF0(g_text_input_cursor_0069c80b);
        g_text_input_cursor_0069c80b = candidate->length;
        g_text_input_cursor_visible_0069c80a = 1;
        g_flag_69c808 = 1;
        return;
    }

    unsigned char position = g_text_input_visible_end_0069c80e;
    if (g_flag_6f04ed != 0) {
        if (g_text_input_current_0069c7fc->text == 0) {
            position = 0;
        }
        else {
            W8TextInputField005D39B0* field = g_text_input_current_0069c7fc;
            int mouse_offset = gusMouseXPos - field->region.RegionTopLeftX;
            unsigned int start = g_text_input_visible_end_0069c80e;
            short width = StringPixLengthArg(
                g_text_input_style_0069c7ec->font, 1,
                (unsigned short*)(field->text + start));
            if ((width / 2) / 2 < mouse_offset) {
                int count = 1;
                int previous_width = width / 2;
                do {
                    if (field->length <= position) break;
                    position = (unsigned char)(position + 1);
                    ++count;
                    width = StringPixLengthArg(
                        g_text_input_style_0069c7ec->font, count,
                        (unsigned short*)(field->text + start));
                    int midpoint = (width - previous_width) / 2 + previous_width;
                    previous_width = width;
                    if (mouse_offset <= midpoint) break;
                } while (true);
            }
        }
        if (position == g_text_input_click_start_0069c7e9)
            g_text_input_cursor_visible_0069c80a = 0;
        Function5D5BF0(position);
    }
}

// FUNCTION: WIZ8 0x005D52C0
void Function5D52C0(W8TextInputField005D39B0* field)
{
    W8TextInputStyle005D35E0* style = g_text_input_style_0069c7ec;
    int left = field->region.RegionTopLeftX;
    int top = field->region.RegionTopLeftY;
    int right = field->region.RegionBottomRightX;
    int bottom = field->region.RegionBottomRightY;

    if (style->flag_09 != 0) {
        FillSurfaceRect(-14, left, top, right, bottom, style->colour_0c);
        FillSurfaceRect(-14, left + 1, top + 1, right, bottom, style->colour_0a);
    }

    unsigned short colour;
    if (field->region_enabled == 0 && style->flag_10 == 0)
        colour = style->disabled_colour;
    else
        colour = style->foreground;
    if (field->flag_60 != 0 && field != g_text_input_current_0069c7fc)
        colour = style->colour_16;

    FillSurfaceRect(-14, left, top, right, bottom, colour);
    MarkScreenRectDirty(left, top, right, bottom, 0);
}

// FUNCTION: WIZ8 0x005D5390
void Function5D5390(void)
{
    W8TextInputField005D39B0* field = g_text_input_current_0069c7fc;
    if (field == 0 || field->text == 0) return;

    if (g_flag_6f04ed != 0) {
        if ((int)gusMouseXPos < field->region.RegionTopLeftX) {
            if (g_text_input_cursor_0069c80b != 0) {
                --g_text_input_cursor_0069c80b;
                g_text_input_visible_end_0069c80e = Function5D5A10(
                    field->region.RegionBottomRightX -
                        field->region.RegionTopLeftX - 10,
                    g_text_input_cursor_0069c80b, field->text,
                    &g_text_input_cursor_width_0069c5d8,
                    &g_text_input_visible_count_0069c7e4);
            }
            if (g_text_input_cursor_visible_0069c80a != 0)
                g_text_input_anchor_0069c80c =
                    g_text_input_visible_start_0069c80f;
        }
        else if (field->region.RegionBottomRightX < (int)gusMouseXPos) {
            if (g_text_input_cursor_0069c80b < field->length) {
                ++g_text_input_cursor_0069c80b;
                g_text_input_visible_end_0069c80e = Function5D5A10(
                    field->region.RegionBottomRightX -
                        field->region.RegionTopLeftX - 10,
                    g_text_input_cursor_0069c80b, field->text,
                    &g_text_input_cursor_width_0069c5d8,
                    &g_text_input_visible_count_0069c7e4);
            }
            if (g_text_input_cursor_visible_0069c80a != 0)
                g_text_input_end_0069c80d =
                    (unsigned char)(g_text_input_visible_count_0069c7e4 +
                                    g_text_input_visible_start_0069c80f);
        }
    }

    SaveFontSettings();
    SetValue5FF5F0(g_text_input_style_0069c7ec->font);
    unsigned short font_height =
        GetFontHeight(g_text_input_style_0069c7ec->font);
    unsigned int vertical_offset =
        (field->region.RegionBottomRightY - field->region.RegionTopLeftY -
         font_height) / 2;
    Function5D52C0(field);

    wchar_t escaped[256];
    wchar_t visible[512];
    int escaped_length = 0;
    for (const wchar_t* source = field->text; *source != L'\0'; ++source) {
        if (*source == L'%') escaped[escaped_length++] = L'%';
        escaped[escaped_length++] = *source;
    }
    escaped[escaped_length] = L'\0';
    wcscpy(visible, escaped + g_text_input_visible_end_0069c80e);

    bool has_selection =
        g_text_input_cursor_visible_0069c80a != 0 &&
        g_text_input_anchor_0069c80c != g_text_input_end_0069c80d;
    unsigned char selection_first = g_text_input_end_0069c80d;
    unsigned char selection_last = g_text_input_anchor_0069c80c;
    if (g_text_input_anchor_0069c80c < g_text_input_end_0069c80d) {
        selection_first = g_text_input_anchor_0069c80c;
        selection_last = g_text_input_end_0069c80d;
    }

    for (size_t index = 0; index < g_text_input_visible_count_0069c7e4;
         ++index) {
        short prefix = StringPixLengthArg(
            g_text_input_style_0069c7ec->font, index,
            (unsigned short*)visible);
        if (has_selection &&
            (int)(selection_first - g_text_input_visible_end_0069c80e) <=
                (int)index &&
            (int)index <
                (int)(selection_last - g_text_input_visible_end_0069c80e)) {
            SetFontForeground(g_text_input_style_0069c7ec->value_06);
            SetFontBackground(g_text_input_style_0069c7ec->value_07);
            SetFontShadow(g_text_input_style_0069c7ec->value_08);
        }
        else {
            SetFontForeground(g_text_input_style_0069c7ec->value_04);
            SetFontBackground(g_text_input_style_0069c7ec->value_05);
            SetFontShadow(0);
        }
        if (visible[index] == L'%') {
            mprintf(field->region.RegionTopLeftX + prefix + 3,
                    field->region.RegionTopLeftY + vertical_offset, L"%%");
        }
        else {
            mprintf(field->region.RegionTopLeftX + prefix + 3,
                    field->region.RegionTopLeftY + vertical_offset, L"%c",
                    visible[index]);
        }
    }

    if (g_flag_69c808 != 0 && field->text != 0 && g_flag_6f04ed == 0 &&
        GetTickCount() % 1000 < 500) {
        int left = field->region.RegionTopLeftX +
                   g_text_input_cursor_width_0069c5d8;
        int top = field->region.RegionTopLeftY + vertical_offset;
        FillSurfaceRect(-14, left, top, left + 1, top + font_height,
                        g_text_input_style_0069c7ec->colour_0e);
    }
    RestoreFontSettings();
}

// FUNCTION: WIZ8 0x005D5770
void Function5D5770(W8TextInputField005D39B0* field)
{
    if (field == 0 || field->text == 0) return;

    SaveFontSettings();
    SetValue5FF5F0(g_text_input_style_0069c7ec->font);
    bool disabled = field->region_enabled == 0 &&
                    g_text_input_style_0069c7ec->flag_10 != 0;
    if (disabled) {
        SetFontForeground(g_text_input_style_0069c7ec->disabled_foreground);
        SetFontBackground(g_text_input_style_0069c7ec->disabled_background);
    }
    else {
        SetFontForeground(g_text_input_style_0069c7ec->value_04);
        SetFontBackground(g_text_input_style_0069c7ec->value_05);
    }
    unsigned short font_height =
        GetFontHeight(g_text_input_style_0069c7ec->font);
    unsigned int vertical_offset =
        (field->region.RegionBottomRightY - field->region.RegionTopLeftY -
         font_height) / 2;
    SetFontShadow(0);
    Function5D52C0(field);

    wchar_t escaped[256];
    int escaped_length = 0;
    for (const wchar_t* source = field->text; *source != L'\0'; ++source) {
        if (*source == L'%') escaped[escaped_length++] = L'%';
        escaped[escaped_length++] = *source;
    }
    escaped[escaped_length] = L'\0';

    for (size_t index = 0; index < wcslen(escaped); ++index) {
        short prefix = StringPixLengthArg(
            g_text_input_style_0069c7ec->font, index,
            (unsigned short*)escaped);
        if (field->region.RegionBottomRightX -
                field->region.RegionTopLeftX - 10 < prefix + 3) {
            break;
        }
        if (escaped[index] == L'%') {
            mprintf(field->region.RegionTopLeftX + prefix + 3,
                    field->region.RegionTopLeftY + vertical_offset, L"%%");
        }
        else {
            mprintf(field->region.RegionTopLeftX + prefix + 3,
                    field->region.RegionTopLeftY + vertical_offset, L"%c",
                    escaped[index]);
        }
    }
    RestoreFontSettings();

    if (disabled) {
        int rectangle[4] = {
            field->region.RegionTopLeftX,
            field->region.RegionTopLeftY,
            field->region.RegionBottomRightX,
            field->region.RegionBottomRightY,
        };
        unsigned int pitch;
        void* pixels = Function402B90(-14, &pitch);
        Function4124A0(pixels, pitch, rectangle);
        Function402C30(-14);
    }
}

// FUNCTION: WIZ8 0x005D59A0
void Function5D59A0(void)
{
    for (W8TextInputSession005D3520* session =
             g_text_input_sessions_0069c7f0;
         session != 0; session = session->previous) {
        for (W8TextInputField005D39B0* field = session->first;
             field != 0; field = field->next) {
            Function5D5770(field);
        }
    }
    for (W8TextInputField005D39B0* field = g_text_input_first_0069c7f4;
         field != 0; field = field->next) {
        if (field == g_text_input_current_0069c7fc)
            Function5D5390();
        else
            Function5D5770(field);
    }
}

// FUNCTION: WIZ8 0x005D5A00
unsigned char GetFlag69C808(void)
{
    return g_flag_69c808;
}

// FUNCTION: WIZ8 0x005D5A10
unsigned int Function5D5A10(int width, int cursor, const wchar_t* text,
                           int* cursor_width, size_t* visible_count)
{
    wchar_t buffer[512];
    if (cursor < g_text_input_visible_start_0069c80f)
        g_text_input_visible_start_0069c80f = (unsigned char)cursor;

    unsigned int start = g_text_input_visible_start_0069c80f;
    wcscpy(buffer, text + start);
    buffer[cursor - start] = L'\0';
    int measured = StringPixLength((unsigned short*)buffer,
                                   g_text_input_style_0069c7ec->font);
    size_t count = wcslen(buffer);
    unsigned char retained_start;

    if (width < measured) {
        wchar_t* suffix = buffer;
        do {
            ++suffix;
            ++start;
            measured = StringPixLength((unsigned short*)suffix,
                                       g_text_input_style_0069c7ec->font);
        } while (width < measured);
        retained_start = (unsigned char)start;

        if (g_text_input_visible_start_0069c80f < start) {
            wcscpy(buffer, text + start);
            size_t length = wcslen(buffer);
            count = length;
            for (size_t index = 0; index < wcslen(buffer); ++index) {
                short prefix = StringPixLengthArg(g_text_input_style_0069c7ec->font,
                                                  index, (unsigned short*)buffer);
                count = index;
                if (width < prefix + 3) break;
                count = length;
            }
        }
    }
    else {
        wcscpy(buffer, text + start);
        size_t length = wcslen(buffer);
        count = length;
        retained_start = g_text_input_visible_start_0069c80f;
        for (size_t index = 0; index < wcslen(buffer); ++index) {
            short prefix = StringPixLengthArg(g_text_input_style_0069c7ec->font,
                                              index, (unsigned short*)buffer);
            retained_start = g_text_input_visible_start_0069c80f;
            count = index;
            if (width < prefix + 3) break;
            count = length;
        }
    }

    g_text_input_visible_start_0069c80f = retained_start;
    *cursor_width = measured + 2;
    *visible_count = count;
    return start;
}

// FUNCTION: WIZ8 0x005D5BF0
void Function5D5BF0(unsigned char cursor)
{
    g_text_input_cursor_0069c80b = cursor;
    if (g_text_input_current_0069c7fc != 0) {
        g_text_input_visible_end_0069c80e = Function5D5A10(
            g_text_input_current_0069c7fc->region.RegionBottomRightX -
                g_text_input_current_0069c7fc->region.RegionTopLeftX - 10,
            cursor, g_text_input_current_0069c7fc->text,
            &g_text_input_cursor_width_0069c5d8,
            &g_text_input_visible_count_0069c7e4);
    }
}

// FUNCTION: WIZ8 0x005D5C40
void Function5D5C40(void)
{
    W8TextInputField005D39B0* field = g_text_input_current_0069c7fc;
    unsigned char position = g_text_input_visible_end_0069c80e;
    if (field->text == 0) {
        position = 0;
    }
    else {
        int mouse_offset = gusMouseXPos - field->region.RegionTopLeftX;
        unsigned int start = g_text_input_visible_end_0069c80e;
        short width = StringPixLengthArg(
            g_text_input_style_0069c7ec->font, 1,
            (unsigned short*)(field->text + start));
        if ((width / 2) / 2 < mouse_offset) {
            int count = 1;
            int previous_width = width / 2;
            do {
                if (field->length <= position) break;
                ++position;
                ++count;
                width = StringPixLengthArg(
                    g_text_input_style_0069c7ec->font, count,
                    (unsigned short*)(field->text + start));
                int midpoint = (width - previous_width) / 2 + previous_width;
                previous_width = width;
                if (mouse_offset <= midpoint) break;
            } while (true);
        }
    }

    if (field->text[position] == L' ') return;
    unsigned char first = 0;
    if (position != 0) {
        unsigned int scan = position;
        const wchar_t* character = field->text + position;
        do {
            if (*character == L' ') {
                first = (unsigned char)(scan + 1);
                break;
            }
            --scan;
            --character;
        } while (scan != 0);
    }

    unsigned char last = (unsigned char)wcslen(field->text);
    for (unsigned int scan = position + 1; scan < wcslen(field->text); ++scan) {
        if (field->text[scan] == L' ') {
            last = (unsigned char)scan;
            break;
        }
    }
    g_text_input_anchor_0069c80c = first;
    g_text_input_end_0069c80d = last;
    g_text_input_cursor_visible_0069c80a = 1;
}
