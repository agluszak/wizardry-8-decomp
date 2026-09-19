#ifndef WIZ8_LOCAL_CODE_MONSTER_MANAGER_H
#define WIZ8_LOCAL_CODE_MONSTER_MANAGER_H

#include <stddef.h>

#include "surrender/srMath.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/gameplay_modifiers.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/mouth_gap.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/vector.h"

struct W8CharacterEvent;
struct W8NpcState;

struct W8PortraitQuoteState {
    int quote_handle;
    unsigned short x;
    unsigned short y;
    unsigned short width;
    unsigned short height;
};

/* One party-slot record. The element constructor and destructor at
   0x004E6A30 and 0x004E6A10 exist because each record owns the ordinary
   vector at +0x0D8. The reset at 0x0054B300 clears a record wholesale despite
   that non-trivial member; that source behavior does not turn the vector into
   a second layout projection.

   Eight of these are the leading member of packed gXStatus at 0x006836B8.
   The constructor at 0x004E6970 and destructor at 0x004E6940 are W8XStatus
   lifecycle, not TU dynamic initializers for standalone globals. */
#pragma pack(push, 1)
struct W8MonsterManagerEntry {
    W8MonsterManagerEntry();
    ~W8MonsterManagerEntry();

    unsigned char portrait_event_active;
    int voice_sound_handle;
    W8MouthGapTrack mouth_gap;  /* 0x005 */
    W8PortraitQuoteState quote; /* 0x019 */
    unsigned char unknown_029[0x4c];
    W8CharacterEvent* active_character_event;
    int previous_portrait_frame;
    int portrait_frame;
    int portrait_frame_clock;
    int voice_time_remaining_ms;
    int previous_portrait_pose;
    int portrait_pose;
    int target_portrait_pose;
    int portrait_pose_clock;
    int portrait_idle_clock;
    unsigned char portrait_pose_animation_active;
    unsigned char portrait_pose_dirty;
    unsigned char portrait_frame_dirty;
    unsigned char field_09c;
    unsigned char field_09d;
    unsigned char field_09e;
    int field_09f;
    int field_0a3;
    int field_0a7;
    unsigned char field_0ab;
    int field_0ac;
    int field_0b0;
    int field_0b4;
    int field_0b8;
    unsigned char field_0bc;
    unsigned char field_0bd;
    int field_0be;
    int field_0c2;
    int field_0c6;
    int field_0ca;
    unsigned char field_0ce;
    unsigned char field_0cf;
    unsigned char field_0d0;
    unsigned char field_0d1;
    int field_0d2;
    unsigned short field_0d6;
    W8GrowableVector<int> highlighted_monsters; /* 0x0d8 */
    unsigned char field_0e8;
    unsigned char unknown_0e9[0x2a];
    unsigned char field_113;             /* 0x113: pose/direction threshold comparisons */
    unsigned int pending_event_type_114; /* 0x114: last queued portrait event type */
}; /* 0x118 */
#pragma pack(pop)

static_assert(sizeof(W8GrowableVector<int>) == 0x10, "W8GrowableVector_int_size_must_be_0x10");
static_assert(sizeof(W8PortraitQuoteState) == 0x0c, "W8PortraitQuoteState_size");
static_assert(offsetof(W8MonsterManagerEntry, mouth_gap) == 0x05,
              "W8MonsterManagerEntry_mouth_gap_offset");
static_assert(offsetof(W8MonsterManagerEntry, quote) == 0x19, "W8MonsterManagerEntry_quote_offset");
static_assert(offsetof(W8MonsterManagerEntry, quote.quote_handle) == 0x19,
              "W8MonsterManagerEntry_quote_handle_offset");
static_assert(offsetof(W8MonsterManagerEntry, quote.x) == 0x1d,
              "W8MonsterManagerEntry_quote_x_offset");
static_assert(offsetof(W8MonsterManagerEntry, quote.y) == 0x1f,
              "W8MonsterManagerEntry_quote_y_offset");
static_assert(offsetof(W8MonsterManagerEntry, quote.width) == 0x21,
              "W8MonsterManagerEntry_quote_width_offset");
static_assert(offsetof(W8MonsterManagerEntry, quote.height) == 0x23,
              "W8MonsterManagerEntry_quote_height_offset");
