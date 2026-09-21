#pragma once

struct RuntimeObservation {
    unsigned char engine_ready;
    unsigned char semantic_ok;
    int menu_state;
    unsigned int region_set_enabled;
    unsigned int first_region;
    unsigned int last_region;
    unsigned char menu_seen;
    unsigned char shade_table_ok;
    unsigned char exit_observed;
    unsigned char transition_observed;
    unsigned char character_entered;
    unsigned char character_returned;
    unsigned char final_page_entered;
    unsigned char final_page_redrawn;
    unsigned char character_name_typed;
    unsigned char character_summary_opened;
    unsigned char character_committed;
    unsigned char character_in_party;
    unsigned char main_game_entered;
    unsigned char party_moved;
    unsigned char party_turned;
    unsigned char world_soaked;
    unsigned char automap_opened;
    unsigned char automap_closed;
    unsigned char game_saved;
    unsigned char game_loaded;
    unsigned char load_position_restored;
    unsigned char combat_started;
    unsigned char combat_action_queued;
    unsigned char combat_party_moved;
    unsigned char combat_ended;
    unsigned char combat_aggroed;
    unsigned char monster_engaged;
    unsigned char return_observed;
    unsigned char timed_out;
    int character_page_start;
    int character_page_after;
    unsigned char tooltip_shown;
    unsigned char tooltip_removed;
    unsigned char skill_tooltip_shown;
    unsigned char skill_tooltip_removed;
    unsigned char skill_interacted;
    unsigned char npc_state_reset_ok;
    unsigned char playlist_active;
    int playlist_tracks;
    int playlist_weight;
    int playlist_pause_min;
    int playlist_pause_max;
    int playlist_pause_chance;
    int patch_catalog_count;
    unsigned int item_database_count;
    unsigned int monster_database_count;
    unsigned int npc_database_count;
    unsigned char patch_precedence_ok;
    unsigned char physical_fallback_ok;
};

enum RuntimePhase { RUNTIME_ENGINE_READY, RUNTIME_MAIN_MENU, RUNTIME_MAIN_GAME };
enum RuntimeTier { RUNTIME_PR, RUNTIME_MAIN, RUNTIME_NIGHTLY };
enum RuntimeKind { RUNTIME_ACCEPTANCE, RUNTIME_INTEGRATION, RUNTIME_SEMANTIC };

typedef unsigned long (*RuntimeScenarioFn)();
typedef bool (*RuntimeValidateFn)(const RuntimeObservation& observation);

struct RuntimeScenario {
    const char* name;
    RuntimePhase phase;
    RuntimeTier tier;
    RuntimeKind kind;
    unsigned int timeout_ms;
    RuntimeScenarioFn run;
    RuntimeValidateFn validate;
};
