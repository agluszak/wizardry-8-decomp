#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/npc_state.h"
#include "wiz8/string_database.h"
#include "wiz8/xstatus.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/screen_state.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/startup_runtime_state.h"
#include "random.h"
#include "timer.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "bink.h"
#include "FileMan.h"

#include <stdio.h>
#include <wchar.h>

// GLOBAL: WIZ8 0x0068c57c
unsigned int g_value_0068c57c;
/* 0x0068C580: the shared wide buffer formatted character text lands in. The
   message reader admits at most 0x7D0 code units, so the buffer holds exactly
   the two thousand characters that reach the next global at 0x0068D520. */
// GLOBAL: WIZ8 0x0068C580
wchar_t g_character_text_0068c580[2000];
/* 0x005ED91C: the quote file-name stem per personality, a twenty-byte fixed
   buffer each. The nine personas end exactly at the next global; the quote
   lookup composes Data\Quotes\PCs\<m|f>_<stem><1|2>0.MSG from them. */
// GLOBAL: WIZ8 0x005ed91c
const char g_quote_personality_names_005ed91c[9][0x14] = {
    "aggr", "intell", "burly", "chaos", "cun", "ecc", "kind", "laid", "loner",
};
// GLOBAL: WIZ8 0x0068c554
unsigned int g_value_0068c554;
// GLOBAL: WIZ8 0x0061cb44
int g_pose_transition_table_0061cb44[30] = {
    0x1380080, 0x1380080, 0x130013, 0x670067, 0xbc00bc,
    0x1110111, 1, 3, 3, 4, 5, 3, 2, 3, 3,
    3, 1, 2, 3, 4, 1, 1, 3, 3, 4, 1, 1, 1, 1, 1,
};

// GLOBAL: WIZ8 0x005ED8C8
int g_effect_argument_005ed8c8 = 0;
// GLOBAL: WIZ8 0x005ED8F8
unsigned int g_flee_hp_fraction_005ed8f8 = 50;
// GLOBAL: WIZ8 0x005ED8FC
unsigned int g_value_005ed8fc = 20;
// GLOBAL: WIZ8 0x005ED914
int g_effect_argument_005ed914 = 127;
// GLOBAL: WIZ8 0x005EE590
int g_effect_005ee590 = 2;
// GLOBAL: WIZ8 0x005EE594
int g_effect_005ee594 = 3;
// GLOBAL: WIZ8 0x005ee598
int g_effect_005ee598 = 4;
// GLOBAL: WIZ8 0x005ee588
int g_effect_005ee588 = 0;
// GLOBAL: WIZ8 0x005EE5F8
int g_effect_005ee5f8 = 28;
// GLOBAL: WIZ8 0x005ee610
int g_effect_005ee610 = 34;
// GLOBAL: WIZ8 0x005ee640
int g_item_message_005ee640 = 46;
// GLOBAL: WIZ8 0x005ee644
int g_item_message_005ee644 = 47;
// GLOBAL: WIZ8 0x005ee648
int g_item_message_005ee648 = 48;
// GLOBAL: WIZ8 0x005ee64c
int g_item_message_005ee64c = 49;
// GLOBAL: WIZ8 0x005ee664
int g_item_message_005ee664 = 55;
// GLOBAL: WIZ8 0x005ee68c
int g_item_message_005ee68c = 65;
// GLOBAL: WIZ8 0x005ee690
int g_item_message_005ee690 = 66;
// GLOBAL: WIZ8 0x005ee6fc
int g_item_message_005ee6fc = 132;
// GLOBAL: WIZ8 0x005EE70C
unsigned int g_last_event_005ee70c = 146;
// GLOBAL: WIZ8 0x005EE718
unsigned int g_first_remapped_event_005ee718 = 500;

// GLOBAL: WIZ8 0x0068C504
int g_special_event_0068c504;
// GLOBAL: WIZ8 0x0068C50C
int g_special_event_0068c50c;
// GLOBAL: WIZ8 0x0068C51C
int g_special_event_0068c51c;
// GLOBAL: WIZ8 0x0068C538
int g_special_event_0068c538;
// GLOBAL: WIZ8 0x0068C540
int g_special_event_0068c540;
// GLOBAL: WIZ8 0x0068C550
int g_special_event_0068c550;
// GLOBAL: WIZ8 0x0068C558
int g_special_event_0068c558;
// GLOBAL: WIZ8 0x0068C564
int g_special_event_0068c564;
// GLOBAL: WIZ8 0x0068C568
int g_special_event_0068c568;

/* Character-event queue and portrait/voice updates. The original
   translation-unit name is unknown; the existing unit is retained intact. */

// FUNCTION: WIZ8 0x0052E360
bool IsVoiceMuted(void)
{
    return g_settings_6850c8.muted_voice_volume != 0xff;
}