static_assert(sizeof(W8MonsterManagerEntry) == 0x118, "W8MonsterManagerEntry_size_must_be_0x118");

W8MonsterRecord* MonsterDBFromSpecies(unsigned int monster_species);
W8MonsterInfo* CreateMonsterInfo(W8MonsterGroup* group, W8MonsterRecord* record,
                                 srVector3T<float>* position);

/* One queued monster action, 0x30 bytes: the kind/detail pair, the attack
   index the plain attack alone carries, an inline combat slot whose type and
   id words QueueMonsterAction fills by target kind, and a random 1..100
   tie-break so equal decisions do not always resolve the same way. */
struct W8MonsterAction {
    int action_kind;         /* 0x00 */
    int action_detail;       /* 0x04 */
    int attack_index;        /* 0x08 */
    W8CombatSlot target;     /* 0x0c */
    unsigned char tie_break; /* 0x2c */
    unsigned char unknown_2d[3];
}; /* 0x30 */

enum { W8_MONSTER_ATTR_COUNT = 5 };

/* The 0x153-byte combat allocation has two adjacent runs of 0x11-byte records.
   ClearEffectSlot consumes a record whenever its leading active byte is set. */
#pragma pack(push, 1)
extern int g_dword_6850be;

struct W8MonsterCombatState {
    /* 0x000: the phase of the round this monster next acts on, zero when it
       has finished acting. */
    unsigned int phase;
    bool active; /* 0x004 */
    /* 0x005: the round's attack count staged beside attacks_per_round when a
       chosen attack is committed; a four-byte store. */
    unsigned int unknown_005;
    /* 0x009: how many attacks it gets this round, which is what divides the
       remaining phases between them. */
    int attacks_per_round;
    /* 0x00d: swings left in the current attack, rolled from the record's
       swings_per_round_0e6 when the attack starts and read back for the
       announcement message. A four-byte store; the attack-resolution
       assertion spells it uiSwingsRemaining. */
    unsigned int uiSwingsRemaining;
    /* 0x011: the attack index Monster.cpp launches when combat has already
       selected this monster. It is asserted below MAX_MONSTER_ATTACKS before
       indexing the database record. */
    unsigned int attack_index_11;
    unsigned char unknown_015;
    /* 0x016: the queue of actions the monster's AI has decided on, one
       W8MonsterAction each. The AI owns the list and destroys it outright. */
    W8PList* plsCombatActionList;
    int character_hate[9];
    W8EffectSlot effect_slots_3e[9]; /* 0x03e .. 0x0d7 */
    W8EffectSlot effect_slots_d7[6]; /* 0x0d7 .. 0x13d */
    /* 0x13d: how many times the monster already rolled to notice an attacker
       this round, the same scheme as the character row's spot_attempts_90. */
    int spot_attempts_13d;
    /* 0x141: the monster's pending-action repick count, the same scheme as
       the character row's pending_action_repick_count; the attack-score
       surprise penalty scales with it. */
    unsigned int pending_action_repick_count;
    unsigned char unknown_145[2];
    /* 0x147: the round's interception count, checked against the record's
       attacks_per_round before another intercept is allowed and bumped on
       each successful one. */
    unsigned int interception_count;
    /* 0x14b: the monster is committed to advancing on the party. Set when the
       action executor starts the advance and cleared when an enemy is inside
       short range or when the forcing condition is removed. */
    unsigned char advancing_14b;
    int value_14c; /* 0x14c */
    /* 0x150: the monster's turn has been set up already, so the setup runs
       once per turn however often it is asked for. */
    unsigned char turn_started;
    unsigned char unknown_151[2];
}; /* 0x153 */
#pragma pack(pop)

#pragma pack(push, 1)
/* 0x286: the party-side sight record for one monster. The live-threat gate,
   the clock and two position triples the player-sight pass stamps, the
   use-bounds flag IsVisibleToPlayer consumes, and its two sight flags. The
   per-turn reset zeroes all 0x30 bytes together, which fixes the extent. */
