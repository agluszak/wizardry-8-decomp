#include "soundman.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/dialog_code/SpellInfoDialog.h"
#include "wiz8/dialog_code/CharacterSummaryDialog.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"

#include "wiz8/cursor.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/dialog_code/MessageDialogBase.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/ProfRaceInfoDialog.h"
#include "wiz8/dialog_code/StatInfoDialogs.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/music_playlist.h"
#include "wiz8/sound_man.h"
#include "wiz8/regions.h"
#include "wiz8/fonts.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"

#include "input.h"
#include "Types.h"
#include "mousesystem.h"

#include "Font.h"

#include <new>
#include <string.h>
#include <wchar.h>

#include "FileMan.h"
#include "wiz8/local_screens/OptionsScreen.h"

// GLOBAL: WIZ8 0x0061e3a4
unsigned short g_character_description_first_ids[22] = {
    0x274, 0x275, 0x276, 0x277, 0x278, 0x279, 0x27a, 0,     0x27b, 0x27c, 0x27d,
    0x27e, 0x27f, 0x280, 0x281, 0,     0x6ac, 0x6ad, 0x6ae, 0x6af, 0x6b0, 0,
};
// GLOBAL: WIZ8 0x0061e3d0
unsigned short g_race_name_message_ids[16] = {
    0x284, 0x285, 0x286, 0x287, 0x288, 0x289, 0x28a, 0x28b,
    0x28c, 0x28d, 0x28e, 0x28f, 0x290, 0x291, 0x292, 0x293,
};
// GLOBAL: WIZ8 0x0061e3f0
unsigned short g_profession_name_message_ids[32] = {
    0x2a4, 0x2a5, 0x2a6, 0x2a7, 0x2a8, 0x2a9, 0x2aa, 0x2ab, 0x2ac, 0x2ad, 0x2ae,
    0x2af, 0x2b0, 0x2b1, 0x2b2, 0,     0x2c2, 0x2c3, 0x2c4, 0x2c5, 0x2c6, 0x2c7,
    0x2c8, 0x2c9, 0x2ca, 0x2cb, 0x2cc, 0x2cd, 0x2ce, 0x2cf, 0x2d0, 0,
};
// GLOBAL: WIZ8 0x0061e430
unsigned short g_gender_name_message_rows[4][4] = {
    {0x2d1, 0x2d4, 0x2d7, 0x2da},
    {0x2d2, 0x2d5, 0x2d8, 0x2db},
    {0x2d3, 0x2d6, 0x2d9, 0x2dc},
    {0x2dd, 0x2de, 0x2df, 0x2e0},
};
// GLOBAL: WIZ8 0x0061e454
unsigned short g_character_skill_name_ids[84] = {
    0x2e2, 0x2e3, 0x2e4, 0x2e5, 0x2e6, 0x2e7, 0x2e8, 0x2e9, 0x2ea, 0x2eb, 0x2ec, 0x2ed,
    0x2ee, 0x2ef, 0x2f0, 0x2f1, 0x2f2, 0x2f3, 0x2f4, 0x2f5, 0x2f6, 0x2f7, 0x2f8, 0x2f9,
    0x2fa, 0x2fb, 0x2fc, 0x2fd, 0x2fe, 0x2ff, 0x300, 0x301, 0x302, 0x303, 0x304, 0x305,
    0x306, 0x307, 0x308, 0x309, 0x30a, 0,     0x677, 0x678, 0x679, 0x67a, 0x67b, 0x67c,
    0x67d, 0x67e, 0x67f, 0x680, 0x682, 0x683, 0x684, 0x685, 0x681, 0x686, 0x687, 0x688,
    0x689, 0x68a, 0x68b, 0x68c, 0x68d, 0x68e, 0x68f, 0x690, 0x691, 0x692, 0x693, 0x694,
    0x695, 0x696, 0x697, 0x698, 0x699, 0x69a, 0x69b, 0x69c, 0x69d, 0x69e, 0x69f, 0,
};
// GLOBAL: WIZ8 0x0061e674
unsigned short g_personality_message_ids[10] = {
    0x3ad, 0x3ae, 0x3af, 0x3b0, 0x3b1, 0x3b2, 0x3b3, 0x3b4, 0x3b5, 0,
};
// GLOBAL: WIZ8 0x0061e688
unsigned short g_profession_level_name_message_ids[15][9] = {
    {0x3b6, 0x3b7, 0x3b8, 0x3b9, 0x3ba, 0x3bb, 0x3bc, 0x3bd, 0x3be},
    {0x3b6, 0x3b7, 0x3bf, 0x3c0, 0x3c1, 0x3c2, 0x3c3, 0x3c4, 0x3c5},
    {0x3b6, 0x3b7, 0x3c6, 0x3b9, 0x3c7, 0x3c2, 0x3c8, 0x3c9, 0x3ca},
    {0x3b6, 0x3b7, 0x3cb, 0x3cc, 0x3cd, 0x3ce, 0x3cf, 0x3d0, 0x3d1},
    {0x3b6, 0x3b7, 0x3d2, 0x3d3, 0x3d4, 0x3d5, 0x3d6, 0x3bd, 0x3d7},
    {0x3b6, 0x3b7, 0x3d8, 0x3d9, 0x3da, 0x3db, 0x3dc, 0x3dd, 0x3de},
    {0x3b6, 0x3b7, 0x3f9, 0x3df, 0x3e0, 0x3e1, 0x3dc, 0x3e2, 0x3e3},
    {0x3b6, 0x3b7, 0x3e4, 0x3e5, 0x3e6, 0x3e7, 0x3e8, 0x3e9, 0x3ea},
    {0x3b6, 0x3b7, 0x3eb, 0x3ec, 0x3ed, 0x3ee, 0x3ef, 0x3f0, 0x3f1},
    {0x3b6, 0x3b7, 0x3f2, 0x3f3, 0x3f4, 0x3f5, 0x3f6, 0x3f7, 0x3f8},
    {0x3b6, 0x3b7, 0x3f9, 0x3fa, 0x3fb, 0x3fc, 0x3fd, 0x3fe, 0x3ff},
    {0x3b6, 0x3b7, 0x400, 0x401, 0x402, 0x403, 0x404, 0x405, 0x406},
    {0x3b6, 0x3b7, 0x407, 0x408, 0x409, 0x40a, 0x40b, 0x40c, 0x40d},
    {0x3b6, 0x3b7, 0x40e, 0x40f, 0x410, 0x411, 0x412, 0x413, 0x414},
    {0x3b6, 0x3b7, 0x415, 0x416, 0x417, 0x418, 0x419, 0x41a, 0x41b},
};
// GLOBAL: WIZ8 0x0064da9c
static int g_character_page_title_ids[4] = {0xcf, 0xd1, 0xd0, 0xd2};

