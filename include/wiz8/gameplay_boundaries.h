#ifndef WIZ8_GAMEPLAY_BOUNDARIES_H
#define WIZ8_GAMEPLAY_BOUNDARIES_H

/* Quarantine only: move declarations to their owning subsystem before
   changing them. This header may shrink, but must not gain new APIs or become
   a compatibility facade for extracted declarations. */

#include <stddef.h>

#include "wiz8/item_tables.h"
#include "wiz8/item_instance.h"
#include "wiz8/character.h"
#include "wiz8/geometry.h"
#include "surrender/srMath.h"
#include "wiz8/startup_runtime_state.h"

/* Shared recovered Wizardry interfaces used by matching translation units. */

#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/vector.h"
#include "wiz8/combat_state.h"
#include "wiz8/magic.h"
#include "wiz8/screen_state.h"
#include "wiz8/game_state.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/layouts/encounter_tables.h"
#include "wiz8/regions.h"
#include "wiz8/ui_state.h"
#include "wiz8/utility.h"
#include "random.h"
#include "timer.h"

#pragma pack(push, 1)


struct W8World;

/* 0x0068C09C addresses a flat array of localized notice pointers. ShowRegionHelp
   indexes it by a region's help id, which is what fixes the shape as an array
   rather than a record; the constants below are the entries a ported body picks
   by name instead of out of data, and are the byte offsets those bodies use
   divided by the pointer stride. The element type is not established, so the
   array stays void*. */
enum W8NoticeId {
    W8_NOTICE_MONSTER_SLAIN = 0x74c / 4,
    W8_NOTICE_CHARACTER_SAVE_FAILED = 0x780 / 4,
    W8_NOTICE_CHARACTER_LOAD_FAILED = 0x784 / 4,
    W8_NOTICE_COMBAT_CANNOT_END = 0x878 / 4,
    W8_NOTICE_COMBAT_CANNOT_END_ENGAGED = 0x87c / 4,
    W8_NOTICE_COMBAT_CANNOT_END_PENDING = 0x880 / 4,
    W8_NOTICE_COMBAT_ENDED = 0x884 / 4,
    W8_NOTICE_COMBAT_STANCE_RELAXED = 0x888 / 4,
    W8_NOTICE_COMBAT_STANCE_READY = 0x88c / 4
};

#pragma pack(pop)

