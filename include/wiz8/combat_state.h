#ifndef WIZ8_COMBAT_STATE_H
#define WIZ8_COMBAT_STATE_H

struct W8Character;
struct W8CombatState;

#ifdef __cplusplus
extern "C" {
#endif

extern W8CombatState* g_combat_state;    /* 0x006836A8 */
extern W8Character* g_party_characters;  /* 0x00685174 */
extern unsigned char g_in_combat_00683f94;

#ifdef __cplusplus
}
#endif

#endif
