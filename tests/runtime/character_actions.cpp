#include "character_actions.h"

#include "gameplay_actions.h"

#include "wiz8/regions.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/PartyImport.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/IntroScreen.h"
#include "wiz8/local_screens/PartySelectionScreen.h"
#include "wiz8/engine_code/Video2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* Every field is read on the game thread; the driver only sees this copy. */
static void ReadCharacterFlowOnGameThread(void* opaque)
{
    CharacterFlowState* state = static_cast<CharacterFlowState*>(opaque);
    memset(state, 0, sizeof(*state));
    state->current = g_current_screen_state.id;
    state->pending = g_pending_screen_state.id;
    state->party_selection_set = (int)g_party_selection_character_region_set_69c4f0;
    state->party_selection_ready = state->current == W8_SCREEN_PARTY_SELECTION &&
                                   state->pending == -1 && state->party_selection_set != 0 &&
                                   state->party_selection_set < (int)g_region_set_count &&
                                   g_region_sets[state->party_selection_set].enabled != 0;
    state->left_action_set = (int)g_party_selection_left_action_region_set_69c504;
    state->bottom_action_set = (int)g_party_selection_bottom_action_region_set_69c508;
    state->transition_objects = HasScreenTransitionObjects() ? 1 : 0;
    state->active_characters = CountActiveCharacters();
    state->captured_region = g_captured_region_index;
    state->intro_index = g_intro_video_index;
    state->skip_loose_check = g_status_685170.skip_loose_character_check_2444;
    state->wiz7_ending = g_wiz7_ending_68de50;

    W8CharacterScreen* screen = g_character_screen_0069c2e8;
    state->character_screen_present = screen != 0;
    if (screen == 0) {
        return;
    }
    state->page_index = screen->m_page_index_00c;
    for (int page = 0; page < 4; ++page) {
        state->page_present[page] = screen->m_pages_1b0c[page] != 0;
    }
    state->stats_set_enabled = g_character_stats_region_set_0069c550 != 0 &&
                               g_character_stats_region_set_0069c550 < g_region_set_count &&
                               g_region_sets[g_character_stats_region_set_0069c550].enabled != 0;
    state->dialog_present = screen->m_dialog_1b1c != 0;
    memcpy(state->name, screen->m_character_018.name, sizeof(state->name));
    memcpy(state->name_part_2, screen->m_character_018.name_part_2, sizeof(state->name_part_2));

    W8CharacterCreationState* creation = &screen->m_creation_state_187c;
    state->attributes_complete = creation->attributes_complete ? 1 : 0;
    state->skills_complete = creation->skills_complete ? 1 : 0;

    W8CharacterPage005EF778* stats_page =
        static_cast<W8CharacterPage005EF778*>(screen->m_pages_1b0c[0]);
    if (state->page_present[0] && stats_page != 0) {
        state->profession_index = stats_page->m_profession_row_07c != 0
                                      ? stats_page->m_profession_row_07c->m_index_004
                                      : -1;
        state->race_index =
            stats_page->m_race_row_080 != 0 ? stats_page->m_race_row_080->m_index_004 : -1;
        state->gender_index =
            stats_page->m_gender_row_084 != 0 ? stats_page->m_gender_row_084->m_index_004 : -1;
        state->stat_count = stats_page->m_entries_04c.count;
        if (state->stat_count > 16) {
            state->stat_count = 16;
        }
        for (int index = 0; index < state->stat_count; ++index) {
            W8CharacterPageEntry* entry = stats_page->m_entries_04c.data[index];
            FlowEntryState* out = &state->stat_entries[index];
            if (entry == 0) {
                continue;
            }
            out->enabled = entry->m_enabled_03a ? 1 : 0;
            out->increment_allowed = entry->m_increment_allowed_03b ? 1 : 0;
            out->spent = entry->m_second_024 != 0 ? *entry->m_second_024 : 0;
            out->limit = entry->m_third_028 != 0 ? *entry->m_third_028 : 0;
        }
        state->stat_entries_enabled = state->stat_count > 0 && state->stat_entries[0].enabled;
    }

    W8CharacterPage005EF5C8* skills_page =
        static_cast<W8CharacterPage005EF5C8*>(screen->m_pages_1b0c[2]);
    if (state->page_present[2] && skills_page != 0) {
        state->skill_count = skills_page->m_entries_04c.count;
        if (state->skill_count > 16) {
            state->skill_count = 16;
        }
        for (int index = 0; index < state->skill_count; ++index) {
            W8CharacterPageEntry* entry = skills_page->m_entries_04c.data[index];
            FlowEntryState* out = &state->skill_entries[index];
            if (entry == 0) {
                continue;
            }
            out->enabled = entry->m_enabled_03a ? 1 : 0;
            out->increment_allowed = entry->m_increment_allowed_03b ? 1 : 0;
            out->spent = entry->m_second_024 != 0 ? *entry->m_second_024 : 0;
            out->limit = entry->m_third_028 != 0 ? *entry->m_third_028 : 0;
        }
    }

    W8CharacterPage005EF57C* final_page =
        static_cast<W8CharacterPage005EF57C*>(screen->m_pages_1b0c[3]);
    if (state->page_present[3] && final_page != 0) {
        state->final_prepared = final_page->m_prepared_06c != 0;
    }
}

