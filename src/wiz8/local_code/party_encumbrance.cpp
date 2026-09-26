#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/GameplayTime.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/character.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"

/* The "carrying too much" camp event id. Outside camp the live special
   event slot supplies it instead. */
// GLOBAL: WIZ8 0x005ee5a8
int g_camp_overload_event_id = 8;

/* Party encumbrance redistribution. Live query: 0x004ED9D0 is a gap between
   Local Code\Combat.cpp (upper 0x004ED390) and Local Code\GameplayCode.cpp
   (lower 0x004EE000). Not a Combat tail and not an invisible named unit. */

/* The full derived-stat recompute: clamp the level and the fifteen profession
   levels, rebuild the attributes, skills and pools, the damage reduction, the
   resistances, the encumbrance and load category, and then the initiative,
   attacks and armor classes. A character whose profession, race or gender is
   still unset is left alone. */
// FUNCTION: WIZ8 0x004ed9d0
void RecalculateCharacterDerivedStats(W8Character* character)
{
    int index;

    if (character->iProfession >= 0xf || character->iRace >= 0x10 || character->gender == -1) {
        return;
    }
    if (character->uiExpLevel > 0x32) {
        character->uiExpLevel = 0x32;
    }
    for (index = 0; index < 0xf; ++index) {
        if ((unsigned int)character->profession_levels[index] > 0x32) {
            character->profession_levels[index] = 0x32;
        }
    }

    ResetCharacterAttributes(character);
    ResetCharacterSkills(character);
    RecalculateCharacterHitPoints(character);
    RecalculateCharacterStamina(character);
    RecalculateRealmSpellPoints(character);
    CalcArmorClasses(character);
    RebuildCharacterRegenRates(character);

    character->damage_reduction = 0;
    if (CharacterHasTrait(character, W8_TRAIT_DWARF_DAMAGE_RESISTANCE)) {
        character->damage_reduction += character->attributes[W8_ATTRIBUTE_VITALITY].effective / 10;
    }
    if (CharacterHasTrait(character, W8_TRAIT_MONK_DAMAGE_RESISTANCE)) {
        character->damage_reduction += static_cast<int>(
            ScaleValueByProfessionLevel(character, W8_TRAIT_MONK_DAMAGE_RESISTANCE, 15.0f));
    }
    if (character->skills[W8_SKILL_IRON_SKIN].active_00 != 0) {
        character->damage_reduction += (character->skills[W8_SKILL_IRON_SKIN].level >> 2) + 5;
    }
    character->damage_reduction += character->bonus_1770.damage_reduction_adjustment;
    RecalculateCharacterResistances(character);

    int base = character->attributes[W8_ATTRIBUTE_VITALITY].effective +
               character->attributes[W8_ATTRIBUTE_STRENGTH].effective * 2;
    unsigned int previous_capacity = character->carrying_capacity;
    unsigned int capacity = base * 0xc;
    if (CharacterHasTrait(character, W8_TRAIT_FAERIE_REDUCED_CARRY_CAPACITY)) {
        capacity = capacity * 2 / 3;
    }
    bool changed = previous_capacity != capacity;
    character->carrying_capacity = capacity;
    bool recalculated = RecalculateCarriedWeight(character);
    if (g_status.game_started == 0) {
        character->party_weight_share = 0;
    } else if (changed || recalculated) {
        RedistributePartyEncumbrance();
    }
    character->total_carried_weight = character->party_weight_share + character->inventory_weight;

    unsigned int load =
        (unsigned int)(character->total_carried_weight * 100) / character->carrying_capacity;
    if (load < 0x32) {
        character->load_category = W8_LOAD_NONE;
    } else if (load < 0x46) {
        character->load_category = W8_LOAD_LIGHT;
    } else if (load < 0x55) {
        character->load_category = W8_LOAD_MEDIUM;
    } else if (load <= 100) {
        character->load_category = W8_LOAD_HEAVY;
    } else {
        character->load_category = W8_LOAD_EXTREME;
    }

    CalcInitiative(character);
    CalcAttacks(character);
    CalcArmorClasses(character);
    if (character->fInParty != 0 && g_current_screen_state.id != 3) {
        RequestPartySlotRedraw(CharacterPointerToPartySlot(character));
    }
}

/* Carrying capacity from strength and the carrying trait. The two attributes
   are the third and first effective values, the same pair the stamina
   recomputation reads. */
// FUNCTION: WIZ8 0x004edc10
bool RecalculateCarryingCapacity(W8Character* character)
{
    unsigned int previous = character->carrying_capacity;
    int base = character->attributes[W8_ATTRIBUTE_VITALITY].effective +
               character->attributes[W8_ATTRIBUTE_STRENGTH].effective * 2;
    unsigned int capacity = base * 0xc;
    if (CharacterHasTrait(character, W8_TRAIT_FAERIE_REDUCED_CARRY_CAPACITY)) {
        capacity = (unsigned int)(base * 0x18) / 3;
    }
    character->carrying_capacity = capacity;
    return previous != capacity;
}

