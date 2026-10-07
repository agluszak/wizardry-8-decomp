#ifndef WIZ8_LOCAL_CODE_MONSTER_MANAGER_H
#define WIZ8_LOCAL_CODE_MONSTER_MANAGER_H

#include <stddef.h>

#include "wiz8/monster_actions.h"
#include "wiz8/sight_state.h"

#include "timer.h"

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

/* One party-slot record; eight of these lead gXStatus. The reset clears a
   record wholesale despite its vector member. */
#pragma pack(push, 1)
struct W8MonsterManagerEntry {
    bool portrait_event_active;
    unsigned int voice_sound_handle;
    W8MouthGapTrack mouth_gap;
    W8PortraitQuoteState quote;
    unsigned char unknown_025[0x4c];
    W8CharacterEvent* active_character_event;
    int previous_portrait_frame;
    int portrait_frame;
    TIMER portrait_frame_clock;
    unsigned int voice_time_remaining_ms;
    int previous_portrait_pose;
    int portrait_pose;
    int target_portrait_pose;
    TIMER portrait_pose_clock;
    TIMER portrait_idle_clock;
    bool portrait_pose_animation_active;
    bool portrait_pose_dirty;
    bool portrait_frame_dirty;
    /* The floating damage-number splat animation. Posting a hit opens it with
       the hit's amount, accumulates further hits into
       the amount while it is already up, picks the normal splat catalog object
       (0x90/0x91 = damage_splat_anim.sti / damage_splat_anim2.sti), and switches
       to the death variant (0x92 = death_splat_anim.sti, 0x1e frames instead of
       8) when the character's hit points are gone. The ticker steps the frame
       once per portrait_fx_clock expiry; the frame is -1 while
       the splat waits for a forced portrait refresh or for the effect-icon
       animation to finish. At death-variant frame 0xd the reveal flag lets the
       portrait swap to the dead graphic underneath the splat. */
    bool damage_splat_active;
    bool damage_splat_death_variant;
    bool dead_portrait_revealed;
    int damage_splat_amount;
    int damage_splat_frame;
    int damage_splat_end_frame;
    unsigned char damage_splat_catalog;
    /* The last-drawn portrait vitals, so the dirty flag is raised only when
       the computed bars or the numeric hit-point display actually changed. */
    int cached_hp_bar;
    int cached_stamina_bar;
    int cached_spell_bar;
    int cached_hp;
    bool portrait_stats_dirty;
    /* The spell/condition effect-icon flash over the portrait.
       The setup derives the icon's catalog base from the spell's
       realm and takes the frame count from the same record, shares the
       portrait_fx_clock cadence with the damage splat, and likewise uses -1 as
       the deferred-start frame. */
    bool effect_icon_active;
    int effect_icon_frame;
    int effect_icon_catalog;
    int effect_icon_end_frame;
    /* The shared 100 ms frame clock both portrait animations tick on;
       the ticker rearms it whenever it expires. */
    TIMER portrait_fx_clock;
    /* Set when the keyboard SELECT_PC command pins the pending portrait
       refresh, so the formation sync does not auto-release it. */
    bool portrait_refresh_pinned;
    /* Marks a pending portrait refresh that was auto-issued for an
       in-flight splat/icon animation or portrait event, so the formation sync
       may release it even while the slot is under the cursor. */
    bool auto_portrait_refresh;
    /* The keyboard menu is open on this slot's portrait; region input is
       disabled and the menu panel redraws with the portrait. */
    bool keyboard_menu_open;
    /* The combat side-strip portrait needs redrawing; set by hover and
       combat-state changes, cleared by the combat portrait redraw. */
    bool combat_portrait_dirty;
    /* The acting combatant's portrait pulse - a countdown clock that rearms
       the 1..0xc brightness phase. */
    TIMER acting_portrait_pulse_clock;
    unsigned short acting_portrait_pulse;
    W8GrowableVector<int> highlighted_monsters;
    /* The character has reached its experience goal; set once to post
       the level-up notice line and cleared when the character is no longer
       ready to advance. */
    bool level_up_ready;
    /* Edge latch mirroring the character's uiCondition[19]: the
       periodic party sync copies it in and the reaction pass fires the
       condition-change event once while the latch is still clear. */
    bool condition_19_latch;
    /* Per-skill "increased" notice flags posted by PracticeCharacterSkill
       and drained into W8_NPC_MSG_SKILL_NOTICES message lines. */
    bool skill_notice_pending[W8_SKILL_COUNT];
    /* Set around SwapItemInstances so the autoswap-weapons check does
       not fire on the intermediate item states. */
    bool item_swap_in_progress;
    unsigned int pending_event_type; /* last queued portrait event type */
};
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
static_assert(offsetof(W8MonsterManagerEntry, active_character_event) == 0x71,
              "W8MonsterManagerEntry_active_character_event_offset");
