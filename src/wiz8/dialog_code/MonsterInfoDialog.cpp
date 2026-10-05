#include "wiz8/dialog_code/MonsterInfoDialog.h"
#include "wiz8/dialog_code/SpellInfoDialog.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/stCube.h"
#include "wiz8/engine_code/stScript.h"
#include "wiz8/float_constants.h"
#include "wiz8/fonts.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/regions.h"
#include "wiz8/startup_world.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "Font.h"

#include <string.h>
#include <wchar.h>

// STRING: WIZ8 0x0064F6B0
#define MONSTER_INFO_DIALOG_CPP "C:\\Projects\\Wizardry 8\\Dialog Code\\MonsterInfoDialog.cpp"

/* The background and scroll track the monster and statistic info dialogs
   load, read through this pointer. */
// GLOBAL: WIZ8 0x0064f610
// STRING: WIZ8 0x0064f660
char* g_info_dialog_background = "Data\\Dialogs\\popup_monsterinfo.sti";

// FUNCTION: WIZ8 0x005d5e30
W8MonsterInfoDialog::W8MonsterInfoDialog(int location_id) : m_location_id(location_id)
{
    SetOrigin(0x9c, 0x31);
    SetExtent(0x14a, 0x10e);
    SetBackground(g_info_dialog_background, 0);
}

// FUNCTION: WIZ8 0x005d5f00
W8MonsterInfoDialog::~W8MonsterInfoDialog()
{
    W8MonsterInfoDialog::DestroyControls();
}

// FUNCTION: WIZ8 0x005d5f90
int W8MonsterInfoDialog::CreateControls()
{
    W8DialogBase::CreateControls();
    if (PopulateText() == 0) {
        m_error = 7;
        return 7;
    }

    W8DialogScrollBar::Resources resources;
    resources.arrows_path = "Data\\Main Interface\\main_scroll.sti";
    resources.track_path = g_info_dialog_background;
    resources.track_frame = 1;
    resources.on_scroll = ScrollCallback;
    m_scroll_bar.CreateControls(&resources);
    int x = m_x;
    m_scroll_bar.SetLayout(x + 0x12b, m_y + 0x26, m_text_area.GetTotalLineCount(), 0,
                              m_text_area.GetLineHeight(), 0xb9);
    m_scroll_bar.m_owner = this;

    m_button.Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 0, 1, 4, 2,
                          DialogCloseButtonCallback, 0, 0, 0x7f, -1, 0, 0);
    m_button.SetPosition(m_x + 0x11a, m_y + 0xe6);
    m_button.m_owner = this;
    return 0;
}

/* 0x0064F614: gppStringList indices naming the seven classes the monster's
   name line falls into by how far its level sits above the party average. */
// GLOBAL: WIZ8 0x0064f614
static int g_monster_level_name_ids[7] = {301, 302, 303, 304, 305, 306, 307};

/* 0x0064F630: knowledge threshold and gppStringList label id pairs gating the
   record's six resistance entries. */
// GLOBAL: WIZ8 0x0064f630
static int g_monster_resistance_label_gates[6][2] = {
    {70, 308}, {60, 309}, {40, 310}, {50, 311}, {80, 312}, {90, 313},
};

