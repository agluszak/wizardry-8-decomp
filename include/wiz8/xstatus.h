#ifndef WIZ8_XSTATUS_H
#define WIZ8_XSTATUS_H

#include <stddef.h>

#include "wiz8/3d_code/PList.h"
#include "wiz8/local_code/MonsterManager.h"

struct W8StartupRuntimeState;

/* Packed gXStatus at 0x006836B8. The thiscall constructor at 0x004E6970
   constructs eight W8MonsterManagerEntry objects at this, then the growable
   vector at this+0x9B7. The destructor at 0x004E6940 tears those members down
   in reverse. That is one C++ object of size 0x9C7, not an entries array, a
   0x73 prefix, and a separate targeting vector.

   Database loaders and retail assertions name later members of this same
   global (uiItemsInDatabase, fCombatMode, plsMonsterList, ...). Overlapping
   GLOBAL aliases at 0x683F94, 0x683F95..0x683F9B, 0x683FAD, 0x683FB1,
   0x683FC5, 0x683FCD, 0x683FCE, 0x683FD7, 0x684000 and 0x68406F are those
   members, not separate roots.

   Neighbouring state from 0x0068407F (target position) through the monster
   record cache at 0x006840C7 and the timer at 0x00685067 is not constructed
   by 0x004E6970. The 0x1A0A-byte stos at 0x0054AFD0 is a reset region, not
   sizeof(W8XStatus). */
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
    int field_02d;
    int item_manager_pending;              /* 0x8f1 */
    W8PList* plsMonsterList;               /* 0x8f5: 0x00683FAD */
    W8PList* plsMonsterGroupList;          /* 0x8f9: 0x00683FB1 */
    W8PList* plsItemList;                  /* 0x8fd */
    W8PList* plsUnbornMonsterList;         /* 0x901 */
    W8PList* plsMonsterGroupEncounterList; /* 0x905 */
    unsigned char unknown_049[4];
    unsigned char fSurprisePossible; /* 0x90d: 0x00683FC5 */
    unsigned char unknown_04e[7];
    unsigned char fPartyMovementUi;   /* 0x915: 0x00683FCD; region set 0x1c / panels */
    unsigned char fPartyMovementMode; /* 0x916: 0x00683FCE */
    float flPartyMoveDistLimit;       /* 0x917 */
    float field_05b;
    W8StartupRuntimeState* pStartupRuntime; /* 0x91f: 0x00683FD7 */
    int iCurrentCursor;                     /* 0x923 */
    int current_cursor_frame;               /* 0x927 */
    int current_cursor_time;                /* 0x92b */
    int iTargetingMode;                     /* 0x92f: 0x00683FE7 */
    unsigned char unknown_933[0x15];
    unsigned char party_slot_state[8][0xc]; /* 0x948: 0x00684000 */
    unsigned char unknown_9a8[0x0f];
    W8GrowableVector<int> target_markers; /* 0x9b7: 0x0068406F */
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
static_assert(offsetof(W8XStatus, fPartyMovementUi) == 0x915, "W8XStatus_party_movement_ui_offset");
static_assert(offsetof(W8XStatus, fPartyMovementMode) == 0x916,
              "W8XStatus_party_movement_mode_offset");
static_assert(offsetof(W8XStatus, flPartyMoveDistLimit) == 0x917,
              "W8XStatus_party_movement_limit_offset");
static_assert(offsetof(W8XStatus, pStartupRuntime) == 0x91f, "W8XStatus_startup_runtime_offset");
static_assert(offsetof(W8XStatus, iCurrentCursor) == 0x923, "W8XStatus_cursor_offset");
static_assert(offsetof(W8XStatus, iTargetingMode) == 0x92f, "W8XStatus_targeting_mode_offset");
static_assert(offsetof(W8XStatus, party_slot_state) == 0x948, "W8XStatus_party_slot_state_offset");
static_assert(offsetof(W8XStatus, target_markers) == 0x9b7, "W8XStatus_target_markers_offset");
static_assert(sizeof(W8XStatus) == 0x9c7, "W8XStatus_size");

extern W8XStatus gXStatus;

#endif