static_assert(offsetof(W8MonsterManagerEntry, voice_time_remaining_ms) == 0x81,
              "W8MonsterManagerEntry_voice_time_remaining_ms_offset");
static_assert(offsetof(W8MonsterManagerEntry, damage_splat_active) == 0x9c,
              "W8MonsterManagerEntry_damage_splat_active_offset");
static_assert(offsetof(W8MonsterManagerEntry, damage_splat_amount) == 0x9f,
              "W8MonsterManagerEntry_damage_splat_amount_offset");
static_assert(offsetof(W8MonsterManagerEntry, damage_splat_catalog) == 0xab,
              "W8MonsterManagerEntry_damage_splat_catalog_offset");
static_assert(offsetof(W8MonsterManagerEntry, cached_hp_bar) == 0xac,
              "W8MonsterManagerEntry_cached_hp_bar_offset");
static_assert(offsetof(W8MonsterManagerEntry, portrait_stats_dirty) == 0xbc,
              "W8MonsterManagerEntry_portrait_stats_dirty_offset");
static_assert(offsetof(W8MonsterManagerEntry, effect_icon_active) == 0xbd,
              "W8MonsterManagerEntry_effect_icon_active_offset");
static_assert(offsetof(W8MonsterManagerEntry, portrait_fx_clock) == 0xca,
              "W8MonsterManagerEntry_portrait_fx_clock_offset");
static_assert(offsetof(W8MonsterManagerEntry, highlighted_monsters) == 0xd8,
              "W8MonsterManagerEntry_highlighted_monsters_offset");
static_assert(offsetof(W8MonsterManagerEntry, level_up_ready) == 0xe8,
              "W8MonsterManagerEntry_level_up_ready_offset");
static_assert(offsetof(W8MonsterManagerEntry, skill_notice_pending) == 0xea,
              "W8MonsterManagerEntry_skill_notice_pending_offset");
static_assert(offsetof(W8MonsterManagerEntry, item_swap_in_progress) == 0x113,
              "W8MonsterManagerEntry_item_swap_in_progress_offset");
static_assert(offsetof(W8MonsterManagerEntry, pending_event_type) == 0x114,
              "W8MonsterManagerEntry_pending_event_type_offset");
static_assert(sizeof(W8MonsterManagerEntry) == 0x118, "W8MonsterManagerEntry_size_must_be_0x118");

W8MonsterRecord* MonsterDBFromSpecies(unsigned int monster_species);
W8MonsterInfo* CreateMonsterInfo(W8MonsterGroup* group, W8MonsterRecord* record,
                                 srVector3T<float>* position);

/* One queued monster action, 0x30 bytes: the kind/detail pair, the attack
   index the plain attack alone carries, an inline combat slot whose type and
   id words QueueMonsterAction fills by target kind, and a random 1..100
   tie-break so equal decisions do not always resolve the same way. */
struct W8MonsterAction {
    W8MonsterActionKind action_kind;
    int action_detail;
    int attack_index;
    W8CombatSlot target;
    unsigned char tie_break;
};

enum { W8_MONSTER_ATTR_COUNT = 5 };

