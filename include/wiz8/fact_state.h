#ifndef WIZ8_FACT_STATE_H
#define WIZ8_FACT_STATE_H

/* 1000 entries, indices 0..999, stored at 0x00689B78 and ending exactly at
   g_npc_name_buffer. GetFact/SetFact check fact_id > 1000, so index 1000
   aliases g_npc_name_buffer[0] - a retail off-by-one, kept faithful. */
extern unsigned char g_fact_values[1000];
extern unsigned char g_fact_notifications_suppressed;
extern bool g_import_party_loaded;
extern unsigned char g_import_flag_0068de5d;

/* Fact ids recovered code tests. The names are the FACT.DBS record strings;
   the database carries them itself. */
enum {
    W8_FACT_ARNIKA_MYLES_MEET_ONCE = 0x1c,
    W8_FACT_ALIGNMENT_UMPANI = 0x3c,
    W8_FACT_RFS81_HAS_BEEN_FIXED = 0x44,
    W8_FACT_IMPORT_TRANG = 0x4b,
    W8_FACT_UMISSION_SCUBA_DONE = 0x8b,
    W8_FACT_PEACE_ACHIEVED = 0xbf,
    W8_FACT_TEMPLAR = 0x15f,
    W8_FACT_TRYNNIE_MADRAS_WILL_JOIN = 0x1a7,
    W8_FACT_SEXUS_PAID = 0x20d,
    W8_FACT_VI_IS_DEAD = 0x216,
    W8_FACT_PARTY_MET_VI = 0x2ee,
    W8_FACT_TRANG_YOU_ARE_BUSTED = 0x2f1,
};

unsigned char GetFact(int fact_id);
unsigned char EvaluateFact(int fact_id); /* 0x005080F0 */
void HandleFactChange(int fact_id, unsigned char value);
/* Scripted consequences of an NPC-bound monster's death; lives in NPC
   Scripting Facts.cpp, driven by MonsterManager's death switch. */
void HandleScriptedNpcDeath(unsigned int monster_list_index); /* 0x00508D70 */
void SetFact(int fact_id, unsigned char value, unsigned char suppress_side_effects);
void SaveFactState(int save_handle);
void InitializeFactState(void);
void LoadFactState(int save_handle);
void SetFactNotificationsSuppressed(unsigned char suppressed);

void PostNewGameLoad005063E0(void);

#endif
