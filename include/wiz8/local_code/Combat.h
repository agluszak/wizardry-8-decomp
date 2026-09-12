#pragma once

#include "wiz8/targeting.h"

void ChooseAction(int party_slot, int action, int detail, const void* data, int a,
                  int b); /* 0x004E7CC0 */
void ChooseCombatAction(int party_slot, int context, int* out_kind, int* out_action,
                        W8CombatSlot** out_target,
                        W8ActionDetailBlock** out_detail); /* 0x004E77B0 */
void SetCharacterCombatAction(int party_slot, int action_kind, int action_detail, int arg_4,
                              void* data); /* 0x004E8000 */
void EndCombat004EA310(int mode);          /* 0x004EA310 */
unsigned char IsSlotActionChosen(int party_slot, int context, int arg_3, int arg_4);
void SwitchCharacterTo(int party_slot, int action); /* 0x004ED390 */
void Function4EA1F0(void);                          /* 0x004EA1F0 */
void Function517780(void);                          /* 0x00517780 */
void Function5A3470(void);                          /* 0x005A3470 */
struct W8TargetSource;
struct W8CombatSlot;
union W8ActionDetailBlock;
struct W8ItemInstance;

void Function51EB90(W8Character* character, W8ItemInstance* item, int a, int b); /* 0x0051EB90 */
void Function51EA90(W8Character* character, W8ItemInstance* item);               /* 0x0051EA90 */
unsigned char Function4F96F0(W8Character* character);                            /* 0x004F96F0 */
unsigned char Function4F9750(W8Character* character, int target);                /* 0x004F9750 */
void Function4ECC80(W8TargetSource* source, W8CombatSlot* target);               /* 0x004ECC80 */
void Function537540(int party_slot);                                             /* 0x00537540 */
void Function4EA5C0(int party_slot);                                             /* 0x004EA5C0 */
void Function4E7EE0(int party_slot, int action, int detail, const void* data,
                    int arg_6);                             /* 0x004E7EE0 */
void Function52E5C0(int event, int a, int b, int argument); /* 0x0052E5C0 */
extern int g_special_event_0068c50c;                        /* 0x0068C50C */
int GetSelectedOrFallbackValue0059E0D0(void);               /* 0x0059E0D0 */
int Function5A1350(void);                                   /* 0x005A1350 */