/* Monster attribute indices into W8MonsterInfo::attributes. ConvertMonsterAttributes
   builds the five slots from character-domain attributes, skipping Piety (2) and
   Vitality (3): slot 0 is Strength, 1 Intelligence, 2 Dexterity, 3 Speed, 4 Senses. */
enum {
    W8_MONSTER_ATTRIBUTE_STRENGTH = 0,
    W8_MONSTER_ATTRIBUTE_INTELLIGENCE = 1,
    W8_MONSTER_ATTRIBUTE_DEXTERITY = 2,
    W8_MONSTER_ATTRIBUTE_SPEED = 3,
    W8_MONSTER_ATTRIBUTE_SENSES = 4
};

/* The 0x153-byte combat allocation has two adjacent runs of 0x11-byte records.
   ClearEffectSlot consumes a record whenever its leading active byte is set. */
#pragma pack(push, 1)

struct W8MonsterCombatState {
    /* The phase of the round this monster next acts on, zero when it
       has finished acting. */
    unsigned int phase;
    bool active;
    /* The round's attack count staged beside attacks_per_round when a
       chosen attack is committed; a four-byte store. */
    unsigned int attacks_per_round0; /* runtime copy of the record field */
    /* How many attacks it gets this round, which is what divides the
       remaining phases between them. */
    int attacks_per_round;
    /* Swings left in the current attack, rolled from the record's
       swings_per_round when the attack starts and read back for the
       announcement message. */
    unsigned int uiSwingsRemaining;
    /* The attack index Monster.cpp launches when combat has already
       selected this monster. It is asserted below MAX_MONSTER_ATTACKS before
       indexing the database record. */
    unsigned int attack_index;
    /* Berserk latch: interrupt case 8 raises it so the monster attacks
       indiscriminately (friends included); Combat Range counts allies as
       hostile while set. */
    bool berserk;
    /* The queue of actions the monster's AI has decided on, one
       W8MonsterAction each. The AI owns the list and destroys it outright. */
    W8PList* plsCombatActionList;
    int character_hate[9];
    W8EffectSlot combat_effects[9];   /* 0x03e .. 0x0d7 */
    W8EffectSlot combat_effects_2[6]; /* 0x0d7 .. 0x13d */
    /* How many times the monster already rolled to notice an attacker
       this round, the same scheme as the character row's spot_attempts. */
    int spot_attempts;
    /* The monster's pending-action repick count, the same scheme as
       the character row's pending_action_repick_count; the attack-score
       surprise penalty scales with it. */
    unsigned int pending_action_repick_count;
    /* The special/breath attack is available this round. */
    bool special_ready;
    /* Rounds until the special attack can fire again, loaded from the
       record's special_attack_cooldown after each use. */
    unsigned char special_cooldown;
    /* The round's interception count, checked against the record's
       attacks_per_round before another intercept is allowed and bumped on
       each successful one. */
    unsigned int interception_count;
    /* The monster is committed to advancing on the party. Set when the
       action executor starts the advance and cleared when an enemy is inside
       short range or when the forcing condition is removed. */
    bool advancing;
    /* Combat ticks since the member last acted; the AI treats a value
       under three as still settling. */
    int settle_ticks;
    /* The monster's turn has been set up already, so the setup runs
       once per turn however often it is asked for. */
    bool turn_started;
    /* Set when a navigator completes movement while this monster is in
       combat; the combat tick then refreshes its sight and clears it. */
    bool sight_refresh_pending;
    /* Per-turn ~75% roll made during turn setup; while set the monster
       skips friendly targets and gets one extra action repick. */
    bool reconsider_action;
};
#pragma pack(pop)

/* The sight state both visibility records carry: unseen, seen this pass, or
   seen within the decay window since last_seen_clock. Stored as a byte. */

/* The party-side sight record for one monster. The live-threat gate,
   the clock and two position triples the player-sight pass stamps, the
   use-bounds flag IsVisibleToPlayer consumes, and its two sight flags. The
   per-turn reset zeroes all 0x30 bytes together, which fixes the extent. */
