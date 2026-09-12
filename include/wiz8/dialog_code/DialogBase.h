#pragma once

#include "wiz8/local_code/Widget.h"
#include <wchar.h>

class W8DialogBase;
typedef void (*W8DialogDestroyCallback)(W8DialogBase* dialog);
void SetDialogDestroyCallback(W8DialogBase* dialog, W8DialogDestroyCallback callback);

/* Names describe recovered roles; retail does not expose their original
   source spellings. Address markers retain the binary identities.
   This shell uses SGP Button System resources; it is not a Controls panel or
   a W8Widget. Concrete dialogs contain their own button/text/scroll helpers. */
// VTABLE: WIZ8 0x005efaf8
class W8DialogBase {
public:
    W8DialogBase();                            /* 0x005DC7A0 */
    virtual ~W8DialogBase();                   /* 0x005DC860 */
    virtual int CreateControls();              /* 0x005DCAF0 */
    virtual void DestroyControls();            /* 0x005DCC30 */
    virtual void Draw();                       /* 0x005DC890 */
    virtual int GetDialogType();               /* 0x005D6FA0: base=0, modal=1, list=3 */
    virtual void SetText(const wchar_t* text); /* 0x005DC940 */
    /* Slots 6, 7 and 8 of the table at 0x005EFAF8 are 0x005DC9C0, 0x005DC9F0 and
       0x005DCA70 - the three setters a derived constructor calls directly
       because its dynamic type is fixed, and that a caller holding a base
       pointer reaches through the table. */
    virtual void SetOrigin(int x, int y);                    /* 0x005DC9C0 */
    virtual void SetExtent(int width, int height);           /* 0x005DC9F0 */
    virtual void SetBackground(const char* path, int flags); /* 0x005DCA70 */
    virtual unsigned char ProcessInput();                    /* 0x005DCCE0 */
    virtual void OnNumericInputChanged(int control_id);
    virtual void OnRightButtonDown();
    virtual void OnRightButtonUp();
    virtual void OnMouseWheel(int delta);

    friend void SetDialogDestroyCallback(W8DialogBase* dialog, W8DialogDestroyCallback callback);

public:
    /* Main Game raises the redraw bit when promoting its pending dialog. */
    unsigned int m_dirty_flags; /* 0x04 */
protected:
    int m_error;                /* 0x08 */
    int m_resource;             /* 0x0c */
    wchar_t* m_text;            /* 0x10 */
    int m_font;                 /* 0x14 */
    unsigned char m_foreground; /* 0x18 */
    unsigned char m_background; /* 0x19 */
    unsigned char unknown_01a[2];
    char* m_background_path; /* 0x1c */
    int m_background_flags;  /* 0x20 */
    short m_border;          /* 0x24 */
    unsigned char unknown_026[2];
    int m_x;      /* 0x28 */
    int m_y;      /* 0x2c */
    int m_width;  /* 0x30 */
    int m_height; /* 0x34 */
    unsigned char unknown_038[8];
    unsigned char m_initialized; /* 0x40 */
public:
    /* Cleared to close the dialog; ProcessInput keeps running while set. */
    unsigned char m_keep_open; /* 0x41 */
public:
    /* The trigger update installs its callback with a plain store, so this
       slot is public rather than reachable only through the setter. */
    unsigned char unknown_042[2];
    W8DialogDestroyCallback m_destroy_callback; /* 0x44 */
    /* Generic owner slot. The item dialog stores its Trigger here and the
       destroy callback reads it back. */
    int m_user_data; /* 0x48 */
protected:
    int m_field_4c;                    /* 0x4c */
    unsigned char m_right_button_down; /* 0x50 */
    unsigned char unknown_051[3];
}; /* 0x54 */

static_assert(sizeof(W8DialogBase) == 0x54, "W8DialogBase_must_be_0x54");

extern int g_dword_69ca28;