// FUNCTION: WIZ8 0x0052E370
void SetVoiceMuted(unsigned char muted)
{
    if (muted != 0) {
        if (g_settings_6850c8.muted_voice_volume == 0xff) {
            g_settings_6850c8.muted_voice_volume = g_settings_6850c8.voice_volume;
            g_settings_6850c8.voice_volume = 0;
        }
    }
    else if (g_settings_6850c8.muted_voice_volume != 0xff) {
        g_settings_6850c8.voice_volume = g_settings_6850c8.muted_voice_volume;
        g_settings_6850c8.muted_voice_volume = 0xff;
    }
}

// FUNCTION: WIZ8 0x0052C810
W8StartupStateElement005EE748::W8StartupStateElement005EE748(
    W8Character* character, unsigned int type, int value_0c_arg,
    unsigned int flags, int value_14_arg)
    : handled_00(0), character_04(character), type_08(type),
      value_0c(value_0c_arg), flags_10(flags), value_14(value_14_arg),
      value_30(0)
{
    item_24.item_id = -1;
    switch (type) {
    case 2:
    case 3:
        value_18 = character->hp_current;
        break;
    case 5:
    case 6:
    case 9:
    case 0x54:
        value_18 = character->unknown_0b01;
        break;
    case 7:
        value_18 = 12;
        break;
    case 0x38:
    case 0x55:
        value_18 = character->hp_current;
        value_1c = character->unknown_0b01;
        break;
    }
}

/* 0x0052D0B0: format one character quote for the given event type into the
   shared wide text buffer. A party member on any screen but the character
   screen takes the text from the NPC bound to its slot; otherwise the type
   selects an entry of the sex/personality/voice quote file, which is then
   wrapped in quotes. Clears the buffer and answers zero when no quote exists. */
// FUNCTION: WIZ8 0x0052D0B0
unsigned char FormatCharacterQuoteText(
    W8Character* character, unsigned int type, unsigned int* metadata)
{
    char path[80];
    wchar_t text[500];
    int npc_index;
    unsigned char has_npc;

    if (type >= 0x92) {
        return 0;
    }
    if (metadata != 0) {
        *metadata = 0xffffffff;
    }
    npc_index = -1;
    has_npc = 0;
    if (character->in_party != 0 &&
        g_current_screen_state.id != W8_SCREEN_CHARACTER) {
        unsigned int slot = CharacterPointerToPartySlot(character);
        npc_index = g_status_685170.buffers.party_rows[slot].animation_0fa;
        has_npc = npc_index != -1;
    }
    if (!has_npc) {
        char gender_code =
            static_cast<char>(((character->gender != 0) - 1U & 7) + 0x66);
        sprintf(path, "Data\\Quotes\\PCs\\%c_%s%d0.MSG",
                gender_code,
                g_quote_personality_names_005ed91c[character->personality_0081],
                (character->voice_0085 != 0) + 1);
        if (!FileExists(path)) {
            g_character_text_0068c580[0] = 0;
            return 0;
        }
        GetStringFromStringDatabase(
            path, type, g_character_text_0068c580, 0, metadata);
        g_character_text_0068c580[wcslen(g_character_text_0068c580) - 1] = 0;
    }
    else {
        W8NpcState* npc = GetNpcState(npc_index);
        if (GetNpcQuoteText(npc, type, g_character_text_0068c580) == 0) {
            g_character_text_0068c580[0] = 0;
            return 0;
        }
    }

    if (wcslen(g_character_text_0068c580) == 0) {
        return 0;
    }
    swprintf(text, L"\"%s\"", g_character_text_0068c580);
    wcscpy(g_character_text_0068c580, text);
    return 1;
}

/* The quote text builder fills the shared wide buffer; the final character
   page's description area displays it. */
// FUNCTION: WIZ8 0x0052D240
wchar_t* W8StartupStateElement005EE748::GetQuoteText()
{
    FormatCharacterQuoteText(character_04, type_08, 0);
    return g_character_text_0068c580;
}

/* Restarts the follow-up clock for entries of the middle event band while the
   state flag selects it. */
// FUNCTION: WIZ8 0x0052E160
void W8StartupRuntimeState::RestartFollowUpClock(W8StartupStateElement005EE748* entry)
{
    int flags = value_5c;
    unsigned int type = entry->type_08;
    int duration;

    if ((flags & 1) == 0 || type < 14 || type >= 16) {
        return;
    }
    if ((flags & 2) == 0) {
        duration = Random(60000) + 300000;
    }
    else {
        duration = Random(6000) + 2000;
    }
    unknown_60 = SetCountdownClock(duration);
}

