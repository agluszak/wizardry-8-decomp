#include "wiz8/unattributed/quarantine_common.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/startup_runtime_state.h"

extern void Function52F890(
    int party_slot, int active, int animation, int argument, int show_text);
extern void __fastcall ProcessStartupStateEntry(
    W8StartupStateElement005EE748* entry);
extern int Function52E690(
    W8Character* character, int effect, int argument, int value_1, unsigned int value_2);
extern void Function5E2F40(int sound_handle, unsigned char* state);
extern unsigned char Function56EC90(unsigned int party_slot);
extern void Function55EC90(unsigned int party_slot);
extern unsigned int g_value_0068c57c;
extern unsigned int g_value_0068c554;
extern int g_pose_transition_table_0061cb44[];
extern unsigned int g_value_005ed8fc;
extern unsigned int g_flee_hp_fraction_005ed8f8;
extern int g_effect_005ee594;
extern int g_effect_005ee590;
extern int g_effect_005ee5f8;
extern unsigned int g_first_remapped_event_005ee718;
extern unsigned int g_last_event_005ee70c;
extern unsigned char Function525DF0(unsigned char require_group_entry);
extern unsigned char Function525DD0(void);
extern void Function52E4D0(W8StartupStateElement005EE748* entry);
extern void Function52E160(W8StartupStateElement005EE748* entry);
extern void Function52CA60(void);
extern W8GameSettings g_settings_6850c8;
extern int g_special_event_0068c504;
extern int g_special_event_0068c50c;
extern int g_special_event_0068c51c;
extern int g_special_event_0068c538;
extern int g_special_event_0068c540;
extern int g_special_event_0068c550;
extern int g_special_event_0068c564;
extern int g_special_event_0068c568;

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
// GLOBAL: WIZ8 0x005EE5F8
int g_effect_005ee5f8 = 28;
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
// GLOBAL: WIZ8 0x0068C564
int g_special_event_0068c564;
// GLOBAL: WIZ8 0x0068C568
int g_special_event_0068c568;

/* Address quarantine 0052c241-0053014f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x0052E360
bool IsFlag6850FCSet(void)
{
    return g_flag_6850fc != 0xff;
}

// FUNCTION: WIZ8 0x0052C810
W8StartupStateElement005EE748::W8StartupStateElement005EE748(
    W8Character* character, unsigned int type, int value_0c_arg,
    unsigned int flags, int value_14_arg)
    : handled_00(0), character_04(character), type_08(type),
      value_0c(value_0c_arg), flags_10(flags), value_14(value_14_arg),
      item_id_24(-1), value_30(0)
{
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
        ::operator delete(entry);
        return 0;
    }
    if (g_status_685170.flag_2497 != 0) {
        ::operator delete(entry);
        return 0;
    }
    if (entry->type_08 > 0x91 && (entry->flags_10 & 0x20) == 0) {
        W8MonsterManagerEntry* slot =
            &g_monster_manager_state.entries[party_slot];
        if (slot->field_071 != 0) {
            Function52E4D0(slot->field_071);
            Function52E160(slot->field_071);
            ProcessStartupStateEntry(slot->field_071);
            ::operator delete(slot->field_071);
        }
        Function52CA60();
        return 1;
    }
    if (entry->type_08 == 0x21) {
        int index;
        if (vector_40.count > 0 && vector_40.data[0]->type_08 == 0x21) {
            ::operator delete(entry);
            return 0;
        }
        for (index = 0; index < vector_10.count; ++index) {
            if (vector_10.data[index]->type_08 == 0x21) {
                ::operator delete(entry);
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
    unsigned int event_type, unsigned int party_slot, unsigned char enabled)
{
    unsigned char mask = (unsigned char)(1 << (party_slot & 31));

    if (event_type >= g_first_remapped_event_005ee718) {
        event_type += g_last_event_005ee70c - g_first_remapped_event_005ee718;
    }
    if (enabled == 0) {
        bytes_68[event_type] &= (unsigned char)~mask;
    }
    else {
        bytes_68[event_type] |= mask;
    }
}

// FUNCTION: WIZ8 0x0052E690
int Function52E690(
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
    return reinterpret_cast<int>(entry);
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
    ProcessStartupStateEntry(entry);
    ::operator delete(entry);
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
        W8MonsterManagerEntry* record = &g_monster_manager_state.entries[party_slot];
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
                    g_monster_manager_state.entries[scan].field_000 != 0) {
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
                        g_monster_manager_state.entries[scan].field_000 != 0) {
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

        if (g_flag_00683f97 != 0 && (party_slot & 1) != 0 &&
            Function56EC90(party_slot) != 0) {
            continue;
        }
        if (record->field_09b == 0 && record->field_0bd == 0 &&
            (g_screen_state_0068ec78.id != 3 || record->field_000 != 0)) {
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
                Function55EC90(party_slot);
            }
        }
    }
    return any_active;
}
