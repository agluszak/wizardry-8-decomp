#include "wiz8/local_code/Search.h"

#include <stdio.h>

#include "wiz8/character_event_queue.h"
#include "wiz8/character_skills.h"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/engine_code/stCube.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/float_constants.h"
#include "wiz8/item_spawning.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/notices.h"
#include "wiz8/string_database.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"
#include "wiz8/sr_api.h"
#include "random.h"
#include "timer.h"

/* Local Code\search.cpp. The assertion in RegisterSearchableTrigger at source
   line 337 establishes the original translation unit. */

// GLOBAL: WIZ8 0x00689fa8
W8GrowableVector<W8Searchable*> g_searchables_00689fa8(5);
// GLOBAL: WIZ8 0x00689fb8
W8SearchableView g_search_view_00689fb8;
// GLOBAL: WIZ8 0x00689fcc
TIMER g_search_pulse_clock_00689fcc;

/* 10000.0: the unit search radius PickBestSearcher scales by score, and the
   collector's outer range gate. */
// GLOBAL: WIZ8 0x0061a364
float g_float_0061a364 = 10000.0f;
// GLOBAL: WIZ8 0x0061a368
float g_float_0061a368 = 3.1415925f;

/* The view's own constructor: seeds the cursor ahead of the first element and
   builds the item vector with capacity five. */
// FUNCTION: WIZ8 0x00517900
W8SearchableView::W8SearchableView() : cursor(-1), items() {}

/* Searchable was detected: items shed their hidden flag and become findable,
   triggers run; either way the record leaves the registry. */
// FUNCTION: WIZ8 0x00516ad0
void W8Searchable::Reveal()
{
    if (world_item != 0) {
        if (!ItemHasFlags(world_item, 1)) {
            return;
        }
        SetItemFlags(world_item, 1, false);
        SetItemFlags(world_item, 4, true);
        SetItemAndEntityFlags(world_item, 0x80, false);
    } else {
        if (trigger == 0) {
            return;
        }
        trigger->Run(-1);
        trigger->flags_0a0 |= 0x4000000;
    }
    g_searchables_00689fa8.Remove(this);
    delete this;
}

/* Refill the view with the registered searchables inside the search radius
   that either face the party or have a clear line of sight. Returns null when
   nothing qualified. */
// FUNCTION: WIZ8 0x00516ba0
W8SearchableView* CollectSearchablesInView(void)
{
    int total = g_searchables_00689fa8.count;
    srVector3T<float> camera;
    GetCameraPosition(&camera);
    g_search_view_00689fb8.items.Clear();
    g_search_view_00689fb8.cursor = -1;
    for (int index = 0; index < total; ++index) {
        W8Searchable* searchable = *g_searchables_00689fa8.GetAt(index);
        srVector3T<float> position;
        searchable->GetPosition(&position);
        srVector3T<float> delta = camera - position;
        if (delta.Length() < g_float_0061a364) {
            if (searchable->world_item == 0 && searchable->trigger != 0) {
                g_search_view_00689fb8.items.Add(searchable);
            } else {
                float half_cone = g_float_0061a368 * g_float_005ebc7c;
                srVector3T<float> from = camera;
                srVector3T<float> to = position;
                float yaw = GetCameraYawRadians();
                float heading = GetHeadingAngle(&from, &to);
                float ahead = NormalizeAngle(yaw - heading);
                float behind = NormalizeAngle(heading - yaw);
                if ((half_cone <= ahead && half_cone <= behind) || g_octree_6598a4 == 0 ||
                    g_octree_6598a4->HasLineOfSight(&camera, &position, 1) == 0) {
                    continue;
                }
                g_search_view_00689fb8.items.Add(searchable);
            }
        }
    }
    return g_search_view_00689fb8.items.count != 0 ? &g_search_view_00689fb8 : 0;
}

/* Register one searchable world item. The item pointer lands in the record's
   first slot, which is what distinguishes the item entries from the trigger
   entries registered by the sibling below. */
// FUNCTION: WIZ8 0x00516e20
void RegisterSearchableWorldItem00516E20(W8WorldItem* item)
{
    W8Searchable* searchable = new W8Searchable;
    if (searchable == 0) {
        srAssertFail("pSearchable", "C:\\Projects\\Wizardry 8\\Local Code\\search.cpp", 0x125, 0);
    }
    searchable->world_item = item;
    g_searchables_00689fa8.Add(searchable);
}

// FUNCTION: WIZ8 0x00516f00
void RegisterSearchableTrigger00516F00(Trigger* trigger)
{
    W8Searchable* searchable = new W8Searchable;
    if (searchable == 0) {
        srAssertFail("pSearchable", "C:\\Projects\\Wizardry 8\\Local Code\\search.cpp", 0x151, 0);
    }
    searchable->trigger = trigger;
    g_searchables_00689fa8.Add(searchable);
}