struct W8PartyThreatRecord {
    unsigned char unknown_00[4];
    unsigned char state_04; /* 0x28a: live-threat gate for the group sight query */
    /* 0x28b: the sight-flag pair GetPlayerToMonsterSightFlags writes;
       CanPartyMemberAimAtMonster indexes it by the resolved action's
       ranged flag. */
    unsigned char sight_flags_05[2];
    unsigned char flag_07;                /* 0x28d */
    int last_seen_clock_08;               /* 0x28e: cleared by the per-turn reset */
    srVector3T<float> camera_position_0c; /* 0x292 */
    srVector3T<float> own_position_18;    /* 0x29e */
    unsigned char threat_state_24;        /* 0x2aa: use-bounds flag handed to IsVisibleToPlayer */
    unsigned char flag_25;                /* 0x2ab */
    unsigned char unknown_26[0x0a];
}; /* 0x30 */
static_assert(sizeof(W8PartyThreatRecord) == 0x30, "W8PartyThreatRecord_size");

/* One 0x31-byte visibility record. W8MonsterInfo embeds the party-facing one
   at 0x348 and the mon-to-mon list allocates one per other monster; both store
   the observer's position at 0x10 and the observed entity's at 0x1c as
   ordinary floats. The reset zeroes exactly its 0x31 bytes. */
struct W8VisibilityRecord {
    int about_location_id;           /* 0x00; always zero in the party record */
    unsigned char state_04;          /* 0x04 */
    unsigned char sight_flags_05[4]; /* 0x05: two flag pairs */
    unsigned char unknown_09[2];
    unsigned char flag_0b;                 /* 0x0b */
    int last_seen_clock_0c;                /* 0x0c */
    srVector3T<float> subject_position_10; /* 0x10: the observer */
    srVector3T<float> target_position_1c;  /* 0x1c: the observed */
    unsigned char line_of_sight_28;        /* 0x28 */
    unsigned char unknown_29[8];           /* 0x29 */
}; /* 0x31 */
static_assert(sizeof(W8VisibilityRecord) == 0x31, "W8VisibilityRecord_size");

