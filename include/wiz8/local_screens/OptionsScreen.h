#pragma once

#include "wiz8/dialog_code/NotificationDialog.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/vector.h"

class W8OptionsPanel;

/* The 0x20-byte panel-set descriptor allocated by SelectPanel at 0x005A93C0.
   It is a real owning boundary: its embedded vector is constructed at +0x10,
   its deleting destructor is 0x005A9380, and the Options menu chrome stores a
   pointer to it under the asserted m_pMenuSet field.  Individual panel
   subclasses are created by the still-unrecovered table factory. */
class W8OptionsPanelSet005EF01C {
public:
    void Advance();
    void Retreat();

    int m_mode_000;
    int unknown_004;
    unsigned char unknown_008;
    unsigned char m_show_page_text_009;
    unsigned char unknown_00a;
    unsigned char pad_00b;
    int m_current_00c;
    W8GrowableVector<W8OptionsPanel*> m_panels_010;
};

static_assert(sizeof(W8OptionsPanelSet005EF01C) == 0x20,
              "W8OptionsPanelSet005EF01C_must_be_0x20");

/* The 0xc0-byte menu-row class constructed at 0x005A7370.  It is a concrete
   W8TextControl with an independent listener subobject and a source-table item
   id; the two optional child controls are owned by the base Controls panel. */
class W8OptionsMenuButton005EED3C
    : public W8TextControl005ED604,
      public W8TextControl005ED604::Listener {
public:
    W8OptionsMenuButton005EED3C(Controls* owner, const int* row);
    virtual void OnPrimary(W8TextControl005ED604* control) override;
    virtual void OnSecondary(W8TextControl005ED604* control) override;

    int m_item_id_0bc;
};

static_assert(sizeof(W8OptionsMenuButton005EED3C) == 0xc0,
              "W8OptionsMenuButton005EED3C_must_be_0xc0");

/* The 0x60-byte controls-derived menu-set object constructed at 0x005A8C90.
   Its independent allocation, constructor, secondary listener vptr, and the
   OptionsScreen.cpp assertion on m_pMenuSet establish this boundary. */
class W8OptionsMenuSet005EEFEC
    : public Controls,
      public W8TextControl005ED604::Listener {
public:
    W8OptionsMenuSet005EEFEC(unsigned int* shared_region_set);
    virtual ~W8OptionsMenuSet005EEFEC();
    virtual void OnPrimary(W8TextControl005ED604* control) override;
    virtual void OnSecondary(W8TextControl005ED604* control) override;

    W8OptionsPanelSet005EF01C* m_pMenuSet; /* 0x50: OptionsScreen.cpp:1481 */
    W8TextControl005ED604* m_next_054;
    W8TextControl005ED604* m_previous_058;
    W8TextBuffer005ED5B8* m_page_text_05c;

    void UpdateMenuSet();
};

static_assert(sizeof(W8OptionsMenuSet005EEFEC) == 0x60,
              "W8OptionsMenuSet005EEFEC_must_be_0x60");

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
    W8OptionsScreen();
    ~W8OptionsScreen();
    void SelectPanel(int selected, int notify);
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
    W8OptionsMenuSet005EEFEC* m_menu_set_028;
    void* m_panel_02c;
    unsigned char unknown_030[8];
    void* m_panel_038[6];
    void* m_panel_050;
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
