#include "surrender/srWindow.h"
#include "surrender/srWindowOut.h"

#include <fstream>
#include <stdio.h>
#include <windows.h>

#include <commdlg.h>
#include <richedit.h>

/* Internal stream buffer owning the "srDebugWndClass" diagnostic console: a
   frame window with a RichEdit child, a File menu and a periodic timer.
   Retail keeps the class translation-unit local - nothing exports it and no
   other code references it. */
class srWindowOutStreamBuf : public std::basic_streambuf<char, std::char_traits<char> > {
public:
    srWindowOutStreamBuf(unsigned long instance, unsigned long parent, const char* title,
                         long width, unsigned long height);
    virtual ~srWindowOutStreamBuf();

private:
    /* Pending output text: a raw buffer grown in 8-byte steps on every
       bounds-checked write. '\n' flushes the whole buffer into the RichEdit;
       a trailing partial line stays pending until the next newline, Clear or
       Save. */
    struct Pending {
        char* text;             /* 0x88 */
        unsigned long capacity; /* 0x8c */

        /* 0x10047C80 */
        void reserve(unsigned long size);
        char* c_str()
        {
            if (capacity == 0)
                reserve(8);
            return text;
        }
    };

    /* The "srDebugWndClass" window procedure; the constructor stores this
       object's pointer in GWL_USERDATA. */
    /* 0x10046870 */
    static long __stdcall windowProc(HWND window, unsigned int message, WPARAM wparam,
                                     LPARAM lparam);
    /* 0x10046810 */
    static void __stdcall timerProc(HWND window, unsigned int message, unsigned int timer,
                                    unsigned long time);
    /* 0x10046990 */
    static unsigned long __stdcall streamOutCallback(unsigned long stream, LPBYTE buffer,
                                                     long count, long* written);

    /* 0x10047670 */
    virtual int overflow(int ch);
    /* 0x10047680 */
    virtual int underflow();
    /* 0x10047640 */
    virtual int sync();

    /* 0x10046EC0 - bounds-checked append into the pending buffer; '\n' and
       '\t' flush/expand instead of storing the byte. */
    void emit(int ch);
    /* 0x10046E90 - scroll the caret into view after text was appended. */
    void scrollCaret();
    /* 0x10047790 - WM_DESTROY teardown; also makes emit() a no-op. */
    void closeWindow();
    /* 0x10046D10 - "&Clear" (wID 1): reset pending text and the RichEdit. */
    void clearText();
    /* 0x100469C0 - "&Save" (wID 2): GetSaveFileName + EM_STREAMOUT into a
       new std::ofstream. */
    void saveText();

    /* Inline companion of Pending::reserve: grow-then-store one character. */
    void put(unsigned long index, char ch)
    {
        if (pending_88.capacity <= index)
            pending_88.reserve(pending_88.capacity + 8 + index);
        pending_88.text[index] = ch;
    }

    unsigned long disabled_38;      /* 0x38: emit()/clear/save gate */
    WNDCLASSA window_class_3c;      /* 0x3c */
    HINSTANCE instance_64;          /* 0x64 */
    HWND parent_68;                 /* 0x68 */
    HWND edit_window_6c;            /* 0x6c */
    HWND frame_window_70;           /* 0x70 */
    unsigned long field_74;         /* 0x74 */
    HFONT font_78;                  /* 0x78 */
    HMENU menu_7c;                  /* 0x7c */
    HMENU file_menu_80;             /* 0x80 */
    HMODULE riched_module_84;       /* 0x84 */
    Pending pending_88;             /* 0x88 */
    unsigned long length_90;        /* 0x90: used bytes in pending_88 */
    unsigned long scroll_94;        /* 0x94: caret scroll needed */
    char* line_buffer_98;           /* 0x98: allocated but unused */
    unsigned long buffer_size_9c;   /* 0x9c */
    unsigned long font_height_a0;   /* 0xa0 */
};

