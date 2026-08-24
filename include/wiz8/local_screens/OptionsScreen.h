#pragma once

#include "wiz8/dialog_code/NotificationDialog.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/vector.h"

/* Local Screens\OptionsScreen.cpp owns the state-10 controller.  Its source
   identity is established by the m_pMenuSet assertion at 0x005A8F14; the
   0x64-byte allocation at 0x005A9B50, constructor/destructor pair
   0x005A9090/0x005A9200, and the three callback-table receivers establish the
   object extent and bases.  The constructor establishes the ordinary pointer
   vector at +0x0c, the selected panel index at +0x20, and the controls owner
   at +0x24; the remaining panel objects stay positional until their types are
   recovered. */
class W8OptionsScreen
    : public W8ControlSelectionListener,
      public W8TextControl005ED604::Listener,
      public W8DialogCloseListener {
public:
    // FUNCTION: WIZ8 0x005a9090
    W8OptionsScreen();
    // FUNCTION: WIZ8 0x005a9200
    ~W8OptionsScreen();
    // FUNCTION: WIZ8 0x005a93c0
    void SelectPanel(int selected, int notify);
    // FUNCTION: WIZ8 0x005a98b0
    void CreateControls();
    virtual void vslot00(W8Control005ED654* control, int selected) override;
    virtual void OnPrimary(W8TextControl005ED604* control) override;
    virtual void OnSecondary(W8TextControl005ED604*) override {}
    virtual void OnDialogClosed(unsigned char reason, int value) override;

    W8GrowableVector<void*> m_owned_panels_00c;
    unsigned char m_input_pending_01c;
    unsigned char m_modal_closing_01d;
    unsigned char unknown_01e[2];
    int m_selected_panel_020;
    Controls* m_controls_024;
    void* m_panel_028;
    void* m_panel_02c;
    unsigned char unknown_030[8];
    void* m_panel_038[6];
    void* m_pMenuSet;       /* 0x50: asserted by OptionsScreen.cpp:1481 */
    void* m_panel_054;
    void* m_active_modal;   /* 0x58: frame/leave own and clear it */
    void* m_panel_05c;
    void* m_panel_060;
};

static_assert(sizeof(W8OptionsScreen) == 0x64,
              "W8OptionsScreen_must_be_0x64");

extern W8OptionsScreen* g_options_screen_0069c254;

unsigned char OptionsScreenEnter005A9B50();
unsigned char OptionsScreenLeave005A9C70(int leaving);
void OptionsScreenFrame005A9CC0();
