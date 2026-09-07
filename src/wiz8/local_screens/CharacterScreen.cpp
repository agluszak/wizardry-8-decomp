#include "wiz8/local_screens/CharacterScreen.h"

#include "wiz8/cursor.h"
#include "wiz8/combat_state.h"
#include "wiz8/dialog_base.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/ProfRaceInfoDialog.h"
#include "wiz8/dialog_code/StatInfoDialogs.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/regions.h"
#include "wiz8/screen_state.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"

extern "C" {
#include "input.h"
#include "mousesystem.h"
}

#include <new>
#include <string.h>
#include <wchar.h>

#include "FileMan.h"

extern void SetViewport(int left, int top, int right, int bottom);
extern void Function425570(int enabled);
extern int MSYS_Init(void);
extern void MSYS_Shutdown(void);
extern void ResetRegions(void);
extern void UpdateHeldItemCursor(void);
extern void SetFontObjectPalette16BPP(int font, unsigned short* palette);
extern unsigned char Function48FC10(const char*, int, int);
extern void PlaySound(const char*, int);
extern wchar_t* FormatWideString(const wchar_t*, ...);
extern unsigned int CharacterPointerToPartySlot(W8Character* character);
extern void RequestScreenTransition(void);
extern void NoOp(void);
extern void Function426790(void);
extern void MSYS_SGP_Mouse_Handler_Hook(unsigned short event, unsigned short x,
                                        unsigned short y, char right_button,
                                        char left_button);
extern void Function556DC0(W8Character*, W8CharacterCreationState*);
extern void Function556CC0(W8Character*, W8CharacterCreationState*);
extern void Function558180(W8Character*, W8CharacterCreationState*);
extern void Function4EFA30(W8Character*);
extern void CalcCharacterTableValue(W8Character*);
extern void Function52DDD0(void);
extern int Function52E750(void);
extern int Function557FD0(W8Character* original, W8Character* edited);
extern void SetPendingScreenState(int state);
extern void Function557580(W8Character*, W8CharacterCreationState*, unsigned char);
extern void Function4EF7E0(W8Character*, W8Character*, int);
extern int Function558640(W8Character*);
extern unsigned char Function5586B0(W8Character*);
extern void BuildCharacterPath00514EC0(char*, const wchar_t*, int);
extern void Function5218C0(W8Character*);
extern void Function51D960(W8Character*);
extern unsigned char SaveCharacter(W8Character*, int, char, void (*)(void));

extern int g_font_683660;
extern int g_wiz_text_bold_font_683664;
extern unsigned short* g_colour_68ee08;
extern unsigned short* g_font_palette_wiz_text_bold_68ee0c;
extern unsigned char g_in_combat_00683f94;
extern unsigned char g_flag_6f04e8;
extern unsigned char g_flag_6f04ed;
extern unsigned short g_profession_name_message_ids_61e3f0[];
extern unsigned short g_race_name_message_ids_61e3d0[];
extern unsigned short g_faction_name_message_rows_61e430[][4];
extern unsigned short g_profession_level_name_message_ids_61e688[][9];
extern unsigned short g_character_description_first_ids_61e3a4[];
extern unsigned short g_character_description_second_ids_61e454[];
extern int g_character_page_title_ids_64da9c[4];
extern int g_options_detail_font_683614;
extern wchar_t g_wchar_00689b34;

// GLOBAL: WIZ8 0x0069c2e4
unsigned int g_character_screen_region_set_0069c2e4;
// GLOBAL: WIZ8 0x0069c2e8
W8CharacterScreen* g_character_screen_0069c2e8;

// VTABLE: WIZ8 0x005ef224 W8CharacterScreen
// VTABLE: WIZ8 0x005ef21c W8TextControl005ED604::Listener
// class W8CharacterScreen

