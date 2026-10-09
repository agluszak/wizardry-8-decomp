#include "surrender/srWindow.h"
#include "surrender/srWindowOut.h"

#include <fstream>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include <commdlg.h>
#include <richedit.h>

/* Internal stream buffer owning the "srDebugWndClass" diagnostic console: a frame window with a
   RichEdit child, a File menu and a periodic timer. */
class srWindowOutStreamBuf : public std::basic_streambuf<char, std::char_traits<char> > {
public:
    srWindowOutStreamBuf(w8_ulong instance, w8_ulong parent, const char* title, w8_long width,
                         w8_ulong height);
    virtual ~srWindowOutStreamBuf();

private:
    /* Pending output text. '\n' flushes the whole buffer into the RichEdit; a trailing partial line
       stays pending until the next newline, Clear or Save. */
    struct Pending {
        char* text;        /* 0x88 */
        w8_ulong capacity; /* 0x8c */

        void reserve(w8_ulong size);
        char* c_str()
        {
            if (capacity == 0)
                reserve(8);
            return text;
        }
    };

    /* The "srDebugWndClass" window procedure; the constructor stores this
       object's pointer in GWL_USERDATA. */
    static w8_long __stdcall windowProc(HWND window, unsigned int message, WPARAM wparam,
                                        LPARAM lparam);
    static void __stdcall timerProc(HWND window, unsigned int message, unsigned int timer,
                                    w8_ulong time);
    static w8_ulong __stdcall streamOutCallback(w8_ulong stream, LPBYTE buffer, w8_long count,
                                                w8_long* written);

    virtual int overflow(int ch);
    virtual int underflow();
    virtual int sync();

    /* Bounds-checked append into the pending buffer; '\n' and '\t' flush/expand instead of storing
       the byte. */
    void emit(int ch);
    /* Scroll the caret into view after text was appended. */
    void scrollCaret();
    /* WM_DESTROY teardown; also makes emit() a no-op. */
    void closeWindow();
    /* "&Clear": reset pending text and the RichEdit. */
    void clearText();
    /* "&Save": GetSaveFileName + EM_STREAMOUT into a new std::ofstream. */
    void saveText();

    /* Grow-then-store one character. */
    void put(w8_ulong index, char ch)
    {
        if (pending.capacity <= index)
            pending.reserve(pending.capacity + 8 + index);
        pending.text[index] = ch;
    }

    w8_ulong disabled;      /* 0x38: emit()/clear/save gate */
    WNDCLASSA window_class; /* 0x3c */
    HINSTANCE instance;     /* 0x64 */
    HWND parent;            /* 0x68 */
    HWND edit_window;       /* 0x6c */
    HWND frame_window;      /* 0x70 */
    w8_ulong field_74;      /* 0x74 */
    HFONT font;             /* 0x78 */
    HMENU menu;             /* 0x7c */
    HMENU file_menu;        /* 0x80 */
    HMODULE riched_module;  /* 0x84 */
    Pending pending;        /* 0x88 */
    w8_ulong length;        /* 0x90: used bytes in pending */
    w8_ulong scroll;        /* 0x94: caret scroll needed */
    char* line_buffer;      /* 0x98: allocated but unused */
    w8_ulong buffer_size;   /* 0x9c */
    w8_ulong font_height;   /* 0xa0 */
};

// GLOBAL: SURRENDER 0x100A49B0
// Unique suffix counter for the "srDebugWndClass<i>" window-class names.
static int class_counter;

// FUNCTION: SURRENDER 0x10047C80
void srWindowOutStreamBuf::Pending::reserve(w8_ulong size)
{
    if (capacity == size)
        return;
    char* next = 0;
    if (size != 0) {
        next = new char[size];
        if (text != 0 && capacity != 0) {
            w8_ulong copy = size <= capacity ? size : capacity;
            for (w8_ulong index = 0; index < copy; ++index)
                next[index] = text[index];
        }
    }
    delete[] text;
    text = next;
    capacity = size;
}

// FUNCTION: SURRENDER 0x10046810
void __stdcall srWindowOutStreamBuf::timerProc(HWND window, unsigned int message,
                                               unsigned int timer, w8_ulong time)
{
    if (message != WM_TIMER || timer != 0x1ce7ea)
        return;
    // reinterpret-ok: GWL_USERDATA holds this object's pointer.
    srWindowOutStreamBuf* self =
        reinterpret_cast<srWindowOutStreamBuf*>(GetWindowLongA(window, GWL_USERDATA));
    if (self != 0 && self->scroll != 0)
        self->scrollCaret();
}

