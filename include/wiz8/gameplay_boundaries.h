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
#ifdef __cplusplus
#include "surrender/srMath.h"
#include "wiz8/startup_runtime_state.h"
#endif

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

/* A world position as the packed records carry it: three floats, C-compatible,
   distinct from srVector3T<float> which only exists for C++ consumers. */

/* The 0x3c-byte anchor a character carries and the recall effect restores.
   Only the leading point is read field by field; the rest travels as one
   block, so nothing beyond it is named. */
typedef struct W8SavedLocation {
    srVector3T<float> point;                    /* 0x00 */
    unsigned char unknown_0c[0x30];
} W8SavedLocation;                       /* 0x3c */

/* The game's wide text format: fixed-size UINT16 arrays stored inline in
   records and manipulated through the CRT wide-string functions. Under VC6
   wchar_t is unsigned short, so the two spellings are one type. */

/* The twenty-one factions, the domain the name lookup enumerates. */
enum { W8_FACTION_COUNT = 21 };

typedef struct W8FactionRuntimeRecord {
    signed char disposition_score;      /* 0x00 */
    unsigned char unknown_01[3];
    int value_04;                       /* 0x04 */
    unsigned char flag_08;              /* 0x08 */
    unsigned char unknown_09[5];
} W8FactionRuntimeRecord;               /* 0x0e */

/* One attribute record. The array is indexed by skill id biased by 0x22, so the
   seven attribute ids sit at the top of the skill numbering; only the leading
   value, which IsCharacterSkillAvailable tests against 100, is established. */
typedef struct W8CharacterAttribute {
    unsigned int value;                   /* 0x00 */
    /* 0x04: the value after equipment and effects. Resistance recalculation
       reads this one, not the base, and only above a threshold of 0x50. */
    unsigned int effective;
    unsigned char unknown_08[0xc];
} W8CharacterAttribute;                   /* 0x14 */

/* One skill record, indexed directly by skill id. PracticeCharacterSkill
   establishes the stride and the leading flag it sets when a skill first
   becomes available; IsCharacterSkillAvailable reads the same flag. */
typedef struct W8CharacterSkill {
    unsigned char flag_00;                /* 0x00 */
    unsigned char unknown_01;
    /* 0x02: a second figure the spell-learning ceiling divides by ten, the
       same way the resistances divide `level`. The two are distinct fields of
       one skill, not one field read two ways. */
    unsigned int value_02;
    /* 0x06: the skill's current level. Resistance recalculation divides it by
       ten for skills 28..33 and by five for skill 36, which is what places it. */
    unsigned int level;
    unsigned char unknown_0a[0x1c];
} W8CharacterSkill;                       /* 0x26 */

/* One resistance channel. Recalculation rebuilds `base` from scratch each time
   and then derives `total` from it, so the two are a computed pair rather than
   a stored value and a cache. */
typedef struct W8CharacterResistance {
    unsigned int base;                    /* 0x00 */
    unsigned int total;                   /* 0x04: clamped to 100 */
    unsigned char unknown_08[8];
} W8CharacterResistance;                  /* 0x10 */

/* The six realms a spell point pool is kept per are W8SpellRealm's, declared
   with the spell record in wiz8/layouts/gameplay_databases.h. */

enum {
    W8_RESISTANCE_COUNT = 6,
    /* The six skills whose level feeds the matching resistance, and the one
       whose presence adds a flat bonus to every one of them. */
    W8_FIRST_RESISTANCE_SKILL = 28,
    W8_RESISTANCE_BONUS_SKILL = 36,
    /* A race adjustment at or below this is a flat amount; above it, it selects
       a character attribute by index biased this far. */
    W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS = 1000
};

/* One hand's attack block. The extent is the stride between the two, and only
   the two leading fields are established. */