struct W8PartyThreatRecord {
    int about_location_id; /* zero in the party-side visibility record */
    /* W8SightState - live-threat gate for the group sight query;
       combat, radar, automap and AI read it. */
    W8SightState sight_state;
    /* The sight-flag pair GetPlayerToMonsterSightFlags writes;
       CanPartyMemberAimAtMonster indexes it by the resolved action's
       ranged flag. */
    bool los_flags[2];
    /* The party-detection result after the per-observer threshold and
       camouflage checks run. */
    bool party_detected;
    int last_seen_clock; /* cleared by the per-turn reset */
    srVector3T<float> camera_position;
    srVector3T<float> own_position;
    /* The use-bounds mode the last UpdateMonsterSight pass handed to
       IsVisibleToPlayer. */
    bool use_bounds;
    /* The immediate IsVisibleToPlayer result; gates notices, camera
       and path behavior. */
    bool visible_to_player;
    unsigned char unknown_26[0x0a];
};
static_assert(sizeof(W8PartyThreatRecord) == 0x30, "W8PartyThreatRecord_size");

#pragma pack(push, 1)
/* One 0x31-byte visibility record. W8MonsterInfo embeds the party-facing one
   at 0x348 and the mon-to-mon list allocates one per other monster; both store
   the observer's position at 0x10 and the observed entity's at 0x1c as
   ordinary floats. The reset zeroes exactly its 0x31 bytes. */
struct W8VisibilityRecord {
    int about_location_id;    /* 0x00; always zero in the party record */
    W8SightState sight_state; /* W8SightState */
    /* Two sight-flag pairs - GetMonsterSightFlags writes [0]/[2], and
       the missile/spell vertex traces overwrite [1]/[3]. */
    bool los_flags[4];
    unsigned char unknown_09[2];
    /* The CanMonsterSeeMonster result for mon-to-mon records; the
       party-facing record stores its visible_to_player result here. */
    bool can_see;
    int last_seen_clock;
    srVector3T<float> subject_position; /* the observer */
    srVector3T<float> target_position;  /* the observed */
    bool line_of_sight;
    unsigned char unknown_29[8];
};
static_assert(sizeof(W8VisibilityRecord) == 0x31, "W8VisibilityRecord_size");

