#include "soundman.h"
#include "wiz8/magic.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/render_state.h"
#include "wiz8/targeting.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/screen_state.h"
#include "wiz8/fonts.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/cursor.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/ModalDialogBase.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/music_playlist.h"
#include "wiz8/sound_man.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"
#include "Font.h"
#include "english.h"
#include "input.h"
#include "Types.h"
#include "mousesystem.h"
#include "random.h"
#include "timer.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* Local Screens\ReviewCharacterScreen.cpp, named by entry's
   fFoundEquipChar assertion. This is state 6, reached both from the party
   selector and from the running game. */

// GLOBAL: WIZ8 0x0069c0f4
W8CampScreenState0069C0F4* g_camp_screen_0069c0f4;
// GLOBAL: WIZ8 0x0064cbe8
int g_rcs_mode_0064cbe8 = -1;
// GLOBAL: WIZ8 0x0069c0f8
W8Character* g_value_0069c0f8;
// GLOBAL: WIZ8 0x0069c0fc
int g_camp_entry_parameter_0069c0fc;
// GLOBAL: WIZ8 0x0069c100
W8Character* g_camp_character_0069c100;
// GLOBAL: WIZ8 0x0069c104
unsigned char g_camp_character_pending_0069c104;
// GLOBAL: WIZ8 0x0069c108
unsigned int g_camp_item_region_set_0069c108;
// GLOBAL: WIZ8 0x0069c40c
unsigned int g_camp_spell_region_sets_0069c40c[6];
// GLOBAL: WIZ8 0x0069c51c
unsigned int g_camp_skill_region_set_0069c51c;
// GLOBAL: WIZ8 0x0069c520
unsigned int g_camp_skill_controls_region_set;
// GLOBAL: WIZ8 0x0069c408
unsigned int g_camp_character_info_region_set;

// GLOBAL: WIZ8 0x00648c8c
int g_camp_spell_animations[6][3] = {
    {0, 486, 22}, {3, 487, 18}, {8, 488, 14},
    {8, 489, 21}, {1, 490, 16}, {14, 491, 24}
};

extern unsigned char g_flag_689b32;

// GLOBAL: WIZ8 0x005ee6ec
int g_effect_005ee6ec = 109;

// GLOBAL: WIZ8 0x005ed8cc
int g_effect_argument_005ed8cc = 1;

void Function5187E0(void);
#include "line.h"
void Function5B4EB0(void);
void Function5B9070(void);
void Function5B9350(void);
void Function5B9900(void);
void Function5A45B0(void);
void Function5A4770(void);
void Function5B55F0(void);
void Function4EF1F0(void);
void Function5B6B30(unsigned int slot);
void Function5B59B0(int page);
void Function52DDD0(void);
void Function5A42A0(void);
void Function5C5240(void);
// GLOBAL: WIZ8 0x0069c428
Controls* g_camp_secondary_panel_0069c428;

// The review-screen panel sets torn down on leave: each set is a panel with
// its button controls beside it. Their creators are not yet recovered, so the
// members keep neutral names.
// GLOBAL: WIZ8 0x0069c43c
W8TextControl* g_panel_controls_69c43c[2];
// GLOBAL: WIZ8 0x0069c464
Controls* g_panel_69c464;
// GLOBAL: WIZ8 0x0069c468
W8TextControl* g_panel_controls_69c468[2];
// GLOBAL: WIZ8 0x0069c470
W8TextControl* g_panel_controls_69c470[7];
// GLOBAL: WIZ8 0x0069c48c
Controls* g_panel_69c48c;

// FUNCTION: WIZ8 0x005B9220
void Function5B9220(void)
{
    Controls* panel = g_panel_69c464;
    if (panel != 0) {
        panel->~Controls();
        ::operator delete(panel);
        g_panel_69c464 = 0;
    }
    W8TextControl** control = g_panel_controls_69c468;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_panel_controls_69c468 + 2);
}

// FUNCTION: WIZ8 0x005B9760
void Function5B9760(void)
{
    Controls* panel = g_panel_69c48c;
    if (panel != 0) {
        panel->~Controls();
        ::operator delete(panel);
        g_panel_69c48c = 0;
    }
    W8TextControl** control = g_panel_controls_69c470;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_panel_controls_69c470 + 7);
}

// FUNCTION: WIZ8 0x005B9EA0
void Function5B9EA0(void)
{
    Controls* panel = g_camp_secondary_panel_0069c428;
    if (panel != 0) {
        panel->~Controls();
        ::operator delete(panel);
        g_camp_secondary_panel_0069c428 = 0;
    }
    W8TextControl** control = g_panel_controls_69c43c;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_panel_controls_69c43c + 2);
}

// FUNCTION: WIZ8 0x005b9ef0
void InvalidateCampPanel005B9EF0(void)
{
    g_camp_secondary_panel_0069c428->Invalidate(0);
}

