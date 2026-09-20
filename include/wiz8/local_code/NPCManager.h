#pragma once

#include "surrender/srMath.h"
#include "wiz8/layouts/npc_state.h"

struct W8Chunk;
class W8Monster;
class Trigger;
struct W8GameplayModifierBlock;
struct W8MonsterInfo;
struct W8MonsterManagerEntry;

/* NPC database record indices used as name-style ids at runtime. */
enum {
    W8_NPC_ZANT = 0x0f,
    W8_NPC_DRAZIC = 0x10,
    W8_NPC_RODAN = 0x11,
    W8_NPC_VI_DOMINA = 0x18,
    W8_NPC_RFS81_A = 0x20,
    W8_NPC_GLUMPH = 0x2b,
    W8_NPC_AL_ADRYIAN = 0x3b,
    W8_NPC_MADRAS = 0x4a,
    W8_NPC_SEXUS = 0x4f,
    W8_NPC_PHOONZANG = 0x87,
    W8_NPC_ENDGAME2 = 0x8e,
};

enum W8NpcServiceFlag {
    W8_NPC_SERVICE_ARNIKA = 0x1,
    W8_NPC_SERVICE_TRYNTON = 0x2,
    W8_NPC_SERVICE_MARTEN_BLUFF = 0x4,
    W8_NPC_SERVICE_SEA_CAVES = 0x8,
    W8_NPC_SERVICE_MT_GIGAS = 0x10,
    W8_NPC_SERVICE_RIFT = 0x20,
    W8_NPC_SERVICE_RAPAX = 0x40,
    W8_NPC_SERVICE_BAYJIN = 0x80,
    W8_NPC_SERVICE_ASCENSION = 0x100,
    W8_NPC_SERVICE_CIRCLE = 0x200,
    W8_NPC_SERVICE_SWAMP = 0x400,
    W8_NPC_SERVICE_RAPAX_CAMP = 0x800,
    W8_NPC_SERVICE_GIGAS_CAVES = 0x1000,
    W8_NPC_SERVICE_MTN_PASS = 0x2000,
};

W8Monster* GetNpcMonster(W8NpcState* npc);

void ChooseNewGameStartLocation(int* level, int* entrance);                         /* 0x005092F0 */
void SelectStartNpcGreeting(void);                                                  /* 0x00509560 */
int SelectNewGameStartLevel(void);                                                  /* 0x00509750 */
void BindNpcToMonster(unsigned char value, int enabled, int location_id);           /* 0x00509CD0 */
bool RecruitNpcIntoParty(W8NpcState* npc);                                          /* 0x0050B160 */
int DismissNpcFromParty(int party_slot, int unused, bool skip_spawn, bool neutral); /* 0x0050B590 */
void ReturnDismissedNpcItems(W8NpcState* npc, W8Character* character);              /* 0x0050DDC0 */
/* 0x0050B9B0: how many leading party slots are occupied. */
unsigned char CountLeadingPartySlots(void);
char GetNpcDisposition(W8NpcState* npc);               /* 0x0050A280 */
bool NpcKnowsFact(W8NpcState* npc, unsigned int fact); /* 0x0050DD10 */
unsigned char FindNpcOfKind(int kind);                 /* 0x0050DD80 */
bool CanNpcJoinParty(W8NpcState* npc);                 /* 0x0050C870 */
bool ProbeNpcPlacementNearParty(int party_slot, int mode,
                                srVector3T<float>* position_out); /* 0x0050B2F0 */
bool CanPlaceNpcNearParty(int party_slot);                        /* 0x0050B2D0 */
W8MonsterInfo* GetNpcMonsterInfo(W8NpcState* npc);                /* 0x0050A3C0 */
W8NpcState* GetNpcStateForMonsterInfo(W8MonsterInfo* monster_info,
                                      unsigned char allow_unavailable); /* 0x0050A4A0 */
void ResumeNpc(W8NpcState* npc, int enabled);                           /* 0x0050AE40 */
void QueueNpcTravelRefusals(int destination_level);                     /* 0x0050E230 */
void MarkNpcOfKind(int kind);                                           /* 0x0050CA30 */
W8Character* GetNpcGroupCharacter(W8NpcState* npc);                     /* 0x0050B8B0 */

/* The NPC-side global frame operation: timed world events and the per-frame
   NPC state passes. */
void UpdateNpcEvents0050D530(void);
void AdvanceNpcTimers0050C7D0(unsigned int elapsed);
void ProcessNpcPendingEvents0050CA80(void);

/* 0x00509890 and 0x00509920: lazily create and then empty the shared NPC-state
   vector, recreating a runtime node for every database record still in use. */
