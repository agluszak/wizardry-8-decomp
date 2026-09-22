#include "wiz8/xstatus.h"
#include <wchar.h>
#include "wiz8/dialog_code/AssayDialog.h"
#include "wiz8/dialog_code/SpellInfoDialog.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/character_skills.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/string_database.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/float_constants.h"
#include "wiz8/fonts.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"

// GLOBAL: WIZ8 0x0064f908
W8DialogScrollBar::Resources g_assay_scroll_resources = {
    "Data\\Main Interface\\main_scroll.sti",
    "Data\\Dialogs\\popup_iteminfo.sti",
    4,
    W8AssayDialog::ScrollCallback,
};
// GLOBAL: WIZ8 0x0064f918
int g_assay_scroll_x_offset = 0x160;
// GLOBAL: WIZ8 0x0064f91c
int g_assay_scroll_y_offset = 0x48;
struct W8AssayButtonOffset {
    int x;
    int y;
};

// GLOBAL: WIZ8 0x0064f920
W8AssayButtonOffset g_assay_button_offsets[W8_ASSAY_BUTTON_COUNT] = {
    {0xb, 0xb},   {0xb, 0xb},   {0xd, 0xd},   {0x27, 0xd},  {0x46, 0x48}, {0x14f, 0xf5},
    {0xd, 0x2a},  {0xd, 0x47},  {0xd, 0x64},  {0xd, 0x81},  {0xd, 0x9e},  {0xd, 0xbb},
    {0xd, 0xd8},  {0x26, 0x2a}, {0x26, 0x47}, {0x26, 0x64}, {0x26, 0x81}, {0x26, 0x9e},
    {0x26, 0xbb}, {0x26, 0xd8}, {0x26, 0xf5}, {0xd, 0x2a},  {0xd, 0x47},  {0xd, 0x64},
    {0xd, 0x81},  {0xd, 0x9e},  {0xd, 0xbb},  {0xd, 0xd8},  {0xd, 0xf5},  {0x26, 0x2a},
    {0x26, 0x47}, {0x26, 0x64}, {0x26, 0x81}, {0x26, 0x9e}, {0x26, 0xbb}, {0x26, 0xd8},
    {0x26, 0xf5},
};
// GLOBAL: WIZ8 0x0064fa48
W8ControlsRect g_assay_text_buffer_offsets[W8_ASSAY_TEXT_BUFFER_COUNT] = {
    {117, 15, 343, 31}, {117, 35, 170, 47}, {174, 35, 343, 47},
    {117, 53, 170, 65}, {174, 53, 343, 65},
};
// GLOBAL: WIZ8 0x0064fa98
W8ControlsRect g_assay_text_area_offsets = {0x48, 0x4a, 0x156, 0xef};
// GLOBAL: WIZ8 0x0064faa8
int g_assay_text_buffer_string_ids[W8_ASSAY_TEXT_BUFFER_COUNT] = {270, 271, 270, 272, 270};
// GLOBAL: WIZ8 0x0061e7dc
unsigned short g_equip_class_name_ids_61e7dc[32] = {
    1087, 1088, 1089, 1090, 1091, 1092, 1093, 1094, 1095, 1096, 1097, 1098, 1099, 1100, 1101, 1102,
    1103, 1104, 1105, 1106, 1107, 1108, 1109, 1110, 1111, 1112, 1114, 1115, 1116, 1117, 1118, 1119};
// GLOBAL: WIZ8 0x0064fbb4
const wchar_t g_assay_format_1f_0064fbb4[] = L"%.1f";
// GLOBAL: WIZ8 0x0064fbc0
const wchar_t g_assay_format_1f_1f_s_0064fbc0[] = L"%.1f (%.1f %s)";
/* String-list label ids indexed by W8ItemDatabaseRecord::flags_041 bit. */
// GLOBAL: WIZ8 0x0061e938
const unsigned short g_item_flag_name_ids_61e938[8] = {
    1255, 1256, 1257, 1258, 1259, 1260, 1261, 1262,
};
/* String-list label ids indexed by W8ItemDatabaseRecord::quantity_kind. */
// GLOBAL: WIZ8 0x0061e948
const unsigned short g_quantity_kind_name_ids_61e948[7] = {
    1264, 1265, 1266, 1267, 1268, 0, 1269,
};
/* String-list label ids indexed by W8ItemDatabaseRecord::unknown_075. */
// GLOBAL: WIZ8 0x0061e97c
const unsigned short g_item_property_name_ids_61e97c[6] = {
    1289, 1290, 1291, 1292, 1293, 1294,
};
// GLOBAL: WIZ8 0x0069c818
static wchar_t g_assay_entry_text[0x101];

// GLOBAL: WIZ8 0x0064FAE4
static const char ASSAY_DIALOG_CPP[] = "C:\\Projects\\Wizardry 8\\Dialog Code\\AssayDialog.cpp";

enum { NUM_RPC_RACES = 5 };

// SYNTHETIC: WIZ8 0x005d7070
// W8AssayDialog::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005d6fb0
W8AssayDialog::W8AssayDialog(W8ItemInstance* item, W8Character* character)
{
    int index;

    SetExtent(0x180, 0x11d);
    SetBackground("Data\\Dialogs\\popup_iteminfo.sti", 0);
    for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
        m_buttons[index] = 0;
    }
    for (index = 0; index < W8_ASSAY_TEXT_BUFFER_COUNT; ++index) {
        m_text_buffers[index] = 0;
    }
    m_item_portrait_dirty = 0;
    m_character = character;
    m_item = item;
}

// FUNCTION: WIZ8 0x005d7090
W8AssayDialog::~W8AssayDialog()
{
    int index;

    W8DialogBase::DestroyControls();
    m_scroll_bar.DestroyControls();
    for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
        if (m_buttons[index] != 0) {
            delete m_buttons[index];
            m_buttons[index] = 0;
        }
    }
    for (index = 0; index < W8_ASSAY_TEXT_BUFFER_COUNT; ++index) {
        if (m_text_buffers[index] != 0) {
            delete m_text_buffers[index];
            m_text_buffers[index] = 0;
        }
    }
    NoOp();
}

