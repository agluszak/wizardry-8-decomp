#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/npc_state.h"
#include "wiz8/record_file_0055a480.h"
#include "wiz8/string_database.h"
#include "wiz8/xstatus.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/screen_state.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/startup_runtime_state.h"
#include "wiz8/local_code/PC_Item.h"
#include "soundman.h"
#include "random.h"
#include "timer.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/Strings.h"
#undef S32
#undef U32
#include "bink.h"
#include "FileMan.h"

extern unsigned char IsSoundPlaying(int sound_handle);
extern unsigned char StopSound(int sound_handle);
extern void QueueGameplayEvent(int event_type, int party_slot);
extern int g_effect_argument_005ed8cc;
unsigned char Function52CFB0(unsigned int party_slot, unsigned int event_type,
                             unsigned int flags);                 /* 0x0052CFB0 */
unsigned char Function52C910(unsigned int event_type);            /* 0x0052C910 */
unsigned char Function52D260(void);                               /* 0x0052D260 */
void Function525110(W8NpcState* npc, int value);                  /* 0x00525110 */
void Function525FA0(unsigned int event_type, unsigned char flag); /* 0x00525FA0 */
void SetFlag68C500(unsigned char value);                          /* 0x0052A1A0 */

#include <stdio.h>
#include <wchar.h>

// GLOBAL: WIZ8 0x0068C578
int g_special_event_0068c578;
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
    0x1380080, 0x1380080, 0x130013, 0x670067, 0xbc00bc, 0x1110111, 1, 3, 3, 4, 5, 3, 2, 3, 3,
    3,         1,         2,        3,        4,        1,         1, 3, 3, 4, 1, 1, 1, 1, 1,
};
// GLOBAL: WIZ8 0x005ED8C8
int g_effect_argument_005ed8c8 = 0;
// GLOBAL: WIZ8 0x005ED8E4
unsigned char g_character_event_flags_mask_005ed8e4 = 16;
// GLOBAL: WIZ8 0x005ED8F8
unsigned int g_flee_hp_fraction_005ed8f8 = 50;
// GLOBAL: WIZ8 0x005ED8FC
unsigned int g_value_005ed8fc = 20;
// GLOBAL: WIZ8 0x005ED8D0
int g_effect_argument_005ed8d0 = 2;
// GLOBAL: WIZ8 0x005ED8D4
int g_effect_argument_005ed8d4 = 1;
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
// GLOBAL: WIZ8 0x005EE710
unsigned int g_remapped_event_count_005ee710 = 31;
// GLOBAL: WIZ8 0x005EE718
unsigned int g_first_remapped_event_005ee718 = 500;

/* 0x005EE004: eight-byte dispatch records indexed by remapped event type.
   TryAdjustQueuedEvent reads field_00; ProcessDeferredCharacterEvents reads
   field_01 before attempting to hand an entry to the owned queue. */
struct W8EventDispatchRecord {
    unsigned char field_00;
    unsigned char field_01;
    unsigned char unknown_02[6];
};
// GLOBAL: WIZ8 0x005EE004
W8EventDispatchRecord g_event_dispatch_records_005ee004[0xb1];

// GLOBAL: WIZ8 0x0068C504
int g_special_event_0068c504;
// GLOBAL: WIZ8 0x0068C508
int g_special_event_0068c508;
// GLOBAL: WIZ8 0x0068C50C
int g_special_event_0068c50c;
// GLOBAL: WIZ8 0x0068C51C
int g_special_event_0068c51c;
// GLOBAL: WIZ8 0x0068C538
int g_special_event_0068c538;
// GLOBAL: WIZ8 0x0068C544
int g_special_event_0068c544;
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
    } else if (g_settings_6850c8.muted_voice_volume != 0xff) {
        g_settings_6850c8.voice_volume = g_settings_6850c8.muted_voice_volume;
        g_settings_6850c8.muted_voice_volume = 0xff;
    }
}

// FUNCTION: WIZ8 0x0052C810
W8StartupStateElement005EE748::W8StartupStateElement005EE748(W8Character* character,
                                                             unsigned int type, int value_0c_arg,
                                                             unsigned int flags, int value_14_arg)
    : handled_00(0), character_04(character), type_08(type), value_0c(value_0c_arg),
      flags_10(flags), value_14(value_14_arg), value_30(0)
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
        value_18 = character->highest_condition;
        break;
    case 7:
        value_18 = 12;
        break;
    case 0x38:
    case 0x55:
        value_18 = character->hp_current;
        value_1c = character->highest_condition;
        break;
    }
}

