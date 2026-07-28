#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/* Local Code\Magic.cpp, named by the assertion this body embeds. */

// FUNCTION: WIZ8 0x004FF3B0
int GetProfessionCasterLevel(W8Character* character, int profession_id)
{
    int magic_level_offset;

    if (profession_id == -1) {
        profession_id = character->current_profession;
        if (profession_id == -1) {
            srAssertFail(
                "iProfession != -1",
                "C:\\Projects\\Wizardry 8\\Local Code\\Magic.cpp",
                0xe13,
                0);
        }
    }

    magic_level_offset = g_profession_magic_level_offsets[profession_id];
    if (magic_level_offset == -255) {
        return -1;
    }
    return character->profession_levels[profession_id] + magic_level_offset;
}

extern unsigned char Function51D610(int caster, int item_id);   /* 0x0051D610 */

/* The target type that costs three off the difficulty: whatever
   GetSpellTargetType answers seven for. */
enum { W8_SPELL_TARGET_DISCOUNTED = 7 };

/* How hard one spell is to bring off. Half the caster's own figure, plus the
   caller's bonus, plus half of the spell's level and half its point cost taken
   together - so the point cost counts a quarter and the level a half. One
   target type is three easier than the rest, and the answer never goes below
   zero. */
// FUNCTION: WIZ8 0x004FF790
int GetSpellDifficulty(unsigned int caster_figure, int spell_id, int bonus)
{
    int difficulty = (caster_figure >> 1) + bonus +
                     (g_spell_records[spell_id].spell_point_cost / 2 +
                      g_spell_records[spell_id].spell_level) / 2;

    if (GetSpellTargetType(spell_id, 0) == W8_SPELL_TARGET_DISCOUNTED) {
        difficulty -= 3;
    }
    if (difficulty < 0) {
        return 0;
    }
    return difficulty;
}

/* Whether one carried item can be cast from. It has to be of the spell-source
   kind, it has to be identified, and the caster has to be able to use it. */
// FUNCTION: WIZ8 0x00500010
bool CanCastFromItem(int caster, const W8ItemInstance* item)
{
    if (g_item_records[item->item_id].category != 3) {
        return false;
    }
    if (item->identified == 0) {
        return false;
    }
    return Function51D610(caster, item->item_id) != 0;
}
