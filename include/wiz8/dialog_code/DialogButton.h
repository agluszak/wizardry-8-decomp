#pragma once

#include "Button System.h"
#include "mousesystem_macros.h"

/* Update the non-toggle dialog arrow's pressed/hover state. The caller
   retains its action and invalidates only when this operation reports a change. */
inline bool UpdateDialogArrowState(GUI_BUTTON* button, INT32 reason)
{
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        if (!(button->uiFlags & BUTTON_CLICKED_ON)) {
            button->uiFlags |= BUTTON_CLICKED_ON;
            return true;
        }
    } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
        if (button->uiFlags & BUTTON_CLICKED_ON) {
            button->uiFlags &= ~BUTTON_CLICKED_ON;
            return true;
        }
    } else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
        button->Area.uiFlags |= MSYS_MOUSE_IN_AREA;
        return true;
    } else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
        button->Area.uiFlags &= ~MSYS_MOUSE_IN_AREA;
        return true;
    }
    return false;
}

/* Tooltip string-table sentinel; independent of the five frame indices. */
enum { W8_DIALOG_BUTTON_NO_TOOLTIP = -1 };

class W8DialogBase;
class W8DialogButton;

/* Callbacks stored through Configure and dispatched by the button's own SGP
   callback with the W8DialogButton, not the raw SGP button, as the subject. */
typedef void (*W8DialogButtonCallback)(W8DialogButton* button);

void DialogButtonCallback(GUI_BUTTON* button, INT32 reason);

// VTABLE: WIZ8 0x005efa98
class W8DialogButton {
public:
    W8DialogButton();
    virtual ~W8DialogButton();
    void Draw();
    void SetPosition(int x, int y);
    int GetWidth();
    int GetHeight();
    int GetX();
    int GetY();
    void SetEnabled(bool enabled);
    void UpdateEnabledState(bool enabled)
    {
        if (!enabled) {
            SetEnabled(false);
            m_dirty = true;
        } else if (!IsEnabled()) {
            SetEnabled(true);
            m_dirty = true;
        }
    }
    bool IsEnabled();
    void SetPressed(bool pressed);
    unsigned char IsPressed();
    void SetVisible(bool visible);
    /* Installs the tooltip string on the SGP button (when one is configured
       and tooltips are on), or clears it. */
    void SetTooltipEnabled(bool enabled);
    int GetUserData();
    /* Load the frame images, create the SGP button, and store the
       per-button callbacks, left-toggle flag, priority and tooltip index. */
    bool Configure(const char* image_path, int gray_frame, int off_normal_frame,
                   int off_hover_frame, int on_normal_frame, int on_hover_frame,
                   W8DialogButtonCallback left_callback, W8DialogButtonCallback move_callback,
                   bool left_toggles, short priority, int tooltip_index,
                   W8DialogButtonCallback right_callback,
                   W8DialogButtonCallback double_click_callback);
    bool ConfigureIcon(const char* image_path, int disabled_frame, int normal_frame, short priority,
                       int tooltip_index)
    {
        return Configure(image_path, disabled_frame, normal_frame, BUTTON_NO_IMAGE, normal_frame,
                         BUTTON_NO_IMAGE, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK, false, priority,
                         tooltip_index, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    }

    /* Create an SGP text button (BUTTON_NO_TOGGLE, priority 0x7f),
       store this in its user-data slot 0 and the payload in slot 1 (read back
       by GetUserData), then install the left-click callback. */
    bool ConfigureTextButton(const wchar_t* text, unsigned int font, short fore_color,
                             short shadow_color, short x, short y, short width, short height,
                             W8DialogButtonCallback left_callback, int user_data);
    /* Configure from an already-loaded vobject; the five state
       frames derive from base_frame (gray +3, off +0/+1, on states +2/+2). */
    bool ConfigureVObjButton(HVOBJECT object, int base_frame, W8DialogButtonCallback left_callback,
                             bool left_toggles);
    /* Store the tooltip string-table index and, when tooltips are
       enabled, push its text onto the SGP button. */
    void SetTooltipIndex(int tooltip_index);

    friend void DialogButtonCallback(GUI_BUTTON* button, INT32 reason);

private:
    int m_gray_frame;
    int m_off_normal_frame;
    int m_off_hover_frame;
    int m_on_normal_frame;
    int m_on_hover_frame;
    int m_image_handle;  /* loaded button-image handle */
    int m_button_handle; /* SGP button handle */
    int m_tooltip_index; /* gppStringList index, or -1 */
    W8DialogButtonCallback m_left_callback;
    W8DialogButtonCallback m_right_callback;
    W8DialogButtonCallback m_move_callback;
    W8DialogButtonCallback m_double_click_callback;
    unsigned char m_latched_press_bits; /* latched BUTTON_CLICKED_ON bit */
    bool m_enabled;
    bool m_left_toggles; /* left down xors CLICKED_ON */

public:
    /* Raised by the dialog factories on their item-row buttons; right
       down then toggles CLICKED_ON the same way left-toggle does. */
    bool m_right_toggles;
    bool m_dirty; /* set by owning screens before Draw */
private:
    bool silent;       /* suppresses all button sounds */
    bool hover_silent; /* suppresses hover/exit sounds only */

public:
    /* Raised on spinner-style buttons (the split dialog's arrows); the
       dispatch then fires the stored left/right callback on press and clears
       the pending state on release instead of firing on release. */
    bool m_fires_on_press;

private:
    bool press_armed; /* clicked-on and fires_on_press: release completes */

public:
    /* Set by the dialog factories to the owning dialog; the per-button
       dispatch callback reads it back when a stored callback needs it. */
    W8DialogBase* m_owner;

private:
    /* Snapshot of the live-dialog count; dispatch ignores events when a
       nested dialog has changed that count. */
    int m_live_dialog_count;
};

W8_ABI_ASSERT(sizeof(W8DialogButton) == 0x48, "W8DialogButton_size");

/* Button arrays keep unallocated slots as supplied by their owner. Failure
   deletes every non-null slot in the full array, in ascending order. */
inline void DestroyDialogButtons(W8DialogButton** buttons, int count)
{
    for (int index = 0; index < count; ++index) {
        if (buttons[index] != 0) {
            delete buttons[index];
            buttons[index] = 0;
        }
    }
}

inline bool AllocateDialogButtons(W8DialogButton** buttons, int count)
{
    for (int index = 0; index < count; ++index) {
        buttons[index] = new W8DialogButton;
        if (buttons[index] == 0) {
            DestroyDialogButtons(buttons, count);
            return false;
        }
    }
    return true;
}

/* Dialog handles keep their sentinel until release finishes. Removal,
   ordinary images and generic borders have distinct SGP resource owners. */
inline void ReleaseDialogButtonHandle(INT32& handle)
{
    if (handle != -1) {
        RemoveButton(handle);
        handle = -1;
    }
}

inline void ReleaseDialogButtonImage(INT32& handle)
{
    if (handle != -1) {
        UnloadButtonImage(handle);
        handle = -1;
    }
}

inline void ReleaseDialogBorderImage(INT16& handle)
{
    if (handle != -1) {
        UnloadGenericButtonImage(handle);
        handle = -1;
    }
}
