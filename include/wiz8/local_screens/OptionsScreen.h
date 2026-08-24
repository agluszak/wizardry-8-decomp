#pragma once

#include "wiz8/dialog_code/NotificationDialog.h"
#include "wiz8/local_code/Controls.h"

/* Local Screens\OptionsScreen.cpp owns the state-10 controller.  Its source
   identity is established by the m_pMenuSet assertion at 0x005A8F14; the
   0x64-byte allocation at 0x005A9B50, constructor/destructor pair
   0x005A9090/0x005A9200, and the three callback-table receivers establish the
   object extent and bases.  Only the two fields independently read by the
   recovered frame/update paths are named here. */
class W8OptionsScreen
    : public W8ControlSelectionListener,
      public W8TextControl005ED604::Listener,
      public W8DialogCloseListener {
public:
    virtual void vslot00(W8Control005ED654* control, int selected) override;
    virtual void OnPrimary(W8TextControl005ED604* control) override;
    virtual void OnSecondary(W8TextControl005ED604*) override {}
    virtual void OnDialogClosed(unsigned char reason, int value) override;

    unsigned char unknown_00c[0x44];
    void* m_pMenuSet;       /* 0x50: asserted by OptionsScreen.cpp:1481 */
    unsigned char unknown_054[4];
    void* m_active_modal;   /* 0x58: frame/leave own and clear it */
    unsigned char unknown_05c[8];
};

static_assert(sizeof(W8OptionsScreen) == 0x64,
              "W8OptionsScreen_must_be_0x64");

extern W8OptionsScreen* g_options_screen_0069c254;

unsigned char OptionsScreenEnter005A9B50();
unsigned char OptionsScreenLeave005A9C70(int leaving);
void OptionsScreenFrame005A9CC0();