struct W8MonsterInfo {
    int location_id;              /* 0x00 */
    int monster_group_id;         /* 0x04: group lookup input in 0x004e6020 */
    unsigned int monster_species; /* 0x08 */
    W8Monster* monster;           /* 0x0c: p3D, named by source assertions */
    /* 0x10: pCombat, named by the MonsterManager.cpp:672 assertion
       "pMonsterInfo->pCombat != NULL" over the malloc 0x004e4390 stores here.
       The allocation is 0x153 bytes, zeroed as 0x54 dwords plus a word and a
       byte, and 0x004e4500 frees it and nulls the field again. */
    W8MonsterCombatState* pCombat;
    bool fActive; /* 0x14: live-entry gate in 0x004e5c00 */
    /* 0x15: fInCombat, named by the MonsterManager.cpp:666 and :712 assertions
       "!pMonsterInfo->fInCombat" and "pMonsterInfo->fInCombat", which bracket
       the pair that allocates and releases pCombat. */
    unsigned char fInCombat;
    /* 0x16: the monster's disposition, named by the 0x00530f10 assertion
       "pMonsterInfo->ubDisposition != DISP_HOSTILE". Copied from the group's
       ubDisposition when the entry is created. */
    W8Disposition ubDisposition;
    /* 0x17: the spawn position, unaligned. 0x004e3930 copies the caller's three
       floats here and hands the same triple to GetCameraFacingYaw004BE5C0,
       whose result it stores next, and to 0x0042e620 with the new entry's id. */
    srVector3T<float> position_17;
    float derived_23;        /* 0x23: camera-facing yaw over position_17 */
    int hp_max;              /* 0x27: signed divisor at 00531657 and 004E5A7A */
    unsigned int hp_current; /* 0x2b: unsigned conversion at 0053164B */
    int stamina_max;         /* 0x02f: initialized from MONSTERS.DBS dice */
    int stamina;             /* 0x033: initialized to the same roll */
    /* 0x37: the position and radius of the last noise this monster heard;
       Noise.cpp writes the heard position and the radius that carried. */
    srVector3T<float> heard_noise_position_37;
    int heard_noise_radius_43;
    /* 0x47/0x4b: the hit-point regeneration rate and its fractional
       accumulator, styled on 0x0048c120's stamina pair below. */
    float hp_regen_rate_47;
    float hp_regen_accumulator_4b;
    float stamina_regen_rate_4f;
    float stamina_regen_accumulator_53;
    /* 0x057: the monster's copy of the character condition array, entry for
       entry - condition two doubles its action fatigue at 0x05f, eight blocks
       its spellcasting at 0x077, thirteen makes it hostile at 0x08b, fifteen
       at 0x093 and seventeen is exhaustion at 0x09b. */
    unsigned int condition_turns[W8_CONDITION_COUNT]; /* 0x057 */
    W8Enchantment enchantments[8];                    /* 0x0a7 */
    /* 0x107: highest set condition_turns index; 0x12 when deactivated. The
       0x0056C5E0 gate compares it unsigned. */
    unsigned int highest_condition;
    /* 0x10b: the argument a condition carries when a monster's conditions are
       copied onto a character. */
    int condition_argument;
    W8EffectSlot effect_slots_10f[12];
    W8GameplayModifierBlock modifiers_1db; /* 0x1db */
    int fatigue_band;                      /* 0x242: derived from stamina */
    unsigned char unknown_246;
    unsigned char attributes[W8_MONSTER_ATTR_COUNT]; /* 0x247: values clamped to 1..125 */
    unsigned char unknown_24c;
    unsigned char within_viewing_distance; /* 0x24d: cycle-2 eligibility gate */
    unsigned char fMotionless;             /* 0x24e: fMotionless in the demo diagnostic */
    float scale_24f;                       /* 0x24f: HP-dependent live Monster scale */
    unsigned char flag_253;                /* 0x253: set by 0x004e5c00 after processing */
    unsigned char unknown_254;
    unsigned char flag_255; /* 0x255: reset by 0x004e5ea0 and 0x004e6020 */
    unsigned char unknown_256[0x30];
    W8PartyThreatRecord party_threat; /* 0x286 */
    /* 0x2b6: what this monster can see of other monsters, one heap record per
       other monster. The two release paths own it: one drops every record
       about a departing monster, the other empties and destroys the whole
       list. */
    W8PList* plsVisMonToMon;
    /* 0x2ba: passed by address to 0x00536170 when combat begins; extent runs to
       the next established field, so the array bound is a partition of the
       unknown run rather than a proven size. */
    W8CombatSlot Target;
    /* 0x2da: summon marker - 0 ordinary, 1 friendly summon, 2 hostile summon;
       nonzero raises the summoned spell icon and feeds the slain cleanup. */
    int summoned_2da;
    /* 0x2de: the monster is under the effect the magic code clears by name;
       clearing it posts a notice and drops the visual. The NPC price-check
       dispatch reads it signed (MOVSX) as a percentage discount on the quoted
       price. */
    signed char effect_2de;
    /* 0x2df: the committed attack already launched its missile; asserted by
       ContinueMonsterAttack when an out-of-range attack reports no release. */
    unsigned char f_missile_released;
    /* 0x2e0: the committed spell/special attack already released its payload;
       the action step asserts on it in the spell-wait case. */
    unsigned char fSpellReleased;
    /* 0x2e1: the action the monster is taking, -1 through 9. Its whole domain
       is enumerated by MonsterActionFatigueCost, whose error text names it. */
    int action_kind;
    /* 0x2e5: qualifies action kind zero; three costs markedly more. */
    int action_detail;
    unsigned int spell_power_level;
    unsigned char unknown_2ed[4];
    /* 0x2f1: this script part's bound NPC slot in g_npc_states, released when
       the entry is destroyed. */
    int bound_npc_index;
    /* 0x2f5: the monster's own contribution to the spell-point budget its
       database record sets a base for; the power-level chooser adds the two
       and reports a DATA ERROR when the base is zero. */
    int sp_budget_bonus;
    /* 0x2f9: the monster's live spell-point pool; the group-attack drain
       halves each resisted amount off it. */
    unsigned int spell_points_2f9;
    int control_state; /* 0x2fd: group-recomputed control state */
    unsigned char cycle17_state;
    /* 0x302/0x303: the two alternating look-around timers the aging pass
       counts down and rearms from the monster's look frequency/duration. */
    unsigned char look_timer_302;
    unsigned char look_timer_303;
    /* 0x304: the condition's own target source, copied in whole by the
       condition setter. */
    W8TargetSource condition_target_304;
    int movement_watch_position[3]; /* 0x338: creator clears as one unit */
    /* 0x344: location id of the phantom an Insanity effect summoned against
       this monster, -1 while none is bound; a bound monster cannot be picked
       again. */
    int insanity_summon_344;
    W8VisibilityRecord player_visibility; /* 0x348 */
    unsigned char unknown_379;
    unsigned char has_missile_37a;
    unsigned char unknown_37b;
    unsigned char has_spell_37c;
    unsigned char unknown_37d[0xa8];
}; /* 0x425 */
#pragma pack(pop)

