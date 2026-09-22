#include "wiz8/engine_code/Monster.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/engine_code/AnimRep.hpp"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/cursor.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/float_constants.h"
#include "wiz8/xstatus.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/startup_world.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/utility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/sr_api.h"
#include "surrender/srCamera.h"
#include "Types.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/engine_code/Cursor3d.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/geometry.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/regions.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/PolyPick.h"

#define TARGETING_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Targeting.cpp"

/* Point a source at one party character. Everything is cleared first and the
   monster id invalidated, so a source built this way never reads as a monster;
   the character id is the only thing left set. */
// FUNCTION: WIZ8 0x0053be00
void SetTargetSourceToCharacter(int party_slot, W8TargetSource* source)
{
    if (source == 0) {
        srAssertFail("pSource != NULL", TARGETING_CPP, 0xcc9, 0);
    }
    memset(source, 0, sizeof(W8TargetSource));
    source->iMonsterID = BAD_INDEX;
    source->iType = W8_TARGET_SOURCE_CHARACTER;
    source->iChar = party_slot;
}

/* The same for a monster, and the mirror image of it: the character id is the
   one invalidated and the monster id the one left set. The monster is passed
   as its info record rather than as an id, so the id is read out of it here -
   which is what makes the two builders take different kinds of argument for
   the same job. */
// FUNCTION: WIZ8 0x0053be50
void SetTargetSourceToMonster(const W8MonsterInfo* monster_info, W8TargetSource* source)
{
    if (source == 0) {
        srAssertFail("pSource != NULL", TARGETING_CPP, 0xcd5, 0);
    }
    memset(source, 0, sizeof(W8TargetSource));
    source->iMonsterID = BAD_INDEX;
    source->iChar = BAD_INDEX;
    source->iType = W8_TARGET_SOURCE_MONSTER;
    source->iMonsterID = monster_info->location_id;
}

// FUNCTION: WIZ8 0x0053bea0
bool TargetSourceIsCharacter(const W8TargetSource* source, int allow_indirect)
{
    if (source->iType == W8_TARGET_SOURCE_CHARACTER) {
        if (source->iChar == BAD_INDEX) {
            srAssertFail("pSource->iChar != BAD_INDEX", TARGETING_CPP, 0xce3, 0);
        }
        return true;
    }
    if (allow_indirect == 1 && source->iType == W8_TARGET_SOURCE_INDIRECT &&
        source->iChar != BAD_INDEX) {
        if (!source->fBackfire && !source->fReflection) {
            srAssertFail("pSource->fBackfire || pSource->fReflection", TARGETING_CPP, 0xceb, 0);
        }
        return true;
    }
    return false;
}

// FUNCTION: WIZ8 0x0053bf10
bool TargetSourceIsMonster(const W8TargetSource* source, int allow_indirect)
{
    if (source->iType == W8_TARGET_SOURCE_MONSTER) {
        if (source->iMonsterID == BAD_INDEX) {
            srAssertFail("pSource->iMonsterID != BAD_INDEX", TARGETING_CPP, 0xcf8, 0);
        }
        return true;
    }
    if (allow_indirect == 1 && source->iType == W8_TARGET_SOURCE_INDIRECT &&
        source->iMonsterID != BAD_INDEX) {
        if (!source->fBackfire && !source->fReflection) {
            srAssertFail("pSource->fBackfire || pSource->fReflection", TARGETING_CPP, 0xd00, 0);
        }
        return true;
    }
    return false;
}

// FUNCTION: WIZ8 0x0053c320
char GetSourceNoticeColor(const W8TargetSource* source)
{
    if (TargetSourceIsCharacter(source, 1)) {
        return 8;
    }
    if (TargetSourceIsMonster(source, 1)) {
        return 9;
    }
    return 12;
}

// FUNCTION: WIZ8 0x0053c3f0
char GetTargetNoticeColor(const W8TargetSource* source, const W8CombatSlot* target)
{
    if (TargetSourceIsCharacter(source, 1)) {
        return 8;
    }
    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        return g_status_685170.buffers.XChar[target->iChar].party_order_index;
    }
    if (target->iType == W8_TARGET_KIND_MONSTER) {
        return 9;
    }
    return 12;
}

/* Whether a peer aiming at the just-applied target should drop that aim. */
// FUNCTION: WIZ8 0x0053C490
bool ShouldClearAimForAppliedTarget(W8TargetSource* source, W8CombatSlot* target,
                                    W8TargetingContext context,
                                    unsigned char action_targets_enemies)
{
    bool source_hostile;
    bool target_hostile;

    if (context == W8_TARGETING_CONTEXT_OUT_OF_COMBAT) {
        return 1;
    }
    if (source->iType == W8_TARGET_SOURCE_CHARACTER) {
        if (source->iChar == BAD_INDEX) {
            srAssertFail("pSource->iChar != BAD_INDEX", TARGETING_CPP, 0xce3, 0);
        }
        source_hostile = g_status_685170.buffers.Char[source->iChar].uiCondition[13] != 0;
    } else {
        if (source->iType != W8_TARGET_SOURCE_MONSTER) {
            srAssertFail("FALSE", TARGETING_CPP, 0xdf9, 0);
            return 0;
        }
        if (source->iMonsterID == BAD_INDEX) {
            srAssertFail("pSource->iMonsterID != BAD_INDEX", TARGETING_CPP, 0xcf8, 0);
        }
        source_hostile =
            MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0xdf3, TARGETING_CPP, source->iMonsterID, 1))
                ->ubDisposition == DISP_HOSTILE;
    }
    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        target_hostile = g_status_685170.buffers.Char[target->iChar].uiCondition[13] != 0;
    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
        target_hostile =
            MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0xe06, TARGETING_CPP, target->iMonsterID, 1))
                ->ubDisposition == DISP_HOSTILE;
    } else if (target->iType == W8_TARGET_KIND_GROUP) {
        target_hostile = GetMonsterGroupByListIndex(
                             GetMonsterGroupIndexByID(0xe0b, TARGETING_CPP, target->iGroupID, 1))
                             ->ubDisposition == DISP_HOSTILE;
    } else {
        srAssertFail("FALSE", TARGETING_CPP, 0xe11, 0);
        return 0;
    }
    if (source_hostile == target_hostile) {
        return action_targets_enemies != 0;
    }
    return action_targets_enemies == 0;
}

/* The faction names, thirty bytes apart, in the same order as the faction ids.
   Twenty-one of them, which is the whole faction domain. */
// GLOBAL: WIZ8 0x0061CE74
extern const char g_faction_names[W8_FACTION_COUNT][0x1e] = {
    "UNALIGNED",
    "FACTION_PARTY",
    "FACTION_DARK_SAVANT",
    "FACTION_COSMIC_LORDS",
    "FACTION_UMPANI",
    "FACTION_TRANG",
    "FACTION_MOOK",
    "FACTION_RATTKIN_COMMON",
    "FACTION_RATTKIN_MAFIA",
    "FACTION_BROTHERHOOD",
    "FACTION_HIGARDI_BANK",
    "FACTION_HIGARDI_HLL",
    "FACTION_HIGARDI_COMMON",
    "FACTION_TRYNNIE",
    "FACTION_MAD_MARTEN",
    "FACTION_RAPAX_COMMON",
    "FACTION_RAPAX_TEMPLAR",
    "FACTION_RAPAX_ARMY",
    "FACTION_KINGS_ASSASINS",
    "FACTION_filler3",
    "FACTION_filler4",
};

/* Look a faction up by name, case-insensitively. -1 for a name that is not one
   of the twenty-one. */
// FUNCTION: WIZ8 0x005360b0
char FindFactionByName(const char* name)
{
    char faction;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wchar-subscripts"
    /* Faction ids are a recovered char, including the -1 miss sentinel. */
    for (faction = 0; faction < W8_FACTION_COUNT; ++faction) {
        if (_stricmp(g_faction_names[faction], name) == 0) {
            return faction;
        }
    }
    return -1;
}

/* One faction's runtime value. */
// FUNCTION: WIZ8 0x005360f0
int GetFactionValue(char faction)
{
    return g_factions[faction].band_changed_clock_06;
}

/* Raise or lower one faction's flag. */
// FUNCTION: WIZ8 0x00536110
void SetFactionFlag(char faction, unsigned char flag)
{
    g_factions[faction].flag_0a = flag;
}

// FUNCTION: WIZ8 0x00536130
unsigned char GetFactionFlag(char faction)
{
    return g_factions[faction].flag_0a;
}
#pragma clang diagnostic pop

/* Build an empty target block: everything zeroed, then the two ids set to
   BAD_INDEX. The kind is written twice, once by the clear and once on its
   own. */
// FUNCTION: WIZ8 0x00536150
void ResetTargetSource(W8TargetSource* source)
{
    memset(source, 0, sizeof(W8TargetSource));
    source->iType = W8_TARGET_SOURCE_NONE;
    source->iChar = BAD_INDEX;
    source->iMonsterID = BAD_INDEX;
}

/* The same for the shorter inline form, which has a third id to invalidate. */
// FUNCTION: WIZ8 0x00536170
void ResetCombatSlot(W8CombatSlot* slot)
{
    memset(slot, 0, sizeof(W8CombatSlot));
    slot->iType = W8_TARGET_KIND_NONE;
    slot->iMonsterID = BAD_INDEX;
    slot->iChar = BAD_INDEX;
    slot->iGroupID = BAD_INDEX;
}

/* Clear whatever a monster was aiming at, and report that it now has no
   target. */
// FUNCTION: WIZ8 0x005369f0
bool ClearMonsterCombatSlot(W8MonsterInfo* monster_info)
{
    ResetCombatSlot(&monster_info->Target);
    return false;
}

/* Whether one party slot's target is still good in a given context: it has to
   be of the kind the spell needs and within range. */
// FUNCTION: WIZ8 0x00537220
bool IsSpellTargetStillValidIn(int party_slot, int spell_id, W8TargetingContext context)
{
    char needed = GetTargetNeededForSpellFriendly(spell_id, 0, context);

    if (!TargetMatchesNeeded(GetTargetBlockForContext(party_slot, context), needed)) {
        return false;
    }
    return IsCurrentTargetInRange(party_slot, 0, context) != 0;
}

/* The same check without the range half, and with one target kind that a
   global override always accepts. */
// FUNCTION: WIZ8 0x00537270
bool IsSpellTargetOfNeededKind(int party_slot, int spell_id)
{
    W8CombatSlot* target = GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT);
    int needed = GetTargetNeededForSpellFriendly(spell_id, 0, W8_TARGETING_CONTEXT_CURRENT);

    if (needed == 2 && g_settings_6850c8.autoscroll_combat_messages != 0) {
        return 1;
    }
    return TargetMatchesNeeded(target, (char)needed);
}

/* What an item's spell needs picked before it can be cast. An item with no
   spell needs nothing. */
// FUNCTION: WIZ8 0x00537330
int GetTargetNeededForItem(const W8ItemInstance* item)
{
    const W8ItemDatabaseRecord* record;

    if (item == 0 || item->iItemNo == -1) {
        return 0;
    }
    record = &g_item_records[item->iItemNo];
    if (record->spell_id == 0) {
        return 0;
    }
    return GetTargetNeededForSpellFriendly(record->spell_id, ItemClassNormalizesTarget(record),
                                           W8_TARGETING_CONTEXT_CURRENT);
}

/* Aim one party slot at a clicked monster location. The pending action's
   needed target kind decides between the single monster and its whole group:
   a group-needing action (needed 5) aims at the location's group instead. */
// FUNCTION: WIZ8 0x00537950
void AimAtMonsterLocation00537950(int party_slot, int location_id, int allow_single_target)
{
    W8CombatSlot target;
    W8ActionDetailBlock* detail_block;
    int action;
    int detail;
    int needed;

    if (CanTargetMonster(party_slot, location_id, allow_single_target, 1)) {
        needed = -1;
        if (ResolveTargetingContext(party_slot, GetCurrentTargetingContext(party_slot)) != 0) {
            ChooseCombatAction(party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0,
                               &detail_block);
            needed = GetTargetNeededForAction(action, detail, detail_block);
            if (needed == 5) {
                memset(&target, 0, sizeof(target));
                target.iMonsterID = BAD_INDEX;
                target.iChar = BAD_INDEX;
                target.iType = W8_TARGET_KIND_GROUP;
                target.iGroupID =
                    MonsterGetScriptPartByLocationIndex(
                        MonsterGetIndexByLocationID(0x390, TARGETING_CPP, location_id, 1))
                        ->monster_group_id;
                AimAtTarget(party_slot, &target, W8_TARGETING_CONTEXT_CURRENT);
                StartBreathCycle(party_slot, 0);
                return;
            }
        }
        memset(&target, 0, sizeof(target));
        target.iChar = BAD_INDEX;
        target.iGroupID = BAD_INDEX;
        target.iType = W8_TARGET_KIND_MONSTER;
        target.iMonsterID = location_id;
        AimAtTarget(party_slot, &target, W8_TARGETING_CONTEXT_CURRENT);
        StartBreathCycle(party_slot, 0);
    }
}

/* Aim at whatever the caller names, by kind. The other three fields are left
   at BAD_INDEX, so only the kind's own field is meaningful. */
// FUNCTION: WIZ8 0x00538620
void AimByKind(int actor, W8TargetKind kind, W8TargetingContext context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.iMonsterID = BAD_INDEX;
    target.iChar = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = kind;
    AimAtTarget(actor, &target, context);
}

/* Aim at one character. */
// FUNCTION: WIZ8 0x00538670
void AimAtCharacter(int actor, int character_slot, W8TargetingContext context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.iMonsterID = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iChar = character_slot;
    target.iType = W8_TARGET_KIND_CHARACTER;
    AimAtTarget(actor, &target, context);
}

/* Aim at a character through the indirect kind (type 9). */
// FUNCTION: WIZ8 0x005386C0
void AimAtCharacterIndirect(int actor, int character_slot, W8TargetingContext context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.iMonsterID = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iChar = character_slot;
    target.iType = W8_TARGET_KIND_CHARACTER_INDIRECT;
    AimAtTarget(actor, &target, context);
}

/* Aim at the place the party is looking, drop the marker and let the display
   know. */
// FUNCTION: WIZ8 0x00538710
void AimAtPlace(int actor)
{
    W8CombatSlot target;
    srVector3T<float> position;

    memset(&target, 0, sizeof(target));
    target.iMonsterID = BAD_INDEX;
    target.iChar = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = W8_TARGET_KIND_PLACE;
    GetWorldCursorTargetPosition00492500(&position);
    AimAtTarget(actor, &target, W8_TARGETING_CONTEXT_CURRENT);
    gXStatus.target_markers.Clear();
    RequestRefreshPartyState();
}

/* Aim at the ground point the camera is looking at, the world-cursor-free
   twin of AimAtPlace used by click-to-move targeting. */
// FUNCTION: WIZ8 0x00538770
void AimAtGroundTarget00538770(int party_slot)
{
    W8CombatSlot target;
    srVector3T<float> position;

    memset(&target, 0, sizeof(target));
    target.iMonsterID = BAD_INDEX;
    target.iChar = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = W8_TARGET_KIND_PLACE;
    GetCameraForwardPoint00421150(GetRangeConstant5EC35C(), &position);
    target.point = position;
    AimAtTarget(party_slot, &target, W8_TARGETING_CONTEXT_CURRENT);
    StartBreathCycle(party_slot, 0);
}

/* The three wrappers that set the party's own target rather than a
   combatant's, one per kind that names something. */
// FUNCTION: WIZ8 0x00538d10
void SetTargetToCharacter(int character_slot, W8TargetingContext context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.iMonsterID = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = W8_TARGET_KIND_CHARACTER;
    target.iChar = character_slot;
    ApplyTarget(&target, context);
}

// FUNCTION: WIZ8 0x00538d60
void SetTargetToMonster(int monster_id, W8TargetingContext context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.iChar = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = W8_TARGET_KIND_MONSTER;
    target.iMonsterID = monster_id;
    ApplyTarget(&target, context);
}

// FUNCTION: WIZ8 0x00538db0
void SetTargetToGroup(int group_id, W8TargetingContext context)
{
    W8CombatSlot target;

    memset(&target, 0, sizeof(target));
    target.iMonsterID = BAD_INDEX;
    target.iChar = BAD_INDEX;
    target.iType = W8_TARGET_KIND_GROUP;
    target.iGroupID = group_id;
    ApplyTarget(&target, context);
}

/* Walk every party slot and every live monster: anyone already aiming at
   `target` drops that aim when the applied context and their action say so. A
   monster target also clears its highlight bit before the walk. */