/* Sum the stack weights of everything the character carries. Crossing the
   carrying capacity in the old-to-new direction raises the overloaded
   notice. */
// FUNCTION: WIZ8 0x004edc60
bool RecalculateCarriedWeight(W8Character* character)
{
    unsigned int previous = character->inventory_weight;
    character->inventory_weight = 0;
    for (int index = 0; index < W8_EQUIP_SLOT_COUNT; ++index) {
        character->inventory_weight += GetItemStackWeight(&character->EquippedItem[index]);
    }
    for (int backpack_index = 0; backpack_index < 8; ++backpack_index) {
        character->inventory_weight += GetItemStackWeight(&character->backpack[backpack_index]);
    }
    if (previous < character->carrying_capacity &&
        character->carrying_capacity < character->inventory_weight) {
        int effect = g_camp_overload_event_id;
        if (g_current_screen_state.id != W8_SCREEN_CAMP) {
            effect = g_special_event_0068c558;
        }
        QueueCharacterEvent(character, effect, 0, g_character_event_no_flags,
                            g_character_event_full_volume);
    }
    return previous != character->inventory_weight;
}

/* Split the shared party-pool weight across eligible members by remaining
   carrying capacity, then refresh each member's load band. Combat defers the
   work by raising the pending-redistribution flag instead. */
// FUNCTION: WIZ8 0x004edd20
void RedistributePartyEncumbrance(void)
{
    int capacity[8];
    int unassigned[8];
    float load_ratio[8];
    W8Character* characters = g_status.buffers.Char;
    W8PartySlotRow* active = g_status.buffers.XChar;

    if (!g_status.game_started) {
        return;
    }
    if (gXStatus.fCombatMode) {
        gXStatus.fEncumbranceDirty = true;
        return;
    }

    unsigned int slot;
    for (slot = 0; slot < 8; ++slot) {
        W8Character* character = &characters[slot];
        character->party_weight_share = 0;
        if (active[slot].fOccupied != 0 && character->highest_condition < W8_CONDITION_DEAD) {
            capacity[slot] = character->carrying_capacity;
            unassigned[slot] = capacity[slot] - character->inventory_weight;
            load_ratio[slot] = unassigned[slot] * 100.0f / capacity[slot];
        }
    }

    unsigned int party_weight = 0;
    for (slot = 0; slot < g_status.party_item_count_1791; ++slot) {
        party_weight += GetItemStackWeight(&g_status.party_item_pool_0021[slot]);
    }

    for (party_weight >>= 1; party_weight != 0; --party_weight) {
        unsigned int best_slot = (unsigned int)-1;
        float best_ratio = -999999.0f;
        for (slot = 0; slot < 8; ++slot) {
            W8Character* character = &characters[slot];
            if (active[slot].fOccupied != 0 && character->highest_condition < W8_CONDITION_DEAD &&
                load_ratio[slot] > best_ratio) {
                best_ratio = load_ratio[slot];
                best_slot = slot;
            }
        }
        if (best_slot == (unsigned int)-1) {
            return;
        }
        W8Character* character = &characters[best_slot];
        ++character->party_weight_share;
        --unassigned[best_slot];
        load_ratio[best_slot] = unassigned[best_slot] * 100.0f / capacity[best_slot];
    }

    for (slot = 0; slot < 8; ++slot) {
        if (active[slot].fOccupied == 0) {
            continue;
        }
        W8Character* character = &characters[slot];
        int carried = character->inventory_weight + character->party_weight_share;
        character->total_carried_weight = carried;
        unsigned int percent =
            static_cast<unsigned int>(carried * 100) / character->carrying_capacity;
        int old_band = character->load_category;
        if (percent < 50) {
            character->load_category = W8_LOAD_NONE;
        } else if (percent < 70) {
            character->load_category = W8_LOAD_LIGHT;
        } else if (percent < 85) {
            character->load_category = W8_LOAD_MEDIUM;
        } else {
            character->load_category = (percent > 100) + W8_LOAD_HEAVY;
        }
        if (old_band != character->load_category) {
            CalcInitiative(character);
            CalcAttacks(character);
            CalcArmorClasses(character);
        }
    }

    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        if (gXStatus.fNpcDialogueMode == 0) {
            RequestRedraw(0xff);
            gXStatus.fEncumbranceDirty = false;
            return;
        }
    } else if (g_current_screen_state.id == W8_SCREEN_CAMP && g_camp_screen != 0) {
        g_camp_screen->redraw_flags |= 0x2100;
    }
    gXStatus.fEncumbranceDirty = false;
}
