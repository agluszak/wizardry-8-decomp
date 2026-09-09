#include "Font.h"
#include "Types.h"
#include "mousesystem.h"
#include "wiz8/bringup_gates.h"
#include "wiz8/render_state.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/ControlSelection.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/xstatus.h"

#include "wiz8/combat_state.h"
#include "wiz8/cursor.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/geometry.h"
#include "wiz8/music_playlist.h"
#include "wiz8/regions.h"
#include "wiz8/screen_state.h"
#include "wiz8/fonts.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/text_input.h"
#include "wiz8/font_manager.h"
#include "wiz8/local_code/Strings.h"
#include "vsurface.h"
#include "himage.h"
#include "wiz8/input_hooks.h"

#include <string.h>

// GLOBAL: WIZ8 0x0069C130
unsigned int* g_options_panel_region_sets;
extern unsigned char g_flag_689b32;

void MSYS_SGP_Mouse_Handler_Hook(unsigned short event, unsigned short x, unsigned short y,
                    char right_button, char left_button);

/* Shared zero-initialized wide string: binary-wide DATA references read it as
   empty text and as a swprintf format argument. No recovered writer owns it
   yet; this definition only anchors the address until that function lands. */
// GLOBAL: WIZ8 0x00689b34
extern const wchar_t g_wchar_00689b34 = 0;

/* Layout flag handed to every row-registered child text control.  No other
   recovered site reads it yet; ownership stays with this constant until a
   writer shows a wider family. */
// GLOBAL: WIZ8 0x005ed590
extern const unsigned int g_W8TextControlLayoutMask005ED590 = 0x40;

// GLOBAL: WIZ8 0x0069c1c8
unsigned char g_options_first_frame;
// GLOBAL: WIZ8 0x0069c138
W8OptionsValues g_options_values;
// GLOBAL: WIZ8 0x0069c254
W8OptionsScreen* g_options_screen_0069c254;

// GLOBAL: WIZ8 0x0064d72c
int g_last_options_panel = 5;

// GLOBAL: WIZ8 0x0069c24c
unsigned int g_options_menu_region_set;
// GLOBAL: WIZ8 0x0069c250
unsigned int g_options_page_region_set;

// GLOBAL: WIZ8 0x0064d0b8
int g_options_menu_rows[8][18] = {
    {1, 20, 51, 69, 137, -1, -1, -1, -1, 16, 194, 0, 0, 0, 2, 1, -1, -1},
    {4, 90, 72, 117, 117, 217, 72, 244, 117, 16, 220, 0, 0, 3, 5, 4, -1, -1},
    {3, 132, 29, 201, 102, -1, -1, -1, -1, 16, 246, 0, 0, 6, 8, 7, -1, -1},
    {2, 124, 141, 194, 172, -1, -1, -1, -1, 16, 272, 0, 0, 9, 11, 10, -1, -1},
    {0, -1, -1, -1, -1, -1, -1, -1, -1, 7, 322, 0, 0, 12, 14, 13, -1, 25},
    {0, -1, -1, -1, -1, -1, -1, -1, -1, 7, 360, 0, 0, 15, 17, 16, -1, 24},
    {0, -1, -1, -1, -1, -1, -1, -1, -1, 7, 398, 0, 0, 18, 20, 19, -1, -1},
    {0, -1, -1, -1, -1, -1, -1, -1, -1, 7, 436, 0, 0, 21, 23, 22, -1, -1}
};

// GLOBAL: WIZ8 0x0064d078
int g_options_panel_ranges[8][2] = {
    {0, 2}, {3, 3}, {4, 5}, {6, 10}, {11, 11}, {12, 12}, {0, 0}, {0, 0}
};
// GLOBAL: WIZ8 0x0064d730
int g_options_difficulty_labels[3] = {0x7f8, 0x7f9, 0x7fa};
// GLOBAL: WIZ8 0x0064d73c
int g_options_camera_rotation_labels[3] = {0x7fe, 0x7ff, 0x800};
// GLOBAL: WIZ8 0x0064d748
int g_options_combat_mode_labels[2] = {0x7f6, 0x7f5};

W8OptionsPanelSet::W8OptionsPanelSet()
    : m_mode_000(0), m_compact_layout(0), m_hide_navigation(0), m_active(0), m_current_00c(0)
{
}

W8OptionsPanelSet::~W8OptionsPanelSet()
{
    for (int index = m_panels_010.count - 1; index >= 0; --index) {
        delete m_panels_010.RemoveAt(index);
    }
}

W8OptionsGamePanel::W8OptionsGamePanel() : W8OptionsPanel(0) {}
W8OptionsMousePanel::W8OptionsMousePanel() : W8OptionsPanel(1) {}
W8OptionsInterfacePanel::W8OptionsInterfacePanel() : W8OptionsPanel(2) {}
W8OptionsAudioPanel::W8OptionsAudioPanel() : W8OptionsPanel(3) {}
W8OptionsGraphicsPanel::W8OptionsGraphicsPanel() : W8OptionsPanel(4) {}
W8OptionsAdvancedGraphicsPanel::W8OptionsAdvancedGraphicsPanel() : W8OptionsPanel(5) {}
W8OptionsKeyboardPanel::W8OptionsKeyboardPanel(int panel)
    : W8OptionsPanel(panel), m_panel(panel), m_captured_button(0) {}
W8OptionsSaveLoadPanel::W8OptionsSaveLoadPanel(int panel)
    : W8OptionsPanel(panel), m_panel(panel)
{
    m_renderTarget = 0xf7;
    m_renderArg_20 = 0;
}
W8OptionsUnavailablePanel::W8OptionsUnavailablePanel(int message)
    : W8OptionsPanel(13), m_message(message) {}

// FUNCTION: WIZ8 0x005ab810
W8OptionsKeyboardPanel::~W8OptionsKeyboardPanel() {}