// GLOBAL: SURRENDER 0x100A49B0
// Unique suffix counter for the "srDebugWndClass<i>" window-class names.
static int class_counter_100a49b0;

// FUNCTION: SURRENDER 0x10047C80
void srWindowOutStreamBuf::Pending::reserve(unsigned long size)
{
    if (capacity == size)
        return;
    char* next = 0;
    if (size != 0) {
        next = new char[size];
        if (text != 0 && capacity != 0) {
            unsigned long copy = size <= capacity ? size : capacity;
            for (unsigned long index = 0; index < copy; ++index)
                next[index] = text[index];
        }
    }
    delete[] text;
    text = next;
    capacity = size;
}

// FUNCTION: SURRENDER 0x10046810
void __stdcall srWindowOutStreamBuf::timerProc(HWND window, unsigned int message,
                                               unsigned int timer, unsigned long time)
{
    if (message != WM_TIMER || timer != 0x1ce7ea)
        return;
    srWindowOutStreamBuf* self =
        reinterpret_cast<srWindowOutStreamBuf*>(  // reinterpret-ok: GWL_USERDATA
            // stores this object's pointer as a raw LONG across the Win32 ABI.
            GetWindowLongA(window, GWL_USERDATA));
    if (self != 0 && self->scroll_94 != 0)
        self->scrollCaret();
}

// FUNCTION: SURRENDER 0x10046870
long __stdcall srWindowOutStreamBuf::windowProc(HWND window, unsigned int message,
                                                WPARAM wparam, LPARAM lparam)
{
    srWindowOutStreamBuf* self =
        reinterpret_cast<srWindowOutStreamBuf*>(  // reinterpret-ok: GWL_USERDATA
            // stores this object's pointer as a raw LONG across the Win32 ABI.
            GetWindowLongA(window, GWL_USERDATA));
    if (message < 0x15) {
        if (message == WM_ERASEBKGND)
            return 1;
        if (message == WM_DESTROY) {
            if (self != 0) {
                self->closeWindow();
                return DefWindowProcA(window, message, wparam, lparam);
            }
        } else if (message == WM_SIZE && self != 0 && self->edit_window_6c != 0) {
            MoveWindow(self->edit_window_6c, 0, 0, lparam & 0xffff,
                       (unsigned long)lparam >> 0x10, 1);
            return DefWindowProcA(window, message, wparam, lparam);
        }
    } else if (message == WM_KEYDOWN) {
        if (wparam == VK_ESCAPE && self != 0)
            SetFocus(self->parent_68);
    } else if (message == WM_COMMAND) {
        if ((short)wparam == 2) {
            if (self != 0) {
                self->saveText();
                return DefWindowProcA(window, message, wparam, lparam);
            }
        } else if ((short)wparam == 1 && self != 0) {
            self->clearText();
            return DefWindowProcA(window, message, wparam, lparam);
        }
    }
    return DefWindowProcA(window, message, wparam, lparam);
}

// FUNCTION: SURRENDER 0x10046990
unsigned long __stdcall srWindowOutStreamBuf::streamOutCallback(unsigned long stream,
                                                                LPBYTE buffer, long count,
                                                                long* written)
{
    if (count != 0) {
        std::ofstream* output =
            reinterpret_cast<std::ofstream*>(  // reinterpret-ok: the EDITSTREAM
                // cookie carries the ofstream across the Win32 callback ABI.
                stream);
        output->write(reinterpret_cast<const char*>(  // reinterpret-ok: the
                          // Win32 callback delivers the text as raw bytes.
                          buffer),
                      count);
        *written = count;
        return 0;
    }
    return 0xffffffff;
}

// FUNCTION: SURRENDER 0x10046D10
void srWindowOutStreamBuf::clearText()
{
    if (disabled_38 != 0)
        return;
    pending_88.reserve(1);
    *pending_88.c_str() = 0;
    length_90 = 0;
    SendMessageA(edit_window_6c, WM_SETTEXT, 0, (LPARAM)pending_88.c_str());
}

