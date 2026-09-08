#include "wiz8/input_hooks.h"

#include "wiz8/cursor.h"
#include "wiz8/utility.h"
#include "wiz8/wiz8_windows.h"

#include <string.h>

extern unsigned char g_flag_6f0630;
extern unsigned char Function4229B0(void);
extern unsigned char Function4229D0(void);
extern void NoOp(void);
extern HWND g_window_6596cc;

/* The text line editor descriptor the hooks edit through. Only the fields
   Function4023B0 touches are named; the rest stay positional. */
struct W8TextEditState {
    unsigned short* buffer_00;
    int unknown_04;
    unsigned short* allowed_08;
    unsigned short max_0c;
    unsigned short end_0e;
    unsigned short cursor_10;
    unsigned short last_key_12;
    unsigned char insert_14;
    unsigned char active_15;
    unsigned char unknown_16[2];
    W8TextEditState* prev_18;
    W8TextEditState* next_1c;
};

/* Retail 0x006F04E0: the key-event buffer cursor. */
unsigned short g_key_event_index_6f04e0;
/* Retail 0x006F04E2. */
unsigned short g_input_state_6f04e2;
/* Retail 0x006F04E4: double-click delay, 300 ms at startup. */
unsigned int g_double_click_delay_6f04e4;
/* Retail 0x006F04E8: mouse-button-4/5 latch. */
unsigned char g_flag_6f04e8;
/* Retail 0x006F04E9. */
unsigned char g_input_state_6f04e9;
/* Retail 0x006F04EA: shift-key latch. */
unsigned short g_shift_state_6f04ea;
/* Retail 0x006F04EC: mouse-move event gate. */
unsigned char g_input_state_6f04ec;
/* Retail 0x006F04ED: mouse-button-1 latch. */
unsigned char g_flag_6f04ed;
/* Retail 0x006F04F0: button-2 release deadline. */
unsigned int g_input_state_6f04f0;
/* Retail 0x006F04F4. */
unsigned char g_input_state_6f04f4;
/* Retail 0x006F04F6: buffered key-event count. */
unsigned short g_key_event_count_6f04f6;
/* Retail 0x006F04F8: last mouse y. */
unsigned short g_last_mouse_y_6f04f8;
/* Retail 0x006F04FC: the thread keyboard hook. */
HHOOK g_keyboard_hook_6f04fc;
/* Retail 0x006F0500: text-entry active flag. */
unsigned char g_text_entry_active_6f0500;
/* Retail 0x006F0504: last shift-tap tick. */
unsigned int g_input_state_6f0504;
/* Retail 0x006F0508: menu-key latch. */
unsigned short g_menu_state_6f0508;
/* Retail 0x006F050A: last mouse x. */
unsigned short g_last_mouse_x_6f050a;
/* Retail 0x006F050C: the thread mouse hook. */
HHOOK g_mouse_hook_6f050c;
/* Retail 0x006F0510: the active text line editor, or 0. */
struct W8TextEditState* g_text_editor_6f0510;
/* Retail 0x006F0514: queued-event modifiers. */
unsigned short g_queued_modifiers_6f0514;
/* Retail 0x006F0518: button-1 release deadline. */
unsigned int g_input_state_6f0518;
/* Retail 0x006F051C: control-key latch. */
unsigned short g_control_state_6f051c;
/* Retail 0x006F0520: per-virtual-key down flags. */
unsigned char g_key_down_6f0520[0x100];
/* Retail 0x006F0530: shift down flag. */
unsigned char g_shift_down_6f0530;
/* Retail 0x006F0531: menu down flag. */
unsigned char g_menu_down_6f0531;
/* Retail 0x006F0532: control down flag. */
unsigned char g_control_down_6f0532;
/* Retail 0x00650DB9: sticky input-received flag the hooks raise. */
unsigned char g_input_received_650db9;
/* Retail 0x00650DBA: non-client mouse flag the mouse hook maintains. */
unsigned char g_input_flag_650dba;