// SYNTHETIC: WIZ8 0x005ab7f0
// W8OptionsKeyboardPanel::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005a8050
// W8OptionsTextEditor::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005abfc0
W8OptionsPanel* CreateOptionsPanel(int panel, unsigned char* compact, unsigned char* hide_navigation)
{
    switch (panel) {
    case 0: return new W8OptionsGamePanel();
    case 1: return new W8OptionsMousePanel();
    case 2: return new W8OptionsInterfacePanel();
    case 3: return new W8OptionsAudioPanel();
    case 4: return new W8OptionsGraphicsPanel();
    case 5: return new W8OptionsAdvancedGraphicsPanel();
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        return new W8OptionsKeyboardPanel(panel);
    case 11:
        if (g_status_685170.game_started != 0 && g_status_685170.iron_man != 0) {
            *hide_navigation = 1;
            return new W8OptionsUnavailablePanel(0x82c);
        }
        *compact = 1;
        return new W8OptionsSaveLoadPanel(11);
    case 12:
        if (g_status_685170.game_started != 0) {
            if (g_status_685170.iron_man != 0) {
                *hide_navigation = 1;
                return new W8OptionsUnavailablePanel(0x82c);
            }
            if (gXStatus.fCombatMode != 0) {
                *hide_navigation = 1;
                return new W8OptionsUnavailablePanel(0x82b);
            }
        }
        *compact = 1;
        return new W8OptionsSaveLoadPanel(12);
    }
    return 0;
}

// FUNCTION: WIZ8 0x005a9fd0
void W8OptionsGamePanel::Populate()
{
    m_content_top_050 += 22;
    W8OptionsSlider* slider = AddSlider(0x805, &g_options_values.combat_speed, 0);
    slider->m_minimumPosition = 0.0f;
    slider->m_maximumPosition = 1.0f;
    slider->UpdatePixelPosition();
    m_content_top_050 += 22;
    slider = AddSlider(0x80c, &g_options_values.text_display_delay_ms, 0);
    slider->m_minimumPosition = 0.0f;
    slider->m_maximumPosition = 5000.0f;
    slider->UpdatePixelPosition();
    m_content_top_050 += 22;
    slider = AddSlider(0x802, &g_options_values.monster_movement_speed, 1);
    slider->m_minimumPosition = 1.0f;
    slider->m_maximumPosition = 5.0f;
    slider->UpdatePixelPosition();
    m_content_top_050 += 22;
    AddChoices(0x7f7, 3, g_options_difficulty_labels, &g_options_values.difficulty);
    AddChoices(0x7f4, 2, g_options_combat_mode_labels, &g_options_values.continuous_combat);
    AddChoices(0x7fd, 3, g_options_camera_rotation_labels, &g_options_values.camera_auto_rotation);
}

// FUNCTION: WIZ8 0x005aa0c0
void W8OptionsMousePanel::Populate()
{
    m_content_top_050 += 22;
    AddCheckbox(0x7f0, &g_options_values.mouselook_toggle);
    AddCheckbox(0x7f1, &g_options_values.mouselook_smoothing);
    AddCheckbox(0x7f2, &g_options_values.invert_mouse_y);
    AddCheckbox(0x801, &g_options_values.stop_movement_for_events);
    AddCheckbox(0x809, &g_options_values.autotarget_spells);
    AddCheckbox(0x804, &g_options_values.auto_advance_character);
    AddCheckbox(0x808, &g_options_values.autoswap_weapons);
    if (g_status_685170.game_started == 0 || g_status_685170.iron_man == 0) {
        AddCheckbox(0x80d, &g_options_values.auto_save);
    }
}

// FUNCTION: WIZ8 0x005aa170
void W8OptionsInterfacePanel::Populate()
{
    m_content_top_050 += 22;
    m_tooltip_delay = AddSlider(0x80e, 0, 0);
    m_tooltip_delay->m_minimumPosition = 0.0f;
    m_tooltip_delay->m_maximumPosition = 1200.0f;
    m_tooltip_delay->UpdatePixelPosition();
    m_tooltip_delay->m_position = static_cast<float>(g_settings_6850c8.tooltip_delay_ms);
    m_tooltip_delay->UpdatePixelPosition();
    m_tooltip_delay->m_listener = this;
    W8TextControl* button = AddChoiceButton(0x80f);
    button->EnableRegionHelp(0x810);
    button->m_listener = this;
    if (g_settings_6850c8.tooltips_enabled == 0) {
        button->EnableSecondaryState(0);
        m_tooltip_delay->SetEnabled(0);
    }
    m_content_top_050 += 22;
    AddCheckbox(0x80b, &g_options_values.simplified_npc_interaction);
    AddCheckbox(0x807, &g_options_values.verbose_combat_messages);
    AddCheckbox(0x7f3, &g_options_values.ctrl_right_click_info);
    AddCheckbox(0x7fc, &g_options_values.skill_increase_messages);
    AddCheckbox(0x806, &g_options_values.numeric_hit_points);
    AddCheckbox(0x80a, &g_options_values.autoscroll_combat_messages);
}

// FUNCTION: WIZ8 0x005aa2a0
void W8OptionsInterfacePanel::OnDragEnd(W8HorizontalRangeThumb* thumb)
{
    g_settings_6850c8.tooltip_delay_ms = static_cast<int>(thumb->m_position);
    SetFastHelpDelay(static_cast<short>(g_settings_6850c8.tooltip_delay_ms));
    SetRegionHelpDelay(0);
}

// FUNCTION: WIZ8 0x005aa2d0
void W8OptionsInterfacePanel::OnPrimary(W8TextControl* control)
{
    g_settings_6850c8.tooltips_enabled =
        (control->m_stateFlags & g_W8TextControlMask005ED570) == 0;
    m_tooltip_delay->SetEnabled(g_settings_6850c8.tooltips_enabled);
    m_tooltip_delay->Invalidate(0);
}