struct W8MonsterInfo {
    int location_id;
    int monster_group_id;
    unsigned int monster_species;
    W8Monster* p3D;
    /* Allocated while the monster is in combat. */
    W8MonsterCombatState* pCombat;
    bool fActive;
    bool fInCombat;
    /* Copied from the group's ubDisposition when the entry is created. */
    W8Disposition ubDisposition;
    /* The spawn position; the facing yaw toward the camera follows. */
    srVector3T<float> position;
    float derived; /* camera-facing yaw over position */
    int uiHPMax;
    unsigned int hp_current; /* unsigned conversion at 0053164B */
    int stamina_max;         /* initialized from MONSTERS.DBS dice */
    int stamina;             /* initialized to the same roll */
    /* The position and radius of the last noise this monster heard;
       Noise.cpp writes the heard position and the radius that carried. */
    srVector3T<float> heard_noise_position;
    int heard_noise_radius;
    /* The hit-point regeneration rate and its fractional accumulator. */
    float hp_regen_rate;
    float hp_regen_accumulator;
    float stamina_regen_rate;
    float stamina_regen_accumulator;
    /* The monster's copy of the character condition array, entry for
       entry - condition two doubles its action fatigue at 0x05f, eight blocks
       its spellcasting at 0x077, thirteen makes it hostile at 0x08b, fifteen
       at 0x093 and seventeen is exhaustion at 0x09b. */
    unsigned int uiCondition[W8_CONDITION_COUNT];
    W8Enchantment enchantments[8];
    /* Highest set uiCondition index; 0x12 when deactivated. */
    W8Condition highest_condition;
    /* The argument a condition carries when a monster's conditions are
       copied onto a character. */
    int condition_argument;
    W8EffectSlot effect_slots[12];
    W8GameplayModifierBlock modifiers;
    int fatigue_band; /* derived from stamina */
    /* Countdown set on pathing failure (0x14) or after a long stall
       (0x1e); each AI tick decrements it, and reaching zero clears
       heard_noise_margin. Also gates the face-party proximity check. */
    unsigned char pathing_cooldown;
    unsigned char attributes[W8_MONSTER_ATTR_COUNT]; /* values clamped to 1..125 */
    unsigned char condition_binding_mask;
    bool within_viewing_distance; /* cycle-2 eligibility gate */
    bool fMotionless;
    float scale; /* HP-dependent live Monster scale */
    /* Set once the non-forced death path has run MonsterDies; gates
       the death notice and skips repeat processing. */
    bool death_processed;
    /* Movement-stall tick counter - incremented each watch tick while
       the monster is unlinked, floored at 2 on pathing failure, reset when
       the watch cycle clears. */
    signed char movement_stall_ticks;
    /* Monster AI mode in the low nibble (0..8), bit 0x80 marks a
       pending decision write, bit 0x10 set on load. */
    unsigned char ai_mode;
    unsigned char unknown_256[0x30];
    W8PartyThreatRecord party_threat;
    /* What this monster can see of other monsters, one heap record per
       other monster. The two release paths own it: one drops every record
       about a departing monster, the other empties and destroys the whole
       list. */
    W8PList* plsVisMonToMon;
    W8CombatSlot Target;
    /* Summon marker - 0 ordinary, 1 friendly summon, 2 hostile summon;
       nonzero raises the summoned spell icon and feeds the slain cleanup. */
    W8MonsterSummonKind summoned;
    /* Charm strength; clearing it posts a notice and drops the charmed icon.
       The NPC price check reads it as a percentage discount on the quoted
       price. */
    signed char charm_strength;
    /* The committed attack already launched its missile; asserted by
       ContinueMonsterAttack when an out-of-range attack reports no release. */
    bool fMissileReleased;
    /* The committed spell/special attack already released its payload;
       the action step asserts on it in the spell-wait case. */
    bool fSpellReleased;
    /* The action the monster is taking, -1 through 9. Its whole domain
       is enumerated by MonsterActionFatigueCost, whose error text names it. */
    W8MonsterActionKind action_kind;
    /* Qualifies action kind zero; three costs markedly more. */
    int action_detail;
    unsigned int spell_power_level;
    unsigned char unknown_2ed[4];
    /* This script part's bound NPC slot in g_npc_states, released when
       the entry is destroyed. */
    int bound_npc_index;
    /* Remaining noise-hearing margin, refreshed by nearby sounds and
       cleared when pathing cooldown expires. */
    int heard_noise_margin;
    /* Live spell-point pool; spell-budget calculations add it to the
       database base, and group attacks drain it. */
    unsigned int spell_points;
    W8MonsterControlState control_state; /* Lure success/resistance state */
    bool saved_mirror_x;
    /* The two alternating look-around timers the aging pass
       counts down and rearms from the monster's look frequency/duration. */
    unsigned char look_time;
    unsigned char pause_time;
    /* The condition's own target source, copied in whole by the
       condition setter. */
    W8TargetSource condition_target;
    srVector3T<float> movement_watch_position;
    /* Location id of the elemental Summon Elemental bound to
       this monster, -1 while none is bound; a bound monster cannot be picked
       again. */
    int elemental_summon;
    W8VisibilityRecord player_visibility;
    unsigned char unknown_379;
    bool has_projectile_origin;
    unsigned char unknown_37b;
    bool has_spell_origin;
    unsigned char unknown_37d[0xa8];
};
#pragma pack(pop)

static_assert(sizeof(W8MonsterInfo) == 0x425, "W8MonsterInfo_size_must_be_0x425");
static_assert(offsetof(W8MonsterInfo, modifiers) == 0x1db, "W8MonsterInfo_modifiers_offset");
static_assert(offsetof(W8MonsterInfo, heard_noise_margin) == 0x2f5,
              "W8MonsterInfo_heard_noise_margin_offset");