/* 0x0052D0B0: format one character quote for the given event type into the
   shared wide text buffer. A party member on any screen but the character
   screen takes the text from the NPC bound to its slot; otherwise the type
   selects an entry of the sex/personality/voice quote file, which is then
   wrapped in quotes. Clears the buffer and answers zero when no quote exists. */
// FUNCTION: WIZ8 0x0052D0B0
unsigned char FormatCharacterQuoteText(W8Character* character, unsigned int type,
                                       unsigned int* metadata)
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
    if (character->in_party != 0 && g_current_screen_state.id != W8_SCREEN_CHARACTER) {
        unsigned int slot = CharacterPointerToPartySlot(character);
        npc_index = g_status_685170.buffers.party_rows[slot].animation_0fa;
        has_npc = npc_index != -1;
    }
    if (!has_npc) {
        char gender_code = static_cast<char>(((character->gender != 0) - 1U & 7) + 0x66);
        sprintf(path, "Data\\Quotes\\PCs\\%c_%s%d0.MSG", gender_code,
                g_quote_personality_names_005ed91c[character->personality_0081],
                (character->voice_0085 != 0) + 1);
        if (!FileExists(path)) {
            g_character_text_0068c580[0] = 0;
            return 0;
        }
        GetStringFromStringDatabase(path, type, g_character_text_0068c580, 0, metadata);
        g_character_text_0068c580[wcslen(g_character_text_0068c580) - 1] = 0;
    } else {
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

/* 0x0052D460 proves four equal derived growable-vector instantiations followed
   by a fifth instantiation with a distinct vtable and the tail state below. Element identity and the
   complete lifetime remain tracked by wiz8-bxj; this gives startup the real
   allocation and field shape without inventing semantic names. */

// FUNCTION: WIZ8 0x0052d460
W8StartupRuntimeState::W8StartupRuntimeState()
    : value_50(-1), value_54(-1), value_5c(0), value_64(-1)
{
    bytes_68 = new unsigned char[0xb1];
    memset(bytes_68, 0, 0xb1);
}

// FUNCTION: WIZ8 0x0052d5b0
W8StartupRuntimeState::~W8StartupRuntimeState()
{
    delete[] bytes_68;
}

// FUNCTION: WIZ8 0x0052db80
void W8StartupRuntimeState::ClearOwnedEntries()
{
    W8StartupStateElement005EE748* entry;
    int count;

    count = vector_40.count;
    while (count > 0) {
        entry = vector_40.RemoveAt(0);
        entry->Process0052CED0();
        count = vector_40.count;
    }
    count = vector_30.count;
    while (count > 0) {
        entry = vector_30.RemoveAt(0);
        delete entry;
        count = vector_30.count;
    }
    count = vector_10.count;
    while (count > 0) {
        entry = vector_10.RemoveAt(0);
        delete entry;
        count = vector_10.count;
    }
    count = vector_00.count;
    while (count > 0) {
        entry = vector_00.RemoveAt(0);
        delete entry;
        count = vector_00.count;
    }
}

// FUNCTION: WIZ8 0x0052e3b0
void W8StartupRuntimeState::ProcessNextPendingEntry()
{
    W8StartupStateElement005EE748* entry;

    if (vector_40.count > 0) {
        entry = *vector_40.GetAt(0);
        vector_40.RemoveAt(vector_40.IndexOf(entry));
        if ((value_5c & 1) != 0 && entry->type_08 >= 14 && entry->type_08 < 16) {
            if ((value_5c & 2) != 0) {
                unknown_60 = SetCountdownClock(Random(6000) + 2000);
            } else {
                unknown_60 = SetCountdownClock(Random(60000) + 300000);
            }
        }
        entry->Process0052CED0();
        delete entry;
    }
}

// FUNCTION: WIZ8 0x0052ced0
void W8StartupStateElement005EE748::Process0052CED0()
{
    W8MonsterManagerEntry* slot;
    int party_slot;
    unsigned char sound_was_active;

    party_slot = CharacterPointerToPartySlot(character_04);
    slot = &g_monster_manager_entries[party_slot];
    sound_was_active = slot->field_000;
    slot->field_071 = 0;
    if (sound_was_active != 0) {
        if (IsSoundPlaying(slot->field_001) != 0) {
            handled_00 = 1;
            StopSound(slot->field_001);
        }
        Function52F890(party_slot, 0, -1, 0, 1);
    }
    if (type_08 == 23 || type_08 == 24) {
        if ((flags_10 & 0x40) == 0) {
            if (item_24.item_id == -1) {
                PostCharacterMessage(party_slot, gppStringList[0x1dc4 / 4]);
            } else {
                PostCharacterMessage(party_slot, gppStringList[0x1dc8 / 4],
                                     GetItemDisplayName(&item_24));
            }
        }
    } else if (type_08 == 51) {
        QueueGameplayEvent(30, party_slot);
    }
}

// FUNCTION: WIZ8 0x0052CA60
unsigned char DispatchCharacterEventEntry(W8StartupStateElement005EE748* entry)
{
    unsigned int party_slot;
    unsigned int event_type;
    unsigned int metadata;
    W8Character* character;
    W8MonsterManagerEntry* slot;
    W8PartySlotRow* row;
    int npc_index;
    W8NpcState* npc;
    unsigned char has_quote;

    metadata = 0xffffffff;
    party_slot = CharacterPointerToPartySlot(entry->character_04);
    event_type = entry->type_08;
    if (g_last_event_005ee70c <= event_type) {
        if (event_type < g_first_remapped_event_005ee718) {
            return 0;
        }
        if (g_remapped_event_count_005ee710 + g_first_remapped_event_005ee718 <= event_type) {
            return 0;
        }
    }
    if (entry->character_04 == 0) {
        return 0;
    }
    if ((entry->flags_10 & 4) == 0) {
        if (Function52CFB0(party_slot, event_type, entry->flags_10) == 0 ||
            Function52C910(event_type) == 0) {
            goto finish_without_dispatch;
        }
        event_type = entry->type_08;
        if (event_type != (unsigned int)g_special_event_0068c578 &&
            event_type != (unsigned int)g_special_event_0068c508) {
            character = entry->character_04;
            if (character->condition_turns[W8_CONDITION_SPELLCASTING_BLOCKED] != 0) {
                QueueCharacterEvent(character, g_special_event_0068c508, 0, 1, 0x7f);
                return 0;
            }
            if (character->condition_turns[11] != 0) {
                QueueCharacterEvent(character, g_special_event_0068c578, 0, 1, 0x7f);
                return 0;
            }
        }
    }
    slot = &g_monster_manager_entries[party_slot];
    if (slot->field_000 == 0) {
        if (event_type == 0x33) {
            Function577880(0);
        }
        row = &g_status_685170.buffers.party_rows[party_slot];
        npc_index = row->animation_0fa;
        if (npc_index != -1 && entry->type_08 < 0x93) {
            npc = GetNpcState(npc_index);
            *reinterpret_cast<unsigned int*>(row->unknown_ff) = entry->type_08;
            if (npc == 0) {
                return 1;
            }
            if (npc->name_style == 0x18 && entry->type_08 > 0x8b && entry->type_08 < 0x92 &&
                g_status_685170.current_level != 0) {
                return 0;
            }
            if ((entry->flags_10 & 0x40) != 0) {
                SetFlag68C500(1);
                ReleaseRecordFile0055A0A0(npc->record_file);
                Function524CA0(npc);
            }
            Function525110(npc, 1);
            Function525FA0(entry->type_08, (unsigned char)(entry->flags_10 & 0x40));
            if ((entry->flags_10 & 0x40) != 0) {
                SetFlag68C500(0);
                ReleaseRecordFile0055A0A0(npc->record_file);
                Function524CA0(npc);
            }
            slot->field_071 = entry;
            event_type = entry->type_08;
            if (event_type > 1 && (event_type < 4 || event_type == 0x1c)) {
                g_startup_runtime_state->SetEventCharacterMask(event_type, party_slot, 1);
            }
            if (entry->type_08 != 10) {
                g_startup_runtime_state->value_50 = entry->type_08;
                g_startup_runtime_state->value_54 = party_slot;
            }
            g_startup_runtime_state->unknown_58 = SetCountdownClock(5000);
            return 1;
        }
        has_quote = FormatCharacterQuoteText(entry->character_04, entry->type_08, &metadata);
        if (Function52D260() != 0) {
            event_type = entry->type_08;
            if (event_type > 1 && (event_type < 4 || event_type == 0x1c)) {
                g_startup_runtime_state->SetEventCharacterMask(event_type, party_slot, 1);
            }
            if (entry->type_08 != 10) {
                g_startup_runtime_state->value_50 = entry->type_08;
                g_startup_runtime_state->value_54 = party_slot;
            }
            g_startup_runtime_state->unknown_58 = SetCountdownClock(5000);
            event_type = entry->type_08;
            if (event_type < 0x92) {
                Function52F890(party_slot, 1, event_type, (int)g_character_text_0068c580,
                               1 - (((unsigned char)entry->flags_10 &
                                     g_character_event_flags_mask_005ed8e4) != 0));
                slot->field_071 = entry;
                *reinterpret_cast<unsigned int*>(row->unknown_ff) = entry->type_08;
                *reinterpret_cast<unsigned int*>(
                    reinterpret_cast<unsigned char*>(&slot->field_113) + 1) = entry->type_08;
                return 1;
            }
            Function52F890(party_slot, 1, event_type, 0, 1);
            *reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(&slot->field_113) +
                                             1) = entry->type_08;
            return 1;
        }
        entry->Process0052CED0();
        if (has_quote == 0) {
            return 0;
        }
        event_type = entry->type_08;
        if (event_type > 1 && (event_type < 4 || event_type == 0x1c)) {
            g_startup_runtime_state->SetEventCharacterMask(event_type, party_slot, 1);
        }
        if (entry->type_08 != 10) {
            g_startup_runtime_state->value_50 = entry->type_08;
            g_startup_runtime_state->value_54 = party_slot;
        }
        g_startup_runtime_state->unknown_58 = SetCountdownClock(5000);
        if ((g_startup_runtime_state->value_5c & 1) == 0) {
            return 0;
        }
        if (entry->type_08 < 14) {
            return 0;
        }
        if (entry->type_08 >= 16) {
            return 0;
        }
        if ((g_startup_runtime_state->value_5c & 2) != 0) {
            g_startup_runtime_state->unknown_60 = SetCountdownClock(Random(6000) + 2000);
            return 0;
        }
        g_startup_runtime_state->unknown_60 = SetCountdownClock(Random(60000) + 300000);
        return 0;
    }
finish_without_dispatch:
    entry->Process0052CED0();
    return 0;
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
    } else {
        duration = Random(6000) + 2000;
    }
    unknown_60 = SetCountdownClock(duration);
}

