#pragma once

/* Enable the pinned SDK's SendInput declarations for the harness only. */
#define _WIN32_WINNT 0x0500

#include "game_thread_executor.h"
#include "surrender/srMath.h"

struct GameplaySnapshot {
    srVector3T<float> position;
    float yaw;
    float input_motion;
    float world_motion;
    int screen;
    int pending;
    bool combat;
    bool movement_ui;
    unsigned int round_active;
    unsigned int party_action_status;
    int action_status;
    int action_monster;
    int movement_budget;
    /* Diagnostics copied on the game thread for failure reporting; these
       used to be live reads in MoveParty's failure print. */
    unsigned char modal_owner_present;
    unsigned int world_update_blocked;
    unsigned char world_render_flags;
    /* gfKeyState[held_key] sampled on the game thread when held_key != 0. */
    unsigned short held_key;
    unsigned char held_key_down;
    /* GetTickCount() at capture time, for snapshot-age reporting. */
    unsigned long taken_ms;
};

/* A bound command copied out of MGSKeyboard on the game thread; the driver
   never keeps a pointer into the binding table. */
struct CommandBinding {
    int command;
    unsigned short key;
    unsigned short modifiers;
};

struct GameplayReadyCheck {
    bool ready;
    bool waiting_on_ground;
    unsigned int calls;
    int screen;
    int pending;
    int keyboard_present;
    int level_block_present;
    int review_transition_done_328;
    int review_transition_active;
    int level_data_present;
    unsigned int flags;
    int flag4_effective;
    int blocked;
    float camera_x;
    float camera_y;
    float camera_z;
    unsigned int timer_flags;
    int timer_paused;
    int timer_d1;
    int timer_d2;
    float timer_scale;
    int ground_latch;
    unsigned int world_update_blocked;
};

enum GameplayWait { GAMEPLAY_READY, GAMEPLAY_NOT_READY, GAMEPLAY_EXECUTOR_UNRESPONSIVE };

class RuntimeCase;

typedef bool (*RuntimeConditionFn)(const GameplaySnapshot& now, void* context);

/* Scoped ownership of one held bound command: resolving the binding and the
   key-state observation happen on the game thread, while the OS-level held
   key events are re-sent like a physical key (injected input does not
   autorepeat). release() is idempotent and the destructor releases a key
   still down on every exit path. */
class HeldCommand {
public:
    HeldCommand(RuntimeCase& test, int command);
    ~HeldCommand();

    bool begin();
    void repeat();
    void release();
    bool consumed() const;
    unsigned short key() const;
    void note_snapshot(const GameplaySnapshot& now);

private:
    RuntimeCase& test_;
    int command_;
    CommandBinding binding_;
    bool held_;
    bool consumed_;
    unsigned long last_repeat_;
};

class RuntimeCase {
public:
    RuntimeCase(const char* name, unsigned long budget_ms);

    const char* name() const;
    unsigned long remaining_ms() const;
    unsigned long elapsed_ms() const;

    void step(const char* step);
    const char* last_step() const;

    bool fail(const char* step, const char* reason);
    bool failed() const;
    const char* failure_step() const;
    const char* failure_reason() const;
    void expected(const char* fmt, ...);

    bool on_game_thread(const char* step, RuntimeGameThreadCallback cb, void* ctx,
                        unsigned long budget_ms);
    bool snapshot(GameplaySnapshot& out, const char* step = "snapshot");
    const GameplaySnapshot& last_snapshot() const;
    bool has_snapshot() const;

    bool resolve_binding(int command, CommandBinding& out, const char* step);
    bool tap(int command, const char* step);

    bool wait_until(const char* condition, unsigned long budget_ms, RuntimeConditionFn fn,
                    void* ctx, HeldCommand* held = 0, unsigned long poll_ms = 10);

    GameplayWait wait_gameplay_ready(unsigned long budget_ms, const char* step);
    const GameplayReadyCheck& last_ready_check() const;

    /* Schedules fn on the game thread, then records a pass step or fails with
       required-invariant-failed. */
    bool run_invariant(const char* step, bool (*fn)(void* ctx), void* ctx,
                       unsigned long budget_ms = 10000);

    void set_fixture(const char* name, const char* path);
    void finish(bool passed);

    unsigned short held_key() const;
    void set_held_key(unsigned short key);

private:
    const char* name_;
    unsigned long started_;
    unsigned long deadline_;
    const char* last_step_;
    bool failed_;
    const char* failure_step_;
    const char* failure_reason_;
    char expected_[256];
    GameplaySnapshot last_snapshot_;
    bool has_snapshot_;
    GameplayReadyCheck last_ready_check_;
    unsigned short held_key_;
    bool finished_;
    const char* fixture_name_;
    const char* fixture_path_;
};

/* Free helpers moved out of wiz8_runtime_test.cpp: every game-state touch
   below happens on the game thread; only OS input injection runs on the
   driver. */
void SendScenarioKeyHeld(unsigned short key, unsigned char release);
int IsExtendedScenarioKey(unsigned short key);
void ParkMouseOutsideActiveRegions();
bool PointHitsEnabledRegion(int x, int y);
void MoveScenarioMouse(int client_x, int client_y);
void CheckGameplayReadyOnGameThread(void* opaque);
void ReadGameplaySnapshotOnGameThread(void* opaque);

#define RT_REQUIRE(test, expr)                                                                     \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            if (!(test).failed())                                                                  \
                (test).fail(#expr, "requirement-failed");                                          \
            return false;                                                                          \
        }                                                                                          \
    } while (0)