/* Retail 0x006EF4E0: the 256-entry key-event ring the hooks append to. */
struct W8KeyEvent {
    unsigned int tick;
    unsigned short modifiers;
    short kind;
    unsigned int param_08;
    unsigned int param_0c;
};
W8KeyEvent g_key_events_6ef4e0[256];

/* Retail 0x005FFC3C/0x005FFE3C: shift translation tables the text editor
   reads. FUN_00402780 fills them; until that lands they stay zero. */
unsigned char g_shift_table_5ffc3c[0x100];
unsigned short g_shift_table_5ffe3c[0x100];

/* Retail 0x005FF51C: virtual-key remap for keys 0x21-0x2E without the
   extended flag, transcribed from the image. */
unsigned short g_key_remap_5ff51c[14] = {
    0x0069, 0x0063, 0x0061, 0x0067, 0x0064, 0x0068, 0x0066,
    0x0062, 0x0000, 0x0000, 0x0000, 0x0000, 0x0060, 0x006e
};

void Function401F90(short kind, unsigned int param_08, unsigned int param_0c);
void Function402270(unsigned int key, unsigned int flags, char pressed);
void Function4023B0(unsigned short key);
void Function427A70(RECT* rect);
long __stdcall Function401B30(int code, unsigned int key, long flags);
long __stdcall Function401C70(int code, unsigned int button, long info);

// FUNCTION: WIZ8 0x00401ea0
unsigned char InitializeInputManager00401EA0(void)
{
    DWORD thread;

    memset(g_key_down_6f0520, 0, sizeof(g_key_down_6f0520));
    g_key_event_count_6f04f6 = 0;
    g_input_state_6f04e2 = 0;
    g_key_event_index_6f04e0 = 0;
    g_input_state_6f04ec = 0;
    g_shift_state_6f04ea = 0;
    g_control_state_6f051c = 0;
    g_menu_state_6f0508 = 0;
    g_input_state_6f04f4 = 1;
    g_double_click_delay_6f04e4 = 300;
    g_input_state_6f0504 = 0;
    g_input_state_6f04e9 = 0;
    g_flag_6f04ed = 0;
    g_flag_6f04e8 = 0;
    g_input_state_6f0518 = 0;
    g_input_state_6f04f0 = 0;
    g_last_mouse_x_6f050a = 0x140;
    g_last_mouse_y_6f04f8 = 0xf0;
    g_text_entry_active_6f0500 = 0;
    g_text_editor_6f0510 = 0;
    thread = GetCurrentThreadId();
    g_keyboard_hook_6f04fc = SetWindowsHookExA(
        WH_KEYBOARD, Function401B30, NULL, thread);
    thread = GetCurrentThreadId();
    g_mouse_hook_6f050c = SetWindowsHookExA(
        WH_MOUSE, Function401C70, NULL, thread);
    return 1;
}

// FUNCTION: WIZ8 0x00401b30
long __stdcall Function401B30(int code, unsigned int key, long flags)
{
    if (code < 0 || g_flag_6f0630 == 0) {
        return CallNextHookEx(g_keyboard_hook_6f04fc, code, key, flags);
    }
    if ((flags & 0x80000000) == 0) {
        if (key == 0x10) {
            g_shift_state_6f04ea = 1;
            g_shift_down_6f0530 = 1;
            g_input_received_650db9 = 1;
            return 1;
        }
        if (key == 0x11) {
            g_menu_down_6f0531 = 1;
            g_input_received_650db9 = 1;
            g_menu_state_6f0508 = 2;
            return 1;
        }
        if (key == 0x12) {
            g_control_down_6f0532 = 1;
            g_input_received_650db9 = 1;
            g_control_state_6f051c = 4;
            return 1;
        }
        if (key != 0x2c) {
            Function402270(key, flags, 1);
        }
        g_input_received_650db9 = 1;
        return 1;
    }
    if (key == 0x10) {
        g_shift_state_6f04ea = 0;
        g_shift_down_6f0530 = 0;
        return 1;
    }
    if (key == 0x11) {
        g_menu_state_6f0508 = 0;
        g_menu_down_6f0531 = 0;
        return 1;
    }
    if (key == 0x12) {
        g_control_state_6f051c = 0;
        g_control_down_6f0532 = 0;
        return 1;
    }
    if (key == 0x2c) {
        if (g_menu_down_6f0531 != 0) {
            NoOp();
            return 1;
        }
        Function4229D0();
        return 1;
    }
    Function402270(key, flags, 0);
    return 1;
}

