#ifndef WIZ8_XSTATUS_H
#define WIZ8_XSTATUS_H

#include <stddef.h>

#include "wiz8/3d_code/PList.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/targeting.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/MonsterManager.h"

struct W8CharacterEventQueue;
class W8GameTimer;

/* Packed gXStatus at 0x006836B8. The thiscall constructor at 0x004E6970
   constructs eight W8MonsterManagerEntry objects at this, then the growable
   vector at this+0x9B7. The destructor at 0x004E6940 tears those members down
   in reverse. These members form the non-trivial prefix of one C++ object,
   not an entries array, a 0x73 prefix, and a separate targeting vector.

   Database loaders and retail assertions name later members of this same
   global (uiItemsInDatabase, fCombatMode, plsMonsterList, ...). Overlapping
   GLOBAL aliases at 0x683F94, 0x683F95..0x683F9B, 0x683FAD, 0x683FB1,
   0x683FC5, 0x683FCD, 0x683FCE, 0x683FD7, 0x684000 and 0x68406F are those
   members, not separate roots.

   The constructor and destructor only visit the non-trivial prefix members.
   InitializeGameplayRuntimeObjects clears 0x1A0A bytes starting at this
   object, establishing the POD state through +0x1A09 as its tail. */
#pragma pack(push, 1)
struct W8XStatus {
    W8XStatus();
    ~W8XStatus();