// FUNCTION: WIZ8 0x00538E00
void ApplyTarget(W8CombatSlot* target, W8TargetingContext context)
{
    W8TargetSource source;
    W8PartySlotRow* row;
    W8Character* character;
    W8MonsterInfo* monster_info;
    unsigned char action_targets_enemies;
    int party_slot;

    if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x638, TARGETING_CPP, target->iMonsterID, 1));
        MonsterSetRuntimeFlag5BC(monster_info->p3D, 0);
    }

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        row = &g_status_685170.buffers.XChar[party_slot];
        character = &g_status_685170.buffers.Char[party_slot];
        if (row->fOccupied == 0 || character->hp_current == 0 ||
            character->highest_condition >= 0x12) {
            continue;
        }
        if (target->iType == W8_TARGET_KIND_MONSTER) {
            NotifyMonsterHighlight(party_slot, target->iMonsterID, 0);
        }
        SetTargetSourceToCharacter(party_slot, &source);
        if (gXStatus.fCombatMode != 0) {
            if (memcmp(&row->target_in_combat, target, sizeof(W8CombatSlot)) == 0) {
                action_targets_enemies = CharacterActionTargetsEnemies(
                    character, row->action_03d, row->action_detail_041, &row->action_detail_045);
                if (ShouldClearAimForAppliedTarget(&source, target, context,
                                                   action_targets_enemies) != 0) {
                    if (row->action_03d == W8_ACTION_PROTECT) {
                        DropCharacterFromRound(party_slot);
                    } else {
                        RepickActionTarget(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0);
                    }
                }
            }
        }
        if (memcmp(&row->target_out_of_combat, target, sizeof(W8CombatSlot)) == 0) {
            action_targets_enemies =
                CharacterActionTargetsEnemies(character, row->pending_action, row->attack_mode[0],
                                              &row->pending_action_detail_015);
            if (ShouldClearAimForAppliedTarget(&source, target, context, action_targets_enemies) !=
                0) {
                RepickActionTarget(party_slot, W8_TARGETING_CONTEXT_OUT_OF_COMBAT, 0);
            }
        }
    }

    for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        SetTargetSourceToMonster(monster_info, &source);
        if (memcmp(&monster_info->Target, target, sizeof(W8CombatSlot)) == 0) {
            action_targets_enemies =
                MonsterActionTargetsEnemies(monster_info->action_kind, monster_info->action_detail,
                                            &monster_info->spell_power_level);
            if (ShouldClearAimForAppliedTarget(&source, target, context, action_targets_enemies) !=
                0) {
                ResetCombatSlot(&monster_info->Target);
            }
        }
    }
}

/* Put the on-screen marker over one monster, from the party's eye to the
   monster's own bounds. */
// FUNCTION: WIZ8 0x00539870
bool ShowMonsterTargetMarker(W8MonsterInfo* monster_info)
{
    srVector3T<float> eye;
    srVector3T<float> lower;
    srVector3T<float> upper;

    if (monster_info == 0) {
        srAssertFail("pMonsterInfo", TARGETING_CPP, 2040, 0);
    }
    GetCameraPosition(&eye);
    MonsterGetWorldAnimationBounds004CA4F0(monster_info->p3D, &lower, &upper);
    return ShowTargetMarker(&eye, &lower, &upper);
}

/* Whether a recorded target is of the kind a caller needs. Each needed kind
   admits one or two target kinds, and every admitted target additionally has
   to be reachable - so the answer is never just the kind test. */
// FUNCTION: WIZ8 0x00537160
char TargetMatchesNeeded(W8CombatSlot* target, int needed)
{
    char matched = 0;

    if (target == 0) {
        return 0;
    }
    switch (needed) {
    case 1:
    case 2:
    case 8:
        if (target->iType == W8_TARGET_KIND_CHARACTER && target->iChar != BAD_INDEX) {
            matched = 1;
        }
        if (target->iType == W8_TARGET_KIND_MONSTER && target->iMonsterID != BAD_INDEX) {
            return IsTargetStillPresent(target);
        }
        if (matched) {
            return IsTargetStillPresent(target);
        }
        break;
    case 3:
    case 4:
        if (target->iType == W8_TARGET_KIND_PLACE) {
            return IsTargetStillPresent(target);
        }
        break;
    case 5:
        if (target->iType == W8_TARGET_KIND_GROUP && target->iGroupID != BAD_INDEX) {
            matched = 1;
        }
        if (target->iType == W8_TARGET_KIND_PARTY) {
            return IsTargetStillPresent(target);
        }
        if (matched) {
            return IsTargetStillPresent(target);
        }
        break;
    case 6:
        if (target->iType == W8_TARGET_KIND_ITEM && target->pPCItem != 0) {
            return IsTargetStillPresent(target);
        }
        break;
    case 7:
        if (target->iType == W8_TARGET_KIND_CHARACTER_INDIRECT && target->iChar != BAD_INDEX) {
            return IsTargetStillPresent(target);
        }
        break;
    }
    return 0;
}

/* What the interface has to ask the player to pick for one action. Most
   actions answer a fixed kind; casting asks the spell and using an item asks
   the item's own spell, which is the same two-step the item path takes. */
// FUNCTION: WIZ8 0x00536400
void RepickInvalidCombatTargets00536400(void)
{
    for (int party_slot = 0; party_slot < 8; ++party_slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
        W8Character* character = &g_status_685170.buffers.Char[party_slot];
        if (row->fOccupied == 0 || character->hp_current == 0 ||
            character->highest_condition >= 0xd ||
            !CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 1, 0)) {
            continue;
        }
        int action;
        int detail;
        W8CombatSlot* target;
        W8ActionDetailBlock* detail_block;
        ChooseCombatAction(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, &action, &detail, &target,
                           &detail_block);
        int needed = GetTargetNeededForAction(action, detail, detail_block);
        if (TargetMatchesNeeded(target, needed) == 0 ||
            !CharacterActionReachesTarget(party_slot, 2, W8_TARGETING_CONTEXT_IN_COMBAT)) {
            RepickActionTarget(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0);
        }
    }
}

// FUNCTION: WIZ8 0x00536a20
int GetTargetNeededForAction(int action, int spell_id, const W8ActionDetailBlock* detail_block)
{
    const W8ItemDatabaseRecord* record;

    switch (action) {
    case 0:
    case 1:
        return 2;
    case 2:
        return 4;
    case 5:
        return 1;
    case 7:
        return GetTargetNeededForSpellFriendly(spell_id, 0, W8_TARGETING_CONTEXT_CURRENT);
    case 8:
        if (detail_block->item_use.item != 0 && detail_block->item_use.item->iItemNo != -1) {
            record = &g_item_records[detail_block->item_use.item->iItemNo];
            if (record->spell_id != 0) {
                return GetTargetNeededForSpellFriendly(record->spell_id,
                                                       ItemClassNormalizesTarget(record),
                                                       W8_TARGETING_CONTEXT_CURRENT);
            }
        }
        break;
    }
    return 0;
}

/* The target kind the slot's current action needs right now. The ordinary
   combat-or-not context is overridden while the main game screen has a
   pending selection - a settled spell or item pick asks through that context,
   anything else pending reads as dialogue - and while the slot is the
   selected character in spell or item mode the shared context applies. */
// FUNCTION: WIZ8 0x00537380
int GetTargetNeededForCurrentAction(int party_slot)
{
    W8ActionDetailBlock* detail_block;
    const W8ItemDatabaseRecord* record;
    W8TargetingContext context;
    int action;
    int detail;

    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0 &&
        g_level_block->selection_kind != -1) {
        if (g_level_block->selection_kind == 7 && g_level_block->selection_settled != 0) {
            context = W8_TARGETING_CONTEXT_SPELL;
        } else if (g_level_block->selection_kind == 8 && g_level_block->selection_settled != 0) {
            context = W8_TARGETING_CONTEXT_ITEM;
        } else {
            context = W8_TARGETING_CONTEXT_DIALOGUE;
        }
    } else if (party_slot == g_status_685170.selected_character &&
               (gXStatus.fSpellCastMode != 0 || gXStatus.fItemSelectMode != 0)) {
        context = W8_TARGETING_CONTEXT_SHARED;
    } else {
        context = gXStatus.fCombatMode != 0 ? W8_TARGETING_CONTEXT_IN_COMBAT
                                            : W8_TARGETING_CONTEXT_OUT_OF_COMBAT;
    }
    if (context == W8_TARGETING_CONTEXT_CURRENT) {
        context = GetCurrentTargetingContext(party_slot);
    }
    switch (context) {
    case W8_TARGETING_CONTEXT_OUT_OF_COMBAT:
        return 0;
    case W8_TARGETING_CONTEXT_IN_COMBAT:
    case W8_TARGETING_CONTEXT_SHARED:
    case W8_TARGETING_CONTEXT_SPELL:
    case W8_TARGETING_CONTEXT_ITEM:
    case W8_TARGETING_CONTEXT_FIVE:
    case W8_TARGETING_CONTEXT_DIALOGUE:
        break;
    default:
        srAssertFail("FALSE", TARGETING_CPP, 0xc5b, 0);
    }
    ChooseCombatAction(party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0,
                       &detail_block);
    switch (action) {
    case 0:
    case 1:
        return 2;
    case 2:
        return 4;
    case 5:
        return 1;
    case 7:
        return GetTargetNeededForSpellFriendly(detail, 0, W8_TARGETING_CONTEXT_CURRENT);
    case 8:
        if (detail_block->item_use.item != 0 && detail_block->item_use.item->iItemNo != -1) {
            record = &g_item_records[detail_block->item_use.item->iItemNo];
            if (record->spell_id != 0) {
                return GetTargetNeededForSpellFriendly(record->spell_id,
                                                       ItemClassNormalizesTarget(record),
                                                       W8_TARGETING_CONTEXT_CURRENT);
            }
        }
        break;
    }
    return 0;
}

/* Whether a slot's recorded target suits the item it would be used with. An
   item with no spell needs nothing picked, and one target kind a global
   override always accepts. */
// FUNCTION: WIZ8 0x005372b0
bool IsItemTargetOfNeededKind(int party_slot, const W8ItemInstance* item)
{
    W8CombatSlot* target = GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT);
    const W8ItemDatabaseRecord* record;
    int needed = 0;

    if (item != 0 && item->iItemNo != -1) {
        record = &g_item_records[item->iItemNo];
        if (record->spell_id != 0) {
            needed =
                GetTargetNeededForSpellFriendly(record->spell_id, ItemClassNormalizesTarget(record),
                                                W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
            if (needed == 2 && g_settings_6850c8.autoscroll_combat_messages != 0) {
                return 1;
            }
        }
    }
    return TargetMatchesNeeded(target, (char)needed);
}

/* Tint one monster for whoever is highlighting it. Three tints are named -
   nothing, and two that differ only in which channel is lit - and everything
   else leaves the colour as it was. */
// FUNCTION: WIZ8 0x00539480
void TintHighlightedMonster(W8Monster* monster, int tint)
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;

    if (tint == 1) {
        g = 1.0f;
        a = 1.0f;
    } else if (tint == 2) {
        r = 1.0f;
        a = 1.0f;
    }
    SetMonsterHighlightColour(monster, r, g, b, a);
}

/* Raise or lower one character's bit in a monster's highlight mask, and tell
   whatever draws it. */
// FUNCTION: WIZ8 0x00539630
void SetMonsterHighlight(int party_slot, int location_id, char on)
{
    int index = MonsterGetIndexByLocationID(1879, TARGETING_CPP, location_id, 0);
    W8MonsterInfo* monster_info;
    W8Monster* monster;
    unsigned char bit;

    if (index == -1) {
        return;
    }
    monster_info = MonsterGetScriptPartByLocationIndex(index);
    monster = monster_info->p3D;
    if (monster == 0) {
        srAssertFail("pMonster", TARGETING_CPP, 1888, 0);
    }

    bit = (unsigned char)(1 << (party_slot & 0x1f));
    if (on) {
        MonsterSetRuntimeFlag5BC(monster, MonsterGetRuntimeFlag5BC(monster) | bit);
        NotifyMonsterHighlight(party_slot, location_id, 1);
        return;
    }
    MonsterSetRuntimeFlag5BC(monster, MonsterGetRuntimeFlag5BC(monster) & ~bit);
    NotifyMonsterHighlight(party_slot, location_id, 0);
}

/* The location id of the nearest live monster whose current model instance is
   under the cursor, or -1. Born monsters come first; the unborn list joins the
   scan only while g_flag_689b32 is set. The cursor coordinates are carried but
   unused - the hover test is MonsterUsesCurrentModelInstance. */
// FUNCTION: WIZ8 0x005396d0
int PickNearestMonsterUnderCursor005396D0(int cursor_x, int cursor_y)
{
    int result;
    float best_distance;
    unsigned int index;

    result = -1;
    best_distance = 999999.0f;
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        float distance;

        if (monster_info->fActive == 0) {
            continue;
        }
        if (monster_info->p3D->IsDying() != 0) {
            continue;
        }
        if (monster_info->p3D->copied_flag_332 != 0) {
            continue;
        }
        if (!MonsterUsesCurrentModelInstance(monster_info->p3D)) {
            continue;
        }
        if (monster_info->party_threat.use_bounds_24 == 0) {
            UpdateMonsterSight(monster_info, 1, 1);
        }
        if (monster_info->p3D->IsRenderable004C7C00(1) == 0) {
            continue;
        }
        distance = MonsterDistanceToCamera004BE710(GetWorld(), monster_info->p3D);
        if (distance < best_distance) {
            result = monster_info->location_id;
            best_distance = distance;
        }
    }
    if (g_flag_689b32 != 0) {
        for (index = 0; index < PLLength(gXStatus.plsUnbornMonsterList); ++index) {
            W8MonsterInfo* monster_info =
                static_cast<W8MonsterInfo*>(PLGet(gXStatus.plsUnbornMonsterList, index));
            float distance;

            if (monster_info->fActive == 0) {
                continue;
            }
            if (monster_info->p3D->IsDying() != 0) {
                continue;
            }
            if (monster_info->p3D->copied_flag_332 != 0) {
                continue;
            }
            if (!MonsterUsesCurrentModelInstance(monster_info->p3D)) {
                continue;
            }
            if (monster_info->party_threat.use_bounds_24 == 0) {
                UpdateMonsterSight(monster_info, 1, 1);
            }
            if (monster_info->p3D->IsRenderable004C7C00(1) == 0) {
                continue;
            }
            distance = MonsterDistanceToCamera004BE710(GetWorld(), monster_info->p3D);
            if (distance < best_distance) {
                result = monster_info->location_id;
                best_distance = distance;
            }
        }
    }
    return result;
}

/* The same over a whole group, one member at a time. The count is re-read each
   step because highlighting can remove a member. */
// FUNCTION: WIZ8 0x00538c00
void SetGroupHighlight(int party_slot, int group_id, char on)
{
    unsigned int group_index = GetMonsterGroupIndexByID(1498, TARGETING_CPP, group_id, 0);
    W8MonsterGroup* group;
    unsigned int member;
    int location_id;
    int index;
    W8MonsterInfo* monster_info;
    W8Monster* monster;
    unsigned char bit;

    if (group_index == 0xffffffff) {
        return;
    }
    group = GetMonsterGroupByListIndex(group_index);
    for (member = 0; member < ILLength(group->monsters); ++member) {
        location_id = IListGetAt(group->monsters, member);
        index = MonsterGetIndexByLocationID(1879, TARGETING_CPP, location_id, 0);
        if (index == -1) {
            continue;
        }
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        monster = monster_info->p3D;
        if (monster == 0) {
            srAssertFail("pMonster", TARGETING_CPP, 1888, 0);
        }
        bit = (unsigned char)(1 << (party_slot & 0x1f));
        if (on == 0) {
            MonsterSetRuntimeFlag5BC(monster, MonsterGetRuntimeFlag5BC(monster) & ~bit);
        } else {
            MonsterSetRuntimeFlag5BC(monster, MonsterGetRuntimeFlag5BC(monster) | bit);
        }
        NotifyMonsterHighlight(party_slot, location_id, on != 0);
    }
}

/* Re-tint every monster for one character. A character whose highlight is
   overridden tints for whoever overrode it instead, and then only the monsters
   that character was already highlighting. */
// FUNCTION: WIZ8 0x00539570
void UpdateAllMonsterHighlights(int party_slot, int location_id)
{
    bool overridden = false;
    int owner = party_slot;
    unsigned int index;
    W8MonsterInfo* monster_info;
    int tint;

    if (g_level_block->highlight_override != -1 && gXStatus.iTargetingMode == 0) {
        overridden = true;
        owner = g_level_block->highlight_override;
    }

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fActive == 0) {
            continue;
        }
        if (location_id == monster_info->location_id) {
            tint = 1;
        } else if (overridden &&
                   ((1 << (owner & 0x1f)) & MonsterGetRuntimeFlag5BC(monster_info->p3D)) != 0) {
            tint = 1;
        } else {
            tint = 0;
        }
        HighlightMonsterAsTarget(monster_info->location_id, owner, (char)tint);
    }
}

/* One monster considered as an auto-attack target, and the four things the
   ordering below reads out of it. The rest of the record is filled in as the
   candidate is built and is what makes the sort stable across the fields it
   does not compare. */