/* Region index for one logical target. Returns -1 when the control or its
   region is not currently registered. */
static int FlowRegionForTarget(int target, int index)
{
    W8CharacterScreen* screen = g_character_screen_0069c2e8;
    W8TextControl* control = 0;
    unsigned int region_set = 0;
    int help_text_id = 0;
    switch (target) {
    case FLOW_TARGET_NEW_GAME:
        if (g_region_set_count <= 1) {
            return -1;
        }
        /* New Game is the second live menu region. */
        return (int)g_region_sets[1].first_region + 1;
    case FLOW_TARGET_CREATE_CHARACTER:
        region_set = g_party_selection_left_action_region_set_69c504;
        if (region_set == 0 || region_set >= g_region_set_count) {
            return -1;
        }
        /* The left action panel registers "Create Character" first. */
        return (int)g_region_sets[region_set].first_region;
    case FLOW_TARGET_PROFESSION_NEXT:
    case FLOW_TARGET_RACE_NEXT:
    case FLOW_TARGET_GENDER_NEXT:
    case FLOW_TARGET_STAT_INCREMENT:
    case FLOW_TARGET_SKILL_INCREMENT:
    case FLOW_TARGET_SKILL_HELP:
    case FLOW_TARGET_NEXT:
        if (screen == 0) {
            return -1;
        }
        break;
    default:
        break;
    }
    if (target == FLOW_TARGET_PROFESSION_NEXT || target == FLOW_TARGET_RACE_NEXT ||
        target == FLOW_TARGET_GENDER_NEXT || target == FLOW_TARGET_STAT_INCREMENT) {
        W8CharacterPage005EF778* stats_page =
            static_cast<W8CharacterPage005EF778*>(screen->m_pages_1b0c[0]);
        if (stats_page == 0) {
            return -1;
        }
        if (target == FLOW_TARGET_PROFESSION_NEXT) {
            control = stats_page->m_profession_row_07c != 0
                          ? stats_page->m_profession_row_07c->m_increment_020
                          : 0;
        } else if (target == FLOW_TARGET_RACE_NEXT) {
            control =
                stats_page->m_race_row_080 != 0 ? stats_page->m_race_row_080->m_increment_020 : 0;
        } else if (target == FLOW_TARGET_GENDER_NEXT) {
            control = stats_page->m_gender_row_084 != 0
                          ? stats_page->m_gender_row_084->m_increment_020
                          : 0;
        } else if (index >= 0 && index < stats_page->m_entries_04c.count &&
                   stats_page->m_entries_04c.data[index] != 0) {
            control = stats_page->m_entries_04c.data[index]->m_increment_008;
        }
        return control != 0 ? control->m_region : -1;
    }
    if (target == FLOW_TARGET_SKILL_INCREMENT || target == FLOW_TARGET_SKILL_HELP) {
        W8CharacterPage005EF5C8* skills_page =
            static_cast<W8CharacterPage005EF5C8*>(screen->m_pages_1b0c[2]);
        if (skills_page == 0 || index < 0 || index >= skills_page->m_entries_04c.count ||
            skills_page->m_entries_04c.data[index] == 0) {
            return -1;
        }
        W8CharacterPageEntry* entry = skills_page->m_entries_04c.data[index];
        control =
            target == FLOW_TARGET_SKILL_INCREMENT ? entry->m_increment_008 : entry->m_help_010;
        return control != 0 ? control->m_region : -1;
    }
    if (target == FLOW_TARGET_NEXT) {
        control = screen->m_next_1af8;
        return control != 0 ? control->m_region : -1;
    }
    if (target == FLOW_TARGET_VOICE_SAMPLE) {
        region_set = g_character_page4_region_set_0069c52c;
        help_text_id = 0xf5;
    } else if (target == FLOW_TARGET_START_PARTY) {
        region_set = g_party_selection_bottom_action_region_set_69c508;
        help_text_id = 0x6cb;
    } else {
        return -1;
    }
    if (region_set == 0 || region_set >= g_region_set_count) {
        return -1;
    }
    unsigned int first = g_region_sets[region_set].first_region;
    unsigned int last = g_region_sets[region_set].last_region;
    for (unsigned int region = first; region <= last && region < g_region_count; ++region) {
        if (g_regions[region].help_text_id == help_text_id) {
            return (int)region;
        }
    }
    return -1;
}