// FUNCTION: WIZ8 0x00516fe0
void UnregisterSearchableTrigger00516FE0(Trigger* trigger)
{
    for (int index = 0; index < g_searchables_00689fa8.count; ++index) {
        W8Searchable* searchable = *g_searchables_00689fa8.GetAt(index);
        if (searchable->trigger == trigger) {
            g_searchables_00689fa8.Remove(searchable);
            delete searchable;
            return;
        }
    }
}

/* Resolve this searchable's world position: the item's rep midpoint when it
   has a live owner, the recorded position otherwise; cursor-node and prop
   searchables resolve through their own objects. */
// FUNCTION: WIZ8 0x00517080
void W8Searchable::GetPosition(srVector3T<float>* position)
{
    position->SetZero();
    if (world_item != 0) {
        if (world_item->owner != 0) {
            world_item->owner->GetSearchPosition(position);
            return;
        }
        position->x = world_item->position.x;
        position->y = world_item->position.y + g_octree_cell_scale_005ebcd0;
        position->z = world_item->position.z;
        return;
    }
    if (cursor_node != 0) {
        if (cursor_node->GetLocation0048D050(position) == 0) {
            position->x = 3.4028235e+38f;
            position->y = 3.4028235e+38f;
            position->z = 3.4028235e+38f;
        }
        return;
    }
    if (trigger != 0) {
        if (trigger->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        if (trigger->m_pProp != 0) {
            srVector3T<float> minimum;
            srVector3T<float> maximum;
            trigger->m_pProp->PlayRepAnimation(&minimum, &maximum);
            position->x = (minimum.x + maximum.x) * g_double_005ebe80;
            position->y = (minimum.y + maximum.y) * g_double_005ebe80;
            position->z = (minimum.z + maximum.z) * g_double_005ebe80;
        }
    }
}

// FUNCTION: WIZ8 0x005171b0
void ClearSearchables005171B0()
{
    g_searchables_00689fa8.Clear();
}

/* The 500ms search sweep: while nothing is interacting or surprising the
   party, walk the in-view searchables, pick the best searcher for each, queue
   the found event and turn the camera toward the find. */
// FUNCTION: WIZ8 0x005171c0
void RunSearchPulse(void)
{
    if (ClockIsTicking(g_search_pulse_clock_00689fcc) == 0) {
        g_search_pulse_clock_00689fcc = SetCountdownClock(500);
        if (GetEnvironmentFlag0060A394() != 0 && g_flag_006840bc == 0 &&
            gXStatus.fSurprisePossible == 0 && gXStatus.fLockInteractMode == 0 &&
            gXStatus.fLockInteract == 0 && gXStatus.fTrapInteractMode == 0 &&
            gXStatus.fTrapInteract == 0) {
            if ((g_level_data_00652dac->flags & 0x100) != 0 && g_status_685170.search_mode != 0) {
                ShowNotice(0xc, gppStringList[W8_NOTICE_SEARCH_SPECIAL_LEVEL], -1, -1, 0);
            }
            W8SearchableView* view = CollectSearchablesInView();
            bool found = false;
            if (view != 0) {
                for (;;) {
                    ++view->cursor;
                    if (view->items.count <= view->cursor) {
                        --view->cursor;
                        break;
                    }
                    W8Searchable* searchable = *view->items.GetAt(view->cursor);
                    if (searchable == 0) {
                        break;
                    }
                    int slot = searchable->PickBestSearcher();
                    if (slot != -1) {
                        W8Character* character = &g_status_685170.buffers.characters[slot];
                        if (searchable->world_item == 0) {
                            if (searchable->trigger == 0) {
                                QueueCharacterEvent(character, g_effect_005ee5f0, 0,
                                                    g_effect_argument_005ed8cc,
                                                    g_effect_argument_005ed914);
                                found = true;
                            } else {
                                QueueCharacterEvent(character, g_effect_005ee5f0, 0,
                                                    g_effect_argument_005ed8cc,
                                                    g_effect_argument_005ed914);
                                int message = searchable->trigger->m_lData1;
                                found = true;
                                if (message >= 0) {
                                    const char* folder = GetLevelFolderName(GetLoadedLevelID());
                                    if (folder == 0) {
                                        folder = "Test";
                                    }
                                    char path[512];
                                    sprintf(path, "Data\\Messages\\%s.msg", folder);
                                    wchar_t text[0x7ce];
                                    if (GetStringFromStringDatabase(path, message, text, 0, 0) !=
                                        0) {
                                        ShowString(text);
                                    }
                                }
                            }
                        } else {
                            PartyAttemptsToIdentifyItem(&searchable->world_item->item, 0);
                            unsigned int event_type = g_effect_005ee5e4;
                            if (Random(2) != 0) {
                                event_type = g_effect_005ee5e8;
                            }
                            W8CharacterEvent* event = new W8CharacterEvent(
                                character, event_type, 0, g_effect_argument_005ed8cc,
                                g_effect_argument_005ed914);
                            if (searchable->world_item != 0) {
                                event->item = searchable->world_item->item;
                            }
                            if (event != 0) {
                                gXStatus.character_event_queue->QueueEntry(event);
                            }
                        }
                        srVector3T<float> position;
                        searchable->GetPosition(&position);
                        ResetInactiveLevelDataVectors0041EF50();
                        PointCameraAtTarget(&position, 1, 1);
                        searchable->Reveal();
                    }
                }
            }
            if (g_status_685170.search_mode == 0 || (g_level_data_00652dac->flags & 0x100) != 0) {
                /* Retail scans the party for a live member carrying the
                   Scouting skill and then discards the result. */
                for (int slot = 0; slot < W8_PARTY_SLOT_COUNT; ++slot) {
                    W8Character* character = &g_status_685170.buffers.characters[slot];
                    if (g_status_685170.buffers.party_rows[slot].occupied != 0 &&
                        character->hp_current != 0 &&
                        character->highest_condition < W8_CONDITION_TURNCOAT &&
                        character->skills[W8_SKILL_SCOUTING].level != 0) {
                        break;
                    }
                }
            }
            if (g_status_685170.search_mode != 0 && (g_level_data_00652dac->flags & 0x100) == 0 &&
                !found && Random(100) == 0) {
                ApplyItemEffectToRandomCharacter(g_value_0068c548, -1, 0,
                                                 g_effect_argument_005ed8c8);
            }
        }
    }
}

/* Pick the best searching party member for this searchable: eligible members
   score their Scouting skill (halved, quartered in the special level state)
   plus a Senses-derived bonus; the detect-secrets party effect instead
   force-picks random eligible members without practicing the skill. The pick
   must also reach this searchable's position. */
// FUNCTION: WIZ8 0x00517560
int W8Searchable::PickBestSearcher()
{
    unsigned int best_score = 0;
    int best_slot = -1;
    bool earned = false;
    for (int slot = 0; slot < W8_PARTY_SLOT_COUNT; ++slot) {
        W8Character* character = &g_status_685170.buffers.characters[slot];
        if (g_status_685170.buffers.party_rows[slot].occupied == 0 || character->hp_current == 0 ||
            character->highest_condition >= W8_CONDITION_TURNCOAT) {
            continue;
        }
        if (CharacterHasTrait00547940(character, W8_TRAIT_SEARCH) ||
            g_status_685170.search_mode != 0) {
            unsigned int level = character->skills[W8_SKILL_SCOUTING].level;
            unsigned int base = level >> 1;
            if ((g_level_data_00652dac->flags & 0x100) != 0) {
                base = level >> 2;
            }
            unsigned int score = 0;
            if (base != 0 ||
                (g_status_685170.search_mode != 0 && (g_level_data_00652dac->flags & 0x100) == 0)) {
                unsigned int attribute = character->attributes[W8_ATTRIBUTE_SENSES].effective;
                if (attribute < 0x33) {
                    score = base + attribute / 5;
                } else if (attribute < 0x51) {
                    score = base + 10 + (attribute - 0x32) / 3;
                } else if (attribute < 0x5b) {
                    score = base - 0x3c + attribute;
                } else if (attribute < 100) {
                    score = base - 0x96 + attribute * 2;
                } else if (attribute < 0x65) {
                    score = base + 0x32;
                } else {
                    score = base + 0x32 + (attribute * 10 - 1000) / 0x19;
                }
            }
            if (score > 99) {
                score = 100;
            }
            if (best_score < score) {
                earned = true;
                best_score = score;
                best_slot = slot;
            }
        }
        if (g_status_685170.party_modifiers_22e3.flag_46 != 0) {
            if (best_slot != -1 && Random(2) == 0) {
                continue;
            }
            best_score = 100;
            earned = false;
            best_slot = slot;
        }
    }
    float range = best_score * g_float_0061a364 * g_movement_speed_step_005ed490;
    srVector3T<float> camera;
    GetCameraPosition(&camera);
    srVector3T<float> position;
    GetPosition(&position);
    srVector3T<float> delta = camera - position;
    if (delta.Length() < range) {
        if (earned) {
            PracticeCharacterSkill(&g_status_685170.buffers.characters[best_slot],
                                   W8_SKILL_SCOUTING, 5, 0);
        }
        return best_slot;
    }
    return -1;
}

/* Toggle the party's search mode: leaving it posts the off notice, entering it
   posts the on notice and arms the 500ms pulse that sweeps the searchable
   registry; combat blocks the toggle outright. */
// FUNCTION: WIZ8 0x00517780
void ToggleSearchMode(void)
{
    if (g_status_685170.search_mode != 0) {
        g_status_685170.search_mode = 0;
        ShowNotice(0xc, gppStringList[W8_NOTICE_SEARCH_MODE_OFF], -1, -1, 0);
        return;
    }
    if (gXStatus.fCombatMode == 0) {
        g_status_685170.search_mode = 1;
        ShowNotice(0xc, gppStringList[W8_NOTICE_SEARCH_MODE_ON], -1, -1, 0);
        g_search_pulse_clock_00689fcc = SetCountdownClock(0x1f4);
        ClearValue6834D4();
    } else {
        ShowNotice(0xc, gppStringList[W8_NOTICE_SEARCH_BLOCKED_COMBAT], -1, -1, 0);
    }
}
