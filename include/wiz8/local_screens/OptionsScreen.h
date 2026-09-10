#pragma once
#include "wiz8/screen_state.h"

#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/ControlSelection.h"
#include "wiz8/local_code/RangeControl.h"
#include "wiz8/dialog_code/NotificationDialog.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/vector.h"
#include "wiz8/text_input.h"

class W8OptionsPanel;
class W8OptionsSaveRow;

// VTABLE: WIZ8 0x005eef68
class W8OptionsTextEditor {
public:
    class Listener {
    public:
        virtual void OnTextEditComplete(W8OptionsTextEditor* editor, unsigned char cancelled) = 0;
    };
    virtual ~W8OptionsTextEditor()
    {
        RemoveTextInputField(0);
        KillTextInputMode();
    }
    Listener* m_listener;
};

static_assert(sizeof(W8OptionsTextEditor) == 8, "W8OptionsTextEditor_size");

class W8OptionsKeyCapture {
public:
    virtual unsigned char OnKey(unsigned short key, unsigned short modifiers) = 0;
};

class W8OptionsSaveRowListener {
public:
    virtual void OnEditSaveName(W8OptionsSaveRow* row) = 0;
    virtual void OnActivateSave(W8OptionsSaveRow* row) = 0;
};

// VTABLE: WIZ8 0x005eee2c
class W8OptionsSaveRow : public W8TextControl {
public:
    __forceinline W8OptionsSaveRow(Controls* owner, int top,
                                  unsigned char save_mode);
    virtual ~W8OptionsSaveRow() override;
    virtual void Redraw(int full_redraw) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnLeftButtonDoubleClick(int event) override;

    unsigned char m_save_mode;
    unsigned char m_editing;
    unsigned char pad_0ba[2];
    W8SaveSlot* m_save;
    W8OptionsSaveRowListener* m_listener;
};

static_assert(sizeof(W8OptionsSaveRow) == 0xc4, "W8OptionsSaveRow_size");

/* The settings transfer routines at 005A6E20/005A72F0/005A7320 share this
   receiver. The trailing direction flag is part of the record, not a global.
   Float loads/stores distinguish the slider values from the integer options. */
struct W8OptionsValues {
    void TransferSettings();
    void TransferByte(int* value, unsigned char* setting);
    void TransferRenderOption(int* value, int option);

    int mouselook_toggle;
    int mouselook_smoothing;
    int invert_mouse_y;
    int ctrl_right_click_info;
    int difficulty;
    int camera_auto_rotation;
    int continuous_combat;
    int numeric_hit_points;
    int autoscroll_combat_messages;
    int simplified_npc_interaction;
    int verbose_combat_messages;
    int autoswap_weapons;
    int auto_advance_character;
    int autotarget_spells;
    int stop_movement_for_events;
    int skill_increase_messages;
    float combat_speed;
    float monster_movement_speed;
    float text_display_delay_ms;
    int auto_save;
    float gamma;
    int render_options[9];
    int values_078[3];
    int value_084;
    int value_088;
    int applying;
};

static_assert(sizeof(W8OptionsValues) == 0x90, "W8OptionsValues_size");
extern W8OptionsValues g_options_values;

/* The 0x20-byte panel-set descriptor allocated by SelectPanel at 0x005A93C0.
   Its vector at +0x10 owns panels. The descriptor itself has no vtable. */
class W8OptionsPanelSet {
public:
    W8OptionsPanelSet();
    ~W8OptionsPanelSet();
    void Advance();
    void Retreat();

    int m_mode_000;
    int unknown_004;
    unsigned char m_compact_layout;
    unsigned char m_hide_navigation;
    unsigned char m_active;
    unsigned char pad_00b;
    int m_current_00c;
    W8GrowableVector<W8OptionsPanel*> m_panels_010;
};

static_assert(sizeof(W8OptionsPanelSet) == 0x20,
              "W8OptionsPanelSet_must_be_0x20");

/* These controls add actual value bindings to the canonical widget classes:
   a checkbox integer at +0xb8, slider float at +0x70, and selection integer
   at +0x24. Their input handlers store through those pointers. */
// VTABLE: WIZ8 0x005eed8c
class W8OptionsCheckbox : public W8TextControl {
public:
    W8OptionsCheckbox(Controls* owner, int top, int* value);
    virtual ~W8OptionsCheckbox() override;
    virtual void OnLeftButtonUp(int event) override;
    int* m_value;
};

