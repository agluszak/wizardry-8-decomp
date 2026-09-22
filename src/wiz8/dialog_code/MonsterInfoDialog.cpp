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
#include "wiz8/regions.h"
#include "wiz8/startup_world.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "Font.h"

#include <string.h>
#include <wchar.h>

static const char MONSTER_INFO_DIALOG_CPP[] =
    "C:\\Projects\\Wizardry 8\\Dialog Code\\MonsterInfoDialog.cpp";

// SYNTHETIC: WIZ8 0x005d5ee0
// W8MonsterInfoDialog::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005d5e30
W8MonsterInfoDialog::W8MonsterInfoDialog(int location_id) : m_location_id(location_id)
{
    SetOrigin(0x9c, 0x31);
    SetExtent(0x14a, 0x10e);
    SetBackground("Data\\Dialogs\\popup_monsterinfo.sti", 0);
}

// FUNCTION: WIZ8 0x005d5f00
W8MonsterInfoDialog::~W8MonsterInfoDialog()
{
    m_scroll_bar_58.DestroyControls();
    W8DialogBase::DestroyControls();
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
    resources.track_path = "Data\\Dialogs\\popup_monsterinfo.sti";
    resources.track_frame = 1;
    resources.on_scroll = ScrollCallback;
    m_scroll_bar_58.CreateControls(&resources);
    int x = m_x;
    m_scroll_bar_58.SetLayout(x + 0x12b, m_y + 0x26, m_text_area_ec.GetTotalLineCount(), 0,
                              m_text_area_ec.GetLineHeight(), 0xb9);
    m_scroll_bar_58.m_owner = this;

    m_button_a4.Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 0, 1, 4, 2,
                          DialogCloseButtonCallback, 0, 0, 0x7f, -1, 0, 0);
    m_button_a4.SetPosition(m_x + 0x11a, m_y + 0xe6);
    m_button_a4.m_owner_040 = this;
    return 0;
}

/* 0x0064F614: gppStringList indices naming the seven classes the monster's
   name line falls into by how far its level sits above the party average. */
// GLOBAL: WIZ8 0x0064f614
const int g_monster_level_name_ids_64f614[7] = {301, 302, 303, 304, 305, 306, 307};

/* 0x0064F630: knowledge threshold and gppStringList label id pairs gating the
   record's six resistance entries. */
// GLOBAL: WIZ8 0x0064f630
const int g_monster_resistance_label_gates_64f630[6][2] = {
    {70, 308}, {60, 309}, {40, 310}, {50, 311}, {80, 312}, {90, 313},
};

