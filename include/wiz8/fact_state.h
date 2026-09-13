#ifndef WIZ8_FACT_STATE_H
#define WIZ8_FACT_STATE_H

/* 1000 entries, indices 0..999, stored at 0x00689B78 and ending exactly at
   g_npc_name_buffer. GetFact/SetFact check fact_id > 1000, so index 1000
   aliases g_npc_name_buffer[0] - a retail off-by-one, kept faithful. */
extern unsigned char g_fact_values[1000];
extern unsigned char g_fact_notifications_suppressed;
extern unsigned char g_import_party_loaded;
extern unsigned char g_import_flag_0068de5d;

unsigned char GetFact(int fact_id);
unsigned char EvaluateFact(int fact_id); /* 0x005080F0 */
void HandleFactChange(int fact_id, unsigned char value);
void SetFact(int fact_id, unsigned char value, unsigned char suppress_side_effects);
void SaveFactState(int save_handle);
void InitializeFactState(void);
void LoadFactState(int save_handle);
void SetFactNotificationsSuppressed(unsigned char suppressed);

void Function5063E0(void);

#endif
