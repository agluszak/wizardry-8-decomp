#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/combat_state.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/GameplayTime.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/magic.h"
#include "wiz8/xstatus.h"
#include "wiz8/character.h"
#include "wiz8/game_status.h"
#include "wiz8/item_instance.h"
#include "wiz8/screen_state.h"
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

    if (character->current_profession >= 0xf || character->race >= 0x10 ||
        character->gender == -1) {
        return;
    }
    if (character->level > 0x32) {
        character->level = 0x32;
    }
    for (index = 0; index < 0xf; ++index) {
        if ((unsigned int)character->profession_levels[index] > 0x32) {
            character->profession_levels[index] = 0x32;
        }
    }

    ResetCharacterAttributes005539E0(character);
    ResetCharacterSkills00553A60(character);
    RecalculateCharacterHitPoints(character);
    RecalculateCharacterStamina(character);
    RecalculateRealmSpellPoints(character);
    CalcArmorClasses(character);
    RebuildCharacterRegenRates00502B50(character);

    character->damage_reduction = 0;
    if (CharacterHasTrait00547940(character, 0x1d)) {
        character->damage_reduction += character->attributes[3].effective / 10;
    }
    if (CharacterHasTrait00547940(character, 6)) {
        character->damage_reduction +=
            (int)ScaleValueByProfessionLevel005479B0(character, 6, 30.0f);
    }
    if (character->skills[0x25].flag_00 != 0) {
        character->damage_reduction += (character->skills[0x25].level >> 2) + 5;
    }
    character->damage_reduction += character->bonus_1770.damage_reduction_adjustment;
    RecalculateCharacterResistances(character);

    int base = character->attributes[3].effective + character->attributes[0].effective * 2;
    unsigned int previous_capacity = character->carrying_capacity;
    unsigned int capacity = base * 0xc;
    if (CharacterHasTrait00547940(character, 0x18)) {
        capacity = capacity * 2 / 3;
    }
    bool changed = previous_capacity != capacity;
    character->carrying_capacity = capacity;
    bool recalculated = RecalculateCarriedWeight(character);
    if (g_status_685170.game_started == 0) {
        character->party_weight_share = 0;
    } else if (changed || recalculated) {
        RedistributePartyEncumbrance();
    }
    character->total_carried_weight = character->party_weight_share + character->inventory_weight;

    unsigned int load =
        (unsigned int)(character->total_carried_weight * 100) / character->carrying_capacity;
    if (load < 0x32) {
        character->load_category = 0;
    } else if (load < 0x46) {
        character->load_category = 1;
    } else if (load < 0x55) {
        character->load_category = 2;
    } else if (load <= 100) {
        character->load_category = 3;
    } else {
        character->load_category = 4;
    }

    CalcInitiative(character);
    CalcAttacks(character);
    CalcArmorClasses(character);
    if (character->in_party != 0 && g_current_screen_state.id != 3) {
        RequestPartySlotRedraw(CharacterPointerToPartySlot(character));
    }
}

/* Carrying capacity from strength and the carrying trait. The two attributes
   are the third and first effective values, the same pair the stamina
   recomputation reads. */
// FUNCTION: WIZ8 0x004edc10
bool RecalculateCarryingCapacity004EDC10(W8Character* character)
{
    unsigned int previous = character->carrying_capacity;
    int base = character->attributes[3].effective + character->attributes[0].effective * 2;
    unsigned int capacity = base * 0xc;
    if (CharacterHasTrait00547940(character, 0x18)) {
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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    unsigned int previous = character->inventory_weight;
    character->inventory_weight = 0;
    for (int index = 0; index < 0xc; ++index) {
        character->inventory_weight += GetItemStackWeight(&character->equipment[index]);
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
        QueueCharacterEvent(character, effect, 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
    }
    return previous != character->inventory_weight;
#pragma clang diagnostic pop
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
    W8Character* characters = g_status_685170.buffers.characters;
    W8PartySlotRow* active = g_status_685170.buffers.party_rows;

    if (!g_status_685170.game_started) {
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
        if (active[slot].occupied != 0 && character->highest_condition < 0x12) {
            capacity[slot] = character->carrying_capacity;
            unassigned[slot] = capacity[slot] - character->inventory_weight;
            load_ratio[slot] = (float)unassigned[slot] * 100.0f / (float)capacity[slot];
        }
    }

    unsigned int party_weight = 0;
    for (slot = 0; slot < (unsigned int)g_status_685170.party_item_count_1791; ++slot) {
        party_weight += GetItemStackWeight(&g_status_685170.party_item_pool_0021[slot]);
    }

    for (party_weight >>= 1; party_weight != 0; --party_weight) {
        unsigned int best_slot = (unsigned int)-1;
        float best_ratio = -999999.0f;
        for (slot = 0; slot < 8; ++slot) {
            W8Character* character = &characters[slot];
            if (active[slot].occupied != 0 && character->highest_condition < 0x12 &&
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
        load_ratio[best_slot] = (float)unassigned[best_slot] * 100.0f / (float)capacity[best_slot];
    }

    for (slot = 0; slot < 8; ++slot) {
        if (active[slot].occupied == 0) {
            continue;
        }
        W8Character* character = &characters[slot];
        int carried = character->inventory_weight + character->party_weight_share;
        character->total_carried_weight = carried;
        unsigned int percent =
            (unsigned int)(carried * 100) / (unsigned int)character->carrying_capacity;
        int old_band = character->load_category;
        if (percent < 50) {
            character->load_category = 0;
        } else if (percent < 70) {
            character->load_category = 1;
        } else if (percent < 85) {
            character->load_category = 2;
        } else {
            character->load_category = (percent > 100) + 3;
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
    } else if (g_current_screen_state.id == W8_SCREEN_CAMP && g_camp_screen_0069c0f4 != 0) {
        g_camp_screen_0069c0f4->redraw_flags |= 0x2100;
    }
    gXStatus.fEncumbranceDirty = false;
}