// FUNCTION: WIZ8 0x005aa310
void W8OptionsGraphicsPanel::Populate()
{
    m_content_top_050 += 22;
    W8OptionsSlider* slider = AddSlider(0x811, &g_options_values.gamma, 0);
    slider->m_minimumPosition = 0.1f;
    slider->m_maximumPosition = 1.9f;
    slider->UpdatePixelPosition();
    slider->m_listener = this;
    slider->SetEnabled(GetRendererModeByte());
    m_content_top_050 += 22;
    AddCheckbox(0x816, &g_options_values.render_options[4]);
    AddCheckbox(0x819, &g_options_values.values_078[1]);
    AddCheckbox(0x81a, &g_options_values.values_078[2]);
    AddCheckbox(0x81b, &g_options_values.render_options[6]);
    AddCheckbox(0x818, &g_options_values.values_078[0]);
    AddCheckbox(0x817, &g_options_values.render_options[5]);
    AddCheckbox(0x81d, &g_options_values.render_options[8]);
}

// FUNCTION: WIZ8 0x005aa3f0
void W8OptionsGraphicsPanel::OnDrag(W8HorizontalRangeThumb*)
{
    SetDisplayGamma(g_options_values.gamma);
}

// FUNCTION: WIZ8 0x005aa410
void W8OptionsAdvancedGraphicsPanel::Populate()
{
    m_content_top_050 += 22;
    AddCheckbox(0x815, &g_options_values.render_options[3]);
    AddCheckbox(0x814, &g_options_values.render_options[2]);
    AddCheckbox(0x812, &g_options_values.render_options[0]);
    AddCheckbox(0x81c, &g_options_values.render_options[7]);
    AddCheckbox(0x813, &g_options_values.render_options[1]);
}

// FUNCTION: WIZ8 0x005a9ea0
void W8OptionsUnavailablePanel::Populate()
{
    m_content_top_050 += 44;
    W8ControlsRect bounds = {origin_x + 30, origin_y + m_content_top_050, right - 30, bottom};
    W8TextBuffer* text = new W8TextBuffer(&bounds, gppStringList[m_message],
        g_options_detail_font_683614, g_W8TextBufferLayoutMask005ED558 |
        g_W8TextBufferLayoutMask005ED54C, 4);
    m_text_buffers_058.Add(text);
    (*m_text_buffers_058.GetAt(0))->SetLineHeight(22);
}

// FUNCTION: WIZ8 0x005a9090
W8OptionsScreen::W8OptionsScreen()
    : m_redraw_pending(1), m_modal_closing_01d(0), m_selected_panel_020(-1),
      m_controls_024(0), m_menu_set_028(0), m_menu_selection(0),
      m_active_modal(0), m_text_editor(0), m_key_capture(0)
{
    for (int index = 0; index < 8; ++index) {
        m_panel_038[index] = 0;
    }
    W8SaveSlot* current = new W8SaveSlot;
    m_save_slots.Add(current);
    if (g_status_685170.game_started != 0) {
        FillCurrentSaveSlot(current);
    }
    EnumerateSaveSlots(&m_save_slots);
}

// FUNCTION: WIZ8 0x005a98b0
void W8OptionsScreen::CreateControls()
{
    m_controls_024 = new Controls();
    m_menu_selection = new W8ControlSelection();
    m_controls_024->AcquireRegionSet(&g_options_menu_region_set);
    for (int index = 0; index < 8; ++index) {
        W8OptionsMenuButton* button =
            new W8OptionsMenuButton(m_controls_024, g_options_menu_rows[index]);
        if (index == 7) {
            button->m_primaryActivationCallback = RequestScreenTransition;
        } else if (index == 6) {
            button->m_listener = this;
        } else {
            m_menu_selection->AddEntry(button);
            if (index == 5) {
                button->SetEnabled(g_status_685170.game_started);
            } else if (index == 4) {
                button->SetEnabled(m_save_slots.count > 1);
            }
        }
    }
    m_menu_selection->m_selectionListener = this;
    m_controls_024->SetEnabled(1);
    m_controls_024->EnableRegionSet(1);
    m_controls_024->Invalidate(0);
    m_menu_set_028 = new W8OptionsMenuSet(&g_options_page_region_set);
    m_menu_set_028->SetEnabled(1);
    m_menu_set_028->EnableRegionSet(1);
}

// FUNCTION: WIZ8 0x005a9200
W8OptionsScreen::~W8OptionsScreen()
{
    int index;
    SelectPanel(-1, 1);
    for (index = m_save_slots.count - 1; index >= 0; --index) {
        delete m_save_slots.RemoveAt(index);
    }
    delete m_controls_024;
    delete m_menu_set_028;
    delete m_menu_selection;
    for (index = 0; index < 8; ++index) {
        delete m_panel_038[index];
    }
}

// FUNCTION: WIZ8 0x005a93c0
void W8OptionsScreen::SelectPanel(int selected, unsigned char notify)
{
    if (selected == m_selected_panel_020) {
        return;
    }
    if (m_text_editor != 0) {
        if (m_text_editor->m_listener != 0) {
            m_text_editor->m_listener->OnTextEditComplete(m_text_editor, 1);
        }
        delete m_text_editor;
        m_text_editor = 0;
    }
    if (m_selected_panel_020 != -1) {
        W8OptionsPanelSet* previous = m_panel_038[m_selected_panel_020];
        (*previous->m_panels_010.GetAt(previous->m_current_00c))->SetActive(0);
        previous->m_active = 0;
    }
    m_selected_panel_020 = selected;
    if (selected != -1) {
        if (m_panel_038[selected] == 0) {
            m_panel_038[selected] = new W8OptionsPanelSet();
            W8OptionsPanelSet* created = m_panel_038[m_selected_panel_020];
            for (int index = g_options_panel_ranges[m_selected_panel_020][0];
                 index <= g_options_panel_ranges[m_selected_panel_020][1]; ++index) {
                W8OptionsPanel* panel = CreateOptionsPanel(index,
                    &created->m_compact_layout, &created->m_hide_navigation);
                panel->Populate();
                created->m_panels_010.Add(panel);
            }
        }
        W8OptionsPanelSet* current = m_panel_038[m_selected_panel_020];
        if (current->m_active == 0) {
            current->m_current_00c = 0;
        }
        (*current->m_panels_010.GetAt(current->m_current_00c))->SetActive(1);
        current->m_active = 1;
        m_menu_set_028->m_pMenuSet = m_panel_038[m_selected_panel_020];
        m_menu_set_028->UpdateMenuSet();
        if (m_selected_panel_020 == 4) {
            m_panel_038[4]->m_mode_000 = (m_save_slots.count - 2) / 5 + 1;
        } else if (m_selected_panel_020 == 5) {
            m_panel_038[5]->m_mode_000 = (m_save_slots.count - 1) / 5 + 1;
        }
        m_menu_set_028->UpdateMenuSet();
    }
    if (notify != 0) {
        m_menu_selection->SetSelected(m_selected_panel_020);
    }
    if (m_selected_panel_020 >= 0) {
        g_last_options_panel = m_selected_panel_020;
    }
}

