#pragma once

#include "wiz8/local_code/NPCScripting.h"

/* The NPC-scripting gates NPC Scripting.cpp sets and npc_interaction.cpp
   reads. */
const char* GetNpcDisplayName(W8NpcState* npc);

unsigned char IsNpcScriptSessionActive(void);
unsigned char ShouldDeferCharacterEventForNpcScript(unsigned char require_group_entry);

/* 0x005294C0: format the bound NPC's quote for one event type into the wide
   output. Answers zero outside the NPC's quote count or with no text there;
   the body remains a frontier because the bound-object quote table is not yet
   typed. */
unsigned char GetNpcQuoteText(W8NpcState* npc, unsigned int type, wchar_t* output);

bool IsPartySlotEligible00524A10(int slot);
void ClearNpcMessageQueue(void); /* 0x00524C50 */

void FormatNpcVoiceSoundPath(W8NpcState* npc, char* output);
void BeginNpcScriptDialogue(W8NpcState* npc, unsigned char preserve_state);
void FinishNpcVoicePlayback(unsigned char resume_script);
int ComputePortraitMessageDuration(wchar_t* text);
void RunNpcScriptLine(int script_line, unsigned char param);
void ProcessNpcScriptingIdlePass(void);
void UpdateNpcDialogueVoiceIdle(void);