static_assert(offsetof(W8MonsterInfo, spell_points) == 0x2f9, "W8MonsterInfo_spell_points_offset");

W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int monster_list_index);
bool InitializeMonsterManagerState(void);
void ActivateMonsterInWorld(W8MonsterInfo* monster_info);
void ActivateMonster(W8MonsterInfo* monster_info, W8MonsterActivationMode mode);
void ClearMonsterPathAndResume(W8MonsterInfo* monster_info);
void MonsterStartsDying(W8MonsterInfo* monster_info, bool display_message);
W8MonsterRecord* GetMonsterDataForInfo(W8MonsterInfo* monster_info);
unsigned int MonsterGetIndexByLocationID(int caller_line, const char* caller_file, int location_id,
                                         bool assert_on_failure);
W8MonsterInfo* MonsterInfoFromID(int caller_line, const char* caller_file, int location_id,
                                 bool assert_on_failure);
W8MonsterRecord* GetMonsterDataByLocationID(int location_id);
W8Monster* GetMonsterByLocationID(int location_id);
float GetMonsterCombatMoveRange(W8MonsterInfo* monster_info);
void UpdateMonsterDamageAppearance(W8MonsterInfo* monster_info);
W8MonsterInfo* GetNextMonsterInfo(bool reset_iterator);
int GetMonsterQuadrant(W8MonsterInfo* monster_info);
int GetMonsterCycleFallbackValue(unsigned int monster_species);
void ProcessMonstersAtCombatEnd(bool forced_cleanup);
void ConvertMonsterAttributes(W8MonsterInfo* monster_info);
W8MonsterInfo* FindMonsterInfoBySpecies(unsigned int monster_species);
void ResetLivingMonstersAfterCombat(void);
void DestroyUngroupedMonsters(void);
void SetMonsterControlState(W8MonsterInfo* monster_info, W8MonsterControlState control_state);
void MonsterInfoSetMotionless(W8MonsterInfo* monster_info, bool motionless);
void MoveMonsterToLiveList(W8MonsterInfo* monster_info);
W8MonsterInfo* FindNearestMonsterInfo(const srVector3T<float>* position, double maximum_distance);
void InitializeMonsterRuntimeStats(void);
float CalculateMonsterScale(W8MonsterInfo* monster_info);
void TryStartMonsterCycle2(W8MonsterInfo* monster_info, W8Monster* monster, int query_state);
void ProcessMonsterManagerFrame(void);
void FormatMonsterHealth(W8MonsterInfo* monster_info, wchar_t* health_text);
unsigned int GetMonsterExperience(const W8MonsterRecord* record);
bool AnyMonsterDying(void);
float GetAveragePartyMemberLevel(void);
/* The highest `skills[skill_index].level` among live party members;
   `party_slot` receives the best member's slot. */
unsigned int GetBestPartySkillLevel(W8Skill skill_index, int* party_slot);

void StartMonsterCycle(W8MonsterInfo* monster_info, int cycle, int behavior);
void MonsterInfoLeaveCombat(W8MonsterInfo* monster_info);
unsigned char ShutdownMonsterManager(void);

wchar_t* GetMonsterName(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                        unsigned char name_form);
bool RemoveMonster(unsigned int monster_list_index, bool destroy_monster);
void MonsterInfoEnterCombat(W8MonsterInfo* monster_info);
void DeactivateMonster(W8MonsterInfo* monster_info);
void ToggleCombatMode(void);
void TogglePartyCombatStance(void);
void DetectMonsterGroups(void);
void EvaluateCombatDifficulty(void);
/* The kill bookkeeping a monster's death runs: credit the killer, post the
   "%s %s!" notice, clear conditions the dead monster sourced, apply the
   faction fallout, and bank the kill count and experience when it fought. */
void RecordMonsterKill(W8MonsterInfo* monster_info, bool announce);
/* Record a kill fact for a monster record and the killer party slot. */
void MonsterKilled(int record_id, int killer_party_slot);

#endif