// FUNCTION: WIZ8 0x0052D610
int W8StartupRuntimeState::QueueEntry(W8StartupStateElement005EE748* entry)
{
    unsigned int party_slot;

    party_slot = CharacterPointerToPartySlot(entry->character_04);
    if (HasEventCharacter(entry->type_08, party_slot)) {
        delete entry;
        return 0;
    }
    if (g_status_685170.flag_2497 != 0) {
        delete entry;
        return 0;
    }
    if (entry->type_08 > 0x91 && (entry->flags_10 & 0x20) == 0) {
        W8MonsterManagerEntry* slot = &g_monster_manager_entries[party_slot];
        if (slot->field_071 != 0) {
            vector_40.Remove(slot->field_071);
            RestartFollowUpClock(slot->field_071);
            slot->field_071->Process0052CED0();
            delete slot->field_071;
        }
        DispatchCharacterEventEntry(entry);
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
    if ((Function525DF0(0) != 0 || Function525DD0() != 0) && (entry->flags_10 & 8) == 0) {
        vector_30.Add(entry);
        return 1;
    }
    vector_10.Add(entry);
    return 1;
}

// FUNCTION: WIZ8 0x0052DD20
void W8StartupRuntimeState::SetEventCharacterMask(unsigned int event_type, unsigned int party_slot,
                                                  bool enabled)
{
    unsigned char mask = (unsigned char)(1 << (party_slot & 31));

    if (event_type >= g_first_remapped_event_005ee718) {
        event_type += g_last_event_005ee70c - g_first_remapped_event_005ee718;
    }
    if (!enabled) {
        bytes_68[event_type] &= (unsigned char)~mask;
    } else {
        bytes_68[event_type] |= mask;
    }
}

// FUNCTION: WIZ8 0x0052DD90
bool W8StartupRuntimeState::HasEventCharacter(unsigned int event_type, unsigned int party_slot)
{
    if (event_type >= g_first_remapped_event_005ee718) {
        event_type += g_last_event_005ee70c - g_first_remapped_event_005ee718;
    }

    unsigned char mask = (unsigned char)(1 << (party_slot & 31));

    return (bytes_68[event_type] & mask) != 0;
}

// FUNCTION: WIZ8 0x0052DC80
unsigned char W8StartupRuntimeState::TryAdjustQueuedEvent(W8StartupStateElement005EE748* entry)
{
    if (entry == 0 || value_50 == -1 || entry->type_08 != (unsigned int)value_50) {
        return 1;
    }

    unsigned int party_slot = CharacterPointerToPartySlot(entry->character_04);
    if (party_slot == (unsigned int)value_54) {
        return 1;
    }

    if (ClockIsTicking(unknown_58) == 0) {
        value_50 = -1;
        value_54 = -1;
        return 1;
    }

    unsigned int event_type = entry->type_08;
    if (event_type > g_last_event_005ee70c) {
        return 1;
    }

    unsigned int event_index = event_type;
    if (event_type >= g_first_remapped_event_005ee718) {
        event_index = (g_last_event_005ee70c - g_first_remapped_event_005ee718) + event_type;
    }
    if (g_event_dispatch_records_005ee004[event_index].field_00 == 0) {
        return 1;
    }

    *reinterpret_cast<unsigned int*>(entry->unknown_20) =
        event_type; /* reinterpret-ok: retail stores the pending type at +0x20 */
    if (event_type == 4) {
        return 0;
    }
    entry->type_08 = 10;
    return 1;
}

static unsigned int RemapEventIndex(unsigned int event_type)
{
    if (event_type >= g_first_remapped_event_005ee718) {
        return (g_last_event_005ee70c - g_first_remapped_event_005ee718) + event_type;
    }
    return event_type;
}

// FUNCTION: WIZ8 0x0052DDD0
void W8StartupRuntimeState::ProcessDeferredCharacterEvents()
{
    W8StartupStateElement005EE748* entry;
    W8StartupStateElement005EE748* baseline;
    int index;
    int scan;
    int conflict_count;
    int* conflict_indices;
    int remaining_conflicts;
    unsigned int event_type;
    unsigned int party_slot;

    if (gXStatus.fSurprisePossible != 0) {
        return;
    }

    if (vector_30.count > 0 && Function525DF0(0) == 0 && Function525DD0() == 0) {
        for (index = 0; index < vector_30.count; ++index) {
            QueueEntry(vector_30.data[index]);
        }
        vector_30.count = 0;
    }

    if (vector_10.count != 0) {
        conflict_count = 1;
        baseline = vector_10.data[0];
        conflict_indices = new int[vector_10.count];
        conflict_indices[0] = 0;
        for (index = 1; index < vector_10.count; ++index) {
            entry = vector_10.data[index];
            event_type = entry->type_08;
            if (event_type == baseline->type_08 && entry->character_04 != baseline->character_04 &&
                event_type != 0x2a && event_type != 0x24) {
                conflict_indices[conflict_count] = index;
                ++conflict_count;
            }
        }

        if (conflict_count > 3) {
            remaining_conflicts = conflict_count;
            while (remaining_conflicts > 3) {
                int pick = Random(remaining_conflicts);
                if (conflict_indices[pick] != -1) {
                    conflict_indices[pick] = -1;
                    --remaining_conflicts;
                }
            }
            for (index = conflict_count - 1; index >= 0; --index) {
                if (conflict_indices[index] != -1) {
                    delete vector_10.RemoveAt(conflict_indices[index]);
                }
            }
        }
        delete[] conflict_indices;
    }

    if (vector_10.count == 0) {
        UpdateNpcDialogueVoiceAndCursor();
        if (PartyPortraitEventsIdle() != 0) {
            ProcessNpcScriptingFrame();
        }
        return;
    }

    index = 0;
    while (index < vector_10.count) {
        entry = vector_10.data[index];
        event_type = entry->type_08;
        if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME &&
            event_type <= g_last_event_005ee70c) {
            if (g_event_dispatch_records_005ee004[RemapEventIndex(event_type)].field_01 != 0) {
                ++index;
                continue;
            }
        }

        if (entry->character_04->in_party != 0) {
            party_slot = CharacterPointerToPartySlot(entry->character_04);
            if (!HasEventCharacter(event_type, party_slot)) {
                if (PartyPortraitEventsIdle() == 0) {
                    return;
                }
                if (entry->value_30 != 0 &&
                    GetTickCount() - entry->clock_34 <= (unsigned int)entry->value_30) {
                    return;
                }
                vector_10.RemoveAt(index);
                if (TryAdjustQueuedEvent(entry) == 0) {
                    delete entry;
                    return;
                }
                if (DispatchCharacterEventEntry(entry) == 0) {
                    return;
                }
                vector_40.Add(entry);
                return;
            }
        }

        for (scan = 0; scan < vector_10.count; ++scan) {
            if (vector_10.data[scan] == entry) {
                vector_10.RemoveAt(scan);
                return;
            }
        }
        return;
    }
}