// FUNCTION: SURRENDER 0x10046870
w8_long __stdcall srWindowOutStreamBuf::windowProc(HWND window, unsigned int message, WPARAM wparam,
                                                   LPARAM lparam)
{
    // reinterpret-ok: GWL_USERDATA holds this object's pointer.
    srWindowOutStreamBuf* self =
        reinterpret_cast<srWindowOutStreamBuf*>(GetWindowLongA(window, GWL_USERDATA));
    if (message < 0x15) {
        if (message == WM_ERASEBKGND)
            return 1;
        if (message == WM_DESTROY) {
            if (self != 0) {
                self->closeWindow();
                return DefWindowProcA(window, message, wparam, lparam);
            }
        } else if (message == WM_SIZE && self != 0 && self->edit_window != 0) {
            MoveWindow(self->edit_window, 0, 0, lparam & 0xffff,
                       static_cast<w8_ulong>(lparam) >> 0x10, 1);
            return DefWindowProcA(window, message, wparam, lparam);
        }
    } else if (message == WM_KEYDOWN) {
        if (wparam == VK_ESCAPE && self != 0)
            SetFocus(self->parent);
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
w8_ulong __stdcall srWindowOutStreamBuf::streamOutCallback(w8_ulong stream, LPBYTE buffer,
                                                           w8_long count, w8_long* written)
{
    if (count != 0) {
        // reinterpret-ok: the EDITSTREAM cookie carries the ofstream.
        std::ofstream* output = reinterpret_cast<std::ofstream*>(stream);
        // reinterpret-ok: the callback delivers the text as raw bytes.
        output->write(reinterpret_cast<const char*>(buffer), count);
        *written = count;
        return 0;
    }
    return 0xffffffff;
}

// FUNCTION: SURRENDER 0x10046D10
void srWindowOutStreamBuf::clearText()
{
    if (disabled != 0)
        return;
    pending.reserve(1);
    *pending.c_str() = 0;
    length = 0;
    SendMessageA(edit_window, WM_SETTEXT, 0, (LPARAM)pending.c_str());
}

// FUNCTION: SURRENDER 0x10046E90
void srWindowOutStreamBuf::scrollCaret()
{
    SendMessageA(edit_window, EM_SCROLLCARET, 0, 0);
    scroll = 0;
}

// FUNCTION: SURRENDER 0x10046EC0
void srWindowOutStreamBuf::emit(int ch)
{
    if (disabled != 0)
        return;
    if ((char)ch == '\n') {
        scroll = 1;
        SendMessageA(edit_window, EM_SETSEL, (WPARAM)-1, -1);
        w8_ulong start;
        w8_ulong end;
        SendMessageA(edit_window, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
        SendMessageA(edit_window, EM_SETSEL, start + end, start + end);
        put(length++, '\r');
        put(length++, '\n');
        put(length, 0);
        SendMessageA(edit_window, EM_REPLACESEL, 0, (LPARAM)pending.c_str());
        length = 0;
        return;
    }
    if ((char)ch == '\t') {
        for (int index = 0; index < 4; ++index)
            put(length++, ' ');
        put(length, 0);
        return;
    }
    put(length++, (char)ch);
    put(length, 0);
}

// FUNCTION: SURRENDER 0x100469C0
void srWindowOutStreamBuf::saveText()
{
    if (disabled != 0)
        return;
    OPENFILENAMEA info;
    char file[300];
    for (w8_ulong index = 0; index < 300; ++index)
        file[index] = 0;
    info.hwndOwner = frame_window;
    info.hInstance = instance;
    info.lpstrFile = file;
    info.lStructSize = sizeof(info);
    info.lpstrFilter = 0;
    info.lpstrCustomFilter = 0;
    info.nMaxCustFilter = 0;
    info.nFilterIndex = 0;
    info.nMaxFile = 299;
    info.lpstrFileTitle = 0;
    /* nMaxFileTitle is never written. */
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
    if (scroll == 0 && length != 0) {
        SendMessageA(edit_window, EM_SETSEL, (WPARAM)-1, -1);
        w8_ulong start;
        w8_ulong end;
        SendMessageA(edit_window, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
        SendMessageA(edit_window, EM_SETSEL, start + end, start + end + 1);
        SendMessageA(edit_window, EM_REPLACESEL, 0, (LPARAM)pending.c_str());
        length = 0;
        scroll = 1;
    }
    EDITSTREAM output;
    // reinterpret-ok: the EDITSTREAM cookie carries the ofstream.
    output.dwCookie = reinterpret_cast<w8_ulong>(stream);
    output.dwError = 0;
    output.pfnCallback = streamOutCallback;
    SendMessageA(edit_window, EM_STREAMOUT, SF_TEXT, (LPARAM)&output);
    delete stream;
}

// FUNCTION: SURRENDER 0x10047790
void srWindowOutStreamBuf::closeWindow()
{
    disabled = 1;
    if (frame_window == 0)
        return;
    if (srWindow::isWindow((w8_ulong)frame_window)) {
        SetMenu(frame_window, 0);
        DestroyMenu(file_menu);
        DestroyMenu(menu);
        KillTimer(frame_window, 0x1ce7ea);
        SetWindowLongA(frame_window, GWL_USERDATA, 0);
        file_menu = 0;
        menu = 0;
    }
    frame_window = 0;
}

// FUNCTION: SURRENDER 0x10047150
srWindowOutStreamBuf::srWindowOutStreamBuf(w8_ulong instance, w8_ulong parent, const char* title,
                                           w8_long width, w8_ulong height)
{
    pending.text = 0;
    pending.capacity = 0;
    buffer_size = 1;
    length = 0;
    disabled = 1;
    scroll = 1;
    line_buffer = new char;
    edit_window = 0;
    frame_window = 0;
    riched_module = 0;
    font = 0;
    this->instance = (HINSTANCE)instance;
    this->parent = (HWND)parent;

    char class_name[256];
    sprintf(class_name, "%s%i", "srDebugWndClass", class_counter++);

    window_class.style = 0xb;
    window_class.lpfnWndProc = windowProc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = (HINSTANCE)instance;
    window_class.hIcon = LoadIconA(0, (const char*)0x7f00);
    window_class.hCursor = LoadCursorA(0, (const char*)0x7f00);
    window_class.hbrBackground = (HBRUSH)GetStockObject(4);
    window_class.lpszMenuName = 0;
    window_class.lpszClassName = class_name;
    if (RegisterClassA(&window_class) == 0)
        return;

    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = 400;
    rect.bottom = 400;
    AdjustWindowRect(&rect,
                     WS_VISIBLE | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, 0);
    rect.right -= rect.left;
    rect.bottom -= rect.top;
    rect.left = 0;
    rect.top = 0;
    rect.right += GetSystemMetrics(SM_CXSCREEN) - rect.right;

    riched_module = LoadLibraryA("riched32.dll");
    if (riched_module == 0)
        return;

    frame_window =
        CreateWindowExA(0, class_name, title,
                        WS_VISIBLE | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
                        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                        (HWND)parent, 0, (HINSTANCE)instance, 0);
    SetWindowLongA(frame_window, GWL_USERDATA, (w8_long)this);
    edit_window = CreateWindowExA(0, "RichEdit", "",
                                  WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_READONLY |
                                      ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE,
                                  0, 0, 400, 400, frame_window, 0, (HINSTANCE)instance, 0);
    RECT client;
    GetClientRect(frame_window, &client);
    MoveWindow(edit_window, 0, 0, client.right - client.left, client.bottom - client.top, 1);
    font = CreateFontA(10, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 1, 1, "Lucida Console");
    font_height = 0xb;
    if (font == 0) {
        font = CreateFontA(9, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 1, 1, "Fixedsys");
        font_height = 0xa;
    }
    SendMessageA(edit_window, WM_SETFONT, (WPARAM)font, 0);
    SendMessageA(edit_window, EM_SETBKGNDCOLOR, 0, 0);
    CHARFORMATA format;
    memset(&format, 0, sizeof(format));
    SendMessageA(edit_window, EM_GETCHARFORMAT, 0, (LPARAM)&format);
    format.dwMask |= 0x40000008;
    format.cbSize = 0x3c;
    format.dwEffects = 0;
    /* The window height is stored into crTextColor. */
    format.crTextColor = height;
    SendMessageA(edit_window, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&format);
    disabled = 0;

    menu = CreateMenu();
    file_menu = CreateMenu();
    MENUITEMINFOA item;
    memset(&item, 0, sizeof(item));
    item.cbSize = 0x2c;
    item.fMask = 0x37;
    item.fType = 0;
    item.fState = 0;
    item.wID = 0;
    item.hSubMenu = file_menu;
    item.dwItemData = 0;
    item.dwTypeData = (char*)"&File";
    item.cch = 5;
    InsertMenuItemA(menu, 0, 0, &item);
    item.fMask |= 4;
    item.wID = 1;
    item.dwTypeData = (char*)"&Clear";
    item.hSubMenu = 0;
    InsertMenuItemA(file_menu, 0, 0, &item);
    item.fMask |= 4;
    item.wID = 2;
    item.dwTypeData = (char*)"&Save";
    item.hSubMenu = 0;
    InsertMenuItemA(file_menu, 1, 0, &item);
    SetMenu(frame_window, menu);
    SetTimer(frame_window, 0x1ce7ea, 100, timerProc);
}

// FUNCTION: SURRENDER 0x10047690
srWindowOutStreamBuf::~srWindowOutStreamBuf()
{
    if (font != 0)
        DeleteObject(font);
    if (frame_window != 0 && srWindow::isWindow((w8_ulong)frame_window))
        DestroyWindow(frame_window);
    delete line_buffer;
    if (riched_module != 0)
        FreeLibrary(riched_module);
    delete[] pending.text;
    pending.text = 0;
    pending.capacity = 0;
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
srWindowOut::srWindowOut(w8_ulong handle, const char* title, w8_long width, w8_ulong height)
    : std::basic_ostream<char, std::char_traits<char> >(new srWindowOutStreamBuf(
          /* reinterpret-ok: raw window handle. */
          // reinterpret-ok: the window handle retains its historical Win32 representation
          static_cast<w8_ulong>(GetWindowLongA(reinterpret_cast<HWND>(handle), GWL_HINSTANCE)),
          handle, title, width, height))
{
    /* reinterpret-ok: raw window handle. */
    SetFocus(reinterpret_cast<HWND>(handle));
}

// FUNCTION: SURRENDER 0x10047920
srWindowOut::~srWindowOut()
{
    delete rdbuf();
}