void InitializeNpcStates(void);
void ResetNpcStates(void);
void ReleaseNpcStates005099D0(void); /* 0x005099D0 */
/* 0x00509AA0: build one runtime state from its database record and return it,
   reusing the slot of a released node when one is free. */
W8NpcState* CreateNpcRuntimeNode(int npc_id);
/* 0x0050AED0: expand the record's character block into a fresh character. */
unsigned char InitializeNpcCharacter(W8NpcState* npc, W8Character* character);
/* 0x0050B9E0: copy the record's item table into the state's runtime arrays. */
void InitializeNpcItemTable(W8NpcState* npc);

/* The NPC-side consequence pass the sight code runs when a marked NPC's
   binding is released. */
void HandleMarkedNpcEvent0050CF70(W8NpcState* npc, char mode);
/* 0x0050DBF0: the bound-NPC penalty the condition/enchantment rebuild folds
   into the character's modifier block while the slot's flag_fe is set. */
void ApplyBoundNpcPenalty0050DBF0(W8Character* character, W8GameplayModifierBlock* target);
/* 0x0050C560: place or move the NPC's monster at the named world entity. */
unsigned char RestoreNpcMonster0050C560(W8NpcState* npc, const char* entity_name);
/* 0x0050ABF0: the activation callback the rebinding installs on the level's
   NPC triggers. */
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
bool NpcWantsItem0050DC50(W8NpcState* npc, W8ItemInstance* item);
bool NpcLeadHasNameStyle(unsigned int kind);
/* 0x00509EA0: clear one NPC binding's monster link and hand the handle to the
   owned item-list teardown. */
void ReleaseNpcBinding(int value);
/* 0x0050A440: the NPC binding selected by a monster-list index, or null. */
W8NpcState* FindNpcBindingForMonster(unsigned int monster_list_index);
unsigned char GetNpcDispositionBand(W8NpcState* npc);
void SetNpcDispositionBand(W8NpcState* npc, char band);          /* 0x0050A520 */
char WillNpcTradeForItem(W8NpcState* npc, W8ItemInstance* item); /* 0x0050A9C0 */
void ApplyNpcInteraction0050A570(W8NpcState* npc, int kind, int value, W8ItemInstance* item,
                                 unsigned int gold); /* 0x0050A570: every retail call pushes five */
/* 0x0050AA00: whether the NPC's database entry carries the value at 0x002. */
bool NpcRecordHasValue002(W8NpcState* npc);
/* 0x0050C140: push a topic onto the NPC's five-slot topic list. */
void AddNpcTopic(W8NpcState* npc, int topic);
/* 0x0050BC90: resolve one pickpocket attempt; the taken item goes to
   item_out and the taken gold to gold_out. Result codes feed the
   0x00576D80 dispatch. */
int AttemptNpcPickpocket0050BC90(W8Character* character, W8NpcState* npc, W8ItemInstance* item_out,
                                 unsigned int* gold_out);
/* 0x0050E4B0: clear the npc's item_ids_30 slots matching the item the quote
   entry just handed out. */
void ClearNpcItemId(W8NpcState* npc, int item_id);
/* 0x0050ADA0: the live NPC state whose display (or fact-substituted) name
   matches case-insensitively; 0 when none does. */
W8NpcState* FindNpcStateByName(const char* name);
W8MonsterManagerEntry* GetNpcGroupEntry(W8NpcState* npc);
const char* GetNpcDisplayName(W8NpcState* npc);
void ReleaseNpcMonsterBinding0050C440(W8NpcState* npc, char level);                 /* 0x0050C440 */
void RestoreNamedNpcAtLevel0050C1C0(int kind, char level, const char* entity_name); /* 0x0050C1C0 */
unsigned char ClearNpcScheduledItem(W8NpcState* npc, int item_id,
                                    W8ItemInstance* out); /* 0x0050BA80 */
void ReleaseNpcMonsterByKind(int kind);                   /* 0x0050C680 */
/* 0x0050DD50: record that the NPC has told the party the given fact. */
void TellNpcFact(W8NpcState* npc, short fact);
struct W8Chunk;
unsigned char SaveNpcStates00509F00(W8Chunk* chunks);
void LoadNpcStates00509FC0(W8Chunk* chunks);
unsigned char SaveNpcItemLists0050AA10(int file);
unsigned char LoadNpcItemLists0050AAF0(unsigned int file);
char ScoreNpcTheft0050BAF0(W8Character* character, W8NpcState* npc, int item_id, int count);
char AttemptNpcItemTheft0050C040(W8Character* character, W8NpcState* npc, int item_id, int count);
void UpdateNpcPartyMember0050B3B0(int party_slot);
char QueueNpcDepartureEvents0050DEC0(int destination_level);