// VTABLE: WIZ8 0x005eef20
class W8OptionsSlider : public W8HorizontalRangeThumb {
public:
    W8OptionsSlider(Controls* owner, int top, float* value, unsigned char alternate);
    virtual ~W8OptionsSlider() override;
    virtual void OnMouseMove(int event) override;
    virtual void Redraw(int full_redraw) override;
    float* m_value;
};

// VTABLE: WIZ8 0x005eefd4
class W8OptionsSelection : public W8ControlSelection {
public:
    explicit W8OptionsSelection(int* value);
    virtual void OnPrimary(W8TextControl* control) override;
    int* m_value;
};

static_assert(sizeof(W8OptionsCheckbox) == 0xbc, "W8OptionsCheckbox_size");
static_assert(sizeof(W8OptionsSlider) == 0x74, "W8OptionsSlider_size");
static_assert(sizeof(W8OptionsSelection) == 0x28, "W8OptionsSelection_size");

/* All concrete option panels share this 0x78-byte Controls-derived base.  Its
   0x005A81E0 constructor establishes the two owning vectors and its vtable
   supplies the active-state and selected-index operations used by a panel set.
   The distinct concrete panel implementations remain in the table factory. */
// VTABLE: WIZ8 0x005eefa8
class W8OptionsPanel : public Controls {
public:
    W8OptionsPanel(int region_index);
    virtual ~W8OptionsPanel();
    virtual void Redraw() override;
    virtual void Populate() = 0;
    virtual void SetActive(unsigned char active);
    virtual void SetCurrent(int current);
    W8OptionsCheckbox* AddCheckbox(int label, int* value);
    W8TextControl* AddChoiceButton(int label);
    W8OptionsSlider* AddSlider(int label, float* value, unsigned char alternate);
    void AddChoices(int label, int count, const int* choices, int* value);

    int m_current_04c;
    int m_content_top_050;
    int unknown_054;
    W8GrowableVector<W8TextBuffer*> m_text_buffers_058;
    W8GrowableVector<W8OptionsSelection*> m_option_selections;
};

static_assert(sizeof(W8OptionsPanel) == 0x78,
              "W8OptionsPanel_must_be_0x78");

// VTABLE: WIZ8 0x005ef174
class W8OptionsGamePanel : public W8OptionsPanel {
public:
    W8OptionsGamePanel();
    virtual void Populate() override;
};

// VTABLE: WIZ8 0x005ef158
class W8OptionsMousePanel : public W8OptionsPanel {
public:
    W8OptionsMousePanel();
    virtual void Populate() override;
};

// VTABLE: WIZ8 0x005ef134
// VTABLE: WIZ8 0x005ef12c W8HorizontalRangeThumbListener
// VTABLE: WIZ8 0x005ef124 W8TextControl::Listener
class W8OptionsInterfacePanel : public W8OptionsPanel,
    public W8HorizontalRangeThumbListener, public W8TextControl::Listener {
public:
    W8OptionsInterfacePanel();
    virtual void Populate() override;
    virtual void OnDrag(W8HorizontalRangeThumb*) override {}
    virtual void OnDragEnd(W8HorizontalRangeThumb* thumb) override;
    virtual void OnPrimary(W8TextControl* control) override;
    W8OptionsSlider* m_tooltip_delay;
};

// VTABLE: WIZ8 0x005ef108
// VTABLE: WIZ8 0x005ef100 W8HorizontalRangeThumbListener
// VTABLE: WIZ8 0x005ef0f8 W8TextControl::Listener
class W8OptionsAudioPanel : public W8OptionsPanel,
    public W8HorizontalRangeThumbListener, public W8TextControl::Listener {
public:
    W8OptionsAudioPanel();
    virtual void Populate() override;
    virtual void OnDrag(W8HorizontalRangeThumb* thumb) override;
    virtual void OnDragEnd(W8HorizontalRangeThumb* thumb) override;
    virtual void OnPrimary(W8TextControl* control) override;
    W8OptionsSlider* m_sliders[4];
    W8TextControl* m_mute_buttons[4];
};

// VTABLE: WIZ8 0x005ef0dc
// VTABLE: WIZ8 0x005ef0d4 W8HorizontalRangeThumbListener
class W8OptionsGraphicsPanel : public W8OptionsPanel,
    public W8HorizontalRangeThumbListener {
public:
    W8OptionsGraphicsPanel();
    virtual void Populate() override;
    virtual void OnDrag(W8HorizontalRangeThumb* thumb) override;
    virtual void OnDragEnd(W8HorizontalRangeThumb*) override {}
};

