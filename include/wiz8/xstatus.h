#ifndef WIZ8_XSTATUS_H
#define WIZ8_XSTATUS_H

#include <stddef.h>

#include "wiz8/3d_code/PList.h"

/* The recovered prefix of the packed gXStatus runtime object at 0x00683F78.
   Database loaders establish the first seven dwords. Retail assertions name
   selected later members and the instructions establish their offsets. The
   object continues beyond this prefix; no recovered body exposes its complete
   sizeof, so this type deliberately stops after iCurrentCursor's state. */
#pragma pack(push, 1)
typedef struct W8XStatus {
    unsigned int uiItemsInDatabase;             /* 0x00: 0x00683F78 */
    unsigned int uiItemTablesInDatabase;         /* 0x04 */
    unsigned int uiItemTableCategories;          /* 0x08 */
    unsigned int uiMonstersInDatabase;           /* 0x0c */
    unsigned int uiNpcsInDatabase;               /* 0x10 */
    unsigned int uiFactsInDatabase;              /* 0x14 */
    unsigned int uiLevelsInDatabase;             /* 0x18 */
    unsigned char fCombatMode;                    /* 0x1c */
    unsigned char field_01d;
    unsigned char fItemSelectMode;                /* 0x1e */
    unsigned char field_01f;
    unsigned char field_020;
    unsigned char field_021;
    unsigned char field_022;
    unsigned char fCampMode;                      /* 0x23 */
    unsigned char field_024;
    unsigned char field_025;
    unsigned char unknown_026[2];
    unsigned char field_028;
    int active_monster_count;                     /* 0x29 */
    int field_02d;
    int item_manager_pending;                     /* 0x31 */
    W8PList* plsMonsterList;                      /* 0x35 */
    W8PList* plsMonsterGroupList;                 /* 0x39 */
    W8PList* plsItemList;                         /* 0x3d */
    W8PList* plsUnbornMonsterList;                /* 0x41 */
    W8PList* plsMonsterGroupEncounterList;        /* 0x45 */
    unsigned char unknown_049[4];
    unsigned char fSurprisePossible;              /* 0x4d */
    unsigned char unknown_04e[7];
    unsigned char field_055;
    unsigned char fPartyMovementMode;             /* 0x56 */
    float flPartyMoveDistLimit;                   /* 0x57 */
    float field_05b;
    void* field_05f;
    int iCurrentCursor;                           /* 0x63 */
    int current_cursor_frame;                     /* 0x67 */
    int current_cursor_time;                      /* 0x6b */
    int field_06f;                                /* 0x6f: 0x00683FE7 */
} W8XStatus;
#pragma pack(pop)

static_assert(offsetof(W8XStatus, uiMonstersInDatabase) == 0x0c,
              "W8XStatus_monster_count_offset");
static_assert(offsetof(W8XStatus, fCombatMode) == 0x1c,
              "W8XStatus_combat_mode_offset");
static_assert(offsetof(W8XStatus, plsMonsterList) == 0x35,
              "W8XStatus_monster_list_offset");
static_assert(offsetof(W8XStatus, plsMonsterGroupList) == 0x39,
              "W8XStatus_monster_group_list_offset");
static_assert(offsetof(W8XStatus, plsItemList) == 0x3d,
              "W8XStatus_item_list_offset");
static_assert(offsetof(W8XStatus, plsUnbornMonsterList) == 0x41,
              "W8XStatus_unborn_monster_list_offset");
static_assert(offsetof(W8XStatus, fPartyMovementMode) == 0x56,
              "W8XStatus_party_movement_mode_offset");
static_assert(offsetof(W8XStatus, flPartyMoveDistLimit) == 0x57,
              "W8XStatus_party_movement_limit_offset");
static_assert(offsetof(W8XStatus, iCurrentCursor) == 0x63,
              "W8XStatus_cursor_offset");
static_assert(sizeof(W8XStatus) == 0x73, "W8XStatus_recovered_prefix_size");

extern W8XStatus gXStatus;

#endif