// FUNCTION: WIZ8 0x00401c70
long __stdcall Function401C70(int code, unsigned int button, long info)
{
    MOUSEHOOKSTRUCT* state = (MOUSEHOOKSTRUCT*)info;
    unsigned int x = state->pt.x;
    unsigned int y = state->pt.y;
    unsigned int packed;
    RECT client;
    unsigned char outside;

    outside = 0;
    if (Function4229B0() == 0) {
        if (button == 0xa1) {
            g_input_flag_650dba = 1;
        }
        Function427A70(&client);
        if (x < (unsigned int)client.left || (unsigned int)client.right < x ||
            y < (unsigned int)client.top || (unsigned int)client.bottom < y) {
            outside = 1;
        }
    }
    if (code >= 0 && g_flag_6f0630 != 0 && !outside &&
        g_input_flag_650dba == 0) {
        switch (button - 0x200) {
        case 0:
        case 1:
        case 2:
        case 4:
        case 5:
            if (Function4229B0() == 0) {
                x -= client.left;
                y -= client.top;
                g_last_mouse_x_6f050a = (unsigned short)x;
            }
            else {
                g_last_mouse_x_6f050a = (unsigned short)x;
            }
            g_last_mouse_y_6f04f8 = (unsigned short)y;
            packed = (y << 0x10) | (x & 0xffff);
            g_input_received_650db9 = 1;
            break;
        default:
            packed = button;
            break;
        }
        if (button == 0x20a) {
            return 0;
        }
        switch (button - 0x200) {
        case 0:
            if (g_input_state_6f04ec != 0) {
                Function401F90(0x400, 0, packed);
            }
            break;
        case 1:
            g_flag_6f04ed = 1;
            Function401F90(8, 0, packed);
            return 1;
        case 2:
            g_flag_6f04ed = 0;
            Function401F90(0x10, 0, packed);
            return 1;
        case 4:
            g_flag_6f04e8 = 1;
            Function401F90(0x80, 0, packed);
            return 1;
        case 5:
            g_flag_6f04e8 = 0;
            Function401F90(0x100, 0, packed);
            return 1;
        }
        return 1;
    }
    LRESULT next = CallNextHookEx(g_mouse_hook_6f050c, code, button, info);
    if (button == 0x202 || button == 0xa2) {
        g_input_flag_650dba = 0;
    }
    return next;
}

/* Queues one key event, doubling button presses that land within the
   double-click window. */
