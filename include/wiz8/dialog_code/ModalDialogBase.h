#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "input.h"
#include "Button System.h"

/* The modal branch at vtable 0x005EF8B0 derives from W8DialogBase.
   W8NotificationDialog's table is identical except slot 0, which is each
   class's own scalar deleting destructor, and slot 9, which it overrides.
   Other dialog families, including monster and spell information dialogs,
   derive directly from W8DialogBase, not through this class.

   W8ModalDialogBase is a recovered role name, not an original source spelling. */
// VTABLE: WIZ8 0x005ef8b0
class W8ModalDialogBase : public W8DialogBase {
public:
    W8ModalDialogBase();                                       /* 0x005D25B0 */
    virtual ~W8ModalDialogBase() override;                     /* 0x005D2610 */
    virtual int CreateControls() override;                     /* 0x005D2D00 */
    virtual void DestroyControls() override;                   /* slot 2, 0x005D2F40 */
    virtual void Draw() override;                              /* 0x005D2660 */
    virtual int GetDialogType() override;                      /* 0x005AD280 */
    virtual unsigned char ProcessInput() override;             /* slot 9, 0x005D3080 */
    virtual unsigned char HandleInput(const InputAtom* input); /* slot 14 */

    /* Called on this object from outside the class by the Please Wait screen,
       which is what puts it here rather than under protected. */
    void SetMessage(wchar_t* message, int line_count, int characters_per_line, int confirmation,
                    int cancel, int size_to_message, int wrap_message, int maximum_width,
                    int maximum_height); /* 0x005D2800 */
    /* State 5 calls this centering helper on a freshly allocated base dialog,
       so it is part of the public surface rather than a derived-only helper. */
    void SetClientExtent(int width, int height); /* 0x005D2CB0 */

    unsigned int WrapMessage(wchar_t* message); /* 0x005D2A50 */

    friend void ModalDialogConfirmCallback(GUI_BUTTON* button, int reason);
    friend void ModalDialogCancelCallback(GUI_BUTTON* button, int reason);

    /* Both are read and written on this object from outside the class by the
       Please Wait screen's frame handler, which is what puts them here. */
    unsigned char close_result; /* 0x54: cleared; a derived close passes it on */
    bool is_open;               /* 0x55: set, and gates the close path */

protected:
    short m_edge_image;   /* 0x56 generic-button-image resource */
    int m_message_button; /* 0x58 */
    int m_confirm_button; /* 0x5c */
    int m_confirm_image;  /* 0x60 */
    unsigned char unknown_064[0x10];
    int m_cancel_button; /* 0x74 */
    int m_cancel_image;  /* 0x78 */
    unsigned char unknown_07c[0x10];
    wchar_t** m_lines;            /* 0x8c */
    unsigned int m_line_count;    /* 0x90 */
    unsigned char m_show_confirm; /* 0x94 */
    unsigned char allow_cancel;   /* 0x95: changes Escape handling */
    unsigned char unknown_096[2];
}; /* 0x98 */

static_assert(sizeof(W8ModalDialogBase) == 0x98, "W8ModalDialogBase_must_be_0x98");

void ModalDialogConfirmCallback(GUI_BUTTON* button, int reason);
void ModalDialogCancelCallback(GUI_BUTTON* button, int reason);