// VTABLE: WIZ8 0x005ef0b8
class W8OptionsAdvancedGraphicsPanel : public W8OptionsPanel {
public:
    W8OptionsAdvancedGraphicsPanel();
    virtual void Populate() override;
};

/* The 0xb8-byte options button is instantiated directly for Reset Defaults.
   Key rows run the same inlined construction first, then install the derived
   vtable and two binding ids at +0xb8/+0xbc. */
// VTABLE: WIZ8 0x005eee80
class W8OptionsButton : public W8TextControl {
public:
    __forceinline W8OptionsButton(Controls* owner, int left, int top, int right,
                                 int bottom, const wchar_t* text);
    virtual ~W8OptionsButton() override;
    virtual void Redraw(int full_redraw) override;
    virtual void OnMouseEnter(int event) override;
    virtual void OnMouseLeave(int event) override;
};

// VTABLE: WIZ8 0x005eeed0
class W8OptionsKeyButton : public W8OptionsButton {
public:
    __forceinline W8OptionsKeyButton(Controls* owner, int top,
                                    int primary_binding, int secondary_binding);
    virtual ~W8OptionsKeyButton() override;

    void SetKey(unsigned short key);
    void SetKeyText(unsigned short key);

    int m_primary_binding;
    int m_secondary_binding;
};

static_assert(sizeof(W8OptionsButton) == 0xb8, "W8OptionsButton_size");
static_assert(sizeof(W8OptionsKeyButton) == 0xc0, "W8OptionsKeyButton_size");

// VTABLE: WIZ8 0x005ef034
// VTABLE: WIZ8 0x005ef02c W8TextControl::Listener
// VTABLE: WIZ8 0x005ef028 W8ControlSelectionListener
// VTABLE: WIZ8 0x005ef024 W8OptionsKeyCapture
// VTABLE: WIZ8 0x005ef020 W8DialogCloseListener
class W8OptionsKeyboardPanel : public W8OptionsPanel,
    public W8TextControl::Listener, public W8ControlSelectionListener,
    public W8OptionsKeyCapture, public W8DialogCloseListener {
public:
    explicit W8OptionsKeyboardPanel(int panel);
    virtual ~W8OptionsKeyboardPanel() override;
    virtual void Populate() override;
    virtual void Invalidate(const W8ControlsRect* bounds) override;
    virtual void SetActive(unsigned char active) override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSelectionChanged(W8ControlSelection* selection, int selected) override;
    virtual unsigned char OnKey(unsigned short key, unsigned short modifiers) override;
    virtual void OnDialogClosed(unsigned char reason, int value) override;
    void ClearDuplicateBinding(unsigned short key);
    int m_panel;
    W8ControlSelection m_selection;
    W8OptionsKeyButton* m_captured_button;
};

// VTABLE: WIZ8 0x005ef09c
class W8OptionsUnavailablePanel : public W8OptionsPanel {
public:
    explicit W8OptionsUnavailablePanel(int message);
    virtual void Populate() override;
    int m_message;
};

// VTABLE: WIZ8 0x005ef070
// VTABLE: WIZ8 0x005ef068 W8TextControl::Listener
// VTABLE: WIZ8 0x005ef064 W8DialogCloseListener
// VTABLE: WIZ8 0x005ef060 W8OptionsTextEditor::Listener
// VTABLE: WIZ8 0x005ef058 W8OptionsSaveRowListener
// VTABLE: WIZ8 0x005ef054 W8ControlSelectionListener
class W8OptionsSaveLoadPanel : public W8OptionsPanel,
    public W8TextControl::Listener, public W8DialogCloseListener,
    public W8OptionsTextEditor::Listener, public W8OptionsSaveRowListener,
    public W8ControlSelectionListener {
public:
    explicit W8OptionsSaveLoadPanel(int panel);
    virtual ~W8OptionsSaveLoadPanel() override;
    virtual void Populate() override;
    virtual void SetActive(unsigned char active) override;
    virtual void SetCurrent(int current) override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnDialogClosed(unsigned char reason, int value) override;
    virtual void OnTextEditComplete(W8OptionsTextEditor* editor, unsigned char cancelled) override;
    virtual void OnEditSaveName(W8OptionsSaveRow* row) override;
    virtual void OnActivateSave(W8OptionsSaveRow* row) override;
    virtual void OnSelectionChanged(W8ControlSelection* selection, int selected) override;
    void DeleteSelectedSave();
    void LoadSelectedSave();
    void SaveSelectedSave();
    int m_panel;
    W8GrowableVector<W8OptionsSaveRow*> m_rows;
    W8ControlSelection m_selection;
    W8TextControl* m_delete_button;
    W8TextControl* m_action_button;
    wchar_t m_previous_name[64];
    int m_editing_row;
};

