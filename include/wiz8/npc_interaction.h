#pragma once

/* The NPC-scripting gates NPC Scripting.cpp sets and npc_interaction.cpp
   reads. */
extern unsigned char g_flag_68c4a0;
extern unsigned char g_flag_68c4f6;
extern unsigned char g_flag_68c4f7;

struct W8NpcState;
extern W8NpcState* g_npc_state_68c4ac;

unsigned char Function525DD0(void);
unsigned char Function525DF0(unsigned char require_group_entry);

/* 0x005294C0: format the bound NPC's quote for one event type into the wide
   output. Answers zero outside the NPC's quote count or with no text there;
   the body remains a frontier because the bound-object quote table is not yet
   typed. */
unsigned char GetNpcQuoteText(W8NpcState* npc, unsigned int type, wchar_t* output);

bool IsPartySlotEligible00524A10(int slot);