struct W8MonsterTargetCandidate {
    int location_id;        /* 0x00 */
    int state_04;           /* 0x04: the monster's own 0x107 */
    unsigned char in_reach; /* 0x08: reachable with a real attack */
    unsigned char pad_09[3];
    unsigned int range_band;  /* 0x0c: the first band that covers it */
    unsigned int hp_current;  /* 0x10 */
    unsigned char same_group; /* 0x14: shares the caller's group */
    unsigned char pad_15[3];
    float distance; /* 0x18 */
}; /* 0x1c */

/* The order the candidates are taken in: the monster in the lowest state
   first, then the one that can actually be reached, then the nearest. Only
   three of the record's seven fields are compared, so the rest are carried for
   the caller rather than for the sort. */
// FUNCTION: WIZ8 0x0053c920
int CompareMonsterTargetCandidates(const void* left, const void* right)
{
    const W8MonsterTargetCandidate* a = (const W8MonsterTargetCandidate*)left;
    const W8MonsterTargetCandidate* b = (const W8MonsterTargetCandidate*)right;

    if ((unsigned int)a->state_04 < (unsigned int)b->state_04) {
        return -1;
    }
    if ((unsigned int)a->state_04 > (unsigned int)b->state_04) {
        return 1;
    }
    if (a->in_reach != 0 && b->in_reach == 0) {
        return -1;
    }
    if (a->in_reach == 0 && b->in_reach != 0) {
        return 1;
    }
    if (a->distance < b->distance) {
        return -1;
    }
    if (a->distance > b->distance) {
        return 1;
    }
    return 0;
}

/* Resolve where a target physically is into the slot's own point: the camera
   for the character and party kinds, the monster's model position plus its
   height for the monster kind. The sight-probe variant takes the navigator
   position lifted by the sight offset instead. Answers zero for any kind
   without a place. */
// FUNCTION: WIZ8 0x0053c630
bool ResolveTargetPoint(W8CombatSlot* target, char sight_probe)
{
    srVector3T<float> point;
    W8Monster* monster;

    if (target->iType == W8_TARGET_KIND_PARTY || target->iType == W8_TARGET_KIND_CHARACTER) {
        if (sight_probe) {
            point = g_startup_world_659c0c->GetPosition();
        } else {
            GetCameraPosition(&point);
        }
    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster = GetMonsterByLocationID(target->iMonsterID);
        if (sight_probe) {
            point = monster->GetPosition();
        } else {
            point = monster->movement_0c0.position_040;
            point.y += monster->movement_0c0.height_offset_0b8;
        }
    } else {
        return 0;
    }
    if (sight_probe) {
        point.y += g_float_005ebc64;
    }
    target->point = point;
    return 1;
}

/* Which monster a party slot should turn on when it has to pick one for
   itself. Every live, in-combat, still-standing monster the slot is hostile to
   and can reach becomes a candidate; the candidates are then ordered and the
   first one taken.

   The whole array is built before any of it is compared, which is why the
   record carries fields the ordering never reads - they are what the caller
   would need if it took more than the first. A monster with no hit points at
   all is a data error rather than a candidate to skip. */
// FUNCTION: WIZ8 0x0053c720
int ChooseMonsterTarget(int party_slot, int group_id, W8TargetingContext context)
{
    unsigned int monster_count = PLLength(gXStatus.plsMonsterList);
    W8MonsterTargetCandidate* candidates;
    W8MonsterTargetCandidate* next;
    size_t found = 0;
    unsigned int index;
    int chosen;

    if (monster_count == 0) {
        return BAD_INDEX;
    }
    candidates =
        (W8MonsterTargetCandidate*)malloc(monster_count * sizeof(W8MonsterTargetCandidate));
    if (candidates == 0) {
        return BAD_INDEX;
    }

    next = candidates;
    for (index = 0; index < monster_count; ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        W8MonsterRecord* record;
        unsigned int band;

        if (monster_info->fActive == 0 || monster_info->fInCombat == 0 ||
            monster_info->hp_current == 0 || MonsterIsHostileTo(party_slot, monster_info) != 1 ||
            !CanPartyMemberAimAtMonster(party_slot, 2, monster_info, context, 0)) {
            continue;
        }

        next->location_id = monster_info->location_id;
        next->state_04 = monster_info->highest_condition;
        next->in_reach = 0;

        record = GetMonsterDataForInfo(monster_info);
        if (GetBestMonsterAttackRange(record, 1) != -1) {
            unsigned int quadrant = GetMonsterQuadrant(monster_info) & 0xff;

            if ((unsigned char)quadrant == 2 ||
                (!AnyoneStandsAhead((unsigned char)quadrant) && AnyoneStandsAhead(4))) {
                next->in_reach = 1;
            }
        }

        /* The first range band whose reach covers where the monster is. */
        for (band = 0; band < 4; ++band) {
            if (monster_info->p3D->GetDistanceToPlayer004C7CB0() <=
                CalcRangeDistance(static_cast<W8RangeCategory>(band))) {
                next->range_band = band;
                break;
            }
        }

        if (monster_info->uiHPMax == 0) {
            srAssertFail("pMonsterInfo->uiHPMax > 0", TARGETING_CPP, 0xeac, 0);
        }
        next->hp_current = monster_info->hp_current;
        next->same_group = (unsigned char)(monster_info->monster_group_id == group_id);
        next->distance = monster_info->p3D->GetDistanceToPlayer004C7CB0();

        ++found;
        ++next;
    }

    if (found == 0) {
        free(candidates);
        return BAD_INDEX;
    }
    qsort(candidates, found, sizeof(W8MonsterTargetCandidate), CompareMonsterTargetCandidates);
    chosen = candidates[0].location_id;
    free(candidates);
    return chosen;
}

/* The two conditions the indirect character kind reads: eighteen has to be
   running and nineteen not. The monster kind reads the same eighteenth entry of
   its own array, which is what pairs the two arrays entry for entry. */
enum { W8_CONDITION_REACHABLE_WHEN_DOWN = 18, W8_CONDITION_BEYOND_REACH = 19 };

/* Whether whatever a target names is still there to be acted on. Each kind
   checks its own field and then whatever that field points at, which is what
   makes the four assertions here - on iChar, iMonsterID, iGroupID and pPCItem -
   name four different fields of one block rather than one field four times.

   The two character kinds differ in what "still there" means: the direct one
   wants somebody alive and in a state under 0x12, and the indirect one wants
   the opposite, somebody with no hit points left whose two death fields say
   they can still be reached. Everything else - a place, the party as a whole -
   is always there and is not answered here at all. */
// FUNCTION: WIZ8 0x00536190
bool IsTargetStillPresent(const W8CombatSlot* target)
{
    if (target == 0) {
        return false;
    }

    switch (target->iType) {
    case W8_TARGET_KIND_CHARACTER:
        if (target->iChar == BAD_INDEX) {
            srAssertFail("pTarget->iChar != BAD_INDEX", TARGETING_CPP, 0x6c, 0);
        }
        if (g_status_685170.buffers.XChar[target->iChar].fOccupied == 0 ||
            g_status_685170.buffers.Char[target->iChar].hp_current == 0 ||
            g_status_685170.buffers.Char[target->iChar].highest_condition >= W8_CONDITION_DEAD) {
            return false;
        }
        break;

    case W8_TARGET_KIND_CHARACTER_INDIRECT:
        if (target->iChar == BAD_INDEX) {
            srAssertFail("pTarget->iChar != BAD_INDEX", TARGETING_CPP, 0x75, 0);
        }
        if (g_status_685170.buffers.XChar[target->iChar].fOccupied == 0 ||
            g_status_685170.buffers.Char[target->iChar].hp_current != 0 ||
            g_status_685170.buffers.Char[target->iChar]
                    .uiCondition[W8_CONDITION_REACHABLE_WHEN_DOWN] == 0 ||
            g_status_685170.buffers.Char[target->iChar].uiCondition[W8_CONDITION_BEYOND_REACH] !=
                0) {
            return false;
        }
        break;

    case W8_TARGET_KIND_MONSTER: {
        int index;
        W8MonsterInfo* monster_info;

        if (target->iMonsterID == BAD_INDEX) {
            srAssertFail("pTarget->iMonsterID != BAD_INDEX", TARGETING_CPP, 0x7e, 0);
        }
        index = MonsterGetIndexByLocationID(0x80, TARGETING_CPP, target->iMonsterID, 0);
        if (index == BAD_INDEX) {
            return false;
        }
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo != NULL", TARGETING_CPP, 0x88, 0);
        }
        if (monster_info->hp_current == 0 ||
            monster_info->uiCondition[W8_CONDITION_REACHABLE_WHEN_DOWN] != 0 ||
            monster_info->fActive == 0) {
            return false;
        }
        break;
    }

    case W8_TARGET_KIND_GROUP: {
        unsigned int group_list_index;
        W8MonsterGroup* group;

        if (target->iGroupID == BAD_INDEX) {
            srAssertFail("pTarget->iGroupID != BAD_INDEX", TARGETING_CPP, 0x99, 0);
        }
        group_list_index = GetMonsterGroupIndexByID(0x9b, TARGETING_CPP, target->iGroupID, 0);
        if (group_list_index == 0xffffffff) {
            return false;
        }
        group = GetMonsterGroupByListIndex(group_list_index);
        if (group == 0) {
            srAssertFail("pMonsterGroup != NULL", TARGETING_CPP, 0xa3, 0);
        }
        if (group->member_count == 0 || group->flag_28 == 0) {
            return false;
        }
        break;
    }

    case W8_TARGET_KIND_ITEM:
        if (target->pPCItem == 0) {
            srAssertFail("pTarget->pPCItem != NULL", TARGETING_CPP, 0xb4, 0);
        }
        if (target->pPCItem->iItemNo == BAD_INDEX) {
            return false;
        }
        break;

    default:
        break;
    }
    return true;
}

/* Tint one monster to say whether the character could act on it, and move the
   cursor to match. Green means yes and red means no; asking for no highlight at
   all tints it to nothing and answers no without touching the cursor.

   With no character named the answer is yes outright, so a bare hover over a
   monster reads as valid - it is the action, not the monster, that makes a
   target invalid. The cursor is only moved while nothing modal is up, and the
   valid case additionally waits for the screen to be idle, which is what keeps
   a cursor from flickering under a menu. */
// FUNCTION: WIZ8 0x00539110
char HighlightMonsterAsTarget(int location_id, int party_slot, char highlight)
{
    int index = MonsterGetIndexByLocationID(0x68d, TARGETING_CPP, location_id, 0);
    W8MonsterInfo* monster_info;
    W8Monster* monster;
    char valid = 0;

    if (index == BAD_INDEX) {
        return 0;
    }
    monster_info = MonsterGetScriptPartByLocationIndex(index);
    monster = monster_info->p3D;

    if (highlight == 0) {
        SetMonsterHighlightColour(monster, 0.0f, 0.0f, 0.0f, 0.0f);
        return 0;
    }

    if (party_slot == BAD_INDEX ||
        (IsSlotActionChosen(party_slot, W8_TARGETING_CONTEXT_CURRENT, 1, 0) &&
         CanTargetMonster(party_slot, location_id, 1, 0))) {
        valid = 1;
    }
    CombatAllowsLiveGroups();

    if (valid == 0) {
        SetMonsterHighlightColour(monster, 1.0f, 0.0f, 0.0f, 1.0f);
        if (g_modal_owner_0068edd0 == 0 && gXStatus.iCurrentCursor == W8_CURSOR_VALID_TARGET) {
            UpdateHeldItemCursor();
        }
        return 0;
    }

    SetMonsterHighlightColour(monster, 0.0f, 1.0f, 0.0f, 1.0f);
    if (g_modal_owner_0068edd0 == 0 && gXStatus.iTargetingMode == 0 && IsScreenIdle() &&
        gXStatus.iCurrentCursor != W8_CURSOR_INVALID_TARGET) {
        SetTargetCursor(W8_CURSOR_VALID_TARGET);
    }
    return valid;
}

/* Take down whatever one party slot was highlighting. A slot with a highlight
   list of its own empties that and nothing else; only a slot with none falls
   through to the target it was given, and then a monster and a group are
   handled separately because each has its own way of naming its monsters.

   Clearing a monster drops this slot's bit out of the monster's own highlight
   mask, so a monster several slots are highlighting stays lit for the rest;
   the original carries that body inline at both places rather than calling
   it. */
// FUNCTION: WIZ8 0x0053ac30
void ClearTargetHighlights(int party_slot, const W8CombatSlot* target)
{
    W8MonsterManagerEntry* slot = &gXStatus.monster_manager_entries[party_slot];
    unsigned int index;

    if (slot->highlighted_monsters.GetCount() > 0) {
        for (index = 0; index < (unsigned int)slot->highlighted_monsters.GetCount(); ++index) {
            SetMonsterHighlight(party_slot, *slot->highlighted_monsters.GetAt(index), 0);
        }
        slot->highlighted_monsters.Clear();
        return;
    }

    if (target->iType == W8_TARGET_KIND_MONSTER && target->iMonsterID != BAD_INDEX) {
        SetMonsterHighlight(party_slot, target->iMonsterID, 0);
    }

    if (target->iType == W8_TARGET_KIND_GROUP && target->iGroupID != BAD_INDEX) {
        unsigned int group_list_index =
            GetMonsterGroupIndexByID(0x5da, TARGETING_CPP, target->iGroupID, 0);

        if (group_list_index != 0xffffffff) {
            W8MonsterGroup* group = GetMonsterGroupByListIndex(group_list_index);

            for (index = 0; index < ILLength(group->monsters); ++index) {
                SetMonsterHighlight(party_slot, IListGetAt(group->monsters, index), 0);
            }
        }
    }
}

/* Combat's end: untint every live monster for each party slot whose bit it
   still carries, then drop the whole mask. */
// FUNCTION: WIZ8 0x0053ae00
void ClearAllMonsterHighlights(void)
{
    unsigned int index;
    unsigned int party_slot;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        W8Monster* monster = monster_info->p3D;
        unsigned char flags;

        if (monster_info->fActive == 0 || monster == 0) {
            continue;
        }
        flags = MonsterGetRuntimeFlag5BC(monster);
        if (flags == 0) {
            continue;
        }
        for (party_slot = 0; party_slot < 8; ++party_slot) {
            if ((flags & (1 << party_slot)) != 0) {
                NotifyMonsterHighlight(party_slot, monster_info->location_id, 0);
            }
        }
        MonsterSetRuntimeFlag5BC(monster, 0);
    }
}

/* 0x005EBB34: the float that stands for "no distance given". GameData.cpp
   reads the same constant as the level vector's absent value. */
/* The side selector that means any side at all. */
enum { W8_SIDE_ANY = 3 };

/* Gather every monster within one distance of a point, appending their
   location ids to the caller's vector. The distance is measured to the
   monster's surface rather than its centre, which is what the radius
   subtraction is; a distance of the "no distance given" constant gathers
   nothing at all rather than everything.

   The highlighting caller and the line-of-sight caller share the body: with
   highlighting on, a monster out of range has its tint cleared and one in
   range is taken without looking, and with it off nothing is tinted and a
   monster in range still has to be visible from the given eye point. */
// FUNCTION: WIZ8 0x00539e70
void CollectMonstersWithinRadius(const srVector3T<float>* centre, const srVector3T<float>* eye,
                                 W8GrowableVector<int>* found, float radius, char side,
                                 char highlighting)
{
    unsigned int index;

    if (radius == g_float_005ebb34) {
        return;
    }

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        W8Monster* monster = monster_info->p3D;
        W8MonsterRecord* record;
        srVector3T<float> position;

        if (monster_info->fActive == 0 || monster_info->hp_current == 0 ||
            monster_info->uiCondition[W8_CONDITION_REACHABLE_WHEN_DOWN] != 0) {
            continue;
        }
        record = GetMonsterDataForInfo(monster_info);
        if (record->untargetable_24a != 0) {
            continue;
        }
        if (monster_info->ubDisposition != side && side != W8_SIDE_ANY) {
            continue;
        }

        position = monster->GetPosition();
        if (radius < (*centre - position).Length() - monster->radius_084) {
            if (highlighting != 0) {
                SetMonsterHighlightColour(monster, 0.0f, 0.0f, 0.0f, 0.0f);
            }
            continue;
        }
        if (highlighting != 0 || monster_info->p3D->HasLineOfSightFromPoint004C4C40(*eye)) {
            found->Add(monster_info->location_id);
        }
    }
}

/* The two dialogue selections that have a targeting context of their own, and
   they are the same two action kinds - casting and using an item. */

/* Which targeting context is in force. A dialogue that is up and has settled on
   casting or on using an item owns the choice; failing that, the active slot
   with either overlay up gets the shared context, and otherwise it is simply
   whether a fight is on.

   Everything below carries this body inline rather than calling it, which is
   why the same fifteen-odd instructions open three of them. */
