#pragma once

struct RuntimeObservation {
    unsigned char engine_ready;
    int menu_state;
    unsigned int region_set_enabled;
    unsigned int first_region;
    unsigned int last_region;
    unsigned char menu_seen;
    unsigned char main_game_entered;
    unsigned char case_passed;
    unsigned char timed_out;
};

enum RuntimePhase { RUNTIME_ENGINE_READY, RUNTIME_MAIN_MENU, RUNTIME_MAIN_GAME };
enum RuntimeFixtureId {
    FIXTURE_ENGINE_READY,
    FIXTURE_MAIN_MENU,
    FIXTURE_MONASTERY_PARTY,
    FIXTURE_MONASTERY_SINGLE_PARTY
};
enum RuntimeFixturePath { FIXTURE_PATH_NATURAL, FIXTURE_PATH_SHORTCUT };
enum RuntimeTier { RUNTIME_PR, RUNTIME_MAIN, RUNTIME_NIGHTLY };
enum RuntimeKind { RUNTIME_ACCEPTANCE, RUNTIME_INTEGRATION, RUNTIME_SEMANTIC };

class RuntimeCase;
typedef bool (*RuntimeCaseFn)(RuntimeCase& test);

struct RuntimeScenario {
    const char* name;
    RuntimePhase phase;
    RuntimeFixtureId fixture;
    RuntimeTier tier;
    RuntimeKind kind;
    unsigned int timeout_ms;
    RuntimeCaseFn case_run;
    /* Opt-in same-process batching: only cases whose fixture owns explicit
       cleanup may set this. */
    unsigned char batch;
};