// FUNCTION: WIZ8 0x005a9690
void W8OptionsScreen::ShowNotification(
    W8DialogCloseListener* listener, int caption, int message, int value)
{
    m_modal_closing_01d = 1;
    delete m_active_modal;
    W8NotificationDialog* dialog = new W8NotificationDialog(message, caption, value);
    m_active_modal = dialog;
    if (listener != 0) {
        dialog->notify_target = listener;
    }
}

// FUNCTION: WIZ8 0x005a9a60
void W8OptionsScreen::OnPrimary(W8TextControl*)
{
    if (m_text_editor != 0) {
        if (m_text_editor->m_listener != 0) {
            m_text_editor->m_listener->OnTextEditComplete(m_text_editor, 1);
        }
        delete m_text_editor;
        m_text_editor = 0;
    }
    ShowNotification(this, 1, 0x832, 0);
}

// FUNCTION: WIZ8 0x005a9720
unsigned char W8OptionsScreen::ProcessInput(const InputAtom* input)
{
    W8OptionsTextEditor* editor = m_text_editor;
    if (editor != 0) {
        if (input->usEvent == KEY_DOWN || input->usEvent == KEY_REPEAT) {
            if (input->usParam != VK_ESCAPE && input->usParam != VK_RETURN) {
                unsigned short character = TranslateKeyToCharacter(
                    static_cast<unsigned short>(input->usParam), input->usKeyState);
                if (character != 0 && strchr("\\/:*?\"<>|", character) != 0) {
                    return 0;
                }
                HandleTextInput(input);
                return 0;
            }
            SetCurrentCursor(-1);
            if (editor->m_listener != 0) {
                editor->m_listener->OnTextEditComplete(editor, input->usParam == VK_ESCAPE);
            }
            delete m_text_editor;
            m_text_editor = 0;
            return 1;
        }
        Function568950(input);
    } else if (m_key_capture != 0 && input->usEvent == KEY_DOWN) {
        return m_key_capture->OnKey(static_cast<unsigned short>(input->usParam), input->usKeyState);
    }
    return 0;
}

// FUNCTION: WIZ8 0x005a95f0
void W8OptionsScreen::Redraw()
{
    if (m_redraw_pending != 0) {
        ClearPrimarySurface();
        ClearSurfaceRect(0, 0, 640, 480);
        DrawCatalogImageAndInvalidate(-14, 0xee, 0, 0, 0, 0, 2, 0);
        m_redraw_pending = 0;
    }
    if (m_selected_panel_020 != -1) {
        W8OptionsPanelSet* panel_set = m_panel_038[m_selected_panel_020];
        if (panel_set->m_active != 0) {
            (*panel_set->m_panels_010.GetAt(panel_set->m_current_00c))->Redraw();
        }
    }
    m_controls_024->Redraw();
    if (m_selected_panel_020 != -1) {
        m_menu_set_028->Redraw();
    }
    if (m_active_modal != 0) {
        m_active_modal->Draw();
    }
    if (m_text_editor != 0) {
        RenderActiveTextField();
    }
}

// FUNCTION: WIZ8 0x005a72f0
void W8OptionsValues::TransferByte(int* value, unsigned char* setting)
{
    if (applying == 0) {
        *value = *setting;
    } else {
        *setting = static_cast<unsigned char>(*value);
    }
}

// FUNCTION: WIZ8 0x005a7320
void W8OptionsValues::TransferRenderOption(int* value, int option)
{
    if (applying == 0) {
        *value = GetRenderOptionState(option);
    } else if (*value != 0) {
        EnableRenderOption(option);
    } else {
        DisableRenderOption(option);
    }
}

