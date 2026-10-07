#pragma once

#include "wiz8/engine_code/Quality.h"
#include "wiz8/mgs_commands.h"
#include "wiz8/layouts/screen_state.h"

#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/ControlSelection.h"
#include "wiz8/local_code/RangeControl.h"
#include "wiz8/dialog_code/NotificationDialog.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/ControlsRect.h"
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
    W8OptionsSaveRow(Controls* owner, int top, unsigned char save_mode);
    void SetSave(W8SaveSlot* save);
    virtual void Redraw(bool full_redraw) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnLeftButtonDoubleClick(int event) override;

    unsigned char m_save_mode;
    bool m_editing;
    unsigned char pad_0ba[2];
    W8SaveSlot* m_save;
    W8OptionsSaveRowListener* m_save_listener;
};

static_assert(sizeof(W8OptionsSaveRow) == 0xc4, "W8OptionsSaveRow_size");

/* The settings transfer routines at 005A6E20/005A72F0/005A7320 share this
   receiver. The trailing direction flag is part of the record, not a global.
   Float loads/stores distinguish the slider values from the integer options. */
struct W8OptionsValues {
    void TransferSettings();
    void TransferByte(int* value, unsigned char* setting);
    void TransferRenderOption(int* value, W8RenderOption option);

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
    int stop_movement_for_events; /* UI checkbox 0x801; TransferSettings does not persist it */
    int skill_increase_messages;
    float combat_speed;
    float monster_movement_speed;
    float text_display_delay_ms;
    int auto_save;
    float gamma;
    int render_options[9];
    int monster_shadows;
    int smooth_monster_animations;
    int smooth_world_animations;
    int pc_confirmations;
    int pc_subtitles;
    int applying;
};

static_assert(sizeof(W8OptionsValues) == 0x90, "W8OptionsValues_size");
extern W8OptionsValues g_options_values;

/* A set of option panels; it owns the panels in its vector. */
class W8OptionsPanelSet {
public:
    W8OptionsPanelSet();
    ~W8OptionsPanelSet();
    void Advance();
    void Retreat();

    int m_page_count;
    int unknown_004;
    unsigned char m_compact_layout;
    unsigned char m_hide_navigation;
    bool m_active; /* set once the set's panels are built */
    int m_current;
    W8Vector<W8OptionsPanel*> m_panels;
};

static_assert(sizeof(W8OptionsPanelSet) == 0x20, "W8OptionsPanelSet_must_be_0x20");

/* Widgets bound to an option value; their input handlers store through the
   value pointer. */
// VTABLE: WIZ8 0x005eed8c
class W8OptionsCheckbox : public W8TextControl {
public:
    W8OptionsCheckbox(Controls* owner, int top, int* value);
    virtual void OnLeftButtonUp(int event) override;
    int* m_value;
};

// VTABLE: WIZ8 0x005eef20
class W8OptionsSlider : public W8HorizontalRangeThumb {
public:
    W8OptionsSlider(Controls* owner, int top, float* value, bool alternate);
    virtual void OnMouseMove(int event) override;
    virtual void Redraw(bool full_redraw) override;
    float* m_value;
};

// VTABLE: WIZ8 0x005eefd4
class W8OptionsSelection : public W8ControlSelection {
public:
    explicit W8OptionsSelection(int* value);
    virtual ~W8OptionsSelection();
    virtual void OnPrimary(W8TextControl* control) override;
    int* m_value;
};

static_assert(sizeof(W8OptionsCheckbox) == 0xbc, "W8OptionsCheckbox_size");
static_assert(sizeof(W8OptionsSlider) == 0x74, "W8OptionsSlider_size");
static_assert(sizeof(W8OptionsSelection) == 0x28, "W8OptionsSelection_size");