// FUNCTION: WIZ8 0x005d7160
int W8AssayDialog::CreateControls()
{
    int index;

    W8DialogBase::CreateControls();
    m_item_portrait_dirty = 1;
    if (PopulateText() == 0) {
        m_error = 7;
        return 7;
    }
    if (m_scroll_bar.CreateControls(&g_assay_scroll_resources) != 0) {
        m_scroll_bar.SetLayout(g_assay_scroll_x_offset + m_x, g_assay_scroll_y_offset + m_y,
                               m_text_area.GetTotalLineCount(), 0, m_text_area.GetLineHeight(),
                               g_assay_text_area_offsets.bottom - g_assay_text_area_offsets.top);
        m_scroll_bar.m_owner = this;
        if (PopulateRequirements() == 0) {
            m_error = 7;
            return 7;
        }
        if (CreateTextBuffers() != 0) {
            m_buttons[0]->SetVisible(gXStatus.assay_professions_tab_19b8 != 0);
            SetProfessionIconsVisible(gXStatus.assay_professions_tab_19b8);
            m_buttons[1]->SetVisible(gXStatus.assay_professions_tab_19b8 == 0);
            SetRaceIconsVisible(gXStatus.assay_professions_tab_19b8 == 0);
            if (gXStatus.assay_professions_tab_19b8 != 0) {
                m_buttons[2]->SetPressed(true);
                return 0;
            }
            m_buttons[3]->SetPressed(true);
            return 0;
        }
        for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
            if (m_buttons[index] != 0) {
                delete m_buttons[index];
                m_buttons[index] = 0;
            }
        }
    }
    m_error = 7;
    return 7;
}

// FUNCTION: WIZ8 0x005d72b0
void W8AssayDialog::DestroyControls()
{
    int index;

    W8DialogBase::DestroyControls();
    m_scroll_bar.DestroyControls();
    for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
        if (m_buttons[index] != 0) {
            delete m_buttons[index];
            m_buttons[index] = 0;
        }
    }
    for (index = 0; index < W8_ASSAY_TEXT_BUFFER_COUNT; ++index) {
        if (m_text_buffers[index] != 0) {
            delete m_text_buffers[index];
            m_text_buffers[index] = 0;
        }
    }
}