extern "C" {

/* 0x0068C09C holds a pointer to the loaded string table, one wide string per
   entry, which Strings.cpp fills from the localisation file. Bodies name
   entries by their byte offset into it, so the index is spelled as one. It was
   reached under five names before this - notices, a dialog message table, a
   plain dword and a message-string array - and one of those had it a level of
   indirection out, treating the pointer itself as the table.

   The two are named by the Strings.cpp assertions gppStringList and
   giStringListLen rather than described. */
extern wchar_t** gppStringList;
extern int giStringListLen;
/* 0x00517A90. One shared buffer, so the answer is only good until the next
   call - which is why every caller consumes it immediately. */
/* The gameplay globals reached from more than a couple of units. Each was
   declared separately in every file that used it, which is how two of them
   ended up with two names and two more with two types; one declaration each is
   what keeps that from happening again. */
/* 0x00687599: one byte, and the image proves it. All 16 relocation sites that
   name the address touch it a byte at a time - fifteen `mov r8, byte ptr [..]`
   reads across seven functions and the single write `mov byte ptr [..], 1` in
   Function54B250 - and the globals census puts its extent at one byte, between
   a dword at 0x00687595 and another at 0x0068759A. It had been declared `int`
   in quarantine_common.h, `bool` in GameplayDatabase.cpp and `unsigned char` in
   LoadSaveGame.cpp, which emitted three different symbols for one address and
   only linked because /FORCE leaves them unresolved.
   It admits a save the autosave interval gate would otherwise refuse and
   selects the current slot over the fixed AutoSave name, so the recovered name
   stays positional until the meaning behind that pairing is established. */
extern unsigned char g_save_flag_00687599;
/* 0x00683FAD: every monster currently in the level. */
/* 0x0068EC78: which screen is up. The whole 0x98-byte block lives there; the
   leading id is what everything outside the screen code reads, which is why
   this is a union rather than two globals. */
/* 0x005EBB34: the float that stands for "no distance given", which the level
   vector reads as its absent value too. */
extern float g_float_005ebb34;

/* 0x00685178: one 0x106-byte row per party slot; only the leading byte is
   established, and GetRandomCharacter treats it as a slot-occupied flag. */
/* Flat table indexed by skill_id * 15 + profession. */
/* 0x00615570: hit points a profession contributes per level it has been taken,
   before the vitality-derived multiplier. Fifteen entries; the recalculation at
   0x0052A2F0 walks it to the address one past the last, which is what fixes the
   extent. The entries are floats - the multiply is `fmul dword ptr` - and the
   table sits in .data rather than .rdata. */
/* 0x00616E84: one entry per item category, giving the interface presentation
   of whatever spell the item carries. */
extern const int g_item_spell_presentation[];
/* 0x00648C5C: one entry per equipment slot; -1 marks a slot that has no
   interface position, which is what the unequip rule tests for. */
extern const int g_equip_slot_icons[];
/* 0x0068517C: gStatus.fGameStarted, named by the PC Item.cpp assertion at line
   3795 that guards the party-pool sort. Item placement consults it because the
   character-creation screens fill different slots than play does. */
#define g_game_started (g_status_685170.game_started_000c)
/* 0x00685189: the party's purse. AddPartyGold plays Data\Sound\Misc\ChaChing.wav
   and posts the pickup message when it is told to announce; SpendPartyGold is
   the matching debit and floors at zero rather than wrapping. */
extern unsigned int g_party_gold;
/* Reset together by ResetGameplayStatusBlock; their meaning is not established,
   so they keep address-positional names. The object at 0x00683FD7 is torn down
   by 0x0054B0B0 through operator delete after the same method's sibling. */
extern unsigned char g_status_block_685078[56];
extern W8StartupRuntimeState* g_startup_runtime_state;
extern void* g_object_685067;
extern unsigned char g_party_moving_006850b5;
/* Written by Targeting.cpp and reset to -1 here; meaning not established. */
extern int g_target_state_6840b3;
extern int g_picked_group_006840b7;
extern W8EncounterTableRuntime** g_encounter_tables;
extern char** g_encounter_names;
extern unsigned int g_encounter_name_count;
/* 0x0060A6BC: the level whose encounter tables are loaded, -1 when none are. */
extern int g_encounter_tables_level;
extern unsigned int g_encounter_table_count;
extern int g_loaded_level_id;
/* Current party level, -1 while no level is loaded. */
/* 0x00659AB8: the second world. It was spelled `void*` where it is defined and
   `W8World*` where the viewport reaches through it, which is two claims about
   one object; the viewport's is the one a body proves, since it reads the
   camera member off it. One declaration here settles it. */
extern W8World* g_world_659ab8;
/* Defined by DisplayList.cpp, which owns the list this gates; the recovered
   clear at 0x0040C220 reads it from outside that unit, so the declaration sits
   here rather than being spelled a second time where it is used. */
extern unsigned char g_display_flag_650e90;
/* Starting item ids, terminated by the address after the last, with 0xffffffff
   marking an empty slot. */
extern unsigned int g_starting_item_ids[];
/* 0x00686A70: the level the party is on. The save loader compares it against
   the level id in an LVLS chunk, and it indexes the per-level rows below -
   bounded at 0x2f, the same forty-seven the folder table has. */
/* 0x00686B74: one row per level. LoadLevel owns the leading visited flag. Gold
   picked up there starts at +9 (0x00686B7D), and the clock its sight state was
   last brought up to follows at +0xd (0x00686B81). */
extern int g_location_variable_count;
extern char** g_location_variable_names;
extern int g_location_variable_level_count;
extern int* g_location_variable_levels;
/* 0x006840C7: lazily populated cache of runtime monster records, one slot per
   species ID. MAX_MONSTERS_IN_DATABASE comes from the canonical assertion text. */
/* Provisional name: a fixed-address, non-per-character item pool distinct
   from the equipped/carried slots GetOriginOfCharacterItem also searches. */
extern unsigned char g_shared_item_pool[];
extern unsigned int g_shared_item_pool_count;


/* 3D Code\IList.cpp: the integer sibling of W8PList, same shape with int
   elements, which is why a failed lookup returns -1 rather than null. */



/* 0x00521EF0, the address CFAgent seeds. Answers whether the item found a
   home, which one of its two callers tests and the other ignores. */
bool AddItemToParty(
    W8ItemInstance* item, unsigned char announce, unsigned char skip_stacking);
void FreeStringTable(void);
bool IsStringTableLoaded(void);
unsigned char InitializeFactDatabase(void);
unsigned char InitializeItemDatabase(void);
unsigned char InitializeLevelDatabase(void);
unsigned char InitializeItemTables(void);
void DestroyItemTables(void);
void Function54B100(void);
void Function54B560(void);
void Function54B300(unsigned int slot);
unsigned char InitializeNpcDatabase(void);
void DestroyNpcDatabase(void);
void DestroyFactDatabase(void);
void DestroyItemDatabase(void);
void DestroyLevelDatabase(void);
void FreeIfNotNull(void* block);
unsigned char AllocateStatusBuffers(W8StatusBuffers* status);
void FreeStatusBuffers(W8StatusBuffers* status);
void ResetPartySlotRow(int slot);
void ResetGameplayStatusBlock(void);
void ResetTargetingState(void);
void DestroyGameplayObjects(void);
bool CheckCdPresent(void);
W8MonsterRecord* MonsterDBFromSpecies(unsigned int monster_species);
void WorldUpdateProps(W8World* world);
/* 0x0054A8A0, reviewed in evidence/reviewed/wiz8/functions.csv. */
unsigned char LoadMonsterDatabaseRecord(unsigned int monster_species, W8MonsterRecord* record);
W8World* GetWorld(void);
/* The second world the 3D API keeps, and the renderer's catch-up request.
   Both are defined in Engine Code\3dapi.cpp. */
W8World* GetWorld659AB8(void);                                           /* 0x004512A0 */
void MarkRendererReady(void);                                            /* 0x00451010 */
int GetItemInHand(void);
int GetLocationVarIDByName(const char* name);
unsigned char GetStringFromStringDatabase(
    const char* path,
    int index,
    W8WideChar* output,
    unsigned int* metadata_04,
    unsigned int* metadata_00);
void ShowString(W8WideChar* text);
void GetOriginOfCharacterItem(
    int character_index,
    void* item,
    unsigned char* origin,
    unsigned short* slot);
void AddMessageBoxLine(int type, W8WideChar* text, void* extra);

}

#endif