typedef struct W8HandAttack {
    unsigned char in_play;                /* 0x00 */
    int weapon_skill;                     /* 0x01, unaligned */
    unsigned char unknown_05[0xc];
    /* 0x11: what the hand's attack is worth, read only once the hand has a
       range to the target at all. */
    int attack_value;
    unsigned char unknown_15[0x46];
} W8HandAttack;                           /* 0x5b */

typedef struct W8Character {
    /* 0x0000: SaveCharacter stamps 1 here before writing the record, so the
       leading dword is a saved-record version rather than runtime state. */
    unsigned int record_version;
    unsigned char in_party;              /* 0x0004 */
    /* 0x0005: the character's name, wide, and the stem SaveCharacter formats
       "%ls.CHR" from. The extent below partitions the unknown run up to the
       profession at 0x0069; it is not proven, and only the fact that a wide
       string starts here is. */
    wchar_t name[0x10];
    unsigned char unknown_0025[0x44];
    /* 0x0069 and 0x006d: iProfession, named by the GameplayCode.cpp:399
       assertion that bounds it against PROF_COUNT, and the profession the
       character started in. The level band subtracts a base only while the two
       agree. */
    int current_profession;               /* 0x0069 */
    int original_profession;              /* 0x006d */
    int race;                             /* 0x0071: indexes the race resistance table */
    int faction;                          /* 0x0075: compared against the caller's faction */
    /* 0x0079: looked up from the table at 0x00616604 by faction, race and
       profession together. Note that the index multiplies 0x0075 by sixteen
       against an eleven-race domain, so either that field is not the faction
       the disposition code reads or the table is sparse; the disagreement is
       recorded rather than resolved. */
    int table_value_0079;
    unsigned char unknown_007d[0xc];
    unsigned int level;                   /* 0x0089: averaged across occupied slots */
    int profession_levels[15];            /* 0x008d */
    unsigned char unknown_00c9[0x14];
    /* 0x00dd: the eight-band ladder over the character's level in their
       current profession, and the base subtracted from it while they are still
       in the profession they started in. */
    int level_band;
    int level_band_base;
    W8CharacterAttribute attributes[7];   /* 0x00e5, indexed by skill_id - 0x22 */
    unsigned char unknown_0171[0x2c];
    W8CharacterSkill skills[0x29];        /* 0x019d, indexed by skill_id */
    unsigned char unknown_07b3[0x23a];
    /* 0x09ed..0x09f8: experience, the goal for the next level, and the goal
       the previous level had. A character is ready to advance once the first
       reaches the second. */
    unsigned int experience;
    unsigned int experience_goal;
    unsigned int experience_previous_goal;
    unsigned char unknown_09f9[4];
    /* 0x09fd: how many times this character has died. */
    int death_count_09fd;
    /* 0x0a01: one entry per condition, holding how long it has left to run;
       W8_CONDITION_INDEFINITE means until something lifts it. The exhausted
       condition the fatigue path calls 0x11 lands exactly on element
       seventeen, which is what fixes base and stride, and the monster carries
       the identical array at its own 0x57 with the same indices meaning the
       same things. Several entries were read individually before this array
       explained them: two doubles the fatigue an action costs, eight blocks
       spellcasting, eighteen unlocks bound equipment. */
    int condition_turns[W8_CONDITION_COUNT];   /* 0x0a01 */
    unsigned char unknown_0a51[0x14];
    W8Enchantment enchantments[8];             /* 0x0a65 */
    unsigned char unknown_0ac5[0x3c];
    /* 0x0b01 gates party-member selection alongside hp_current: a slot is
       eligible when it still has hit points and this is under 0x12, and a
       second tier tests it against 0x0f. It is unsigned - the canonical
       compares are JB/JBE, not JL/JE - but its meaning is not established. */
    unsigned int unknown_0b01;
    /* 0x0b05: the highest enchantment slot still in use, recomputed by
       scanning down from the last one whenever a slot is cleared. */
    int enchantment_top;
    /* 0x0b09: the argument the seventh condition carries. */
    int condition_argument;
    /* 0x0b0d..0x0b20: the two pools with a ceiling each, plus the adjustment
       damage is booked against before hit points are recalculated. A character
       whose hp_current is zero is treated as out of the fight everywhere. */
    int hp_max;                           /* 0x0b0d */
    unsigned int hp_current;              /* 0x0b11 */
    int hp_adjustment;                    /* 0x0b15 */
    int stamina_max;                      /* 0x0b19 */
    int stamina;                          /* 0x0b1d */
    unsigned char unknown_0b21[4];
    /* 0x0b25 and 0x0b45: iSPMax and iSPLeft, one per spell realm, named by the
       Health Stamina Mana.cpp:1067 assertion pPC->iSPLeft[uiRealm] and bounded
       at six realms by the total the party-wide restore accumulates. */
    int sp_max[W8_SPELL_REALM_COUNT];     /* 0x0b25 */
    unsigned char unknown_0b3d[8];
    int sp_left[W8_SPELL_REALM_COUNT];    /* 0x0b45 */
    unsigned char unknown_0b5d[0x6c];
    /* 0x0bc9: the load category, zero through four, which scales what an
       action costs in fatigue. FatigueCharacter's error text calls it that. */
    int load_category;
    /* 0x0bcd: one entry per spell. CanCharacterUseItem refuses a spell-source
       item whose spell already reads one here, so one is the learned state. */
    int spell_learned[136];               /* 0x0bcd */
    /* 0x0ded: indexed by skill_id. For the six realm skills it counts the
       spells known in that realm, which is what makes the skill available at
       all - LearnSpell bumps the entry and IsCharacterSkillAvailable reads it.
       LearnSpell also writes a recomputed figure into entry 36, which is a
       real skill id but not a count; that disagreement is recorded rather than
       resolved. */
    unsigned int skill_unlocks[0x29];
    unsigned char unknown_0e91[0x48];
    /* 0x0ed9: a percentage taken off incoming damage, the character's
       counterpart of the monster's own at 0x1e1. */
    int damage_reduction;
    W8CharacterResistance resistances[W8_RESISTANCE_COUNT]; /* 0x0edd */
    unsigned char unknown_0f3d[0x20];
    /* 0x0f5d: the twelve worn/held slots, indexed by the same slot numbering
       GetItemDefaultEquipSlot answers and GetItemEquipSlotMask sets bits for.
       Slots six and seven are the primary hands and eight and nine the
       alternate pair, which is what the two-handed and off-hand tests read. */
    W8ItemInstance equipment[12];         /* 0x0f5d */
    unsigned char unknown_0fed[0x3c];
    /* 0x1029: the eight per-character carried slots. GetOriginOfCharacterItem
       reports this array as origin zero and the equipment array as origin one. */
    W8ItemInstance backpack[8];           /* 0x1029 */
    unsigned char unknown_1089[0xc4];
    /* 0x114d: one block per hand. Only the leading flag - the hand is in play -
       and the weapon skill the attack setup stamps into it are established. */
    W8HandAttack hand_attacks[2];         /* 0x114d */
    unsigned char unknown_1203[0x49b];
    /* 0x169e: the fatigue band, zero through four, recomputed from the stamina
       fraction whenever it moves; a change re-runs the armour class pass. */
    int fatigue_band;
    unsigned char unknown_16a2[0xd5];
    signed char resistance_bonus_all;     /* 0x1777: added to every resistance */
    unsigned char unknown_1778[0x34];
    signed char resistance_bonus[W8_RESISTANCE_COUNT];      /* 0x17ac */
    unsigned char unknown_17b2[3];
    /* 0x17b5: the character is out of the formation, which is what the
       front-rank counts skip. */
    unsigned char out_of_formation;
    unsigned char unknown_17b6[0x21];
    /* 0x17d7: where the character was last anchored, restored by the recall
       effect in Magic Effects.cpp. Its extent is proven rather than assumed:
       the cross-level path copies exactly 0x3c bytes from here with one
       rep movsd, which lands precisely on the level id below. */
    W8SavedLocation saved_location;      /* 0x17d7 */
    /* 0x1813: which level that anchor belongs to. The recall compares it
       against g_current_level and takes a different path when they differ. */
    int saved_level;                     /* 0x1813 */
    unsigned char unknown_1817[0x44];
    /* 0x185b: the deep-fatigue effect is already on this character, which is
       what stops FatigueCharacter re-applying it every turn. */
    unsigned char deep_fatigue_applied;
    unsigned char unknown_185c[5];
    /* 0x1861: the anchor above has been set. Recall does nothing without it. */
    unsigned char has_saved_location;
} W8Character;                           /* 0x1862 */