struct FlowTargetQuery {
    int target;
    int index;
    int ok;
    int x;
    int y;
};

static void ResolveFlowTargetOnGameThread(void* opaque)
{
    FlowTargetQuery* query = static_cast<FlowTargetQuery*>(opaque);
    query->ok = 0;
    int region = FlowRegionForTarget(query->target, query->index);
    if (region < 0 || (unsigned int)region >= g_region_count) {
        return;
    }
    W8Region* bounds = &g_regions[region];
    query->x = (bounds->x1 + bounds->x2) / 2;
    query->y = (bounds->y1 + bounds->y2) / 2;
    query->ok = 1;
}

bool ReadCharacterFlow(RuntimeCase& test, CharacterFlowState& out, const char* step)
{
    memset(&out, 0, sizeof(out));
    return test.on_game_thread(step, ReadCharacterFlowOnGameThread, &out, 60000);
}

bool WaitCharacterFlow(RuntimeCase& test, const char* step, unsigned long budget_ms,
                       bool (*predicate)(const CharacterFlowState& state, void* ctx), void* ctx)
{
    unsigned int started = GetTickCount();
    CharacterFlowState state;
    while (GetTickCount() - started < budget_ms) {
        if (!ReadCharacterFlow(test, state, step)) {
            return false;
        }
        if (predicate(state, ctx)) {
            test.step(step);
            return true;
        }
        Sleep(10);
    }
    fprintf(stderr,
            "runtime-test flow-wait: step=%s screen=%d pending=%d page=%d pages=%d%d%d%d "
            "attrs=%d skills=%d dialog=%u captured=%d intro=%lu\n",
            step, state.current, state.pending, state.page_index, state.page_present[0],
            state.page_present[1], state.page_present[2], state.page_present[3],
            state.attributes_complete, state.skills_complete, state.dialog_present,
            state.captured_region, state.intro_index);
    fflush(stderr);
    return test.fail(step, "condition-not-met");
}

bool FlowTargetCenter(RuntimeCase& test, int target, int index, int* x, int* y)
{
    FlowTargetQuery query;
    query.target = target;
    query.index = index;
    query.ok = 0;
    if (!test.on_game_thread("flow-target", ResolveFlowTargetOnGameThread, &query, 60000) ||
        !query.ok) {
        return false;
    }
    *x = query.x;
    *y = query.y;
    return true;
}

bool ClickFlowTarget(RuntimeCase& test, int target, int index)
{
    int x;
    int y;
    if (!FlowTargetCenter(test, target, index, &x, &y)) {
        return false;
    }
    SendScenarioMouseClick(x, y);
    return true;
}

bool HoverFlowTarget(RuntimeCase& test, int target, int index)
{
    int x;
    int y;
    if (!FlowTargetCenter(test, target, index, &x, &y)) {
        return false;
    }
    MoveScenarioMouse(x, y);
    return true;
}

static bool TransitionObjectsMatch(const CharacterFlowState& state, void* ctx)
{
    return (state.transition_objects != 0) == (*(int*)ctx != 0);
}

/* The tooltip's owning object count is the structural marker; the pixel
   output is not part of this assertion. */