// FUNCTION: WIZ8 0x005b3150
W8CampCharacterInfo::W8CampCharacterInfo()
    : Controls(0x136, 0, 0x280, 0xa5, 0x122, 0, 0)
{
    AcquireRegionSet(&g_camp_character_info_region_set);
    m_combat_view = 1;
    m_button_058 = new W8TextControl(
        this, -1, 8, 0x8b, 0, 0, 0x121, 0, 15, 16, 17, 19, 18);
    m_button_058->m_listener = this;
    m_button_058->EnableRegionHelp(0x960);
    m_button_054 = new W8TextControl(
        this, -1, 8, 0x8b, 0, 0, 0x121, 0, 10, 11, 12, 14, 13);
    m_button_054->m_listener = this;
    m_button_054->EnableRegionHelp(0x95f);
    m_values[0] = new W8HelpTextControl(this, -1, 0xa0, 0x4c, 0xc0, 0x58);
    m_values[1] = new W8HelpTextControl(this, -1, 0x122, 0x4c, 0x142, 0x58);
    m_values[2] = new W8HelpTextControl(this, -1, 0xa0, 0x3e, 0xc0, 0x4a);
    m_values[3] = new W8HelpTextControl(this, -1, 0x122, 0x3e, 0x142, 0x4a);
}

// FUNCTION: WIZ8 0x005b33a0
void W8CampCharacterInfo::SetEnabled(unsigned char enabled)
{
    EnableRegionSet(enabled);
    Controls::SetEnabled(enabled);
    if (enabled) SetCombatView(m_combat_view);
}

// FUNCTION: WIZ8 0x005b33d0
void W8CampCharacterInfo::SetCombatView(unsigned char enabled)
{
    m_combat_view = enabled;
    m_button_058->SetActive(enabled);
    m_button_054->SetActive(enabled == 0);
    m_values[0]->SetActive(enabled);
    m_values[1]->SetActive(enabled);
    m_values[2]->SetActive(enabled);
    m_values[3]->SetActive(enabled);
    if (enabled) {
        m_renderArg_20 = 0;
    } else if (g_value_0069c0f8->armor_class_components[11] > 0) {
        m_renderArg_20 = 2;
    } else {
        m_renderArg_20 = 1;
    }
    Invalidate(0);
}

// FUNCTION: WIZ8 0x005b3470
void W8CampCharacterInfo::OnPrimary(W8TextControl* control)
{
    SetCombatView(control == m_button_058);
}

// GLOBAL: WIZ8 0x0061e798
const unsigned short g_camp_armor_class_labels[12] = {
    1052, 1053, 1054, 1055, 1056, 1057, 1058, 1059, 1060, 1061, 1062, 1063
};

