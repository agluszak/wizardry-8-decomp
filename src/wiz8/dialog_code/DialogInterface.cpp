#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/sr_api.h"

/*
 * Dialog Code\DialogInterface.cpp, named by the assertion this body embeds at
 * line 152.
 *
 * The forwarders below operate on the shared dialog base recovered elsewhere:
 * slot three remains positional, slot nine is Close, and the setter establishes
 * the base field at 0x44. The assertion does not return, which is why each
 * operation remains unconditional after its null check.
 */

extern "C" {

int g_dialog_font_64fde8;
unsigned char g_dialog_font_enabled_69ca32;
unsigned char g_dialog_font_foreground_64fdec;
unsigned char g_dialog_font_background_64fded;

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
    return dialog->Close();
}

// FUNCTION: WIZ8 0x005cf580
void Function5CF580(W8DialogBase005DC7A0* dialog, int value)
{
    if (dialog == 0) {
        srAssertFail("pCDialog", DIALOG_INTERFACE_CPP, 0x98, 0);
    }
    dialog->m_field_44 = value;
}

}
