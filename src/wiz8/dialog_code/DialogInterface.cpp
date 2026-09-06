#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_base.h"
#include "wiz8/sr_api.h"

extern wchar_t g_split_item_dialog_text_689b34[];

/*
 * Dialog Code\DialogInterface.cpp, named by the assertion this body embeds at
 * line 152.
 *
 * The forwarders below operate on the shared dialog base recovered elsewhere:
 * slot three draws, slot nine processes input, and the setter establishes
 * the base field at 0x44. The assertion does not return, which is why each
 * operation remains unconditional after its null check.
 */

extern "C" {

int g_dialog_font_64fde8;
unsigned char g_dialog_font_enabled_69ca32;
unsigned char g_dialog_font_foreground_64fdec;
unsigned char g_dialog_font_background_64fded;

// FUNCTION: WIZ8 0x005cf300
W8DialogBase005DC7A0* Function5CF300(int kind)
{
    W8DialogBase005DC7A0* dialog;

    switch (kind) {
    case 0:
        dialog = new W8DialogBase005DC7A0;
        dialog->vslot5(L"Test Dialog");
        dialog->SetOrigin(160, 120);
        dialog->SetExtent(320, 240);
        break;
    case 1:
        dialog = new W8ModalDialogBase;
        dialog->SetOrigin(240, 190);
        dialog->SetExtent(160, 100);
        dialog->SetBackground("Data\\Dialogs\\DialogBackground.sti", 0);
        return dialog;
    case 3:
        dialog = new W8Dialog005CBB40;
        dialog->vslot5(L"ListBox Dialog");
        dialog->SetOrigin(200, 100);
        dialog->SetExtent(240, 280);
        break;
    case 5:
        dialog = new W8Dialog005D97D0;
        dialog->vslot5(g_split_item_dialog_text_689b34);
        dialog->SetOrigin(159, 184);
        return dialog;
    default:
        return 0;
    }
    dialog->SetBackground("Data\\Dialogs\\DialogBackground.sti", 0);
    return dialog;
}

// FUNCTION: WIZ8 0x005cf250
void Function5CF250(int font, unsigned char enabled,
                    unsigned char foreground, unsigned char background)
{
    g_dialog_font_64fde8 = font;
    g_dialog_font_enabled_69ca32 = enabled;
    g_dialog_font_foreground_64fdec = foreground;
    g_dialog_font_background_64fded = background;
}

#define DIALOG_INTERFACE_CPP "C:\\Projects\\Wizardry 8\\Dialog Code\\DialogInterface.cpp"

// FUNCTION: WIZ8 0x005cf520
void Function5CF520(W8DialogBase005DC7A0* dialog)
{
    if (dialog == 0) {
        srAssertFail("pDialog", DIALOG_INTERFACE_CPP, 0x66, 0);
    }
    dialog->vslot3();
}

// FUNCTION: WIZ8 0x005cf550
unsigned char Function5CF550(W8DialogBase005DC7A0* dialog)
{
    if (dialog == 0) {
        srAssertFail("pDialog", DIALOG_INTERFACE_CPP, 0x74, 0);
    }
    return dialog->ProcessInput();
}

// FUNCTION: WIZ8 0x005cf580
void Function5CF580(
    W8DialogBase005DC7A0* dialog, W8DialogDestroyCallback callback)
{
    if (dialog == 0) {
        srAssertFail("pCDialog", DIALOG_INTERFACE_CPP, 0x98, 0);
    }
    dialog->m_destroy_callback = callback;
}

}
