#pragma once

/* Condition duration indices; the icon path maps live condition N to icon N-1. */
enum W8Condition {
    W8_CONDITION_NONE = 0,
    W8_CONDITION_DRAINED = 1,
    W8_CONDITION_DISEASED = 2,
    W8_CONDITION_IRRITATED = 3,
    W8_CONDITION_NAUSEATED = 4,
    W8_CONDITION_SLOWED = 5,
    W8_CONDITION_AFRAID = 6,
    W8_CONDITION_POISONED = 7,
    W8_CONDITION_SILENCED = 8,
    W8_CONDITION_HEXED = 9,
    W8_CONDITION_INFATUATED = 10,
    W8_CONDITION_INSANE = 11,
    W8_CONDITION_BLIND = 12,
    W8_CONDITION_TURNCOAT = 13,
    W8_CONDITION_WEBBED = 14,
    W8_CONDITION_ASLEEP = 15,
    W8_CONDITION_PARALYZED = 16,
    W8_CONDITION_UNCONSCIOUS = 17,
    W8_CONDITION_DEAD = 18,
    W8_CONDITION_MISSING = 19,
};

/* Counts and duration values are separate from condition identities. */
enum { W8_CONDITION_COUNT = 20, W8_CONDITION_CLEARABLE_COUNT = 18, W8_CONDITION_INDEFINITE = 9999 };

/* BindMonsterToCharacterDependence's DEPEND_COND_COUNT domain. Summon Elemental
   binds slot zero; the swallow attack binds slot one and applies MISSING. These
   index the separate binding records, not the condition-duration array. */
enum W8CharacterDependence {
    W8_DEPENDENCE_SUMMON = 0,
    W8_DEPENDENCE_SWALLOWED = 1,
    W8_DEPENDENCE_COUNT = 2
};

/* Enchantment N maps to monster icon N+16. Slot zero is the empty/top-scan sentinel. */
enum W8EnchantmentSlot {
    W8_ENCHANTMENT_NONE = 0,
    W8_ENCHANTMENT_DRACON_BREATH = 1,
    W8_ENCHANTMENT_GUARDIAN_ANGEL = 2,
    W8_ENCHANTMENT_RAZOR_CLOAK = 3,
    W8_ENCHANTMENT_EYE_FOR_AN_EYE = 4,
    W8_ENCHANTMENT_HASTE = 5,
    W8_ENCHANTMENT_SUPERMAN = 6,
    W8_ENCHANTMENT_BODY_OF_STONE = 7,
};
