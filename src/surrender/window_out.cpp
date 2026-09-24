#include "surrender/srWindowOut.h"

#include <windows.h>

/* Internal stream buffer owning the "srDebugWndClass" diagnostic console: a
   frame window with a RichEdit child, a File menu and a periodic timer.
   Retail keeps the class translation-unit local - nothing exports it and no
   other code references it. Its member bodies are still unrecovered, so the
   declarations carry the reviewed retail addresses and stay link-unresolved
   like the other first-party gaps. */
class srWindowOutStreamBuf : public std::basic_streambuf<char, std::char_traits<char> > {
public:
    /* 0x10047150 */
    srWindowOutStreamBuf(unsigned long instance, unsigned long parent, const char* title,
                         long width, unsigned long height);
    /* 0x10047690 */
    virtual ~srWindowOutStreamBuf();

private:
    /* The "srDebugWndClass" window procedure; the constructor stores this
       object's pointer in GWL_USERDATA. */
    /* 0x10046870 */
    static long __stdcall windowProc(unsigned long window, unsigned int message,
                                     unsigned int wparam, long lparam);
    /* 0x10047670 */
    virtual int overflow(int ch);
    /* 0x10047680 */
    virtual int underflow();
    /* 0x10047640 */
    virtual int sync();

    unsigned long field_38;         /* 0x38 */
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
    unsigned long field_88;         /* 0x88 */
    unsigned long field_8c;         /* 0x8c */
    unsigned long field_90;         /* 0x90 */
    unsigned long field_94;         /* 0x94 */
    char* line_buffer_98;           /* 0x98 */
    unsigned long buffer_size_9c;   /* 0x9c */
    unsigned long font_height_a0;   /* 0xa0 */
};

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
