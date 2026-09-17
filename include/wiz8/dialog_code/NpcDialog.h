#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/local_code/ControlsRect.h"

class W8TextBuffer;

/* +0x0e of an option-list request points at an array of these records; the
   popup reads only the narrow display string. */
struct W8NpcDialogOption {
    int value;
    char* text;
};

/* The request buffer handed to the W8NpcDialog constructor; only the fields
   the popup reads are named. The dispatch code owns the full layout - for the
   0x12/0x1e price-check messages 0x01 carries the base price. */
#pragma pack(push, 1)
struct W8NpcDialogRequest {
    char opcode; /* 0x00: 0x05 option list, 0x12/0x1e price check, 0x13 keyword entry */
    char unknown_01[0xc];
    unsigned char option_count; /* 0x0d */
    W8NpcDialogOption* options; /* 0x0e */
};
#pragma pack(pop)

/* Dialog Code\NpcDialog.cpp. The NPC dialogue popup: the dispatch code
   allocates 0x270 bytes and hands the constructor a request buffer whose
   opcode byte selects the layout - 0x05 option list, 0x12/0x1e yes/no price
   check, 0x13 keyword text entry. The constructor publishes the instance in
   the file-scope singleton in NpcDialog.cpp; the option callback and the
   Enter-key handling in ProcessInput reach back through it. */
// VTABLE: WIZ8 0x005efa4c
class W8NpcDialog : public W8DialogBase {
public:
    W8NpcDialog(W8NpcDialogRequest* message, int aux_data); /* 0x005DA6B0 */
    virtual ~W8NpcDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual int GetDialogType() override; /* 0x005DB1A0: 7 */
    virtual unsigned char ProcessInput() override;

private:
    /* Copies the pressed option's user-data index into m_selected_option and
       closes the dialog. */
    static void OptionSelected(W8DialogButton* button); /* 0x005DB150 */

    W8TextBuffer* m_text_buffers[2]; /* 0x54 */
    W8DialogButton* m_buttons[3];    /* 0x5c */
    unsigned char m_selected_option; /* 0x68 */
    unsigned char unknown_069[3];
    W8NpcDialogRequest* m_message;   /* 0x6c */
    int m_aux_data;                  /* 0x70 */
    unsigned char m_compact_options; /* 0x74: option list may size under 200 wide */
    unsigned char unknown_075;
    unsigned short m_text_width; /* 0x76: widest option text + 6 */
    unsigned char m_input_field; /* 0x78: AddTextInputField id for opcode 0x13 */
    unsigned char unknown_079;
    wchar_t m_input_text[251]; /* 0x7a: fills the 0x270-byte allocation */
};
static_assert(sizeof(W8NpcDialog) == 0x270, "W8NpcDialog_size");