/* Common base of the concrete option panels. */
// VTABLE: WIZ8 0x005eefa8
class W8OptionsPanel : public Controls {
public:
    W8OptionsPanel(int region_index);
    virtual ~W8OptionsPanel();
    virtual void Redraw() override;
    virtual void Populate() = 0;
    virtual void SetActive(bool active);
    virtual void SetCurrent(int current);
    W8OptionsCheckbox* AddCheckbox(int label, int* value);
    W8TextControl* AddChoiceButton(int label);
    W8OptionsSlider* AddSlider(int label, float* value, bool alternate);
    void AddChoices(int label, int count, const int* choices, int* value);

    int m_current;
    int m_content_top;
    int unknown_054;
    W8Vector<W8TextBuffer*> m_text_buffers;
    W8Vector<W8OptionsSelection*> m_option_selections;
};

static_assert(sizeof(W8OptionsPanel) == 0x78, "W8OptionsPanel_must_be_0x78");

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

// VTABLE: WIZ8 0x005ef134 W8OptionsPanel
// VTABLE: WIZ8 0x005ef12c W8HorizontalRangeThumbListener
// VTABLE: WIZ8 0x005ef124 W8TextControl::Listener
class W8OptionsInterfacePanel : public W8OptionsPanel,
                                public W8HorizontalRangeThumbListener,
                                public W8TextControl::Listener {
public:
    W8OptionsInterfacePanel();
    virtual void Populate() override;
    virtual void OnDrag(W8HorizontalRangeThumb*) override {}
    virtual void OnDragEnd(W8HorizontalRangeThumb* thumb) override;
    virtual void OnPrimary(W8TextControl* control) override;
    W8OptionsSlider* m_tooltip_delay;
};

// VTABLE: WIZ8 0x005ef108 W8OptionsPanel
// VTABLE: WIZ8 0x005ef100 W8HorizontalRangeThumbListener
// VTABLE: WIZ8 0x005ef0f8 W8TextControl::Listener
class W8OptionsAudioPanel : public W8OptionsPanel,
                            public W8HorizontalRangeThumbListener,
                            public W8TextControl::Listener {
public:
    W8OptionsAudioPanel();
    virtual void Populate() override;
    virtual void OnDrag(W8HorizontalRangeThumb* thumb) override;
    virtual void OnDragEnd(W8HorizontalRangeThumb* thumb) override;
    virtual void OnPrimary(W8TextControl* control) override;
    W8OptionsSlider* m_sliders[4];
    W8TextControl* m_mute_buttons[4];
};

// VTABLE: WIZ8 0x005ef0dc W8OptionsPanel
// VTABLE: WIZ8 0x005ef0d4 W8HorizontalRangeThumbListener
class W8OptionsGraphicsPanel : public W8OptionsPanel, public W8HorizontalRangeThumbListener {
public:
    W8OptionsGraphicsPanel();
    virtual void Populate() override;
    virtual void OnDrag(W8HorizontalRangeThumb* thumb) override;
    virtual void OnDragEnd(W8HorizontalRangeThumb*) override;
};

// VTABLE: WIZ8 0x005ef0b8
class W8OptionsAdvancedGraphicsPanel : public W8OptionsPanel {
public:
    W8OptionsAdvancedGraphicsPanel();
    virtual void Populate() override;
};

/* The plain options button (Reset Defaults); key rows derive from it. */
// VTABLE: WIZ8 0x005eee80
class W8OptionsButton : public W8TextControl {
public:
    W8OptionsButton(Controls* owner, int left, int top, int right, int bottom, const wchar_t* text);
    virtual void Redraw(bool full_redraw) override;
    virtual void OnMouseEnter(int event) override;
    virtual void OnMouseLeave(int event) override;
};

// VTABLE: WIZ8 0x005eeed0
class W8OptionsKeyButton : public W8OptionsButton {
public:
    W8OptionsKeyButton(Controls* owner, int top, W8MGSCommand primary_binding,
                       W8MGSCommand secondary_binding);

    void SetKey(unsigned short key);
    void SetKeyText(unsigned short key);

    W8MGSCommand m_primary_binding;
    W8MGSCommand m_secondary_binding;
};

static_assert(sizeof(W8OptionsButton) == 0xb8, "W8OptionsButton_size");
static_assert(sizeof(W8OptionsKeyButton) == 0xc0, "W8OptionsKeyButton_size");