// FUNCTION: WIZ8 0x005d7310
unsigned char W8AssayDialog::PopulateText()
{
    W8ControlsRect bounds;
    const W8ItemDatabaseRecord* record;
    const wchar_t* text;
    unsigned short slot_mask;
    const unsigned short* label;
    unsigned int bit;
    int index;
    int count;
    const W8ItemRequirement* requirement;
    const unsigned short (*flag_names)[2];
    wchar_t modifier_text[0x100];
    char path[0x200];
    wchar_t description[0x7d0];

    bounds.left = m_x + g_assay_text_area_offsets.left;
    bounds.top = m_y + g_assay_text_area_offsets.top;
    bounds.right = m_x + g_assay_text_area_offsets.right;
    bounds.bottom = m_y + g_assay_text_area_offsets.bottom;
    m_text_area.Configure(&bounds, g_font_683660, 0);
    m_text_area.SetEntrySpacing(1);
    record = &g_item_records[m_item->iItemNo];
    if (m_item->identified != 0 && record->damage_dice.count + record->damage_dice.base > 0) {
        if (!ItemHasSingledOutGenericName(m_item->iItemNo) || record->damage_dice.base == 0) {
            text = FormatWideString(
                L"%d - %d", record->damage_dice.base + record->damage_dice.count,
                record->damage_dice.sides * record->damage_dice.count + record->damage_dice.base);
        } else {
            text = FormatWideString(L"%+d%%", record->damage_dice.base * 10);
        }
        m_text_area.AddEntry(gppStringList[0x23a0 / 4], text, 10, 0xf, 0);
    }
    if (record->attack_hit_bonus != 0 && m_item->identified != 0) {
        m_text_area.AddEntry(gppStringList[0x22dc / 4],
                             FormatWideString(g_format_plus_d_0064dc24, record->attack_hit_bonus),
                             10, 0xf, 0);
    }
    if (record->attack_damage_bonus != 0 && m_item->identified != 0) {
        m_text_area.AddEntry(
            gppStringList[0x22c4 / 4],
            FormatWideString(g_format_plus_d_0064dc24, record->attack_damage_bonus), 10, 0xf, 0);
    }
    slot_mask = GetItemEquipSlotMask(m_item->iItemNo, 1, 1, 1, 1);
    count = 0;
    for (bit = 0; bit < 12; ++bit) {
        if (bit != 8 && bit != 9 && (slot_mask & (1 << bit)) != 0) {
            if (count == 0) {
                wcscpy(g_assay_entry_text, &g_wchar_00689b34);
            } else if (wcslen(g_assay_entry_text) + 1 + wcslen(g_comma_space_00619794) < 0x101) {
                wcscat(g_assay_entry_text, g_comma_space_00619794);
            }
            text = gppStringList[g_equip_slot_label_ids_61e7c4[bit]];
            if (wcslen(g_assay_entry_text) + 1 + wcslen(text) < 0x101) {
                wcscat(g_assay_entry_text, text);
            }
            ++count;
        }
    }
    if (count != 0) {
        m_text_area.AddEntry(gppStringList[0x2394 / 4], g_assay_entry_text, 10, 0xf, 0);
    }
    if (GetItemDefaultEquipSlot(m_item->iItemNo) != -1 &&
        g_item_records[m_item->iItemNo].equip_class != 4) {
        unsigned int palette;
        if (m_item->bound == 0) {
            text = gppStringList[0x23f0 / 4];
            palette = 0xf;
        } else if (record->binds_on_equip == 0) {
            text = gppStringList[0x23ec / 4];
            palette = 0xf;
        } else if (m_item->bind_announced == 0) {
            text = gppStringList[0x23e8 / 4];
            palette = 0;
        } else {
            text = gppStringList[0x23e4 / 4];
            palette = 0xf;
        }
        m_text_area.AddEntry(gppStringList[0x23f4 / 4], text, 10, palette, 0);
    }
    count = 0;
    bit = 0;
    label = g_item_flag_name_ids_61e938;
    do {
        if ((1 << bit) == 4 && (record->flags_041 & 4) != 0) {
            if (count == 0) {
                wcscpy(g_assay_entry_text, &g_wchar_00689b34);
            } else if (wcslen(g_assay_entry_text) + 1 + wcslen(g_comma_space_00619794) < 0x101) {
                wcscat(g_assay_entry_text, g_comma_space_00619794);
            }
            text = gppStringList[*label];
            if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
                wcscat(g_assay_entry_text, text);
            }
            ++count;
        }
        ++label;
        ++bit;
    } while (label < g_item_flag_name_ids_61e938 + 8);
    if (count != 0) {
        m_text_area.AddEntry(gppStringList[0x244c / 4], g_assay_entry_text, 10, 0xf, 0);
    }
    if (m_item->identified != 0) {
        count = 0;
        for (index = 0; index < 0x10; ++index) {
            if (record->missile_values_050[index] != 0) {
                if (count == 0) {
                    wcscpy(g_assay_entry_text, &g_wchar_00689b34);
                } else if (wcslen(g_assay_entry_text) + 1 + wcslen(g_comma_space_00619794) <
                           0x101) {
                    wcscat(g_assay_entry_text, g_comma_space_00619794);
                }
                text = gppStringList[g_damage_type_name_ids_61e9cc[index]];
                if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
                    wcscat(g_assay_entry_text, text);
                }
                if (wcslen(g_assay_entry_text) + 1 + wcslen(L" ") < 0x101) {
                    wcscat(g_assay_entry_text, L" ");
                }
                text = FormatWideString(g_format_d_percent_0064bab0,
                                        record->missile_values_050[index]);
                if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
                    wcscat(g_assay_entry_text, text);
                }
                ++count;
                if (index == 2) {
                    if (wcslen(g_assay_entry_text) + 1 + wcslen(L" (") < 0x101) {
                        wcscat(g_assay_entry_text, L" (");
                    }
                    text = gppStringList[0x2354 / 4];
                    if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
                        wcscat(g_assay_entry_text, text);
                    }
                    text = FormatWideString(L" %d)", record->missile_value_060);
                    if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
                        wcscat(g_assay_entry_text, text);
                    }
                }
            }
        }
        if (count != 0) {
            m_text_area.AddEntry(gppStringList[0x2358 / 4], g_assay_entry_text, 10, 0xf, 0);
        }
        if (record->slays_kind_061 != 0xff) {
            m_text_area.AddEntry(
                gppStringList[0x2360 / 4],
                gppStringList[g_special_category_name_ids_61ea78[record->slays_kind_061]], 10, 0xf,
                0);
        }
        if (record->equip_class == 5 && m_character != 0 &&
            IsItemWornByCharacter(m_character, m_item)) {
            m_text_area.AddEntry(
                gppStringList[0x237c / 4],
                FormatWideString(L"%+d, %+d %s", record->armor_class_bonus,
                                 m_character->armor_class_components[3] - record->armor_class_bonus,
                                 gppStringList[0x10a0 / 4]),
                10, 0xf, 0);
        } else if (record->armor_class_bonus != 0) {
            m_text_area.AddEntry(
                gppStringList[0x237c / 4],
                FormatWideString(g_format_plus_d_0064dc24, record->armor_class_bonus), 10, 0xf, 0);
        }
    }
    if (record->unknown_075 != 0) {
        m_text_area.AddEntry(gppStringList[0x23a4 / 4],
                             gppStringList[g_item_property_name_ids_61e97c[record->unknown_075]],
                             10, 0xf, 0);
    }
    switch (record->equip_class) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 0x11:
    case 0x12:
        m_text_area.AddEntry(gppStringList[0x239c / 4],
                             gppStringList[g_spell_range_name_ids_61e9a0[record->wield_group]], 10,
                             0xf, 0);
    }
    index = GetItemSpellPresentation(record);
    if (index == -1) {
        if (record->weapon_skill != -1) {
            m_text_area.AddEntry(
                gppStringList[0x2320 / 4],
                gppStringList[g_character_skill_name_ids_61e454[record->weapon_skill]], 10, 0xf, 0);
        }
    } else {
        if (record->weapon_skill == -1) {
            text = gppStringList[g_character_skill_name_ids_61e454[index]];
        } else {
            text = FormatWideString(
                L"%s, %s", gppStringList[g_character_skill_name_ids_61e454[index]],
                gppStringList[g_character_skill_name_ids_61e454[record->weapon_skill]]);
        }
        m_text_area.AddEntry(gppStringList[0x2320 / 4], text, 10, 0xf, 0);
    }
    count = 0;
    bit = 0;
    flag_names = g_attack_flag_name_ids_61e9a8;
    do {
        if ((record->attack_flags_04e & (1 << bit)) != 0) {
            if (count == 0) {
                wcscpy(g_assay_entry_text, &g_wchar_00689b34);
            } else if (wcslen(g_assay_entry_text) + 1 + wcslen(g_comma_space_00619794) < 0x101) {
                wcscat(g_assay_entry_text, g_comma_space_00619794);
            }
            text = gppStringList[(*flag_names)[0]];
            if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
                wcscat(g_assay_entry_text, text);
            }
            ++count;
        }
        ++flag_names;
        ++bit;
    } while (flag_names < g_attack_flag_name_ids_61e9a8 + 9);
    if (count != 0) {
        m_text_area.AddEntry(gppStringList[0x235c / 4], g_assay_entry_text, 10, 0xf, 0);
    }
    if ((m_item->identified != 0 || m_item->unknown_07[0] != 0) && record->spell_id != 0) {
        if (record->equip_class == 0xd || record->equip_class == 0xe ||
            record->equip_class == 0x13) {
            text = FormatWideString(g_format_s_006068e4,
                                    g_spell_records[record->spell_id].display_name);
        } else {
            text = FormatWideString(L"%s (Pwr %d)", g_spell_records[record->spell_id].display_name,
                                    record->unknown_064[0]);
        }
        m_text_area.AddEntry(gppStringList[0x23a8 / 4], text, 10, 0xf, 0);
        if (record->equip_class == 0x13) {
            m_text_area.AddEntry(gppStringList[0x23ac / 4],
                                 FormatWideString(g_format_d_0060aa20,
                                                  g_spell_records[record->spell_id].spell_level),
                                 10, 0xf, 0);
        }
    }
    if (m_item->identified != 0) {
        if (record->quantity_kind == 2) {
            m_text_area.AddEntry(
                gppStringList[g_quantity_kind_name_ids_61e948[record->quantity_kind]],
                FormatWideString(g_format_d_0060aa20, m_item->uses_or_charges), 10, 0xf, 0);
        }
        if (record->quantity_kind == 3 || record->quantity_kind == 4) {
            m_text_area.AddEntry(
                gppStringList[g_quantity_kind_name_ids_61e948[record->quantity_kind]],
                FormatWideString(g_format_d_slash_d_00614b58, m_item->uses_or_charges,
                                 record->initial_quantity.sides * record->initial_quantity.count +
                                     record->initial_quantity.base),
                10, 0xf, 0);
        }
        if (record->modifier_06c > 0) {
            m_text_area.AddEntry(gppStringList[0x2364 / 4],
                                 FormatWideString(g_format_plus_d_0064dc24, record->modifier_06c),
                                 10, 0xf, 0);
        }
        if (record->modifier_06c < 0) {
            m_text_area.AddEntry(gppStringList[0x2368 / 4],
                                 FormatWideString(g_format_d_0060aa20, record->modifier_06c), 10,
                                 0xf, 0);
        }
        if (record->modifier_06d > 0) {
            m_text_area.AddEntry(gppStringList[0x236c / 4],
                                 FormatWideString(g_format_plus_d_0064dc24, record->modifier_06d),
                                 10, 0xf, 0);
        }
        if (record->modifier_06d < 0) {
            m_text_area.AddEntry(gppStringList[0x2370 / 4],
                                 FormatWideString(g_format_d_0060aa20, record->modifier_06d), 10,
                                 0xf, 0);
        }
        if (record->modifier_06e > 0) {
            m_text_area.AddEntry(gppStringList[0x2374 / 4],
                                 FormatWideString(g_format_plus_d_0064dc24, record->modifier_06e),
                                 10, 0xf, 0);
        }
        if (record->modifier_06e < 0) {
            m_text_area.AddEntry(gppStringList[0x2378 / 4],
                                 FormatWideString(g_format_d_0060aa20, record->modifier_06e), 10,
                                 0xf, 0);
        }
        if (record->modifier_0b3_index != -1 && record->modifier_0b3_value > 0) {
            swprintf(
                modifier_text, L"%s %+d",
                gppStringList[g_character_description_first_ids_61e3a4[record->modifier_0b3_index]],
                record->modifier_0b3_value);
            m_text_area.AddEntry(gppStringList[0x230c / 4], modifier_text, 10, 0xf, 0);
        }
        if (record->modifier_0b3_index != -1 && record->modifier_0b3_value < 0) {
            swprintf(
                modifier_text, L"%s %d",
                gppStringList[g_character_description_first_ids_61e3a4[record->modifier_0b3_index]],
                record->modifier_0b3_value);
            m_text_area.AddEntry(gppStringList[0x2310 / 4], modifier_text, 10, 0xf, 0);
        }
        if (record->modifier_0b1_index != -1 && record->modifier_0b1_value > 0) {
            swprintf(modifier_text, L"%s %+d",
                     gppStringList[g_character_skill_name_ids_61e454[record->modifier_0b1_index]],
                     record->modifier_0b1_value);
            m_text_area.AddEntry(gppStringList[0x2314 / 4], modifier_text, 10, 0xf, 0);
        }
        if (record->modifier_0b1_index != -1 && record->modifier_0b1_value < 0) {
            swprintf(modifier_text, L"%s %d",
                     gppStringList[g_character_skill_name_ids_61e454[record->modifier_0b1_index]],
                     record->modifier_0b1_value);
            m_text_area.AddEntry(gppStringList[0x2318 / 4], modifier_text, 10, 0xf, 0);
        }
        count = 0;
        for (index = 0; index < 6; ++index) {
            if (record->resistance_bonus_06f[index] > 0) {
                if (count == 0) {
                    wcscpy(g_assay_entry_text, &g_wchar_00689b34);
                } else if (wcslen(g_assay_entry_text) + 1 + wcslen(g_comma_space_00619794) <
                           0x101) {
                    wcscat(g_assay_entry_text, g_comma_space_00619794);
                }
                text = FormatWideString(g_format_d_percent_0064bab0,
                                        record->resistance_bonus_06f[index]);
                if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
                    wcscat(g_assay_entry_text, text);
                }
                if (wcslen(g_assay_entry_text) + 1 + wcslen(L" vs. ") < 0x101) {
                    wcscat(g_assay_entry_text, L" vs. ");
                }
                text = gppStringList[g_realm_message_offsets[index]];
                if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
                    wcscat(g_assay_entry_text, text);
                }
                ++count;
            }
        }
        if (count != 0) {
            m_text_area.AddEntry(gppStringList[0x232c / 4], g_assay_entry_text, 10, 0xf, 0);
        }
    }
    if (record->gender_mask != 3) {
        if (record->gender_mask == 0) {
            text = gppStringList[0x2448 / 4];
        } else if (record->gender_mask != 1) {
            text = gppStringList[0x2444 / 4];
        } else {
            text = gppStringList[0x2440 / 4];
        }
        m_text_area.AddEntry(gppStringList[0x23cc / 4], text, 10, 0xf, 0);
    }
    count = 0;
    wcscpy(g_assay_entry_text, &g_wchar_00689b34);
    requirement = record->attribute_requirements;
    for (index = 0; index < 2; ++index, ++requirement) {
        if (requirement->stat_id != 0xff) {
            if (count != 0 &&
                wcslen(g_assay_entry_text) + 1 + wcslen(g_comma_space_00619794) < 0x101) {
                wcscat(g_assay_entry_text, g_comma_space_00619794);
            }
            text = gppStringList[g_character_description_first_ids_61e3a4[static_cast<signed char>(
                requirement->stat_id)]];
            if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
                wcscat(g_assay_entry_text, text);
            }
            text = FormatWideString(L" %d", requirement->minimum);
            if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
                wcscat(g_assay_entry_text, text);
            }
            ++count;
        }
    }
    requirement = record->skill_requirements;
    for (index = 0; index < 2; ++index, ++requirement) {
        if (requirement->stat_id != 0xff) {
            if (count != 0 &&
                wcslen(g_assay_entry_text) + 1 + wcslen(g_comma_space_00619794) < 0x101) {
                wcscat(g_assay_entry_text, g_comma_space_00619794);
            }
            text = gppStringList[g_character_skill_name_ids_61e454[static_cast<signed char>(
                requirement->stat_id)]];
            if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
                wcscat(g_assay_entry_text, text);
            }
            text = FormatWideString(L" %d", requirement->minimum);
            if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
                wcscat(g_assay_entry_text, text);
            }
            ++count;
        }
    }
    if (record->category == 6 || record->category == 8) {
        if (record->spell_id == 0) {
            srAssertFail("uiSpell != SPELL_NONE", ASSAY_DIALOG_CPP, 0x390, 0);
        }
        if (count != 0) {
            if (wcslen(g_assay_entry_text) + 1 + wcslen(g_comma_space_00619794) < 0x101) {
                wcscat(g_assay_entry_text, g_comma_space_00619794);
            }
        }
        if (record->category == 6) {
            text = gppStringList[0x23f8 / 4];
        } else {
            text = gppStringList[0x23fc / 4];
        }
        if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
            wcscat(g_assay_entry_text, text);
        }
        text = FormatWideString(L" %ld", GetMinimumCasterLevelForSpell(record->spell_id));
        if (wcslen(text) + 1 + wcslen(g_assay_entry_text) < 0x101) {
            wcscat(g_assay_entry_text, text);
        }
    }
    if (count != 0) {
        m_text_area.AddEntry(gppStringList[0x23d0 / 4], g_assay_entry_text, 10, 0xf, 0);
    }
    if (record->equip_class == 0x13 && (m_item->identified != 0 || m_item->unknown_07[0] != 0) &&
        m_character != 0) {
        if (m_character->spell_learned[record->spell_id] == 1) {
            m_text_area.AddEntry(0, gppStringList[0x23b4 / 4], 10, 1, 0);
        } else if (CanCharacterLearnSpell(m_character, record->spell_id) == 0 &&
                   CanCastFromItem(m_character, m_item)) {
            m_text_area.AddEntry(0, gppStringList[0x23b0 / 4], 10, 0, 0);
        }
    }
    if (record->maximum_quantity != 0) {
        m_text_area.AddEntry(gppStringList[0x23e0 / 4],
                             FormatWideString(g_format_d_0060aa20, record->maximum_quantity), 10,
                             0xf, 0);
    }
    strcpy(path, "Data\\Databases\\ItemDesc.dbs");
    GetStringFromStringDatabase(path, m_item->iItemNo, description, 0, 0);
    if (wcslen(description) != 0 && m_item->identified != 0) {
        m_text_area.AddEntry(gppStringList[0x243c / 4], description, 10, 0xf, 0);
    }
    m_text_area.m_dirty = 1;
    return 1;
}