// GLOBAL: WIZ8 0x0069c2e4
static unsigned int g_character_screen_region_set;
// GLOBAL: WIZ8 0x0069c2e8
W8CharacterScreen* g_character_screen;

// VTABLE: WIZ8 0x005ef224 W8CharacterPageHost
// VTABLE: WIZ8 0x005ef21c W8TextControl::Listener
// class W8CharacterScreen

// FUNCTION: WIZ8 0x005b0040
W8CharacterScreen::W8CharacterScreen(int mode, W8Character* character)
    : m_mode(mode), m_original(character), m_block_advance(0),
      m_confirm_profession(0), m_force_transition(0), m_dialog(0),
      m_capture_dialog_result(0)
{
    if (character != 0) {
        memcpy(&m_character, character, sizeof(m_character));
        m_character.fInParty = false;
    }
    for (int index = 0; index < 4; ++index) {
        m_pages[index] = 0;
    }
    if (m_mode == 3) {
        m_mode = 2;
        SoundPlay("Data\\Sound\\Misc\\GainLevel.wav", 0);
        ShowMessage(FormatWideString(gppStringList[0xd9], m_character.name, 0, 0), 0, 0);
    }
    m_page_enabled[0] = m_mode != 1;
    m_page_enabled[1] = m_mode != 1;
    m_page_enabled[2] = m_mode != 1;
    m_page_enabled[3] = m_mode != 2;
}