static_assert(sizeof(W8MonsterInfo) == 0x425, "W8MonsterInfo_size_must_be_0x425");
static_assert(offsetof(W8MonsterInfo, modifiers_1db) == 0x1db,
              "W8MonsterInfo_modifiers_1db_offset");

W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int monster_list_index);
bool InitializeMonsterManagerState(void);
void ActivateMonsterInWorld(W8MonsterInfo* monster_info);
void ActivateMonster(W8MonsterInfo* monster_info, int mode);
void ClearMonsterPathAndResume(W8MonsterInfo* monster_info);
void MonsterStartsDying(W8MonsterInfo* monster_info, char display_message);
W8MonsterRecord* GetMonsterDataForInfo(W8MonsterInfo* monster_info);
unsigned int MonsterGetIndexByLocationID(int caller_line, const char* caller_file, int location_id,
                                         unsigned char assert_on_failure);
W8MonsterInfo* MonsterInfoFromID(int caller_line, const char* caller_file, int location_id,
                                 unsigned char assert_on_failure);
W8MonsterRecord* GetMonsterDataByLocationID(int location_id);
W8Monster* GetMonsterByLocationID(int location_id);
float GetMonsterCombatMoveRange(W8MonsterInfo* monster_info);
void UpdateMonsterDamageAppearance(W8MonsterInfo* monster_info);
W8MonsterInfo* GetNextMonsterInfo(unsigned char reset_iterator);
int GetMonsterQuadrant(W8MonsterInfo* monster_info);
int GetMonsterCycleFallbackValue004E5B50(unsigned int monster_species);
void ProcessMonstersAtCombatEnd(unsigned char forced_cleanup);
void ConvertMonsterAttributes(W8MonsterInfo* monster_info);
W8MonsterInfo* FindMonsterInfoBySpecies(unsigned int monster_species);
void ResetLivingMonstersAfterCombat(void);
void DestroyUngroupedMonsters(void);
void SetMonsterControlState(W8MonsterInfo* monster_info, int control_state);
void MonsterInfoSetMotionless(W8MonsterInfo* monster_info, unsigned char motionless);
void MoveMonsterToLiveList(W8MonsterInfo* monster_info);
W8MonsterInfo* FindNearestMonsterInfo(const srVector3T<float>* position, double maximum_distance);
void InitializeMonsterRuntimeStats(void);
float CalculateMonsterScale(W8MonsterInfo* monster_info);
void TryStartMonsterCycle2(W8MonsterInfo* monster_info, W8Monster* monster, int query_state);
void ProcessMonsterManagerFrame(void);
void FormatMonsterHealth(W8MonsterInfo* monster_info, wchar_t* health_text);
unsigned int GetMonsterExperience(const W8MonsterRecord* record);
bool AnyMonsterDying(void);
float GetAveragePartyMemberLevel(void); /* 0x004EFB60 */
/* 0x00554490: the highest `skills[skill_index].level` among live party members;
   `party_slot` receives the best member's slot. */
unsigned int GetBestPartySkillLevel(int skill_index, int* party_slot);

void StartMonsterCycle(W8MonsterInfo* monster_info, int cycle, int behavior);
void MonsterInfoLeaveCombat(W8MonsterInfo* monster_info);
unsigned char ShutdownMonsterManager(void);

wchar_t* GetMonsterName(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                        unsigned char name_form);
unsigned char RemoveMonster(unsigned int monster_list_index, unsigned char destroy_monster);
void MonsterInfoEnterCombat(W8MonsterInfo* monster_info);
void DeactivateMonster(W8MonsterInfo* monster_info);
void ToggleCombatMode(void); /* 0x004E6A80 */
void TogglePartyCombatStance(void);
void Function4E4AB0(void); /* 0x004E4AB0 */
void Function4E6CE0(void); /* 0x004E6CE0 */

#endif