/* The SurRender headers pull <iostream>, so retail emitted the VC6 stream
   statics here: __winit (ios_base::_Winit) at 0x0069C814 and __ioinit
   (ios_base::Init) at 0x0069C815. The lint iostream stub shadows the real
   header, so the toolchain does not emit the objects or these thunks. */
// SYNTHETIC: WIZ8 0x005D87F0
// `dynamic initializer for '__ioinit''
// SYNTHETIC: WIZ8 0x005D8810
// `dynamic atexit destructor for '__ioinit''
// SYNTHETIC: WIZ8 0x005D8820
// `dynamic initializer for '__winit''
// SYNTHETIC: WIZ8 0x005D8840
// `dynamic atexit destructor for '__winit''

// FUNCTION: WIZ8 0x005d9200
void W8AssayDialog::Draw()
{
    int index;

    if ((m_dirty_flags & 1) != 0) {
        if (m_initialized == 0) {
            CreateControls();
        }
        m_item_portrait_dirty = 1;
        m_text_area.m_dirty = 1;
        m_scroll_bar.m_dirty = 1;
        for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
            m_buttons[index]->m_dirty = 1;
        }
        for (index = 0; index < W8_ASSAY_TEXT_BUFFER_COUNT; ++index) {
            m_text_buffers[index]->SetGeometryDirty();
        }
        W8DialogBase::Draw();
    }
    if (m_item_portrait_dirty != 0) {
        DrawCatalogImageAndInvalidate(
            -0xe, g_item_video_objects_68ec68.GetOrCreateVideoObject(m_item->iItemNo), 0, 0,
            m_x + 0x45, m_y + 0xe, 2, 0);
        if (m_item->identified == 0) {
            DrawCatalogImageAndInvalidate(-0xe, 0x11b, 0, 0, m_x + 0x45, m_y + 0xe, 2, 0);
        }
        m_item_portrait_dirty = 0;
    }
    for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
        if (m_buttons[index] != 0) {
            m_buttons[index]->Draw();
        }
    }
    for (index = 0; index < W8_ASSAY_TEXT_BUFFER_COUNT; ++index) {
        if (m_text_buffers[index] != 0) {
            m_text_buffers[index]->RenderToTarget(0, 0, -0xe);
        }
    }
    m_text_area.Draw(0);
    m_scroll_bar.Draw(0);
}

