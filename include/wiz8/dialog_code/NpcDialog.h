#pragma once

extern wchar_t g_format_S[];

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/npc_script_file.h"

class W8TextBuffer;

/* The dialog receives the script entry itself. Its sub-entry array is the
   option list for kind 5; operand0 is the price for kinds 0x12/0x1e. */

/* Dialog Code\NpcDialog.cpp. The NPC dialogue popup: the dispatch code
   allocates 0x270 bytes and hands the constructor a request buffer whose
   opcode byte selects the layout - 0x05 option list, 0x12/0x1e yes/no price
   check, 0x13 keyword text entry. The constructor publishes the instance in
   the file-scope singleton in NpcDialog.cpp; the option callback and the
   Enter-key handling in ProcessInput reach back through it. */
// VTABLE: WIZ8 0x005efa4c
class W8NpcDialog : public W8DialogBase {
    friend void OnNpcDialogClosed(W8DialogBase* dialog);

public:
    W8NpcDialog(W8NpcQuoteEntry* message, int aux_data);
    virtual ~W8NpcDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual W8DialogKind GetDialogType() override; /* 7 */
    virtual bool ProcessInput() override;

private:
    /* Copies the pressed option's user-data index into m_selected_option and
       closes the dialog. */
    static void OptionSelected(W8DialogButton* button);

    W8TextBuffer* m_text_buffers[2];
    W8DialogButton* m_buttons[3];
    signed char m_selected_option;
    W8NpcQuoteEntry* m_message;
    int m_aux_data;
    bool m_compact_options; /* option list may size under 200 wide */
    unsigned char unknown_075;
    unsigned short m_text_width; /* widest option text + 6 */
    unsigned char m_input_field; /* AddTextInputField id for opcode 0x13 */
    unsigned char unknown_079;
    wchar_t m_input_text[251]; /* fills the 0x270-byte allocation */
};
static_assert(sizeof(W8NpcDialog) == 0x270, "W8NpcDialog_size");