// FUNCTION: WIZ8 0x00401f90
void Function401F90(short kind, unsigned int param_08, unsigned int param_0c)
{
    unsigned int tick = GetTickCount();
    unsigned short modifiers = (unsigned short)(
        g_control_state_6f051c | g_menu_state_6f0508 | g_shift_state_6f04ea);
    unsigned int index;

    if (g_key_event_count_6f04f6 == 0x100) {
        return;
    }
    if (kind == 8) {
        g_input_state_6f0518 = tick + 0xfa;
    }
    else if (kind == 0x80) {
        g_input_state_6f04f0 = tick + 0xfa;
    }
    else if (kind == 0x10) {
        unsigned int delta = tick - g_input_state_6f0504;
        g_input_state_6f0518 = 0;
        g_input_state_6f0504 = tick;
        if (delta < 300) {
            unsigned int next;
            g_input_state_6f0504 = 0;
            index = g_key_event_index_6f04e0;
            g_key_events_6ef4e0[index].tick = tick;
            g_key_events_6ef4e0[index].modifiers = g_queued_modifiers_6f0514;
            g_key_events_6ef4e0[index].kind = 0x10;
            g_key_events_6ef4e0[index].param_08 = param_08;
            g_key_events_6ef4e0[index].param_0c = param_0c;
            if (index == 0xff) {
                next = 0;
            }
            else {
                next = index + 1;
            }
            g_key_events_6ef4e0[next].tick = tick;
            g_key_events_6ef4e0[next].modifiers = g_queued_modifiers_6f0514;
            g_key_events_6ef4e0[next].kind = 0x20;
            g_key_events_6ef4e0[next].param_08 = param_08;
            g_key_events_6ef4e0[next].param_0c = param_0c;
            g_key_event_count_6f04f6 += 2;
            if (next != 0xff) {
                g_key_event_index_6f04e0 = (unsigned short)(next + 1);
                return;
            }
            g_key_event_index_6f04e0 = 0;
            return;
        }
    }
    else if (kind == 0x100) {
        g_input_state_6f04f0 = 0;
    }
    index = g_key_event_index_6f04e0;
    g_key_event_count_6f04f6 += 1;
    g_key_events_6ef4e0[index].tick = tick;
    g_key_events_6ef4e0[index].modifiers = modifiers;
    g_key_events_6ef4e0[index].kind = kind;
    g_key_events_6ef4e0[index].param_08 = param_08;
    g_key_events_6ef4e0[index].param_0c = param_0c;
    if (index == 0xff) {
        g_key_event_index_6f04e0 = 0;
        return;
    }
    g_key_event_index_6f04e0 = (unsigned short)(index + 1);
}

// FUNCTION: WIZ8 0x00402270
void Function402270(unsigned int key, unsigned int flags, char pressed)
{
    W8ScreenPoint point;
    unsigned int packed;
    unsigned int code;

    if (key == 0x0c) {
        key = 0x65;
    }
    else if (key < 0x2f && key > 0x20 && (flags & 0x1000000) == 0) {
        key = g_key_remap_5ff51c[key - 0x21];
    }
    else if (key == 0x0d && (flags & 0x1000000) != 0) {
        key = 0x6c;
    }
    GetScreenPoint004284F0(&point);
    packed = ((unsigned int)point.y << 0x10) | ((unsigned int)point.x & 0xffff);
    if (pressed == 1) {
        code = key & 0xffff;
        if (g_key_down_6f0520[code] == 0) {
            if (g_text_entry_active_6f0500 == 0) {
                g_key_down_6f0520[code] = 1;
                Function401F90(1, code, packed);
                return;
            }
        }
        else if (g_text_entry_active_6f0500 == 0) {
            Function401F90(4, code, packed);
            return;
        }
        Function4023B0((unsigned short)key);
        return;
    }
    code = key & 0xffff;
    if (g_key_down_6f0520[code] == 1) {
        g_key_down_6f0520[code] = 0;
        Function401F90(2, code, packed);
        return;
    }
    if ((short)key == 9 && g_control_state_6f051c != 0) {
        ShowWindow(g_window_6596cc, 6);
        g_control_down_6f0532 = 0;
        g_control_state_6f051c = 0;
    }
}