// FUNCTION: WIZ8 0x005a6e20
void W8OptionsValues::TransferSettings()
{
    TransferByte(&mouselook_toggle, &g_settings_6850c8.mouselook_toggle);
    TransferByte(&mouselook_smoothing, &g_settings_6850c8.mouselook_smoothing);
    TransferByte(&invert_mouse_y, &g_settings_6850c8.invert_mouse_y);
    TransferByte(&ctrl_right_click_info, &g_settings_6850c8.ctrl_right_click_info);
    TransferByte(&numeric_hit_points, &g_settings_6850c8.numeric_hit_points);
    TransferByte(&autoscroll_combat_messages, &g_settings_6850c8.autoscroll_combat_messages);
    TransferByte(&simplified_npc_interaction, &g_settings_6850c8.simplified_npc_interaction);
    TransferByte(&verbose_combat_messages, &g_settings_6850c8.verbose_combat_messages);
    TransferByte(&autoswap_weapons, &g_settings_6850c8.autoswap_weapons);
    TransferByte(&skill_increase_messages, &g_settings_6850c8.skill_increase_messages);
    if (applying == 0) {
        monster_movement_speed = g_settings_6850c8.monster_movement_speed;
    } else {
        g_settings_6850c8.monster_movement_speed = monster_movement_speed;
    }
    TransferByte(&auto_advance_character, &g_settings_6850c8.auto_advance_character);
    TransferByte(&autotarget_spells, &g_settings_6850c8.autotarget_spells);
    if (applying == 0) {
        gamma = g_settings_6850c8.gamma;
    } else {
        g_settings_6850c8.gamma = gamma;
    }
    TransferByte(&value_084, &g_settings_6850c8.field_040);
    TransferByte(&value_088, &g_settings_6850c8.field_042);
    if (applying == 0) {
        text_display_delay_ms = static_cast<float>(g_settings_6850c8.text_display_delay_ms);
    } else {
        g_settings_6850c8.text_display_delay_ms = static_cast<unsigned int>(text_display_delay_ms);
    }
    TransferByte(&auto_save, &g_settings_6850c8.auto_save);
    if (applying == 0) {
        difficulty = g_status_685170.difficulty;
    } else {
        g_status_685170.difficulty = difficulty;
    }
    if (applying == 0) {
        difficulty = g_settings_6850c8.difficulty;
    } else {
        g_settings_6850c8.difficulty = difficulty;
    }
    TransferRenderOption(&render_options[0], 4);
    TransferRenderOption(&render_options[1], 9);
    TransferRenderOption(&render_options[2], 5);
    TransferRenderOption(&render_options[3], 10);
    TransferRenderOption(&render_options[4], 13);
    TransferRenderOption(&render_options[5], 14);
    TransferRenderOption(&render_options[6], 11);
    TransferRenderOption(&render_options[7], 12);
    TransferRenderOption(&render_options[8], 16);
    TransferByte(&values_078[0], &g_settings_6850c8.field_048);
    TransferByte(&values_078[1], &g_settings_6850c8.field_049);
    TransferByte(&values_078[2], &g_settings_6850c8.field_04a);
    if (applying == 0) {
        combat_speed = static_cast<float>(g_settings_6850c8.combat_delay_ms - 5000) * -0.0002f;
        camera_auto_rotation = g_settings_6850c8.camera_rotation_mode == 2
                            ? 2 : g_settings_6850c8.camera_rotation_style;
        TransferByte(&continuous_combat, &g_settings_6850c8.continuous_combat);
    } else {
        g_settings_6850c8.combat_delay_ms = 5000 - static_cast<int>(combat_speed * 5000.0f);
        if (camera_auto_rotation == 2) {
            g_settings_6850c8.camera_rotation_mode = 2;
        } else {
            g_settings_6850c8.camera_rotation_mode = 1;
            g_settings_6850c8.camera_rotation_style = camera_auto_rotation;
        }
        if (continuous_combat != g_settings_6850c8.continuous_combat) {
            TogglePartyCombatStance();
        }
    }
}

/* Menu rows come from the shared source table: the button's own geometry and
   text parameters sit in row[9..17], the item id in row[0], and the two
   optional child controls in row[1..4] and row[5..8].  Each child joins the
   button's owner panel, registers the button's listener subobject, and picks
   up the shared layout flag. */
// FUNCTION: WIZ8 0x005a7370
W8OptionsMenuButton::W8OptionsMenuButton(Controls* owner, const int* row)
    : W8TextControl(owner, 0xffffffff, row[9], row[10], row[11], row[12],
                            0xef, 0, row[13], row[14], row[15], row[16], row[17])
{
    m_item_id_0bc = row[0];

    if (row[1] != -1) {
        W8TextControl* control = new W8TextControl(
            m_pPanel, 0xffffffff, row[1], row[2], row[3], row[4],
            -1, -1, -1, -1, -1, -1, -1);
        control->m_listener = this;
        control->AddLayoutFlags(g_W8TextControlLayoutMask005ED590);
    }

    if (row[5] != -1) {
        W8TextControl* control = new W8TextControl(
            m_pPanel, 0xffffffff, row[5], row[6], row[7], row[8],
            -1, -1, -1, -1, -1, -1, -1);
        control->m_listener = this;
        control->AddLayoutFlags(g_W8TextControlLayoutMask005ED590);
    }
}

// FUNCTION: WIZ8 0x005a7510
W8OptionsMenuButton::~W8OptionsMenuButton()
{
}

// SYNTHETIC: WIZ8 0x005a74f0
// W8OptionsMenuButton::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005a7570
void W8OptionsMenuButton::OnPrimary(W8TextControl*)
{
    if (m_listener != 0) {
        m_listener->OnPrimary(this);
    }
}

W8OptionsCheckbox::W8OptionsCheckbox(Controls* owner, int top, int* value)
    : W8TextControl(owner, 0xffffffff, 0x14d, top - 2, 0, 0,
                    0xf1, 0, 2, 0, 3, 1, -1), m_value(value)
{
    AddLayoutFlags(g_W8TextControlMask005ED588 | g_W8TextControlMask005ED578);
    if (*m_value != 0) {
        EnableSecondaryState(0);
    }
}

// FUNCTION: WIZ8 0x005a7620
W8OptionsCheckbox::~W8OptionsCheckbox()
{
}

// SYNTHETIC: WIZ8 0x005a7600
// W8OptionsCheckbox::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005a7680
void W8OptionsCheckbox::OnLeftButtonUp(int event)
{
    W8TextControl::OnLeftButtonUp(event);
    *m_value = static_cast<unsigned char>(m_stateFlags) &
               g_W8TextControlMask005ED570 & 0xff;
}

W8OptionsSlider::W8OptionsSlider(
    Controls* owner, int top, float* value, unsigned char alternate)
    : W8HorizontalRangeThumb(owner, 0xffffffff, 0xc9, top - 2,
                             0xf5, 0, alternate != 0 ? 4 : 0, 1, 2, 3),
      m_value(value)
{
    if (m_value != 0) {
        m_position = *m_value;
    }
}

// FUNCTION: WIZ8 0x005a7f50
W8OptionsSlider::~W8OptionsSlider()
{
}

// SYNTHETIC: WIZ8 0x005a7f30
// W8OptionsSlider::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005a7f60
void W8OptionsSlider::OnMouseMove(int event)
{
    W8HorizontalRangeThumb::OnMouseMove(event);
    if (m_dragging != 0 && m_value != 0) {
        *m_value = m_position;
    }
}