bool WaitTransitionObjects(RuntimeCase& test, bool present, unsigned long timeout_ms)
{
    int wanted = present ? 1 : 0;
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < timeout_ms) {
        CharacterFlowState state;
        if (!ReadCharacterFlow(test, state)) {
            return false;
        }
        if (TransitionObjectsMatch(state, &wanted)) {
            return true;
        }
        Sleep(5);
    }
    return false;
}

static bool FlowPartySelectionReady(const CharacterFlowState& state, void*)
{
    return state.party_selection_ready != 0;
}

static bool FlowCharacterEntered(const CharacterFlowState& state, void*)
{
    return state.current == W8_SCREEN_CHARACTER && state.pending == -1 &&
           state.character_screen_present && state.page_present[0] && state.stats_set_enabled;
}

static bool FlowProfessionSet(const CharacterFlowState& state, void*)
{
    return state.profession_index != -1;
}

static bool FlowRaceSet(const CharacterFlowState& state, void*)
{
    return state.race_index != -1;
}

static bool FlowGenderSet(const CharacterFlowState& state, void*)
{
    return state.gender_index != -1;
}

static bool FlowStatEntriesEnabled(const CharacterFlowState& state, void*)
{
    return state.stat_entries_enabled != 0;
}

static bool FlowPageIs(const CharacterFlowState& state, void* ctx)
{
    return state.character_screen_present && state.page_index == *(int*)ctx;
}

static bool FlowSkillsReady(const CharacterFlowState& state, void*)
{
    return state.page_index == 2 && state.skill_count > 0 && state.skill_entries[0].enabled;
}

static bool FlowFinalPageDrawn(const CharacterFlowState& state, void*)
{
    return state.page_index == 3 && state.page_present[3] && !state.final_prepared;
}

static bool FlowNameTyped(const CharacterFlowState& state, void*)
{
    return wcscmp(state.name_part_2, L"probe") == 0 && wcscmp(state.name, L"name") == 0;
}

static bool FlowDialogOpen(const CharacterFlowState& state, void* ctx)
{
    return state.dialog_present == *(int*)ctx;
}

static bool FlowPartySelectionScreen(const CharacterFlowState& state, void*)
{
    return state.current == W8_SCREEN_PARTY_SELECTION && state.pending == -1;
}

static bool FlowCharacterInParty(const CharacterFlowState& state, void*)
{
    return state.active_characters != 0;
}

static bool FlowMainMenuReturned(const CharacterFlowState& state, void*)
{
    return state.current == W8_SCREEN_MAIN_MENU && state.pending == -1;
}

/* Spend one allocation page's pool through each entry's own increment
   control until the page itself reports the allocation complete. `stats`
   selects the entry table and completion flag to poll. */
static bool SpendFlowPool(RuntimeCase& test, const char* step, int skills_page)
{
    unsigned int started = GetTickCount();
    bool progress = true;
    CharacterFlowState state;
    while (progress && GetTickCount() - started < 90000) {
        progress = false;
        if (!ReadCharacterFlow(test, state, step)) {
            return false;
        }
        int complete = skills_page ? state.skills_complete : state.attributes_complete;
        if (complete) {
            return true;
        }
        int count = skills_page ? state.skill_count : state.stat_count;
        FlowEntryState* entries = skills_page ? state.skill_entries : state.stat_entries;
        for (int index = 0; index < count; ++index) {
            FlowEntryState* entry = &entries[index];
            if (!entry->enabled || (skills_page && !entry->increment_allowed)) {
                continue;
            }
            while (entry->spent < entry->limit) {
                int before = entry->spent;
                if (!ClickFlowTarget(test,
                                     skills_page ? FLOW_TARGET_SKILL_INCREMENT
                                                 : FLOW_TARGET_STAT_INCREMENT,
                                     index)) {
                    break;
                }
                unsigned int click_started = GetTickCount();
                bool moved = false;
                while (GetTickCount() - click_started < 1000) {
                    Sleep(5);
                    if (!ReadCharacterFlow(test, state, step)) {
                        return false;
                    }
                    complete = skills_page ? state.skills_complete : state.attributes_complete;
                    entry = skills_page ? &state.skill_entries[index] : &state.stat_entries[index];
                    if (entry->spent != before || complete) {
                        moved = entry->spent != before;
                        break;
                    }
                }
                if (moved) {
                    progress = true;
                }
                if (complete || !moved) {
                    break;
                }
            }
            if (skills_page ? state.skills_complete : state.attributes_complete) {
                return true;
            }
        }
    }
    fprintf(stderr, "runtime-test flow-pool: step=%s page=%d complete=%d count=%d\n", step,
            skills_page, skills_page ? state.skills_complete : state.attributes_complete,
            skills_page ? state.skill_count : state.stat_count);
    int count = skills_page ? state.skill_count : state.stat_count;
    FlowEntryState* entries = skills_page ? state.skill_entries : state.stat_entries;
    for (int index = 0; index < count; ++index) {
        int x = 0;
        int y = 0;
        bool region = FlowTargetCenter(
            test, skills_page ? FLOW_TARGET_SKILL_INCREMENT : FLOW_TARGET_STAT_INCREMENT, index, &x,
            &y);
        fprintf(stderr,
                "runtime-test flow-entry: index=%d enabled=%d increment=%d spent=%d limit=%d "
                "region=%d center=%d,%d\n",
                index, entries[index].enabled, entries[index].increment_allowed,
                entries[index].spent, entries[index].limit, region, x, y);
    }
    return test.fail(step,
                     skills_page ? "skill-points-not-committed" : "attribute-points-not-committed");
}