// FUNCTION: WIZ8 0x004023b0
void Function4023B0(unsigned short key)
{
    W8TextEditState* editor = g_text_editor_6f0510;
    unsigned int code = key;
    unsigned short current;

    if (editor == 0) {
        return;
    }
    switch (code) {
    case 8:
        current = editor->cursor_10;
        if (current == 0) {
            return;
        }
        if (current <= editor->end_0e) {
            do {
                code = current;
                ++current;
                editor->buffer_00[code - 1] = editor->buffer_00[code];
            } while (current <= editor->end_0e);
        }
        --editor->cursor_10;
        --editor->end_0e;
        return;
    case 9:
        if (g_shift_state_6f04ea == 0) {
            if (editor->next_1c == 0) {
                return;
            }
            editor->active_15 = 0;
            goto next_editor;
        }
        if (editor->prev_18 == 0) {
            return;
        }
        editor->active_15 = 0;
        editor = editor->prev_18;
        goto activate_editor;
    default:
        current = (unsigned short)(
            g_control_state_6f051c | g_menu_state_6f0508 | g_shift_state_6f04ea);
        if ((current & 6) != 0) {
            return;
        }
        if ((current & 1) == 0) {
            key = g_shift_table_5ffc3c[code];
        }
        else {
            key = g_shift_table_5ffe3c[code];
        }
        if (key == 0) {
            return;
        }
        if (editor->allowed_08 != 0) {
            unsigned int allowed = 1;
            current = *editor->allowed_08;
            if (current == 0) {
                return;
            }
            while (key != editor->allowed_08[allowed]) {
                ++allowed;
                if (current < allowed) {
                    return;
                }
            }
        }
        if (editor->insert_14 == 1) {
            current = editor->end_0e;
            if ((int)(editor->max_0c - 1) <= (int)(unsigned int)current) {
                break;
            }
            if (editor->cursor_10 < current) {
                do {
                    code = current;
                    --current;
                    editor->buffer_00[code] = editor->buffer_00[code - 1];
                } while (editor->cursor_10 < current);
            }
            editor->buffer_00[current] = key;
            ++editor->cursor_10;
        }
        else {
            if ((int)(unsigned int)editor->cursor_10 <
                (int)(editor->max_0c - 1)) {
                editor->buffer_00[editor->cursor_10] = key;
                ++editor->cursor_10;
            }
            if (editor->cursor_10 <= editor->end_0e) {
                break;
            }
            editor->buffer_00[editor->cursor_10] = 0;
        }
        ++editor->end_0e;
        break;
    case 0x0d:
        editor->active_15 = 0;
        if (editor->next_1c == 0) {
            editor->last_key_12 = key;
            g_text_entry_active_6f0500 = 0;
            return;
        }
next_editor:
        editor = editor->next_1c;
        editor->active_15 = 1;
        editor->last_key_12 = 0;
        return;
    case 0x1b:
        editor->active_15 = 0;
        editor->last_key_12 = key;
        g_text_entry_active_6f0500 = 0;
        return;
    case 0x23:
        editor->cursor_10 = editor->end_0e;
        editor->last_key_12 = key;
        return;
    case 0x24:
        editor->cursor_10 = 0;
        editor->last_key_12 = key;
        return;
    case 0x25:
        if (editor->cursor_10 != 0) {
            --editor->cursor_10;
            editor->last_key_12 = key;
            return;
        }
        break;
    case 0x26:
        if (editor->prev_18 == 0) {
            return;
        }
        editor->active_15 = 0;
        editor = editor->prev_18;
activate_editor:
        editor->active_15 = 1;
        editor->last_key_12 = 0;
        return;
    case 0x27:
        if (editor->cursor_10 < editor->end_0e) {
            ++editor->cursor_10;
            editor->last_key_12 = key;
            return;
        }
        break;
    case 0x28:
        if (editor->next_1c == 0) {
            return;
        }
        editor->active_15 = 0;
        editor = editor->next_1c;
        editor->active_15 = 1;
        editor->last_key_12 = 0;
        return;
    case 0x2d:
        editor->insert_14 = editor->insert_14 != 1;
        editor->last_key_12 = key;
        return;
    case 0x2e:
        current = editor->cursor_10;
        if (current < editor->end_0e) {
            do {
                code = current;
                ++current;
                editor->buffer_00[code] = editor->buffer_00[code + 1];
            } while (current < editor->end_0e);
            --editor->end_0e;
            editor->last_key_12 = key;
            return;
        }
        break;
    }
    editor->last_key_12 = key;
}

// FUNCTION: WIZ8 0x004027a70
void Function427A70(RECT* rect)
{
    GetClientRect(g_window_6596cc, rect);
    ClientToScreen(g_window_6596cc, (POINT*)rect);
    ClientToScreen(g_window_6596cc, (POINT*)&rect->right);
}