// FUNCTION: WIZ8 0x0053bc10
W8TargetingContext GetCurrentTargetingContext(int party_slot)
{
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0 &&
        g_level_block->selection_kind != -1) {
        if (g_level_block->selection_kind == W8_ACTION_CAST_SPELL &&
            g_level_block->selection_settled != 0) {
            return W8_TARGETING_CONTEXT_SPELL;
        }
        if (g_level_block->selection_kind == W8_ACTION_USE_ITEM &&
            g_level_block->selection_settled != 0) {
            return W8_TARGETING_CONTEXT_ITEM;
        }
        return W8_TARGETING_CONTEXT_DIALOGUE;
    }
    if (party_slot == g_status_685170.selected_character &&
        (gXStatus.fSpellCastMode != 0 || gXStatus.fItemSelectMode != 0)) {
        return W8_TARGETING_CONTEXT_SHARED;
    }
    return gXStatus.fCombatMode != 0 ? W8_TARGETING_CONTEXT_IN_COMBAT
                                     : W8_TARGETING_CONTEXT_OUT_OF_COMBAT;
}

/* Resolve "current" to a real context and check that what comes back is one.
   The switch answers each context with itself, so it exists only to catch a
   sixth value the caller invented rather than to map anything. */
// FUNCTION: WIZ8 0x0053b920
W8TargetingContext ResolveTargetingContext(int party_slot, W8TargetingContext context)
{
    if (context == W8_TARGETING_CONTEXT_CURRENT) {
        context = GetCurrentTargetingContext(party_slot);
    }
    switch (context) {
    case W8_TARGETING_CONTEXT_OUT_OF_COMBAT:
        return W8_TARGETING_CONTEXT_OUT_OF_COMBAT;
    case W8_TARGETING_CONTEXT_IN_COMBAT:
        return W8_TARGETING_CONTEXT_IN_COMBAT;
    case W8_TARGETING_CONTEXT_SHARED:
        return W8_TARGETING_CONTEXT_SHARED;
    case W8_TARGETING_CONTEXT_SPELL:
        return W8_TARGETING_CONTEXT_SPELL;
    case W8_TARGETING_CONTEXT_ITEM:
        return W8_TARGETING_CONTEXT_ITEM;
    case W8_TARGETING_CONTEXT_FIVE:
        return W8_TARGETING_CONTEXT_FIVE;
    case W8_TARGETING_CONTEXT_DIALOGUE:
        return W8_TARGETING_CONTEXT_DIALOGUE;
    default:
        srAssertFail("FALSE", TARGETING_CPP, 0xc5b, 0);
    }
    return W8_TARGETING_CONTEXT_IN_COMBAT;
}

/* The target block one context uses. Five of the six live on the slot's own
   row and the sixth is shared by the party, which is what makes this the one
   place that knows the row holds five blocks of the same shape rather than one
   block and four runs of numbers. */
// FUNCTION: WIZ8 0x0053b7f0
W8CombatSlot* GetTargetBlockForContext(int party_slot, W8TargetingContext context)
{
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];

    if (context == W8_TARGETING_CONTEXT_CURRENT) {
        context = GetCurrentTargetingContext(party_slot);
    }
    switch (context) {
    case W8_TARGETING_CONTEXT_OUT_OF_COMBAT:
        return &row->target_out_of_combat;
    case W8_TARGETING_CONTEXT_IN_COMBAT:
        return &row->target_in_combat;
    case W8_TARGETING_CONTEXT_SHARED:
    case W8_TARGETING_CONTEXT_SPELL:
        return &row->spell_target;
    case W8_TARGETING_CONTEXT_ITEM:
        return &row->item_target;
    case W8_TARGETING_CONTEXT_FIVE:
        return &row->target_context_5;
    default:
        srAssertFail("FALSE", TARGETING_CPP, 0xc37, 0);
    }
    return 0;
}

/* Replace a monster's current combat target with one monster id. The target
   block is cleared as a whole before its four discriminating fields are
   established, matching the other target builders in this unit. */
// FUNCTION: WIZ8 0x0053A2C0
void SetMonsterCombatTarget(W8MonsterInfo* monster_info, int location_id)
{
    W8CombatSlot* target = &monster_info->Target;

    memset(target, 0, sizeof(*target));
    target->iType = W8_TARGET_KIND_NONE;
    target->iMonsterID = BAD_INDEX;
    target->iChar = BAD_INDEX;
    target->iGroupID = BAD_INDEX;
    target->iMonsterID = location_id;
    target->iType = W8_TARGET_KIND_MONSTER;
}

/* Probe whether a monster's current combat target is one of the kinds its
   chosen hostile spell accepts. The caller only needs the validator's side
   effects, so this wrapper discards its answer. */
// FUNCTION: WIZ8 0x0053A300
bool MonsterTargetMatchesSpell(W8MonsterInfo* monster_info, int spell_id)
{
    return TargetMatchesNeeded(&monster_info->Target, GetTargetNeededForSpellHostile(spell_id));
}

/* Map gXStatus.iTargetingMode to a cursor-table index for SetTargetCursor.
   Returned values are cursor slots (including 10..12), not W8TargetingContext. */
// FUNCTION: WIZ8 0x0053A3D0
int GetTargetingCursorForState(int alternate)
{
    switch (gXStatus.iTargetingMode) {
    case 1:
    case 6:
    case 7:
        return (alternate != 0) + 3;
    case 2:
        return alternate != 0;
    case 3:
        return 2;
    case 4:
        return 12;
    case 5:
        return 11 - (alternate != 0);
    default:
        return W8_CURSOR_NONE;
    }
}

/* Whether the pending spell in one party row needs an explicit target. */
// FUNCTION: WIZ8 0x0053A700
bool ActionNeedsExplicitTarget(int party_slot)
{
    switch (GetSpellTargetType(g_status_685170.buffers.XChar[party_slot].spell_id, 0)) {
    case W8_TARGET_TYPE_CASTER:
    case W8_TARGET_TYPE_PARTY:
    case W8_TARGET_TYPE_ALL_ENEMIES:
    case W8_TARGET_TYPE_LOCK_OR_TRAP:
        return false;
    case W8_TARGET_TYPE_ENEMY:
        if (gXStatus.fCampMode != 0) {
            return false;
        }
        return g_settings_6850c8.autoscroll_combat_messages == 0;
    default:
        return true;
    }
}

/* Return the spell-like id carried by a chosen action: the fixed attack id,
   a spell's detail word, or the spell attached to an item use. */
// FUNCTION: WIZ8 0x0053A8D0
unsigned int GetActionSpellLikeId(int party_slot, W8TargetingContext context)
{
    int action;
    int detail;
    W8ActionDetailBlock* detail_block;

    ChooseCombatAction(party_slot, context, &action, &detail, 0, &detail_block);
    if (action == 2) {
        return 0x77;
    }
    if (action == 7) {
        return detail;
    }
    if (action == 8) {
        return GetItemSpell(detail_block->item_use.item);
    }
    return 0;
}

/* plsMonsterList index that last satisfied AnyMonsterVisible0053A1D0. */
// GLOBAL: WIZ8 0x0061D14C
static int g_last_visible_monster_0061d14c = -1;

/* Select the cursor and renderer-side targeting mode for one targeting state,
   then clear the cached world point so the following refresh recomputes it. */
// FUNCTION: WIZ8 0x0053A320
void SetTargetingMode(int state)
{
    int cursor;

    gXStatus.iTargetingMode = state;
    switch (state) {
    case 1:
    case 6:
    case 7:
        cursor = 3;
        break;
    case 2:
        cursor = 0;
        break;
    case 3:
        cursor = 2;
        break;
    case 4:
        cursor = 12;
        break;
    case 5:
        cursor = 11;
        break;
    default:
        cursor = W8_CURSOR_NONE;
        break;
    }
    if (cursor != gXStatus.iCurrentCursor) {
        SetTargetCursor(cursor);
    }
    gXStatus.target_position.SetZero();
    RequestRefreshPartyState();
    if (state == 4) {
        SetTargetConeEnabled004ADD30(1);
        PauseMainGameWorld();
    } else {
        SetTargetConeEnabled004ADD30(0);
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
            ResumeMainGameWorld();
        }
    }
}

/* Re-check the selected character's committed target after its action has
   changed. A slot that cannot act at all falls back to no targeting;
   otherwise the action's needed kind is worked out again and the recorded
   target either still satisfies it - leaving no targeting mode - or the mode
   matching what the action now needs is entered so the player can pick. */
// FUNCTION: WIZ8 0x00537540
void RevalidateSelectedTarget(int party_slot)
{
    W8CombatSlot* target;
    W8ActionDetailBlock* detail_block;
    W8TargetingContext context;
    int needed;
    int action;
    int detail;

    if (party_slot != g_status_685170.selected_character) {
        return;
    }
    if (CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_CURRENT, 1, 0) == 0) {
        SetTargetingMode(0);
        return;
    }
    target = GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT);
    context = GetCurrentTargetingContext(party_slot);
    needed = W8_TARGET_KIND_NONE;
    if (ResolveTargetingContext(party_slot, context) != 0) {
        ChooseCombatAction(party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0,
                           &detail_block);
        switch (action) {
        case 0:
        case 1:
            needed = W8_TARGET_KIND_PARTY;
            break;
        case 2:
            needed = W8_TARGET_KIND_GROUP;
            break;
        case 5:
            needed = W8_TARGET_KIND_CHARACTER;
            break;
        case 7:
            needed = GetTargetNeededForSpellFriendly(detail, 0, W8_TARGETING_CONTEXT_CURRENT);
            break;
        case 8:
            needed = GetTargetNeededForItem(detail_block->item_use.item);
            break;
        default:
            needed = W8_TARGET_KIND_NONE;
            break;
        }
    }
    if (TargetMatchesNeeded(target, needed) != 0) {
        SetTargetingMode(0);
    } else {
        SetTargetingMode(needed);
    }
}

/* Whether anything in `group` is close enough to `source` for the source's
   action to reach it. A character source asks the range of its chosen action;
   a monster source asks the range of its own pending action. The group's
   members are then walked until one stands inside that distance - the source
   measures to the party for a character and to its own monster for a
   monster. */
// FUNCTION: WIZ8 0x00537780
bool IsTargetSourceInRangeOfGroup(const W8TargetSource* source, W8MonsterGroup* group,
                                  W8TargetingContext context)
{
    W8Monster* source_monster;
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    unsigned int index;
    int range;
    float distance;
    float max_distance;

    source_monster = 0;
    if (source->iType == W8_TARGET_SOURCE_CHARACTER) {
        if (source->iChar == BAD_INDEX) {
            srAssertFail("pSource->iChar != BAD_INDEX", TARGETING_CPP, 0xce3, 0);
        }
        range = GetCharActionRange(source->iChar, 0, context);
    } else {
        if (source->iType != W8_TARGET_SOURCE_MONSTER) {
            srAssertFail("FALSE", TARGETING_CPP, 0x34b, 0);
            return false;
        }
        if (source->iMonsterID == BAD_INDEX) {
            srAssertFail("pSource->iMonsterID != BAD_INDEX", TARGETING_CPP, 0xcf8, 0);
        }
        if (source->iMonsterID == BAD_INDEX) {
            srAssertFail("pSource->iMonsterID != -1", TARGETING_CPP, 0x343, 0);
        }
        index = MonsterGetIndexByLocationID(0x344, TARGETING_CPP, source->iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        source_monster = GetMonsterByLocationID(source->iMonsterID);
        record = GetMonsterDataForInfo(monster_info);
        range = GetMonsterActionRangeCategory(monster_info, record, 0);
    }
    max_distance = CalcRangeDistance(static_cast<W8RangeCategory>(range));
    for (index = 0; index < ILLength(group->monsters); ++index) {
        W8Monster* member = GetMonsterByLocationID(IListGetAt(group->monsters, index));

        if (source->iType == W8_TARGET_SOURCE_CHARACTER) {
            if (source->iChar == BAD_INDEX) {
                srAssertFail("pSource->iChar != BAD_INDEX", TARGETING_CPP, 0xce3, 0);
            }
            distance = member->GetDistanceToPlayer004C7CB0();
        } else if (source->iType == W8_TARGET_SOURCE_MONSTER) {
            if (source->iMonsterID == BAD_INDEX) {
                srAssertFail("pSource->iMonsterID != BAD_INDEX", TARGETING_CPP, 0xcf8, 0);
            }
            distance = member->GetDistanceToMonster004C7DD0(source_monster);
        }
        if (distance <= max_distance) {
            return true;
        }
    }
    return false;
}

/* Remove one party slot's highlight bit from every live monster that carries
   it, notifying the render-side highlight owner for each changed monster. */
// FUNCTION: WIZ8 0x0053AEB0
void ClearPartySlotMonsterHighlights(unsigned int party_slot)
{
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        W8Monster* monster = monster_info->p3D;

        if (monster_info->fActive != 0 && monster != 0) {
            unsigned char flags = MonsterGetRuntimeFlag5BC(monster);
            unsigned char bit = static_cast<unsigned char>(1 << (party_slot & 31));

            if ((flags & bit) != 0) {
                MonsterSetRuntimeFlag5BC(monster, static_cast<unsigned char>(flags & ~bit));
                NotifyMonsterHighlight(party_slot, monster_info->location_id, 0);
            }
        }
    }
}

/* Select the party member whose spell-targeting state is being edited. Any
   monster highlights owned by that slot and its shared targeting record are
   cleared before the ordinary targeting UI is restored. */
// FUNCTION: WIZ8 0x0053AF40
void SelectSpellCastingPartySlot(int party_slot)
{
    W8CombatSlot target;

    ClearPartySlotMonsterHighlights(party_slot);
    ResetCombatSlot(&target);
    AimAtTarget(party_slot, &target, W8_TARGETING_CONTEXT_SHARED);
    SetTargetingMode(0);
    gXStatus.target_markers.Clear();
    RequestRefreshPartyState();
}

/* Commit the shared spell target into the selected character's active action
   target and restart that portrait's breathing cycle. */
// FUNCTION: WIZ8 0x0053A830
void CommitSelectedSpellTarget(void)
{
    int action;
    int detail;
    W8ActionDetailBlock* detail_block;
    int party_slot = g_status_685170.selected_character;
    W8TargetingContext context = gXStatus.fCombatMode != 0 ? W8_TARGETING_CONTEXT_IN_COMBAT
                                                           : W8_TARGETING_CONTEXT_OUT_OF_COMBAT;

    ChooseCombatAction(party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0,
                       &detail_block);
    if (action == 8) {
        GetItemSpell(detail_block->item_use.item);
    }
    *GetTargetBlockForContext(party_slot, context) = gXStatus.shared_target;
    StartBreathCycle(party_slot, 1);
}

/* Recompute one party slot's combat-target highlights from its current action.
   Spell actions repopulate the slot's highlighted-monster list through the
   magic helper; other actions set or clear direct monster and group highlights. */
// FUNCTION: WIZ8 0x0053A930
void RefreshCombatTargetHighlights(int party_slot, W8CombatSlot* target)
{
    int action;
    int detail;
    W8ActionDetailBlock* detail_block;
    unsigned int spell_id;
    W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[party_slot];

    entry->highlighted_monsters.Clear();
    ChooseCombatAction(party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0,
                       &detail_block);
    spell_id = 0;
    if (action == 2) {
        spell_id = 0x77;
    } else if (action == 7) {
        spell_id = detail;
    } else if (action == 8) {
        spell_id = GetItemSpell(detail_block->item_use.item);
    }

    if (spell_id != 0) {
        if (target == 0) {
            srAssertFail("pSource != NULL", TARGETING_CPP, 0xcc9, 0);
        }

        {
            W8TargetSource source;
            W8GrowableVector<int> scratch;

            memset(&source, 0, sizeof(source));
            source.iType = W8_TARGET_SOURCE_CHARACTER;
            source.iChar = party_slot;
            PopulateSpellTargetMarkers(spell_id, 1, &source, target, &entry->highlighted_monsters,
                                       &scratch, 0);
        }

        for (unsigned int monster_list_index = 0;
             monster_list_index < PLLength(gXStatus.plsMonsterList); ++monster_list_index) {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
            W8Monster* monster = monster_info->p3D;

            if (monster_info->fActive != 0 && monster != 0) {
                unsigned char flags = MonsterGetRuntimeFlag5BC(monster);
                unsigned char bit = static_cast<unsigned char>(1 << (party_slot & 31));

                if ((flags & bit) != 0) {
                    MonsterSetRuntimeFlag5BC(monster, static_cast<unsigned char>(flags & ~bit));
                    NotifyMonsterHighlight(party_slot, monster_info->location_id, 0);
                }
            }
        }

        for (int highlight_index = 0; highlight_index < entry->highlighted_monsters.GetCount();
             ++highlight_index) {
            SetMonsterHighlight(party_slot, *entry->highlighted_monsters.GetAt(highlight_index), 1);
        }
        return;
    }

    if (target->iType == W8_TARGET_KIND_MONSTER && target->iMonsterID != BAD_INDEX) {
        unsigned int monster_index =
            MonsterGetIndexByLocationID(0x757, TARGETING_CPP, target->iMonsterID, 0);

        if (monster_index != 0xffffffff) {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            W8Monster* monster = monster_info->p3D;

            if (monster == 0) {
                srAssertFail("pMonster", TARGETING_CPP, 0x760, 0);
            }
            unsigned char flags = MonsterGetRuntimeFlag5BC(monster);
            unsigned char bit = static_cast<unsigned char>(1 << (party_slot & 31));

            MonsterSetRuntimeFlag5BC(monster, static_cast<unsigned char>(flags | bit));
            NotifyMonsterHighlight(party_slot, target->iMonsterID, 1);
        }
    }

    if (target->iType == W8_TARGET_KIND_GROUP && target->iGroupID != BAD_INDEX) {
        unsigned int group_index =
            GetMonsterGroupIndexByID(0x5da, TARGETING_CPP, target->iGroupID, 0);

        if (group_index != 0xffffffff) {
            W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);

            for (unsigned int member_index = 0; member_index < ILLength(group->monsters);
                 ++member_index) {
                SetMonsterHighlight(party_slot, IListGetAt(group->monsters, member_index), 1);
            }
        }
    }
}

