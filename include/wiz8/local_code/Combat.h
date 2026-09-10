#pragma once

void ChooseAction(int party_slot, int action, int detail, int a, int b, int c); /* 0x004E7CC0 */
void ChooseCombatAction(
    int party_slot, int is_monster_turn, int* out_kind, int a, int b, int c); /* 0x004E77B0 */
void Function4E7CC0(
    int party_slot, int arg_2, int arg_3, void* arg_4, int arg_5, int arg_6);
void Function4E8000(int party_slot, int action_kind, int action_detail, int a, int b);
void EndCombat004EA310(int mode);  /* 0x004EA310 */
unsigned char IsSlotActionChosen(int party_slot, int context, int arg_3, int arg_4);
void SwitchCharacterTo(int party_slot, int action);               /* 0x004ED390 */
void Function4EA1F0(void);          /* 0x004EA1F0 */
void Function5A1890(void);          /* 0x005A1890 */
void Function517780(void);          /* 0x00517780 */
void Function5A3470(void);          /* 0x005A3470 */