typedef struct W8RPCSlot {
    unsigned char opaque[0x118];
} W8RPCSlot;



typedef struct W8LevelFolderRecord {
    char folder_name[50];
    char level_name[50];                 /* 0x32 */
    char location_code[4];               /* 0x64: three-letter code plus NUL */
    signed char sky_index;                /* 0x68: -1 means this level has no sky */
    signed char unknown_69;
    signed char unknown_6a;
} W8LevelFolderRecord;                   /* 0x6b */

struct W8World;

typedef struct W8NPCRecordRef {
    unsigned char unknown_00[0x58];
    int record_id;                        /* 0x058: matches W8NpcDatabaseRecord::record_id */
} W8NPCRecordRef;

typedef struct W8NPCItemList {
    int unknown_00;                       /* 0x00: handle passed to 0x0055A0A0 */
    unsigned char unknown_04[2];
    W8NPCRecordRef* npc_record;           /* 0x06 */
    unsigned char unknown_0a[0x10];
    unsigned char flag_1a;                /* 0x1a: gates the teardown in LoadFactState */
} W8NPCItemList;



typedef unsigned char W8FactionDisposition;

/* Local Code\UtilityFunctions.cpp. These are the signed screen-space rectangle
   and point shapes consumed by 0x00517E20 and 0x00517E70. */