/* When the ranged target point moves, clear any stale spell-target tint and
   rebuild highlights for the current action at the new position. */
// FUNCTION: WIZ8 0x0053B310
void RefreshSpellTargetHighlightsAtRange(void)
{
    srVector3T<float> position;
    W8MonsterInfo* monster_info;

    GetCameraForwardPoint00421150(GetRangeConstant5EC35C(), &position);
    if (position.x == gXStatus.target_position.x && position.y == gXStatus.target_position.y &&
        position.z == gXStatus.target_position.z) {
        return;
    }

    gXStatus.target_position = position;
    monster_info = GetNextMonsterInfo(1);
    while (monster_info != 0) {
        if (monster_info->fActive != 0 && monster_info->hp_current != 0 &&
            monster_info->uiCondition[W8_CONDITION_DEAD] == 0) {
            W8Monster* monster = monster_info->p3D;
            float channels[4];

            memcpy(channels, &monster->m_pRep->render_state_04c, sizeof(channels));
            if (channels[0] != g_float_005ebb34 || channels[1] != g_float_005ebb34 ||
                channels[2] != g_float_005ebb34 || channels[3] != g_float_005ebb34) {
                SetMonsterHighlightColour(monster, 0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
        monster_info = GetNextMonsterInfo(0);
    }
    HighlightSpellTargetsAtCachedPosition();
}

/* Tint every monster the current spell action can reach at the cached target
   point. Non-spell actions leave every monster cleared to the default block. */
// FUNCTION: WIZ8 0x0053B480
void HighlightSpellTargetsAtCachedPosition(void)
{
    int party_slot = g_status_685170.selected_character;
    unsigned int spell_id = GetActionSpellLikeId(party_slot, W8_TARGETING_CONTEXT_CURRENT);

    if (spell_id == 0) {
        return;
    }

    W8TargetSource source;
    W8CombatSlot target;
    W8GrowableVector<int> markers;
    W8GrowableVector<int> scratch;

    SetTargetSourceToCharacter(party_slot, &source);
    ResetCombatSlot(&target);
    target.iType = W8_TARGET_KIND_PLACE;
    target.point = gXStatus.target_position;
    PopulateSpellTargetMarkers(spell_id, 1, &source, &target, &markers, &scratch, 0);

    for (int index = 0; index < markers.GetCount(); ++index) {
        W8Monster* monster = GetMonsterByLocationID(*markers.GetAt(index));

        SetMonsterHighlightColour(monster, 0.0f, 1.0f, 0.0f, 1.0f);
    }
}

/* Rebuild the world-space target marker from the selected character's current
   combat action and one world point. When the chosen action carries no
   spell-like id, the marker vector is left empty. */
// FUNCTION: WIZ8 0x0053B660
void PopulateTargetMarkerForCurrentAction(const srVector3T<float>* position,
                                          W8GrowableVector<int>* marker_vector, int enabled)
{
    int party_slot = g_status_685170.selected_character;
    int action;
    int detail;
    W8ActionDetailBlock* detail_block;
    unsigned int spell_id;

    ChooseCombatAction(party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0,
                       &detail_block);
    if (action == 2) {
        spell_id = 0x77;
    } else if (action == 7) {
        spell_id = detail;
    } else if (action == 8) {
        spell_id = GetItemSpell(detail_block->item_use.item);
    } else {
        return;
    }
    if (spell_id == 0) {
        return;
    }

    marker_vector->Clear();

    if (position == 0) {
        srAssertFail("pSource != NULL", TARGETING_CPP, 0xcc9, 0);
    }

    W8TargetSource source;
    memset(&source, 0, sizeof(source));
    source.iType = W8_TARGET_SOURCE_CHARACTER;
    source.iChar = party_slot;

    W8CombatSlot target;
    memset(&target, 0, sizeof(target));
    target.iChar = BAD_INDEX;
    target.iMonsterID = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = W8_TARGET_KIND_PLACE;
    target.point = *position;

    W8GrowableVector<int> scratch;
    PopulateSpellTargetMarkers(spell_id, 1, &source, &target, marker_vector, &scratch, enabled);
}

/* Clear the target marker and request the party-display refresh that consumes
   the change. */
// FUNCTION: WIZ8 0x0053B160
void ClearTargetMarker(void)
{
    gXStatus.target_markers.Clear();
    RequestRefreshPartyState();
}

/* Recompute the target point and refresh the marker only when it differs
   from the cached three-float position. */
// FUNCTION: WIZ8 0x0053B170
void RefreshTargetMarker(void)
{
    srVector3T<float> position;

    GetWorldCursorTargetPosition00492500(&position);
    if (position.x != gXStatus.target_position.x || position.y != gXStatus.target_position.y ||
        position.z != gXStatus.target_position.z) {
        gXStatus.target_position = position;
        PopulateTargetMarkerForCurrentAction(&position, &gXStatus.target_markers, 1);
    }
}

/* Whether the slot holds a dead character a dead-targeting mode may still
   reach - occupied, actually dead, still reachable while down, and not yet
   beyond reach. */
// FUNCTION: WIZ8 0x0053C2C0
bool IsDeadCharacterTargetable(int party_slot)
{
    if (g_status_685170.buffers.XChar[party_slot].fOccupied == 0) {
        return false;
    }
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    if (character->hp_current > 0 ||
        character->uiCondition[W8_CONDITION_REACHABLE_WHEN_DOWN] == 0) {
        return false;
    }
    return character->uiCondition[W8_CONDITION_BEYOND_REACH] <= 0;
}

/* A party slot can participate only while occupied, alive, and below the
   terminal character-state threshold. */
// FUNCTION: WIZ8 0x0053C270
bool CanPartySlotParticipate(int party_slot)
{
    return g_status_685170.buffers.XChar[party_slot].fOccupied != 0 &&
           g_status_685170.buffers.Char[party_slot].hp_current != 0 &&
           g_status_685170.buffers.Char[party_slot].highest_condition < W8_CONDITION_DEAD;
}

/* Validate a targeting context a second time, after resolving "current". The
   inner resolution has an assertion of its own, so a context that gets this far
   has already been checked once; this one guards the caller's own use of the
   answer, and the two report different lines. */
// FUNCTION: WIZ8 0x0053ba20
W8TargetingContext GetValidatedTargetingContext(int party_slot, W8TargetingContext context)
{
    if (context == W8_TARGETING_CONTEXT_CURRENT) {
        context = ResolveTargetingContext(party_slot, W8_TARGETING_CONTEXT_CURRENT);
    }
    switch (context) {
    case W8_TARGETING_CONTEXT_OUT_OF_COMBAT:
        return W8_TARGETING_CONTEXT_OUT_OF_COMBAT;
    case W8_TARGETING_CONTEXT_IN_COMBAT:
        return W8_TARGETING_CONTEXT_IN_COMBAT;
    case W8_TARGETING_CONTEXT_SHARED:
        return W8_TARGETING_CONTEXT_SHARED;
    case W8_TARGETING_CONTEXT_SPELL:
        return W8_TARGETING_CONTEXT_SPELL;
    case W8_TARGETING_CONTEXT_ITEM:
        return W8_TARGETING_CONTEXT_ITEM;
    case W8_TARGETING_CONTEXT_FIVE:
        return W8_TARGETING_CONTEXT_FIVE;
    case W8_TARGETING_CONTEXT_DIALOGUE:
        return W8_TARGETING_CONTEXT_DIALOGUE;
    default:
        srAssertFail("FALSE", TARGETING_CPP, 0xc87, 0);
    }
    return W8_TARGETING_CONTEXT_IN_COMBAT;
}

/* Whether a party slot could aim its chosen action at one whole monster group.
   An untargetable group is refused before anything else is worked out. What
   the action needs then decides how the question is asked: a group-wide need
   is a range test against the group, and a single-monster need is satisfied by
   any one member the slot can reach.

   The source block is built here rather than passed in, so this always asks on
   the character's own behalf. */
// FUNCTION: WIZ8 0x00536d60
bool CanTargetMonsterGroup(int party_slot, W8MonsterGroup* group)
{
    W8TargetSource source;
    int action;
    int detail;
    W8ActionDetailBlock* detail_block;
    char needed;
    unsigned int index;
    int reachable;

    if (MonsterGroupGetRecord(group)->untargetable_24a != 0) {
        return 0;
    }

    if (ResolveTargetingContext(party_slot, W8_TARGETING_CONTEXT_CURRENT) == 0) {
        needed = 0;
    } else {
        ChooseCombatAction(party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0,
                           &detail_block);
        needed = GetTargetNeededForAction(action, detail, detail_block);
    }

    SetTargetSourceToCharacter(party_slot, &source);

    if (needed == 5) {
        return IsTargetSourceInRangeOfGroup(&source, group, W8_TARGETING_CONTEXT_CURRENT);
    }
    if (needed != 2 && needed != 1) {
        return 0;
    }

    reachable = 0;
    for (index = 0; index < ILLength(group->monsters); ++index) {
        W8MonsterInfo* monster_info =
            MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x37a, TARGETING_CPP, IListGetAt(group->monsters, index), 1));

        if (CanPartyMemberAimAtMonster(party_slot, 2, monster_info, W8_TARGETING_CONTEXT_CURRENT,
                                       0)) {
            ++reachable;
        }
    }
    return reachable != 0;
}

/* Re-aim a party slot whose pending action's target can no longer be used:
   outside combat the stored target is simply cleared, while in combat a live
   attack, spell or item action repicks a monster - the target's own group
   first, then the generic fallback picker - and aims at it. When the repick
   lands the option flag drives a held-item cursor refresh, and with it clear
   the slot's stored combat target is re-validated once against the in-combat
   action before being aimed at again. */
// FUNCTION: WIZ8 0x00536570
bool RepickActionTarget(int party_slot, W8TargetingContext context, int arg)
{
    W8ActionDetailBlock* detail_block;
    W8ActionDetailBlock* detail_block_2;
    W8CombatSlot* target;
    W8CombatSlot* target_2;
    W8CombatSlot new_target;
    W8ItemDatabaseRecord* record;
    W8MonsterInfo* monster_info;
    W8TargetingContext action_context;
    W8TargetingContext resolved;
    char is_attack_kind = 0;
    unsigned char result = 0;
    int needed;
    int kind;
    int kind_2;
    int action;
    int action_2;
    int previous_kind;
    int group_id;
    unsigned int monster_index;
    int selected;

    if (g_status_685170.buffers.XChar[party_slot].fOccupied == 0 ||
        g_status_685170.buffers.Char[party_slot].hp_current == 0 ||
        g_status_685170.buffers.Char[party_slot].highest_condition >= W8_CONDITION_DEAD) {
        return 0;
    }

    action_context = W8_TARGETING_CONTEXT_DIALOGUE;
    resolved = context;
    if (context == W8_TARGETING_CONTEXT_CURRENT) {
        if (g_current_screen_state.id == 7 && g_level_block != 0 &&
            g_level_block->selection_kind != -1) {
            if (g_level_block->selection_kind == W8_ACTION_CAST_SPELL &&
                g_level_block->selection_settled != 0) {
                resolved = W8_TARGETING_CONTEXT_SPELL;
            } else if (g_level_block->selection_kind == W8_ACTION_USE_ITEM &&
                       g_level_block->selection_settled != 0) {
                resolved = W8_TARGETING_CONTEXT_ITEM;
            } else {
                resolved = W8_TARGETING_CONTEXT_DIALOGUE;
            }
        } else if (party_slot == g_status_685170.selected_character &&
                   (gXStatus.fSpellCastMode != 0 || gXStatus.fItemSelectMode != 0)) {
            resolved = W8_TARGETING_CONTEXT_SHARED;
        } else {
            resolved = (W8TargetingContext)(gXStatus.fCombatMode != 0);
        }
    }
    if (resolved == W8_TARGETING_CONTEXT_CURRENT) {
        resolved = GetCurrentTargetingContext(party_slot);
    }
    switch (resolved) {
    case W8_TARGETING_CONTEXT_OUT_OF_COMBAT:
        action_context = W8_TARGETING_CONTEXT_OUT_OF_COMBAT;
        break;
    case W8_TARGETING_CONTEXT_IN_COMBAT:
        action_context = W8_TARGETING_CONTEXT_IN_COMBAT;
        break;
    case W8_TARGETING_CONTEXT_SHARED:
        action_context = W8_TARGETING_CONTEXT_SHARED;
        break;
    case W8_TARGETING_CONTEXT_SPELL:
        action_context = W8_TARGETING_CONTEXT_SPELL;
        break;
    case W8_TARGETING_CONTEXT_ITEM:
        action_context = W8_TARGETING_CONTEXT_ITEM;
        break;
    case W8_TARGETING_CONTEXT_FIVE:
        action_context = W8_TARGETING_CONTEXT_FIVE;
        break;
    case W8_TARGETING_CONTEXT_DIALOGUE:
        break;
    default:
        srAssertFail("FALSE", TARGETING_CPP, 0xc5b, 0);
        break;
    }
    ChooseCombatAction(party_slot, action_context, &kind, &action, &target, &detail_block);
    if (target == 0) {
        srAssertFail("pTarget", TARGETING_CPP, 0xfc, 0);
    }
    switch (kind) {
    case 0:
    case 1:
        needed = 2;
        break;
    case 2:
        needed = 4;
        break;
    case 5:
        needed = 1;
        break;
    case 7:
        needed = GetTargetNeededForSpellFriendly(action, 0, W8_TARGETING_CONTEXT_CURRENT);
        break;
    case 8:
        if (detail_block->item_use.item != 0 && detail_block->item_use.item->iItemNo != -1 &&
            (record = &g_item_records[detail_block->item_use.item->iItemNo],
             record->spell_id != 0)) {
            needed = GetTargetNeededForSpellFriendly(
                record->spell_id, ItemClassNormalizesTarget(record), W8_TARGETING_CONTEXT_CURRENT);
            break;
        }
        /* fall through */
    default:
        needed = 0;
        break;
    }
    previous_kind = target->iType;
    if (gXStatus.fCombatMode != 0) {
        if (resolved == W8_TARGETING_CONTEXT_OUT_OF_COMBAT) {
            ++g_combat_state->characters[party_slot].pending_action_repick_count;
        }
        switch (kind) {
        case 0:
        case 1:
            is_attack_kind = 1;
            break;
        case 7:
        case 8:
            break;
        default:
            goto clear_target;
        }
        if (needed == 2) {
            group_id = -1;
            if (target->iMonsterID != -1 && (monster_index = MonsterGetIndexByLocationID(
                                                 0xf40, TARGETING_CPP, target->iMonsterID, 0),
                                             monster_index != 0xffffffff)) {
                monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                group_id = monster_info->monster_group_id;
            }
            selected = ChooseMonsterTarget(party_slot, group_id, action_context);
            if (selected != -1 ||
                (is_attack_kind != 0 && (selected = ChooseFallbackMonsterTarget0053C990(
                                             party_slot, group_id, action_context)) != -1)) {
                memset(&new_target, 0, sizeof(new_target));
                new_target.iChar = -1;
                new_target.iGroupID = -1;
                new_target.iType = W8_TARGET_KIND_MONSTER;
                new_target.iMonsterID = selected;
                AimAtTarget(party_slot, &new_target, (W8TargetingContext)arg);
                result = 1;
                if (arg != 0) {
                    StartBreathCycle(party_slot, 0);
                }
                if (arg == 0) {
                    if (g_settings_6850c8.verbose_combat_messages != 0) {
                        PostCharacterNotice(party_slot, gppStringList[0x26b]);
                    }
                    ChooseCombatAction(
                        party_slot,
                        ResolveTargetingContext(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT),
                        &kind_2, &action_2, &target_2, &detail_block_2);
                    needed = GetTargetNeededForAction(kind_2, action_2, detail_block_2);
                    if (TargetMatchesNeeded(target_2, needed) == 0 ||
                        IsCurrentTargetInRange(party_slot, 2, W8_TARGETING_CONTEXT_IN_COMBAT) ==
                            0) {
                        AimAtTarget(party_slot, target, W8_TARGETING_CONTEXT_IN_COMBAT);
                    }
                }
                goto done;
            }
        }
    }
clear_target:
    memset(&new_target, 0, sizeof(new_target));
    new_target.iMonsterID = -1;
    new_target.iChar = -1;
    new_target.iGroupID = -1;
    new_target.iType = W8_TARGET_KIND_NONE;
    AimAtTarget(party_slot, &new_target, (W8TargetingContext)arg);
done:
    if (gXStatus.fCombatMode != 0 && action_context == W8_TARGETING_CONTEXT_IN_COMBAT &&
        target->iType != previous_kind) {
        RequestPartySlotRedraw(party_slot);
    }
    return result;
}

/* Whether the party slot's pending action still has a target it can use.
   A CURRENT context is resolved first: while the pick-an-item screen owns the
   main view the level's selection kind decides, while a spell or item choice
   is open on the selected character the shared context wins, and otherwise it
   falls back to combat state. The kind the slot's chosen action produces is
   then turned into the target need it satisfies - attacks need a monster or
   group, spells and item casts ask the spell record - and the stored target
   has to both match that need and still be in range. */
// FUNCTION: WIZ8 0x00536f60
bool TargetIsInPlay(int party_slot, int value, W8TargetingContext context)
{
    W8TargetingContext resolved;
    W8ActionDetailBlock* detail_block;
    W8ItemDatabaseRecord* record;
    W8CombatSlot* target;
    int kind;
    int action;
    int needed;

    resolved = context;
    if (context == W8_TARGETING_CONTEXT_CURRENT) {
        if (g_current_screen_state.id == 7 && g_level_block != 0 &&
            g_level_block->selection_kind != -1) {
            if (g_level_block->selection_kind == W8_ACTION_CAST_SPELL &&
                g_level_block->selection_settled != 0) {
                resolved = W8_TARGETING_CONTEXT_SPELL;
            } else if (g_level_block->selection_kind == W8_ACTION_USE_ITEM &&
                       g_level_block->selection_settled != 0) {
                resolved = W8_TARGETING_CONTEXT_ITEM;
            } else {
                resolved = W8_TARGETING_CONTEXT_DIALOGUE;
            }
        } else if (party_slot == g_status_685170.selected_character &&
                   (gXStatus.fSpellCastMode != 0 || gXStatus.fItemSelectMode != 0)) {
            resolved = W8_TARGETING_CONTEXT_SHARED;
        } else {
            resolved = (W8TargetingContext)(gXStatus.fCombatMode != 0);
        }
    }
    switch (resolved) {
    case W8_TARGETING_CONTEXT_OUT_OF_COMBAT:
    case W8_TARGETING_CONTEXT_IN_COMBAT:
    case W8_TARGETING_CONTEXT_SHARED:
    case W8_TARGETING_CONTEXT_SPELL:
    case W8_TARGETING_CONTEXT_ITEM:
    case W8_TARGETING_CONTEXT_FIVE:
    case W8_TARGETING_CONTEXT_DIALOGUE:
        break;
    default:
        srAssertFail("FALSE", TARGETING_CPP, 0xc5b, 0);
        break;
    }
    ChooseCombatAction(party_slot, resolved, &kind, &action, &target, &detail_block);
    switch (kind) {
    case 0:
    case 1:
        needed = 2;
        break;
    case 2:
        needed = 4;
        break;
    case 5:
        needed = 1;
        break;
    case 7:
        needed = GetTargetNeededForSpellFriendly(action, 0, W8_TARGETING_CONTEXT_CURRENT);
        break;
    case 8:
        if (detail_block->item_use.item != 0 && detail_block->item_use.item->iItemNo != -1 &&
            (record = &g_item_records[detail_block->item_use.item->iItemNo],
             record->spell_id != 0)) {
            needed = GetTargetNeededForSpellFriendly(
                record->spell_id, ItemClassNormalizesTarget(record), W8_TARGETING_CONTEXT_CURRENT);
            break;
        }
        /* fall through */
    default:
        needed = 0;
        break;
    }
    if (TargetMatchesNeeded(target, needed) == 0) {
        return 0;
    }
    return IsCurrentTargetInRange(party_slot, action, context) != 0;
}

/* Whether a party slot's chosen action can be aimed at one monster. The
   monster has to be live, standing and reachable; while the camp screen is up
   only the monster it is showing can be aimed at at all.

   What the action needs decides the rest: a group need is answered against the
   monster's group, a single-monster need only when the caller allows one, and
   an action needing nothing is refused outright in combat. The database's
   untargetable flag is checked last rather than first, so an action needing
   nothing still reaches a monster carrying it. */
// FUNCTION: WIZ8 0x00536ad0
bool CanTargetMonster(int party_slot, int location_id, int allow_single_target, int reason)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0x1ce, TARGETING_CPP, location_id, 1));
    W8MonsterRecord* record;
    int action;
    int detail;
    W8ActionDetailBlock* detail_block;
    char needed;

    if (monster_info->fActive == 0) {
        return 0;
    }
    if (monster_info->hp_current == 0) {
        return 0;
    }
    if (monster_info->uiCondition[W8_CONDITION_REACHABLE_WHEN_DOWN] != 0) {
        return 0;
    }
    if (gXStatus.fCampMode != 0 && g_screen_state_00649f1c->target_location_id_f8 != location_id) {
        return 0;
    }

    if (ResolveTargetingContext(party_slot, W8_TARGETING_CONTEXT_CURRENT) == 0) {
        needed = 0;
        if (gXStatus.fCombatMode != 0) {
            return 0;
        }
    } else {
        ChooseCombatAction(party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0,
                           &detail_block);
        needed = GetTargetNeededForAction(action, detail, detail_block);

        if (needed != 2 && needed != 5) {
            if (needed == 1) {
                if (allow_single_target == 0) {
                    return 0;
                }
            } else {
                if (needed != 0) {
                    return 0;
                }
                if (gXStatus.fCombatMode != 0) {
                    return 0;
                }
            }
        }
    }

    record = GetMonsterDataForInfo(monster_info);
    if (record->untargetable_24a != 0 && needed != 0) {
        return 0;
    }
    if (needed == 5) {
        return IsSlotInRangeOfGroup(party_slot, monster_info->monster_group_id,
                                    W8_TARGETING_CONTEXT_CURRENT, reason) != 0;
    }
    return CanPartyMemberAimAtMonster(party_slot, 2, monster_info, W8_TARGETING_CONTEXT_CURRENT,
                                      reason) != 0;
}