// FUNCTION: WIZ8 0x005b0140
void W8CharacterScreen::BuildControls()
{
    m_controls = new Controls(0, 0x1c2, 0, 0, 0x107, 0, 4);
    m_controls->AcquireRegionSet(&g_character_screen_region_set);

    m_next =
        new W8TextControl(m_controls, 0xffffffff, 0x254, 0, 0, 0, 0x106, 0, 8, 10, 9, 10, 0xb);
    m_next->EnableRegionHelp(0xdc);
    m_next->m_listener = this;

    m_previous = new W8TextControl(m_controls, 0xffffffff, 0x228, 0, 0, 0, 0x106, 0, 0xc,
                                        0xe, 0xd, 0xe, 0xf);
    m_previous->EnableRegionHelp(0xdd);
    m_previous->m_listener = this;

    m_exit = new W8TextControl(m_controls, 0xffffffff, 0x1fc, 0, 0, 0, 0x106, 0, 0x14,
                                    0x16, 0x15, 0x16, 0x17);
    m_exit->EnableRegionHelp(0xde);
    m_exit->m_listener = this;

    m_accept =
        new W8TextControl(m_controls, 0xffffffff, 0x1d0, 0, 0, 0, 0x106, 0, 4, 6, 5, 6, 7);
    m_accept->EnableRegionHelp(0xdf);
    m_accept->m_listener = this;

    m_reset = new W8TextControl(m_controls, 0xffffffff, 0, 0, 0, 0, 0x106, 0, 0x1d, 0x1f,
                                     0x1e, 0x1f, 0x20);
    m_reset->EnableRegionHelp(0xe1);
    m_reset->m_listener = this;

    m_controls->SetEnabled(1);
    m_controls->EnableRegionSet(1);
    m_controls->Invalidate(0);
    m_reset->SetActive(0);
    if (m_mode == 1 && g_status.game_started != 0 &&
        CharacterPointerToPartySlot(m_original) > 1) {
        m_reset->SetActive(1);
        if (gXStatus.fCombatMode != 0) {
            m_reset->SetEnabled(0);
        }
    }

    m_page_index = -1;
    int index = 0;
    while (index < 4 && !m_page_enabled[index]) {
        ++index;
    }
    SelectPage(index < 4 ? index : -1);
}

// FUNCTION: WIZ8 0x005b04b0
void W8CharacterScreen::UpdateDialog()
{
    if (m_dialog != 0) {
        if (m_dialog_response == 1) {
            gXStatus.character_event_queue->ProcessDeferredCharacterEvents();
            if (UpdateCharacterEventState() == 0 &&
                static_cast<W8MessageDialogBase*>(m_dialog)->close_result) {
                m_dialog->m_keep_open = false;
            }
        }
        if (m_dialog->ProcessInput() == 0) {
            ClearActiveRegionIfMatches(0x138);
            m_controls->Invalidate(0);
            m_pages[m_page_index]->Prepare();
            m_header_dirty = true;
#pragma clang diagnostic push
#pragma clang diagnostic ignored                                                                   \
    "-Wsometimes-uninitialized" // uninit-ok: uncaptured response can use a stale stack byte as acceptance
            unsigned char accepted;
            if (m_capture_dialog_result) {
                accepted = static_cast<W8MessageDialogBase*>(m_dialog)->close_result;
                m_capture_dialog_result = 0;
            }
            delete m_dialog;
            m_dialog = 0;
            HandleDialogResult(m_dialog_response, accepted);
#pragma clang diagnostic pop
        }
    }
}

