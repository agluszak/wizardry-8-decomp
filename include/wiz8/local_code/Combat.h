#pragma once

void ChooseAction(int party_slot, int action, int detail, int a, int b, int c); /* 0x004E7CC0 */
void ChooseCombatAction(
    int party_slot, int is_monster_turn, int* out_kind, int a, int b, int c); /* 0x004E77B0 */
void Function4E7CC0(
    int party_slot, int arg_2, int arg_3, void* arg_4, int arg_5, int arg_6);
void Function4E8000(int party_slot, int action_kind, int action_detail, int a, int b);
void Function4EA310(int mode);
unsigned char IsSlotActionChosen(int party_slot, int context, int arg_3, int arg_4);
void SwitchCharacterTo(int party_slot, int action);               /* 0x004ED390 */
