#pragma once

#include "Button System.h"

class W8DialogBase;
class W8DialogButton;

/* Callbacks stored through Configure and dispatched by the button's own SGP
   callback with the W8DialogButton, not the raw SGP button, as the subject. */
typedef void (*W8DialogButtonCallback)(W8DialogButton* button);

void DialogButtonCallback(GUI_BUTTON* button, INT32 reason);

// VTABLE: WIZ8 0x005efa98
class W8DialogButton {
public:
    W8DialogButton();          /* 0x005DB1B0 */
    virtual ~W8DialogButton(); /* 0x005DB260 */
    void Draw();
    void SetPosition(int x, int y);
    int GetWidth();
    int GetHeight();
    int GetX();
    int GetY();
    void SetEnabled(bool enabled);
    bool IsEnabled();
    void SetPressed(bool pressed);
    unsigned char IsPressed();
    void SetVisible(bool visible);
    int GetUserData();
    /* 0x005DB3E0: load the frame images, create the SGP button, and store the
       per-button callbacks, left-toggle flag, priority and tooltip index. */
    unsigned char Configure(const char* image_path, int gray_frame, int off_normal_frame,
                            int off_hover_frame, int on_normal_frame, int on_hover_frame,
                            W8DialogButtonCallback left_callback,
                            W8DialogButtonCallback move_callback, unsigned char left_toggles,
                            short priority, int tooltip_index,
                            W8DialogButtonCallback right_callback,
                            W8DialogButtonCallback double_click_callback);

    friend void DialogButtonCallback(GUI_BUTTON* button, INT32 reason);

private:
    int m_gray_frame;       /* 0x04 */
    int m_off_normal_frame; /* 0x08 */
    int m_off_hover_frame;  /* 0x0c */
    int m_on_normal_frame;  /* 0x10 */
    int m_on_hover_frame;   /* 0x14 */
    int m_image_018;        /* loaded button-image handle */
    int m_button_01c;       /* SGP button handle */
    int m_tooltip_index;    /* 0x20: gppStringList index, or -1 */
    W8DialogButtonCallback m_left_callback;
    W8DialogButtonCallback m_right_callback;
    W8DialogButtonCallback m_move_callback;
    W8DialogButtonCallback m_double_click_callback;
    unsigned char m_clicked; /* 0x34: latched BUTTON_CLICKED_ON bit */
    bool m_enabled_035;
    unsigned char m_left_toggles; /* 0x36: left down xors CLICKED_ON */

public:
    /* 0x37: raised by the dialog factories on their item-row buttons; right
       down then toggles CLICKED_ON the same way left-toggle does. */
    unsigned char m_right_toggles;
    bool m_dirty; /* 0x38: set by owning screens before Draw */
private:
    unsigned char unknown_039;
    unsigned char unknown_03a;
    unsigned char unknown_03b;
    unsigned char unknown_03c;
    unsigned char unknown_03d[3];

public:
    /* 0x40: set by the dialog factories to the owning dialog; the per-button
       dispatch callback reads it back when a stored callback needs it. */
    W8DialogBase* m_owner_040;

private:
    /* 0x44: snapshot of the live-dialog count; dispatch ignores events when a
       nested dialog has changed that count. */
    int m_live_dialog_count;
}; /* 0x48 */

static_assert(sizeof(W8DialogButton) == 0x48, "W8DialogButton_size");
