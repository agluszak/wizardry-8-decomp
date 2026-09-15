#pragma once

#include "surrender/srMath.h"
#include "wiz8/layouts/npc_state.h"

class W8Monster;
class Trigger;
struct W8GameplayModifierBlock;
struct W8MonsterInfo;
struct W8MonsterManagerEntry;

W8Monster* GetNpcMonster(W8NpcState* npc);

void ChooseNewGameStartLocation(int* level, int* entrance);             /* 0x005092F0 */
void SelectStartNpcGreeting00509560(void);                              /* 0x00509560 */
int SelectNewGameStartLevel(void);                                      /* 0x00509750 */
void Function509CD0(unsigned char value, int enabled, int location_id); /* 0x00509CD0 */
void Function50B160(W8NpcState* npc);                                   /* 0x0050B160 */
void Function50B590(int value, int a, int b, int c);                    /* 0x0050B590 */
/* 0x0050B9B0: how many leading party slots are occupied. */
unsigned char CountLeadingPartySlots(void);
char GetNpcDisposition(W8NpcState* npc);                                           /* 0x0050A280 */
bool NpcKnowsFact(W8NpcState* npc, unsigned int fact);                             /* 0x0050DD10 */
unsigned char FindNpcOfKind(int kind);                                             /* 0x0050DD80 */
void Function55BB10(W8NpcState* npc);                                              /* 0x0055BB10 */
unsigned char CanNpcJoinParty(W8NpcState* npc);                                  /* 0x0050C870 */
void RestockNpcInventory(W8NpcState* npc);                                         /* 0x0055BCC0 */
unsigned char UpdateNpcAt(W8NpcState* npc, int arg_2, srVector3T<float>* scratch); /* 0x0050B2F0 */
W8MonsterInfo* GetNpcMonsterInfo(W8NpcState* npc);                                 /* 0x0050A3C0 */
void Function50AE40(W8NpcState* npc, int enabled);                                 /* 0x0050AE40 */
void QueueNpcTravelRefusals(int destination_level);                                /* 0x0050E230 */
void MarkNpcOfKind(int kind);                                                      /* 0x0050CA30 */
W8Character* GetNpcGroupCharacter(W8NpcState* npc);                                /* 0x0050B8B0 */

/* The NPC-side global frame operation: timed world events and the per-frame
   NPC state passes. */
void UpdateNpcEvents0050D530(void);

/* 0x00509890 and 0x00509920: lazily create and then empty the shared NPC-state
   vector, recreating a runtime node for every database record still in use. */
void InitializeNpcStates(void);
void ResetNpcStates(void);
/* 0x00509AA0: build one runtime state from its database record and return it,
   reusing the slot of a released node when one is free. */
W8NpcState* CreateNpcRuntimeNode(int npc_id);
/* 0x0050AED0: expand the record's character block into a fresh character. */
unsigned char InitializeNpcCharacter(W8NpcState* npc, W8Character* character);
/* 0x0050B9E0: copy the record's item table into the state's runtime arrays. */
void InitializeNpcItemTable(W8NpcState* npc);

/* The NPC-side consequence pass the sight code runs when a marked NPC's
   binding is released. */
void Function50CF70(W8NpcState* npc, int mode);
/* 0x0050DBF0: the bound-NPC penalty the condition/enchantment rebuild folds
   into the character's modifier block while the slot's flag_fe is set. */
void ApplyBoundNpcPenalty0050DBF0(W8Character* character, W8GameplayModifierBlock* target);
/* 0x0050C560: place or move the NPC's monster at the named world entity. */
unsigned char RestoreNpcMonster0050C560(W8NpcState* npc, char* entity_name);
/* 0x0050ABF0: the activation callback the rebinding installs on the level's
   NPC triggers. */
bool Function50ABF0(Trigger* trigger);
void ResetNpcBindingsForParty0050DB50(void);
void ClearPendingNpcLevelFlags0050C270(void);
void ReleaseNpcMonsterBindings0050C2E0(void);
void ReleaseMarkedNpcBindings0050DA00(void);
void RebindNpcLevelTriggers0050AC60(void);
W8NpcState* GetNpcState(int index);
W8NpcState* GetNpcStateByKind(int kind);
/* 0x0050DC50: whether the NPC wants the offered item - it matches one of the
   record's wanted entries by id or by the shared 0x83 name kind, and a grouped
   NPC whose member already carries more than one declines. */
char NpcWantsItem0050DC50(W8NpcState* npc, W8ItemInstance* item);
bool NpcLeadHasNameStyle(unsigned int kind);
/* 0x00509EA0: clear one NPC binding's monster link and hand the handle to the
   owned item-list teardown. */
void ReleaseNpcBinding(int value);
/* 0x0050A440: the NPC binding selected by a monster-list index, or null. */
W8NpcState* FindNpcBindingForMonster(unsigned int monster_list_index);
unsigned char GetNpcDispositionBand(W8NpcState* npc);
void SetNpcDispositionBand(W8NpcState* npc, char band);          /* 0x0050A520 */
char WillNpcTradeForItem(W8NpcState* npc, W8ItemInstance* item); /* 0x0050A9C0 */
void Function50A570(W8NpcState* npc, char kind, int value, W8ItemInstance* item);
W8MonsterManagerEntry* GetNpcGroupEntry(W8NpcState* npc);
const char* GetNpcDisplayName(W8NpcState* npc);
void Function50C440(W8NpcState* npc, int value);                                   /* 0x0050C440 */
void Function50C1C0(unsigned char name_style, int value, const char* entity_name); /* 0x0050C1C0 */
unsigned char ClearNpcScheduledItem(W8NpcState* npc, int item_id,
                                    W8ItemInstance* out); /* 0x0050BA80 */
void ReleaseNpcMonsterByKind(int kind);                   /* 0x0050C680 */
