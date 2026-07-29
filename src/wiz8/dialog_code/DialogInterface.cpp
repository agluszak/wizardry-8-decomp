#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/*
 * Dialog Code\DialogInterface.cpp, named by the assertion this body embeds at
 * line 152.
 *
 * The forwarders below establish virtual slots three and nine, while the
 * setter establishes the field at 0x44. Everything between stays opaque. The
 * assertion does not return, which is why each operation remains unconditional
 * after its null check.
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

class W8DialogInterface {
public:
    virtual void Method0();
    virtual void Method1();
    virtual void Method2();
    virtual void Method3();
    virtual void Method4();
    virtual void Method5();
    virtual void Method6();
    virtual void Method7();
    virtual void Method8();
    virtual void Method9();

    unsigned char unknown_04[0x40];
    int value_44;                         /* 0x44: the only field this proves */
};

static_assert(sizeof(W8DialogInterface) == 0x48,
              "W8DialogInterface_must_be_0x48");

#define DIALOG_INTERFACE_CPP "C:\\Projects\\Wizardry 8\\Dialog Code\\DialogInterface.cpp"

/* Two interface forwarders. Their slot positions, not semantic names, are the
   reviewed fact, so the methods remain positional. */
// FUNCTION: WIZ8 0x005cf520
void Function5CF520(W8DialogInterface* dialog)
{
    if (dialog == 0) {
        srAssertFail("pDialog", DIALOG_INTERFACE_CPP, 0x66, 0);
    }
    dialog->Method3();
}

// FUNCTION: WIZ8 0x005cf550
void Function5CF550(W8DialogInterface* dialog)
{
    if (dialog == 0) {
        srAssertFail("pDialog", DIALOG_INTERFACE_CPP, 0x74, 0);
    }
    dialog->Method9();
}

// FUNCTION: WIZ8 0x005cf580
void Function5CF580(W8DialogInterface* dialog, int value)
{
    if (dialog == 0) {
        srAssertFail("pCDialog", DIALOG_INTERFACE_CPP, 0x98, 0);
    }
    dialog->value_44 = value;
}

}