// FUNCTION: WIZ8 0x005b0580
void W8CharacterScreen::UpdateNavigation(W8CharacterPage* page)
{
    bool next_enabled;
    bool exit_enabled;
    page->GetNavigationState(&next_enabled, &exit_enabled);
    if (!m_next->m_enabled && next_enabled) {
        SoundPlay("Data\\Sound\\Misc\\Points Spent.wav", 0);
    }
    if (m_next->m_enabled != next_enabled) {
        m_next->SetEnabled(next_enabled);
        m_next->Invalidate(0);
    }
    if (m_exit->m_enabled != exit_enabled) {
        m_exit->SetEnabled(exit_enabled);
        m_exit->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x005b0610
void W8CharacterScreen::ShowSpellInfo(int value)
{
    m_dialog_response = 0;
    m_dialog = new W8SpellInfoDialog(value);
    m_dialog->SetText(&g_empty_wide_string);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b06a0
void W8CharacterScreen::ShowProfessionInfo(unsigned int profession)
{
    m_dialog_response = 0;
    m_dialog = new W8ProfessionInfoDialog(profession);
    m_dialog->SetText(&g_empty_wide_string);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b0730
void W8CharacterScreen::ShowRaceInfo(unsigned int race)
{
    m_dialog_response = 0;
    m_dialog = new W8RaceInfoDialog(race);
    m_dialog->SetText(&g_empty_wide_string);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b07c0
void W8CharacterScreen::ShowPrimaryAttributeInfo(unsigned int attribute)
{
    m_dialog_response = 0;
    m_dialog = new W8AttributeInfoDialog(attribute);
    m_dialog->SetText(&g_empty_wide_string);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b0850
void W8CharacterScreen::ShowSecondaryAttributeInfo(unsigned int attribute)
{
    m_dialog_response = 0;
    m_dialog = new W8SecondaryAttributeInfoDialog(attribute);
    m_dialog->SetText(&g_empty_wide_string);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b08e0
void W8CharacterScreen::ShowSkillInfo(int value)
{
    m_dialog_response = 0;
    if (value == g_profession_bonus_skills[m_character.iProfession]) {
        m_dialog = new W8SkillInfoDialog(value, 0, 0, 1);
    } else {
        m_dialog = new W8SkillInfoDialog(value, 0, 0, 0);
    }
    m_dialog->SetText(&g_empty_wide_string);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b0a40
void W8CharacterScreen::OnPrimary(W8TextControl* control)
{
    if (control == m_accept) {
        if (m_mode == 1 && !m_exit->m_enabled) {
            RequestScreenTransition();
        } else {
            ShowMessage(gppStringList[0xd3], 1, 2);
        }
    } else if (control == m_exit) {
        m_pages[m_page_index]->Accept();
    } else if (control == m_previous) {
        int index = m_page_index - 1;
        while (index >= 0 && !m_page_enabled[index]) {
            --index;
        }
        if (index != -1) {
            SelectPage(index);
        }
    } else if (control == m_next) {
        AdvancePage(0);
    } else if (control == m_reset) {
        memset(m_page_enabled, 1, sizeof(m_page_enabled));
        m_force_transition = 1;
        InitializeCharacterCreation(&m_character, &m_creation_state);
        m_mode = 0;
        m_pages[3]->m_mode = 0;
        m_reset->SetActive(0);
        SelectPage(0);
    }
}

void W8CharacterScreen::OnSecondary(W8TextControl*) {}

// FUNCTION: WIZ8 0x005b09b0
void W8CharacterScreen::ShowDescription(int first, int second)
{
    ShowMessage(FormatWideString(
                    gppStringList[0x1d6], gppStringList[g_character_description_first_ids[first]],
                    m_character.name, gppStringList[g_character_skill_name_ids[second]]),
                0, 0);
}

// FUNCTION: WIZ8 0x005b0a10
void W8CharacterScreen::ShowCharacterSummary()
{
    m_dialog_response = 1;
    m_dialog = CreateCharacterSummaryDialog(&m_character);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005b0b50
void W8CharacterScreen::AdvancePage(bool forward)
{
    if (!forward || (m_page_index != 3 && m_next->m_enabled)) {
        int index = m_page_index;
        if (index == 1 && m_creation_state.spell_points_remaining > 0 &&
            !m_block_advance) {
            ShowMessage(gppStringList[0xc4], 1, 3);
            return;
        }
        if (index == 0) {
            if (m_mode == 2 && m_character.iProfession != m_original->iProfession &&
                !m_confirm_profession) {
                int value = ComputeRealmSkillDebt(m_original, &m_character);
                if (value > 0) {
                    ShowMessage(
                        FormatWideString(
                            gppStringList[0xdb],
                            gppStringList
                                [g_profession_name_message_ids[m_original->iProfession]],
                            gppStringList
                                [g_profession_name_message_ids[m_character.iProfession]],
                            value),
                        1, 4);
                } else {
                    ShowMessage(
                        FormatWideString(
                            gppStringList[0xda],
                            gppStringList
                                [g_profession_name_message_ids[m_original->iProfession]],
                            gppStringList
                                [g_profession_name_message_ids[m_character.iProfession]]),
                        1, 4);
                }
                return;
            }
            m_page_enabled[1] = m_creation_state.spell_points_total > 0;
        }
        m_block_advance = 0;
        m_confirm_profession = 0;
        do {
            ++index;
        } while (index < 4 && !m_page_enabled[index]);
        if (index < 4) {
            SelectPage(index);
        } else if (!m_force_transition) {
            if (CommitCharacter()) {
                RequestScreenTransition();
                if (m_mode == 2 &&
                    m_character.experience_goal <= m_character.experience) {
                    g_pending_screen_state.mode = 3;
                    g_pending_screen_state.parameter_3 = m_original;
                    SetPendingScreenState(W8_SCREEN_CHARACTER);
                }
            }
        } else if (ValidateName()) {
            ShowMessage(gppStringList[0xd6], 1, 5);
        }
    }
}

// FUNCTION: WIZ8 0x005b0d50
void W8CharacterScreen::SelectPage(int index)
{
    if (m_page_index >= 0) {
        m_pages[m_page_index]->SetEnabled(0);
        m_pages[m_page_index]->Deactivate();
    }
    SyncCharacterForPage(index);
    if (m_pages[index] == 0) {
        W8CharacterPage* page = 0;
        switch (index) {
        case 0:
            page = CreateCharacterStatsPage();
            break;
        case 1:
            page = CreateCharacterSpellsPage();
            break;
        case 2:
            page = CreateCharacterSkillsPage();
            break;
        case 3:
            page = CreateCharacterPersonalityPage();
            break;
        }
        page->m_screen = this;
        page->SetCharacter(&m_character, &m_creation_state, m_mode);
        m_pages[index] = page;
    }
    m_page_index = index;
    W8CharacterPage* page = m_pages[index];
    page->Activate();
    page->SetEnabled(1);

    int previous = index - 1;
    while (previous >= 0 && !m_page_enabled[previous])
        --previous;
    m_previous->SetActive(previous != -1);
    int next = index + 1;
    while (next < 4 && !m_page_enabled[next])
        ++next;
    if (next == 4) {
        m_next->m_normalSprite = 0x10;
        m_next->m_pressedSprite = 0x12;
        m_next->m_alternatePressedSprite = 0x12;
        m_next->m_alternateNormalSprite = 0x11;
        m_next->m_disabledSprite = 0x13;
        m_next->EnableRegionHelp(0xe0);
    } else {
        m_next->m_normalSprite = 8;
        m_next->m_pressedSprite = 10;
        m_next->m_alternatePressedSprite = 10;
        m_next->m_alternateNormalSprite = 9;
        m_next->m_disabledSprite = 0xb;
        m_next->EnableRegionHelp(0xdc);
    }
    m_controls->Invalidate(0);
    page->Prepare();
    m_header_dirty = true;
    UpdateNavigation(page);
}

// FUNCTION: WIZ8 0x005b0f30
void W8CharacterScreen::SyncCharacterForPage(int index)
{
    if (m_page_index > index)
        return;
    if (index == 0) {
        if (m_mode == 0)
            InitializeCharacterCreation(&m_character, &m_creation_state);
        else if (m_mode == 2)
            InitializeCharacterLevelUp(&m_character, &m_creation_state);
    } else if (index == 1) {
        CountRemainingSpellPoints(&m_character, &m_creation_state);
    } else if (index == 3) {
        if (m_character.personality < 0)
            DeriveCharacterPersonality(&m_character);
        if (m_character.portrait_index < 0)
            CalcCharacterTableValue(&m_character);
    }
}

// FUNCTION: WIZ8 0x005b1110
void W8CharacterScreen::DrawHeader()
{
    W8TextBuffer text;
    DrawCatalogImageAndInvalidate(-14, 0x107, 0, 0, 0xc3, 0, 2, 0);
    W8ControlsRect bounds = {0xc3, 0, 0x285, 0x2c};
    text.SetLayoutBounds(&bounds, 1, 1);
    text.SetText(gppStringList[g_character_page_title_ids[m_page_index]],
                 g_options_detail_font);
    text.RenderToTarget(0, 1, -14);
    DrawCatalogImageAndInvalidate(-14, 0x107, 0, 1, 0, 0, 2, 0);

    if (m_mode == 0) {
        DrawCatalogImage(-14, 0x107, 0, 2, 10, 0xc, 2, 0);
    } else {
        int frame =
            m_original == 0 ? m_character.portrait_index : m_original->portrait_index;
        DrawCatalogImage(-14, 0x11, frame, 0, 10, 0xc, 2, 0);
    }

    if (m_mode == 0 && m_page_index == 0) {
        DrawCatalogImage(-14, 0x107, 0, 3, 5, 0xa5, 2, 0);
    } else {
        bounds.left = 5;
        bounds.top = 0xa5;
        bounds.right = 0xc1;
        bounds.bottom = 0xdf;
        text.SetLayoutMode(g_W8TextBufferAlignTop | g_W8TextBufferAlignCenter);
        if (m_mode != 0) {
            text.SetLayoutBounds(&bounds, 1, 1);
            text.SetText(m_character.name, g_wiz_text_font_secondary);
            text.RenderToTarget(0, 1, -14);
        }
        bounds.top += 0xe;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(
            FormatWideString(L"%s %s",
                             gppStringList[g_gender_name_message_rows[m_character.gender][0]],
                             gppStringList[g_race_name_message_ids[m_character.iRace]]),
            g_wiz_text_font_secondary);
        text.RenderToTarget(0, 1, -14);
        bounds.top += 0xe;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[g_profession_name_message_ids[m_character.iProfession]],
                     g_wiz_text_font_secondary);
        text.RenderToTarget(0, 1, -14);
        bounds.top += 0xe;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(
            FormatWideString(
                L"%s %d (%s)", gppStringList[0x6b9], m_character.uiExpLevel,
                gppStringList[g_profession_level_name_message_ids[m_character.iProfession]
                                                                 [m_character.level_band]]),
            g_wiz_text_font_secondary);
        text.RenderToTarget(0, 1, -14);
    }
    m_header_dirty = false;
}

// FUNCTION: WIZ8 0x005b0fd0
bool W8CharacterScreen::CommitCharacter()
{
    if (!ValidateName())
        return false;

    int mode = m_mode;
    W8Character backup;
    memcpy(&backup, &m_character, sizeof(backup));
    if (mode != 1) {
        FinalizeCreatedCharacter(&m_character, &m_creation_state, mode == 0);
    }
    if (!g_status.game_started && !g_status.skip_loose_character_check) {
        if (m_original != 0) {
            char path[260];
            BuildCharacterPath(path, m_original->name, -1);
            DeleteFileA(path);
        }
        m_character.fInParty = false;
        if (!SaveCharacter(&m_character, -1, 0, 0)) {
            memcpy(&m_character, &backup, sizeof(m_character));
            ShowMessage(gppStringList[0xd4], 0, 0);
            return false;
        }
        m_character.fInParty = backup.fInParty;
    }
    if (m_original != 0) {
        bool was_in_party = m_original->fInParty;
        memcpy(m_original, &m_character, sizeof(*m_original));
        if (was_in_party)
            m_original->fInParty = true;
        if (m_original->iProfession == W8_PROFESSION_GADGETEER)
            UpgradeProfessionClassItem(m_original);
        UnequipUnusableItems(m_original);
    }
    return true;
}

/* Skill-availability hooks raised by RefreshCharacterSkillAvailability while this screen is
   current. They adjust the named skill through the page-2 helpers and then
   refresh page 2, the skills list. */
// FUNCTION: WIZ8 0x005b1af0
void ResetCharacterScreenSkill(int skill_id)
{
    W8CharacterScreen* screen = g_character_screen;
    ResetSkillContribution(&screen->m_character, &screen->m_creation_state, skill_id);
    if (screen->m_pages[2] != 0) {
        screen->m_pages[2]->Refresh();
    }
}

// FUNCTION: WIZ8 0x005b1b30
void RefundCharacterScreenSkill(int skill_id)
{
    W8CharacterScreen* screen = g_character_screen;
    RefundSkillAllocation(&screen->m_character, &screen->m_creation_state, skill_id);
    if (screen->m_pages[2] != 0) {
        screen->m_pages[2]->Refresh();
    }
}

// FUNCTION: WIZ8 0x005b1430
void W8CharacterScreen::ShowMessage(wchar_t* text, int confirmation, int response)
{
    m_dialog_response = response;
    m_dialog = new W8MessageDialogBase;
    if (m_dialog != 0) {
        m_dialog->SetExtent(0xf0, 0xbe);
        m_dialog->SetOrigin(0xa0, 100);
        m_dialog->SetBackground("Data\\Dialogs\\DialogBackground.sti", 0);
        static_cast<W8MessageDialogBase*>(m_dialog)->SetClientExtent(0xfa, 200);
        static_cast<W8MessageDialogBase*>(m_dialog)
            ->SetMessage(text, 1, 0x32, 1, confirmation, 1, 1, 0, 0x15e);
        ActivateDialogRegion(0x138);
        m_capture_dialog_result = 1;
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
            m_block_advance = 1;
            AdvancePage(0);
            break;
        case 4:
            m_confirm_profession = 1;
            AdvancePage(0);
            break;
        case 5: {
            int value = ComputeStartingEquipmentCost(&m_character);
            int next_response;
            wchar_t* format;
            if (CanAffordStartingEquipment(&m_character)) {
                format = gppStringList[0xd7];
                next_response = 6;
            } else {
                format = gppStringList[0xd8];
                next_response = 7;
            }
            ShowMessage(FormatWideString(format, value), 1, next_response);
            break;
        }
        case 6:
            FinalizeCreatedCharacter(&m_character, &m_creation_state, 0);
            RecruitCharacterIntoParty(m_original, &m_character, 1);
            RequestScreenTransition();
            break;
        case 7:
            FinalizeCreatedCharacter(&m_character, &m_creation_state, 0);
            RecruitCharacterIntoParty(m_original, &m_character, 0);
            RequestScreenTransition();
            break;
        }
    } else if (response == 6) {
        FinalizeCreatedCharacter(&m_character, &m_creation_state, 0);
        RecruitCharacterIntoParty(m_original, &m_character, 0);
        RequestScreenTransition();
    }
}

// FUNCTION: WIZ8 0x005b1670
bool W8CharacterScreen::ValidateName()
{
    if (m_original != 0 && wcscmp(m_original->name, m_character.name) == 0) {
        return true;
    }
    if (!g_status.game_started && !g_status.skip_loose_character_check) {
        char path[260];
        BuildCharacterPath(path, m_character.name, -1);
        if (FileExists(path)) {
            ShowMessage(gppStringList[0xd5], 0, 0);
            return false;
        }
    }
    for (int index = 0; index < W8_PARTY_SLOT_COUNT; ++index) {
        if (g_status.buffers.XChar[index].fOccupied &&
            wcscmp(g_status.buffers.Char[index].name, m_character.name) == 0) {
            ShowMessage(gppStringList[0xd5], 0, 0);
            return false;
        }
    }
    return true;
}

// FUNCTION: WIZ8 0x005b0120
unsigned char W8CharacterScreen::HasDialog()
{
    return m_dialog != 0;
}

// FUNCTION: WIZ8 0x005b0130
W8Character* W8CharacterScreen::GetOriginalCharacter()
{
    return m_original;
}

/* Retail's shared success return, also used by the screen lifecycle table. */
// FUNCTION: WIZ8 0x005b1740
unsigned char ScreenLifecycleSuccess(void)
{
    return 1;
}

// FUNCTION: WIZ8 0x005b1750
unsigned char CharacterScreenEnter(void)
{
    SetViewport(0, 0, 0x280, 0x1e0);
    SetPrimarySurfaceTextureHint2Enabled(0);
    MSYS_Init();
    ResetRegions();
    UpdateHeldItemCursor();
    SetFontObjectPalette16BPP(g_wiz_text_font_secondary, g_wiz_text_font_secondary_palette);
    SetFontObjectPalette16BPP(g_wiz_text_bold_font, g_font_palette_wiz_text_bold);
    g_character_screen = new W8CharacterScreen(
        g_current_screen_state.mode, static_cast<W8Character*>(g_current_screen_state.parameter_3));
    g_character_screen->BuildControls();
    if (!g_status.game_started &&
        (g_current_screen_state.mode == 0 || g_current_screen_state.mode == 2)) {
        StartMusicResource("Menus.MPL", 1, 1);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005b1840
unsigned char CharacterScreenLeave(int leaving)
{
    if (leaving) {
        W8CharacterScreen* screen = g_character_screen;
        if (screen != 0) {
            screen->m_pages[screen->m_page_index]->Deactivate();
            for (int index = 0; index < 4; ++index) {
                delete screen->m_pages[index];
            }
            screen->m_controls->DestroyAllControls();
            delete screen->m_controls;
            delete screen;
        }
        g_character_screen = 0;
    }
    NoOp();
    MSYS_Shutdown();
    ResetRegions();
    return 1;
}

// FUNCTION: WIZ8 0x005b18e0
void CharacterScreenFrame(void)
{
    POINT point;
    InputAtom input;
    SGPMouseGetPos(&point);
    g_character_screen->UpdateDialog();
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, static_cast<unsigned short>(point.x),
                                static_cast<unsigned short>(point.y), gfLeftButtonState,
                                gfRightButtonState);
    UpdateRegionMousePosition(point.x, point.y);
    while (DequeueEvent(&input) == 1) {
        W8CharacterScreen* screen = g_character_screen;
        screen->m_pages[screen->m_page_index]->HandleInput(&input);
        if (!DispatchRegionInput(&input) && input.usEvent == KEY_DOWN) {
            if (input.usParam == 0xd || input.usParam == 0x27 || input.usParam == 0x4e) {
                screen->AdvancePage(1);
            } else if (input.usParam == 0x1b) {
                if (screen->m_mode == 1 && !screen->m_exit->m_enabled) {
                    RequestScreenTransition();
                } else {
                    screen->ShowMessage(gppStringList[0xd3], 1, 2);
                }
            } else if ((input.usParam == 0x25 || input.usParam == 0x42) &&
                       screen->m_page_index != 3) {
                int index = screen->m_page_index - 1;
                while (index >= 0 && !screen->m_page_enabled[index])
                    --index;
                if (index != -1)
                    screen->SelectPage(index);
            }
        }
    }
    W8CharacterScreen* screen = g_character_screen;
    if (screen->m_header_dirty)
        screen->DrawHeader();
    screen->m_pages[screen->m_page_index]->Redraw();
    screen->m_controls->Redraw();
    if (screen->m_dialog != 0)
        screen->m_dialog->Draw();
    RenderFrame();
}

// FUNCTION: WIZ8 0x005b1ad0
void RefreshCharacterScreenPartySlot(unsigned int)
{
    if (g_character_screen->m_dialog != 0 && g_character_screen->m_dialog_response == 1) {
        static_cast<W8CharacterSummaryDialog*>(g_character_screen->m_dialog)
            ->DrawPortraitAnimationFrame();
    }
}