// FUNCTION: WIZ8 0x005a7f90
void W8OptionsSlider::Redraw(int full_redraw)
{
    if (m_active != 0 && (static_cast<unsigned char>(full_redraw) != 0 || m_dirty != 0)) {
        W8HorizontalRangeThumb::Redraw(full_redraw);
        if (m_enabled != 0 && m_pixelPosition > 13) {
            int left = m_pPanel->origin_x + m_left;
            int top = m_pPanel->origin_y + m_top;
            unsigned short color = Get16BPPColor(0x00ff00);
            int end = m_pixelPosition < 74 ? m_pixelPosition - 1 : 73;
            ColorFillVideoSurfaceArea(-14, left + 13, top + 7, left + end, top + 9, color);
            if (m_pixelPosition > 83) {
                ColorFillVideoSurfaceArea(-14, left + 83, top + 7,
                                         left + m_pixelPosition - 1, top + 9, color);
            }
        }
    }
}

W8OptionsSelection::W8OptionsSelection(int* value) : m_value(value)
{
}

// FUNCTION: WIZ8 0x005a8080
void W8OptionsSelection::OnPrimary(W8TextControl* control)
{
    W8ControlSelection::OnPrimary(control);
    *m_value = m_selectedIndex;
}

// FUNCTION: WIZ8 0x005a8320
W8OptionsPanel::~W8OptionsPanel()
{
    int index;
    DestroyAllControls();
    for (index = m_text_buffers_058.count - 1; index >= 0; --index) {
        delete m_text_buffers_058.RemoveAt(index);
    }
    for (index = m_option_selections.count - 1; index >= 0; --index) {
        delete m_option_selections.RemoveAt(index);
    }
}

// SYNTHETIC: WIZ8 0x005a8300
// W8OptionsPanel::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005a84e0
W8OptionsCheckbox* W8OptionsPanel::AddCheckbox(int label, int* value)
{
    W8ControlsRect bounds = {origin_x + 20, origin_y + m_content_top_050,
                            right, origin_y + m_content_top_050 + 22};
    W8TextBuffer* text = new W8TextBuffer(&bounds, gppStringList[label],
        g_options_detail_font_683614, g_W8TextBufferLayoutMask005ED558 |
        g_W8TextBufferLayoutMask005ED548, 4);
    m_text_buffers_058.Add(text);
    W8OptionsCheckbox* checkbox = new W8OptionsCheckbox(this, m_content_top_050, value);
    m_content_top_050 += 44;
    return checkbox;
}

// FUNCTION: WIZ8 0x005a8670
W8TextControl* W8OptionsPanel::AddChoiceButton(int label)
{
    W8ControlsRect bounds = {origin_x, origin_y + m_content_top_050,
                            origin_x + 0x147, origin_y + m_content_top_050 + 22};
    W8TextBuffer* text = new W8TextBuffer(&bounds, gppStringList[label],
        g_options_detail_font_683614, g_W8TextBufferLayoutMask005ED558 |
        g_W8TextBufferLayoutMask005ED550, 4);
    m_text_buffers_058.Add(text);
    W8TextControl* button = new W8TextControl(this, 0xffffffff, 0x151,
        m_content_top_050 + 1, 0, 0, 0xf1, 0, 4, 6, 5, 7, -1);
    button->AddLayoutFlags(g_W8TextControlMask005ED588 | g_W8TextControlMask005ED578);
    m_content_top_050 += 22;
    return button;
}

// FUNCTION: WIZ8 0x005a8800
W8OptionsSlider* W8OptionsPanel::AddSlider(int label, float* value, unsigned char alternate)
{
    W8ControlsRect bounds = {origin_x + 20, origin_y + m_content_top_050,
                            right, origin_y + m_content_top_050 + 22};
    W8TextBuffer* text = new W8TextBuffer(&bounds, gppStringList[label],
        g_options_detail_font_683614, g_W8TextBufferLayoutMask005ED558 |
        g_W8TextBufferLayoutMask005ED548, 4);
    m_text_buffers_058.Add(text);
    W8OptionsSlider* slider = new W8OptionsSlider(this, m_content_top_050, value, alternate);
    m_content_top_050 += 22;
    return slider;
}

// FUNCTION: WIZ8 0x005a8980
void W8OptionsPanel::AddChoices(int label, int count, const int* choices, int* value)
{
    W8ControlsRect bounds = {origin_x + 20, origin_y + m_content_top_050,
                            right, origin_y + m_content_top_050 + 22};
    W8TextBuffer* text = new W8TextBuffer(&bounds, gppStringList[label],
        g_options_detail_font_683614, g_W8TextBufferLayoutMask005ED558 |
        g_W8TextBufferLayoutMask005ED548, 4);
    m_text_buffers_058.Add(text);
    W8OptionsSelection* selection = new W8OptionsSelection(value);
    m_option_selections.Add(selection);
    for (int index = 0; index < count; ++index) {
        selection->AddEntry(AddChoiceButton(choices[index]));
    }
    selection->SetSelected(*selection->m_value);
    m_content_top_050 += 22;
}

// FUNCTION: WIZ8 0x005a7590
void W8OptionsMenuButton::Redraw(int full_redraw)
{
    if ((static_cast<unsigned char>(full_redraw) != 0 || m_dirty != 0) &&
        (m_stateFlags & g_W8TextControlMask005ED570) != 0 && m_item_id_0bc != -1) {
        DrawCatalogImageAndInvalidate(-14, 0xf0, 0, m_item_id_0bc, 12, 15, 2, 0);
    }
    W8TextControl::Redraw(full_redraw);
}

// FUNCTION: WIZ8 0x005a8470
void W8OptionsPanel::Redraw()
{
    bool redraw_text = m_fDirty != 0 && m_fEnabled != 0;
    Controls::Redraw();
    if (redraw_text) {
        for (int index = 0; index < m_text_buffers_058.count; ++index) {
            (*m_text_buffers_058.GetAt(index))->RenderToTarget(0, 1, -14);
        }
    }
}

// FUNCTION: WIZ8 0x005a9020
void W8OptionsMenuSet::Redraw()
{
    if (m_fEnabled != 0 && (m_fDirty != 0 || m_fLayoutDirty != 0)) {
        Controls::Redraw();
        m_page_text_05c->RenderToTarget(0, 1, -14);
    }
}