/* The same shape as W8ControlsRect; see the note there for why the two are
   kept apart. */
typedef struct W8ScreenRect {
    int left;
    int top;
    int right;
    int bottom;
} W8ScreenRect;

typedef struct W8ScreenPoint {
    int x;
    int y;
} W8ScreenPoint;

enum {
    W8_FACTION_HOSTILE = 0,
    W8_FACTION_NEUTRAL = 1,
    W8_FACTION_FRIENDLY = 2
};

/* Two faction ids the disposition calculation singles out before consulting the
   faction table at all. Their values come from that body's switch: zero falls
   through to the record's own hostility range, and one is always friendly. */
enum {
    W8_FACTION_UNALIGNED = 0,
    W8_FACTION_PARTY = 1
};

/* What a default disposition answers with. Distinct from W8FactionDisposition:
   the two scales disagree, and the calculation maps between them. */
enum {
    W8_DISPOSITION_NEUTRAL = 0,
    W8_DISPOSITION_HOSTILE = 1,
    W8_DISPOSITION_FRIENDLY = 2
};

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

#ifdef __cplusplus
extern "C" {
#endif

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
extern unsigned char g_camp_open_00683f9b;
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
extern W8PList* g_active_monster_list_00683fad;
/* 0x0068EC78: which screen is up. The whole 0x98-byte block lives there; the
   leading id is what everything outside the screen code reads, which is why
   this is a union rather than two globals. */
/* 0x005EBB34: the float that stands for "no distance given", which the level
   vector reads as its absent value too. */
extern float g_float_005ebb34;

/* Which screen is up, as g_screen_state_0068ec78.id holds it. Only the two the
   recovered bodies test are named. */
enum {
    W8_SCREEN_CAMP = 6,
    W8_SCREEN_MAIN_GAME = 7
};


extern W8FactionRuntimeRecord g_factions[21];
extern W8RaceResistanceProfile g_race_resistance_profiles[];
/* 0x00685178: one 0x106-byte row per party slot; only the leading byte is
   established, and GetRandomCharacter treats it as a slot-occupied flag. */
/* Flat table indexed by skill_id * 15 + profession. */
extern int g_profession_skill_availability[0x29][15];
extern int g_profession_bonus_skills[15];
/* Four skill ids per profession. */
extern int g_profession_skills[15][4];
extern int g_profession_magic_level_offsets[15];
/* 0x00615570: hit points a profession contributes per level it has been taken,
   before the vitality-derived multiplier. Fifteen entries; the recalculation at
   0x0052A2F0 walks it to the address one past the last, which is what fixes the
   extent. The entries are floats - the multiply is `fmul dword ptr` - and the
   table sits in .data rather than .rdata. */
extern float g_profession_hit_point_factors[15];
extern unsigned char g_log_fact_checks;
extern unsigned char g_fact_values[1001];
/* Suppresses the journal entry and its sound in the fact-change recorder at
   0x005588F0, which consults this before displaying anything. InitializeFactState
   brackets its whole run of SetFact calls with it. */
extern unsigned char g_fact_notifications_suppressed;
/* The Wizardry 7 party import, read from Saves\\Import by the parser at
   0x00558D00 and named for that path. It stores up to six 0x248-byte character
   records, then unpacks one option byte and a 96-bit flag mask into the
   following. Set when an import loaded successfully. */
extern unsigned char g_import_party_loaded;
extern int g_import_character_count;
/* Four mutually exclusive high bits of the imported option byte, mapped to 0-3,
   or -1 when no import applies. InitializeFactState turns 1 and 2 into two
   different facts and everything else into a third. */
extern int g_import_ending_choice;
/* One byte per bit of the imported 96-bit mask. Only elements 5 and 11 are
   consumed so far, by InitializeFactState. */
extern unsigned char g_import_flags[0x60];
/* 0x00616E84: one entry per item category, giving the interface presentation
   of whatever spell the item carries. */
extern const int g_item_spell_presentation[];
/* 0x00648C5C: one entry per equipment slot; -1 marks a slot that has no
   interface position, which is what the unequip rule tests for. */
extern const int g_equip_slot_icons[];
/* 0x0068517C: gStatus.fGameStarted, named by the PC Item.cpp assertion at line
   3795 that guards the party-pool sort. Item placement consults it because the
   character-creation screens fill different slots than play does. */
extern unsigned char g_game_started;
/* 0x00685189: the party's purse. AddPartyGold plays Data\Sound\Misc\ChaChing.wav
   and posts the pickup message when it is told to announce; SpendPartyGold is
   the matching debit and floors at zero rather than wrapping. */
extern unsigned int g_party_gold;
/* gXStatus.uiMonstersInDatabase, named by the assertion at GameplayDatabase.cpp
   line 320. It sits immediately below the NPC count, so the database counts are
   fields of one gXStatus structure rather than separate globals; they are kept
   separate here until that structure is recovered. */
/* Two gXStatus members named by the MonsterManager.cpp assertions at lines 92
   and 93. Recovering the whole structure is tracked separately; until then its
   members stay separate externs. */
/* Reset together by ResetGameplayStatusBlock; their meaning is not established,
   so they keep address-positional names. The object at 0x00683FD7 is torn down
   by 0x0054B0B0 through operator delete after the same method's sibling. */
extern unsigned char g_status_block_685078[56];
#ifdef __cplusplus
extern W8StartupRuntimeState* g_startup_runtime_state;
#else
extern void* g_startup_runtime_state;
#endif
extern void* g_object_685067;
extern unsigned char g_party_moving_006850b5;
extern unsigned char g_surprise_possible_00683fc5;
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
/* 0x00659AB8: the second world. It was spelled `void*` where it is defined and
   `W8World*` where the viewport reaches through it, which is two claims about
   one object; the viewport's is the one a body proves, since it reads the
   camera member off it. One declaration here settles it. */
extern W8World* g_world_659ab8;
/* Defined by DisplayList.cpp, which owns the list this gates; the recovered
   clear at 0x0040C220 reads it from outside that unit, so the declaration sits
   here rather than being spelled a second time where it is used. */
extern unsigned char g_display_flag_650e90;
extern unsigned char g_item_in_hand_valid;
extern W8ItemInstance g_item_in_hand;
/* The party's carried pool, bounded by the count that follows it in memory;
   0x0054B100 walks it by address rather than by index. */
extern W8ItemInstance g_party_item_pool[];
extern int g_party_item_count;
/* Starting item ids, terminated by the address after the last, with 0xffffffff
   marking an empty slot. */
extern unsigned int g_starting_item_ids[];
extern unsigned int g_starting_item_ids_end[];
extern W8LevelFolderRecord g_level_folders[47];
/* 0x00686A70: the level the party is on. The save loader compares it against
   the level id in an LVLS chunk, and it indexes the per-level rows below -
   bounded at 0x2f, the same forty-seven the folder table has. */
/* 0x00686B74: one row per level. LoadLevel owns the leading visited flag. Gold
   picked up there starts at +9 (0x00686B7D), and the clock its sight state was
   last brought up to follows at +0xd (0x00686B81). */
typedef struct W8LevelProgressRow {
    unsigned char visited;                /* 0x00 */
    unsigned char unknown_01[8];
    int gold_collected;                   /* 0x09 */
    int sight_clock;                      /* 0x0d */
    unsigned char unknown_11[0x10];
} W8LevelProgressRow;                     /* 0x21 */
extern W8LevelProgressRow g_level_progress[47];
extern int g_location_variable_count;
extern char** g_location_variable_names;
extern int g_location_variable_level_count;
extern int* g_location_variable_levels;
extern W8GrowableVector<W8NPCItemList*>* g_npc_item_lists;
/* 0x006840C7: lazily populated cache of runtime monster records, one slot per
   species ID. MAX_MONSTERS_IN_DATABASE comes from the canonical assertion text. */
/* Provisional name: a fixed-address, non-per-character item pool distinct
   from the equipped/carried slots GetOriginOfCharacterItem also searches. */
extern unsigned char g_shared_item_pool[];
extern unsigned int g_shared_item_pool_count;


/* 3D Code\IList.cpp: the integer sibling of W8PList, same shape with int
   elements, which is why a failed lookup returns -1 rather than null. */



void SetDice(W8Dice* dice, unsigned char count, unsigned char sides, short base);
int RollDice(const W8Dice* dice);
int IntegerPower(int base, unsigned int exponent);
void ClampInteger(int* value, int minimum, int maximum);
void ClampUnsignedInteger(unsigned int* value, unsigned int minimum,
                          unsigned int maximum);
int CompareUnsignedDescending(const unsigned int* first, const unsigned int* second);
int CompareSignedAscending(const int* first, const int* second);
int CompareSignedDescending(const int* first, const int* second);
char* FormatString(const char* format, ...);
wchar_t* FormatWideString(const wchar_t* format, ...);
wchar_t* ConvertStringToWide(const char* string);
char* ConvertWideStringToString(const wchar_t* string);
wchar_t* FormatUnsignedIntegerWithCommas(wchar_t* output, unsigned int value);
char* TitleCaseString(char* string);
float ShortestAngleDistance(float first, float second);
int GetNextCharacter(int require_primary, int require_secondary, int previous_slot);
int GetSpellTargetType(int spell_id, unsigned char normalize_single_target);
#ifdef __cplusplus
bool CanSpellBackfire(int spell_id);
#else
unsigned char CanSpellBackfire(int spell_id);
#endif
int MinimumCasterLevelForSpellLevel(int spell_level);
int GetMinimumCasterLevelForSpell(int spell_id);
/* 0x00521EF0, the address CFAgent seeds. Answers whether the item found a
   home, which one of its two callers tests and the other ignores. */
bool AddItemToParty(W8ItemInstance* item, int arg_2, int arg_3);
W8FactionDisposition GetFactionDisposition(signed char faction);
void RegionSetEnable(unsigned int region_set_index);
void RegionSetDisable(unsigned int region_set_index);
void ClearRegionSetModeBits(unsigned int region_set_index);
void SetRegionSetMode4(unsigned int region_set_index);
void ClearRegionModeBits(unsigned int region_index);
void SetRegionMode4(unsigned int region_index);
void SetRegionBounds(unsigned int region_index, unsigned short x1, unsigned short y1,
                     unsigned short x2, unsigned short y2);
#ifdef __cplusplus
bool RegionContainsPoint(unsigned int region_index, unsigned short x, unsigned short y);
bool RegionHasFlags(unsigned int region_index, unsigned int flags);
#else
unsigned char RegionContainsPoint(unsigned int region_index, unsigned short x,
                                  unsigned short y);
unsigned char RegionHasFlags(unsigned int region_index, unsigned int flags);
#endif
unsigned int CreateRegionSet(void);
void ResetRegionSet(unsigned int region_set_index);
unsigned int AddRegionToSet(unsigned int region_set_index);
void SetRegionCallback(unsigned int region_index, W8RegionCallback callback,
                       unsigned short callback_id);
void SetRegionOwner(unsigned int region_index, void* owner);
void SetRegionHelp(unsigned int region_index, unsigned char enabled, int help_text_id);
unsigned char ClearActiveRegionIfMatches(unsigned int region_index);
int RPCPtrToPCSlot(const W8RPCSlot* rpc);
void FreeStringTable(void);
#ifdef __cplusplus
bool IsStringTableLoaded(void);
#else
unsigned char IsStringTableLoaded(void);
#endif
void UnionScreenRects(const W8ScreenRect* first, const W8ScreenRect* second,
                      W8ScreenRect* result);
unsigned char ScreenPointInRect(const W8ScreenRect* rect, const W8ScreenPoint* point);
void StripMonsterNameSuffix(W8WideChar* name);
unsigned int CharacterPointerToPartySlot(W8Character* character);
unsigned char IsPartyCharacterPointer(const W8Character* character);
int GetProfessionCasterLevel(W8Character* character, int profession_id);
unsigned char IsCharacterSkillAvailable(W8Character* character, unsigned int skill_id,
                                        const unsigned char* expert_realm_flags);
unsigned char GetFact(int fact_id);
void SetFact(int fact_id, unsigned char value, unsigned char suppress_side_effects);
void SaveFactState(int save_handle);
void InitializeFactState(void);
void LoadFactState(int save_handle);
void SetFactNotificationsSuppressed(unsigned char suppressed);
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
int GetLoadedLevelID(void);
const char* GetLevelFolderName(int level_id);
unsigned char GetLevelLocationCode(int level_id, char* location_code);
W8MonsterRecord* MonsterDBFromSpecies(unsigned int monster_species);
void WorldUpdateProps(W8World* world);
/* 0x0054A8A0, reviewed in evidence/reviewed/wiz8/functions.csv. */
unsigned char LoadMonsterDatabaseRecord(unsigned int monster_species, W8MonsterRecord* record);
int FindLevelIdByLocationCode(const char* location_code);
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
W8NPCItemList* GetNPCItemListByID(int npc_record_id);
void GetOriginOfCharacterItem(
    int character_index,
    void* item,
    unsigned char* origin,
    unsigned short* slot);
void AddMessageBoxLine(int type, W8WideChar* text, void* extra);

#ifdef __cplusplus
}
#endif

#endif