// FUNCTION: WIZ8 0x005b0040
W8CharacterScreen::W8CharacterScreen(int mode, W8Character* character)
    : m_mode_008(mode), m_original_014(character),
      m_block_advance_1aec(0), m_confirm_profession_1aed(0),
      m_force_transition_1aee(0), m_dialog_1b1c(0),
      m_capture_dialog_result_1b24(0)
{
    if (character != 0) {
        memcpy(&m_character_018, character, sizeof(m_character_018));
        m_character_018.in_party = 0;
    }
    for (int index = 0; index < 4; ++index) {
        m_pages_1b0c[index] = 0;
    }
    if (m_mode_008 == 3) {
        m_mode_008 = 2;
        PlaySound("Data\\Sound\\Misc\\GainLevel.wav", 0);
        ShowMessage(FormatWideString(gppStringList[0x364 / 4],
                                     m_character_018.name, 0, 0), 0, 0);
    }
    m_page_enabled_1b08[0] = static_cast<unsigned char>(m_mode_008 != 1);
    m_page_enabled_1b08[1] = static_cast<unsigned char>(m_mode_008 != 1);
    m_page_enabled_1b08[2] = static_cast<unsigned char>(m_mode_008 != 1);
    m_page_enabled_1b08[3] = static_cast<unsigned char>(m_mode_008 != 2);
}

// FUNCTION: WIZ8 0x005b0140
void W8CharacterScreen::BuildControls()
{
    m_controls_1af0 = new Controls(0, 0x1c2, 0, 0, 0x107, 0, 4);
    m_controls_1af0->AcquireRegionSet(&g_character_screen_region_set_0069c2e4);

    m_next_1af8 = new W8TextControl005ED604(
        m_controls_1af0, 0xffffffff, 0x254, 0, 0, 0, 0x106, 0,
        8, 10, 9, 10, 0xb);
    m_next_1af8->EnableRegionHelp(0xdc);
    m_next_1af8->m_listener = this;

    m_previous_1af4 = new W8TextControl005ED604(
        m_controls_1af0, 0xffffffff, 0x228, 0, 0, 0, 0x106, 0,
        0xc, 0xe, 0xd, 0xe, 0xf);
    m_previous_1af4->EnableRegionHelp(0xdd);
    m_previous_1af4->m_listener = this;

    m_exit_1afc = new W8TextControl005ED604(
        m_controls_1af0, 0xffffffff, 0x1fc, 0, 0, 0, 0x106, 0,
        0x14, 0x16, 0x15, 0x16, 0x17);
    m_exit_1afc->EnableRegionHelp(0xde);
    m_exit_1afc->m_listener = this;

    m_accept_1b00 = new W8TextControl005ED604(
        m_controls_1af0, 0xffffffff, 0x1d0, 0, 0, 0, 0x106, 0,
        4, 6, 5, 6, 7);
    m_accept_1b00->EnableRegionHelp(0xdf);
    m_accept_1b00->m_listener = this;

    m_reset_1b04 = new W8TextControl005ED604(
        m_controls_1af0, 0xffffffff, 0, 0, 0, 0, 0x106, 0,
        0x1d, 0x1f, 0x1e, 0x1f, 0x20);
    m_reset_1b04->EnableRegionHelp(0xe1);
    m_reset_1b04->m_listener = this;

    m_controls_1af0->SetEnabled(1);
    m_controls_1af0->EnableRegionSet(1);
    m_controls_1af0->Invalidate(0);
    m_reset_1b04->SetEnabled(0);
    if (m_mode_008 == 1 && g_status_685170.game_started != 0 &&
        CharacterPointerToPartySlot(m_original_014) > 1) {
        m_reset_1b04->SetEnabled(1);
        if (g_in_combat_00683f94 != 0) {
            m_reset_1b04->SetVisible(0);
        }
    }

    m_page_index_00c = -1;
    int index = 0;
    while (index < 4 && m_page_enabled_1b08[index] == 0) {
        ++index;
    }
    SelectPage(index < 4 ? index : -1);
}

// FUNCTION: WIZ8 0x005b04b0
void W8CharacterScreen::UpdateDialog()
{
    if (m_dialog_1b1c != 0) {
        if (m_dialog_response_1b20 == 1) {
            Function52DDD0();
            if (Function52E750() == 0 &&
                static_cast<W8ModalDialogBase*>(m_dialog_1b1c)->close_result) {
                m_dialog_1b1c->m_field_41 = 0;
            }
        }
        if (m_dialog_1b1c->ProcessInput() == 0) {
            ClearActiveRegionIfMatches(0x138);
            m_controls_1af0->Invalidate(0);
            m_pages_1b0c[m_page_index_00c]->Refresh();
            m_header_dirty_010 = 1;
            unsigned char accepted = 0;
            if (m_capture_dialog_result_1b24) {
                accepted = static_cast<W8ModalDialogBase*>(m_dialog_1b1c)->close_result;
                m_capture_dialog_result_1b24 = 0;
            }
            delete m_dialog_1b1c;
            m_dialog_1b1c = 0;
            HandleDialogResult(m_dialog_response_1b20, accepted);
        }
    }
}

