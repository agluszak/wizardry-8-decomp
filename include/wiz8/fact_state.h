#ifndef WIZ8_FACT_STATE_H
#define WIZ8_FACT_STATE_H

extern "C" {

extern unsigned char g_log_fact_checks;
extern unsigned char g_fact_values[1001];
extern unsigned char g_fact_notifications_suppressed;
extern unsigned char g_import_party_loaded;
extern int g_import_character_count;
extern int g_import_ending_choice;
extern unsigned char g_import_flags[0x60];

unsigned char GetFact(int fact_id);
void SetFact(
    int fact_id, unsigned char value, unsigned char suppress_side_effects);
void SaveFactState(int save_handle);
void InitializeFactState(void);
void LoadFactState(int save_handle);
void SetFactNotificationsSuppressed(unsigned char suppressed);

}

#endif
