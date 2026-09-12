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
#include "wiz8/local_code/PC_Item.h"
#include "soundman.h"
#include "random.h"
#include "timer.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/notices.h"
#include "wiz8/regions.h"
#include "sgp.h"
#undef S32
#undef U32
#include "bink.h"
#include "FileMan.h"

extern unsigned char IsSoundPlaying(int sound_handle);
extern unsigned char StopSound(int sound_handle);
extern void QueueGameplayEvent(int event_type, int party_slot);
extern void SetFlag68C500(unsigned char value);
extern void FormatNpcVoiceSoundPath(W8NpcState* npc, char* output);
extern void ReloadNpcScriptResources(W8NpcState* npc);
extern void BeginNpcScriptDialogue(W8NpcState* npc, unsigned char value);
extern void RunNpcScriptLine(int event_type, unsigned char value);
extern int ComputePortraitMessageDuration(wchar_t* text);
extern void Function5E2D10(char* path, int* gap_data);
extern int LayoutPortraitQuoteBubble(int surface_handle, unsigned char param_2,
                                     unsigned char param_3, const wchar_t* text,
                                     unsigned int max_width, int param_6, int param_7, int param_8,
                                     unsigned short* out_width, unsigned short* out_height,
                                     unsigned int font_palette);
extern unsigned char ReleasePortraitQuoteBubble(int quote_handle);
extern void RefreshSelectedPartyPortrait(unsigned int party_slot);
extern const wchar_t* FormatPortraitQuoteNoticeText(int quote_handle, int channel, int scroll_range,
                                                    const void* unused);

#include <stdio.h>
#include <wchar.h>
#include <windows.h>

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
// GLOBAL: WIZ8 0x005EE710
unsigned int g_remapped_event_span_005ee710 = 31;
// GLOBAL: WIZ8 0x005ED8E4
unsigned int g_event_flags_mask_005ed8e4 = 16;

