#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "input.h"
#include "Button System.h"

/* The shared dialog base at vtable 0x005EF8B0, between W8DialogBase005DC7A0
   and the concrete dialogs. Its fifteen slots are what every derived dialog
   inherits: W8NotificationDialog's table is identical except slot 0, which is each
   class's own scalar deleting destructor, and slot 9, which it overrides.

   The released binary exposes no original name for it, so the name is
   qualified by its constructor address. */
// VTABLE: WIZ8 0x005ef8b0
class W8ModalDialogBase : public W8DialogBase005DC7A0 {
public:
    W8ModalDialogBase();                              /* 0x005D25B0 */
    virtual ~W8ModalDialogBase() override;            /* 0x005D2610 */
    virtual int vslot1() override;                    /* 0x005D2D00 */
    virtual void ResetSubobjectAndRefresh() override;    /* slot 2, 0x005D2F40 */
    virtual void vslot3() override;                   /* 0x005D2660 */
    virtual int vslot4() override;                    /* 0x005AD280 */
    virtual unsigned char ProcessInput() override;       /* slot 9, 0x005D3080 */
    virtual unsigned char HandleInput(
        const InputAtom* input);                         /* slot 14 */

    /* Called on this object from outside the class by the Please Wait screen,
       which is what puts it here rather than under protected. */
    void SetMessage(void* payload, int a, int b, int c, int d,
                    int e, int f, int g, int h);     /* 0x005D2800 */
    /* State 5 calls this centering helper on a freshly allocated base dialog,
       so it is part of the public surface rather than a derived-only helper. */
    void SetClientExtent(int width, int height);     /* 0x005D2CB0 */

    unsigned int WrapMessage(wchar_t* message);      /* 0x005D2A50 */

    friend void Function5D32C0(GUI_BUTTON* button, int reason);
    friend void Function5D3370(GUI_BUTTON* button, int reason);

    /* Both are read and written on this object from outside the class by the
       Please Wait screen's frame handler, which is what puts them here. */
    unsigned char close_result;            /* 0x54: cleared; a derived close passes it on */
    unsigned char is_open;            /* 0x55: set, and gates the close path */

protected:
    short m_field_56;                    /* 0x56 */
    int m_field_58;                      /* 0x58 */
    int m_field_5c;                      /* 0x5c */
    int m_field_60;                      /* 0x60 */
    unsigned char unknown_064[0x10];
    int m_field_74;                      /* 0x74 */
    int m_field_78;                      /* 0x78 */
    unsigned char unknown_07c[0x10];
    wchar_t** m_lines;                   /* 0x8c */
    unsigned int m_line_count;           /* 0x90 */
    unsigned char m_field_94;            /* 0x94 */
    unsigned char allow_cancel;            /* 0x95: changes Escape handling */
    unsigned char unknown_096[2];
};                                       /* 0x98 */

static_assert(sizeof(W8ModalDialogBase) == 0x98,
              "W8ModalDialogBase_must_be_0x98");
