#pragma once

#include "runtime_case.h"

/* Everything the character-creation and party-selection flow polls, copied on
   the game thread in one read. The driver works from this record and never
   dereferences a character screen, page, or entry pointer. */
struct FlowEntryState {
    int enabled;
    int increment_allowed;
    int spent;
    int limit;
};

struct CharacterFlowState {
    int current;
    int pending;
    int party_selection_ready;
    int party_selection_set;
    int left_action_set;
    int bottom_action_set;
    int character_screen_present;
    int page_index;
    int page_present[4];
    int stats_set_enabled;
    int profession_index;
    int race_index;
    int gender_index;
    int stat_entries_enabled;
    int attributes_complete;
    int skills_complete;
    int stat_count;
    FlowEntryState stat_entries[16];
    int skill_count;
    FlowEntryState skill_entries[16];
    int final_prepared;
    int dialog_present;
    wchar_t name[10];
    wchar_t name_part_2[6];
    int active_characters;
    int captured_region;
    unsigned long intro_index;
    int skip_loose_check;
    int wiz7_ending;
    int transition_objects;
};

bool ReadCharacterFlow(RuntimeCase& test, CharacterFlowState& out,
                       const char* step = "character-flow");
bool WaitCharacterFlow(RuntimeCase& test, const char* step, unsigned long budget_ms,
                       bool (*predicate)(const CharacterFlowState& state, void* ctx),
                       void* ctx = 0);

/* Click/hover targets resolved on the game thread by identity rather than by
   a product pointer the driver would have to chase. */
enum FlowTarget {
    FLOW_TARGET_NEW_GAME,
    FLOW_TARGET_CREATE_CHARACTER,
    FLOW_TARGET_PROFESSION_NEXT,
    FLOW_TARGET_RACE_NEXT,
    FLOW_TARGET_GENDER_NEXT,
    FLOW_TARGET_STAT_INCREMENT,
    FLOW_TARGET_SKILL_INCREMENT,
    FLOW_TARGET_SKILL_HELP,
    FLOW_TARGET_NEXT,
    FLOW_TARGET_VOICE_SAMPLE,
    FLOW_TARGET_START_PARTY,
};

bool FlowTargetCenter(RuntimeCase& test, int target, int index, int* x, int* y);
bool ClickFlowTarget(RuntimeCase& test, int target, int index = 0);
bool HoverFlowTarget(RuntimeCase& test, int target, int index = 0);

bool WaitTransitionObjects(RuntimeCase& test, bool present, unsigned long timeout_ms);

bool CharacterReturnCase(RuntimeCase& test);
bool MainGameStartCase(RuntimeCase& test);