// FUNCTION: WIZ8 0x005d6160
unsigned char W8MonsterInfoDialog::PopulateText()
{
    W8ControlsRect bounds;
    wchar_t text[2000];
    bool is_npc = false;
    unsigned int knowledge;
    int leader_location_id;
    int leader_group_id;
    int count;
    int index;
    int slot;
    bool has_unknown;
    bool any_shown;
    unsigned char max_values[0x10];
    int range_category;
    const wchar_t* entry_text;
    const wchar_t* prefix;
    float combat_range;
    float average_level;
    int monster_level;
    int best_party_slot;
    int sight;
    W8MonsterGroup* group;
    W8MonsterInfo* leader_info;
    W8Navigator* linked;
    stScript* script;
    W8NpcState* npc;
    W8MonsterAttack* attack;
    W8ConditionImmunity* immunity;
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;

    bounds.left = m_x + 0x11;
    bounds.right = m_x + 0x11f;
    bounds.top = m_y + 0x26;
    bounds.bottom = m_y + 0xdf;

    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0xa0, MONSTER_INFO_DIALOG_CPP, m_location_id, 1));
    record = GetMonsterDataForInfo(monster_info);
    average_level = GetAveragePartyMemberLevel();
    monster_level = record->display_level;
    if (monster_info->ubDisposition != W8_DISPOSITION_HOSTILE &&
        (record->flags & W8_MONSTER_FLAG_NPC) != 0 &&
        (npc = GetNpcStateByKind(record->npc_kind)) != 0 && npc->record->has_group != 0) {
        is_npc = true;
    }
    if (monster_info->summoned == 1) {
        knowledge = 0x7d;
    } else {
        knowledge = GetBestPartySkillLevel(W8_SKILL_MYTHOLOGY, &best_party_slot);
        if (static_cast<int>(average_level) < monster_level) {
            float adjusted_knowledge =
                knowledge - (monster_level - average_level) * g_float_005ec52c + g_float_005ebc7c;
            if (adjusted_knowledge < g_float_zero) {
                adjusted_knowledge = g_float_zero;
            }
            knowledge = static_cast<unsigned int>(adjusted_knowledge);
        }
    }
    combat_range = monster_info->p3D->GetDistanceToPlayer();
    m_text_area.Configure(&bounds, g_wiz_text_font_secondary, 0);
    m_text_area.SetEntrySpacing(1);

    if (g_dev_mode) {
        group = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
            0xc8, MONSTER_INFO_DIALOG_CPP, monster_info->monster_group_id, 1));
        linked = monster_info->p3D->linked_navigator;
        if (linked == 0) {
            leader_location_id = m_location_id;
        } else {
            leader_location_id = linked->movement.location_id;
        }
        leader_group_id = group->leader_group_id;
        if (leader_group_id == 0) {
            leader_group_id = monster_info->monster_group_id;
        }
        m_text_area.AddEntry(L"ID/Group (Leader)",
                             FormatWideString(L"%d/%d (%d/%d) %s", m_location_id,
                                              monster_info->monster_group_id, leader_location_id,
                                              leader_group_id,
                                              group->encounter_registered ? L"random" : L"placed"),
                             5, 0xf, 0);
    }

    if (is_npc) {
        prefix = gppStringList[0x13b];
        entry_text = gppStringList[0x13a];
    } else {
        int level_class =
            static_cast<int>(monster_level / average_level * g_float_005ebc28 - g_float_005ec3b8);
        ClampInteger(&level_class, 0, 6);
        entry_text = FormatWideString(L"%d (%s)", monster_level,
                                      gppStringList[g_monster_level_name_ids[level_class]]);
        prefix = gppStringList[0x13b];
    }
    m_text_area.AddEntry(prefix, entry_text, 10, 0xf, 0);

    if (g_status.status_ints[monster_info->monster_species] == 2) {
        FormatUnsignedIntegerWithCommas(text, GetMonsterExperience(record));
    } else {
        wcscpy(text, gppStringList[0x13a]);
    }
    m_text_area.AddEntry(gppStringList[0x13c], text, 10, 0xf, 0);

    if (knowledge < 10 || is_npc) {
        wcscpy(text, gppStringList[0x13a]);
    } else {
        wcscpy(text, FormatWideString(g_format_d, monster_info->hp_current));
    }
    wcscat(text, L" / ");
    if (knowledge < 5 || is_npc) {
        wcscpy(text, gppStringList[0x13a]);
    } else {
        wcscat(text, FormatWideString(g_format_d, monster_info->uiHPMax));
    }
    m_text_area.AddEntry(gppStringList[0x13d], text, 10, 0xf, 0);
    if (g_dev_mode && knowledge < 10) {
        wcscpy(text, FormatWideString(g_journal_page_format, monster_info->hp_current,
                                      monster_info->uiHPMax));
        m_text_area.AddEntry(gppStringList[0x13d], text, 5, 0xf, 0);
    }

    if (knowledge < 0x14 || is_npc) {
        wcscpy(text, gppStringList[0x13a]);
    } else {
        wcscpy(text, FormatWideString(g_format_d, monster_info->stamina));
    }
    wcscat(text, L" / ");
    if (knowledge < 0xf || is_npc) {
        wcscpy(text, gppStringList[0x13a]);
    } else {
        wcscat(text, FormatWideString(g_format_d, monster_info->stamina_max));
    }
    m_text_area.AddEntry(gppStringList[0x13e], text, 10, 0xf, 0);
    if (g_dev_mode && knowledge < 0x14) {
        wcscpy(text, FormatWideString(g_journal_page_format, monster_info->stamina,
                                      monster_info->stamina_max));
        m_text_area.AddEntry(gppStringList[0x13e], text, 5, 0xf, 0);
    }

    text[0] = L'\0';
    count = 0;
    for (index = 0; index < W8_CONDITION_COUNT; ++index) {
        if (monster_info->uiCondition[index] != 0) {
            if (count > 0) {
                wcscat(text, g_comma_space);
            }
            wcscat(text, gppStringList[g_condition_notices[index * 4]]);
            ++count;
        }
    }
    if (monster_info->fInCombat) {
        for (index = 0; index < 9; ++index) {
            if (monster_info->pCombat->combat_effects[index].active) {
                if (count > 0) {
                    wcscat(text, g_comma_space);
                }
                wcscat(text,
                       g_spell_records[monster_info->pCombat->combat_effects[index].effect_id]
                           .display_name);
                ++count;
            }
        }
    }
    if (monster_info->control_state == 1) {
        if (count > 0) {
            wcscat(text, g_comma_space);
        }
        wcscat(text, gppStringList[0x144]);
        ++count;
    }
    if (monster_info->charm_strength != 0) {
        if (count > 0) {
            wcscat(text, g_comma_space);
        }
        wcscat(text, gppStringList[0x145]);
        ++count;
    }
    if (monster_info->summoned != 0) {
        if (count > 0) {
            wcscat(text, g_comma_space);
        }
        wcscat(text, gppStringList[0x146]);
        ++count;
    }
    if (count > 0) {
        m_text_area.AddEntry(gppStringList[0x147], text, 10, 0xf, 0);
    }

    text[0] = L'\0';
    count = 0;
    for (index = 0; index < 8; ++index) {
        if (monster_info->enchantments[index].turns != 0) {
            if (count > 0) {
                wcscat(text, g_comma_space);
            }
            wcscat(text, gppStringList[g_condition_notices[100 + index]]);
            ++count;
        }
    }
    for (index = 0; index < 12; ++index) {
        if (monster_info->effect_slots[index].active) {
            if (count > 0) {
                wcscat(text, g_comma_space);
            }
            wcscat(text,
                   g_spell_records[monster_info->effect_slots[index].effect_id].display_name);
            ++count;
        }
    }
    if (monster_info->fInCombat) {
        for (index = 0; index < 6; ++index) {
            if (monster_info->pCombat->combat_effects_2[index].active) {
                if (count > 0) {
                    wcscat(text, g_comma_space);
                }
                wcscat(text,
                       g_spell_records[monster_info->pCombat->combat_effects_2[index].effect_id]
                           .display_name);
                ++count;
            }
        }
    }
    if (count > 0) {
        m_text_area.AddEntry(gppStringList[0x148], text, 10, 0xf, 0);
    }

    range_category = W8_RANGE_TOUCH;
    do {
        if (combat_range <= CalcRangeDistance(static_cast<W8RangeCategory>(range_category))) {
            wcscpy(text, gppStringList[g_spell_range_name_ids[range_category]]);
            break;
        }
        ++range_category;
    } while (range_category < 4);
    if (range_category == 4) {
        wcscpy(text, gppStringList[0x140]);
    }
    m_text_area.AddEntry(gppStringList[0x13f], text, 10, 0xf, 0);

    if (0x13 < knowledge) {
        W8RangeCategory best_range = GetMonsterBestRangeCategory(monster_info, 1, &sight);
        if (best_range != W8_RANGE_NONE) {
            m_text_area.AddEntry(gppStringList[0x141],
                                    gppStringList[g_spell_range_name_ids[best_range]], 10, 0xf, 0);
        }
    }
    if (0x31 < knowledge && record->special_attack_kind != 0) {
        m_text_area.AddEntry(
            gppStringList[0x142],
            gppStringList[g_monster_special_attack_name_ids[record->special_attack_kind]], 10,
            0xf, 0);
    }

    if (9 < knowledge) {
        memset(max_values, 0, sizeof(max_values));
        has_unknown = false;
        any_shown = false;
        for (index = 0; index < 3; ++index) {
            attack = &record->attacks[index];
            if (attack->fHasAttack != 0) {
                for (slot = 0; slot < 0x10; ++slot) {
                    if (max_values[slot] < attack->missile_values[slot]) {
                        max_values[slot] = attack->missile_values[slot];
                    }
                }
            }
        }
        text[0] = L'\0';
        count = 0;
        for (index = 0; index < 0x10; ++index) {
            unsigned char value = max_values[index];
            if (value != 0) {
                if (value < 0x4b && (knowledge < 0x1e || value < 0x32) &&
                    (knowledge < 0x3c || value < 0x19) && (knowledge < 0x50 || value < 10) &&
                    (knowledge < 0x5a || value < 5) && knowledge < 100) {
                    has_unknown = true;
                } else {
                    if (count > 0) {
                        wcscat(text, g_comma_space);
                    }
                    wcscat(text, gppStringList[g_attack_effect_name_ids[index]]);
                    ++count;
                    any_shown = true;
                }
            }
        }
        if (any_shown && has_unknown) {
            if (count > 0) {
                wcscat(text, g_comma_space);
            }
            wcscat(text, L"???");
            ++count;
        }
        if (count > 0) {
            m_text_area.AddEntry(gppStringList[0x143], text, 10, 0xf, 0);
        }
    }

    if (0x1d < knowledge) {
        text[0] = L'\0';
        count = 0;
        for (index = 0; index < 3; ++index) {
            immunity = &g_condition_immunities[index];
            if (record->kind == immunity->kind) {
                for (slot = 0; slot < 0x14 && immunity->conditions[slot] != 0; ++slot) {
                    if (count > 0) {
                        wcscat(text, g_comma_space);
                    }
                    wcscat(text,
                           gppStringList[g_condition_notices[immunity->conditions[slot] * 4 + 3]]);
                    ++count;
                }
                if (immunity->immune_all != 0) {
                    if (count > 0) {
                        wcscat(text, g_comma_space);
                    }
                    wcscat(text, gppStringList[0x14b]);
                    ++count;
                }
                if (count > 0) {
                    m_text_area.AddEntry(gppStringList[0x14a], text, 10, 0xf, 0);
                }
                break;
            }
        }
    }

    for (index = 0; index < 6; ++index) {
        if (g_monster_resistance_label_gates[index][0] <= static_cast<int>(knowledge)) {
            m_text_area.AddEntry(gppStringList[g_monster_resistance_label_gates[index][1]],
                                    FormatWideString(g_format_d, record->resistances[index]), 10,
                                    0xf, 0);
        }
    }

    if (g_dev_mode) {
        m_text_area.AddEntry(
            L"Range (Combat/Ground)",
            FormatWideString(
                L"%5.2f / %5.2f M", combat_range * g_float_005ebc60,
                (monster_info->p3D->GetPosition() - g_startup_world->GetPosition()).Length() *
                    g_world_cursor_scale),
            5, 0xf, 0);
        leader_info = MonsterInfoFromID(0x1d6, MONSTER_INFO_DIALOG_CPP, leader_location_id, 1);
        script = leader_info->p3D->script;
        m_text_area.AddEntry(L"Leader's Current Script",
                                FormatWideString(L"<%S>", script != 0 ? script->getName() : 0), 5,
                                0xf, 0);
        m_text_area.AddEntry(L"Leader's AI Mode",
                                FormatWideString(g_format_d, leader_info->ai_mode), 5, 0xf, 0);
        const wchar_t* strategy = L"Close";
        if (record->prefer_ranged_actions != 0) {
            strategy = L"Ranged";
        }
        m_text_area.AddEntry(L"Combat Strategy", strategy, 5, 0xf, 0);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005d6e60
void W8MonsterInfoDialog::OnRightButtonUp()
{
    if (m_right_button_down) {
        m_keep_open = false;
    }
}

// FUNCTION: WIZ8 0x005d6e70
void W8MonsterInfoDialog::OnMouseWheel(int delta)
{
    m_scroll_bar.ScrollBy(delta);
}

// FUNCTION: WIZ8 0x005d6ec0
void W8MonsterInfoDialog::ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry)
{
    int left;
    int top;
    int right;
    int bottom;
    W8MonsterInfoDialog* dialog = static_cast<W8MonsterInfoDialog*>(scroll_bar->m_owner);
    if (dialog != 0) {
        dialog->m_text_area.SetFirstVisibleLine(first_visible_entry);
        left = dialog->m_x + 0x11;
        top = dialog->m_y + 0x26;
        right = left + 0x10e;
        bottom = top + 0xb9;
        InvalidateRegion(left, top, right, bottom, 0);
        BlitCatalogSurfaceRectTo16BPP(-0xe, left, top, right, bottom, 0x1b6, 0, 0);
        dialog->m_text_area.m_dirty = true;
    }
}

// FUNCTION: WIZ8 0x005dbde0
void W8MonsterInfoDialog::DestroyControls()
{
    m_scroll_bar.DestroyControls();
    W8DialogBase::DestroyControls();
}

// FUNCTION: WIZ8 0x005d6080
void W8MonsterInfoDialog::Draw()
{
    if ((m_dirty_flags & 1) != 0) {
        if (!m_initialized) {
            CreateControls();
        }
        m_text_area.m_dirty = true;
        m_scroll_bar.m_dirty = true;
        m_button.m_dirty = true;
        W8DialogBase::Draw();
        SetFont(g_wiz_text_font_secondary);
        SetFontObjectPalette16BPP(g_wiz_text_font_secondary, g_wiz_text_font_secondary_palette);
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0x1f1, MONSTER_INFO_DIALOG_CPP, m_location_id, 1);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        wchar_t* name = GetMonsterName(monster_info, 0, 0);
        INT16 width = StringPixLength(name, g_wiz_text_font_secondary);
        gprintf(m_x + 0xe + (0x112 - width) / 2, m_y + 0x11, g_format_s, name);
    }
    m_text_area.Draw(0);
    m_scroll_bar.Draw(0);
    m_button.Draw();
}