static_assert(sizeof(W8OptionsInterfacePanel) == 0x84, "W8OptionsInterfacePanel_size");
static_assert(sizeof(W8OptionsAudioPanel) == 0xa0, "W8OptionsAudioPanel_size");
static_assert(sizeof(W8OptionsGraphicsPanel) == 0x7c, "W8OptionsGraphicsPanel_size");
static_assert(sizeof(W8OptionsKeyboardPanel) == 0xb4, "W8OptionsKeyboardPanel_size");
static_assert(sizeof(W8OptionsSaveLoadPanel) == 0x150, "W8OptionsSaveLoadPanel_size");

/* The 0xc0-byte menu-row class constructed at 0x005A7370.  It is a concrete
   W8TextControl with an independent listener subobject and a source-table item
   id; the two optional child controls are owned by the base Controls panel. */
// VTABLE: WIZ8 0x005eed3c
// VTABLE: WIZ8 0x005eed34 W8TextControl::Listener
class W8OptionsMenuButton
    : public W8TextControl,
      public W8TextControl::Listener {
public:
    W8OptionsMenuButton(Controls* owner, const int* row);
    virtual ~W8OptionsMenuButton() override;
    virtual void Redraw(int full_redraw) override;
    virtual void OnPrimary(W8TextControl* control) override;

    int m_item_id_0bc;
};

static_assert(sizeof(W8OptionsMenuButton) == 0xc0,
              "W8OptionsMenuButton_must_be_0xc0");

/* The 0x60-byte controls-derived menu-set object constructed at 0x005A8C90.
   Its independent allocation, constructor, secondary listener vptr, and the
   OptionsScreen.cpp assertion on m_pMenuSet establish this boundary. */
// VTABLE: WIZ8 0x005eefec
// VTABLE: WIZ8 0x005eefe4 W8TextControl::Listener
class W8OptionsMenuSet
    : public Controls,
      public W8TextControl::Listener {
public:
    W8OptionsMenuSet(unsigned int* shared_region_set);
    virtual ~W8OptionsMenuSet();
    virtual void Redraw() override;
    virtual void OnPrimary(W8TextControl* control) override;

    W8OptionsPanelSet* m_pMenuSet; /* 0x50: OptionsScreen.cpp:1481 */
    W8TextControl* m_next_054;
    W8TextControl* m_previous_058;
    W8TextBuffer* m_page_text_05c;

    void UpdateMenuSet();
};

static_assert(sizeof(W8OptionsMenuSet) == 0x60,
              "W8OptionsMenuSet_must_be_0x60");

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
      public W8TextControl::Listener,
      public W8DialogCloseListener {
public:
    W8OptionsScreen();
    ~W8OptionsScreen();
    void SelectPanel(int selected, unsigned char notify);
    void CreateControls();
    unsigned char ProcessInput(const InputAtom* input);
    void Redraw();
    void ShowNotification(W8DialogCloseListener* listener, int caption, int message, int value);
    void BeginSaveNameEdit(W8OptionsTextEditor::Listener* listener, int row,
                           const wchar_t* text);
    virtual void OnSelectionChanged(
        W8ControlSelection* control, int selected) override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl*) override {}
    virtual void OnDialogClosed(unsigned char reason, int value) override;

    W8GrowableVector<W8SaveSlot*> m_save_slots;
    unsigned char m_redraw_pending;
    unsigned char m_modal_closing_01d;
    unsigned char unknown_01e[2];
    int m_selected_panel_020;
    Controls* m_controls_024;
    W8OptionsMenuSet* m_menu_set_028;
    W8ControlSelection* m_menu_selection;
    unsigned char unknown_030[8];
    W8OptionsPanelSet* m_panel_038[8];
    W8ModalDialogBase* m_active_modal; /* 0x58: frame/leave own and clear it */
    W8OptionsTextEditor* m_text_editor;
    W8OptionsKeyCapture* m_key_capture;
};

static_assert(sizeof(W8OptionsScreen) == 0x64,
              "W8OptionsScreen_must_be_0x64");

extern W8OptionsScreen* g_options_screen_0069c254;
extern wchar_t g_options_last_save_name_0069c1cc[64];

void Function5A9E70(const wchar_t* target);
wchar_t* GetAddress69C1CC(void);

void ShowModalMessage005A6620(int a, int b, int c,
                                    void (*callback)(void), int d, int e);