/* The shared panel base sizes itself from the chrome sprite of its render
   target and registers one region-set row per concrete panel index. */
// FUNCTION: WIZ8 0x005a81e0
W8OptionsPanel::W8OptionsPanel(int region_index)
    : Controls(0x104, 0, 0, 0, 0xee, 0, 1),
      m_current_04c(0), m_content_top_050(0x18)
{
    AcquireRegionSet(g_options_panel_region_sets + region_index);

    short width;
    short height;
    GetCatalogImageSize(0xf2, 0, 0, &width, &height);
    right = origin_x + (unsigned short)width;
    bottom = origin_y + (unsigned short)height;
}

/* The menu set builds itself from the shared options font: its panel bounds
   follow the rendered chrome sprite, then the two page arrows are constructed
   and registered with the listener subobject, and the page text starts as the
   shared empty wide-string global until UpdateMenuSet fills it. */
// FUNCTION: WIZ8 0x005a8c90
W8OptionsMenuSet::W8OptionsMenuSet(unsigned int* shared_region_set)
    : Controls(0x11b, 0x1a8, 0, 0, 0xf3, 0, 0), m_pMenuSet(0)
{
    AcquireRegionSet(shared_region_set);

    short width;
    short height;
    GetCatalogImageSize(0xf3, 0, 0, &width, &height);
    right = origin_x + (unsigned short)width;
    bottom = origin_y + (unsigned short)height;

    m_previous_058 = new W8TextControl(
        this, 0xffffffff, 3, 3, 0, 0, 0xf4, 0, 0, 2, 1, -1, 3);
    m_previous_058->m_listener = this;

    m_next_054 = new W8TextControl(
        this, 0xffffffff, 0x11f, 3, 0, 0, 0xf4, 0, 4, 6, 5, -1, 7);
    m_next_054->m_listener = this;

    m_page_text_05c = new W8TextBuffer(
        (W8ControlsRect*)&origin_x, &g_wchar_00689b34, g_options_detail_font_683614,
        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554, 4);
}

/* The menu-set table has its own deleting destructor; the normal destructor
   clears the inherited Controls children before releasing its page text. */
// FUNCTION: WIZ8 0x005a8e60
W8OptionsMenuSet::~W8OptionsMenuSet()
{
    DestroyAllControls();
    delete m_page_text_05c;
}

// FUNCTION: WIZ8 0x005a8440
void W8OptionsPanel::SetActive(unsigned char active)
{
    EnableRegionSet(active);
    SetEnabled(active);
    Invalidate(0);
}

// FUNCTION: WIZ8 0x005a84c0
void W8OptionsPanel::SetCurrent(int current)
{
    m_current_04c = current;
    Invalidate(0);
}

// FUNCTION: WIZ8 0x005a8b70
void W8OptionsPanelSet::Advance()
{
    int current;

    if (m_mode_000 == 0) {
        current = m_current_00c;
        if (current < m_panels_010.count - 1) {
            (*m_panels_010.GetAt(current))->SetActive(0);
            current = m_current_00c + 1;
            m_current_00c = current;
            (*m_panels_010.GetAt(current))->SetActive(1);
        }
    }
    else if (m_panels_010.data[0]->m_current_04c < m_mode_000 - 1) {
        m_panels_010.data[0]->SetCurrent(m_panels_010.data[0]->m_current_04c + 1);
    }
}

// FUNCTION: WIZ8 0x005a8c00
void W8OptionsPanelSet::Retreat()
{
    int current;

    if (m_mode_000 == 0) {
        current = m_current_00c;
        if (current > 0) {
            (*m_panels_010.GetAt(current))->SetActive(0);
            current = m_current_00c - 1;
            m_current_00c = current;
            (*m_panels_010.GetAt(current))->SetActive(1);
        }
    }
    else if (m_panels_010.data[0]->m_current_04c > 0) {
        m_panels_010.data[0]->SetCurrent(m_panels_010.data[0]->m_current_04c - 1);
    }
}

// FUNCTION: WIZ8 0x005a8ed0
void W8OptionsMenuSet::UpdateMenuSet()
{
    W8ControlsRect bounds;
    W8OptionsPanelSet* panel_set;
    int current;
    int count;

    bounds.left = origin_x;
    bounds.top = origin_y;
    bounds.right = right;
    bounds.bottom = bottom;
    int height = bottom - origin_y;
    if (m_pMenuSet == 0) {
        srAssertFail("m_pMenuSet",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\OptionsScreen.cpp",
                     0x5c9, 0);
    }
    panel_set = m_pMenuSet;
    if (panel_set->m_mode_000 == 0) {
        current = panel_set->m_current_00c;
        count = panel_set->m_panels_010.count;
    }
    else {
        current = panel_set->m_panels_010.data[0]->m_current_04c;
        count = panel_set->m_mode_000;
    }
    bounds.top = panel_set->m_compact_layout != 0 ? 0x180 : 0x1a8;
    bounds.bottom = height + bounds.top;
    SetBounds(bounds.left, bounds.top, bounds.right, bounds.bottom);
    ++bounds.top;
    m_page_text_05c->SetLayoutBounds(&bounds, 1, 1);
    SetEnabled(panel_set->m_hide_navigation == 0 &&
                       (panel_set->m_compact_layout != 0 || count > 1));
    if (m_fEnabled != 0) {
        m_next_054->SetEnabled(current < count - 1);
        m_previous_058->SetEnabled(current > 0);
        wchar_t text[0x10];

        swprintf(text, L"%d / %d", current + 1, count);
        m_page_text_05c->SetText(text, g_options_detail_font_683614);
    }
    Invalidate(0);
}

/* The chrome listener is attached at the secondary base subobject.  The
   preceding control walks back and all other primary activations walk forward;
   both paths then recompute bounds, navigation availability, and page text. */
// FUNCTION: WIZ8 0x005a9050
void W8OptionsMenuSet::OnPrimary(W8TextControl* control)
{
    if (control == m_previous_058) {
        m_pMenuSet->Retreat();
    }
    else {
        m_pMenuSet->Advance();
    }
    UpdateMenuSet();
}