// GLOBAL: WIZ8 0x005ee000
const W8CharacterEventDescriptor g_character_event_descriptors_005ee000[0x92] = {
    {1, 0, 0}, {5, 0, 1}, {4, 0, 1}, {2, 0, 1}, {3, 1, 1}, {3, 1, 1}, {3, 1, 1}, {3, 1, 1},
    {1, 1, 0}, {1, 1, 1}, {1, 0, 1}, {5, 0, 1}, {1, 0, 1}, {4, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {4, 0, 1}, {4, 0, 1}, {4, 0, 1}, {4, 0, 1}, {4, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {4, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {5, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {5, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {5, 0, 1}, {1, 0, 1}, {1, 0, 1}, {5, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 0}, {4, 0, 1}, {5, 0, 0},
    {5, 0, 1}, {1, 0, 1}, {5, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 0}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1},
    {1, 0, 1}, {1, 0, 1},
};

// GLOBAL: WIZ8 0x0068C508
int g_special_event_0068c508;
// GLOBAL: WIZ8 0x0068C544
int g_special_event_0068c544;
// GLOBAL: WIZ8 0x0068C578
int g_special_event_0068c578;
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

/* 0x0052C560: sound end-of-stream callback; removes the owned entry and runs
   Process0052CED0 when playback finishes. */
static void CharacterEventSoundEndCallback(void* callback_data)
{
    W8StartupStateElement005EE748* entry =
        static_cast<W8StartupStateElement005EE748*>(callback_data);
    W8StartupRuntimeState* runtime;
    int index;

    if (entry->handled_00 != 0) {
        return;
    }
    runtime = gXStatus.pStartupRuntime;
    if (runtime == 0) {
        return;
    }
    index = runtime->vector_40.IndexOf(entry);
    if (index >= 0) {
        runtime->vector_40.RemoveAt(index);
    }
    if ((runtime->value_5c & 1) != 0 && entry->type_08 >= 14 && entry->type_08 < 16) {
        if ((runtime->value_5c & 2) != 0) {
            runtime->unknown_60 = SetCountdownClock(Random(6000) + 2000);
        } else {
            runtime->unknown_60 = SetCountdownClock(Random(60000) + 300000);
        }
    }
    entry->Process0052CED0();
    delete entry;
}

// FUNCTION: WIZ8 0x0052C910
unsigned char W8StartupStateElement005EE748::CharacterEventConditionMet(unsigned int event_type)
{
    W8Character* character;

    do {
        character = reinterpret_cast<W8Character*>(event_type - 2);
        switch (event_type) {
        case 2:
        case 3:
            character = character_04;
            if ((unsigned int)value_18 <= character->hp_current) {
                return 0;
            }
            return 1;
        case 5:
        case 6:
        case 7:
        case 9:
            character = character_04;
            if (character->highest_condition != (unsigned int)value_18) {
                return 0;
            }
            return 1;
        case 10:
            event_type = *reinterpret_cast<unsigned int*>(&unknown_20[0]);
            break;
        case 14:
        case 15:
        case 35:
            if (gXStatus.fCombatMode != 0) {
                return 0;
            }
            return 1;
        case 43:
        case 44:
        case 45:
            character = character_04;
            if (character->gender == W8_GENDER_FEMALE) {
                return 0;
            }
            return 1;
        case 56:
            character = character_04;
            if (character->hp_current < (unsigned int)value_18 ||
                character->highest_condition < (unsigned int)value_1c) {
                return 0;
            }
            return 1;
        case 84:
            character = character_04;
            if ((unsigned int)value_18 <= character->highest_condition) {
                return 0;
            }
            return 1;
        case 85:
            character = character_04;
            if (character->hp_current < (unsigned int)value_18 ||
                character->highest_condition != 0) {
                QueueCharacterEvent(character, 84, 0, 1, 0x7f);
                return 0;
            }
            return 1;
        default:
            return 1;
        }
    } while (1);
}

// FUNCTION: WIZ8 0x0052CFB0
static unsigned char CanDispatchCharacterEvent(unsigned int party_slot, unsigned int event_type,
                                               unsigned int flags)
{
    W8Character* character;

    (void)flags;
    unsigned int mapped_event_type;
    unsigned char slot_mask;

    if (party_slot >= 8) {
        return 0;
    }
    if (event_type >= g_last_event_005ee70c) {
        if (event_type < g_first_remapped_event_005ee718) {
            return 0;
        }
        if (event_type >= g_remapped_event_span_005ee710 + g_first_remapped_event_005ee718) {
            return 0;
        }
    }
    if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
        return 0;
    }
    character = &g_status_685170.buffers.characters[party_slot];
    if (character->highest_condition > 14) {
        if (event_type == (unsigned int)g_special_event_0068c538 ||
            event_type == (unsigned int)g_special_event_0068c540 || g_special_event_0068c564 != 0) {
            if (character->condition_turns[17] != 0) {
                return 0;
            }
            if (character->condition_turns[19] != 0) {
                return 0;
            }
        } else {
            if (character->highest_condition != 0x11 && character->highest_condition != 0xf) {
                return 0;
            }
            if (event_type != (unsigned int)g_special_event_0068c544 &&
                event_type != (unsigned int)g_special_event_0068c550 &&
                event_type != (unsigned int)g_special_event_0068c51c) {
                return 0;
            }
        }
    }
    if (character->hp_current != 0 || event_type == (unsigned int)g_special_event_0068c538) {
        mapped_event_type = event_type;
        if (mapped_event_type >= g_first_remapped_event_005ee718) {
            mapped_event_type += g_last_event_005ee70c - g_first_remapped_event_005ee718;
        }
        slot_mask = (unsigned char)(1 << (party_slot & 31));
        return (gXStatus.pStartupRuntime->bytes_68[mapped_event_type] & slot_mask) == 0;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0052D260
unsigned char W8StartupStateElement005EE748::PlayEventSound()
{
    unsigned int party_slot;
    W8Character* character;
    unsigned int sound_event;
    int npc_index;
    char voice_stem[20];
    char sound_path[80];
    char npc_sound_name[128];
    SOUNDPARMS sound_parms;
    unsigned int sound_handle;
    unsigned int total_ms;
    unsigned int current_ms;
    W8MonsterManagerEntry* record;
    W8NpcState* npc;

    party_slot = CharacterPointerToPartySlot(character_04);
    character = character_04;
    sound_event = type_08;
    if (character->condition_turns[8] != 0) {
        sound_event = g_special_event_0068c508;
    }
    npc_index = g_status_685170.buffers.party_rows[party_slot].animation_0fa;
    if (npc_index == -1 || g_status_685170.game_started == 0 ||
        g_current_screen_state.id == W8_SCREEN_CHARACTER) {
        char gender_code = static_cast<char>(((character->gender != 0) - 1U & 7) + 0x66);
        sprintf(voice_stem, "%c_%s%d0", gender_code,
                g_quote_personality_names_005ed91c[character->personality_0081],
                (character->voice_0085 != 0) + 1);
        sprintf(sound_path, "Data\\Sound\\PCs\\%s\\%s_%03d.wav", voice_stem, voice_stem,
                sound_event);
    } else {
        npc = GetNpcState(npc_index);
        if (npc != 0) {
            FormatNpcVoiceSoundPath(npc, npc_sound_name);
            sprintf(sound_path, "Data\\Sound\\PCs\\%s\\%s_%03d.wav", npc_sound_name, npc_sound_name,
                    sound_event);
        }
    }
    memset(&sound_parms, 0xff, sizeof(sound_parms));
    sound_parms.uiVolume = (value_14 * (g_settings_6850c8.voice_volume & 0xff)) / 0x7f;
    sound_parms.EOSCallback = CharacterEventSoundEndCallback;
    sound_parms.pCallbackData = this;
    sound_handle = SoundPlay(sound_path, &sound_parms);
    record = &gXStatus.monster_manager_entries[party_slot];
    record->field_001 = sound_handle;
    if (sound_handle == 0xffffffff) {
        if (type_08 > 0x91) {
            static const wchar_t kFallbackVoiceText[] = L"Ouch play this sound";
            record->field_081 =
                ComputePortraitMessageDuration(const_cast<wchar_t*>(kFallbackVoiceText));
        } else {
            record->field_081 = ComputePortraitMessageDuration(g_character_text_0068c580);
        }
        return 1;
    }
    SoundGetMilliSecondPosition(sound_handle, &total_ms, &current_ms);
    record->field_081 = total_ms;
    Function5E2D10(
        sound_path,
        reinterpret_cast<int*>(
            &record->unknown_005[0])); // reinterpret-ok: retail gap list storage in portrait record
    return 1;
}

static void RecordDispatchedCharacterEvent(unsigned int party_slot, unsigned int event_type)
{
    W8StartupRuntimeState* runtime = gXStatus.pStartupRuntime;
    unsigned int mapped_event_type = event_type;

    if (event_type > 1 && (event_type < 4 || event_type == 0x1c)) {
        if (mapped_event_type >= g_first_remapped_event_005ee718) {
            mapped_event_type += g_last_event_005ee70c - g_first_remapped_event_005ee718;
        }
        runtime->bytes_68[mapped_event_type] |= (unsigned char)(1 << (party_slot & 31));
    }
    if (event_type != 10) {
        runtime->value_50 = event_type;
        runtime->value_54 = party_slot;
    }
    runtime->unknown_58 = SetCountdownClock(5000);
}

/* 0x0052CA60: dispatch one queued character event through the portrait/voice
   path or the NPC-quote path, recording follow-up state when it succeeds. */
// FUNCTION: WIZ8 0x0052CA60
unsigned char DispatchQueuedCharacterEvent(W8StartupStateElement005EE748* entry)
{
    W8Character* character;
    unsigned int party_slot;
    unsigned int event_type;
    int npc_index;
    W8NpcState* npc;
    W8PartySlotRow* party_row;
    W8MonsterManagerEntry* record;
    unsigned char has_quote;
    unsigned int quote_metadata;
    unsigned char voice_started;

    party_slot = CharacterPointerToPartySlot(entry->character_04);
    event_type = entry->type_08;
    if (g_last_event_005ee70c <= event_type) {
        if (event_type < g_first_remapped_event_005ee718) {
            return 0;
        }
        if (g_remapped_event_span_005ee710 + g_first_remapped_event_005ee718 <= event_type) {
            return 0;
        }
    }
    if (entry->character_04 == 0) {
        return 0;
    }
    if ((entry->flags_10 & 4) == 0) {
        if (CanDispatchCharacterEvent(party_slot, event_type, entry->flags_10) == 0 ||
            entry->CharacterEventConditionMet(event_type) == 0) {
            entry->Process0052CED0();
            return 0;
        }
        event_type = entry->type_08;
        if (event_type != (unsigned int)g_special_event_0068c578 &&
            event_type != (unsigned int)g_special_event_0068c508) {
            character = entry->character_04;
            if (character->condition_turns[8] != 0) {
                QueueCharacterEvent(character, g_special_event_0068c508, 0, 1, 0x7f);
                return 0;
            }
            if (character->condition_turns[11] != 0) {
                QueueCharacterEvent(character, g_special_event_0068c578, 0, 1, 0x7f);
                return 0;
            }
        }
    }
    record = &gXStatus.monster_manager_entries[party_slot];
    if (record->field_000 != 0) {
        entry->Process0052CED0();
        return 0;
    }
    if (event_type == 51) {
        SetNpcDialoguePanelVisible(0);
    }
    party_row = &g_status_685170.buffers.party_rows[party_slot];
    npc_index = party_row->animation_0fa;
    if (npc_index != -1 && event_type < 0x93) {
        *reinterpret_cast<unsigned int*>(party_row->unknown_ff) =
            event_type; // reinterpret-ok: party-row event-type dword at +0xff
        npc = GetNpcState(npc_index);
        if (npc == 0) {
            return 1;
        }
        if (npc->name_style == 24 && event_type > 0x8b && event_type < 0x92 &&
            g_status_685170.current_level != 0) {
            return 0;
        }
        if ((entry->flags_10 & 0x40) != 0) {
            SetFlag68C500(1);
            ReleaseRecordFile0055A0A0(npc->record_file);
            ReloadNpcScriptResources(npc);
        }
        BeginNpcScriptDialogue(npc, 1);
        RunNpcScriptLine(event_type, (entry->flags_10 & 0x40) != 0);
        if ((entry->flags_10 & 0x40) != 0) {
            SetFlag68C500(0);
            ReleaseRecordFile0055A0A0(npc->record_file);
            ReloadNpcScriptResources(npc);
        }
        record->field_071 = entry;
        RecordDispatchedCharacterEvent(party_slot, entry->type_08);
        return 1;
    }
    quote_metadata = 0xffffffff;
    has_quote = FormatCharacterQuoteText(entry->character_04, entry->type_08, &quote_metadata);
    voice_started = entry->PlayEventSound();
    if (voice_started != 0) {
        RecordDispatchedCharacterEvent(party_slot, entry->type_08);
        event_type = entry->type_08;
        if (event_type < 0x92) {
            SetPartyPortraitEventState(party_slot, 1, event_type, g_character_text_0068c580,
                                       1 - ((entry->flags_10 & g_event_flags_mask_005ed8e4) != 0));
            record->field_071 = entry;
            *reinterpret_cast<unsigned int*>(party_row->unknown_ff) =
                event_type; // reinterpret-ok: party-row event-type dword at +0xff
            *reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(&record->field_113) +
                                             1) =
                event_type; // reinterpret-ok: retail stores beside portrait-range dword
            return 1;
        }
        SetPartyPortraitEventState(party_slot, 1, event_type, 0, 1);
        *reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(&record->field_113) + 1) =
            event_type; // reinterpret-ok: retail stores beside portrait-range dword
        return 1;
    }
    entry->Process0052CED0();
    if (has_quote == 0) {
        return 0;
    }
    RecordDispatchedCharacterEvent(party_slot, entry->type_08);
    if ((gXStatus.pStartupRuntime->value_5c & 1) == 0) {
        return 0;
    }
    if (entry->type_08 < 14) {
        return 0;
    }
    if (entry->type_08 > 15) {
        return 0;
    }
    if ((gXStatus.pStartupRuntime->value_5c & 2) != 0) {
        gXStatus.pStartupRuntime->unknown_60 = SetCountdownClock(Random(6000) + 2000);
    } else {
        gXStatus.pStartupRuntime->unknown_60 = SetCountdownClock(Random(60000) + 300000);
    }
    return 0;
}

/* 0x0052F890: turn a party-slot portrait/voice event on or off, optionally
   laying out the quote bubble and posting subtitle notices when one ends. */
// FUNCTION: WIZ8 0x0052F890
void SetPartyPortraitEventState(unsigned int party_slot, unsigned char active,
                                unsigned int event_type, const void* quote_text, int show_quote)
{
    // clang-format off
    W8MonsterManagerEntry* record = &gXStatus.monster_manager_entries[party_slot];
    unsigned char* bubble = record->unknown_016;
    char* character_bytes = reinterpret_cast<char*>(g_status_685170.buffers.characters); // reinterpret-ok: serialized party-character stride base
    int* quote_handle = reinterpret_cast<int*>(bubble + 3); // reinterpret-ok: quote-bubble handle in portrait record
    short* bubble_x = reinterpret_cast<short*>(bubble + 7); // reinterpret-ok: quote-bubble x in portrait record
    short* bubble_y = reinterpret_cast<short*>(bubble + 9); // reinterpret-ok: quote-bubble y in portrait record
    short* bubble_width = reinterpret_cast<short*>(bubble + 0xb); // reinterpret-ok: quote-bubble width in portrait record
    short* bubble_height = reinterpret_cast<short*>(bubble + 0xd); // reinterpret-ok: quote-bubble height in portrait record
    char* portrait_quote_tables = reinterpret_cast<char*>(g_pose_transition_table_0061cb44); // reinterpret-ok: portrait quote tables precede pose transitions

    if (record->field_000 == active) {
        return;
    }

    if (active == 0) {
        record->field_000 = 0;
        if (gfCapturingVideo != 0) {
            return;
        }
        record->field_075 = record->field_079;
        record->field_079 = 6;
        record->field_09b = 1;
        int pc_slot = RPCPtrToPCSlot(record);
        record->field_099 = 0;
        unsigned int highest_condition = *reinterpret_cast<unsigned int*>(character_bytes + pc_slot * W8_CHARACTER_SERIALIZED_SIZE + 0xb01); // reinterpret-ok: serialized character highest_condition
        if (highest_condition < 0xf && gXStatus.fSurprisePossible == 0) {
            if (record->field_08d != 1) {
                record->field_08d = 1;
            }
        } else {
            record->field_08d = 2;
        }
        if (*quote_handle == -1) {
            record->field_000 = active;
            return;
        }
        unsigned int stored_event = *reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(&record->field_113) + 1); // reinterpret-ok: retail stores beside portrait-range dword
        unsigned int mapped_event = stored_event;
        if ((int)g_last_event_005ee70c < (int)mapped_event) {
        show_deactivate_quote:
            wchar_t formatted[100];
            const wchar_t* character_name = reinterpret_cast<const wchar_t*>(character_bytes + party_slot * W8_CHARACTER_SERIALIZED_SIZE + 5); // reinterpret-ok: serialized character name at retail offset
            swprintf(formatted, L"%s", character_name);
            int scroll_range = GetTextBoxScrollRange();
            ShowNotice(1, formatted, 3, scroll_range, 0);
            const wchar_t* suffix =
                FormatPortraitQuoteNoticeText(*quote_handle, 3, scroll_range, 0);
            ShowNotice(0xf, suffix);
        } else {
            if (g_first_remapped_event_005ee718 <= mapped_event) {
                mapped_event =
                    (g_last_event_005ee70c - g_first_remapped_event_005ee718) + mapped_event;
            }
            if (reinterpret_cast<const unsigned char*>(&g_character_event_descriptors_005ee000[mapped_event])[6] != 0) { // reinterpret-ok: retail deactivate-quote flag byte at descriptor+6
                goto show_deactivate_quote;
            }
        }
        ReleasePortraitQuoteBubble(*quote_handle);
        if (g_current_screen_state.id == W8_SCREEN_CAMP ||
            (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block->flag_327 == 0)) {
            int left = *bubble_x;
            int top = *bubble_y;
            ClearSurfaceRect(left, top, *bubble_width + left, *bubble_height + top);
            InvalidateRegion(left, top, *bubble_width + left, *bubble_height + top, 0);
        }
        RegionSetDisable(party_slot + 0x1d);
        DisableRegionSetInput(party_slot + 0x1d);
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
            RequestRedraw(0x8000);
            RequestRedrawParty();
            record->field_000 = 0;
            return;
        }
        if (g_current_screen_state.id == W8_SCREEN_CAMP) {
            g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
        }
        record->field_000 = active;
        return;
    }

    record->field_000 = 1;
    if (gfCapturingVideo != 0) {
        return;
    }
    record->field_075 = record->field_079;
    record->field_079 = 7;
    record->field_09b = 1;
    record->field_07d = SetCountdownClock(0x78);
    int pose_category;
    if ((int)g_last_event_005ee70c < (int)event_type) {
        pose_category = 1;
    } else {
        unsigned int mapped_event = event_type;
        if (g_first_remapped_event_005ee718 <= event_type) {
            mapped_event = (g_last_event_005ee70c - g_first_remapped_event_005ee718) + event_type;
        }
        pose_category = g_character_event_descriptors_005ee000[mapped_event].category_00;
    }
    int pc_slot = RPCPtrToPCSlot(record);
    record->field_099 = 0;
    unsigned int highest_condition = *reinterpret_cast<unsigned int*>(character_bytes + pc_slot * W8_CHARACTER_SERIALIZED_SIZE + 0xb01); // reinterpret-ok: serialized character highest_condition
    if (highest_condition < 0xf && gXStatus.fSurprisePossible == 0) {
        if (record->field_08d != pose_category) {
            record->field_08d = pose_category;
        }
    } else {
        record->field_08d = 2;
    }
    if (quote_text == 0 || show_quote == 0) {
        bubble[3] = 0xff;
        bubble[4] = 0xff;
        bubble[5] = 0xff;
        bubble[6] = 0xff;
    } else {
        unsigned char layout_quote = 0;
        unsigned char use_modal_gate = 0;
        if ((int)g_last_event_005ee70c < (int)event_type) {
            use_modal_gate = 1;
        } else {
            unsigned int mapped_event = event_type;
            if (g_first_remapped_event_005ee718 <= event_type) {
                mapped_event =
                    (g_last_event_005ee70c - g_first_remapped_event_005ee718) + event_type;
            }
            if (g_character_event_descriptors_005ee000[mapped_event].defer_off_char_screen_05 !=
                0) {
                use_modal_gate = 1;
            } else if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                use_modal_gate = 1;
            } else {
                layout_quote = 1;
            }
        }
        if (use_modal_gate != 0) {
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && IsModalOpen() == 0) {
                layout_quote = 1;
            }
        }
        if (layout_quote == 0 || g_settings_6850c8.pc_subtitles == 0) {
            bubble[3] = 0xff;
            bubble[4] = 0xff;
            bubble[5] = 0xff;
            bubble[6] = 0xff;
        } else {
            unsigned short width;
            unsigned short height;
            *quote_handle =
                LayoutPortraitQuoteBubble(-1, 0, 0, static_cast<const wchar_t*>(quote_text), 200, 0,
                                          0, 0, &width, &height, 0xffffffff);
            *bubble_width = static_cast<short>(width);
            *bubble_height = static_cast<short>(height);
            if (g_current_screen_state.id == W8_SCREEN_CAMP) {
                if (static_cast<int>(party_slot) == g_rcs_mode_0064cbe8) {
                    bubble[7] = 10;
                    bubble[8] = 1;
                    bubble[9] = 8;
                    bubble[10] = 0;
                } else {
                    *bubble_x = static_cast<short>(((party_slot & 1) * 0x30) + 0x36);
                    *bubble_y = static_cast<short>((party_slot >> 1) * 0x27 + 5);
                }
                g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
            } else {
                unsigned short base_x = *reinterpret_cast<unsigned short*>(portrait_quote_tables - 8 + party_slot * 2); // reinterpret-ok: portrait quote x table at 0x0061cb3c
                *bubble_x = base_x;
                *bubble_y = *reinterpret_cast<unsigned short*>(portrait_quote_tables + party_slot * 2 + 8); // reinterpret-ok: portrait quote y table beside pose transitions
                if ((party_slot & 1) == 1) {
                    *bubble_x = static_cast<short>(base_x - *bubble_width + 200);
                }
                if (*bubble_y + *bubble_height > 0x165) {
                    *bubble_y = static_cast<short>(0x165 - *bubble_height);
                }
            }
            unsigned short x = *bubble_x;
            unsigned short y = *bubble_y;
            SetRegionBounds(party_slot + 0x12e, x, y, x + *bubble_width, *bubble_height + y);
            RegionSetEnable(party_slot + 0x1d);
            EnableRegionSetInput(party_slot + 0x1d);
        }
    }
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_settings_6850c8.field_006 != 0 &&
        g_level_block->party_bytes_109[party_slot] == 0 &&
        event_type != static_cast<unsigned int>(g_special_event_0068c568)) {
        RefreshSelectedPartyPortrait(party_slot);
        record->field_0cf = 1;
        record->field_000 = active;
        return;
    }
    record->field_000 = active;
    // clang-format on
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
    slot = &gXStatus.monster_manager_entries[party_slot];
    sound_was_active = slot->field_000;
    slot->field_071 = 0;
    if (sound_was_active != 0) {
        if (IsSoundPlaying(slot->field_001) != 0) {
            handled_00 = 1;
            StopSound(slot->field_001);
        }
        SetPartyPortraitEventState(party_slot, 0, static_cast<unsigned int>(-1), 0, 1);
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
        W8MonsterManagerEntry* slot = &gXStatus.monster_manager_entries[party_slot];
        if (slot->field_071 != 0) {
            vector_40.Remove(slot->field_071);
            RestartFollowUpClock(slot->field_071);
            slot->field_071->Process0052CED0();
            delete slot->field_071;
        }
        DispatchQueuedCharacterEvent(entry);
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
    if ((ShouldDeferCharacterEventForNpcScript(0) != 0 || IsNpcScriptSessionActive() != 0) &&
        (entry->flags_10 & 8) == 0) {
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
    if (entry != 0 && gXStatus.pStartupRuntime->QueueEntry(entry) == 0) {
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
        gXStatus.pStartupRuntime->SetEventCharacterMask(effect, party_slot, 1);
    }
}

// FUNCTION: WIZ8 0x0052E470
unsigned char __fastcall StartupRuntimeDeferredQueueEmpty(W8StartupRuntimeState* state)
{
    return *reinterpret_cast<const int*>(reinterpret_cast<const char*>(state) + 0x14) <= 0;
}

/* Occupied slots with field_000 set still have a portrait/voice record in
   flight; trap-trigger follow-up waits until none of those are active. */
// FUNCTION: WIZ8 0x0052E590
unsigned char PartyPortraitEventsIdle(void)
{
    W8PartySlotRow* row = g_status_685170.buffers.party_rows;
    const W8MonsterManagerEntry* current;

    for (current = gXStatus.monster_manager_entries; current < &gXStatus.monster_manager_entries[8];
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
        W8MonsterManagerEntry* record = &gXStatus.monster_manager_entries[party_slot];
        unsigned char sound_active = 0;

        if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
            continue;
        }
        if (record->field_000 != 0) {
            if (record->field_001 == -1) {
                if (record->field_081 == 0) {
                    if (record->field_071 == 0) {
                        SetPartyPortraitEventState(party_slot, 0, static_cast<unsigned int>(-1), 0,
                                                   1);
                    } else {
                        gXStatus.pStartupRuntime->ProcessOwnedEntry(record->field_071);
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
                    gXStatus.monster_manager_entries[scan].field_000 != 0) {
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
                gXStatus.pStartupRuntime->ProcessOwnedEntry(record->field_071);
            }
            if (record->field_000 == 0) {
                unsigned int scan;
                for (scan = 0; scan < 8; ++scan) {
                    if (g_status_685170.buffers.party_rows[scan].occupied != 0 &&
                        gXStatus.monster_manager_entries[scan].field_000 != 0) {
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

// FUNCTION: WIZ8 0x0052dc80
unsigned char W8StartupRuntimeState::FilterFollowUpQueuedEvent(W8StartupStateElement005EE748* entry)
{
    unsigned int party_slot;
    unsigned int event_type;

    if (entry == 0 || value_50 == -1 || entry->type_08 != (unsigned int)value_50) {
        return 1;
    }
    party_slot = CharacterPointerToPartySlot(entry->character_04);
    if (party_slot == (unsigned int)value_54) {
        return 1;
    }
    if (ClockIsTicking(unknown_58) == 0) {
        value_50 = -1;
        value_54 = -1;
        return 1;
    }
    event_type = entry->type_08;
    if (event_type > g_last_event_005ee70c) {
        return 1;
    }
    if (event_type >= g_first_remapped_event_005ee718) {
        event_type += g_last_event_005ee70c - g_first_remapped_event_005ee718;
    }
    if (g_character_event_descriptors_005ee000[event_type].followup_remap_04 == 0) {
        return 1;
    }
    *reinterpret_cast<unsigned int*>(&entry->unknown_20[0]) = entry->type_08;
    if (entry->type_08 == 4) {
        return 0;
    }
    entry->type_08 = 10;
    return 1;
}

/* Drain deferred character-event queues, optionally cull duplicate pending
   events, then dispatch one ready entry or run the NPC idle helpers. */
// FUNCTION: WIZ8 0x0052ddd0
void W8StartupRuntimeState::ProcessQueuedCharacterEvents()
{
    W8StartupStateElement005EE748* entry;
    W8StartupStateElement005EE748** pending;
    int* duplicate_indices;
    int duplicate_count;
    int index;
    int pending_count;
    unsigned int event_type;
    unsigned int party_slot;
    unsigned int delay;
    unsigned int now;

    if (gXStatus.fSurprisePossible != 0) {
        return;
    }
    if (vector_30.count > 0 && ShouldDeferCharacterEventForNpcScript(0) == 0 &&
        IsNpcScriptSessionActive() == 0) {
        pending_count = vector_30.count;
        for (index = 0; index < pending_count; ++index) {
            pending = vector_30.data;
            if (index < vector_30.count) {
                pending += index;
            }
            QueueEntry(*pending);
        }
        vector_30.count = 0;
    }
    pending_count = vector_10.count;
    duplicate_count = 1;
    if (pending_count != 0) {
        duplicate_indices = new int[pending_count];
        entry = vector_10.data[0];
        duplicate_indices[0] = 0;
        if (pending_count > 1) {
            int write_index = 1;
            for (index = 1; index < vector_10.count; ++index) {
                pending = vector_10.data + index;
                event_type = (*pending)->type_08;
                if (event_type == entry->type_08 &&
                    (*pending)->character_04 != entry->character_04 && event_type != 0x2a &&
                    event_type != 0x24) {
                    duplicate_indices[write_index] = index;
                    ++write_index;
                    ++duplicate_count;
                }
            }
            int survivors = duplicate_count;
            while (survivors > 3) {
                int pick = Random(duplicate_count);
                if (duplicate_indices[pick] != -1) {
                    duplicate_indices[pick] = -1;
                    --survivors;
                }
            }
            for (index = duplicate_count - 1; index >= 0; --index) {
                if (duplicate_indices[index] == -1) {
                    continue;
                }
                int remove_index = duplicate_indices[index];
                if (remove_index >= 0 && remove_index < vector_10.count) {
                    entry = vector_10.data[remove_index];
                    if (remove_index < vector_10.count - 1) {
                        int move_index = remove_index;
                        while (move_index < vector_10.count - 1) {
                            vector_10.data[move_index] = vector_10.data[move_index + 1];
                            ++move_index;
                        }
                    }
                    --vector_10.count;
                    delete entry;
                }
            }
        }
        delete[] duplicate_indices;
    }
    pending_count = vector_10.count;
    if (pending_count == 0) {
        UpdateNpcDialogueVoiceIdle();
        if (PartyPortraitEventsIdle() != 0) {
            ProcessNpcScriptingIdlePass();
        }
        return;
    }
    for (index = 0; index < pending_count; ++index) {
        pending = vector_10.data;
        if (index < vector_10.count) {
            pending += index;
        }
        entry = *pending;
        if (g_current_screen_state.id == W8_SCREEN_CHARACTER ||
            entry->type_08 > g_last_event_005ee70c) {
            goto dispatch_entry;
        }
        event_type = entry->type_08;
        if (event_type >= g_first_remapped_event_005ee718) {
            event_type += g_last_event_005ee70c - g_first_remapped_event_005ee718;
        }
        if (g_character_event_descriptors_005ee000[event_type].defer_off_char_screen_05 == 0) {
        dispatch_entry:
            if (entry->character_04->in_party != 0) {
                event_type = entry->type_08;
                party_slot = CharacterPointerToPartySlot(entry->character_04);
                if (event_type >= g_first_remapped_event_005ee718) {
                    event_type += g_last_event_005ee70c - g_first_remapped_event_005ee718;
                }
                if ((bytes_68[event_type] & (unsigned char)(1 << (party_slot & 31))) == 0) {
                    if (PartyPortraitEventsIdle() == 0) {
                        return;
                    }
                    delay = entry->value_30;
                    if (delay != 0) {
                        now = entry->clock_34;
                        if (GetTickCount() - now <= delay) {
                            return;
                        }
                    }
                    if (index >= 0 && index < vector_10.count) {
                        while (index < vector_10.count - 1) {
                            vector_10.data[index] = vector_10.data[index + 1];
                            ++index;
                        }
                        --vector_10.count;
                    }
                    if (FilterFollowUpQueuedEvent(entry) == 0) {
                        delete entry;
                        return;
                    }
                    if (DispatchQueuedCharacterEvent(entry) == 0) {
                        return;
                    }
                    vector_40.Add(entry);
                    return;
                }
            }
            pending_count = vector_10.count;
            index = 0;
            if (pending_count < 1) {
                return;
            }
            pending = vector_10.data;
            while (*pending != entry) {
                ++index;
                ++pending;
                if (index >= pending_count) {
                    return;
                }
            }
            if (index < 0) {
                return;
            }
            vector_10.RemoveAt(index);
            return;
        }
    }
}
