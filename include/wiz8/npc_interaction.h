#pragma once

/* The NPC-scripting gates NPC Scripting.cpp sets and npc_interaction.cpp
   reads. */
extern unsigned char g_flag_68c4a0;
extern unsigned char g_flag_68c4f6;
extern unsigned char g_flag_68c4f7;
extern int g_value_68c4c0;

struct W8NpcState;
extern W8NpcState* g_npc_state_68c4ac;

unsigned char Function525DD0(void);
unsigned char Function525DF0(unsigned char require_group_entry);

bool IsPartySlotEligible00524A10(int slot);
