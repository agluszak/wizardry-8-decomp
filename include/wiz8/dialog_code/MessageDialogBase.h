#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "input.h"
#include "Button System.h"

// VTABLE: WIZ8 0x005ef8b0
class W8MessageDialogBase : public W8DialogBase {
public:
    W8MessageDialogBase();
    virtual ~W8MessageDialogBase() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual W8DialogKind GetDialogType() override;
    virtual bool ProcessInput() override;
    virtual bool HandleInput(const InputAtom* input);

    /* Called on this object from outside the class by the Please Wait screen,
       which is what puts it here rather than under protected. */
    void SetMessage(const wchar_t* message, int line_count, unsigned short characters_per_line,
                    bool confirmation, bool cancel, bool size_to_message, bool wrap_message,
                    int maximum_width, int maximum_height);
    /* The party-selection screen calls this centering helper on a freshly
       allocated base dialog, so it is part of the public surface rather than
       a derived-only helper. */
    void SetClientExtent(int width, int height);

    unsigned int WrapMessage(const wchar_t* message);

    friend void MessageDialogConfirmCallback(GUI_BUTTON* button, int reason);
    friend void MessageDialogCancelCallback(GUI_BUTTON* button, int reason);

    /* Both are read and written on this object from outside the class by the
       Please Wait screen's frame handler, which is what puts them here. */
    bool accepted; /* cleared; a derived close passes it on */
    bool is_open;  /* set, and gates the close path */

protected:
    short m_edge_image; /* 0x56 generic-button-image resource */
    int m_message_button;
    int m_confirm_button;
    int m_confirm_image;
    unsigned char unknown_064[0x10];
    int m_cancel_button;
    int m_cancel_image;
    unsigned char unknown_07c[0x10];
    wchar_t** m_lines;
    unsigned int m_line_count;
    bool m_show_confirm;
    bool allow_cancel; /* changes Escape handling */
    unsigned char unknown_096[2];
};

static_assert(sizeof(W8MessageDialogBase) == 0x98, "W8MessageDialogBase_must_be_0x98");

void MessageDialogConfirmCallback(GUI_BUTTON* button, int reason);
void MessageDialogCancelCallback(GUI_BUTTON* button, int reason);