    W8MonsterManagerEntry monster_manager_entries[8]; /* 0x000: 0x006836B8 */
    unsigned int uiItemsInDatabase;                   /* 0x8c0: 0x00683F78 */
    unsigned int uiItemTablesInDatabase;              /* 0x8c4 */
    unsigned int uiItemTableCategories;               /* 0x8c8 */
    unsigned int uiMonstersInDatabase;                /* 0x8cc */
    unsigned int uiNpcsInDatabase;                    /* 0x8d0 */
    unsigned int uiFactsInDatabase;                   /* 0x8d4 */
    unsigned int uiLevelsInDatabase;                  /* 0x8d8 */
    unsigned char fCombatMode;                        /* 0x8dc: 0x00683F94 */
    unsigned char fSpellCastMode;                     /* 0x8dd: 0x00683F95 */
    unsigned char fItemSelectMode;                    /* 0x8de: 0x00683F96 */
    unsigned char fNpcDialogueMode;                   /* 0x8df: 0x00683F97 */
    unsigned char fLockInteractMode;                  /* 0x8e0: 0x00683F98 */
    unsigned char fTrapInteractMode;                  /* 0x8e1: 0x00683F99 */
    unsigned char fReviewCharacterMode;               /* 0x8e2: 0x00683F9A */
    unsigned char fCampMode;                          /* 0x8e3: 0x00683F9B */
    unsigned char fLockInteract;                      /* 0x8e4: lock session; admits lock spells */
    unsigned char fTrapInteract;                      /* 0x8e5: trap session; admits trap spells */
    unsigned char unknown_026[2];
    unsigned char fEncumbranceDirty; /* 0x8e8: pending party-weight recalc */
    int active_monster_count;        /* 0x8e9 */
    /* 0x8ed: active in-combat monsters with DISP_HOSTILE, recomputed by
       RecountCombatMonsters; nonzero starts combat and blocks ending it.
       Retail compares it unsigned (JA/JBE/SETA), never signed. */
    unsigned int hostile_monster_count;
    int item_manager_pending;              /* 0x8f1 */
    W8PList* plsMonsterList;               /* 0x8f5: 0x00683FAD */
    W8PList* plsMonsterGroupList;          /* 0x8f9: 0x00683FB1 */
    W8PList* plsItemList;                  /* 0x8fd */
    W8PList* plsUnbornMonsterList;         /* 0x901 */
    W8PList* plsMonsterGroupEncounterList; /* 0x905 */
    unsigned char unknown_049[4];
    unsigned char fSurprisePossible; /* 0x90d: 0x00683FC5 */
    /* 0x90e: set when surprise starts with no character engaged; phase 1 waits
       only while this is clear. */
    unsigned char surprise_unengaged;
    /* 0x90f: uiTurnsElapsed deadline the phase-1 hold compares against. */
    unsigned int surprise_deadline_turns;
    /* 0x913: 0 = fade in, 1 = hold, 2 = fade out / resolve. */
    unsigned short surprise_phase;
    unsigned char fPartyMovementUi;   /* 0x915: 0x00683FCD; region set 0x1c / panels */
    unsigned char fPartyMovementMode; /* 0x916: 0x00683FCE */
    float flPartyMoveDistLimit;       /* 0x917 */
    /* 0x91b: accumulated party movement distance; the combat movement update
       converts it into the remaining percentage displayed by the panel. */
    float party_move_distance;
    /* 0x91f: 0x00683FD7. InitializeGameplayRuntimeObjects stores the queue
       here; a standalone BSS pointer at this address is the same member. */
    W8CharacterEventQueue* character_event_queue;
    int iCurrentCursor;       /* 0x923 */
    int current_cursor_frame; /* 0x927 */
    int current_cursor_time;  /* 0x92b */
    int iTargetingMode;       /* 0x92f: 0x00683FE7 */
    /* 0x933: the formation screen's edit buffer - MGSFormation snapshots the
       live formation here on open, edits the copy, and either reconciles it
       back or diffs it against live on accept. */
    W8PartyFormationState edited_formation;
    W8GrowableVector<int> target_markers;        /* 0x9b7: 0x0068406F */
    srVector3T<float> target_position;           /* 0x9c7: 0x0068407F */
    W8CombatSlot shared_target;                  /* 0x9d3: 0x0068408B */
    W8ActionDetailBlock shared_action_detail;    /* 0x9f3: 0x006840AB */
    int picked_monster;                          /* 0x9fb: 0x006840B3 */
    int picked_group;                            /* 0x9ff: 0x006840B7 */
    unsigned char flag_a03;                      /* 0xa03: 0x006840BB */
    unsigned char world_update_blocked;          /* 0xa04: 0x006840BC */
    unsigned char flag_a05;                      /* 0xa05: 0x006840BD */
    unsigned short review_character_slot;        /* 0xa06: 0x006840BE */
    int held_item_source;                        /* 0xa08: 0x006840C0 */
    unsigned char held_item_origin;              /* 0xa0c: 0x006840C4 */
    unsigned short held_item_slot;               /* 0xa0d: 0x006840C5 */
    W8MonsterRecord* monster_record_cache[1000]; /* 0xa0f: 0x006840C7 */
    W8GameTimer* gameplay_timer;                 /* 0x19af: 0x00685067 */
    unsigned char save_notice_shown;             /* 0x19b3: 0x0068506B */
    bool npc_combat_notice_pending;              /* 0x19b4: 0x0068506C */
    unsigned char deferred_skill_notices;        /* 0x19b5: 0x0068506D */
    unsigned char flag_19b6;                     /* 0x19b6: 0x0068506E */
    unsigned char flag_19b7;                     /* 0x19b7: 0x0068506F */
    unsigned char flag_19b8;                     /* 0x19b8: 0x00685070 */
    bool item_drag_active;                       /* 0x19b9: 0x00685071 */
    W8ItemInstance* dragged_item;                /* 0x19ba: 0x00685072 */
    unsigned char dragged_item_origin;           /* 0x19be: 0x00685076 */
    signed char dragged_character_slot;          /* 0x19bf: 0x00685077 */
    unsigned int spell_cooldown_clocks[14];      /* 0x19c0: 0x00685078 */
    unsigned int combat_countdown;               /* 0x19f8: 0x006850B0 */
    unsigned char combat_difficulty;             /* 0x19fc: 0x006850B4 */
    unsigned char party_moving;                  /* 0x19fd: 0x006850B5 */
    int saved_encounter_budget;                  /* 0x19fe: 0x006850B6 */
    int mipe_cube_serial;                        /* 0x1a02: 0x006850BA */
    int hostile_group_count;                     /* 0x1a06: 0x006850BE */
};
#pragma pack(pop)

