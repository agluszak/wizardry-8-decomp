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
    bool portrait_pose_animation_active;
    bool portrait_pose_dirty;
    bool portrait_frame_dirty;
    /* 0x09c..0x0ab: the floating damage-number splat animation. The poster at
       0x0059AC40 opens it with the hit's amount, accumulates further hits into
       the amount while it is already up, picks the normal splat catalog object
       (0x90/0x91 = damage_splat_anim.sti / damage_splat_anim2.sti), and switches
       to the death variant (0x92 = death_splat_anim.sti, 0x1e frames instead of
       8) when the character's hit points are gone. The ticker at 0x0059B1A0
       steps the frame once per portrait_fx_clock expiry; the frame is -1 while
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
    /* 0x0ac..0x0bc: the last-drawn portrait vitals so 0x0059A3A0 can raise the
       dirty flag only when the computed bars or the numeric hit-point display
       actually changed; 0x0059A540 redraws them and clears it. */
    int cached_hp_bar;
    int cached_stamina_bar;
    int cached_spell_bar;
    int cached_hp;
    bool portrait_stats_dirty;
    /* 0x0bd..0x0c6: the spell/condition effect-icon flash over the portrait.
       The setup at 0x0059AF40 derives the icon's catalog base from the spell's
       realm and takes the frame count from the same record, shares the
       portrait_fx_clock cadence with the damage splat, and likewise uses -1 as
       the deferred-start frame. */
    bool effect_icon_active;
    int effect_icon_frame;
    int effect_icon_catalog;
    int effect_icon_end_frame;
    /* 0x0ca: the shared 100 ms frame clock both portrait animations tick on;
       the ticker rearms it whenever it expires. */
    int portrait_fx_clock;
    /* 0x0ce: set when the keyboard SELECT_PC command pins the pending portrait
       refresh, so the formation sync at 0x0059B2D0 does not auto-release it. */
    bool portrait_refresh_pinned;
    /* 0x0cf: marks a pending portrait refresh that was auto-issued for an
       in-flight splat/icon animation or portrait event, so the formation sync
       may release it even while the slot is under the cursor. */
    bool auto_portrait_refresh;
    /* 0x0d0: the keyboard menu is open on this slot's portrait; region input is
       disabled and the menu panel redraws with the portrait. */
    bool keyboard_menu_open;
    /* 0x0d1: the combat side-strip portrait needs redrawing; set by hover and
       combat-state changes, cleared by the combat portrait redraw. */
    bool combat_portrait_dirty;
    /* 0x0d2/0x0d6: the acting combatant's portrait pulse - a countdown clock
       rearms the 1..0xc brightness phase in 0x0059B4C0. */
    int acting_portrait_pulse_clock;
    unsigned short acting_portrait_pulse;
    W8GrowableVector<int> highlighted_monsters; /* 0x0d8 */
    /* 0x0e8: the character has reached its experience goal; set once to post
       the level-up notice line and cleared when the character is no longer
       ready to advance. */
    bool level_up_ready;
    /* 0x0e9: edge latch mirroring the character's uiCondition[19]: the
       periodic party sync copies it in and the reaction pass fires the
       condition-change event once while the latch is still clear. */
    unsigned char condition_19_latch;
    /* 0x0ea: per-skill "increased" notice flags posted by PracticeCharacterSkill
       and drained into W8_NPC_MSG_SKILL_NOTICES message lines. */
    unsigned char skill_notice_pending[W8_SKILL_COUNT];
    /* 0x113: set around SwapItemInstances so the autoswap-weapons check does
       not fire on the intermediate item states. */
    bool item_swap_in_progress;
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
static_assert(offsetof(W8MonsterManagerEntry, active_character_event) == 0x71,
              "W8MonsterManagerEntry_active_character_event_offset");
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
static_assert(offsetof(W8MonsterManagerEntry, pending_event_type_114) == 0x114,
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

struct W8MonsterCombatState {
    /* 0x000: the phase of the round this monster next acts on, zero when it
       has finished acting. */
    unsigned int phase;
    bool active; /* 0x004 */
    /* 0x005: the round's attack count staged beside attacks_per_round when a
       chosen attack is committed; a four-byte store. */
    unsigned int attacks_per_round_005; /* runtime copy of the record field */
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
    /* Berserk latch: interrupt case 8 raises it so the monster attacks
       indiscriminately (friends included); Combat Range counts allies as
       hostile while set. */
    unsigned char berserk_015;
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
    /* 0x145: the special/breath attack is available this round. */
    bool special_ready_145;
    /* 0x146: rounds until the special attack can fire again, loaded from the
       record's special_attack_cooldown_15c after each use. */
    unsigned char special_cooldown_146;
    /* 0x147: the round's interception count, checked against the record's
       attacks_per_round before another intercept is allowed and bumped on
       each successful one. */
    unsigned int interception_count;
    /* 0x14b: the monster is committed to advancing on the party. Set when the
       action executor starts the advance and cleared when an enemy is inside
       short range or when the forcing condition is removed. */
    unsigned char advancing_14b;
    /* 0x14c: combat ticks since the member last acted; the AI treats a value
       under three as still settling. */
    int settle_ticks_14c;
    /* 0x150: the monster's turn has been set up already, so the setup runs
       once per turn however often it is asked for. */
    bool turn_started;
    /* 0x151: set when a navigator completes movement while this monster is in
       combat; the combat tick then refreshes its sight and clears it. */
    bool sight_refresh_pending_151;
    /* 0x152: per-turn ~75% roll made during turn setup; while set the monster
       skips friendly targets and gets one extra action repick. */
    bool reconsider_action_152;
}; /* 0x153 */
#pragma pack(pop)

/* The sight state both visibility records carry: unseen, seen this pass, or
   seen within the decay window since last_seen_clock. Stored as a byte. */
enum W8SightState {
    W8_SIGHT_UNSEEN = 0,
    W8_SIGHT_SEEN = 1,
    W8_SIGHT_RECENT = 2,
};

/* 0x286: the party-side sight record for one monster. The live-threat gate,
   the clock and two position triples the player-sight pass stamps, the
   use-bounds flag IsVisibleToPlayer consumes, and its two sight flags. The
   per-turn reset zeroes all 0x30 bytes together, which fixes the extent. */
struct W8PartyThreatRecord {
    unsigned char unknown_00[4];
    /* 0x28a: W8SightState - live-threat gate for the group sight query;
       combat, radar, automap and AI read it. */
    unsigned char sight_state_04;
    /* 0x28b: the sight-flag pair GetPlayerToMonsterSightFlags writes;
       CanPartyMemberAimAtMonster indexes it by the resolved action's
       ranged flag. */
    unsigned char los_flags_05[2];
    /* 0x28d: the party-detection result after the per-observer threshold and
       camouflage checks run. */
    unsigned char party_detected_07;
    int last_seen_clock_08;               /* 0x28e: cleared by the per-turn reset */
    srVector3T<float> camera_position_0c; /* 0x292 */
    srVector3T<float> own_position_18;    /* 0x29e */
    /* 0x2aa: the use-bounds mode the last UpdateMonsterSight pass handed to
       IsVisibleToPlayer. */
    unsigned char use_bounds_24;
    /* 0x2ab: the immediate IsVisibleToPlayer result; gates notices, camera
       and path behavior. */
    bool visible_to_player_25;
    unsigned char unknown_26[0x0a];
}; /* 0x30 */
static_assert(sizeof(W8PartyThreatRecord) == 0x30, "W8PartyThreatRecord_size");

#pragma pack(push, 1)
/* One 0x31-byte visibility record. W8MonsterInfo embeds the party-facing one
   at 0x348 and the mon-to-mon list allocates one per other monster; both store
   the observer's position at 0x10 and the observed entity's at 0x1c as
   ordinary floats. The reset zeroes exactly its 0x31 bytes. */
struct W8VisibilityRecord {
    int about_location_id;        /* 0x00; always zero in the party record */
    unsigned char sight_state_04; /* 0x04: W8SightState */
    /* 0x05: two sight-flag pairs - GetMonsterSightFlags writes [0]/[2], and
       the missile/spell vertex traces overwrite [1]/[3]. */
    unsigned char los_flags_05[4];
    unsigned char unknown_09[2];
    /* 0x0b: the CanMonsterSeeMonster result for mon-to-mon records; the
       party-facing record stores its visible_to_player result here. */
    bool can_see_0b;
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
    W8Monster* p3D;               /* 0x0c: named by the MonsterManager.cpp/mipe assertions */
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
    float derived_23; /* 0x23: camera-facing yaw over position_17 */
    /* 0x27: uiHPMax, named by the Targeting.cpp:0xeac assertion
       "pMonsterInfo->uiHPMax > 0"; signed divisor at 00531657 and 004E5A7A. */
    int uiHPMax;
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
    /* 0x57: uiCondition, named by the ConditionsAndEnchantments assertions
       "pMonsterInfo->uiCondition[uiCondition] > 0". */
    unsigned int uiCondition[W8_CONDITION_COUNT];
    W8Enchantment enchantments[8]; /* 0x0a7 */
    /* 0x107: highest set uiCondition index; 0x12 when deactivated. The
       0x0056C5E0 gate compares it unsigned. */
    unsigned int highest_condition;
    /* 0x10b: the argument a condition carries when a monster's conditions are
       copied onto a character. */
    int condition_argument;
    W8EffectSlot effect_slots_10f[12];
    W8GameplayModifierBlock modifiers_1db; /* 0x1db */
    int fatigue_band;                      /* 0x242: derived from stamina */
    /* 0x246: countdown set on pathing failure (0x14) or after a long stall
       (0x1e); each AI tick decrements it, and reaching zero clears
       sp_budget_bonus. Also gates the face-party proximity check. */
    unsigned char pathing_cooldown_246;
    unsigned char attributes[W8_MONSTER_ATTR_COUNT]; /* 0x247: values clamped to 1..125 */
    unsigned char condition_binding_mask_24c;
    unsigned char within_viewing_distance; /* 0x24d: cycle-2 eligibility gate */
    unsigned char fMotionless;             /* 0x24e: fMotionless in the demo diagnostic */
    float scale_24f;                       /* 0x24f: HP-dependent live Monster scale */
    /* 0x253: set once the non-forced death path has run MonsterDies; gates
       the death notice and skips repeat processing. */
    bool death_processed_253;
    /* 0x254: movement-stall tick counter - incremented each watch tick while
       the monster is unlinked, floored at 2 on pathing failure, reset when
       the watch cycle clears. Compared as signed char at the read sites. */
    unsigned char movement_stall_ticks_254;
    /* 0x255: monster AI mode in the low nibble (0..8), bit 0x80 marks a
       pending decision write, bit 0x10 set on load. */
    unsigned char ai_mode_255;
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
    unsigned char fMissileReleased;
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
    bool has_missile_37a;
    unsigned char unknown_37b;
    bool has_spell_37c;
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
void DetectMonsterGroups004E4AB0(void);      /* 0x004E4AB0 */
void EvaluateCombatDifficulty004E6CE0(void); /* 0x004E6CE0 */
/* The kill bookkeeping a monster's death runs: credit the killer, post the
   "%s %s!" notice, clear conditions the dead monster sourced, apply the
   faction fallout, and bank the kill count and experience when it fought. */
void RecordMonsterKill(W8MonsterInfo* monster_info, char announce); /* 0x004E46F0 */
/* The kill-fact recorder RecordMonsterKill hands the record id and the killer
   party slot to; its home TU is the gap before NPC Manager.cpp. */
void MonsterKilled(int record_id, int killer_party_slot); /* 0x005090C0 */

#endif