// FUNCTION: WIZ8 0x0052D610
int W8StartupRuntimeState::QueueEntry(W8StartupStateElement005EE748* entry)
{
    unsigned int event_index = entry->type_08;
    unsigned int party_slot;

    if (event_index >= g_first_remapped_event_005ee718) {
        event_index += g_last_event_005ee70c - g_first_remapped_event_005ee718;
    }
    party_slot = CharacterPointerToPartySlot(entry->character_04);
    if ((bytes_68[event_index] & (1 << (party_slot & 31))) != 0) {
        delete entry;
        return 0;
    }
    if (g_status_685170.flag_2497 != 0) {
        delete entry;
        return 0;
    }
    if (entry->type_08 > 0x91 && (entry->flags_10 & 0x20) == 0) {
        W8MonsterManagerEntry* slot =
            &g_monster_manager_entries[party_slot];
        if (slot->field_071 != 0) {
            vector_40.Remove(slot->field_071);
            RestartFollowUpClock(slot->field_071);
            slot->field_071->Process0052CED0();
            delete slot->field_071;
        }
        Function52CA60();
        return 1;
    }
    if (entry->type_08 == 0x21) {
        int index;
        if (vector_40.count > 0 && vector_40.data[0]->type_08 == 0x21) {
            delete entry;
            return 0;
        }
        for (index = 0; index < vector_10.count; ++index) {
            if (vector_10.data[index]->type_08 == 0x21) {
                delete entry;
                return 0;
            }
        }
    }
    if ((Function525DF0(0) != 0 || Function525DD0() != 0) &&
        (entry->flags_10 & 8) == 0) {
        vector_30.Add(entry);
        return 1;
    }
    vector_10.Add(entry);
    return 1;
}

// FUNCTION: WIZ8 0x0052DD20
void W8StartupRuntimeState::SetEventCharacterMask(
    unsigned int event_type, unsigned int party_slot, bool enabled)
{
    unsigned char mask = (unsigned char)(1 << (party_slot & 31));

    if (event_type >= g_first_remapped_event_005ee718) {
        event_type += g_last_event_005ee70c - g_first_remapped_event_005ee718;
    }
    if (!enabled) {
        bytes_68[event_type] &= (unsigned char)~mask;
    }
    else {
        bytes_68[event_type] |= mask;
    }
}

// FUNCTION: WIZ8 0x0052E690
W8StartupStateElement005EE748* Function52E690(
    W8Character* character, int effect, int argument, int value_1,
    unsigned int value_2)
{
    W8StartupStateElement005EE748* entry;

    if (g_settings_6850c8.field_040 == 0 &&
        (effect == g_special_event_0068c50c ||
         effect == g_special_event_0068c568)) {
        return 0;
    }
    if (effect != g_special_event_0068c504 &&
        effect != g_special_event_0068c550 &&
        effect != g_special_event_0068c51c &&
        effect != g_special_event_0068c538 &&
        effect != g_special_event_0068c540 &&
        effect != g_special_event_0068c564) {
        value_2 = value_2 * 70 / 100;
    }
    entry = new W8StartupStateElement005EE748(
        character, effect, argument, value_1, value_2);
    if (entry != 0 && g_startup_runtime_state->QueueEntry(entry) == 0) {
        return 0;
    }
    return entry;
}

/* Remove one queued character event from the owned vector before dispatching
   and deleting it. Event types 14 and 15 also restart the runtime state's
   follow-up clock; bit 1 selects the short interval. */
// FUNCTION: WIZ8 0x0052D8D0
void W8StartupRuntimeState::ProcessOwnedEntry(W8StartupStateElement005EE748* entry)
{
    int index = vector_40.IndexOf(entry);

    if (index >= 0) {
        vector_40.RemoveAt(index);
    }
    if ((value_5c & 1) != 0 && entry->type_08 >= 14 && entry->type_08 < 16) {
        if ((value_5c & 2) == 0) {
            unknown_60 = SetCountdownClock(Random(60000) + 300000);
        }
        else {
            unknown_60 = SetCountdownClock(Random(6000) + 2000);
        }
    }
    entry->Process0052CED0();
    delete entry;
}

/* A character at zero percent hit points may start one of the three recovered
   incapacitation events. Which pair is available is selected by the two data
   flags; successfully queueing the event clears the matching held effect. */
// FUNCTION: WIZ8 0x0052F060
void Function52F060(unsigned int party_slot)
{
    W8Character* character = &g_party_characters[party_slot];
    int effect;

    if ((character->hp_current * 100) / (unsigned int)character->hp_max != 0) {
        return;
    }
    effect = g_effect_005ee594;
    if (g_value_005ed8fc == 0) {
        if (g_flee_hp_fraction_005ed8f8 == 0) {
            return;
        }
        effect = Random(2) == 0 ? g_effect_005ee590 : g_effect_005ee5f8;
    }
    if (effect != -1 &&
        Function52E690(character, effect, 0,
                       g_effect_argument_005ed8c8, g_effect_argument_005ed914) != 0) {
        g_startup_runtime_state->SetEventCharacterMask(effect, party_slot, 1);
    }
}