static_assert(offsetof(W8XStatus, monster_manager_entries) == 0x0, "W8XStatus_entries_offset");
static_assert(offsetof(W8XStatus, uiItemsInDatabase) == 0x8c0, "W8XStatus_items_offset");
static_assert(offsetof(W8XStatus, uiMonstersInDatabase) == 0x8cc, "W8XStatus_monster_count_offset");
static_assert(offsetof(W8XStatus, fCombatMode) == 0x8dc, "W8XStatus_combat_mode_offset");
static_assert(offsetof(W8XStatus, fCampMode) == 0x8e3, "W8XStatus_camp_mode_offset");
static_assert(offsetof(W8XStatus, plsMonsterList) == 0x8f5, "W8XStatus_monster_list_offset");
static_assert(offsetof(W8XStatus, plsMonsterGroupList) == 0x8f9,
              "W8XStatus_monster_group_list_offset");
static_assert(offsetof(W8XStatus, plsItemList) == 0x8fd, "W8XStatus_item_list_offset");
static_assert(offsetof(W8XStatus, plsUnbornMonsterList) == 0x901,
              "W8XStatus_unborn_monster_list_offset");
static_assert(offsetof(W8XStatus, fSurprisePossible) == 0x90d,
              "W8XStatus_surprise_possible_offset");
static_assert(offsetof(W8XStatus, surprise_unengaged) == 0x90e,
              "W8XStatus_surprise_unengaged_offset");
static_assert(offsetof(W8XStatus, surprise_deadline_turns) == 0x90f,
              "W8XStatus_surprise_deadline_turns_offset");
static_assert(offsetof(W8XStatus, surprise_phase) == 0x913, "W8XStatus_surprise_phase_offset");
static_assert(offsetof(W8XStatus, fPartyMovementUi) == 0x915, "W8XStatus_party_movement_ui_offset");
static_assert(offsetof(W8XStatus, fPartyMovementMode) == 0x916,
              "W8XStatus_party_movement_mode_offset");
static_assert(offsetof(W8XStatus, flPartyMoveDistLimit) == 0x917,
              "W8XStatus_party_movement_limit_offset");
static_assert(offsetof(W8XStatus, character_event_queue) == 0x91f,
              "W8XStatus_character_event_queue_offset");
static_assert(offsetof(W8XStatus, iCurrentCursor) == 0x923, "W8XStatus_cursor_offset");
static_assert(offsetof(W8XStatus, iTargetingMode) == 0x92f, "W8XStatus_targeting_mode_offset");
static_assert(offsetof(W8XStatus, edited_formation) == 0x933, "W8XStatus_edited_formation_offset");
static_assert(offsetof(W8XStatus, target_markers) == 0x9b7, "W8XStatus_target_markers_offset");
static_assert(offsetof(W8XStatus, target_position) == 0x9c7, "W8XStatus_target_position_offset");
static_assert(offsetof(W8XStatus, monster_record_cache) == 0xa0f,
              "W8XStatus_monster_record_cache_offset");
static_assert(offsetof(W8XStatus, gameplay_timer) == 0x19af, "W8XStatus_timer_offset");
static_assert(offsetof(W8XStatus, spell_cooldown_clocks) == 0x19c0,
              "W8XStatus_spell_cooldown_clocks_offset");
static_assert(offsetof(W8XStatus, hostile_group_count) == 0x1a06,
              "W8XStatus_hostile_group_count_offset");
static_assert(sizeof(W8XStatus) == 0x1a0a, "W8XStatus_size");

extern W8XStatus gXStatus;

#endif