/* Whether a party slot's chosen action has anything at all to aim at. Each
   action asks its own question: an attack looks for one targetable monster, a
   fifth-kind action looks at the other party members first and then at the
   monsters, and casting or using an item hands the question to the spell.

   An action this does not know about answers yes, so the check narrows rather
   than gates - only the actions with a way of being impossible can fail it.
   The two dialogue-driven actions answer yes as well until the dialogue has
   settled, since until then there is no spell or item to ask about. */
// FUNCTION: WIZ8 0x0053cdf0
bool SlotHasAnyValidTarget(int party_slot)
{
    int action;
    int detail;
    W8ActionDetailBlock* detail_block;
    W8CombatSlot target;
    unsigned int index;
    unsigned int other_slot;

    ChooseCombatAction(party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0,
                       &detail_block);

    switch (action) {
    case 0:
    case 1:
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            if (CanTargetMonster(party_slot,
                                 MonsterGetScriptPartByLocationIndex(index)->location_id, 0, 0)) {
                return 1;
            }
        }
        return 0;

    case 5:
        for (other_slot = 0; other_slot < 8; ++other_slot) {
            if (other_slot == (unsigned int)party_slot) {
                continue;
            }
            memset(&target, 0, sizeof(target));
            target.iMonsterID = BAD_INDEX;
            target.iGroupID = BAD_INDEX;
            target.iType = W8_TARGET_KIND_CHARACTER;
            target.iChar = other_slot;
            if (CharacterHasAttackOn(party_slot, &target)) {
                return 1;
            }
        }
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            memset(&target, 0, sizeof(target));
            target.iChar = BAD_INDEX;
            target.iGroupID = BAD_INDEX;
            target.iType = W8_TARGET_KIND_MONSTER;
            target.iMonsterID = MonsterGetScriptPartByLocationIndex(index)->location_id;
            if (CharacterHasAttackOn(party_slot, &target)) {
                return 1;
            }
        }
        return 0;

    case 7:
        if (g_level_block->selection_settled != 0) {
            return SpellHasAnyValidTarget(party_slot, detail, 0);
        }
        break;

    case 8:
        if (g_level_block->selection_settled != 0) {
            const W8ItemInstance* item = detail_block->item_use.item;

            if (item == 0) {
                return 0;
            }
            return SpellHasAnyValidTarget(
                party_slot, g_item_records[item->iItemNo].spell_id,
                ItemClassNormalizesTarget(&g_item_records[item->iItemNo]));
        }
        break;
    }
    return 1;
}

/* gXStatus.plsMonsterGroupList: every monster group in the level. */

/* The spell target kinds this has an opinion about. Everything else is
   answered yes outright, so the question only ever narrows. */
enum {
    W8_SPELL_TARGET_ONE_MONSTER = 2,
    W8_SPELL_TARGET_MONSTER_GROUP = 5,
    W8_SPELL_TARGET_DEAD_CHARACTER = 7
};

/* Whether a spell has anything to be cast at. What it needs decides the sweep:
   one monster looks for a targetable one, a group looks for a targetable
   group, and the raise-the-dead kind looks for a party member who is down but
   still reachable - occupied, out of hit points, carrying the eighteenth
   condition and not the nineteenth, which is the same pair IsTargetStillPresent
   reads for its indirect character kind.

   Out of combat the one-monster kind answers yes without looking, since
   anything in the level can be walked up to. */
// FUNCTION: WIZ8 0x0053d010
bool SpellHasAnyValidTarget(int party_slot, int spell_id, unsigned char normalize)
{
    unsigned int index;

    switch (GetTargetNeededForSpellFriendly(spell_id, normalize, W8_TARGETING_CONTEXT_CURRENT)) {
    case W8_SPELL_TARGET_ONE_MONSTER:
        if (g_settings_6850c8.autoscroll_combat_messages != 0) {
            return 1;
        }
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            if (CanTargetMonster(party_slot,
                                 MonsterGetScriptPartByLocationIndex(index)->location_id, 0, 0)) {
                return 1;
            }
        }
        return 0;

    case W8_SPELL_TARGET_MONSTER_GROUP:
        for (index = 0; index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
            W8MonsterGroup* group = GetMonsterGroupByListIndex(index);

            if (group->flag_28 != 0 && CanTargetMonsterGroup(party_slot, group)) {
                return 1;
            }
        }
        return 0;

    case W8_SPELL_TARGET_DEAD_CHARACTER:
        for (index = 0; index < 8; ++index) {
            if (g_status_685170.buffers.XChar[index].fOccupied != 0 &&
                g_status_685170.buffers.Char[index].hp_current == 0 &&
                g_status_685170.buffers.Char[index].uiCondition[W8_CONDITION_REACHABLE_WHEN_DOWN] !=
                    0 &&
                g_status_685170.buffers.Char[index].uiCondition[W8_CONDITION_BEYOND_REACH] == 0) {
                return 1;
            }
        }
        return 0;

    default:
        return 1;
    }
}

/* One candidate in the angle sort: the screen angle to the monster and the
   monster itself. The angle leads so that the ordinary signed comparison sorts
   on it. */
struct W8GroupMemberByAngle {
    int angle;       /* 0x00 */
    int location_id; /* 0x04 */
}; /* 0x08 */

/* Step to the next member of a group, going round the party rather than
   through the list: the candidates are sorted by the angle from the party to
   each of them, and the one after whichever is currently picked is taken,
   wrapping at the end. With nothing picked yet the leftmost is taken.

   The angle is what makes this feel like cycling across the screen rather than
   jumping about, and it is why the sort buffer holds a pair per candidate
   rather than just the ids. */
// FUNCTION: WIZ8 0x005383e0
int SelectNextGroupMemberByAngle(const W8GrowableVector<int>* candidates, int current)
{
    unsigned int count = (unsigned int)candidates->GetCount();
    W8GroupMemberByAngle* sorted;
    srVector3T<float> party;
    unsigned int index;
    int result;

    if (count == 0) {
        return BAD_INDEX;
    }
    GetCameraPosition(&party);

    sorted = (W8GroupMemberByAngle*)malloc(count * sizeof(W8GroupMemberByAngle));
    if (sorted == 0) {
        srAssertFail("pSortBuffer != NULL", TARGETING_CPP, 0x499, 0);
    }

    for (index = 0; index < count; ++index) {
        int location_id = *candidates->GetAt(static_cast<int>(index));
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x4a1, TARGETING_CPP, location_id, 1));
        srVector3T<float> position = monster_info->p3D->GetPosition();

        sorted[index].location_id = location_id;
        sorted[index].angle = (int)NormalizeAngle(GetHeadingAngle(&party, &position));
    }
    qsort(sorted, count, sizeof(W8GroupMemberByAngle), CompareSignedAscending);

    result = sorted[0].location_id;
    if (current != BAD_INDEX) {
        for (index = 0; index < count; ++index) {
            if (sorted[index].location_id == current) {
                result = sorted[index + 1 < count ? index + 1 : 0].location_id;
                break;
            }
        }
    }
    free(sorted);
    return result;
}

/* Which member of a group the party should pick out next. Every member the
   slot could aim at becomes a candidate - including the single-target case,
   which is what the third argument allows - and the group's own record of
   where it got to decides which of them comes next. */
/* Which monster the party should pick out next when the action wants a single
   monster. Every live monster the slot could aim at becomes a candidate and
   the angular sweep steps on from whichever monster is currently picked. */
// FUNCTION: WIZ8 0x00538140
int PickNextTargetableMonster(int party_slot)
{
    W8GrowableVector<int> targetable;
    unsigned int index;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

        if (CanTargetMonster(party_slot, monster_info->location_id, 1, 0) != 0) {
            targetable.Add(monster_info->location_id);
        }
    }
    return SelectNextGroupMemberByAngle(&targetable, gXStatus.picked_monster);
}

// FUNCTION: WIZ8 0x00538280
int PickNextTargetableGroupMember(int party_slot, W8MonsterGroup* group)
{
    W8GrowableVector<int> targetable;
    unsigned int index;

    for (index = 0; index < ILLength(group->monsters); ++index) {
        int location_id = IListGetAt(group->monsters, index);

        MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x46e, TARGETING_CPP, location_id, 1));
        if (CanTargetMonster(party_slot, location_id, 1, 0)) {
            targetable.Add(location_id);
        }
    }
    return SelectNextGroupMemberByAngle(&targetable, group->highlighted_member);
}

/* Point one party slot at a monster group, or at one monster inside it. An
   action that wants the whole group is aimed at the group and nothing else
   happens; anything else steps to the next targetable member and moves the
   highlight to it.

   The step is taken three times over: once to find where to go, once - before
   the group's record is updated - to find where the highlight currently is so
   it can be put out, and once after to find it again so it can be lit. That
   the same call answers differently each time is exactly what the group's own
   record of where it got to is for. */
// FUNCTION: WIZ8 0x00537b00
void AimAtMonsterGroupMember(int party_slot, W8MonsterGroup* group)
{
    W8CombatSlot target;
    W8TargetSource source;
    int action;
    int detail;
    W8ActionDetailBlock* detail_block;
    int picked;
    int previous;

    if (ResolveTargetingContext(party_slot, W8_TARGETING_CONTEXT_CURRENT) != 0) {
        ChooseCombatAction(party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0,
                           &detail_block);
        if (GetTargetNeededForAction(action, detail, detail_block) == 5) {
            memset(&target, 0, sizeof(target));
            target.iChar = BAD_INDEX;
            target.iMonsterID = BAD_INDEX;
            target.iType = W8_TARGET_KIND_GROUP;
            target.iGroupID = group->group_id;
            AimAtTarget(party_slot, &target, W8_TARGETING_CONTEXT_CURRENT);
            StartBreathCycle(party_slot, 0);
            SetTargetSourceToCharacter(party_slot, &source);
            NoteTargetChosen(&source,
                             GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT));
            return;
        }
    }

    picked = PickNextTargetableGroupMember(party_slot, group);
    if (picked == BAD_INDEX) {
        return;
    }

    memset(&target, 0, sizeof(target));
    target.iChar = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = W8_TARGET_KIND_MONSTER;
    target.iMonsterID = picked;
    AimAtTarget(party_slot, &target, W8_TARGETING_CONTEXT_CURRENT);

    previous = PickNextTargetableGroupMember(party_slot, group);
    if (previous != BAD_INDEX) {
        SetMonsterHighlightColour(GetMonsterByLocationID(previous), 0.0f, 0.0f, 0.0f, 0.0f);
    }

    group->highlighted_member = picked;

    picked = PickNextTargetableGroupMember(party_slot, group);
    if (picked != BAD_INDEX) {
        SetMonsterHighlightColour(GetMonsterByLocationID(picked), 1.0f, 1.0f, 1.0f, 1.0f);
    }
    g_level_block->pick_changed_154 = 1;

    StartBreathCycle(party_slot, 0);
    SetTargetSourceToCharacter(party_slot, &source);
    NoteTargetChosen(&source, GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT));
}