// FUNCTION: WIZ8 0x005d6160
unsigned char W8MonsterInfoDialog::PopulateText()
{
    W8ControlsRect bounds;
    wchar_t text[2000];
    unsigned char is_npc = 0;
    unsigned int knowledge;
    int leader_location_id;
    int leader_group_id;
    int count;
    int index;
    int slot;
    unsigned char has_unknown;
    unsigned char any_shown;
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
    monster_level = record->display_level_251;
    if (monster_info->ubDisposition != 1 && (record->flags_0d0 & 1) != 0 &&
        (npc = GetNpcStateByKind(record->npc_kind_0cd)) != 0 && npc->record->has_group != 0) {
        is_npc = 1;
    }
    if (monster_info->summoned_2da == 1) {
        knowledge = 0x7d;
    } else {
        knowledge = GetBestPartySkillLevel(0x15, &best_party_slot);
        if (static_cast<int>(average_level) < monster_level) {
            float adjusted_knowledge =
                knowledge - (monster_level - average_level) * g_float_005ec52c + g_float_005ebc7c;
            if (adjusted_knowledge < g_float_005ebb34) {
                adjusted_knowledge = g_float_005ebb34;
            }
            knowledge = static_cast<unsigned int>(adjusted_knowledge);
        }
    }
    combat_range = monster_info->monster->GetDistanceToPlayer004C7CB0();
    m_text_area_ec.Configure(&bounds, g_font_683660, 0);
    m_text_area_ec.SetEntrySpacing(1);

    if (g_flag_689b32 != 0) {
        group = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
            0xc8, MONSTER_INFO_DIALOG_CPP, monster_info->monster_group_id, 1));
        linked = monster_info->monster->linked_navigator_05c;
        if (linked == 0) {
            leader_location_id = m_location_id;
        } else {
            leader_location_id = linked->movement_0c0.location_id_004;
        }
        leader_group_id = group->leader_group_id;
        if (leader_group_id == 0) {
            leader_group_id = monster_info->monster_group_id;
        }
        m_text_area_ec.AddEntry(
            L"ID/Group (Leader)",
            FormatWideString(L"%d/%d (%d/%d) %s", m_location_id, monster_info->monster_group_id,
                             leader_location_id, leader_group_id,
                             group->encounter_registered_c3 != 0 ? L"random" : L"placed"),
            5, 0xf, 0);
    }

    if (is_npc != 0) {
        prefix = gppStringList[0x13b];
        entry_text = gppStringList[0x13a];
    } else {
        int level_class =
            static_cast<int>(monster_level / average_level * g_float_005ebc28 - g_float_005ec3b8);
        ClampInteger(&level_class, 0, 6);
        entry_text = FormatWideString(L"%d (%s)", monster_level,
                                      gppStringList[g_monster_level_name_ids_64f614[level_class]]);
        prefix = gppStringList[0x13b];
    }
    m_text_area_ec.AddEntry(prefix, entry_text, 10, 0xf, 0);

    if (g_status_685170.tail_3121.facts.status_ints_3121[monster_info->monster_species] == 2) {
        FormatUnsignedIntegerWithCommas(text, GetMonsterExperience(record));
    } else {
        wcscpy(text, gppStringList[0x13a]);
    }
    m_text_area_ec.AddEntry(gppStringList[0x13c], text, 10, 0xf, 0);

    if (knowledge < 10 || is_npc != 0) {
        wcscpy(text, gppStringList[0x13a]);
    } else {
        wcscpy(text, FormatWideString(g_format_d_0060aa20, monster_info->hp_current));
    }
    wcscat(text, L" / ");
    if (knowledge < 5 || is_npc != 0) {
        wcscpy(text, gppStringList[0x13a]);
    } else {
        wcscat(text, FormatWideString(g_format_d_0060aa20, monster_info->hp_max));
    }
    m_text_area_ec.AddEntry(gppStringList[0x13d], text, 10, 0xf, 0);
    if (g_flag_689b32 != 0 && knowledge < 10) {
        wcscpy(text, FormatWideString(g_journal_page_format_0064d7f0, monster_info->hp_current,
                                      monster_info->hp_max));
        m_text_area_ec.AddEntry(gppStringList[0x13d], text, 5, 0xf, 0);
    }

    if (knowledge < 0x14 || is_npc != 0) {
        wcscpy(text, gppStringList[0x13a]);
    } else {
        wcscpy(text, FormatWideString(g_format_d_0060aa20, monster_info->stamina));
    }
    wcscat(text, L" / ");
    if (knowledge < 0xf || is_npc != 0) {
        wcscpy(text, gppStringList[0x13a]);
    } else {
        wcscat(text, FormatWideString(g_format_d_0060aa20, monster_info->stamina_max));
    }
    m_text_area_ec.AddEntry(gppStringList[0x13e], text, 10, 0xf, 0);
    if (g_flag_689b32 != 0 && knowledge < 0x14) {
        wcscpy(text, FormatWideString(g_journal_page_format_0064d7f0, monster_info->stamina,
                                      monster_info->stamina_max));
        m_text_area_ec.AddEntry(gppStringList[0x13e], text, 5, 0xf, 0);
    }

    text[0] = L'\0';
    count = 0;
    for (index = 0; index < W8_CONDITION_COUNT; ++index) {
        if (monster_info->condition_turns[index] != 0) {
            if (count > 0) {
                wcscat(text, g_comma_space_00619794);
            }
            wcscat(text, gppStringList[g_condition_notices_0061E570[index * 4]]);
            ++count;
        }
    }
    if (monster_info->fInCombat != 0) {
        for (index = 0; index < 9; ++index) {
            if (monster_info->pCombat->effect_slots_3e[index].active != 0) {
                if (count > 0) {
                    wcscat(text, g_comma_space_00619794);
                }
                wcscat(text,
                       g_spell_records[monster_info->pCombat->effect_slots_3e[index].effect_id]
                           .display_name);
                ++count;
            }
        }
    }
    if (monster_info->control_state == 1) {
        if (count > 0) {
            wcscat(text, g_comma_space_00619794);
        }
        wcscat(text, gppStringList[0x144]);
        ++count;
    }
    if (monster_info->effect_2de != 0) {
        if (count > 0) {
            wcscat(text, g_comma_space_00619794);
        }
        wcscat(text, gppStringList[0x145]);
        ++count;
    }
    if (monster_info->summoned_2da != 0) {
        if (count > 0) {
            wcscat(text, g_comma_space_00619794);
        }
        wcscat(text, gppStringList[0x146]);
        ++count;
    }
    if (count > 0) {
        m_text_area_ec.AddEntry(gppStringList[0x147], text, 10, 0xf, 0);
    }

    text[0] = L'\0';
    count = 0;
    for (index = 0; index < 8; ++index) {
        if (monster_info->enchantments[index].value_08 != 0) {
            if (count > 0) {
                wcscat(text, g_comma_space_00619794);
            }
            wcscat(text, gppStringList[g_condition_notices_0061E570[100 + index]]);
            ++count;
        }
    }
    for (index = 0; index < 12; ++index) {
        if (monster_info->effect_slots_10f[index].active != 0) {
            if (count > 0) {
                wcscat(text, g_comma_space_00619794);
            }
            wcscat(text,
                   g_spell_records[monster_info->effect_slots_10f[index].effect_id].display_name);
            ++count;
        }
    }
    if (monster_info->fInCombat != 0) {
        for (index = 0; index < 6; ++index) {
            if (monster_info->pCombat->effect_slots_d7[index].active != 0) {
                if (count > 0) {
                    wcscat(text, g_comma_space_00619794);
                }
                wcscat(text,
                       g_spell_records[monster_info->pCombat->effect_slots_d7[index].effect_id]
                           .display_name);
                ++count;
            }
        }
    }
    if (count > 0) {
        m_text_area_ec.AddEntry(gppStringList[0x148], text, 10, 0xf, 0);
    }

    range_category = W8_RANGE_TOUCH;
    do {
        if (combat_range <= CalcRangeDistance(static_cast<W8RangeCategory>(range_category))) {
            wcscpy(text, gppStringList[g_spell_range_name_ids_61e9a0[range_category]]);
            break;
        }
        ++range_category;
    } while (range_category < 4);
    if (range_category == 4) {
        wcscpy(text, gppStringList[0x140]);
    }
    m_text_area_ec.AddEntry(gppStringList[0x13f], text, 10, 0xf, 0);

    if (0x13 < knowledge) {
        W8RangeCategory best_range = GetMonsterBestRangeCategory(monster_info, 1, &sight);
        if (best_range != W8_RANGE_NONE) {
            m_text_area_ec.AddEntry(gppStringList[0x141],
                                    gppStringList[g_spell_range_name_ids_61e9a0[best_range]], 10,
                                    0xf, 0);
        }
    }
    if (0x31 < knowledge && record->special_attack_kind_0e3 != 0) {
        m_text_area_ec.AddEntry(
            gppStringList[0x142],
            gppStringList
                [g_monster_special_attack_name_ids_61ec14[record->special_attack_kind_0e3]],
            10, 0xf, 0);
    }

    if (9 < knowledge) {
        memset(max_values, 0, sizeof(max_values));
        has_unknown = 0;
        any_shown = 0;
        for (index = 0; index < 3; ++index) {
            attack = &record->attacks[index];
            if (attack->fHasAttack != 0) {
                for (slot = 0; slot < 0x10; ++slot) {
                    if (max_values[slot] < attack->missile_values_05[slot]) {
                        max_values[slot] = attack->missile_values_05[slot];
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
                    has_unknown = 1;
                } else {
                    if (count > 0) {
                        wcscat(text, g_comma_space_00619794);
                    }
                    wcscat(text, gppStringList[g_damage_type_name_ids_61e9cc[index]]);
                    ++count;
                    any_shown = 1;
                }
            }
        }
        if (any_shown != 0 && has_unknown != 0) {
            if (count > 0) {
                wcscat(text, g_comma_space_00619794);
            }
            wcscat(text, L"???");
            ++count;
        }
        if (count > 0) {
            m_text_area_ec.AddEntry(gppStringList[0x143], text, 10, 0xf, 0);
        }
    }

    if (0x1d < knowledge) {
        text[0] = L'\0';
        count = 0;
        for (index = 0; index < 3; ++index) {
            immunity = &g_condition_immunities_006171A8[index];
            if (record->kind_0cb == immunity->kind) {
                for (slot = 0; slot < 0x14 && immunity->conditions[slot] != 0; ++slot) {
                    if (count > 0) {
                        wcscat(text, g_comma_space_00619794);
                    }
                    wcscat(text,
                           gppStringList
                               [g_condition_notices_0061E570[immunity->conditions[slot] * 4 + 3]]);
                    ++count;
                }
                if (immunity->unknown_01 != 0) {
                    if (count > 0) {
                        wcscat(text, g_comma_space_00619794);
                    }
                    wcscat(text, gppStringList[0x14b]);
                    ++count;
                }
                if (count > 0) {
                    m_text_area_ec.AddEntry(gppStringList[0x14a], text, 10, 0xf, 0);
                }
                break;
            }
        }
    }

    for (index = 0; index < 6; ++index) {
        if (g_monster_resistance_label_gates_64f630[index][0] <= static_cast<int>(knowledge)) {
            m_text_area_ec.AddEntry(
                gppStringList[g_monster_resistance_label_gates_64f630[index][1]],
                FormatWideString(g_format_d_0060aa20, record->resistances[index]), 10, 0xf, 0);
        }
    }

    if (g_flag_689b32 != 0) {
        m_text_area_ec.AddEntry(L"Range / Combat Ground",
                                FormatWideString(L"%.2f / %.2f M", combat_range * g_float_005ebc60,
                                                 (monster_info->monster->GetPosition() -
                                                  g_startup_world_659c0c->GetPosition())
                                                         .Length() *
                                                     g_world_cursor_scale_005ebf50),
                                5, 0xf, 0);
        leader_info = MonsterInfoFromID(0x1d6, MONSTER_INFO_DIALOG_CPP, leader_location_id, 1);
        script = leader_info->monster->script_238;
        m_text_area_ec.AddEntry(L"Leader's Current Script",
                                FormatWideString(L"<%S>", script != 0 ? script->getName() : 0), 5,
                                0xf, 0);
        m_text_area_ec.AddEntry(L"Leader's AI Mode",
                                FormatWideString(g_format_d_0060aa20, leader_info->ai_mode_255), 5,
                                0xf, 0);
        const wchar_t* strategy = L"Close";
        if (record->prefer_ranged_actions_1b9 != 0) {
            strategy = L"Ranged";
        }
        m_text_area_ec.AddEntry(L"Combat Strategy", strategy, 5, 0xf, 0);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005d6e60
void W8MonsterInfoDialog::OnRightButtonUp()
{
    if (m_right_button_down) {
        m_keep_open = 0;
    }
}

// FUNCTION: WIZ8 0x005d6e70
void W8MonsterInfoDialog::OnMouseWheel(int delta)
{
    if (delta > 0) {
        for (int step = 0; step < delta; ++step) {
            m_scroll_bar_58.ScrollUp();
        }
    } else if (delta < 0) {
        for (int step = 0; step < -delta; ++step) {
            m_scroll_bar_58.ScrollDown();
        }
    }
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
        dialog->m_text_area_ec.SetFirstVisibleLine(first_visible_entry);
        left = dialog->m_x + 0x11;
        top = dialog->m_y + 0x26;
        right = left + 0x10e;
        bottom = top + 0xb9;
        InvalidateRegion(left, top, right, bottom, 0);
        BlitCatalogSurfaceRectTo16BPP(-0xe, left, top, right, bottom, 0x1b6, 0, 0);
        dialog->m_text_area_ec.m_dirty = 1;
    }
}

// FUNCTION: WIZ8 0x005dbde0
void W8MonsterInfoDialog::DestroyControls()
{
    m_scroll_bar_58.DestroyControls();
    W8DialogBase::DestroyControls();
}

// FUNCTION: WIZ8 0x005d6080
void W8MonsterInfoDialog::Draw()
{
    if ((m_dirty_flags & 1) != 0) {
        if (m_initialized == 0) {
            CreateControls();
        }
        m_text_area_ec.m_dirty = 1;
        m_scroll_bar_58.m_dirty = 1;
        m_button_a4.m_dirty = 1;
        W8DialogBase::Draw();
        SetFont(g_font_683660);
        SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0x1f1, MONSTER_INFO_DIALOG_CPP, m_location_id, 1);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        wchar_t* name = GetMonsterName(monster_info, 0, 0);
        INT16 width = StringPixLength(name, g_font_683660);
        gprintf(m_x + 0xe + (0x112 - width) / 2, m_y + 0x11, L"%s", name);
    }
    m_text_area_ec.Draw(0);
    m_scroll_bar_58.Draw(0);
    m_button_a4.Draw();
}