/* Advance the eight character portrait/voice records. This is the complete
   per-frame state machine: it drains finished owned events, starts the
   incapacitation path when no record is active, advances facing and pose
   clocks, and asks the current screen to redraw a changed slot. */
// FUNCTION: WIZ8 0x0052E750
int Function52E750(void)
{
    unsigned int party_slot;
    int any_active = 0;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        W8MonsterManagerEntry* record = &g_monster_manager_entries[party_slot];
        unsigned char sound_active = 0;

        if (g_party_slot_rows[party_slot].occupied == 0) {
            continue;
        }
        if (record->field_000 != 0) {
            if (record->field_001 == -1) {
                if (record->field_081 == 0) {
                    if (record->field_071 == 0) {
                        Function52F890(party_slot, 0, -1, 0, 1);
                    }
                    else {
                        g_startup_runtime_state->ProcessOwnedEntry(record->field_071);
                    }
                }
            }
            else {
                Function5E2F40(record->field_001, &record->unknown_005[0]);
                sound_active = record->field_015;
            }
        }

        if (record->field_000 == 0) {
            unsigned int scan;
            for (scan = 0; scan < 8; ++scan) {
                if (g_party_slot_rows[scan].occupied != 0 &&
                    g_monster_manager_entries[scan].field_000 != 0) {
                    break;
                }
            }
            if (scan == 8) {
                Function52F060(party_slot);
            }
        }
        else {
            W8Character* character = &g_party_characters[party_slot];
            if ((character->unknown_0b01 > 14 || character->hp_current == 0) &&
                record->field_071 != 0) {
                g_startup_runtime_state->ProcessOwnedEntry(record->field_071);
            }
            if (record->field_000 == 0) {
                unsigned int scan;
                for (scan = 0; scan < 8; ++scan) {
                    if (g_party_slot_rows[scan].occupied != 0 &&
                        g_monster_manager_entries[scan].field_000 != 0) {
                        break;
                    }
                }
                if (scan == 8) {
                    Function52F060(party_slot);
                }
            }
            else {
                any_active = 1;
                if (sound_active == 0) {
                    if (ClockIsTicking(record->field_07d) == 0) {
                        if (record->field_081 < 120) {
                            record->field_075 = record->field_079;
                            record->field_079 = 6;
                            record->field_09a = 1;
                            record->field_081 = 0;
                        }
                        else {
                            int direction =
                                ChooseDifferentMonsterDirection004C2E00(
                                    (short)record->field_079 - 6) + 6;
                            if (g_value_0068c57c <= record->field_113 &&
                                record->field_113 <= g_value_0068c554) {
                                direction = 8;
                            }
                            record->field_075 = record->field_079;
                            record->field_079 = direction;
                            record->field_09a = 1;
                            record->field_07d = SetCountdownClock(120);
                            record->field_081 -= 120;
                        }
                    }
                }
                else {
                    record->field_075 = record->field_079;
                    record->field_079 = 6;
                    record->field_09a = 1;
                }
            }
        }

        if (gXStatus.field_01f != 0 && (party_slot & 1) != 0 &&
            Function56EC90(party_slot) != 0) {
            continue;
        }
        if (record->field_09b == 0 && record->field_0bd == 0 &&
            (g_current_screen_state.id != W8_SCREEN_CHARACTER ||
             record->field_000 != 0)) {
            if (record->field_099 == 0) {
                if (record->field_089 == record->field_08d) {
                    if (record->field_089 == 1 &&
                        ClockIsTicking(record->field_095) == 0) {
                        record->field_099 = 1;
                        record->field_095 =
                            SetCountdownClock(Random(5000) + 5000);
                    }
                }
                else if (ClockIsTicking(record->field_091) == 0) {
                    int pose = record->field_089;
                    record->field_085 = pose;
                    record->field_089 =
                        g_pose_transition_table_0061cb44
                            [pose * 5 + record->field_08d];
                    record->field_099 = 1;
                    record->field_091 = SetCountdownClock(Random(50) + 50);
                }
            }
            else if (ClockIsTicking(record->field_091) == 0) {
                int pose = record->field_089;
                if (pose != 2) {
                    record->field_085 = pose;
                    record->field_089 =
                        g_pose_transition_table_0061cb44[pose * 5 + 2];
                    record->field_099 = 1;
                    record->field_091 = SetCountdownClock(Random(50) + 50);
                }
                if (record->field_089 == 2) {
                    record->field_099 = 0;
                }
            }
            if ((record->field_099 != 0 || record->field_09a != 0) &&
                record->field_0cf == 0) {
                RefreshPartySlotDisplay(party_slot);
            }
        }
    }
    return any_active;
}