/* 0x006840B7: the group the party currently has picked out, by id, and -1 when
   none. It is where the sweep below starts from and wraps back to. The
   declaration lives in targeting.h (C linkage); the definition is with the
   other targeting globals near the top of this file. */

/* Which monster group the party should pick out next. The sweep starts at the
   one after whichever is currently picked and goes round until it comes back
   to where it began, so the same group is only offered again when nothing else
   will do. A picked group that has since gone away resets the starting point
   rather than ending the sweep. */
// FUNCTION: WIZ8 0x00537ed0
int PickNextTargetableGroup(int party_slot)
{
    unsigned int count = PLLength(gXStatus.plsMonsterGroupList);
    unsigned int start;
    unsigned int index;

    if (count == 0) {
        return BAD_INDEX;
    }

    if (gXStatus.picked_group == BAD_INDEX) {
        start = 0;
    } else {
        start = GetMonsterGroupIndexByID(0x414, TARGETING_CPP, gXStatus.picked_group, 0);
        if (start == 0xffffffff) {
            start = 0;
            gXStatus.picked_group = BAD_INDEX;
        } else {
            ++start;
            if (start == PLLength(gXStatus.plsMonsterGroupList)) {
                start = 0;
            }
        }
    }

    index = start;
    do {
        W8MonsterGroup* group = GetMonsterGroupByListIndex(index);

        if (CanTargetMonsterGroup(party_slot, group)) {
            return group->group_id;
        }
        ++index;
        if (index == PLLength(gXStatus.plsMonsterGroupList)) {
            index = 0;
        }
    } while (index != start);

    return BAD_INDEX;
}

/* The cycle-target key: step the party's pick to the next thing the slot's
   action can aim at. An action that needs a whole group steps between groups
   and remembers the new pick; anything else steps between monsters. Nothing
   targetable leaves the current pick alone. Either way the new aim is
   committed and the slot starts breathing again. */
// FUNCTION: WIZ8 0x00537D20
void CycleToNextTarget(int party_slot)
{
    W8CombatSlot target;
    W8TargetSource source;
    W8ActionDetailBlock* detail_block;
    W8TargetingContext context;
    int needed;
    int action;
    int detail;
    int pick;

    context = GetCurrentTargetingContext(party_slot);
    if (ResolveTargetingContext(party_slot, context) != 0) {
        ChooseCombatAction(party_slot, W8_TARGETING_CONTEXT_CURRENT, &action, &detail, 0,
                           &detail_block);
        switch (action) {
        case 7:
            needed = GetTargetNeededForSpellFriendly(detail, 0, W8_TARGETING_CONTEXT_CURRENT);
            break;
        case 8:
            needed = GetTargetNeededForItem(detail_block->item_use.item);
            break;
        default:
            goto pick_monster;
        }
        if (needed == 5) {
            pick = PickNextTargetableGroup(party_slot);
            if (pick == BAD_INDEX) {
                return;
            }
            memset(&target, 0, sizeof(target));
            target.iMonsterID = BAD_INDEX;
            target.iChar = BAD_INDEX;
            target.iType = W8_TARGET_KIND_GROUP;
            target.iGroupID = pick;
            AimAtTarget(party_slot, &target, W8_TARGETING_CONTEXT_CURRENT);
            gXStatus.picked_group = pick;
            goto finish;
        }
    }
pick_monster:
    pick = PickNextTargetableMonster(party_slot);
    if (pick == BAD_INDEX) {
        return;
    }
    memset(&target, 0, sizeof(target));
    target.iChar = BAD_INDEX;
    target.iGroupID = BAD_INDEX;
    target.iType = W8_TARGET_KIND_MONSTER;
    target.iMonsterID = pick;
    AimAtTarget(party_slot, &target, W8_TARGETING_CONTEXT_CURRENT);
    gXStatus.picked_monster = pick;
finish:
    StartBreathCycle(party_slot, 0);
    SetTargetSourceToCharacter(party_slot, &source);
    PointCameraAtCombatTarget(&source,
                              GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT));
    g_level_block->pick_changed_154 = 1;
}

/* Re-evaluate every party slot's combat target after a sight or range change:
   re-run the two context switches, keep the current target block when neither
   applies, and choose the fallback action when the slot is eligible. */
// FUNCTION: WIZ8 0x0053bf80
void RefreshAllPartyTargets(void)
{
    for (int party_slot = 0; party_slot < W8_PARTY_SLOT_COUNT; ++party_slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
        W8Character* character = &g_status_685170.buffers.Char[party_slot];

        if (row->fOccupied != 0 &&
            (character->hp_current != 0 || character->highest_condition < W8_CONDITION_DEAD)) {
            W8CombatSlot* target =
                GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT);
            bool can_switch = CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_CURRENT, 0, 0);

            if (can_switch) {
                RefreshCombatTargetHighlights(party_slot, target);
            } else {
                can_switch = CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_CURRENT, 1, 0);
                if (can_switch) {
                    RepickActionTarget(party_slot, W8_TARGETING_CONTEXT_CURRENT, 0);
                } else if (target->iType != W8_TARGET_KIND_NONE) {
                    W8CombatSlot action;

                    memset(&action, 0, sizeof(action));
                    action.iChar = BAD_INDEX;
                    action.iMonsterID = BAD_INDEX;
                    action.iGroupID = BAD_INDEX;
                    AimAtTarget(party_slot, &action, W8_TARGETING_CONTEXT_CURRENT);
                }
            }

            if (IsPartySlotEligible00524A10(party_slot) != 0 &&
                (row->action_03d == W8_ACTION_ATTACK || row->action_03d == W8_ACTION_BERSERK) &&
                (row->flag_105 != 0 ||
                 CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0, 0) != 0)) {
                int group_id = -1;

                if (row->target_in_combat.iMonsterID != -1) {
                    unsigned int monster_index = MonsterGetIndexByLocationID(
                        0xf40, TARGETING_CPP, row->target_in_combat.iMonsterID, 0);

                    if (monster_index != 0xffffffff) {
                        group_id =
                            MonsterGetScriptPartByLocationIndex(monster_index)->monster_group_id;
                    }
                }
                int selected = ChooseFallbackMonsterTarget0053C990(party_slot, group_id,
                                                                   W8_TARGETING_CONTEXT_IN_COMBAT);

                if (selected != -1) {
                    W8CombatSlot action;

                    memset(&action, 0, sizeof(action));
                    action.iType = W8_TARGET_KIND_MONSTER;
                    action.iChar = BAD_INDEX;
                    action.iMonsterID = selected;
                    action.iGroupID = BAD_INDEX;
                    AimAtTarget(party_slot, &action, W8_TARGETING_CONTEXT_IN_COMBAT);
                }
            }
        }
    }
}

/* Combat-panel refresh: recount each active group's on-screen and selectable
   members; when either count changed, ask for a party redraw and store the new
   pair in the group's scratch slot. */
// FUNCTION: WIZ8 0x005398D0
void RefreshMonsterTargetCounts005398D0(void)
{
    srVector3T<float> camera;
    srVector3T<float> lower;
    srVector3T<float> upper;
    unsigned int group_index;

    for (group_index = 0; group_index < PLLength(gXStatus.plsMonsterGroupList); ++group_index) {
        W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);
        int on_screen_count;
        int selectable_count;

        if (group->flag_28 == 0) {
            continue;
        }
        on_screen_count = 0;
        selectable_count = 0;
        for (unsigned int index = 0; index < ILLength(group->monsters); ++index) {
            int location_id = IListGetAt(group->monsters, index);
            unsigned int monster_index =
                MonsterGetIndexByLocationID(0x81a, TARGETING_CPP, location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);

            if (monster_info->p3D->IsWithinWorldRange004CA2A0() == 0) {
                continue;
            }
            if (monster_info == 0) {
                srAssertFail("pMonsterInfo", TARGETING_CPP, 0x7f8, 0);
            }
            GetCameraPosition(&camera);
            MonsterGetWorldAnimationBounds004CA4F0(monster_info->p3D, &lower, &upper);
            if (ShowTargetMarker(&camera, &lower, &upper) != 0) {
                on_screen_count += 1;
            }
            if (g_status_685170.selected_character == -1 ||
                CanPartyMemberAimAtMonster(g_status_685170.selected_character, 2, monster_info, 6,
                                           0) != 0) {
                selectable_count += 1;
            }
        }
        if (on_screen_count != group->visible_member_count ||
            selectable_count != group->selectable_member_count) {
            RequestRedrawParty();
            group->visible_member_count = on_screen_count;
            group->selectable_member_count = selectable_count;
        }
    }
}

/* Whether `monster` is closer than `max_distance` to the player and its
   elevated centre or either animation-bound corner still projects on the world
   camera. Callers pass a camera `position`, but retail distance uses
   GetDistanceToPlayer. */
