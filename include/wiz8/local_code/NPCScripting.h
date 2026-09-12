#pragma once

#include "surrender/srMath.h"

struct W8NpcState;
struct W8ItemInstance;
struct W8Character;
struct W8MonsterGroup;
struct W8WorldItem;

void UpdateNpcDialogueVoiceAndCursor(void); /* 0x00524DA0 */
void ProcessNpcScriptingFrame(void);        /* 0x00524EB0 */
void Function525C50(char param_1);          /* 0x00525C50 */
unsigned char Function525DD0(void);         /* 0x00525DD0 */
unsigned char Function525DF0(unsigned char require_group_entry); /* 0x00525DF0 */
void Function526E90(void);                  /* 0x00526E90 */
void Function5289B0(int kind, int argument); /* 0x005289B0 */
void Function529510(void);
void Function528830(int a, int b, int c, int d);                          /* 0x00528830 */
void Function529BE0(void);                                                /* 0x00529BE0 */
void Function529EF0(void);                                                /* 0x00529EF0 */
void Function50C440(W8NpcState* npc, int value);                          /* 0x0050C440 */
void Function50C1C0(char name_style, int value, const char* entity_name); /* 0x0050C1C0 */
unsigned char ClearNpcScheduledItem(W8NpcState* npc, int item_id,
                                    W8ItemInstance* out);         /* 0x0050BA80 */
void ReleaseNpcMonsterByKind(int kind);                           /* 0x0050C680 */
void SetFactionDispositionBand(char faction, unsigned char band); /* 0x00535B40 */
void SetMonsterGroupHostility(W8MonsterGroup* group, unsigned int hostility,
                              char recurse);            /* 0x00547570 */
void Function553AD0(W8Character* character, int value); /* 0x00553AD0 */
void Function553C10(W8Character* character, int skill); /* 0x00553C10 */
void Function420F90(srVector3T<float>* position);       /* 0x00420F90 */
void Function4F6CF0(W8WorldItem* item);                 /* 0x004F6CF0 */
void Function5A6580(void);                              /* 0x005A6580 */
void SetFlag68C4F4(void);                               /* 0x00529560 */
void SetFlag68C4F7(void);                               /* 0x00529BC0 */
void ClearFlag68C4F7(void);                             /* 0x00529BD0 */