/* Same folded body as W8MonsterInfoDialog::OnRightButtonUp at 0x005D6E60;
   /OPT:NOICF emits this copy. */
void W8AssayDialog::OnRightButtonUp()
{
    if (m_right_button_down) {
        m_keep_open = 0;
    }
}

// FUNCTION: WIZ8 0x005d9720
void W8AssayDialog::OnMouseWheel(int delta)
{
    if (delta > 0) {
        for (int step = 0; step < delta; ++step) {
            m_scroll_bar.ScrollUp();
        }
    } else if (delta < 0) {
        for (int step = 0; step < -delta; ++step) {
            m_scroll_bar.ScrollDown();
        }
    }
}

// FUNCTION: WIZ8 0x005d9620
void W8AssayDialog::ShowPrimaryTab()
{
    if (m_buttons[2]->IsPressed() == 0) {
        m_buttons[2]->SetPressed(true);
        m_buttons[2]->m_dirty = 1;
    }
    if (m_buttons[3]->IsPressed() != 0) {
        m_buttons[3]->SetPressed(false);
        m_buttons[3]->m_dirty = 1;
    }
    gXStatus.assay_professions_tab_19b8 = 1;
    m_buttons[0]->SetVisible(true);
    SetProfessionIconsVisible(1);
    m_buttons[1]->SetVisible(false);
    SetRaceIconsVisible(0);
    m_buttons[0]->m_dirty = 1;
    m_buttons[1]->m_dirty = 1;
    m_buttons[3]->m_dirty = 1;
}