// FUNCTION: WIZ8 0x0053A060
bool IsMonsterVisibleWithinDistance0053A060(W8Monster* monster, const srVector3T<float>* position,
                                            float max_distance)
{
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    srVector3T<double> input;
    srVector3T<float> center;
    srVector3T<float> projected;

    if (monster->GetDistanceToPlayer004C7CB0() < max_distance) {
        monster->GetAnimationBounds(&minimum, &maximum);
        center = monster->movement_0c0.position_040;
        center.y += monster->movement_0c0.height_offset_0b8;
        projected = monster->GetPosition();
        minimum.x += projected.x;
        minimum.y += projected.y;
        minimum.z += projected.z;
        maximum.x += projected.x;
        maximum.y += projected.y;
        maximum.z += projected.z;
        input.SetFromFloat(&center);
        if (g_world->camera->project(projected, input) !=
            srCamera::PROJECTION_RESULT_POSITIONAL_0) {
            input.SetFromFloat(&minimum);
            if (g_world->camera->project(projected, input) !=
                srCamera::PROJECTION_RESULT_POSITIONAL_0) {
                input.SetFromFloat(&maximum);
                if (g_world->camera->project(projected, input) !=
                    srCamera::PROJECTION_RESULT_POSITIONAL_0) {
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

/* Any live monster visible to the camera within the far-clip range, resuming
   the scan at the last match. */
// FUNCTION: WIZ8 0x0053A1D0
bool AnyMonsterVisible0053A1D0(void)
{
    srVector3T<float> camera;
    float limit;
    int count;
    int index;

    limit = (float)WorldGetFarClip(GetWorld()) * g_float_005ec3b8;
    if (g_world == 0 || g_world->camera == 0) {
        return 0;
    }
    GetCameraPosition(&camera);
    count = PLLength(gXStatus.plsMonsterList);
    if (0 <= g_last_visible_monster_0061d14c && g_last_visible_monster_0061d14c < count) {
        W8MonsterInfo* monster_info =
            (W8MonsterInfo*)PLGet(gXStatus.plsMonsterList, g_last_visible_monster_0061d14c);
        if (monster_info->p3D != 0 &&
            IsMonsterVisibleWithinDistance0053A060(monster_info->p3D, &camera, limit) != 0) {
            return 1;
        }
    }
    for (index = 0; index < count; ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PLGet(gXStatus.plsMonsterList, index);

        if (monster_info->p3D != 0 &&
            IsMonsterVisibleWithinDistance0053A060(monster_info->p3D, &camera, limit) != 0) {
            g_last_visible_monster_0061d14c = index;
            return 1;
        }
    }
    return 0;
}

/* Mode-3 targeting tick: pop the front marker off the queue and tint the
   monster green when it has line of sight from the target point. */
// FUNCTION: WIZ8 0x0053B1D0
void UpdateTargetMarkerHighlight0053B1D0(void)
{
    W8MonsterRuntimeBlock4C block;
    srVector3T<float> point;
    if (gXStatus.target_markers.GetCount() <= 0) {
        return;
    }
    {
        int location_id = gXStatus.target_markers.RemoveAt(0);
        W8Monster* monster;
        monster = GetMonsterByLocationID(location_id);
        point = gXStatus.target_position;
        if (monster->HasLineOfSightFromPoint004C4C40(point) != 0) {
            block.highlight_red = 0.0f;
            block.highlight_green = 1.0f;
            block.highlight_blue = 0.0f;
            block.highlight_alpha = 1.0f;
            MonsterSetRuntimeBlock4C(monster, block);
            return;
        }
        memset(&block, 0, sizeof(block));
        MonsterSetRuntimeBlock4C(monster, block);
    }
}

/* Pick the next targetable member of the group and paint it with the party
   slot's highlight color - clear for 0, green for 1, red for anything the
   callers pass as 2. */
// FUNCTION: WIZ8 0x00538510
void HighlightPickedGroupMember00538510(int party_slot, W8MonsterGroup* group, int color)
{
    W8ModelInstance3DRenderState block;

    int location_id = PickNextTargetableGroupMember(party_slot, group);
    if (location_id != -1) {
        W8Monster* monster = GetMonsterByLocationID(location_id);
        if (color == 0) {
            block.highlight_red = 0.0f;
            block.highlight_green = 0.0f;
            block.highlight_blue = 0.0f;
            block.highlight_alpha = 0.0f;
        } else if (color == 1) {
            block.highlight_red = 0.0f;
            block.highlight_green = 1.0f;
            block.highlight_blue = 0.0f;
            block.highlight_alpha = 1.0f;
        } else if (color == 2) {
            block.highlight_red = 1.0f;
            block.highlight_green = 0.0f;
            block.highlight_blue = 0.0f;
            block.highlight_alpha = 1.0f;
        }
        MonsterSetRuntimeBlock4C(monster, block);
    }
}

/* Aim the actor's context target block at `target`. When the aim actually
   changes the old highlights come down, the slot is redrawn, and an in-combat
   slot has its chosen action's matching target block updated to follow. */
// FUNCTION: WIZ8 0x005387f0
void AimAtTarget(int actor, W8CombatSlot* target, W8TargetingContext context)
{
    W8CombatSlot* block = GetTargetBlockForContext(actor, context);
    W8TargetingContext resolved = ResolveTargetingContext(actor, context);

    if (memcmp(block, target, sizeof(W8CombatSlot)) != 0) {
        if (context != W8_TARGETING_CONTEXT_OUT_OF_COMBAT) {
            W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[actor];
            if (entry->highlighted_monsters.GetCount() < 1) {
                if (block->iType == W8_TARGET_KIND_MONSTER && block->iMonsterID != -1) {
                    SetMonsterHighlight(actor, block->iMonsterID, 0);
                }
                if (block->iType == W8_TARGET_KIND_GROUP && block->iGroupID != -1) {
                    SetGroupHighlight(actor, block->iGroupID, 0);
                }
            } else {
                for (int index = 0; index < entry->highlighted_monsters.GetCount(); ++index) {
                    SetMonsterHighlight(actor, *entry->highlighted_monsters.GetAt(index), 0);
                }
                entry->highlighted_monsters.Clear();
            }
        }
        *block = *target;
        if (context != W8_TARGETING_CONTEXT_OUT_OF_COMBAT) {
            if (CharacterCanSwitchTo(actor, resolved, 0, 0) &&
                ResolveTargetingContext(actor, GetCurrentTargetingContext(actor)) == resolved) {
                RefreshCombatTargetHighlights(actor, target);
            }
            RequestPartySlotRedraw(actor);
            if (actor == g_status_685170.selected_character) {
                RequestRedrawParty();
                RequestRedraw(0x200000);
            }
            g_level_block->pick_changed_154 = 0;
        }
        if (resolved == W8_TARGETING_CONTEXT_IN_COMBAT && gXStatus.fCombatMode != 0 &&
            target->iType != W8_TARGET_KIND_NONE) {
            int action;
            int detail;
            W8CombatSlot* chosen_target;
            W8ActionDetailBlock* detail_block;
            W8PartySlotRow* row = &g_status_685170.buffers.XChar[actor];

            ChooseCombatAction(actor, W8_TARGETING_CONTEXT_IN_COMBAT, &action, &detail,
                               &chosen_target, &detail_block);
            if (action == W8_ACTION_CAST_SPELL) {
                row->spell_target = *target;
            } else if (action == W8_ACTION_USE_ITEM) {
                row->item_target = *target;
            } else if (action == W8_ACTION_BREATHE) {
                row->target_context_5 = *target;
            }
        }
    }
    if (target->iType != W8_TARGET_KIND_NONE && context != W8_TARGETING_CONTEXT_OUT_OF_COMBAT &&
        actor == g_status_685170.selected_character) {
        if (CharacterCanSwitchTo(actor, W8_TARGETING_CONTEXT_CURRENT, 1, 0)) {
            block = GetTargetBlockForContext(actor, W8_TARGETING_CONTEXT_CURRENT);
            CanPartySlotParticipate(actor);
            int needed;
            if (ResolveTargetingContext(actor, GetCurrentTargetingContext(actor)) ==
                W8_TARGETING_CONTEXT_OUT_OF_COMBAT) {
                needed = 0;
            } else {
                int action;
                int detail;
                W8CombatSlot* chosen_target;
                W8ActionDetailBlock* detail_block;

                ChooseCombatAction(actor, W8_TARGETING_CONTEXT_CURRENT, &action, &detail,
                                   &chosen_target, &detail_block);
                needed = GetTargetNeededForAction(action, detail, detail_block);
            }
            if (TargetMatchesNeeded(block, needed) == 0) {
                SetTargetingMode(needed);
                return;
            }
        }
        SetTargetingMode(0);
    }
}

/* Tint every member of a monster group with the party-slot highlight color.
   The debug format keeps the authored name ModifyGroupColor. */
// FUNCTION: WIZ8 0x005392e0
void ModifyGroupColor(int group_id, int color)
{
    W8ModelInstance3DRenderState block;

    if (group_id == -1) {
        srAssertFail("uiGroupID != BAD_INDEX", TARGETING_CPP, 0x6e5, 0);
    }
    unsigned int group_index = GetMonsterGroupIndexByID(0x6e8, TARGETING_CPP, group_id, 0);
    if (group_index == 0xffffffff) {
        if (color != 0) {
            FormatDebugMessage(1, "ERROR: ModifyGroupColor - invalid ID, groupID %d, color %d",
                               group_id, color);
        }
        return;
    }
    W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);
    if (color == 0) {
        block.highlight_red = 0.0f;
        block.highlight_green = 0.0f;
        block.highlight_blue = 0.0f;
        block.highlight_alpha = 0.0f;
    } else if (color == 1) {
        block.highlight_red = 0.0f;
        block.highlight_green = 1.0f;
        block.highlight_blue = 0.0f;
        block.highlight_alpha = 1.0f;
    } else if (color == 2) {
        block.highlight_red = 1.0f;
        block.highlight_green = 0.0f;
        block.highlight_blue = 0.0f;
        block.highlight_alpha = 1.0f;
    }
    for (unsigned int index = 0; index < ILLength(group->monsters); ++index) {
        W8Monster* monster = GetMonsterByLocationID(IListGetAt(group->monsters, index));
        MonsterSetRuntimeBlock4C(monster, block);
    }
}

/* Whether `source` can currently see `monster_info`: the party threat record's
   sight flag for a character, the mon-to-mon visibility record for a monster
   source, and a point line-of-sight probe for an indirect source. */
// FUNCTION: WIZ8 0x00539a30
static unsigned char SourceCanSeeMonster00539A30(const W8TargetSource* source,
                                                 W8MonsterInfo* monster_info,
                                                 unsigned char flag_index, int sight_flag)
{
    if (source->iType == W8_TARGET_SOURCE_CHARACTER) {
        if (source->iChar == -1) {
            srAssertFail("pSource->iChar != BAD_INDEX", TARGETING_CPP, 0xce3, 0);
        }
        return monster_info->party_threat.los_flags_05[flag_index];
    }
    if (source->iType == W8_TARGET_SOURCE_MONSTER) {
        if (source->iMonsterID == -1) {
            srAssertFail("pSource->iMonsterID != BAD_INDEX", TARGETING_CPP, 0xcf8, 0);
        }
        if (source->iMonsterID == -1) {
            srAssertFail("pSource->iMonsterID != -1", TARGETING_CPP, 0x84c, 0);
        }
        unsigned int monster_index =
            MonsterGetIndexByLocationID(0x84d, TARGETING_CPP, source->iMonsterID, 1);
        W8MonsterInfo* source_info = MonsterGetScriptPartByLocationIndex(monster_index);
        W8VisibilityRecord* visibility = FindMonToMonVisibility(source_info, monster_info);
        if (visibility != 0 && visibility->los_flags_05[sight_flag] != 0) {
            return 1;
        }
        return 0;
    }
    if (source->iType != W8_TARGET_SOURCE_INDIRECT) {
        srAssertFail("pSource->iType != SOURCE_TYPE_3D", TARGETING_CPP, 0x854, 0);
    }
    return monster_info->p3D->HasLineOfSightFromPoint004C4C40(source->point);
}

/* Whether `target` is inside the aim cone: within `bonus` + `eye_radius` of
   `eye`, and within the heading and elevation arc bounds. */
// FUNCTION: WIZ8 0x00539b70
bool TargetInRangeAndArcs00539B70(const srVector3T<float>* target, float bonus,
                                  const srVector3T<float>* eye, float eye_radius, float heading,
                                  float elevation)
{
    float dx = target->x - eye->x;
    float dy = target->y - eye->y;
    float dz = target->z - eye->z;
    float excess =
        static_cast<float>(floor(sqrt(dx * dx + dy * dy + dz * dz) - bonus - eye_radius));
    if (GetRangeConstant5EC35C() >= excess) {
        float angle = GetHeadingAngle(eye, target);
        if (NormalizeAngle(heading - angle) < g_float_005ec29c ||
            NormalizeAngle(angle - heading) < g_float_005ec29c) {
            angle = GetElevationAngle(eye, target);
            if (NormalizeAngle(elevation - angle) < g_float_005ec29c ||
                NormalizeAngle(angle - elevation) < g_float_005ec29c) {
                return true;
            }
        }
    }
    return false;
}

/* Append the location ids of live targetable monsters inside the aim cone to
   `found`. `disposition` filters ubDisposition, with 3 admitting every
   disposition; `sight_flag` picks the visibility flag the source must have. */
// FUNCTION: WIZ8 0x00539ca0
int CollectConeMonsterTargets00539CA0(const W8TargetSource* source, const srVector3T<float>* eye,
                                      float heading, float elevation, W8GrowableVector<int>* found,
                                      unsigned char disposition, int sight_flag)
{
    int start_count = found->GetCount();
    float radius;
    if (source->iType == W8_TARGET_SOURCE_CHARACTER) {
        radius = g_startup_world_659c0c->radius_084;
    } else if (source->iType == W8_TARGET_SOURCE_MONSTER) {
        W8MonsterInfo* source_info = MonsterInfoFromID(0x8bc, TARGETING_CPP, source->iMonsterID, 1);
        radius = source_info->p3D->radius_084;
    } else {
        radius = 0.0f;
    }
    W8MonsterInfo* monster_info = GetNextMonsterInfo(1);
    while (monster_info != 0) {
        if (monster_info->fActive != 0 && monster_info->hp_current != 0 &&
            monster_info->uiCondition[0x12] == 0) {
            W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
            if (record->untargetable_24a == 0 &&
                (monster_info->ubDisposition == disposition || disposition == 3)) {
                W8Monster* monster = monster_info->p3D;
                srVector3T<float> point;
                point.x = monster->movement_0c0.position_040.x;
                point.y =
                    monster->movement_0c0.position_040.y + monster->movement_0c0.height_offset_0b8;
                point.z = monster->movement_0c0.position_040.z;
                if (TargetInRangeAndArcs00539B70(&point, monster->radius_084, eye, radius, heading,
                                                 elevation) != 0 &&
                    SourceCanSeeMonster00539A30(source, monster_info, 1, sight_flag) != 0) {
                    found->Add(monster_info->location_id);
                }
            }
        }
        monster_info = GetNextMonsterInfo(0);
    }
    return found->GetCount() - start_count;
}

/* Set the targeting filter for the spell being aimed: build the context
   target for the spell's target type, aim the selected slot at it, then pick
   the cursor, cone and world-pause state the resulting mode needs. */
// FUNCTION: WIZ8 0x0053a440
void ConfigureSpellTargetFilter(int target_type, unsigned int needed_kind)
{
    W8CombatSlot target;
    int cursor = -1;
    int selected = g_status_685170.selected_character;

    switch (target_type) {
    case 0:
        ResetCombatSlot(&target);
        target.iType = W8_TARGET_KIND_CHARACTER;
        target.iChar = selected;
        break;
    case 2:
        ResetCombatSlot(&target);
        target.iType = W8_TARGET_KIND_PARTY;
        break;
    case 3:
        if (gXStatus.fCampMode != 0) {
            ResetCombatSlot(&target);
            target.iType = W8_TARGET_KIND_MONSTER;
            target.iMonsterID = g_screen_state_00649f1c->target_location_id_f8;
            AimAtTarget(selected, &target, W8_TARGETING_CONTEXT_CURRENT);
            needed_kind = 0;
        } else if (g_settings_6850c8.autotarget_spells != 0 &&
                   RepickActionTarget(selected, W8_TARGETING_CONTEXT_SHARED, 0)) {
            W8TargetSource source;
            SetTargetSourceToCharacter(selected, &source);
            PointCameraAtCombatTarget(&source,
                                      &g_status_685170.buffers.XChar[selected].target_in_combat);
        }
        goto aim_done;
    case 7:
        ResetCombatSlot(&target);
        target.iType = W8_TARGET_KIND_FIVE;
        break;
    case 10:
        ResetCombatSlot(&target);
        target.iType = W8_TARGET_KIND_EIGHT;
        break;
    default:
        ResetCombatSlot(&target);
        break;
    }
    AimAtTarget(selected, &target, W8_TARGETING_CONTEXT_CURRENT);
aim_done:
    gXStatus.iTargetingMode = needed_kind;
    switch (needed_kind) {
    case 1:
    case 6:
    case 7:
        cursor = 3;
        break;
    case 2:
        cursor = 0;
        break;
    case 3:
        cursor = 2;
        break;
    case 4:
        cursor = 0xc;
        break;
    case 5:
        cursor = 0xb;
        break;
    }
    if (cursor != gXStatus.iCurrentCursor) {
        SetTargetCursor(cursor);
    }
    gXStatus.target_position.SetZero();
    RequestRefreshPartyState();
    if (needed_kind == 4) {
        SetTargetConeEnabled004ADD30(1);
        PauseMainGameWorld();
        return;
    }
    SetTargetConeEnabled004ADD30(0);
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        ResumeMainGameWorld();
    }
}

/* Whether the pending item use needs a target picked: most spell target types
   need one, type 3 only when auto-targeting cannot pick it (never in camp). */
// FUNCTION: WIZ8 0x0053a770
bool ItemUseNeedsTarget0053A770(int party_slot)
{
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
    W8ItemInstance* item = FindCharacterItemAt(party_slot, row->item_origin, row->item_slot);
    unsigned char normalize = ItemClassNormalizesTarget(&g_item_records[item->iItemNo]);
    switch (GetSpellTargetType(GetItemSpell(item), normalize)) {
    case 0:
    case 2:
    case 7:
    case 10:
        return false;
    case 3:
        if (gXStatus.fCampMode != 0) {
            return false;
        }
        return g_settings_6850c8.autotarget_spells == 0;
    default:
        return true;
    }
}

/* Drop the party slot's per-monster highlight bits and take the interface out
   of targeting: cursor, target cone and world pause all return to normal, and
   in combat the character's own target highlights come back up. */
// FUNCTION: WIZ8 0x0053b050
void ClearSlotTargeting0053B050(int party_slot)
{
    W8PList* monster_list = gXStatus.plsMonsterList;
    for (unsigned int index = 0; index < PLLength(monster_list); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        W8Monster* monster = monster_info->p3D;
        if (monster_info->fActive != 0 && monster != 0) {
            unsigned char flag = MonsterGetRuntimeFlag5BC(monster);
            if ((flag & (1 << (party_slot & 0x1f))) != 0) {
                MonsterSetRuntimeFlag5BC(monster, ~(1 << (party_slot & 0x1f)) & flag);
                NotifyMonsterHighlight(party_slot, monster_info->location_id, 0);
            }
        }
    }
    gXStatus.iTargetingMode = 0;
    if (gXStatus.iCurrentCursor != -1) {
        SetTargetCursor(-1);
    }
    gXStatus.target_position.SetZero();
    RequestRefreshPartyState();
    SetTargetConeEnabled004ADD30(0);
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        ResumeMainGameWorld();
    }
    if (gXStatus.fCombatMode != 0 &&
        CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0, 0)) {
        RefreshCombatTargetHighlights(party_slot,
                                      &g_status_685170.buffers.XChar[party_slot].target_in_combat);
    }
}

/* The targeting context the slot's combat action runs in: the live context
   resolved through the validating switch. */
// FUNCTION: WIZ8 0x0053bc90
W8TargetingContext GetCombatActionContext0053BC90(int party_slot)
{
    return ResolveTargetingContext(party_slot, GetCurrentTargetingContext(party_slot));
}

/* Raise or clear the party slot's highlight bit on every active monster.
   Clearing while spell targeting is up repaints the spell's own marks. */
// FUNCTION: WIZ8 0x0053c130
void UpdateSlotMonsterHighlights0053C130(int party_slot, char enable)
{
    if ((party_slot != g_status_685170.selected_character) || (gXStatus.iTargetingMode == 0)) {
        W8PList* monster_list = gXStatus.plsMonsterList;
        for (unsigned int index = 0; index < PLLength(monster_list); ++index) {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
            W8Monster* monster = monster_info->p3D;
            if (monster_info->fActive != 0 && monster != 0) {
                unsigned char flag = MonsterGetRuntimeFlag5BC(monster);
                W8ModelInstance3DRenderState block;
                if (enable != 0 && (flag & (1 << (party_slot & 0x1f))) != 0) {
                    block.highlight_red = 1.0f;
                    block.highlight_green = 0.0f;
                    block.highlight_blue = 0.0f;
                    block.highlight_alpha = 1.0f;
                } else {
                    block.highlight_red = 0.0f;
                    block.highlight_green = 0.0f;
                    block.highlight_blue = 0.0f;
                    block.highlight_alpha = 0.0f;
                }
                MonsterSetRuntimeBlock4C(monster, block);
            }
        }
        RequestRedrawParty();
        if (enable == 0 && gXStatus.iTargetingMode == 4) {
            HighlightSpellTargetsAtCachedPosition();
        }
    }
}

/* Pick an attack fallback, allowing the character's alternate weapon set when
   the ordinary group selection has no usable monster. The swap is undone when
   it finds nothing, and the slot's own valid target is kept when it has one. */
// FUNCTION: WIZ8 0x0053c990
int ChooseFallbackMonsterTarget0053C990(int party_slot, int group_id, W8TargetingContext context)
{
    int result = -1;
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
    W8Character* character = &g_status_685170.buffers.Char[party_slot];

    if (g_settings_6850c8.autoswap_weapons != 0 &&
        gXStatus.monster_manager_entries[party_slot].item_swap_in_progress == 0 &&
        row->flag_0f5 == 0 &&
        (g_combat_state->flag_000 == 0 || g_combat_state->characters[party_slot].flag_34 == 0 ||
         g_combat_state->characters[party_slot].phase == 0) &&
        !IsItemBoundToWearer(&character->EquippedItem[8]) &&
        !IsItemBoundToWearer(&character->EquippedItem[9]) &&
        SwapWeaponSetSlots0051D3B0(party_slot, 0, 0) == 1) {
        result = ChooseMonsterTarget(party_slot, group_id, context);
        if (result == -1) {
            if (SwapWeaponSetSlots0051D3B0(party_slot, 0, 0) != 0) {
                return -1;
            }
            PostCharacterNotice(party_slot, gppStringList[0x26c]);
        } else {
            PostCharacterNotice(party_slot, gppStringList[0x26d]);
            W8TargetingContext resolved = GetValidatedTargetingContext(party_slot, context);
            W8CombatSlot* block = GetTargetBlockForContext(party_slot, resolved);
            if (block->iType == W8_TARGET_KIND_MONSTER) {
                unsigned int monster_index =
                    MonsterGetIndexByLocationID(0xf82, TARGETING_CPP, block->iMonsterID, 0);
                if (monster_index != 0xffffffff) {
                    W8MonsterInfo* monster_info =
                        MonsterGetScriptPartByLocationIndex(monster_index);
                    if (CanPartyMemberAimAtMonster(party_slot, 2, monster_info, context, 0)) {
                        result = block->iMonsterID;
                    }
                }
            }
        }
        row->flag_105 ^= 1;
        RefreshAfterItemRecordChange(&character->EquippedItem[6], character, 1);
        RefreshAfterItemRecordChange(&character->EquippedItem[7], character, 1);
    }
    return result;
}

/* When combat ends, swap each occupied, living slot whose pending swap flag is
   set back to its primary weapon set, provided auto-swap is on and nothing is
   already swapping. */
// FUNCTION: WIZ8 0x0053cd60
void ReconcilePartyEquipmentAfterCombat0053CD60(void)
{
    for (int party_slot = 0; party_slot < 8; ++party_slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
        W8Character* character = &g_status_685170.buffers.Char[party_slot];
        if (row->fOccupied != 0 &&
            (character->hp_current != 0 || character->highest_condition < 0xd) &&
            row->flag_105 != 0 && g_settings_6850c8.autoswap_weapons != 0 && row->flag_0f5 == 0) {
            SwapWeaponSetSlots0051D3B0(party_slot, 0, 1);
            row->flag_105 = 0;
        }
    }
}