static bool CharacterFlow(RuntimeCase& test, bool acceptance)
{
    /* Click New Game's live bounds so the case uses the ordinary region
       callback without racing several keyboard pairs through SGP's hook in
       one frame. */
    if (!ClickFlowTarget(test, FLOW_TARGET_NEW_GAME)) {
        return test.fail("party-selection", "new-game-click-unresolved");
    }
    /* Entering is only complete after GameLoop clears the pending state and
       the controller has built and enabled the mode-zero character panel. */
    RT_REQUIRE(test, WaitCharacterFlow(test, "party-selection", 5000, FlowPartySelectionReady, 0));
    if (!ClickFlowTarget(test, FLOW_TARGET_CREATE_CHARACTER)) {
        return test.fail("character-entry", "create-character-region-missing");
    }
    RT_REQUIRE(test, WaitCharacterFlow(test, "character-entered", 5000, FlowCharacterEntered, 0));

    CharacterFlowState state;
    RT_REQUIRE(test, ReadCharacterFlow(test, state));
    if (!state.character_screen_present || state.page_index != 0 || !state.page_present[0]) {
        return test.fail("character-attributes", "initial-page-state-invalid");
    }

    /* Walk the creation pages through their real controls. The first
       profession record is a non-caster, so the spell page is skipped. */
    RT_REQUIRE(test, ClickFlowTarget(test, FLOW_TARGET_PROFESSION_NEXT));
    RT_REQUIRE(test, WaitCharacterFlow(test, "character-attributes", 3000, FlowProfessionSet, 0));
    RT_REQUIRE(test, ClickFlowTarget(test, FLOW_TARGET_RACE_NEXT));
    RT_REQUIRE(test, WaitCharacterFlow(test, "character-attributes", 3000, FlowRaceSet, 0));
    RT_REQUIRE(test, ClickFlowTarget(test, FLOW_TARGET_GENDER_NEXT));
    RT_REQUIRE(test, WaitCharacterFlow(test, "character-attributes", 3000, FlowGenderSet, 0));

    /* The row callback enables the attribute entries once profession and
       race exist. */
    RT_REQUIRE(test,
               WaitCharacterFlow(test, "character-attributes", 3000, FlowStatEntriesEnabled, 0));
    RT_REQUIRE(test, ReadCharacterFlow(test, state));
    test.observe("stat_count", state.stat_count);
    test.observe("profession_index", state.profession_index);
    test.observe("race_index", state.race_index);
    test.observe("gender_index", state.gender_index);
    RT_REQUIRE(test, SpendFlowPool(test, "character-attributes", 0));

    /* Hover the Next control long enough to raise its help box and move off
       it to take the box down before using it. */
    HoverFlowTarget(test, FLOW_TARGET_NEXT);
    WaitTransitionObjects(test, true, 2000);
    ParkMouseOutsideActiveRegions();
    WaitTransitionObjects(test, false, 2000);

    RT_REQUIRE(test, ClickFlowTarget(test, FLOW_TARGET_NEXT));
    {
        int page_two = 2;
        RT_REQUIRE(test, WaitCharacterFlow(test, "character-skills", 5000, FlowPageIs, &page_two));
    }
    RT_REQUIRE(test, WaitCharacterFlow(test, "character-skills", 5000, FlowSkillsReady, 0));

    /* The skills page's first enabled row raises and clears its own help
       box through the row's help control. */
    RT_REQUIRE(test, ReadCharacterFlow(test, state));
    test.observe("skill_count", state.skill_count);
    for (int help_index = 0; help_index < state.skill_count; ++help_index) {
        if (!state.skill_entries[help_index].enabled) {
            continue;
        }
        HoverFlowTarget(test, FLOW_TARGET_SKILL_HELP, help_index);
        WaitTransitionObjects(test, true, 2000);
        ParkMouseOutsideActiveRegions();
        WaitTransitionObjects(test, false, 2000);
        break;
    }

    RT_REQUIRE(test, SpendFlowPool(test, "character-skills", 1));

    /* Enter the final page and let its redraw complete: the prepared block
       clearing is the product-side proof that a frame ran. */
    RT_REQUIRE(test, ClickFlowTarget(test, FLOW_TARGET_NEXT));
    {
        int page_three = 3;
        RT_REQUIRE(test,
                   WaitCharacterFlow(test, "character-final-page", 5000, FlowPageIs, &page_three));
    }
    RT_REQUIRE(test, WaitCharacterFlow(test, "character-final-page", 5000, FlowFinalPageDrawn, 0));

    /* Exercise the final page through Wine's real keyboard path. This case
       intentionally reaches the page with both fields empty, so the product
       selects field zero; Tab then transfers focus to the second field. */
    const unsigned short first_name_keys[] = {'P', 'R', 'O', 'B', 'E'};
    const unsigned short second_name_keys[] = {'N', 'A', 'M', 'E'};
    for (int index = 0; index < 5; ++index) {
        SendScenarioKeyPress(first_name_keys[index], 0);
        Sleep(20);
    }
    SendScenarioKeyPress(VK_TAB, 0);
    Sleep(20);
    for (int second_index = 0; second_index < 4; ++second_index) {
        SendScenarioKeyPress(second_name_keys[second_index], 0);
        Sleep(20);
    }
    RT_REQUIRE(test, WaitCharacterFlow(test, "character-final-page", 3000, FlowNameTyped, 0));

    int want_open = 1;
    int want_closed = 0;
    if (!acceptance) {
        if (!ClickFlowTarget(test, FLOW_TARGET_VOICE_SAMPLE)) {
            return test.fail("character-final-page", "voice-sample-control-missing");
        }
        RT_REQUIRE(test,
                   WaitCharacterFlow(test, "character-summary", 3000, FlowDialogOpen, &want_open));
        SendScenarioKeyPress(VK_SPACE, 0);
        RT_REQUIRE(
            test, WaitCharacterFlow(test, "character-summary", 3000, FlowDialogOpen, &want_closed));
    }

    if (acceptance) {
        /* Commit the character through the final page's own next control.
           A broken AdvancePage callback must fail the case rather than be
           papered over by calling it directly. */
        RT_REQUIRE(test, ClickFlowTarget(test, FLOW_TARGET_NEXT));
        RT_REQUIRE(test, WaitCharacterFlow(test, "character-committed", 5000,
                                           FlowPartySelectionScreen, 0));

        /* Return toggles the selected roster row into the active party
           through the party builder's toggle. */
        SendScenarioKeyPress(VK_RETURN, 0);
        RT_REQUIRE(test,
                   WaitCharacterFlow(test, "character-in-party", 5000, FlowCharacterInParty, 0));
        RT_REQUIRE(test, ReadCharacterFlow(test, state));
        test.observe("active_characters", state.active_characters);

        for (int click = 0; click < 4; ++click) {
            if (!ClickFlowTarget(test, FLOW_TARGET_START_PARTY)) {
                return test.fail("party-start", "start-party-control-missing");
            }
            unsigned int started = GetTickCount();
            while (GetTickCount() - started < 2000) {
                RT_REQUIRE(test, ReadCharacterFlow(test, state, "party-start"));
                if (state.pending != -1 || state.current == W8_SCREEN_MAIN_GAME) {
                    break;
                }
                if (state.captured_region == 0x138) {
                    /* The ordinary fewer-than-six party confirmation. */
                    SendScenarioKeyPress(VK_RETURN, 0);
                    unsigned int dialog_started = GetTickCount();
                    while (state.captured_region == 0x138 &&
                           GetTickCount() - dialog_started < 1000) {
                        Sleep(10);
                        RT_REQUIRE(test, ReadCharacterFlow(test, state, "party-start"));
                    }
                    break;
                }
                Sleep(10);
            }
            if (state.pending != -1 || state.current == W8_SCREEN_MAIN_GAME) {
                break;
            }
        }

        unsigned int started = GetTickCount();
        while (GetTickCount() - started < 30000) {
            unsigned long remaining = 30000 - (GetTickCount() - started);
            RT_REQUIRE(test, test.wait_for_event(RUNTIME_SCREEN_CHANGED, remaining));
            RT_REQUIRE(test, ReadCharacterFlow(test, state, "main-game-entry"));
            if (test.event_count(RUNTIME_MAIN_GAME_ENTERED) != 0) {
                test.step("main-game-entered");
                break;
            }
            if (state.current == W8_SCREEN_INTRO) {
                SendScenarioKeyPress(VK_ESCAPE, 0);
            }
        }
        if (test.event_count(RUNTIME_MAIN_GAME_ENTERED) == 0) {
            return test.fail("main-game-entry", "main-game-not-entered");
        }
        RT_REQUIRE(test, test.wait_for_event(RUNTIME_WORLD_RENDER_END, 5000));
        RT_REQUIRE(test, test.wait_for_event(RUNTIME_FRAME_SUBMITTED, 5000));
        if (getenv("WIZ8_RUNTIME_VOICE_TRACE") != 0) {
            Sleep(3000);
            RuntimeEvent events[512];
            unsigned long count = RuntimeCopyRecentEvents(events, 512);
            for (unsigned long index = 0; index < count; ++index) {
                const RuntimeEvent& event = events[index];
                if (event.kind == RUNTIME_VOICE_STARTED || event.kind == RUNTIME_VOICE_TIMING ||
                    event.kind == RUNTIME_MOUTH_CHANGED ||
                    event.kind == RUNTIME_PORTRAIT_FRAME_CHANGED ||
                    event.kind == RUNTIME_PORTRAIT_BLIT) {
                    fprintf(stderr, "voice-trace %lu %s %lu %lu %lu\n", event.sequence,
                            RuntimeEventName(event.kind), event.a, event.b, event.c);
                }
            }
        }

        /* Acceptance requires observed motion, not StartCombat's grounding
           precondition. */
        RT_REQUIRE(test, MoveUntilDisplaced(test, W8_MGS_COMMAND_MOVE_FORWARD, "party-moved"));
        return true;
    }

    /* Escape raises the discard dialog; accept it once it is up. */
    SendScenarioKeyPress(VK_ESCAPE, 0);
    /* The discard dialog can take a frame to appear; its absence is not a
       failure here, the RETURN below is harmless either way. */
    {
        unsigned int dialog_started = GetTickCount();
        CharacterFlowState dialog_state;
        while (GetTickCount() - dialog_started < 2000) {
            if (!ReadCharacterFlow(test, dialog_state, "character-return")) {
                return false;
            }
            if (dialog_state.dialog_present) {
                break;
            }
            Sleep(10);
        }
    }
    SendScenarioKeyPress(VK_RETURN, 0);
    RT_REQUIRE(test,
               WaitCharacterFlow(test, "character-returned", 5000, FlowPartySelectionScreen, 0));

    SendScenarioKeyPress(VK_ESCAPE, 0);
    RT_REQUIRE(test, WaitCharacterFlow(test, "menu-returned", 5000, FlowMainMenuReturned, 0));
    return true;
}

bool CharacterReturnCase(RuntimeCase& test)
{
    return CharacterFlow(test, false);
}

bool MainGameStartCase(RuntimeCase& test)
{
    return CharacterFlow(test, true);
}
