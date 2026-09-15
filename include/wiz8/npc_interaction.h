#pragma once

#include "wiz8/local_code/NPCScripting.h"

/* The NPC-scripting gates NPC Scripting.cpp sets and npc_interaction.cpp
   reads. */

unsigned char IsNpcScriptSessionActive(void);
void Function525D90(char force); /* 0x00525D90 */
unsigned char ShouldDeferCharacterEventForNpcScript(unsigned char require_group_entry);

/* 0x005294C0: format the bound NPC's quote for one event type into the wide
   output. */
unsigned char GetNpcQuoteText(W8NpcState* npc, unsigned int type, wchar_t* output);

bool IsPartySlotEligible00524A10(int slot);
void ClearNpcMessageQueue(void); /* 0x00524C50 */

void FormatNpcVoiceSoundPath(W8NpcState* npc, char* output);
void BeginNpcScriptDialogue(W8NpcState* npc, unsigned char preserve_state);
void FinishNpcVoicePlayback(unsigned char resume_script);
int ComputePortraitMessageDuration(wchar_t* text);
void UpdateNpcDialogueVoiceIdle(void);