// FUNCTION: WIZ8 0x005d96a0
void W8AssayDialog::ShowSecondaryTab()
{
    if (m_buttons[3]->IsPressed() == 0) {
        m_buttons[3]->SetPressed(true);
        m_buttons[3]->m_dirty = 1;
    }
    if (m_buttons[2]->IsPressed() != 0) {
        m_buttons[2]->SetPressed(false);
        m_buttons[2]->m_dirty = 1;
    }
    gXStatus.assay_professions_tab_19b8 = 0;
    m_buttons[0]->SetVisible(false);
    SetProfessionIconsVisible(0);
    m_buttons[1]->SetVisible(true);
    SetRaceIconsVisible(1);
    m_buttons[0]->m_dirty = 1;
    m_buttons[1]->m_dirty = 1;
    m_buttons[2]->m_dirty = 1;
}

// FUNCTION: WIZ8 0x005d9760
void W8AssayDialog::PrimaryTabCallback(W8DialogButton* button)
{
    if (button != 0) {
        static_cast<W8AssayDialog*>(button->m_owner_040)->ShowPrimaryTab();
    }
}

// FUNCTION: WIZ8 0x005d9780
void W8AssayDialog::SecondaryTabCallback(W8DialogButton* button)
{
    if (button != 0) {
        static_cast<W8AssayDialog*>(button->m_owner_040)->ShowSecondaryTab();
    }
}

// FUNCTION: WIZ8 0x005d97a0
void W8AssayDialog::ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry)
{
    W8AssayDialog* dialog;
    if (scroll_bar != 0) {
        dialog = static_cast<W8AssayDialog*>(scroll_bar->m_owner);
        dialog->m_text_area.SetFirstVisibleLine(first_visible_entry);
        dialog->m_buttons[4]->m_dirty = 1;
        dialog->m_text_area.m_dirty = 1;
    }
}