// FUNCTION: SURRENDER 0x10046E90
void srWindowOutStreamBuf::scrollCaret()
{
    SendMessageA(edit_window_6c, EM_SCROLLCARET, 0, 0);
    scroll_94 = 0;
}

// FUNCTION: SURRENDER 0x10046EC0
void srWindowOutStreamBuf::emit(int ch)
{
    if (disabled_38 != 0)
        return;
    if ((char)ch == '\n') {
        scroll_94 = 1;
        SendMessageA(edit_window_6c, EM_SETSEL, (WPARAM)-1, -1);
        unsigned long start;
        unsigned long end;
        SendMessageA(edit_window_6c, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
        SendMessageA(edit_window_6c, EM_SETSEL, start + end, start + end);
        put(length_90++, '\r');
        put(length_90++, '\n');
        put(length_90, 0);
        SendMessageA(edit_window_6c, EM_REPLACESEL, 0, (LPARAM)pending_88.c_str());
        length_90 = 0;
        return;
    }
    if ((char)ch == '\t') {
        for (int index = 0; index < 4; ++index)
            put(length_90++, ' ');
        put(length_90, 0);
        return;
    }
    put(length_90++, (char)ch);
    put(length_90, 0);
}

// FUNCTION: SURRENDER 0x100469C0
void srWindowOutStreamBuf::saveText()
{
    if (disabled_38 != 0)
        return;
    OPENFILENAMEA info;
    char file[300];
    for (unsigned long index = 0; index < 300; ++index)
        file[index] = 0;
    info.hwndOwner = frame_window_70;
    info.hInstance = instance_64;
    info.lpstrFile = file;
    info.lStructSize = sizeof(info);
    info.lpstrFilter = 0;
    info.lpstrCustomFilter = 0;
    info.nMaxCustFilter = 0;
    info.nFilterIndex = 0;
    info.nMaxFile = 299;
    info.lpstrFileTitle = 0;
    /* Retail never writes nMaxFileTitle; the field stays uninitialized. */
    info.lpstrInitialDir = 0;
    info.lpstrTitle = 0;
    info.Flags = OFN_OVERWRITEPROMPT;
    info.nFileOffset = 0;
    info.lpstrDefExt = 0;
    info.nFileExtension = 0;
    info.lCustData = 0;
    info.lpfnHook = 0;
    info.lpTemplateName = 0;
    if (!GetSaveFileNameA(&info))
        return;
    std::ofstream* stream = new std::ofstream(file);
    if (scroll_94 == 0 && length_90 != 0) {
        SendMessageA(edit_window_6c, EM_SETSEL, (WPARAM)-1, -1);
        unsigned long start;
        unsigned long end;
        SendMessageA(edit_window_6c, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
        SendMessageA(edit_window_6c, EM_SETSEL, start + end, start + end + 1);
        SendMessageA(edit_window_6c, EM_REPLACESEL, 0, (LPARAM)pending_88.c_str());
        length_90 = 0;
        scroll_94 = 1;
    }
    EDITSTREAM output;
    output.dwCookie = reinterpret_cast<unsigned long>(  // reinterpret-ok: the
        // Win32 callback ABI carries the ofstream as a raw cookie.
        stream);
    output.dwError = 0;
    output.pfnCallback = streamOutCallback;
    SendMessageA(edit_window_6c, EM_STREAMOUT, SF_TEXT, (LPARAM)&output);
    delete stream;
}

// FUNCTION: SURRENDER 0x10047790
void srWindowOutStreamBuf::closeWindow()
{
    disabled_38 = 1;
    if (frame_window_70 == 0)
        return;
    if (srWindow::isWindow((unsigned long)frame_window_70)) {
        SetMenu(frame_window_70, 0);
        DestroyMenu(file_menu_80);
        DestroyMenu(menu_7c);
        KillTimer(frame_window_70, 0x1ce7ea);
        SetWindowLongA(frame_window_70, GWL_USERDATA, 0);
        file_menu_80 = 0;
        menu_7c = 0;
    }
    frame_window_70 = 0;
}

// FUNCTION: SURRENDER 0x10047150
srWindowOutStreamBuf::srWindowOutStreamBuf(unsigned long instance, unsigned long parent,
                                           const char* title, long width, unsigned long height)
{
    pending_88.text = 0;
    pending_88.capacity = 0;
    buffer_size_9c = 1;
    length_90 = 0;
    disabled_38 = 1;
    scroll_94 = 1;
    line_buffer_98 = new char;
    edit_window_6c = 0;
    frame_window_70 = 0;
    riched_module_84 = 0;
    font_78 = 0;
    instance_64 = (HINSTANCE)instance;
    parent_68 = (HWND)parent;

    char class_name[256];
    sprintf(class_name, "%s%i", "srDebugWndClass", class_counter_100a49b0++);

    window_class_3c.style = 0xb;
    window_class_3c.lpfnWndProc = windowProc;
    window_class_3c.cbClsExtra = 0;
    window_class_3c.cbWndExtra = 0;
    window_class_3c.hInstance = (HINSTANCE)instance;
    window_class_3c.hIcon = LoadIconA(0, (const char*)0x7f00);
    window_class_3c.hCursor = LoadCursorA(0, (const char*)0x7f00);
    window_class_3c.hbrBackground = (HBRUSH)GetStockObject(4);
    window_class_3c.lpszMenuName = 0;
    window_class_3c.lpszClassName = class_name;
    if (RegisterClassA(&window_class_3c) == 0)
        return;

    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = 400;
    rect.bottom = 400;
    AdjustWindowRect(&rect, WS_VISIBLE | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX |
                                WS_MAXIMIZEBOX,
                     0);
    rect.right -= rect.left;
    rect.bottom -= rect.top;
    rect.left = 0;
    rect.top = 0;
    rect.right += GetSystemMetrics(SM_CXSCREEN) - rect.right;

    riched_module_84 = LoadLibraryA("riched32.dll");
    if (riched_module_84 == 0)
        return;

    frame_window_70 = CreateWindowExA(0, class_name, title,
                                      WS_VISIBLE | WS_CAPTION | WS_THICKFRAME |
                                          WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
                                      rect.left, rect.top, rect.right - rect.left,
                                      rect.bottom - rect.top, (HWND)parent, 0,
                                      (HINSTANCE)instance, 0);
    SetWindowLongA(frame_window_70, GWL_USERDATA, (long)this);
    edit_window_6c =
        CreateWindowExA(0, "RichEdit", "",
                        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_READONLY |
                            ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE,
                        0, 0, 400, 400, frame_window_70, 0, (HINSTANCE)instance, 0);
    RECT client;
    GetClientRect(frame_window_70, &client);
    MoveWindow(edit_window_6c, 0, 0, client.right - client.left, client.bottom - client.top,
               1);
    font_78 = CreateFontA(10, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 1, 1, "Lucida Console");
    font_height_a0 = 0xb;
    if (font_78 == 0) {
        font_78 = CreateFontA(9, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 1, 1, "Fixedsys");
        font_height_a0 = 0xa;
    }
    SendMessageA(edit_window_6c, WM_SETFONT, (WPARAM)font_78, 0);
    SendMessageA(edit_window_6c, EM_SETBKGNDCOLOR, 0, 0);
    CHARFORMATA format;
    unsigned long index;
    for (index = 0; index < sizeof(format) / 4; ++index)
        ((unsigned long*)&format)[index] = 0;
    SendMessageA(edit_window_6c, EM_GETCHARFORMAT, 0, (LPARAM)&format);
    format.dwMask |= 0x40000008;
    format.cbSize = 0x3c;
    format.dwEffects = 0;
    /* Retail stores the window height into crTextColor. */
    format.crTextColor = height;
    SendMessageA(edit_window_6c, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&format);
    disabled_38 = 0;

    menu_7c = CreateMenu();
    file_menu_80 = CreateMenu();
    MENUITEMINFOA item;
    for (index = 0; index < sizeof(item) / 4; ++index)
        ((unsigned long*)&item)[index] = 0;
    item.cbSize = 0x2c;
    item.fMask = 0x37;
    item.fType = 0;
    item.fState = 0;
    item.wID = 0;
    item.hSubMenu = file_menu_80;
    item.dwItemData = 0;
    item.dwTypeData = (char*)"&File";
    item.cch = 5;
    InsertMenuItemA(menu_7c, 0, 0, &item);
    item.fMask |= 4;
    item.wID = 1;
    item.dwTypeData = (char*)"&Clear";
    item.hSubMenu = 0;
    InsertMenuItemA(file_menu_80, 0, 0, &item);
    item.fMask |= 4;
    item.wID = 2;
    item.dwTypeData = (char*)"&Save";
    item.hSubMenu = 0;
    InsertMenuItemA(file_menu_80, 1, 0, &item);
    SetMenu(frame_window_70, menu_7c);
    SetTimer(frame_window_70, 0x1ce7ea, 100, timerProc);
}

// FUNCTION: SURRENDER 0x10047690
srWindowOutStreamBuf::~srWindowOutStreamBuf()
{
    if (font_78 != 0)
        DeleteObject(font_78);
    if (frame_window_70 != 0 && srWindow::isWindow((unsigned long)frame_window_70))
        DestroyWindow(frame_window_70);
    delete line_buffer_98;
    if (riched_module_84 != 0)
        FreeLibrary(riched_module_84);
    delete[] pending_88.text;
    pending_88.text = 0;
    pending_88.capacity = 0;
}

// FUNCTION: SURRENDER 0x10047640
int srWindowOutStreamBuf::sync()
{
    scrollCaret();
    return 0;
}

// FUNCTION: SURRENDER 0x10047670
int srWindowOutStreamBuf::overflow(int ch)
{
    emit(ch);
    return 0;
}

// FUNCTION: SURRENDER 0x10047680
int srWindowOutStreamBuf::underflow()
{
    return 0;
}

// FUNCTION: SURRENDER 0x10047810
srWindowOut::srWindowOut(unsigned long handle, const char* title, long width, unsigned long height)
    : std::basic_ostream<char, std::char_traits<char> >(new srWindowOutStreamBuf(
          /* reinterpret-ok: the Win32 window handle arrives as a raw ulong
             across the srWindowOut ABI, matching srWindow's convention. */
          static_cast<unsigned long>(GetWindowLongA(reinterpret_cast<HWND>(handle), GWL_HINSTANCE)),
          handle, title, width, height))
{
    /* reinterpret-ok: the Win32 window handle arrives as a raw ulong across
       the srWindowOut ABI, matching srWindow's convention. */
    SetFocus(reinterpret_cast<HWND>(handle));
}

// FUNCTION: SURRENDER 0x10047920
srWindowOut::~srWindowOut()
{
    delete rdbuf();
}

// SYNTHETIC: SURRENDER 0X10047650
// srWindowOutStreamBuf scalar deleting destructor

// SYNTHETIC: SURRENDER 0X100479A0
// std::ios_base::Init global static-init block

// SYNTHETIC: SURRENDER 0X100479B0
// std::ios_base::Init global atexit registrar

// SYNTHETIC: SURRENDER 0X100479E0
// std::_Winit global static-init block

// SYNTHETIC: SURRENDER 0X100479F0
// std::_Winit global atexit registrar

// TEMPLATE: SURRENDER 0X10047C60
// srArray<T>::release emission

// SYNTHETIC: SURRENDER 0X10047CF0
// std::ios_base::Init global static-init block

// SYNTHETIC: SURRENDER 0X10047D00
// std::ios_base::Init global atexit registrar

// SYNTHETIC: SURRENDER 0X10047D30
// std::_Winit global static-init block

// SYNTHETIC: SURRENDER 0X10047D40
// std::_Winit global atexit registrar