// FUNCTION: WIZ8 0x0052E690
W8StartupStateElement005EE748* QueueCharacterEvent(W8Character* character, int effect, int argument,
                                                   int value_1, unsigned int value_2)
{
    W8StartupStateElement005EE748* entry;

    if (g_settings_6850c8.pc_confirmations == 0 &&
        (effect == g_special_event_0068c50c || effect == g_special_event_0068c568)) {
        return 0;
    }
    if (effect != g_special_event_0068c504 && effect != g_special_event_0068c550 &&
        effect != g_special_event_0068c51c && effect != g_special_event_0068c538 &&
        effect != g_special_event_0068c540 && effect != g_special_event_0068c564) {
        value_2 = value_2 * 70 / 100;
    }
    entry = new W8StartupStateElement005EE748(character, effect, argument, value_1, value_2);
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
        } else {
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
void MaybeStartIncapacitationEvent(unsigned int party_slot)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
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
    if (effect != -1 && QueueCharacterEvent(character, effect, 0, g_effect_argument_005ed8c8,
                                            g_effect_argument_005ed914) != 0) {
        g_startup_runtime_state->SetEventCharacterMask(effect, party_slot, 1);
    }
}

/* Queue a low-HP flee or incapacitation event when the character is still
   standing, then always roll one of three ambient follow-up events. */