// FUNCTION: WIZ8 0x005b34a0
void W8CampCharacterInfo::Redraw()
{
    bool redraw = m_fEnabled && m_fDirty;
    if (!m_combat_view &&
        ((g_value_0069c0f8->armor_class_components[11] > 0 && m_renderArg_20 != 2) ||
         (g_value_0069c0f8->armor_class_components[11] <= 0 && m_renderArg_20 == 2))) {
        SetCombatView(0);
    }
    Controls::Redraw();
    if (!redraw) return;
    InvalidateCampPanel005B9EF0();
    DrawRcsText(gppStringList[0x24d4 / 4], 0x15e, 0x84, 0x4e,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
    swprintf(g_camp_screen_0069c0f4->caption, L"%d", g_value_0069c0f8->value_09f9);
    DrawRcsText(g_camp_screen_0069c0f4->caption, 0x1ae, 0x84, 0x20,
                g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
    DrawRcsText(gppStringList[0x24d8 / 4], 0x15e, 0x92, 0x4e,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
    swprintf(g_camp_screen_0069c0f4->caption, L"%d", g_value_0069c0f8->death_count_09fd);
    DrawRcsText(g_camp_screen_0069c0f4->caption, 0x1ae, 0x92, 0x20,
                g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
    if (m_combat_view) {
        DrawRcsText(gppStringList[0x22c0 / 4], 0x15b, 10, 0x11d,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22c4 / 4], 0x15e, 0x30, 0x4e,
                    g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
        swprintf(g_camp_screen_0069c0f4->caption, L"%d", g_value_0069c0f8->initiative);
        DrawRcsText(g_camp_screen_0069c0f4->caption, 0x1ae, 0x30, 0x20,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        if (g_value_0069c0f8->equipment[6].item_id == -1 &&
            g_value_0069c0f8->equipment[7].item_id == -1) {
            DrawRcsText(gppStringList[0x22d4 / 4], 0x1d6, 0x22, 0x50,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            DrawRcsText(gppStringList[0x22d0 / 4], 0x228, 0x22, 0x50,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        } else {
            DrawRcsText(gppStringList[0x22c8 / 4], 0x1d6, 0x22, 0x50,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            DrawRcsText(gppStringList[0x22cc / 4], 0x228, 0x22, 0x50,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        }
        DrawRcsText(gppStringList[0x22d8 / 4], 0x1f8, 0x30, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22f4 / 4], 0x1f8, 0x3e, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22f0 / 4], 0x1f8, 0x4c, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22e8 / 4], 0x1f8, 0x5a, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22ec / 4], 0x1f8, 0x68, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22dc / 4], 0x1f8, 0x76, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22e0 / 4], 0x1f8, 0x84, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22e4 / 4], 0x1f8, 0x92, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        bool unknown_partner = ItemHasSingledOutGenericName(g_value_0069c0f8->equipment[6].item_id) &&
                               !g_value_0069c0f8->equipment[7].identified;
        for (unsigned int hand = 0; hand < 2; ++hand) {
            W8HandAttack* attack = &g_value_0069c0f8->hand_attacks[hand];
            if (!attack->in_play) continue;
            int x = hand ? 600 : 0x1d6;
            if (g_value_0069c0f8->equipment[hand + 6].item_id != -1 &&
                (!g_value_0069c0f8->equipment[hand + 6].identified ||
                 (hand == 0 && unknown_partner))) {
                for (int row = 0; row < 8; ++row) {
                    DrawRcsText(L"?", x, 0x30 + row * 14, 0x20,
                                g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
                }
                wcscpy(g_camp_screen_0069c0f4->caption, gppStringList[0x2570 / 4]);
                wcscat(g_camp_screen_0069c0f4->caption, L"?");
                wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x256c / 4]);
                wcscat(g_camp_screen_0069c0f4->caption, L"?");
                wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x2568 / 4]);
                wcscat(g_camp_screen_0069c0f4->caption, L"?");
                m_values[hand + 2]->SetRegionHelp(g_camp_screen_0069c0f4->caption);
                wcscpy(g_camp_screen_0069c0f4->caption, gppStringList[0x2564 / 4]);
                wcscat(g_camp_screen_0069c0f4->caption, L"?");
                wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x2568 / 4]);
                wcscat(g_camp_screen_0069c0f4->caption, L"?");
                m_values[hand]->SetRegionHelp(g_camp_screen_0069c0f4->caption);
                continue;
            }
            W8Dice dice;
            GetCharacterHandDamageDice(g_value_0069c0f8, hand, &dice);
            unsigned int damage_bonus = GetCharacterHandDamageBonus(g_value_0069c0f8, hand);
            unsigned int minimum = ((dice.base + dice.count) * (100 + damage_bonus) + 50) / 100;
            unsigned int maximum = ((dice.base + dice.count * dice.sides) * (100 + damage_bonus) + 50) / 100;
            if (minimum < 2) minimum = 1;
            if (maximum < 2) maximum = 1;
            int hit_bonus = attack->hit_bonus + g_value_0069c0f8->bonus_1771;
            int skill_bonus = (attack->attack_score < 0 ? attack->attack_score - 2 : attack->attack_score + 2) / 5;
            swprintf(g_camp_screen_0069c0f4->caption, L"%+d", attack->damage_bonus + g_value_0069c0f8->bonus_1770);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x30, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            swprintf(g_camp_screen_0069c0f4->caption, L"%d-%d", minimum, maximum);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x3e, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            wcscpy(g_camp_screen_0069c0f4->caption, gppStringList[0x2570 / 4]);
            wcscat(g_camp_screen_0069c0f4->caption, FormatWideString(L" %d, ", dice.base + dice.count));
            wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x256c / 4]);
            wcscat(g_camp_screen_0069c0f4->caption, FormatWideString(L" %d, ", dice.base + dice.count * dice.sides));
            wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x2568 / 4]);
            wcscat(g_camp_screen_0069c0f4->caption, FormatWideString(L" %+d%%", damage_bonus));
            m_values[hand + 2]->SetRegionHelp(g_camp_screen_0069c0f4->caption);
            swprintf(g_camp_screen_0069c0f4->caption, L"%d", skill_bonus + hit_bonus);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x4c, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            wcscpy(g_camp_screen_0069c0f4->caption, gppStringList[0x2564 / 4]);
            wcscat(g_camp_screen_0069c0f4->caption, FormatWideString(L" %d, ", skill_bonus));
            wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x2568 / 4]);
            wcscat(g_camp_screen_0069c0f4->caption, FormatWideString(L" %+d", hit_bonus));
            m_values[hand]->SetRegionHelp(g_camp_screen_0069c0f4->caption);
            swprintf(g_camp_screen_0069c0f4->caption, L"%d", attack->attacks);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x5a, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            swprintf(g_camp_screen_0069c0f4->caption, L"%d", attack->swings);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x68, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            swprintf(g_camp_screen_0069c0f4->caption, L"%+d", hit_bonus);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x76, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            swprintf(g_camp_screen_0069c0f4->caption, L"%+d", attack->value_25 + g_value_0069c0f8->bonus_1772);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x84, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            swprintf(g_camp_screen_0069c0f4->caption, L"%+d%%", damage_bonus);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x92, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        }
    } else {
        DrawRcsText(gppStringList[0x22f8 / 4], 0x15b, 10, 0x11d,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        for (unsigned int component = 0; component < 12; ++component) {
            if (component == 11 && g_value_0069c0f8->armor_class_components[11] == 0) continue;
            int y = (component % 6) * 14 + 0x22;
            int label_x = component / 6 ? 0x1ef : 0x15e;
            int value_x = component / 6 ? 600 : 0x1c7;
            DrawRcsText(gppStringList[g_camp_armor_class_labels[component]], label_x, y, 0x67,
                        g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
            int value = g_value_0069c0f8->armor_class_components[component];
            if (value) {
                swprintf(g_camp_screen_0069c0f4->caption, L"%+d", value);
                DrawRcsText(g_camp_screen_0069c0f4->caption, value_x, y, 0x20,
                            g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            }
        }
        DrawRcsText(gppStringList[0x22fc / 4], 0x1da, 0x84, 0x7c,
                    g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
        swprintf(g_camp_screen_0069c0f4->caption, L"%d%%", g_value_0069c0f8->damage_reduction);
        DrawRcsText(g_camp_screen_0069c0f4->caption, 600, 0x84, 0x20,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
    }
}

// FUNCTION: WIZ8 0x005c5c60
unsigned char CampSkillMouseWheel(const W8RegionEvent* event, W8Region*)
{
    PushButtonSoundScheme005587C0(0, 1);
    if (event->reason != 0x800) {
        return 0;
    }
    int delta = GetMouseWheelDeltaValue(
        reinterpret_cast<const W8RegionMouseEvent*>(event)->mouse_position);
    int step;
    for (step = 0; step < delta; ++step) {
        g_camp_screen_0069c0f4->skill_range->m_range->Decrement();
    }
    for (step = 0; step < -delta; ++step) {
        g_camp_screen_0069c0f4->skill_range->m_range->Increment();
    }
    return 1;
}

// FUNCTION: WIZ8 0x005c4540
W8CampSkillControls::W8CampSkillControls()
{
    AcquireRegionSet(&g_camp_skill_controls_region_set);
    m_buttons[0] = new W8TextControl(
        this, -1, 0x13c, 0xbe, 0, 0, 0x145, 0, 0, 1, 2, 4, 3);
    m_buttons[0]->AddLayoutFlags(g_W8TextControlMask005ED588 | g_W8TextControlMask005ED578);
    m_buttons[0]->m_listener = this;
    m_buttons[0]->EnableRegionHelp(0x954);
    m_buttons[1] = new W8TextControl(
        this, -1, 0x13c, 0xd6, 0, 0, 0x145, 0, 5, 6, 7, 9, 8);
    m_buttons[1]->AddLayoutFlags(g_W8TextControlMask005ED588 | g_W8TextControlMask005ED578);
    m_buttons[1]->m_listener = this;
    m_buttons[1]->EnableRegionHelp(0x955);
    m_buttons[2] = new W8TextControl(
        this, -1, 0x13c, 0xf3, 0, 0, 0x145, 0, 10, 15, 12, 17, 13);
    m_buttons[2]->AddLayoutFlags(g_W8TextControlMask005ED588 | g_W8TextControlMask005ED578);
    m_buttons[2]->m_listener = this;
    m_buttons[2]->EnableRegionHelp(0x956);
    if (g_camp_screen_0069c0f4->skill_flag) {
        m_buttons[2]->EnableSecondaryState(0);
    }
    unsigned int region = AddRegionToSet(g_camp_skill_controls_region_set);
    SetRegionCallback(region, CampSkillMouseWheel, 0);
    SetRegionBounds(region, 0x15d, 0xbe, 0x260, 0x1b5);
    Controls::SetEnabled(1);
}

// SYNTHETIC: WIZ8 0x005c4780
// W8CampSkillControls::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005c47a0
W8CampSkillControls::~W8CampSkillControls()
{
    DestroyAllControls();
}

// FUNCTION: WIZ8 0x005c4800
void W8CampSkillControls::OnPrimary(W8TextControl* control)
{
    if (control == m_buttons[0]) {
        if (m_buttons[0]->m_stateFlags & g_W8TextControlMask005ED570) {
            m_buttons[1]->DisableSecondaryState(0);
            g_camp_screen_0069c0f4->skill_scroll = 1;
        } else {
            g_camp_screen_0069c0f4->skill_scroll = 0;
        }
    } else if (control == m_buttons[1]) {
        if (m_buttons[1]->m_stateFlags & g_W8TextControlMask005ED570) {
            m_buttons[0]->DisableSecondaryState(0);
            g_camp_screen_0069c0f4->skill_scroll = 2;
        } else {
            g_camp_screen_0069c0f4->skill_scroll = 0;
        }
    } else {
        g_camp_screen_0069c0f4->skill_flag =
            (m_buttons[2]->m_stateFlags & g_W8TextControlMask005ED570) != 0;
    }
    Function5C5240();
    g_camp_screen_0069c0f4->redraw_flags |= 0x20000;
}

W8CampItemRange::W8CampItemRange()
{
    m_range = new W8RangeControl(
        0x263, 0xc1, 0x275, 0x1a1, &g_camp_item_region_set_0069c108);
    m_range->SetEnabled(1);
    m_range->m_listener = this;
}

// FUNCTION: WIZ8 0x005a34c0
void W8CampItemRange::OnRangeChanged(W8RangeControl*)
{
    g_camp_screen_0069c0f4->item_scroll = m_range->m_value << 1;
    if (gfKeyState[0x11]) {
        g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
        return;
    }
    g_camp_screen_0069c0f4->item_redraw_flags |= 0x7fc00000;
}

// FUNCTION: WIZ8 0x005b7090
W8CampSpellRange::W8CampSpellRange(int realm)
{
    m_realm = realm;
    int x = (realm % 3) * 0xd5;
    int y = (realm / 3) * 0x8c;
    m_range = new W8RangeControl(
        x + 0xbb, y + 0xc3, x + 0xcd, y + 0x129,
        &g_camp_spell_region_sets_0069c40c[realm]);
    m_range->m_listener = this;
    m_range->SetEnabled(1);
}

// FUNCTION: WIZ8 0x005b7160
W8CampSpellRange::~W8CampSpellRange()
{
    delete m_range;
}

// FUNCTION: WIZ8 0x005b7180
void W8CampSpellRange::OnRangeChanged(W8RangeControl*)
{
    g_camp_screen_0069c0f4->spell_scroll[m_realm] = m_range->m_value;
    g_camp_screen_0069c0f4->redraw_flags |= 0x200000 << m_realm;
}

// FUNCTION: WIZ8 0x005c4430
W8CampSkillRange::W8CampSkillRange()
{
    m_range = new W8RangeControl(
        0x264, 0xbe, 0x276, 0x1b5, &g_camp_skill_region_set_0069c51c);
    m_range->m_listener = this;
    m_range->SetEnabled(1);
}

// FUNCTION: WIZ8 0x005c44c0
W8CampSkillRange::~W8CampSkillRange()
{
    delete m_range;
}

// FUNCTION: WIZ8 0x005c44e0
void W8CampSkillRange::OnRangeChanged(W8RangeControl*)
{
    g_camp_screen_0069c0f4->skill_list_scroll = m_range->m_value;
    g_camp_screen_0069c0f4->redraw_flags |= 0x20000;
}

/* Lifecycle record 6's initializer - the camp record, which is what
   W8_SCREEN_CAMP selects. It drops the camp screen's state pointer rather than
   releasing it; the block is owned by the enter/leave pair. */
// FUNCTION: WIZ8 0x005a3500
unsigned char CampScreenInitialize(void)
{
    g_camp_screen_0069c0f4 = 0;
    CampScreenInitializeRegions();
    Function5B7230();
    return 1;
}

/* 0x0064CBF0: the twelve camp-screen regions the layout rules do not cover,
   given as explicit rectangles. The region initializer reads the first four
   fields; the trailing five are never touched there and stay positional. */
struct W8CampScreenRegion {
    int x;                 /* 0x00 */
    int y;                 /* 0x04 */
    int width;             /* 0x08 */
    int height;            /* 0x0c */
    int unknown_10;
    int unknown_14;
    int unknown_18;
    int unknown_1c;
    int unknown_20;
};

// GLOBAL: WIZ8 0x0064CBF0
const W8CampScreenRegion g_camp_screen_regions_64cbf0[12] = {
    { 0x0bc, 0x0b0, 0x2d, 0x39, 0x00, 0x01, 0x0ee, 0x0c1, 0 },
    { 0x1af, 0x0c6, 0x28, 0x28, 0x28, 0x08, 0x1aa, 0x0de, 1 },
    { 0x1af, 0x0f1, 0x28, 0x28, 0x24, 0x08, 0x1aa, 0x0f1, 1 },
    { 0x178, 0x0b0, 0x26, 0x49, 0x2c, 0x09, 0x173, 0x0b0, 1 },
    { 0x086, 0x0f1, 0x39, 0x3a, 0x04, 0x02, 0x0c4, 0x11c, 0 },
    { 0x183, 0x101, 0x23, 0x2b, 0x20, 0x07, 0x17e, 0x10b, 1 },
    { 0x09e, 0x134, 0x22, 0x65, 0x0c, 0x03, 0x0a0, 0x134, 0 },
    { 0x190, 0x134, 0x22, 0x48, 0x18, 0x06, 0x1b0, 0x134, 1 },
    { 0x079, 0x134, 0x22, 0x65, 0x08, 0x03, 0x07b, 0x154, 0 },
    { 0x1b5, 0x134, 0x22, 0x48, 0x1c, 0x06, 0x1d1, 0x154, 1 },
    { 0x0cb, 0x167, 0x22, 0x4d, 0x10, 0x04, 0x0f2, 0x18f, 0 },
    { 0x16a, 0x17a, 0x1a, 0x3a, 0x14, 0x05, 0x165, 0x19f, 1 }
};

/* The four region blocks the camp screen lays out by rule, and the twelve it
   lays out from the explicit table below. Only the leading four fields of each
   record are read here, so the remaining five stay positional. */
// FUNCTION: WIZ8 0x005a4090
void CampScreenInitializeRegions(void)
{
    unsigned int index;
    for (index = 0; index < 8; ++index) {
        unsigned short x = static_cast<unsigned short>((index & 1) * 0x30 + 6);
        unsigned short y = static_cast<unsigned short>((index >> 1) * 0x27 + 5);
        SetRegionBounds(index + 0xea, x, y, x + 0x2d, y + 0x24);
    }
    for (index = 0; index < 8; ++index) {
        unsigned short x = static_cast<unsigned short>((index & 1) * 0x31 + 0xb);
        unsigned short y = static_cast<unsigned short>((index >> 1) * 0x39 + 0xc0);
        SetRegionBounds(index + 0xf4, x, y, x + 0x2e, y + 0x36);
    }
    for (index = 0; index < 12; ++index) {
        const W8CampScreenRegion& region = g_camp_screen_regions_64cbf0[index];
        SetRegionBounds(index + 0xfc,
                        static_cast<unsigned short>(region.x),
                        static_cast<unsigned short>(region.y),
                        static_cast<unsigned short>(region.x + region.width),
                        static_cast<unsigned short>(region.y + region.height));
    }
    for (index = 0; index < 8; ++index) {
        unsigned short x = static_cast<unsigned short>((index & 1) * 0x31 + 0x200);
        unsigned short y = static_cast<unsigned short>((index >> 1) * 0x39 + 0xc0);
        SetRegionBounds(index + 0x108, x, y, x + 0x2e, y + 0x36);
    }
}

/* The camp screen's remaining six regions, three across and two down. The
   initializer calls this immediately after the block above; nothing else
   reaches it, and nothing here names what the six cells hold. */
// FUNCTION: WIZ8 0x005b7230
void Function5B7230(void)
{
    unsigned int index;
    for (index = 0; index < 6; ++index) {
        unsigned short x = static_cast<unsigned short>((index % 3) * 0xd5 + 0x1d);
        unsigned short y = static_cast<unsigned short>((index / 3) * 0x8c + 0xc3);
        SetRegionBounds(index + 0x119, x, y, x + 0x99, y + 0x65);
    }
}

// FUNCTION: WIZ8 0x005a3520
unsigned char CampScreenEnter(void)
{
    ClearPrimarySurface();
    g_value_0069c0f8 = static_cast<W8Character*>(g_current_screen_state.parameter_3);
    g_rcs_mode_0064cbe8 = g_current_screen_state.parameter_2;
    unsigned char entry_mode;
    if (g_current_screen_state.parameter_4 == 0) {
        g_camp_entry_parameter_0069c0fc = 0;
        entry_mode = 0;
    }
    else {
        g_camp_entry_parameter_0069c0fc = g_current_screen_state.parameter_4;
        entry_mode = 2;
        SoundPlay("Data\\Spells\\Sounds\\GeneralMagic.wav", 0);
        Function53A320(6);
    }
    g_flag_00685071 = 0;
    g_value_00685072 = 0;
    g_flag_00685076 = 0xff;
    g_value_00685077 = -1;
    if (!g_camp_screen_0069c0f4) {
        g_camp_screen_0069c0f4 =
            static_cast<W8CampScreenState0069C0F4*>(malloc(sizeof(W8CampScreenState0069C0F4)));
        if (!g_camp_screen_0069c0f4) {
            if (IsMessageBoxActive()) {
                Function5187E0();
            }
            BeginCombatRound();
            RequestScreenTransition();
            return 0;
        }
        memset(g_camp_screen_0069c0f4, 0, sizeof(W8CampScreenState0069C0F4));
    }
    SetClippingRegionAndImageWidth(0x500, 0, 0, 0x280, 0x1e0);
    g_camp_screen_0069c0f4->entry_mode = entry_mode;
    MSYS_Init();
    for (unsigned int realm = 0; realm < 6; ++realm) {
        g_camp_screen_0069c0f4->realm_flags[realm] = 0;
    }
    g_camp_screen_0069c0f4->item_timer_active = 0;
    g_camp_screen_0069c0f4->item_timer_expired = 0;
    Function5B4EB0();
    Function5B9070();
    Function5B9350();
    Function5B9900();
    CreateRcsLevelUpPanel();
    CreateRcsDismissPanel();
    g_camp_screen_0069c0f4->page = 0;
    g_camp_screen_0069c0f4->item_mode = 0;
    g_camp_screen_0069c0f4->item_range = new W8CampItemRange;
    for (int range_index = 0; range_index < 6; ++range_index) {
        g_camp_screen_0069c0f4->spell_ranges[range_index] = new W8CampSpellRange(range_index);
    }
    g_camp_screen_0069c0f4->skill_stack = 0;
    g_camp_screen_0069c0f4->skill_flag = 1;
    g_camp_screen_0069c0f4->skill_scroll = 0;
    g_camp_screen_0069c0f4->skill_range = new W8CampSkillRange;
    g_camp_screen_0069c0f4->skill_controls = new W8CampSkillControls;
    g_camp_screen_0069c0f4->character_info = new W8CampCharacterInfo;
    g_camp_screen_0069c0f4->flag_d50 = 0;
    Function5A45B0();
    g_camp_screen_0069c0f4->animation_timer = SetCountdownClock(50);
    for (unsigned int animation = 0; animation < 6; ++animation) {
        g_camp_screen_0069c0f4->animation_frames[animation] =
            Random(g_camp_spell_animations[animation][2]);
    }
    if (gXStatus.fCombatMode && g_combat_state->flag_a50) {
        if (g_combat_state->flag_a51 == 1) {
            for (int slot = 0; slot < 8; ++slot) {
                if (g_party_slot_rows[slot].occupied &&
                    IsPartySlotEligible00524A10(slot) &&
                    g_party_slot_rows[slot].pending_action == 9) {
                    swprintf(g_camp_screen_0069c0f4->caption, L"%s %s",
                             g_party_characters[slot].name, gppStringList[0x2464 / 4]);
                    goto show_equip_message;
                }
            }
        }
        else {
            unsigned char count = 0;
            for (int slot = 0; slot < 8; ++slot) {
                if (g_party_slot_rows[slot].occupied &&
                    IsPartySlotEligible00524A10(slot) &&
                    g_party_slot_rows[slot].pending_action == 9) {
                    ++count;
                    if (count == 1) {
                        swprintf(g_camp_screen_0069c0f4->caption, L"%s",
                                 g_party_characters[slot].name);
                    }
                    else {
                        if (count == g_combat_state->flag_a51) {
                            wcscat(g_camp_screen_0069c0f4->caption, L" ");
                            wcscat(g_camp_screen_0069c0f4->caption,
                                   FormatWideString(gppStringList[0x2460 / 4],
                                                    g_party_characters[slot].name));
                            goto show_equip_message;
                        }
                        wcscat(g_camp_screen_0069c0f4->caption, L", ");
                        wcscat(g_camp_screen_0069c0f4->caption,
                               g_party_characters[slot].name);
                    }
                }
            }
        }
        srAssertFail("fFoundEquipChar",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\ReviewCharacterScreen.cpp",
                     0x16f, 0);
show_equip_message:
        W8ModalDialogBase* dialog = static_cast<W8ModalDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(250, 200);
        dialog->SetMessage(g_camp_screen_0069c0f4->caption, 1, 50, 1, 0, 1, 1, 0, 350);
        SetDialogDestroyCallback(dialog, 0);
        g_camp_screen_0069c0f4->dialog = dialog;
        ActivateDialogRegion(0x138);
    }
    g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
    ResetTransientRenderScenes();
    SetPrimarySurfaceTextureHint2Enabled(0);
    if (!g_status_685170.game_started) {
        Function48FC10("MainMenu.MPL", 1, 1);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005a3ae0
void CampScreenFrame(void)
{
    if (g_flag_689b32) {
        RequestExitScreen();
    }
    if (IsMessageBoxActive()) {
        ProcessMessageBoxInput();
    }
    Function48F9E0();
    if (g_camp_screen_0069c0f4->dialog &&
        !ProcessDialogInput(g_camp_screen_0069c0f4->dialog)) {
        ClearActiveRegionIfMatches(0x138);
        delete g_camp_screen_0069c0f4->dialog;
        g_camp_screen_0069c0f4->dialog = 0;
        g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
    }
    if (gXStatus.unknown_026[1] && g_suspended_screen_id != W8_SCREEN_CHARACTER &&
        !gXStatus.fCombatMode && !IsScreenTransitionPending()) {
        Function4EF1F0();
    }
    POINT point;
    SGPMouseGetPos(&point);
    g_camp_screen_0069c0f4->hover_region = UpdateRegionMousePosition(point.x, point.y);
    InputAtom input;
    while (DequeueEvent(&input) == 1) {
        if (!DispatchRegionInput(&input) && input.usEvent == KEY_DOWN) {
            if (input.usParam == ESC) {
                if (!g_camp_screen_0069c0f4->entry_mode || g_camp_screen_0069c0f4->page != 0) {
                    if (!g_camp_character_pending_0069c104) {
                        if (IsMessageBoxActive()) {
                            Function5187E0();
                        }
                        BeginCombatRound();
                        RequestScreenTransition();
                    }
                    else {
                        Function5B6B30(CharacterPointerToPartySlot(g_camp_character_0069c100));
                        if (!IsPartySlotEligible00524A10(g_rcs_mode_0064cbe8)) {
                            wchar_t* text = FormatWideString(
                                gppStringList[0x24c4 / 4], g_camp_character_0069c100->name);
                            W8ModalDialogBase* dialog =
                                static_cast<W8ModalDialogBase*>(CreateDialogByKind(1));
                            dialog->SetClientExtent(250, 200);
                            dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
                            SetDialogDestroyCallback(dialog, 0);
                            g_camp_screen_0069c0f4->dialog = dialog;
                            ActivateDialogRegion(0x138);
                        }
                        else {
                            Function52E690(g_camp_character_0069c100, g_effect_005ee6ec, 0,
                                          g_effect_argument_005ed8cc, g_effect_argument_005ed914);
                        }
                    }
                }
                else {
                    Function5B59B0(0);
                }
            }
            else if (input.usParam == 'P') {
                if (g_status_685170.game_started) {
                    SortPartyItemPool();
                }
            }
            else if (input.usParam == 'X' && gfKeyState[0x12] &&
                     !gfKeyState[0x11] && !gfKeyState[0x10]) {
                W8ModalDialogBase* dialog =
                    static_cast<W8ModalDialogBase*>(CreateDialogByKind(1));
                dialog->SetClientExtent(250, 200);
                dialog->SetMessage(gppStringList[0x20c8 / 4], 1, 50, 1, 1, 1, 1, 0, 350);
                SetDialogDestroyCallback(dialog, OnQuitGameDialogClosed);
                g_camp_screen_0069c0f4->dialog = dialog;
                ActivateDialogRegion(0x138);
            }
        }
    }
    if (g_camp_screen_0069c0f4->page == 3 &&
        !ClockIsTicking(g_camp_screen_0069c0f4->animation_timer)) {
        for (unsigned int realm = 0; realm < 6; ++realm) {
            ++g_camp_screen_0069c0f4->animation_frames[realm];
            if (g_camp_screen_0069c0f4->animation_frames[realm] ==
                g_camp_spell_animations[realm][2]) {
                g_camp_screen_0069c0f4->animation_frames[realm] = 0;
            }
        }
        g_camp_screen_0069c0f4->animation_timer = SetCountdownClock(50);
        g_camp_screen_0069c0f4->redraw_flags |= 0x10000;
    }
    if (g_camp_screen_0069c0f4->page == 0 &&
        g_camp_screen_0069c0f4->item_timer_active &&
        !g_camp_screen_0069c0f4->item_timer_expired &&
        !ClockIsTicking(g_camp_screen_0069c0f4->item_timer)) {
        g_camp_screen_0069c0f4->item_timer_expired = 1;
        g_camp_screen_0069c0f4->item_redraw_flags |= 0x3ffe00;
    }
    if (!g_camp_screen_0069c0f4->input_mode) {
        Function52DDD0();
        Function52E750();
    }
    Function5A42A0();
}

// FUNCTION: WIZ8 0x005a3ee0
unsigned char CampScreenLeave(int)
{
    Function5A4770();
    SetFontObjectPalette16BPP(g_smfnt_font_683694, g_font_palette_smfnt_68ee10);
    SetFontObjectPalette16BPP(g_calligraphy_font_6835f8, g_font_palette_calligraphy_68edfc);
    SetFontObjectPalette16BPP(g_calligraphy_shadow_font_6835f4,
                            g_font_palette_calligraphy_shadow_68ee18);
    SetFontObjectPalette16BPP(g_wiz_text_font_683640, g_font_palette_wiz_text_68ee14);
    Function5B55F0();
    Function5B9220();
    Function5B9760();
    Function5B9EA0();
    DestroyRcsLevelUpPanel();
    DestroyRcsDismissPanel();
    delete g_camp_screen_0069c0f4->item_range;
    if (g_camp_screen_0069c0f4->dialog) {
        ClearActiveRegionIfMatches(0x138);
        delete g_camp_screen_0069c0f4->dialog;
    }
    for (unsigned int realm = 0; realm < 6; ++realm) {
        delete g_camp_screen_0069c0f4->spell_ranges[realm];
    }
    delete g_camp_screen_0069c0f4->skill_range;
    delete g_camp_screen_0069c0f4->skill_controls;
    delete g_camp_screen_0069c0f4->character_info;
    free(g_camp_screen_0069c0f4);
    g_camp_screen_0069c0f4 = 0;
    MSYS_Shutdown();
    ResetRegions();
    if (gXStatus.fCampMode) {
        gXStatus.unknown_026[0] = 1;
    }
    return 1;
}
