#ifndef WIZ8_XSTATUS_H
#define WIZ8_XSTATUS_H

#include <stddef.h>

#include "wiz8/3d_code/PList.h"

struct W8StartupRuntimeState;

/* The recovered prefix of the packed gXStatus runtime object at 0x00683F78.
   Database loaders establish the first seven dwords. Retail assertions name
   selected later members and the instructions establish their offsets. Every
   byte in this prefix is this object: overlapping GLOBAL aliases at 0x683F94,
   0x683F95..0x683F9B, 0x683FAD, 0x683FB1, 0x683FC5, 0x683FCD and 0x683FCE are
   those members, not separate roots.

   The object continues beyond this prefix; no recovered body exposes its
   complete sizeof. 0x73 is the size of this recovered prefix type, not a
   proven retail extent. Do not use sizeof(W8XStatus) as the object's size. */
#pragma pack(push, 1)
struct W8XStatus {
    unsigned int uiItemsInDatabase;      /* 0x00: 0x00683F78 */
    unsigned int uiItemTablesInDatabase; /* 0x04 */
    unsigned int uiItemTableCategories;  /* 0x08 */
    unsigned int uiMonstersInDatabase;   /* 0x0c */
    unsigned int uiNpcsInDatabase;       /* 0x10 */
    unsigned int uiFactsInDatabase;      /* 0x14 */
    unsigned int uiLevelsInDatabase;     /* 0x18 */
    unsigned char fCombatMode;           /* 0x1c: 0x00683F94 */
    unsigned char fSpellCastMode;        /* 0x1d: 0x00683F95 */
    unsigned char fItemSelectMode;       /* 0x1e: 0x00683F96 */
    unsigned char fNpcDialogueMode;      /* 0x1f: 0x00683F97 */
    unsigned char fLockInteractMode;     /* 0x20: 0x00683F98 */
    unsigned char fTrapInteractMode;     /* 0x21: 0x00683F99 */
    unsigned char fReviewCharacterMode;  /* 0x22: 0x00683F9A */
    unsigned char fCampMode;             /* 0x23: 0x00683F9B */
    unsigned char fLockInteract;         /* 0x24: lock session; admits lock spells */
    unsigned char fTrapInteract;         /* 0x25: trap session; admits trap spells */
    unsigned char unknown_026[2];
    unsigned char fEncumbranceDirty; /* 0x28: pending party-weight recalc */
    int active_monster_count;        /* 0x29 */
    int field_02d;
    int item_manager_pending;              /* 0x31 */
    W8PList* plsMonsterList;               /* 0x35: 0x00683FAD */
    W8PList* plsMonsterGroupList;          /* 0x39: 0x00683FB1 */
    W8PList* plsItemList;                  /* 0x3d */
    W8PList* plsUnbornMonsterList;         /* 0x41 */
    W8PList* plsMonsterGroupEncounterList; /* 0x45 */
    unsigned char unknown_049[4];
    unsigned char fSurprisePossible; /* 0x4d: 0x00683FC5 */
    unsigned char unknown_04e[7];
    unsigned char fPartyMovementUi;   /* 0x55: 0x00683FCD; region set 0x1c / panels */
    unsigned char fPartyMovementMode; /* 0x56: 0x00683FCE */
    float flPartyMoveDistLimit;       /* 0x57 */
    float field_05b;
    W8StartupRuntimeState*
        field_05f;            /* 0x5f: 0x00683FD7, same storage as g_startup_runtime_state */
    int iCurrentCursor;       /* 0x63 */
    int current_cursor_frame; /* 0x67 */
    int current_cursor_time;  /* 0x6b */
    int iTargetingMode;       /* 0x6f: 0x00683FE7 */
};
#pragma pack(pop)

static_assert(offsetof(W8XStatus, uiMonstersInDatabase) == 0x0c, "W8XStatus_monster_count_offset");
static_assert(offsetof(W8XStatus, fCombatMode) == 0x1c, "W8XStatus_combat_mode_offset");
static_assert(offsetof(W8XStatus, fCampMode) == 0x23, "W8XStatus_camp_mode_offset");
static_assert(offsetof(W8XStatus, plsMonsterList) == 0x35, "W8XStatus_monster_list_offset");
static_assert(offsetof(W8XStatus, plsMonsterGroupList) == 0x39,
              "W8XStatus_monster_group_list_offset");
static_assert(offsetof(W8XStatus, plsItemList) == 0x3d, "W8XStatus_item_list_offset");
static_assert(offsetof(W8XStatus, plsUnbornMonsterList) == 0x41,
              "W8XStatus_unborn_monster_list_offset");
static_assert(offsetof(W8XStatus, fSurprisePossible) == 0x4d, "W8XStatus_surprise_possible_offset");
static_assert(offsetof(W8XStatus, fPartyMovementUi) == 0x55, "W8XStatus_party_movement_ui_offset");
static_assert(offsetof(W8XStatus, fPartyMovementMode) == 0x56,
              "W8XStatus_party_movement_mode_offset");
static_assert(offsetof(W8XStatus, flPartyMoveDistLimit) == 0x57,
              "W8XStatus_party_movement_limit_offset");
static_assert(offsetof(W8XStatus, field_05f) == 0x5f, "W8XStatus_startup_runtime_state_offset");
static_assert(offsetof(W8XStatus, iCurrentCursor) == 0x63, "W8XStatus_cursor_offset");
static_assert(offsetof(W8XStatus, iTargetingMode) == 0x6f,
              "W8XStatus_last_recovered_member_offset");

extern W8XStatus gXStatus;

#endif