// FUNCTION: WIZ8 0x0052F2C0
void QueueDamageReactionEvents(W8Character* character)
{
    unsigned int hp_percent;
    unsigned int party_slot;
    bool has_incapacitation_event;
    bool has_flee_event;
    W8StartupStateElement005EE748* entry;
    int effect;
    int follow_up_events[3];

    hp_percent = (character->hp_current * 100) / (unsigned int)character->hp_max;
    party_slot = CharacterPointerToPartySlot(character);
    has_incapacitation_event =
        g_startup_runtime_state->HasEventCharacter(g_effect_005ee594, party_slot);
    has_flee_event = g_startup_runtime_state->HasEventCharacter(g_effect_005ee590, party_slot);
    g_startup_runtime_state->HasEventCharacter(g_effect_005ee5f8, party_slot);
    if (character->highest_condition != 0xf && character->highest_condition != 0x11) {
        if (g_value_005ed8fc <= hp_percent || has_incapacitation_event) {
            if (g_flee_hp_fraction_005ed8f8 <= hp_percent) {
                goto queue_follow_up_event;
            }
            if (Random(2) != 0 || has_flee_event) {
                effect = g_effect_005ee5f8;
            } else {
                effect = g_effect_005ee590;
            }
            entry = QueueCharacterEvent(character, effect, g_effect_argument_005ed8d4,
                                        g_effect_argument_005ed8d0, g_effect_argument_005ed914);
        } else {
            entry = QueueCharacterEvent(character, g_effect_005ee594, g_effect_argument_005ed8d4,
                                        g_effect_argument_005ed8d0, g_effect_argument_005ed914);
        }
        if (entry != 0) {
            entry->value_30 = 0x5dc;
            entry->clock_34 = GetTickCount();
        }
    }
queue_follow_up_event:
    follow_up_events[0] = g_special_event_0068c544;
    follow_up_events[1] = g_special_event_0068c550;
    follow_up_events[2] = g_special_event_0068c51c;
    QueueCharacterEvent(character, follow_up_events[Random(3)], g_effect_argument_005ed8d4,
                        g_effect_argument_005ed8cc, g_effect_argument_005ed914);
}