// FUNCTION: WIZ8 0x005d8850
unsigned char W8AssayDialog::PopulateRequirements()
{
    int index;
    unsigned short us_index;
    int button_index;
    int frame;
    int tooltip_index;

    for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
        m_buttons[index] = new W8DialogButton;
        if (m_buttons[index] == 0) {
            for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
                if (m_buttons[index] != 0) {
                    delete m_buttons[index];
                    m_buttons[index] = 0;
                }
            }
            return 0;
        }
    }

    m_buttons[0]->Configure("Data\\Dialogs\\popup_iteminfo.sti", -1, 1, 1, 1, 1, 0, 0, 0, 0, -1, 0,
                            0);
    m_buttons[1]->Configure("Data\\Dialogs\\popup_iteminfo.sti", -1, 2, 2, 2, 2, 0, 0, 0, 0, -1, 0,
                            0);
    m_buttons[2]->Configure("Data\\Dialogs\\iteminfo_tabbutton.sti", 3, 0, 1, 4, 2,
                            PrimaryTabCallback, 0, 1, 0x7f, 0x115, 0, 0);
    m_buttons[3]->Configure("Data\\Dialogs\\iteminfo_tabbutton.sti", 8, 5, 6, 9, 7,
                            SecondaryTabCallback, 0, 1, 0x7f, 0x116, 0, 0);
    m_buttons[4]->Configure("Data\\Dialogs\\popup_iteminfo.sti", -1, 3, 3, 3, 3, 0, 0, 0, 0, -1, 0,
                            0);
    m_buttons[5]->Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 0, 1, 2, 2,
                            W8TriggerItemPickerDialog::CloseOwningDialog, 0, 0, 0x7f, 0x12, 0, 0);
    m_buttons[6]->Configure("Data\\Dialogs\\icons_profession.sti", 1, 0, -1, 0, -1, 0, 0, 0, 0x7f,
                            0x2ad, 0, 0);
    m_buttons[7]->Configure("Data\\Dialogs\\icons_profession.sti", 3, 2, -1, 2, -1, 0, 0, 0, 0x7f,
                            0x2a7, 0, 0);
    m_buttons[8]->Configure("Data\\Dialogs\\icons_profession.sti", 5, 4, -1, 4, -1, 0, 0, 0, 0x7f,
                            0x2b0, 0, 0);
    m_buttons[9]->Configure("Data\\Dialogs\\icons_profession.sti", 7, 6, -1, 6, -1, 0, 0, 0, 0x7f,
                            0x2ac, 0, 0);
    m_buttons[10]->Configure("Data\\Dialogs\\icons_profession.sti", 9, 8, -1, 8, -1, 0, 0, 0, 0x7f,
                             0x2a6, 0, 0);
    m_buttons[11]->Configure("Data\\Dialogs\\icons_profession.sti", 0xb, 10, -1, 10, -1, 0, 0, 0,
                             0x7f, 0x2a4, 0, 0);
    m_buttons[12]->Configure("Data\\Dialogs\\icons_profession.sti", 0xd, 0xc, -1, 0xc, -1, 0, 0, 0,
                             0x7f, 0x2aa, 0, 0);
    m_buttons[13]->Configure("Data\\Dialogs\\icons_profession.sti", 0xf, 0xe, -1, 0xe, -1, 0, 0, 0,
                             0x7f, 0x2b1, 0, 0);
    m_buttons[14]->Configure("Data\\Dialogs\\icons_profession.sti", 0x11, 0x10, -1, 0x10, -1, 0, 0,
                             0, 0x7f, 0x2a8, 0, 0);
    m_buttons[15]->Configure("Data\\Dialogs\\icons_profession.sti", 0x13, 0x12, -1, 0x12, -1, 0, 0,
                             0, 0x7f, 0x2ae, 0, 0);
    m_buttons[16]->Configure("Data\\Dialogs\\icons_profession.sti", 0x15, 0x14, -1, 0x14, -1, 0, 0,
                             0, 0x7f, 0x2a9, 0, 0);
    m_buttons[17]->Configure("Data\\Dialogs\\icons_profession.sti", 0x17, 0x16, -1, 0x16, -1, 0, 0,
                             0, 0x7f, 0x2af, 0, 0);
    m_buttons[18]->Configure("Data\\Dialogs\\icons_profession.sti", 0x19, 0x18, -1, 0x18, -1, 0, 0,
                             0, 0x7f, 0x2a5, 0, 0);
    m_buttons[19]->Configure("Data\\Dialogs\\icons_profession.sti", 0x1b, 0x1a, -1, 0x1a, -1, 0, 0,
                             0, 0x7f, 0x2b2, 0, 0);
    m_buttons[20]->Configure("Data\\Dialogs\\icons_profession.sti", 0x1d, 0x1c, -1, 0x1c, -1, 0, 0,
                             0, 0x7f, 0x2ab, 0, 0);
    m_buttons[21]->Configure("Data\\Dialogs\\icons_race.sti", 1, 0, -1, 0, -1, 0, 0, 0, 0x7f, 0x28a,
                             0, 0);
    m_buttons[22]->Configure("Data\\Dialogs\\icons_race.sti", 3, 2, -1, 2, -1, 0, 0, 0, 0x7f, 0x28c,
                             0, 0);
    m_buttons[23]->Configure("Data\\Dialogs\\icons_race.sti", 5, 4, -1, 4, -1, 0, 0, 0, 0x7f, 0x285,
                             0, 0);
    m_buttons[24]->Configure("Data\\Dialogs\\icons_race.sti", 7, 6, -1, 6, -1, 0, 0, 0, 0x7f, 0x28e,
                             0, 0);
    m_buttons[25]->Configure("Data\\Dialogs\\icons_race.sti", 9, 8, -1, 8, -1, 0, 0, 0, 0x7f, 0x284,
                             0, 0);
    m_buttons[26]->Configure("Data\\Dialogs\\icons_race.sti", 0xb, 10, -1, 10, -1, 0, 0, 0, 0x7f,
                             0x28b, 0, 0);
    m_buttons[27]->Configure("Data\\Dialogs\\icons_race.sti", 0xd, 0xc, -1, 0xc, -1, 0, 0, 0, 0x7f,
                             0x286, 0, 0);
    m_buttons[28]->Configure("Data\\Dialogs\\icons_race.sti", 0xf, 0xe, -1, 0xe, -1, 0, 0, 0, 0x7f,
                             0x288, 0, 0);
    m_buttons[29]->Configure("Data\\Dialogs\\icons_race.sti", 0x11, 0x10, -1, 0x10, -1, 0, 0, 0,
                             0x7f, 0x287, 0, 0);
    m_buttons[30]->Configure("Data\\Dialogs\\icons_race.sti", 0x13, 0x12, -1, 0x12, -1, 0, 0, 0,
                             0x7f, 0x28d, 0, 0);
    m_buttons[31]->Configure("Data\\Dialogs\\icons_race.sti", 0x15, 0x14, -1, 0x14, -1, 0, 0, 0,
                             0x7f, 0x289, 0, 0);

    for (us_index = 0; us_index < NUM_RPC_RACES; ++us_index) {
        if (us_index >= NUM_RPC_RACES) {
            srAssertFail("usIndex < NUM_RPC_RACES", ASSAY_DIALOG_CPP, 1681, 0);
        }
        switch (us_index) {
        case 0:
            button_index = 0x22;
            break;
        case 1:
            button_index = 0x1b;
            break;
        case 2:
            button_index = 0x23;
            break;
        case 3:
            button_index = 0x1c;
            break;
        case 4:
            button_index = 0x24;
            break;
        default:
            button_index = -1;
            srAssertFail("iUnknownButtonIndex != MAXDWORD", ASSAY_DIALOG_CPP, 0x43f, 0);
            break;
        }
        switch (g_status_685170.rpc_races_243a[us_index]) {
        case 0xb:
            frame = 0x1e;
            tooltip_index = 0x28f;
            break;
        case 0xc:
            frame = 0x18;
            tooltip_index = 0x290;
            break;
        case 0xd:
            frame = 0x16;
            tooltip_index = 0x291;
            break;
        case 0xe:
            frame = 0x1a;
            tooltip_index = 0x292;
            break;
        case 0xf:
            frame = 0x1c;
            tooltip_index = 0x293;
            break;
        default:
            frame = 8;
            tooltip_index = 0x284;
            break;
        }
        m_buttons[button_index]->Configure("Data\\Dialogs\\icons_race.sti", frame + 1, frame, -1,
                                           frame, -1, 0, 0, 0, 0x7f, tooltip_index, 0, 0);
        m_buttons[button_index]->SetVisible(false);
    }

    for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
        m_buttons[index]->SetPosition(m_x + g_assay_button_offsets[index].x,
                                      m_y + g_assay_button_offsets[index].y);
        m_buttons[index]->m_owner_040 = this;
    }
    return 1;
}