// VTABLE: WIZ8 0x005ef034 W8OptionsPanel
// VTABLE: WIZ8 0x005ef02c W8TextControl::Listener
// VTABLE: WIZ8 0x005ef028 W8ControlSelectionListener
// VTABLE: WIZ8 0x005ef024 W8OptionsKeyCapture
// VTABLE: WIZ8 0x005ef020 W8DialogCloseListener
class W8OptionsKeyboardPanel : public W8OptionsPanel,
                               public W8TextControl::Listener,
                               public W8ControlSelectionListener,
                               public W8OptionsKeyCapture,
                               public W8DialogCloseListener {
public:
    explicit W8OptionsKeyboardPanel(int panel);
    virtual void Populate() override;
    virtual void Invalidate(const W8ControlsRect* bounds) override;
    virtual void SetActive(bool active) override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSelectionChanged(W8ControlSelection* selection, int selected) override;
    virtual unsigned char OnKey(unsigned short key, unsigned short modifiers) override;
    virtual void OnDialogClosed(bool accepted, int value) override;
    void ClearDuplicateBinding(unsigned short key);
    void RefreshBindingLabels();
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

// VTABLE: WIZ8 0x005ef070 W8OptionsPanel
// VTABLE: WIZ8 0x005ef068 W8TextControl::Listener
// VTABLE: WIZ8 0x005ef064 W8DialogCloseListener
// VTABLE: WIZ8 0x005ef060 W8OptionsTextEditor::Listener
// VTABLE: WIZ8 0x005ef058 W8OptionsSaveRowListener
// VTABLE: WIZ8 0x005ef054 W8ControlSelectionListener
class W8OptionsSaveLoadPanel : public W8OptionsPanel,
                               public W8TextControl::Listener,
                               public W8DialogCloseListener,
                               public W8OptionsTextEditor::Listener,
                               public W8OptionsSaveRowListener,
                               public W8ControlSelectionListener {
public:
    explicit W8OptionsSaveLoadPanel(int panel);
    virtual void Populate() override;
    virtual void SetActive(bool active) override;
    virtual void SetCurrent(int current) override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnDialogClosed(bool accepted, int value) override;
    virtual void OnTextEditComplete(W8OptionsTextEditor* editor, unsigned char cancelled) override;
    virtual void OnEditSaveName(W8OptionsSaveRow* row) override;
    virtual void OnActivateSave(W8OptionsSaveRow* row) override;
    virtual void OnSelectionChanged(W8ControlSelection* selection, int selected) override;
    void DeleteSelectedSave();
    void LoadSelectedSave();
    void SaveSelectedSave();
    int m_panel;
    W8Vector<W8OptionsSaveRow*> m_rows;
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
W8_ASSERT_BASE_END(W8OptionsInterfacePanel, W8TextControl::Listener, m_tooltip_delay, 0x7c);
W8_ASSERT_BASE_END(W8OptionsAudioPanel, W8TextControl::Listener, m_sliders, 0x7c);
W8_ASSERT_BASE_TAIL(W8OptionsGraphicsPanel, W8HorizontalRangeThumbListener, 0x78);
W8_ASSERT_BASE_END(W8OptionsKeyboardPanel, W8DialogCloseListener, m_panel, 0x84);
W8_ASSERT_BASE_END(W8OptionsSaveLoadPanel, W8ControlSelectionListener, m_panel, 0x88);

/* One row of the main options menu table: the menu item id, two optional
   child text controls (left == -1 when absent), the button bounds, and the
   button's W8TextControl image indices. */
struct W8OptionsMenuRow {
    int item_id;
    W8ControlsRect child_bounds[2];
    W8ControlsRect bounds;
    int image_indices[5];
};
static_assert(sizeof(W8OptionsMenuRow) == 0x48, "W8OptionsMenuRow_must_be_0x48");

/* Inclusive range of CreateOptionsPanel indices shown by one menu item. */
struct W8OptionsPanelRange {
    int first;
    int last;
};

/* One options menu row. */
// VTABLE: WIZ8 0x005eed3c W8TextControl
// VTABLE: WIZ8 0x005eed34 W8TextControl::Listener
class W8OptionsMenuButton : public W8TextControl, public W8TextControl::Listener {
public:
    W8OptionsMenuButton(Controls* owner, const W8OptionsMenuRow* row);
    virtual void Redraw(bool full_redraw) override;
    virtual void EnableSecondaryState(bool immediate) override;
    virtual void OnPrimary(W8TextControl* control) override;

    int m_item_id;
};

static_assert(sizeof(W8OptionsMenuButton) == 0xc0, "W8OptionsMenuButton_must_be_0xc0");
W8_ASSERT_BASE_END(W8OptionsMenuButton, W8TextControl::Listener, m_item_id, 0xb8);

/* The options menu: its rows and the panel set they select. */
// VTABLE: WIZ8 0x005eefec Controls
// VTABLE: WIZ8 0x005eefe4 W8TextControl::Listener
class W8OptionsMenuSet : public Controls, public W8TextControl::Listener {
public:
    W8OptionsMenuSet(unsigned int* shared_region_set);
    virtual ~W8OptionsMenuSet();
    virtual void Redraw() override;
    virtual void OnPrimary(W8TextControl* control) override;

    W8OptionsPanelSet* m_pMenuSet; /* OptionsScreen.cpp:1481 */
    W8TextControl* m_next;
    W8TextControl* m_previous;
    W8TextBuffer* m_page_text;

    void UpdateMenuSet();
};

static_assert(sizeof(W8OptionsMenuSet) == 0x60, "W8OptionsMenuSet_must_be_0x60");
W8_ASSERT_BASE_END(W8OptionsMenuSet, W8TextControl::Listener, m_pMenuSet, 0x4c);

// VTABLE: WIZ8 0x005ef008 W8ControlSelectionListener
// VTABLE: WIZ8 0x005ef000 W8TextControl::Listener
// VTABLE: WIZ8 0x005eeffc W8DialogCloseListener
class W8OptionsScreen : public W8ControlSelectionListener,
                        public W8TextControl::Listener,
                        public W8DialogCloseListener {
public:
    W8OptionsScreen();
    ~W8OptionsScreen();
    void SelectPanel(int selected, bool notify);
    void CreateControls();
    unsigned char ProcessInput(const InputAtom* input);
    void Redraw();
    void ShowNotification(W8DialogCloseListener* listener, bool allow_cancel, int message,
                          int value);
    void BeginSaveNameEdit(W8OptionsTextEditor::Listener* listener, int row, const wchar_t* text);
    virtual void OnSelectionChanged(W8ControlSelection* control, int selected) override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl*) override {}
    virtual void OnDialogClosed(bool accepted, int value) override;

    W8Vector<W8SaveSlot*> m_save_slots;
    bool m_redraw_pending;
    bool m_modal_closing;
    unsigned char padding_01e[2];
    int m_selected_panel;
    Controls* m_controls;
    W8OptionsMenuSet* m_menu_set;
    W8ControlSelection* m_menu_selection;
    unsigned char unknown_030[8];
    W8OptionsPanelSet* m_panel[8];
    W8MessageDialogBase* m_active_modal; /* frame/leave own and clear it */
    W8OptionsTextEditor* m_text_editor;
    W8OptionsKeyCapture* m_key_capture;
};

static_assert(sizeof(W8OptionsScreen) == 0x64, "W8OptionsScreen_must_be_0x64");
W8_ASSERT_BASE_END(W8OptionsScreen, W8DialogCloseListener, m_save_slots, 0x8);

extern W8OptionsScreen* g_options_screen;
extern wchar_t g_options_last_save_name[64];

void SetLastSaveName(const wchar_t* target);
wchar_t* GetLastSaveName(void);

unsigned char OptionsScreenInitialize(void);
unsigned char OptionsScreenEnter(void);
void OptionsScreenFrame(void);
unsigned char OptionsScreenLeave(int leaving);
unsigned char OptionsScreenFinalize(void);
extern wchar_t g_empty_wide_string;