/* Occupied slots with field_000 set still have a portrait/voice record in
   flight; trap-trigger follow-up waits until none of those are active. */
// FUNCTION: WIZ8 0x0052E590
unsigned char PartyPortraitEventsIdle(void)
{
    W8PartySlotRow* row = g_status_685170.buffers.party_rows;
    const W8MonsterManagerEntry* current;

    for (current = g_monster_manager_entries; current < &g_monster_manager_entries[8];
         ++current, ++row) {
        if (row->occupied != 0 && current->field_000 != 0) {
            return 0;
        }
    }
    return 1;
}

/* Advance the eight character portrait/voice records. This is the complete
   per-frame state machine: it drains finished owned events, starts the
   incapacitation path when no record is active, advances facing and pose
   clocks, and asks the current screen to redraw a changed slot. */
// FUNCTION: WIZ8 0x0052E750
int UpdateCharacterEventState(void)
{
    unsigned int party_slot;
    int any_active = 0;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        W8MonsterManagerEntry* record = &g_monster_manager_entries[party_slot];
        unsigned char sound_active = 0;

        if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
            continue;
        }
        if (record->field_000 != 0) {
            if (record->field_001 == -1) {
                if (record->field_081 == 0) {
                    if (record->field_071 == 0) {
                        Function52F890(party_slot, 0, -1, 0, 1);
                    } else {
                        g_startup_runtime_state->ProcessOwnedEntry(record->field_071);
                    }
                }
            } else {
                Function5E2F40(record->field_001, &record->unknown_005[0]);
                sound_active = record->field_015;
            }
        }

        if (record->field_000 == 0) {
            unsigned int scan;
            for (scan = 0; scan < 8; ++scan) {
                if (g_status_685170.buffers.party_rows[scan].occupied != 0 &&
                    g_monster_manager_entries[scan].field_000 != 0) {
                    break;
                }
            }
            if (scan == 8) {
                MaybeStartIncapacitationEvent(party_slot);
            }
        } else {
            W8Character* character = &g_status_685170.buffers.characters[party_slot];
            if ((character->highest_condition > 14 || character->hp_current == 0) &&
                record->field_071 != 0) {
                g_startup_runtime_state->ProcessOwnedEntry(record->field_071);
            }
            if (record->field_000 == 0) {
                unsigned int scan;
                for (scan = 0; scan < 8; ++scan) {
                    if (g_status_685170.buffers.party_rows[scan].occupied != 0 &&
                        g_monster_manager_entries[scan].field_000 != 0) {
                        break;
                    }
                }
                if (scan == 8) {
                    MaybeStartIncapacitationEvent(party_slot);
                }
            } else {
                any_active = 1;
                if (sound_active == 0) {
                    if (ClockIsTicking(record->field_07d) == 0) {
                        if (record->field_081 < 120) {
                            record->field_075 = record->field_079;
                            record->field_079 = 6;
                            record->field_09a = 1;
                            record->field_081 = 0;
                        } else {
                            int direction = ChooseDifferentMonsterDirection004C2E00(
                                                (short)record->field_079 - 6) +
                                            6;
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
                } else {
                    record->field_075 = record->field_079;
                    record->field_079 = 6;
                    record->field_09a = 1;
                }
            }
        }

        if (gXStatus.fNpcDialogueMode != 0 && (party_slot & 1) != 0 &&
            Function56EC90(party_slot) != 0) {
            continue;
        }
        if (record->field_09b == 0 && record->field_0bd == 0 &&
            (g_current_screen_state.id != W8_SCREEN_CHARACTER || record->field_000 != 0)) {
            if (record->field_099 == 0) {
                if (record->field_089 == record->field_08d) {
                    if (record->field_089 == 1 && ClockIsTicking(record->field_095) == 0) {
                        record->field_099 = 1;
                        record->field_095 = SetCountdownClock(Random(5000) + 5000);
                    }
                } else if (ClockIsTicking(record->field_091) == 0) {
                    int pose = record->field_089;
                    record->field_085 = pose;
                    record->field_089 =
                        g_pose_transition_table_0061cb44[pose * 5 + record->field_08d];
                    record->field_099 = 1;
                    record->field_091 = SetCountdownClock(Random(50) + 50);
                }
            } else if (ClockIsTicking(record->field_091) == 0) {
                int pose = record->field_089;
                if (pose != 2) {
                    record->field_085 = pose;
                    record->field_089 = g_pose_transition_table_0061cb44[pose * 5 + 2];
                    record->field_099 = 1;
                    record->field_091 = SetCountdownClock(Random(50) + 50);
                }
                if (record->field_089 == 2) {
                    record->field_099 = 0;
                }
            }
            if ((record->field_099 != 0 || record->field_09a != 0) && record->field_0cf == 0) {
                RefreshPartySlotDisplay(party_slot);
            }
        }
    }
    return any_active;
}
