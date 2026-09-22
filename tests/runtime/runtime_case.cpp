/* Reusable integration-case API for the runtime test driver. Every read of
   product state goes through the game-thread executor; the driver thread
   only injects OS-level input and compares copied snapshots. */

#include "runtime_case.h"

#include "wiz8/regions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/xstatus.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/GameTimeAccumulator0043A910.h"
#include "wiz8/layouts/main_game_screen.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "wiz8/wiz8_windows.h"

#include "sgp.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static unsigned int g_ready_check_calls;
/* The scenario name that WIZ8_RUNTIME_* lines carry; RuntimeCase stamps it
   once at construction, matching the old g_scenario global's role. */
static const char* g_case_scenario = "runtime-case";

void MoveScenarioMouse(int client_x, int client_y)
{
    POINT point;
    point.x = client_x;
    point.y = client_y;
    if (ghWindow == NULL || !ClientToScreen(ghWindow, &point)) {
        fprintf(stderr, "runtime-test ClientToScreen failed: %lu\n", GetLastError());
        return;
    }
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    if (screen_width < 2)
        screen_width = 2;
    if (screen_height < 2)
        screen_height = 2;
    INPUT event;
    memset(&event, 0, sizeof(event));
    event.type = INPUT_MOUSE;
    event.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    event.mi.dx = (point.x * 65535) / (screen_width - 1);
    event.mi.dy = (point.y * 65535) / (screen_height - 1);
    SetForegroundWindow(ghWindow);
    SendInput(1, &event, sizeof(INPUT));
}

/* SGP's queue belongs to the game thread. SendInput reaches its WH_KEYBOARD
   hook; posting WM_KEYDOWN directly does not. The private display parks the
   pointer at the window centre, which sits on the Load Game item, so every
   key send first moves it off every enabled region: otherwise hover overrules
   the keyboard selection non-deterministically. */
bool PointHitsEnabledRegion(int x, int y)
{
    unsigned short px = static_cast<unsigned short>(x);
    unsigned short py = static_cast<unsigned short>(y);
    for (unsigned int set = 0; set < g_region_set_count; ++set) {
        if (g_region_sets[set].enabled != 1 ||
            g_region_sets[set].first_region > g_region_sets[set].last_region) {
            continue;
        }
        unsigned int first = g_region_sets[set].first_region;
        unsigned int last = g_region_sets[set].last_region;
        for (unsigned int region = first; region <= last && region < g_region_count; ++region) {
            if (RegionContainsPoint(region, px, py)) {
                return true;
            }
        }
    }
    return false;
}

void ParkMouseOutsideActiveRegions()
{
    RECT client;
    if (ghWindow == NULL || !GetClientRect(ghWindow, &client)) {
        return;
    }
    int width = client.right - client.left;
    int height = client.bottom - client.top;
    if (width < 1 || height < 1) {
        return;
    }
    int candidates[8][2] = {
        {width - 1, 0},          {0, 0},
        {width - 1, height - 1}, {0, height - 1},
        {width / 2, 0},          {width - 1, height / 2},
        {0, height / 2},         {width / 2, height - 1},
    };
    for (int index = 0; index < 8; ++index) {
        int x = candidates[index][0];
        int y = candidates[index][1];
        if (!PointHitsEnabledRegion(x, y)) {
            MoveScenarioMouse(x, y);
            return;
        }
    }
    for (int top_x = 0; top_x < width; ++top_x) {
        if (!PointHitsEnabledRegion(top_x, 0)) {
            MoveScenarioMouse(top_x, 0);
            return;
        }
    }
    for (int bottom_x = 0; bottom_x < width; ++bottom_x) {
        if (!PointHitsEnabledRegion(bottom_x, height - 1)) {
            MoveScenarioMouse(bottom_x, height - 1);
            return;
        }
    }
    MoveScenarioMouse(width - 1, 0);
}

/* The navigation cluster is always extended on PC keyboards; Wine's
   MapVirtualKey does not report the 0xe000 scancode prefix, so extendedness is
   decided from the virtual key itself. */
int IsExtendedScenarioKey(unsigned short key)
{
    switch (key) {
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_END:
    case VK_HOME:
    case VK_INSERT:
    case VK_DELETE:
        return 1;
    }
    return 0;
}

/* Held input uses the OS keyboard path, including SGP's hook, not driver-thread
   writes to gfKeyState or its event queue. The physical scan and extended bit
   distinguish dedicated arrows from the numeric keypad. */
