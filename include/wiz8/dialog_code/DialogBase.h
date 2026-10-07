#pragma once

#include "wiz8/local_code/Widget.h"
#include <wchar.h>

enum W8DialogKind {
    W8_DIALOG_BASIC = 0,
    W8_DIALOG_MESSAGE = 1,
    W8_DIALOG_LIST_BOX = 3,
    W8_DIALOG_TRIGGER_ITEM_PICKER = 4,
    W8_DIALOG_SPLIT_AMOUNT = 5,
    W8_DIALOG_CHARACTER_SUMMARY = 6,
    W8_DIALOG_NPC = 7
};

enum { W8_DIALOG_DIRTY_REDRAW = 1u };

class W8DialogBase;
class W8DialogButton;
class W8DialogTextArea;
typedef void (*W8DialogDestroyCallback)(W8DialogBase* dialog);
void SetDialogDestroyCallback(W8DialogBase* dialog, W8DialogDestroyCallback callback);
/* Shared close-button callback used by the monster and spell info dialogs. */
void DialogCloseButtonCallback(W8DialogButton* button);

/* The modal dialog shell. It uses SGP Button System resources rather than
   Controls panels; concrete dialogs contain their own button/text/scroll
   helpers. */
// VTABLE: WIZ8 0x005efaf8
class W8DialogBase {
public:
    W8DialogBase();
    virtual ~W8DialogBase();
    virtual int CreateControls();
    virtual void DestroyControls();
    virtual void Draw();
    virtual W8DialogKind GetDialogType(); /* Base=0, modal=1, list=3 */
    virtual void SetText(const wchar_t* text);
    virtual void SetOrigin(int x, int y);
    virtual void SetExtent(int width, int height);
    virtual void SetBackground(const char* path, int flags);
    virtual bool ProcessInput();
    virtual void OnNumericInputChanged(int control_id);
    virtual void OnRightButtonDown();
    virtual void OnRightButtonUp();
    virtual void OnMouseWheel(int delta);

    friend void SetDialogDestroyCallback(W8DialogBase* dialog, W8DialogDestroyCallback callback);
    friend class W8DialogNumericInput;

public:
    /* Main Game raises the redraw bit when promoting its pending dialog. */
    unsigned int m_dirty_flags;

protected:
    void CreateCloseButton(W8DialogButton& button, int left, int top);
    void ScrollTextArea(W8DialogTextArea& area, int first_visible_line, int left, int top,
                        int width, int height);

    int m_error;
    int m_resource;
    wchar_t* m_text;
    int m_font;
    unsigned char m_foreground;
    unsigned char m_background;
    unsigned char padding_01a[2];
    char* m_background_path;
    int m_background_flags;
    short m_border;
    unsigned char padding_026[2];
    int m_x;
    int m_y;
    int m_width;
    int m_height;
    unsigned char unknown_038[8];
    bool m_initialized;

public:
    /* Cleared to close the dialog; ProcessInput keeps running while set. */
    bool m_keep_open;

public:
    /* The trigger update installs its callback with a plain store, so this
       slot is public rather than reachable only through the setter. */
    unsigned char padding_042[2];
    W8DialogDestroyCallback m_destroy_callback;
    /* Base constructor clears this slot; the destruction callback consumes
       it. The item-picker callback's payload is a Trigger*. */
    void* m_destroy_callback_context;

protected:
    /* Cleared by W8DialogNumericInput when its field deactivates. */
    int m_field_4c;
    bool m_right_button_down;
    unsigned char padding_051[3];
};

static_assert(sizeof(W8DialogBase) == 0x54, "W8DialogBase_must_be_0x54");

extern int g_live_dialog_count;