// FUNCTION: WIZ8 0x005d8fb0
unsigned char W8AssayDialog::CreateTextBuffers()
{
    int index;
    W8ControlsRect bounds;
    W8ItemInstance* item;
    unsigned char equip_class;
    wchar_t* text;

    for (index = 0; index < W8_ASSAY_TEXT_BUFFER_COUNT; ++index) {
        bounds.left = m_x + g_assay_text_buffer_offsets[index].left;
        bounds.top = m_y + g_assay_text_buffer_offsets[index].top;
        bounds.right = m_x + g_assay_text_buffer_offsets[index].right;
        bounds.bottom = m_y + g_assay_text_buffer_offsets[index].bottom;
        m_text_buffers[index] = new W8TextBuffer(
            &bounds, gppStringList[g_assay_text_buffer_string_ids[index]], g_font_683660,
            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C, 4);
        if (m_text_buffers[index] == 0) {
            for (index = 0; index < W8_ASSAY_TEXT_BUFFER_COUNT; ++index) {
                if (m_text_buffers[index] != 0) {
                    delete m_text_buffers[index];
                    m_text_buffers[index] = 0;
                }
            }
            return 0;
        }
    }

    m_text_buffers[0]->SetText(FormatItemDisplayName(m_item, 0), g_font_683660);
    equip_class = GetItemEquipClass(m_item);
    m_text_buffers[2]->SetText(gppStringList[g_equip_class_name_ids_61e7dc[equip_class]],
                               g_font_683660);
    item = m_item;
    if (g_item_records[item->iItemNo].quantity_kind == 1 && item->stack_count > 1) {
        unsigned int unit_weight = GetItemUnitWeight(item);
        text = FormatWideString(g_assay_format_1f_1f_s_0064fbc0,
                                (double)((float)GetItemStackWeight(m_item) * g_float_005ed8b8),
                                (double)((float)unit_weight * g_float_005ed8b8),
                                gppStringList[0x45c / 4]);
    } else {
        text = FormatWideString(g_assay_format_1f_0064fbb4,
                                (double)((float)GetItemUnitWeight(item) * g_float_005ed8b8));
    }
    m_text_buffers[4]->SetText(text, g_font_683660);
    return 1;
}

// FUNCTION: WIZ8 0x005d9330
void W8AssayDialog::SetProfessionIconsVisible(int show)
{
    unsigned int profession;
    int button_index;
    const W8ItemDatabaseRecord* record = &g_item_records[m_item->iItemNo];

    for (profession = 0; profession < 15; ++profession) {
        switch (profession) {
        case 0:
            button_index = 6;
            break;
        case 1:
            button_index = 7;
            break;
        case 2:
            button_index = 8;
            break;
        case 3:
            button_index = 9;
            break;
        case 4:
            button_index = 10;
            break;
        case 5:
            button_index = 0xb;
            break;
        case 6:
            button_index = 0xc;
            break;
        case 7:
            button_index = 0xd;
            break;
        case 8:
            button_index = 0xf;
            break;
        case 9:
            button_index = 0xe;
            break;
        case 10:
            button_index = 0x10;
            break;
        case 11:
            button_index = 0x14;
            break;
        case 12:
            button_index = 0x11;
            break;
        case 13:
            button_index = 0x13;
            break;
        case 14:
            button_index = 0x12;
            break;
        default:
            continue;
        }
        m_buttons[button_index]->SetVisible(show != 0);
        if (show != 0) {
            m_buttons[button_index]->SetEnabled(
                (record->profession_mask & (1 << (profession & 0x1f))) != 0);
        }
        m_buttons[button_index]->m_dirty = 1;
    }
}

// FUNCTION: WIZ8 0x005d9460
void W8AssayDialog::SetRaceIconsVisible(int show)
{
    unsigned int race;
    unsigned int us_index;
    int button_index;
    const W8ItemDatabaseRecord* record = &g_item_records[m_item->iItemNo];

    for (race = 0; race < 16; ++race) {
        switch (race) {
        case 0:
            button_index = 0x15;
            break;
        case 1:
            button_index = 0x1d;
            break;
        case 2:
            button_index = 0x16;
            break;
        case 3:
            button_index = 0x1e;
            break;
        case 4:
            button_index = 0x17;
            break;
        case 5:
            button_index = 0x1f;
            break;
        case 6:
            button_index = 0x19;
            break;
        case 7:
            button_index = 0x21;
            break;
        case 8:
            button_index = 0x18;
            break;
        case 9:
            button_index = 0x20;
            break;
        case 10:
            button_index = 0x1a;
            break;
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            button_index = -1;
            for (us_index = 0; us_index < NUM_RPC_RACES; ++us_index) {
                if (race == g_status_685170.rpc_races_243a[us_index]) {
                    if (us_index >= NUM_RPC_RACES) {
                        srAssertFail("usIndex < NUM_RPC_RACES", ASSAY_DIALOG_CPP, 0x691, 0);
                    }
                    switch (us_index) {
                    case 0:
                        button_index = 0x22;
                        break;
                    case 1:
                        button_index = 0x1b;
                        break;
                    case 2:
                        button_index = 0x23;
                        break;
                    case 3:
                        button_index = 0x1c;
                        break;
                    case 4:
                        button_index = 0x24;
                        break;
                    }
                    break;
                }
            }
            if (button_index == -1) {
                continue;
            }
            break;
        default:
            continue;
        }
        m_buttons[button_index]->SetVisible(show != 0);
        if (show != 0) {
            m_buttons[button_index]->SetEnabled((record->race_mask & (1 << (race & 0x1f))) != 0);
        }
        m_buttons[button_index]->m_dirty = 1;
    }
}