/* The dialog callback's teardown decision is part of the Options screen's
   state transition: only an accepted close outside combat saves an active
   party, then the common audio-state update runs for every accepted close. */
// FUNCTION: WIZ8 0x005a9ac0
void W8OptionsScreen::OnDialogClosed(unsigned char reason, int)
{
    if (reason != 0) {
        if (g_status_685170.game_started != 0 &&
            gXStatus.fCombatMode == 0 && AnyCharacterActive()) {
            AutoSaveIfAllowed(1);
        }
        RequestExitScreen();
    }
}

/* The selection-listener slot receives the originating control and its selected
   row.  The retail thunk deliberately ignores the control and chooses the
   panel without emitting another notification. */
// FUNCTION: WIZ8 0x005a98a0
void W8OptionsScreen::OnSelectionChanged(
    W8ControlSelection*, int selected)
{
    SelectPanel(selected, 0);
}

/* State 10 first transfers its option values back to the shared settings
   block, then releases every controller-owned control before returning the
   display and region systems to their common screen boundary. */
// FUNCTION: WIZ8 0x005a9c70
unsigned char OptionsScreenLeave(int)
{
    g_options_values.applying = 1;
    g_options_values.TransferSettings();
    W8OptionsScreen* screen = g_options_screen_0069c254;
    if (screen != 0) {
        delete screen;
    }
    g_options_screen_0069c254 = 0;
    NoOp();
    MSYS_Shutdown();
    ResetRegions();
    return 1;
}

// FUNCTION: WIZ8 0x005a9b00
unsigned char OptionsScreenInitialize(void)
{
    if (!g_options_panel_region_sets) {
        g_options_panel_region_sets = new unsigned int[14];
        memset(g_options_panel_region_sets, 0, 0x38);
    }
    return 1;
}

/* The finalizer half of the pair above, and the fifth dword of lifecycle record
   10 - which is what establishes that the record's last slot is the finalizer
   rather than a second initializer: record 10's first slot allocates this exact
   block and its fifth releases it. The release is unguarded and leaves the
   pointer set, so it relies on running once at shutdown. */
// FUNCTION: WIZ8 0x005a9b30
unsigned char OptionsScreenFinalize(void)
{
    delete[] g_options_panel_region_sets;
    return 1;
}

/* State 10 creates the controller after resetting the shared 2D screen
   systems.  The selected panel is determined by the transition mode, with the
   in-game branch preserving the combat and save-state overrides. */
// FUNCTION: WIZ8 0x005a9b50
unsigned char OptionsScreenEnter()
{
    int selected;

    SetViewport(0, 0, 0x280, 0x1e0);
    SetPrimarySurfaceTextureHint2Enabled(0);
    MSYS_Init();
    ResetRegions();
    UpdateHeldItemCursor();
    SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
    g_options_values.applying = 0;
    g_options_values.TransferSettings();
    g_options_screen_0069c254 = new W8OptionsScreen();
    g_options_screen_0069c254->CreateControls();

    if (g_current_screen_state.mode == 1) {
        selected = 4;
    }
    else if (g_current_screen_state.mode == 2) {
        selected = 5;
    }
    else if (g_current_screen_state.mode == 3) {
        if (g_last_options_panel == 4) {
            selected = 5;
        }
        else {
            selected = g_last_options_panel;
            if (g_last_options_panel == 5) {
                if (gXStatus.fCombatMode != 0) {
                    selected = 4;
                }
                if (g_status_685170.iron_man != 0) {
                    selected = 0;
                }
            }
        }
    }
    else {
        selected = 0;
    }
    g_options_screen_0069c254->SelectPanel(selected, 1);
    g_options_first_frame = 1;
    return 1;
}

/* The state-10 frame owns modal retirement, input refusal order and redraw
   sequencing.  Options-specific input gets first refusal, followed by shared
   region dispatch; only an unhandled Escape requests the screen transition. */
// FUNCTION: WIZ8 0x005a9cc0
void OptionsScreenFrame()
{
    W8ScreenPoint point;
    W8ScreenPoint current;
    InputAtom input;

    if (g_flag_689b32) {
        RequestExitScreen();
    }
    RepositionAmbientSounds0047A600(g_world);
    UpdateAmbientSounds0047A3E0(g_world);
    Function48F9E0();
    GetScreenPoint004284F0(&point);

    W8OptionsScreen* screen = g_options_screen_0069c254;
    if (screen->m_text_editor != 0) {
        GetScreenPoint004284F0(&current);
        MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, static_cast<unsigned short>(current.x),
                       static_cast<unsigned short>(current.y),
                       gfLeftButtonState, gfRightButtonState);
        screen = g_options_screen_0069c254;
    }

    W8ModalDialogBase** active_modal = &screen->m_active_modal;
    if (*active_modal != 0) {
        if ((*active_modal)->ProcessInput() == 0) {
            if (screen->m_modal_closing_01d == 0) {
                delete *active_modal;
                *active_modal = 0;
            }
            screen->m_redraw_pending = 1;
            screen->m_controls_024->Invalidate(0);
            if (screen->m_selected_panel_020 != -1) {
                W8OptionsPanelSet* panel_set =
                    screen->m_panel_038[screen->m_selected_panel_020];
                if (panel_set->m_active != 0) {
                    (*panel_set->m_panels_010.GetAt(panel_set->m_current_00c))
                        ->Invalidate(0);
                }
                screen->m_menu_set_028->Invalidate(0);
            }
        }
        screen->m_modal_closing_01d = 0;
    }

    UpdateRegionMousePosition(point.x, point.y);
    while (DequeueEvent(&input) == 1) {
        if (!screen->ProcessInput(&input) &&
            !DispatchRegionInput(&input) &&
            input.usEvent == KEY_DOWN && input.usParam == VK_ESCAPE) {
            RequestScreenTransition();
        }
    }
    screen->Redraw();
    if (g_options_first_frame != 0) {
        SetRendererOption4Enabled(0);
        RenderFrame();
        SetRendererOption4Enabled(1);
        g_options_first_frame = 0;
    }
    RenderFrame();
}