void SendScenarioKeyHeld(unsigned short key, unsigned char release)
{
    ParkMouseOutsideActiveRegions();
    INPUT event;
    memset(&event, 0, sizeof(event));
    event.type = INPUT_KEYBOARD;
    event.ki.wVk = key;
    event.ki.wScan = static_cast<WORD>(MapVirtualKey(key, 0));
    if (IsExtendedScenarioKey(key)) {
        event.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    if (release) {
        event.ki.dwFlags |= KEYEVENTF_KEYUP;
    }
    SetForegroundWindow(ghWindow);
    if (SendInput(1, &event, sizeof(INPUT)) != 1) {
        fprintf(stderr,
                "WIZ8_RUNTIME_FAILURE scenario=%s step=input reason=sendinput-failed error=%lu\n",
                g_case_scenario, GetLastError());
        fflush(stderr);
    }
}

void CheckGameplayReadyOnGameThread(void* opaque)
{
    GameplayReadyCheck* check = static_cast<GameplayReadyCheck*>(opaque);
    check->calls = ++*(volatile unsigned int*)&g_ready_check_calls;
    // StartCombat and QuickSave require ground contact, not merely an entered screen.
    /* GetLevelDataFlag4 is set by the collision path while a motion step
       resolves ground contact. A party spawned exactly at rest may never
       collide, so the flag can stay clear indefinitely even though the game
       is otherwise up - report that separately so the driver can nudge the
       party into one real move. */
    /* world_update_blocked covers the modal stretch after main-game entry
       (arrival narration and friends): while it is up, party movement
       regions are disabled even though input is not blocked. */
    bool settled = g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                   g_pending_screen_state.id == -1 && g_mgs_keyboard != 0 && g_level_block != 0 &&
                   !g_level_block->review_transition_done_328 &&
                   !g_level_block->review_transition_active && g_level_data_00652dac != 0 &&
                   !IsScreenInputBlocked() && gXStatus.world_update_blocked == 0;
    check->screen = g_current_screen_state.id;
    check->pending = g_pending_screen_state.id;
    check->keyboard_present = g_mgs_keyboard != 0;
    check->level_block_present = g_level_block != 0;
    check->review_transition_done_328 =
        g_level_block != 0 ? g_level_block->review_transition_done_328 : -1;
    check->review_transition_active =
        g_level_block != 0 ? g_level_block->review_transition_active : -1;
    check->level_data_present = g_level_data_00652dac != 0;
    check->blocked = IsScreenInputBlocked();
    check->flags = g_level_data_00652dac != 0 ? g_level_data_00652dac->flags : 0;
    check->flag4_effective = IsLevelDataFlag4EffectivelySet();
    /* GetCameraPosition dereferences the camera record unconditionally; it
       does not exist until the world does. */
    srVector3T<float> camera;
    camera.Set(0, 0, 0);
    if (g_gd_camera_65a0f8 != 0) {
        GetCameraPosition(&camera);
    }
    check->camera_x = camera.x;
    check->camera_y = camera.y;
    check->camera_z = camera.z;
    check->timer_flags =
        g_game_time_accumulator_6598bc != 0 ? g_game_time_accumulator_6598bc->m_flags : 0;
    check->timer_paused = g_shared_timer_paused;
    check->timer_d1 = g_shared_timer_flag_d1;
    check->timer_d2 = g_shared_timer_flag_d2;
    check->timer_scale = g_game_time_accumulator_6598bc != 0
                             ? g_game_time_accumulator_6598bc->GetFrameDelta()
                             : -1.0f;
    check->ground_latch = g_environ_ground_latch_00652db8;
    check->world_update_blocked = gXStatus.world_update_blocked;
    /* flag4 is the per-frame walkable-contact bit: ApplyEnvironContact sets
       it while the party capsule touches ground and ApplyCameraMotion clears
       it at the top of each motion step. If it stays clear the party has no
       ground contact at all - input may still register but world motion is
       dead, so readiness must wait for the collision pipeline to resolve. */
    check->ready = settled && GetLevelDataFlag4() != 0;
    check->waiting_on_ground = settled && !check->ready;
}

struct GameplaySnapshotRequest {
    GameplaySnapshot* snapshot;
    unsigned short held_key;
};

void ReadGameplaySnapshotOnGameThread(void* opaque)
{
    GameplaySnapshotRequest* request = static_cast<GameplaySnapshotRequest*>(opaque);
    GameplaySnapshot* s = request->snapshot;
    W8CameraAngleRecord yaw, pitch;
    s->position.Set(0, 0, 0);
    s->yaw = s->input_motion = s->world_motion = 0;
    if (g_level_data_00652dac != 0) {
        GetCameraPosition(&s->position);
        GetCameraOrientation(yaw, pitch);
        s->yaw = yaw[0];
        s->input_motion = g_level_data_00652dac->vector_40.Length();
        s->world_motion = g_level_data_00652dac->vector_a0.Length();
    }
    s->screen = g_current_screen_state.id;
    s->pending = g_pending_screen_state.id;
    s->combat = gXStatus.fCombatMode != 0;
    s->movement_ui = gXStatus.fPartyMovementUi != 0;
    s->movement_budget = g_level_block != 0 ? g_level_block->move_budget_2dc : 0;
    s->round_active = g_combat_state != 0 ? g_combat_state->combat_over_000 : 0;
    s->party_action_status = g_combat_state != 0 ? g_combat_state->uiCurrentPartyActionStatus : 0;
    s->action_status = g_combat_state != 0 ? g_combat_state->eCombatActionStatus : 0;
    s->action_monster = g_combat_state != 0 && g_combat_state->pActionMonsterInfo != 0
                            ? g_combat_state->pActionMonsterInfo->location_id
                            : -1;
    s->modal_owner_present = g_modal_owner_0068edd0 != 0;
    s->world_update_blocked = gXStatus.world_update_blocked;
    s->world_render_flags = g_level_block != 0 ? g_level_block->world_render_flags : 0;
    s->held_key = request->held_key;
    s->held_key_down = request->held_key != 0 && gfKeyState[request->held_key] != 0;
    s->taken_ms = GetTickCount();
}

static void SendBindingKeys(const CommandBinding& binding, unsigned char release)
{
    static const unsigned short keys[] = {VK_SHIFT, VK_CONTROL, VK_MENU};
    static const unsigned int flags[] = {SHIFT_DOWN, CTRL_DOWN, ALT_DOWN};
    if (!release) {
        for (int i = 0; i < 3; ++i) {
            if (binding.modifiers & flags[i])
                SendScenarioKeyHeld(keys[i], 0);
        }
    }
    SendScenarioKeyHeld(binding.key, release);
    if (release) {
        for (int i = 2; i >= 0; --i) {
            if (binding.modifiers & flags[i])
                SendScenarioKeyHeld(keys[i], 1);
        }
    }
}

struct BindingQuery {
    int command;
    CommandBinding* binding;
};

static void ReadBindingOnGameThread(void* opaque)
{
    BindingQuery* query = static_cast<BindingQuery*>(opaque);
    int index = g_mgs_keyboard != 0 ? g_mgs_keyboard->FindBinding(query->command) : -1;
    const MGSKeyBinding* binding = index >= 0 ? g_mgs_keyboard->GetBinding(index) : 0;
    if (binding != 0) {
        query->binding->command = binding->command;
        query->binding->key = binding->key;
        query->binding->modifiers = binding->modifiers;
    } else {
        query->binding->command = query->command;
        query->binding->key = 0;
        query->binding->modifiers = 0;
    }
}

HeldCommand::HeldCommand(RuntimeCase& test, int command)
    : test_(test), command_(command), held_(false), consumed_(false), last_repeat_(0)
{
    binding_.command = command;
    binding_.key = 0;
    binding_.modifiers = 0;
}

HeldCommand::~HeldCommand()
{
    release();
}

bool HeldCommand::begin()
{
    if (!test_.resolve_binding(command_, binding_, "input") || binding_.key == 0) {
        test_.fail("input", "binding-missing");
        return false;
    }
    SendBindingKeys(binding_, 0);
    held_ = true;
    last_repeat_ = GetTickCount();
    test_.set_held_key(binding_.key);
    return true;
}

void HeldCommand::repeat()
{
    if (!held_ || binding_.key == 0) {
        return;
    }
    unsigned long now = GetTickCount();
    if (now - last_repeat_ >= 30) {
        last_repeat_ = now;
        SendScenarioKeyHeld(binding_.key, 0);
    }
}

void HeldCommand::release()
{
    if (!held_) {
        return;
    }
    held_ = false;
    SendBindingKeys(binding_, 1);
    if (test_.held_key() == binding_.key) {
        test_.set_held_key(0);
    }
}

bool HeldCommand::consumed() const
{
    return consumed_;
}

unsigned short HeldCommand::key() const
{
    return binding_.key;
}

void HeldCommand::note_snapshot(const GameplaySnapshot& now)
{
    if (now.held_key == binding_.key && now.held_key_down != 0) {
        consumed_ = true;
    }
}

RuntimeCase::RuntimeCase(const char* name, unsigned long budget_ms)
    : name_(name), started_(GetTickCount()), deadline_(GetTickCount() + budget_ms),
      last_step_("none"), failed_(false), failure_step_("none"), failure_reason_("none"),
      has_snapshot_(false), held_key_(0), finished_(false)
{
    expected_[0] = 0;
    memset(&last_snapshot_, 0, sizeof(last_snapshot_));
    memset(&last_ready_check_, 0, sizeof(last_ready_check_));
    g_case_scenario = name;
}

const char* RuntimeCase::name() const
{
    return name_;
}

unsigned long RuntimeCase::remaining_ms() const
{
    unsigned long now = GetTickCount();
    if (now >= deadline_) {
        return 0;
    }
    return deadline_ - now;
}

unsigned long RuntimeCase::elapsed_ms() const
{
    return GetTickCount() - started_;
}

void RuntimeCase::step(const char* step)
{
    fprintf(stderr, "WIZ8_RUNTIME_STEP scenario=%s step=%s state=pass elapsed_ms=%lu\n", name_,
            step, elapsed_ms());
    fflush(stderr);
    last_step_ = step;
    expected_[0] = 0;
}

const char* RuntimeCase::last_step() const
{
    return last_step_;
}

bool RuntimeCase::fail(const char* step, const char* reason)
{
    if (failed_) {
        fprintf(stderr, "runtime-case %s: also step=%s reason=%s\n", name_, step, reason);
        fflush(stderr);
        return false;
    }
    failed_ = true;
    failure_step_ = step;
    failure_reason_ = reason;
    fprintf(stderr, "WIZ8_RUNTIME_FAILURE scenario=%s step=%s reason=%s line=0\n", name_, step,
            reason);
    fprintf(stderr, "runtime-case %s: last_completed=%s elapsed_ms=%lu remaining_ms=%lu\n", name_,
            last_step_, elapsed_ms(), remaining_ms());
    if (expected_[0] != 0) {
        fprintf(stderr, "runtime-case %s: expected=%s\n", name_, expected_);
    }
    if (has_snapshot_) {
        const GameplaySnapshot& s = last_snapshot_;
        fprintf(stderr,
                "runtime-case %s: observed screen=%d pending=%d position=(%.2f %.2f %.2f) "
                "yaw=%.3f combat=%u movement_ui=%u budget=%d input=%.2f world=%.2f "
                "modal=%u world_blocked=%u render_flags=%02x held_key=%u held_down=%u "
                "snapshot_age_ms=%lu\n",
                name_, s.screen, s.pending, s.position.x, s.position.y, s.position.z, s.yaw,
                s.combat ? 1u : 0u, s.movement_ui ? 1u : 0u, s.movement_budget, s.input_motion,
                s.world_motion, s.modal_owner_present, s.world_update_blocked, s.world_render_flags,
                s.held_key, s.held_key_down, GetTickCount() - s.taken_ms);
    } else {
        fprintf(stderr, "runtime-case %s: observed none\n", name_);
    }
    fprintf(stderr, "runtime-case %s: reproduce uv run wiz8 runtime-test --scenario %s\n", name_,
            name_);
    fflush(stderr);
    /* The runner owns the game lifecycle; a case failure only reports. */
    return false;
}

bool RuntimeCase::failed() const
{
    return failed_;
}

const char* RuntimeCase::failure_step() const
{
    return failure_step_;
}

const char* RuntimeCase::failure_reason() const
{
    return failure_reason_;
}

void RuntimeCase::expected(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _vsnprintf(expected_, sizeof(expected_) - 1, fmt, args);
    expected_[sizeof(expected_) - 1] = 0;
    va_end(args);
}

bool RuntimeCase::on_game_thread(const char* step, RuntimeGameThreadCallback cb, void* ctx,
                                 unsigned long budget_ms)
{
    unsigned long remaining = remaining_ms();
    if (budget_ms > remaining) {
        budget_ms = remaining;
    }
    unsigned long waited = GetTickCount();
    if (RunOnGameThread(cb, ctx, budget_ms)) {
        return true;
    }
    fprintf(stderr,
            "WIZ8_RUNTIME_SYNC scenario=%s step=%s state=game-thread-unresponsive "
            "waited_ms=%lu\n",
            name_, step, GetTickCount() - waited);
    fflush(stderr);
    fail(step, "game-thread-unresponsive");
    return false;
}

bool RuntimeCase::snapshot(GameplaySnapshot& out, const char* step)
{
    GameplaySnapshotRequest request;
    request.snapshot = &out;
    request.held_key = held_key_;
    if (!on_game_thread(step, ReadGameplaySnapshotOnGameThread, &request, 5000)) {
        return false;
    }
    last_snapshot_ = out;
    has_snapshot_ = true;
    return true;
}

const GameplaySnapshot& RuntimeCase::last_snapshot() const
{
    return last_snapshot_;
}

bool RuntimeCase::has_snapshot() const
{
    return has_snapshot_;
}

bool RuntimeCase::resolve_binding(int command, CommandBinding& out, const char* step)
{
    BindingQuery query;
    query.command = command;
    query.binding = &out;
    if (!on_game_thread(step, ReadBindingOnGameThread, &query, 5000)) {
        return false;
    }
    return out.key != 0;
}

bool RuntimeCase::tap(int command, const char* step)
{
    CommandBinding binding;
    memset(&binding, 0, sizeof(binding));
    if (!resolve_binding(command, binding, step)) {
        /* resolve_binding already reported an unresponsive game thread; the
           remaining false path is a resolved-but-empty binding. */
        if (!failed()) {
            fail(step, "binding-missing");
        }
        return false;
    }
    SendBindingKeys(binding, 0);
    SendBindingKeys(binding, 1);
    return true;
}

bool RuntimeCase::wait_until(const char* condition, unsigned long budget_ms, RuntimeConditionFn fn,
                             void* ctx, HeldCommand* held, unsigned long poll_ms)
{
    unsigned long started = GetTickCount();
    while (GetTickCount() - started < budget_ms && remaining_ms() > 0 && gfProgramIsRunning) {
        if (held != 0) {
            held->repeat();
        }
        GameplaySnapshot now;
        GameplaySnapshotRequest request;
        request.snapshot = &now;
        request.held_key = held_key_;
        unsigned long budget = remaining_ms();
        if (budget > 5000) {
            budget = 5000;
        }
        if (!on_game_thread(condition, ReadGameplaySnapshotOnGameThread, &request, budget)) {
            return false;
        }
        last_snapshot_ = now;
        has_snapshot_ = true;
        if (held != 0) {
            held->note_snapshot(now);
        }
        if (fn(now, ctx)) {
            expected_[0] = 0;
            return true;
        }
        Sleep(poll_ms);
    }
    const char* reason =
        held != 0 && !held->consumed() ? "input-not-consumed" : "condition-not-met";
    if (expected_[0] == 0) {
        expected("%s", condition);
    }
    fail(condition, reason);
    return false;
}

GameplayWait RuntimeCase::wait_gameplay_ready(unsigned long budget_ms, const char* step)
{
    unsigned long started = GetTickCount();
    HeldCommand walk(*this, W8_MGS_COMMAND_MOVE_FORWARD);
    bool walking = false;
    while (GetTickCount() - started < budget_ms && gfProgramIsRunning) {
        GameplayReadyCheck check;
        memset(&check, 0, sizeof(check));
        unsigned long elapsed = GetTickCount() - started;
        if (elapsed >= budget_ms) {
            break;
        }
        unsigned long remaining = budget_ms - elapsed;
        if (!on_game_thread("gameplay-ready", CheckGameplayReadyOnGameThread, &check, remaining)) {
            walk.release();
            return GAMEPLAY_EXECUTOR_UNRESPONSIVE;
        }
        last_ready_check_ = check;
        if (check.ready) {
            walk.release();
            return GAMEPLAY_READY;
        }
        /* Ground contact only latches while a move resolves collision, so a
           party that spawned already at rest needs a real step before
           StartCombat will accept it. Injected keys do not autorepeat, so
           keep re-sending the press while waiting for contact. A missing
           binding here is the nudge only - leave it to the case actions. */
        if (check.waiting_on_ground && !walking) {
            CommandBinding probe;
            if (resolve_binding(W8_MGS_COMMAND_MOVE_FORWARD, probe, step)) {
                walking = walk.begin();
            }
        }
        if (walking) {
            walk.repeat();
        }
        if (check.screen == W8_SCREEN_INTRO) {
            SendScenarioKeyHeld(VK_ESCAPE, 0);
            SendScenarioKeyHeld(VK_ESCAPE, 1);
        }
        Sleep(20);
    }
    walk.release();
    return GAMEPLAY_NOT_READY;
}

const GameplayReadyCheck& RuntimeCase::last_ready_check() const
{
    return last_ready_check_;
}

void RuntimeCase::finish(bool passed)
{
    if (finished_) {
        return;
    }
    finished_ = true;
    fprintf(stderr, "WIZ8_RUNTIME_CASE scenario=%s outcome=%s last_step=%s elapsed_ms=%lu\n", name_,
            passed ? "pass" : "fail", last_step_, elapsed_ms());
    fflush(stderr);
}

unsigned short RuntimeCase::held_key() const
{
    return held_key_;
}

void RuntimeCase::set_held_key(unsigned short key)
{
    held_key_ = key;
}