// FUNCTION: WIZ8 0x005b0580
void W8CharacterScreen::UpdateNavigation(W8CharacterPage* page)
{
    unsigned char next_enabled;
    unsigned char exit_enabled;
    page->GetNavigationState(&next_enabled, &exit_enabled);
    if (!m_next_1af8->m_flag_4 && next_enabled) {
        PlaySound("Data\\Sound\\Misc\\Points Spent.wav", 0);
    }
    if (m_next_1af8->m_flag_4 != next_enabled) {
        m_next_1af8->SetVisible(next_enabled);
        m_next_1af8->Invalidate(0);
    }
    if (m_exit_1afc->m_flag_4 != exit_enabled) {
        m_exit_1afc->SetVisible(exit_enabled);
        m_exit_1afc->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x005b0610
void W8CharacterScreen::ShowDialog005B0610(int value)
{
    m_dialog_response_1b20 = 0;
    m_dialog_1b1c = new W8SpellInfoDialog005EFAB0(value);
    m_dialog_1b1c->vslot5(&g_wchar_00689b34);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b06a0
void W8CharacterScreen::ShowProfessionInfo(unsigned int profession)
{
    m_dialog_response_1b20 = 0;
    m_dialog_1b1c = new W8ProfessionInfoDialog005EFBFC(profession);
    m_dialog_1b1c->vslot5(&g_wchar_00689b34);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b0730
void W8CharacterScreen::ShowRaceInfo(unsigned int race)
{
    m_dialog_response_1b20 = 0;
    m_dialog_1b1c = new W8RaceInfoDialog005EFC38(race);
    m_dialog_1b1c->vslot5(&g_wchar_00689b34);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b07c0
void W8CharacterScreen::ShowAttributeInfo005B07C0(unsigned int attribute)
{
    m_dialog_response_1b20 = 0;
    m_dialog_1b1c = new W8StatInfoDialog005DFC70(attribute);
    m_dialog_1b1c->vslot5(&g_wchar_00689b34);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b0850
void W8CharacterScreen::ShowAttributeInfo005B0850(unsigned int attribute)
{
    m_dialog_response_1b20 = 0;
    m_dialog_1b1c = new W8StatInfoDialog005E0180(attribute);
    m_dialog_1b1c->vslot5(&g_wchar_00689b34);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b08e0
void W8CharacterScreen::ShowDialog005B08E0(int value)
{
    m_dialog_response_1b20 = 0;
    if (value == g_profession_bonus_skills[m_character_018.current_profession]) {
        m_dialog_1b1c = new W8SkillInfoDialog005EFD08(value, 0, 0, 1);
    }
    else {
        m_dialog_1b1c = new W8SkillInfoDialog005EFD08(value, 0, 0, 0);
    }
    m_dialog_1b1c->vslot5(&g_wchar_00689b34);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b0a40
void W8CharacterScreen::OnPrimary(W8TextControl005ED604* control)
{
    if (control == m_accept_1b00) {
        if (m_mode_008 == 1 && !m_exit_1afc->m_flag_4) {
            RequestScreenTransition();
        }
        else {
            ShowMessage(gppStringList[0x34c / 4], 1, 2);
        }
    }
    else if (control == m_exit_1afc) {
        m_pages_1b0c[m_page_index_00c]->Accept();
    }
    else if (control == m_previous_1af4) {
        int index = m_page_index_00c - 1;
        while (index >= 0 && m_page_enabled_1b08[index] == 0) {
            --index;
        }
        if (index != -1) {
            SelectPage(index);
        }
    }
    else if (control == m_next_1af8) {
        AdvancePage(0);
    }
    else if (control == m_reset_1b04) {
        memset(m_page_enabled_1b08, 1, sizeof(m_page_enabled_1b08));
        m_force_transition_1aee = 1;
        Function556DC0(&m_character_018, &m_creation_state_187c);
        m_mode_008 = 0;
        m_pages_1b0c[3]->m_mode_068 = 0;
        m_reset_1b04->SetEnabled(0);
        SelectPage(0);
    }
}

void W8CharacterScreen::OnSecondary(W8TextControl005ED604*)
{
}

// FUNCTION: WIZ8 0x005b09b0
void W8CharacterScreen::ShowDescription(int first, int second)
{
    ShowMessage(FormatWideString(
        gppStringList[0x758 / 4],
        gppStringList[g_character_description_first_ids_61e3a4[first]],
        m_character_018.name,
        gppStringList[g_character_description_second_ids_61e454[second]],
        0, 0), 0, 0);
}

// FUNCTION: WIZ8 0x005b0a10
void W8CharacterScreen::ShowCharacterSummary()
{
    m_dialog_response_1b20 = 1;
    m_dialog_1b1c = Function5CF280(&m_character_018);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b0b50
void W8CharacterScreen::AdvancePage(unsigned char forward)
{
    if (!forward || (m_page_index_00c != 3 && m_next_1af8->m_flag_4)) {
        int index = m_page_index_00c;
        if (index == 1 && m_creation_state_187c.spell_points_remaining > 0 &&
            !m_block_advance_1aec) {
            ShowMessage(gppStringList[0x310 / 4], 1, 3);
            return;
        }
        if (index == 0) {
            if (m_mode_008 == 2 &&
                m_character_018.current_profession != m_original_014->current_profession &&
                !m_confirm_profession_1aed) {
                int value = Function557FD0(m_original_014, &m_character_018);
                if (value > 0) {
                    ShowMessage(FormatWideString(
                        gppStringList[0x36c / 4],
                        gppStringList[g_profession_name_message_ids_61e3f0[
                            m_original_014->current_profession]],
                        gppStringList[g_profession_name_message_ids_61e3f0[
                            m_character_018.current_profession]], value), 1, 4);
                }
                else {
                    ShowMessage(FormatWideString(
                        gppStringList[0x368 / 4],
                        gppStringList[g_profession_name_message_ids_61e3f0[
                            m_original_014->current_profession]],
                        gppStringList[g_profession_name_message_ids_61e3f0[
                            m_character_018.current_profession]]), 1, 4);
                }
                return;
            }
            m_page_enabled_1b08[1] = static_cast<unsigned char>(
                m_creation_state_187c.spell_points_total > 0);
        }
        m_block_advance_1aec = 0;
        m_confirm_profession_1aed = 0;
        do {
            ++index;
        } while (index < 4 && m_page_enabled_1b08[index] == 0);
        if (index < 4) {
            SelectPage(index);
        }
        else if (!m_force_transition_1aee) {
            if (CommitCharacter()) {
                RequestScreenTransition();
                if (m_mode_008 == 2 &&
                    m_character_018.experience_goal <= m_character_018.experience) {
                    g_dword_68ed10.mode = 3;
                    g_dword_68ed10.parameter_3 = m_original_014;
                    SetPendingScreenState(W8_SCREEN_CHARACTER);
                }
            }
        }
        else if (ValidateName()) {
            ShowMessage(gppStringList[0x358 / 4], 1, 5);
        }
    }
}

// FUNCTION: WIZ8 0x005b0d50
void W8CharacterScreen::SelectPage(int index)
{
    if (m_page_index_00c >= 0) {
        m_pages_1b0c[m_page_index_00c]->SetEnabled(0);
        m_pages_1b0c[m_page_index_00c]->Deactivate();
    }
    SyncCharacterForPage(index);
    if (m_pages_1b0c[index] == 0) {
        W8CharacterPage* page = 0;
        switch (index) {
        case 0: page = CreateCharacterPage005CBA90(); break;
        case 1: page = CreateCharacterPage005C8DE0(); break;
        case 2: page = CreateCharacterPage005C7CC0(); break;
        case 3: page = CreateCharacterPage005C73F0(); break;
        }
        page->m_screen_05c = this;
        page->SetCharacter(&m_character_018, &m_creation_state_187c, m_mode_008);
        m_pages_1b0c[index] = page;
    }
    m_page_index_00c = index;
    W8CharacterPage* page = m_pages_1b0c[index];
    page->Activate();
    page->SetEnabled(1);

    int previous = index - 1;
    while (previous >= 0 && m_page_enabled_1b08[previous] == 0) --previous;
    m_previous_1af4->SetEnabled(static_cast<unsigned char>(previous != -1));
    int next = index + 1;
    while (next < 4 && m_page_enabled_1b08[next] == 0) ++next;
    if (next == 4) {
        m_next_1af8->m_text_48 = 0x10;
        m_next_1af8->m_text_4c = 0x12;
        m_next_1af8->m_text_50 = 0x12;
        m_next_1af8->m_text_54 = 0x11;
        m_next_1af8->m_text_58 = 0x13;
        m_next_1af8->EnableRegionHelp(0xe0);
    }
    else {
        m_next_1af8->m_text_48 = 8;
        m_next_1af8->m_text_4c = 10;
        m_next_1af8->m_text_50 = 10;
        m_next_1af8->m_text_54 = 9;
        m_next_1af8->m_text_58 = 0xb;
        m_next_1af8->EnableRegionHelp(0xdc);
    }
    m_controls_1af0->Invalidate(0);
    page->Prepare();
    m_header_dirty_010 = 1;
    UpdateNavigation(page);
}

// FUNCTION: WIZ8 0x005b0f30
void W8CharacterScreen::SyncCharacterForPage(int index)
{
    if (m_page_index_00c > index) return;
    if (index == 0) {
        if (m_mode_008 == 0) Function556DC0(&m_character_018, &m_creation_state_187c);
        else if (m_mode_008 == 2) Function556CC0(&m_character_018, &m_creation_state_187c);
    }
    else if (index == 1) {
        Function558180(&m_character_018, &m_creation_state_187c);
    }
    else if (index == 3) {
        if (m_character_018.table_value_0079 < 0) Function4EFA30(&m_character_018);
        if (m_character_018.current_profession < 0) CalcCharacterTableValue(&m_character_018);
    }
}

// FUNCTION: WIZ8 0x005b1110
void W8CharacterScreen::DrawHeader()
{
    W8TextBuffer005ED5B8 text;
    Function549600(-14, 0x107, 0, 0, 0xc3, 0, 2, 0);
    W8ControlsRect bounds = {0xc3, 0, 0x285, 0x2c};
    text.SetLayoutBounds(&bounds, 1, 1);
    text.SetText(gppStringList[g_character_page_title_ids_64da9c[m_page_index_00c]],
                 g_options_detail_font_683614);
    text.RenderToTarget(0, 1, -14);
    Function549600(-14, 0x107, 0, 1, 0, 0, 2, 0);

    if (m_mode_008 == 0) {
        Function548F90(-14, 0x107, 0, 2, 10, 0xc, 2, 0);
    }
    else {
        int frame = m_original_014 == 0
            ? m_character_018.table_value_0079
            : m_original_014->table_value_0079;
        Function548F90(-14, 0x11, frame, 0, 10, 0xc, 2, 0);
    }

    if (m_mode_008 == 0 && m_page_index_00c == 0) {
        Function548F90(-14, 0x107, 0, 3, 5, 0xa5, 2, 0);
    }
    else {
        bounds.left = 5;
        bounds.top = 0xa5;
        bounds.right = 0xc1;
        bounds.bottom = 0xdf;
        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED558 |
                           g_W8TextBufferLayoutMask005ED54C);
        if (m_mode_008 != 0) {
            text.SetLayoutBounds(&bounds, 1, 1);
            text.SetText(m_character_018.name, g_font_683660);
            text.RenderToTarget(0, 1, -14);
        }
        bounds.top += 0xe;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(FormatWideString(
            L"%s %s",
            gppStringList[g_faction_name_message_rows_61e430[
                m_character_018.faction][0]],
            gppStringList[g_race_name_message_ids_61e3d0[m_character_018.race]]),
            g_font_683660);
        text.RenderToTarget(0, 1, -14);
        bounds.top += 0xe;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[g_profession_name_message_ids_61e3f0[
                         m_character_018.current_profession]],
                     g_font_683660);
        text.RenderToTarget(0, 1, -14);
        bounds.top += 0xe;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(FormatWideString(
            L"%s %d (%s)", gppStringList[0x1ae4 / 4], m_character_018.level,
            gppStringList[g_profession_level_name_message_ids_61e688[
                m_character_018.current_profession][m_character_018.level_band]]),
            g_font_683660);
        text.RenderToTarget(0, 1, -14);
    }
    m_header_dirty_010 = 0;
}

// FUNCTION: WIZ8 0x005b0fd0
unsigned char W8CharacterScreen::CommitCharacter()
{
    if (!ValidateName()) return 0;

    int mode = m_mode_008;
    W8Character backup;
    memcpy(&backup, &m_character_018, sizeof(backup));
    if (mode != 1) {
        Function557580(&m_character_018, &m_creation_state_187c,
                       static_cast<unsigned char>(mode == 0));
    }
    if (!g_status_685170.game_started &&
        !g_status_685170.skip_loose_character_check_2444) {
        if (m_original_014 != 0) {
            char path[260];
            BuildCharacterPath00514EC0(path, m_original_014->name, -1);
            DeleteFileA(path);
        }
        m_character_018.in_party = 0;
        if (!SaveCharacter(&m_character_018, -1, 0, 0)) {
            memcpy(&m_character_018, &backup, sizeof(m_character_018));
            ShowMessage(gppStringList[0x350 / 4], 0, 0);
            return 0;
        }
        m_character_018.in_party = backup.in_party;
    }
    if (m_original_014 != 0) {
        unsigned char was_in_party = m_original_014->in_party;
        memcpy(m_original_014, &m_character_018, sizeof(*m_original_014));
        if (was_in_party) m_original_014->in_party = 1;
        if (m_original_014->current_profession == 8) Function5218C0(m_original_014);
        Function51D960(m_original_014);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005b1430
void W8CharacterScreen::ShowMessage(wchar_t* text, int confirmation, int response)
{
    m_dialog_response_1b20 = response;
    m_dialog_1b1c = new W8ModalDialogBase;
    if (m_dialog_1b1c != 0) {
        m_dialog_1b1c->SetExtent(0xf0, 0xbe);
        m_dialog_1b1c->SetOrigin(0xa0, 100);
        m_dialog_1b1c->SetBackground("Data\\Dialogs\\DialogBackground.sti", 0);
        static_cast<W8ModalDialogBase*>(m_dialog_1b1c)->SetClientExtent(0xfa, 200);
        static_cast<W8ModalDialogBase*>(m_dialog_1b1c)->SetMessage(
            text, 1, 0x32, 1, confirmation, 1, 1, 0, 0x15e);
        ActivateDialogRegion(0x138);
        m_capture_dialog_result_1b24 = 1;
    }
}

// FUNCTION: WIZ8 0x005b1520
void W8CharacterScreen::HandleDialogResult(int response, unsigned char accepted)
{
    if (accepted) {
        switch (response) {
        case 2:
            RequestScreenTransition();
            break;
        case 3:
            m_block_advance_1aec = 1;
            AdvancePage(0);
            break;
        case 4:
            m_confirm_profession_1aed = 1;
            AdvancePage(0);
            break;
        case 5: {
            int value = Function558640(&m_character_018);
            int next_response;
            wchar_t* format;
            if (Function5586B0(&m_character_018)) {
                format = gppStringList[0x35c / 4];
                next_response = 6;
            }
            else {
                format = gppStringList[0x360 / 4];
                next_response = 7;
            }
            ShowMessage(FormatWideString(format, value, 1, next_response),
                        1, next_response);
            break;
        }
        case 6:
            Function557580(&m_character_018, &m_creation_state_187c, 0);
            Function4EF7E0(m_original_014, &m_character_018, 1);
            RequestScreenTransition();
            break;
        case 7:
            Function557580(&m_character_018, &m_creation_state_187c, 0);
            Function4EF7E0(m_original_014, &m_character_018, 0);
            RequestScreenTransition();
            break;
        }
    }
    else if (response == 6) {
        Function557580(&m_character_018, &m_creation_state_187c, 0);
        Function4EF7E0(m_original_014, &m_character_018, 0);
        RequestScreenTransition();
    }
}

// FUNCTION: WIZ8 0x005b1670
unsigned char W8CharacterScreen::ValidateName()
{
    if (m_original_014 != 0 &&
        wcscmp(m_original_014->name, m_character_018.name) == 0) {
        return 1;
    }
    if (!g_status_685170.game_started &&
        !g_status_685170.skip_loose_character_check_2444) {
        char path[260];
        BuildCharacterPath00514EC0(path, m_character_018.name, -1);
        if (FileExists(path)) {
            ShowMessage(gppStringList[0x354 / 4], 0, 0);
            return 0;
        }
    }
    for (int index = 0; index < W8_PARTY_SLOT_COUNT; ++index) {
        if (g_status_685170.buffers.party_rows[index].occupied &&
            wcscmp(g_status_685170.buffers.characters[index].name,
                   m_character_018.name) == 0) {
            ShowMessage(gppStringList[0x354 / 4], 0, 0);
            return 0;
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x005b0120
unsigned char W8CharacterScreen::HasDialog()
{
    return m_dialog_1b1c != 0;
}

// FUNCTION: WIZ8 0x005b0130
W8Character* W8CharacterScreen::GetOriginalCharacter()
{
    return m_original_014;
}

// FUNCTION: WIZ8 0x005b1750
unsigned char CharacterScreenEnter005B1750(void)
{
    SetViewport(0, 0, 0x280, 0x1e0);
    Function425570(0);
    MSYS_Init();
    ResetRegions();
    UpdateHeldItemCursor();
    SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
    SetFontObjectPalette16BPP(g_wiz_text_bold_font_683664,
                              g_font_palette_wiz_text_bold_68ee0c);
    g_character_screen_0069c2e8 = new W8CharacterScreen(
        g_screen_state_0068ec78.mode,
        static_cast<W8Character*>(g_screen_state_0068ec78.parameter_3));
    g_character_screen_0069c2e8->BuildControls();
    if (!g_status_685170.game_started &&
        (g_screen_state_0068ec78.mode == 0 || g_screen_state_0068ec78.mode == 2)) {
        Function48FC10("Menus.MPL", 1, 1);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005b1840
unsigned char CharacterScreenLeave005B1840(int leaving)
{
    if (leaving) {
        W8CharacterScreen* screen = g_character_screen_0069c2e8;
        if (screen != 0) {
            screen->m_pages_1b0c[screen->m_page_index_00c]->Deactivate();
            for (int index = 0; index < 4; ++index) {
                delete screen->m_pages_1b0c[index];
            }
            screen->m_controls_1af0->DestroyAllControls();
            delete screen->m_controls_1af0;
            ::operator delete(screen);
        }
        g_character_screen_0069c2e8 = 0;
    }
    NoOp();
    MSYS_Shutdown();
    ResetRegions();
    return 1;
}

// FUNCTION: WIZ8 0x005b18e0
void CharacterScreenFrame005B18E0(void)
{
    W8ScreenPoint point;
    InputAtom input;
    GetScreenPoint004284F0(&point);
    g_character_screen_0069c2e8->UpdateDialog();
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, static_cast<unsigned short>(point.x),
                                static_cast<unsigned short>(point.y),
                                g_flag_6f04ed, g_flag_6f04e8);
    UpdateRegionMousePosition(point.x, point.y);
    while (DequeueEvent(&input) == 1) {
        W8CharacterScreen* screen = g_character_screen_0069c2e8;
        screen->m_pages_1b0c[screen->m_page_index_00c]->HandleInput(&input);
        if (!DispatchRegionInput(&input) && input.usEvent == KEY_DOWN) {
            if (input.usParam == 0xd || input.usParam == 0x27 || input.usParam == 0x4e) {
                screen->AdvancePage(1);
            }
            else if (input.usParam == 0x1b) {
                if (screen->m_mode_008 == 1 && !screen->m_exit_1afc->m_flag_4) {
                    RequestScreenTransition();
                }
                else {
                    screen->ShowMessage(gppStringList[0x34c / 4], 1, 2);
                }
            }
            else if ((input.usParam == 0x25 || input.usParam == 0x42) &&
                     screen->m_page_index_00c != 3) {
                int index = screen->m_page_index_00c - 1;
                while (index >= 0 && !screen->m_page_enabled_1b08[index]) --index;
                if (index != -1) screen->SelectPage(index);
            }
        }
    }
    W8CharacterScreen* screen = g_character_screen_0069c2e8;
    if (screen->m_header_dirty_010) screen->DrawHeader();
    screen->m_pages_1b0c[screen->m_page_index_00c]->Redraw();
    screen->m_controls_1af0->Redraw();
    if (screen->m_dialog_1b1c != 0) screen->m_dialog_1b1c->vslot3();
    Function426790();
}
