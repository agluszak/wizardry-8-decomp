#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/*
 * Dialog Code\DialogInterface.cpp, named by the assertion this body embeds at
 * line 152.
 *
 * Only the one field is established here, so the dialog is modelled as exactly
 * that and nothing more. The assertion does not return, which is why the store
 * below is unconditional even though the null case reaches it.
 */

extern "C" {

int g_dialog_font_64fde8;
unsigned char g_dialog_font_enabled_69ca32;
unsigned char g_dialog_font_foreground_64fdec;
unsigned char g_dialog_font_background_64fded;

// FUNCTION: WIZ8 0x005CF250
void Function5CF250(int font, unsigned char enabled,
                    unsigned char foreground, unsigned char background)
{
    g_dialog_font_64fde8 = font;
    g_dialog_font_enabled_69ca32 = enabled;
    g_dialog_font_foreground_64fdec = foreground;
    g_dialog_font_background_64fded = background;
}

typedef struct W8DialogInterface {
    unsigned char unknown_00[0x44];
    int value_44;                         /* 0x44: the only field this proves */
} W8DialogInterface;

#define DIALOG_INTERFACE_CPP "C:\\Projects\\Wizardry 8\\Dialog Code\\DialogInterface.cpp"

// FUNCTION: WIZ8 0x005CF580
void Function5CF580(W8DialogInterface* dialog, int value)
{
    if (dialog == 0) {
        srAssertFail("pCDialog", DIALOG_INTERFACE_CPP, 0x98, 0);
    }
    dialog->value_44 = value;
}

}
