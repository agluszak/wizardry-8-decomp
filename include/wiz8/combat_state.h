#ifndef WIZ8_COMBAT_STATE_H
#define WIZ8_COMBAT_STATE_H

#include "wiz8/targeting.h"

struct W8Character;
struct W8CombatState;

#pragma pack(push, 1)
/* One party slot row. Only the fields reached by recovered combat and
   targeting code are named. */
typedef struct W8PartySlotRow {
    unsigned char flag_00;
    int pending_action;
    int attack_mode[4];
    unsigned char unknown_015[8];
    W8CombatSlot target_out_of_combat;
    unsigned char unknown_03d[0x10];
    W8CombatSlot target_in_combat;
    int action_kind;
    int action_detail;
    int spell_id;
    int spell_power_level;
    int spell_power_extra;
    W8CombatSlot spell_target;
    int item_use_kind;
    W8ItemInstance* item_in_use;
    W8CombatSlot item_target;
    int item_id_0c9;
    unsigned char item_origin;
    unsigned short item_slot;
    unsigned char flag_0d0;
    W8CombatSlot target_context_5;
    unsigned char unknown_0f1[4];
    unsigned char flag_0f5;
    unsigned char unknown_0f6[4];
    int animation_0fa;
    unsigned char unknown_0fe[6];
    unsigned char action_is_kind_one;
    unsigned char flag_105;
} W8PartySlotRow;
#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

extern W8CombatState* g_combat_state;    /* 0x006836A8 */
extern W8Character* g_party_characters;  /* 0x00685174 */
extern unsigned char g_in_combat_00683f94;
extern W8PartySlotRow* g_party_slot_rows; /* 0x00684938 */

#ifdef __cplusplus
}
#endif

#endif
