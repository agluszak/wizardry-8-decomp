#include "wiz8/engine_code/Camera.h"
#include "wiz8/local_screens/Screens.h"
#include "soundman.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/npc_script_file.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/SpellEffect.h"
#include "wiz8/sr_api.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/utility.h"
#include "wiz8/sound_man.h"
#include "random.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/music_playlist.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/string_database.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/npc_interaction.h"
#include "timer.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/dialog_code/PortraitQuote.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/notices.h"
#include "wiz8/regions.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/engine_code/Video2.h"
#include "sgp.h"
#undef S32
#undef U32
#include "bink.h"
#include "wiz8/bink_video.h"
#include "wiz8/mouth_gap.h"
#include "FileMan.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/PartySelectionScreen.h"
#include "wiz8/video_object_catalog.h"

/* Local Code\QuoteManager.cpp: the character-event subsystem — the event
   descriptor table, the W8CharacterEvent/W8CharacterEventQueue classes, quote
   text formatting and playback, and the portrait pose/reaction machinery.

   Attribution evidence: no assertion path survives in this span, but the
   2001-08-14 demo build embeds this TU's __FILE__ string
   ("E:\Wizardry 8\Local Code\QuoteManager.c"), and that TU's demo string
   cluster holds the quote-personality stems
   ("Aggressive"/"Intellectual"/"Chaotic"/"Cunning"/"Eccentric"/"Kindly"/
   "Laidback"), "Quote %d started for %S" and the
   "Data\Quotes\PCs\%c_%s%d0.MSG" path the functions here reference. The
   functions fill .text span 0x52C810-0x52FEE0, between the anchored
   Health Stamina Mana.cpp tail (0x52C500, ApplyQueuedFatigue) and
   Strings.cpp (0x52FF80). */

/* Re-blit each active portrait quote bubble onto the game surface. Runs when
   the screen comes back from a modal view; slots whose character is dead or
   too far gone to be speaking keep their bubble down. */
// FUNCTION: WIZ8 0x0052FE00
void RedrawPortraitQuoteBubbles(void)
{
    unsigned int party_slot;
    W8MonsterManagerEntry* slot;

    IsModalOpen();
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        slot = &gXStatus.monster_manager_entries[party_slot];
        if (g_status_685170.buffers.party_rows[party_slot].occupied == 0 ||
            g_status_685170.buffers.characters[party_slot].hp_current <= 0 ||
            g_status_685170.buffers.characters[party_slot].highest_condition >= 0xf ||
            slot->portrait_event_active == 0) {
            continue;
        }
        DrawPortraitQuoteBubble(slot->quote.quote_handle, slot->quote.x, slot->quote.y, -0xe);
    }
}

// FUNCTION: WIZ8 0x0052fe80
void StartBreathCycle(int party_slot, char force)
{
    if ((gXStatus.fSpellCastMode == 0 && gXStatus.fItemSelectMode == 0) || force != 0) {
        QueueCharacterEvent(&g_status_685170.buffers.characters[party_slot],
                            g_special_event_0068c50c, 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
    }
}

/* Collect every occupied living slot other than `excluded_slot` whose
   character can actually speak `event_type`, then pick one at random. */
// FUNCTION: WIZ8 0x0052FEE0
int PickRandomPartySpeaker(unsigned int event_type, int excluded_slot)
{
    int eligible[8];
    unsigned int count = 0;

    for (int slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status_685170.buffers.characters[slot];
        if (g_status_685170.buffers.party_rows[slot].occupied != 0 && slot != excluded_slot &&
            character->hp_current != 0 && character->highest_condition < 0xf &&
            FormatCharacterQuoteText(character, event_type, 0) != 0) {
            eligible[count++] = slot;
        }
    }
    if (count != 0) {
        return eligible[Random(count)];
    }
    return -1;
}

/* Character-event subsystem: the event descriptor table, event object, and the
   five-vector dispatch queue. */

// GLOBAL: WIZ8 0x0068C578
int g_special_event_0068c578;
// GLOBAL: WIZ8 0x0068c57c
unsigned int g_value_0068c57c;
/* 0x0068C580: the shared wide buffer formatted character text lands in. The
   message reader admits at most 0x7D0 code units, so the buffer holds exactly
   the two thousand characters that reach the next global at 0x0068D520. */
// GLOBAL: WIZ8 0x0068C580
wchar_t g_character_text_0068c580[2000];
/* 0x005ED91C: the quote file-name stem per personality, a twenty-byte fixed
   buffer each. The nine personas end exactly at the next global; the quote
   lookup composes Data\Quotes\PCs\<m|f>_<stem><1|2>0.MSG from them. */
// GLOBAL: WIZ8 0x005ed91c
const char g_quote_personality_names_005ed91c[9][0x14] = {
    "aggr", "intell", "burly", "chaos", "cun", "ecc", "kind", "laid", "loner",
};
// GLOBAL: WIZ8 0x0068c554
unsigned int g_value_0068c554;
struct W8PortraitTables {
    unsigned short quote_x[8];
    unsigned short quote_y[8];
    int pose_transition[30];
};
// GLOBAL: WIZ8 0x0061cb3c
W8PortraitTables g_portrait_tables_0061cb3c = {
    {0x0080, 0x0138, 0x0080, 0x0138, 0x0080, 0x0138, 0x0080, 0x0138},
    {0x0013, 0x0013, 0x0067, 0x0067, 0x00bc, 0x00bc, 0x0111, 0x0111},
    {1, 3, 3, 4, 5, 3, 2, 3, 3, 3, 1,          2,          3,          4,          1,
     1, 3, 3, 4, 1, 1, 1, 1, 1, 5, 0x01010101, 0x01010101, 0x01010101, 0x01010101, 0x01010101},
};
// GLOBAL: WIZ8 0x005ED8C8
#pragma bss_seg(".data")
int g_effect_argument_005ed8c8 = 0;
#pragma bss_seg()
// GLOBAL: WIZ8 0x005ED8E0
unsigned int g_event_flag_005ed8e0 = 8;
// GLOBAL: WIZ8 0x005ED8E4
unsigned char g_character_event_flags_mask_005ed8e4 = 16;
// GLOBAL: WIZ8 0x005ED8EC
unsigned int g_event_flag_005ed8ec = 0x40;
// GLOBAL: WIZ8 0x005ED8F8
unsigned int g_flee_hp_fraction_005ed8f8 = 50;
// GLOBAL: WIZ8 0x005ED8FC
unsigned int g_value_005ed8fc = 20;
// GLOBAL: WIZ8 0x005ED8D0
int g_effect_argument_005ed8d0 = 2;
// GLOBAL: WIZ8 0x005ED8D4
int g_effect_argument_005ed8d4 = 1;
// GLOBAL: WIZ8 0x005ED914
int g_effect_argument_005ed914 = 127;
// GLOBAL: WIZ8 0x005EE590
int g_effect_005ee590 = 2;
// GLOBAL: WIZ8 0x005EE594
int g_effect_005ee594 = 3;
// GLOBAL: WIZ8 0x005ee598
int g_effect_005ee598 = 4;
// GLOBAL: WIZ8 0x005ee588
#pragma bss_seg(".data")
int g_effect_005ee588 = 0;
#pragma bss_seg()
// GLOBAL: WIZ8 0x005EE58C
int g_effect_005ee58c = 1;
// GLOBAL: WIZ8 0x005EE5A4
int g_effect_005ee5a4 = 7;
// GLOBAL: WIZ8 0x005EE5AC
int g_effect_005ee5ac = 9;
// GLOBAL: WIZ8 0x005EE5B4
int g_effect_005ee5b4 = 11;
// GLOBAL: WIZ8 0x005EE5B8
int g_effect_005ee5b8 = 12;
// GLOBAL: WIZ8 0x005EE5DC
int g_effect_005ee5dc = 0x15;
// GLOBAL: WIZ8 0x005EE5E0
int g_effect_005ee5e0 = 0x16;
/* Search-pulse event ids: the two found-item variants and the found-trigger
   event queued to the searcher. */
// GLOBAL: WIZ8 0x005EE5E4
int g_effect_005ee5e4 = 0x17;
// GLOBAL: WIZ8 0x005EE5E8
int g_effect_005ee5e8 = 0x18;
// GLOBAL: WIZ8 0x005EE5F0
int g_effect_005ee5f0 = 0x1a;
// GLOBAL: WIZ8 0x005EE5F8
int g_effect_005ee5f8 = 28;
// GLOBAL: WIZ8 0x005ee610
int g_effect_005ee610 = 34;
/* 0x005EE624: the character event an item use queues when the attempt ends
   without casting anything. */
// GLOBAL: WIZ8 0x005EE624
int g_effect_005ee624 = 39;
// GLOBAL: WIZ8 0x005EE628
int g_effect_005ee628 = 40;
// GLOBAL: WIZ8 0x005ee640
int g_item_message_005ee640 = 46;
// GLOBAL: WIZ8 0x005ee644
int g_item_message_005ee644 = 47;
// GLOBAL: WIZ8 0x005ee648
int g_item_message_005ee648 = 48;
// GLOBAL: WIZ8 0x005ee64c
int g_item_message_005ee64c = 49;
// GLOBAL: WIZ8 0x005EE654
int g_effect_005ee654 = 51;
// GLOBAL: WIZ8 0x005ee664
int g_item_message_005ee664 = 55;
// GLOBAL: WIZ8 0x005ee68c
int g_item_message_005ee68c = 65;
// GLOBAL: WIZ8 0x005ee690
int g_item_message_005ee690 = 66;
// GLOBAL: WIZ8 0x005EE6D8
int g_effect_005ee6d8 = 0x54;
// GLOBAL: WIZ8 0x005EE6DC
int g_effect_005ee6dc = 0x55;
// GLOBAL: WIZ8 0x005ee6f0
const int g_value_005ee6f0 = 129;
// GLOBAL: WIZ8 0x005ee6fc
int g_item_message_005ee6fc = 132;
// GLOBAL: WIZ8 0x005EE70C
unsigned int g_normal_event_count_005ee70c = 146; /* ordinary-event count */
// GLOBAL: WIZ8 0x005EE710
unsigned int g_remapped_event_count_005ee710 = 31;
// GLOBAL: WIZ8 0x005EE718
unsigned int g_first_remapped_event_005ee718 = 500;

/* 0x005EE000: eight-byte dispatch records indexed by remapped event type.
   The table ends at 0x005EE588 where g_effect_005ee588 begins. TryAdjustQueuedEvent
   reads coalesce_duplicates at +4; ProcessDeferredCharacterEvents reads
   defer_outside_main_game at +5. */
struct W8CharacterEventDescriptor {
    int portrait_pose_category;
    /* 0x04: a queued copy may be rewritten to the shared follow-up event (10)
       when another character already has the same event active. */
    unsigned char coalesce_duplicates;
    unsigned char defer_outside_main_game;
    /* 0x06: when the portrait quote closes, its text is posted to the notice
       log instead of being dropped silently. */
    unsigned char log_quote_on_finish;
    unsigned char unknown_07;
};
static_assert(sizeof(W8CharacterEventDescriptor) == 8, "W8CharacterEventDescriptor_must_be_8");
// GLOBAL: WIZ8 0x005EE000
W8CharacterEventDescriptor g_character_event_descriptors_005ee000[0xb1] = {
    {0x00000001, 0x00, 0x00, 0x00, 0x00}, {0x00000005, 0x00, 0x01, 0x00, 0x00},
    {0x00000004, 0x00, 0x01, 0x00, 0x00}, {0x00000002, 0x00, 0x01, 0x00, 0x00},
    {0x00000003, 0x01, 0x01, 0x00, 0x00}, {0x00000003, 0x01, 0x01, 0x00, 0x00},
    {0x00000003, 0x01, 0x01, 0x00, 0x00}, {0x00000003, 0x01, 0x01, 0x00, 0x00},
    {0x00000001, 0x01, 0x00, 0x00, 0x00}, {0x00000001, 0x01, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000005, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000004, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000004, 0x00, 0x01, 0x00, 0x00}, {0x00000004, 0x00, 0x01, 0x00, 0x00},
    {0x00000004, 0x00, 0x01, 0x00, 0x00}, {0x00000004, 0x00, 0x01, 0x00, 0x00},
    {0x00000004, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000004, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000005, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000005, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000005, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000005, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x00, 0x00, 0x00},
    {0x00000004, 0x00, 0x01, 0x00, 0x00}, {0x00000005, 0x00, 0x00, 0x00, 0x00},
    {0x00000005, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000005, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x00, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x01, 0x00}, {0x00000001, 0x00, 0x01, 0x01, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000004, 0x00, 0x01, 0x00, 0x00},
    {0x00000005, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00}, {0x00000001, 0x00, 0x01, 0x00, 0x00},
    {0x00000001, 0x00, 0x01, 0x00, 0x00},
};

// GLOBAL: WIZ8 0x005EE5D0
int g_effect_005ee5d0 = 18;
// GLOBAL: WIZ8 0x005EE5D4
int g_effect_005ee5d4 = 19;
// GLOBAL: WIZ8 0x005EE5D8
int g_effect_005ee5d8 = 20;
// GLOBAL: WIZ8 0x0068C504
int g_special_event_0068c504;
// GLOBAL: WIZ8 0x0068C508
int g_special_event_0068c508;
// GLOBAL: WIZ8 0x0068C50C
int g_special_event_0068c50c;
// GLOBAL: WIZ8 0x0068C514
int g_special_event_0068c514;
// GLOBAL: WIZ8 0x0068C51C
int g_special_event_0068c51c;
// GLOBAL: WIZ8 0x0068C52C
int g_special_event_0068c52c;
// GLOBAL: WIZ8 0x0068C530
int g_special_event_0068c530;
// GLOBAL: WIZ8 0x0068C538
int g_special_event_0068c538;
// GLOBAL: WIZ8 0x0068C544
int g_special_event_0068c544;
// GLOBAL: WIZ8 0x0068C540
int g_special_event_0068c540;
// GLOBAL: WIZ8 0x0068C550
int g_special_event_0068c550;
// GLOBAL: WIZ8 0x0068C558
int g_special_event_0068c558;
// GLOBAL: WIZ8 0x0068C55C
int g_special_event_0068c55c;
// GLOBAL: WIZ8 0x0068C564
int g_special_event_0068c564;
// GLOBAL: WIZ8 0x0068C568
int g_special_event_0068c568;

static bool MapEventTypeToDescriptorIndex(unsigned int event_type, unsigned int* descriptor_index)
{
    if (event_type < g_normal_event_count_005ee70c) {
        *descriptor_index = event_type;
        return true;
    }
    if (event_type >= g_first_remapped_event_005ee718 &&
        event_type < g_first_remapped_event_005ee718 + g_remapped_event_count_005ee710) {
        *descriptor_index =
            g_normal_event_count_005ee70c + event_type - g_first_remapped_event_005ee718;
        return true;
    }
    return false;
}

/* Character-event queue and portrait/voice updates. The original
   translation-unit name is unknown; the existing unit is retained intact. */

// FUNCTION: WIZ8 0x0052E360
bool IsVoiceMuted(void)
{
    return g_settings_6850c8.muted_voice_volume != 0xff;
}

// FUNCTION: WIZ8 0x0052E370
void SetVoiceMuted(unsigned char muted)
{
    if (muted != 0) {
        if (g_settings_6850c8.muted_voice_volume == 0xff) {
            g_settings_6850c8.muted_voice_volume = g_settings_6850c8.voice_volume;
            g_settings_6850c8.voice_volume = 0;
        }
    } else if (g_settings_6850c8.muted_voice_volume != 0xff) {
        g_settings_6850c8.voice_volume = g_settings_6850c8.muted_voice_volume;
        g_settings_6850c8.muted_voice_volume = 0xff;
    }
}

static void CharacterEventSoundEndCallback(void* callback_data)
{
    W8CharacterEvent* entry = static_cast<W8CharacterEvent*>(callback_data);
    W8CharacterEventQueue* queue = gXStatus.character_event_queue;

    if (entry->sound_end_handled != 0 || queue == 0) {
        return;
    }
    int index = queue->active_events.IndexOf(entry);
    if (index >= 0) {
        queue->active_events.RemoveAt(index);
    }
    if ((queue->follow_up_flags & 1) != 0 && entry->event_type >= 14 && entry->event_type < 16) {
        queue->follow_up_clock = SetCountdownClock(
            (queue->follow_up_flags & 2) != 0 ? Random(6000) + 2000 : Random(60000) + 300000);
    }
    entry->Complete();
    delete entry;
}

// FUNCTION: WIZ8 0x0052C810
W8CharacterEvent::W8CharacterEvent(W8Character* character, unsigned int event_type, int value_0c,
                                   unsigned int flags, int volume)
    : sound_end_handled(0), character(character), event_type(event_type), value_0c(value_0c),
      flags(flags), volume(volume), dispatch_delay_ms(0)
{
    item.item_id = -1;
    switch (event_type) {
    case 2:
    case 3:
        value_18 = character->hp_current;
        break;
    case 5:
    case 6:
    case 9:
    case 0x54:
        value_18 = character->highest_condition;
        break;
    case 7:
        value_18 = 12;
        break;
    case 0x38:
    case 0x55:
        value_18 = character->hp_current;
        value_1c = character->highest_condition;
        break;
    }
}

/* 0x0052D0B0: format one character quote for the given event type into the
   shared wide text buffer. A party member on any screen but the character
   screen takes the text from the NPC bound to its slot; otherwise the type
   selects an entry of the sex/personality/voice quote file, which is then
   wrapped in quotes. Clears the buffer and answers zero when no quote exists. */
// FUNCTION: WIZ8 0x0052D0B0
unsigned char FormatCharacterQuoteText(W8Character* character, unsigned int event_type,
                                       unsigned int* metadata)
{
    char path[80];
    wchar_t text[500];
    int npc_index;
    unsigned char has_npc;

    if (event_type >= 0x92) {
        return 0;
    }
    if (metadata != 0) {
        *metadata = 0xffffffff;
    }
    npc_index = -1;
    has_npc = 0;
    if (character->in_party != 0 && g_current_screen_state.id != W8_SCREEN_CHARACTER) {
        unsigned int slot = CharacterPointerToPartySlot(character);
        npc_index = g_status_685170.buffers.party_rows[slot].animation_0fa;
        has_npc = npc_index != -1;
    }
    if (!has_npc) {
        char gender_code = static_cast<char>(((character->gender != 0) - 1U & 7) + 0x66);
        sprintf(path, "Data\\Quotes\\PCs\\%c_%s%d0.MSG", gender_code,
                g_quote_personality_names_005ed91c[character->personality_0081],
                (character->voice_0085 != 0) + 1);
        if (!FileExists(path)) {
            g_character_text_0068c580[0] = 0;
            return 0;
        }
        GetStringFromStringDatabase(path, event_type, g_character_text_0068c580, 0, metadata);
        g_character_text_0068c580[wcslen(g_character_text_0068c580) - 1] = 0;
    } else {
        W8NpcState* npc = GetNpcState(npc_index);
        if (GetNpcQuoteText(npc, event_type, g_character_text_0068c580) == 0) {
            g_character_text_0068c580[0] = 0;
            return 0;
        }
    }

    if (wcslen(g_character_text_0068c580) == 0) {
        return 0;
    }
    swprintf(text, L"\"%s\"", g_character_text_0068c580);
    wcscpy(g_character_text_0068c580, text);
    return 1;
}

/* The quote text builder fills the shared wide buffer; the final character
   page's description area displays it. */
// FUNCTION: WIZ8 0x0052D240
wchar_t* W8CharacterEvent::GetQuoteText()
{
    FormatCharacterQuoteText(character, event_type, 0);
    return g_character_text_0068c580;
}

/* 0x0052D460 proves four equal derived growable-vector instantiations followed
   by a fifth instantiation with a distinct vtable and the tail state below. */

// FUNCTION: WIZ8 0x0052d460
W8CharacterEventQueue::W8CharacterEventQueue()
    : active_event_type(-1), active_party_slot(-1), follow_up_flags(0), follow_up_speaker_slot(-1)
{
    event_character_masks = new unsigned char[0xb1];
    memset(event_character_masks, 0, 0xb1);
}

// FUNCTION: WIZ8 0x0052d5b0
W8CharacterEventQueue::~W8CharacterEventQueue()
{
    delete[] event_character_masks;
}

// FUNCTION: WIZ8 0x0052db80
void W8CharacterEventQueue::DestroyAllEvents()
{
    while (active_events.count > 0) {
        active_events.RemoveAt(0)->Complete();
    }
    while (npc_deferred_events.count > 0) {
        delete npc_deferred_events.RemoveAt(0);
    }
    while (pending_events.count > 0) {
        delete pending_events.RemoveAt(0);
    }
    while (vector_00.count > 0) {
        delete vector_00.RemoveAt(0);
    }
}

// FUNCTION: WIZ8 0x0052D970
void W8CharacterEventQueue::RemoveCharacterEvents(W8Character* character)
{
    int index;
    W8CharacterEvent* entry;

    for (index = 0; index < active_events.count; ++index) {
        entry = *active_events.GetAt(index);
        if (entry->character == character) {
            active_events.RemoveAt(index);
            --index;
            entry->Complete();
        }
    }
    for (index = 0; index < vector_20.count; ++index) {
        entry = *vector_20.GetAt(index);
        if (entry->character == character) {
            vector_20.RemoveAt(index);
            --index;
            delete entry;
        }
    }
    for (index = 0; index < pending_events.count; ++index) {
        entry = *pending_events.GetAt(index);
        if (entry->character == character) {
            pending_events.RemoveAt(index);
            --index;
            delete entry;
        }
    }
    for (index = 0; index < vector_00.count; ++index) {
        entry = *vector_00.GetAt(index);
        if (entry->character == character) {
            vector_00.RemoveAt(index);
            --index;
            delete entry;
        }
    }
    for (index = 0; index < npc_deferred_events.count; ++index) {
        entry = *npc_deferred_events.GetAt(index);
        if (entry->character == character) {
            npc_deferred_events.RemoveAt(index);
            --index;
            delete entry;
        }
    }
}

// FUNCTION: WIZ8 0x0052DB30
void W8CharacterEventQueue::CompleteAllActiveEvents()
{
    while (active_events.count > 0) {
        active_events.RemoveAt(0)->Complete();
    }
}

// FUNCTION: WIZ8 0x0052e3b0
void W8CharacterEventQueue::CompleteFirstActiveEvent()
{
    W8CharacterEvent* entry;

    if (active_events.count > 0) {
        entry = *active_events.GetAt(0);
        active_events.RemoveAt(active_events.IndexOf(entry));
        if ((follow_up_flags & 1) != 0 && entry->event_type >= 14 && entry->event_type < 16) {
            if ((follow_up_flags & 2) != 0) {
                follow_up_clock = SetCountdownClock(Random(6000) + 2000);
            } else {
                follow_up_clock = SetCountdownClock(Random(60000) + 300000);
            }
        }
        entry->Complete();
        delete entry;
    }
}

// FUNCTION: WIZ8 0x0052ced0
void W8CharacterEvent::Complete()
{
    W8MonsterManagerEntry* slot;
    int party_slot;
    unsigned char sound_was_active;

    party_slot = CharacterPointerToPartySlot(character);
    slot = &gXStatus.monster_manager_entries[party_slot];
    sound_was_active = slot->portrait_event_active;
    slot->active_character_event = 0;
    if (sound_was_active != 0) {
        if (SoundIsPlaying(slot->voice_sound_handle) != 0) {
            sound_end_handled = 1;
            SoundStop(slot->voice_sound_handle);
        }
        SetPartyPortraitEventState(party_slot, 0, -1, 0, 1);
    }
    if (event_type == 23 || event_type == 24) {
        if ((flags & W8_EVENT_NPC_SCRIPT) == 0) {
            if (item.item_id == -1) {
                PostCharacterMessage(party_slot, gppStringList[0x1dc4 / 4]);
            } else {
                PostCharacterMessage(party_slot, gppStringList[0x1dc8 / 4],
                                     GetItemDisplayName(&item));
            }
        }
    } else if (event_type == 51) {
        QueueGameplayEvent(30, party_slot);
    }
}

// FUNCTION: WIZ8 0x0052C910
unsigned char W8CharacterEvent::IsConditionMet(unsigned int event_type)
{
    do {
        switch (event_type) {
        case 2:
        case 3:
            return static_cast<unsigned int>(value_18) > character->hp_current;
        case 5:
        case 6:
        case 7:
        case 9:
            return character->highest_condition == static_cast<unsigned int>(value_18);
        case 10:
            event_type = original_event_type;
            break;
        case 14:
        case 15:
        case 35:
            return gXStatus.fCombatMode == 0;
        case 43:
        case 44:
        case 45:
            return character->gender != W8_GENDER_FEMALE;
        case 56:
            return character->hp_current >= static_cast<unsigned int>(value_18) &&
                   character->highest_condition >= static_cast<unsigned int>(value_1c);
        case 84:
            return static_cast<unsigned int>(value_18) > character->highest_condition;
        case 85:
            if (character->hp_current < static_cast<unsigned int>(value_18) ||
                character->highest_condition != 0) {
                QueueCharacterEvent(character, 84, 0, 1, 0x7f);
                return 0;
            }
            return 1;
        default:
            return 1;
        }
    } while (1);
}

// FUNCTION: WIZ8 0x0052CFB0
static unsigned char CanDispatchCharacterEvent(unsigned int party_slot, unsigned int event_type,
                                               unsigned int flags)
{
    W8Character* character;
    unsigned int mapped_event_type;
    unsigned char slot_mask;

    (void)flags;
    if (party_slot >= 8) {
        return 0;
    }
    if (event_type >= g_normal_event_count_005ee70c) {
        if (event_type < g_first_remapped_event_005ee718 ||
            event_type >= g_remapped_event_count_005ee710 + g_first_remapped_event_005ee718) {
            return 0;
        }
    }
    if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
        return 0;
    }
    character = &g_status_685170.buffers.characters[party_slot];
    if (character->highest_condition > 14) {
        if (event_type == static_cast<unsigned int>(g_special_event_0068c538) ||
            event_type == static_cast<unsigned int>(g_special_event_0068c540) ||
            g_special_event_0068c564 != 0) {
            if (character->condition_turns[17] != 0 || character->condition_turns[19] != 0) {
                return 0;
            }
        } else {
            if (character->highest_condition != 0x11 && character->highest_condition != 0xf) {
                return 0;
            }
            if (event_type != static_cast<unsigned int>(g_special_event_0068c544) &&
                event_type != static_cast<unsigned int>(g_special_event_0068c550) &&
                event_type != static_cast<unsigned int>(g_special_event_0068c51c)) {
                return 0;
            }
        }
    }
    if (character->hp_current == 0 &&
        event_type != static_cast<unsigned int>(g_special_event_0068c538)) {
        return 0;
    }
    mapped_event_type = event_type;
    if (mapped_event_type >= g_first_remapped_event_005ee718) {
        mapped_event_type += g_normal_event_count_005ee70c - g_first_remapped_event_005ee718;
    }
    slot_mask = static_cast<unsigned char>(1 << (party_slot & 31));
    return (gXStatus.character_event_queue->event_character_masks[mapped_event_type] & slot_mask) ==
           0;
}

// FUNCTION: WIZ8 0x0052D260
unsigned char W8CharacterEvent::PlayEventSound()
{
    unsigned int party_slot = CharacterPointerToPartySlot(character);
    unsigned int sound_event = event_type;
    int npc_index = g_status_685170.buffers.party_rows[party_slot].animation_0fa;
    char voice_stem[20];
    char sound_path[80];
    char npc_sound_name[128];
    SOUNDPARMS sound_parms;
    unsigned int sound_handle;
    unsigned int total_ms;
    unsigned int current_ms;
    W8MonsterManagerEntry* record;
    W8NpcState* npc;

    if (character->condition_turns[8] != 0) {
        sound_event = g_special_event_0068c508;
    }
    if (npc_index == -1 || g_status_685170.game_started == 0 ||
        g_current_screen_state.id == W8_SCREEN_CHARACTER) {
        char gender_code = static_cast<char>(((character->gender != 0) - 1U & 7) + 0x66);
        sprintf(voice_stem, "%c_%s%d0", gender_code,
                g_quote_personality_names_005ed91c[character->personality_0081],
                (character->voice_0085 != 0) + 1);
        sprintf(sound_path, "Data\\Sound\\PCs\\%s\\%s_%03d.wav", voice_stem, voice_stem,
                sound_event);
    } else {
        npc = GetNpcState(npc_index);
        if (npc != 0) {
            FormatNpcVoiceSoundPath(npc, npc_sound_name);
            sprintf(sound_path, "Data\\Sound\\PCs\\%s\\%s_%03d.wav", npc_sound_name, npc_sound_name,
                    sound_event);
        }
    }
    memset(&sound_parms, 0xff, sizeof(sound_parms));
    sound_parms.uiVolume = (volume * (g_settings_6850c8.voice_volume & 0xff)) / 0x7f;
    sound_parms.EOSCallback = CharacterEventSoundEndCallback;
    sound_parms.pCallbackData = this;
    sound_handle = SoundPlay(sound_path, &sound_parms);
    record = &gXStatus.monster_manager_entries[party_slot];
    record->voice_sound_handle = sound_handle;
    if (sound_handle == 0xffffffff) {
        if (event_type > 0x91) {
            static const wchar_t kFallbackVoiceText[] = L"Ouch play this sound";
            record->voice_time_remaining_ms =
                ComputePortraitMessageDuration(const_cast<wchar_t*>(kFallbackVoiceText));
        } else {
            record->voice_time_remaining_ms =
                ComputePortraitMessageDuration(g_character_text_0068c580);
        }
        return 1;
    }
    SoundGetMilliSecondPosition(sound_handle, &total_ms, &current_ms);
    record->voice_time_remaining_ms = total_ms;
    LoadMouthGapTrack(sound_path, &record->mouth_gap);
    return 1;
}

// FUNCTION: WIZ8 0x0052CA60
unsigned char W8CharacterEvent::Dispatch()
{
    unsigned int party_slot;
    unsigned int metadata;
    unsigned int descriptor_index;
    W8MonsterManagerEntry* slot;
    W8PartySlotRow* row;
    int npc_index;
    W8NpcState* npc;
    unsigned char has_quote;

    metadata = 0xffffffff;
    if (character == 0) {
        return 0;
    }
    party_slot = CharacterPointerToPartySlot(character);
    if (!MapEventTypeToDescriptorIndex(event_type, &descriptor_index)) {
        return 0;
    }
    metadata = 0xffffffff;
    if ((flags & W8_EVENT_BYPASS_CHECKS) == 0) {
        if (CanDispatchCharacterEvent(party_slot, event_type, flags) == 0 ||
            IsConditionMet(event_type) == 0) {
            goto finish_without_dispatch;
        }
        if (event_type != (unsigned int)g_special_event_0068c578 &&
            event_type != (unsigned int)g_special_event_0068c508) {
            if (character->condition_turns[W8_CONDITION_SPELLCASTING_BLOCKED] != 0) {
                QueueCharacterEvent(character, g_special_event_0068c508, 0, 1, 0x7f);
                return 0;
            }
            if (character->condition_turns[11] != 0) {
                QueueCharacterEvent(character, g_special_event_0068c578, 0, 1, 0x7f);
                return 0;
            }
        }
    }
    slot = &gXStatus.monster_manager_entries[party_slot];
    if (slot->portrait_event_active == 0) {
        if (event_type == 0x33) {
            SetNpcDialoguePanelVisible(0);
        }
        row = &g_status_685170.buffers.party_rows[party_slot];
        npc_index = row->animation_0fa;
        if (npc_index != -1 && event_type < 0x93) {
            npc = GetNpcState(npc_index);
            row->pending_event_type_ff = event_type;
            if (npc == 0) {
                return 1;
            }
            if (npc->name_style == W8_NPC_VI_DOMINA && event_type > 0x8b && event_type < 0x92 &&
                g_status_685170.current_level != 0) {
                return 0;
            }
            if ((flags & W8_EVENT_NPC_SCRIPT) != 0) {
                SetNpcScriptEventActive(1);
                ReleaseNpcScriptFile0055A0A0(npc->script_file);
                ReloadNpcScriptResources(npc);
            }
            BeginNpcScriptDialogue(npc, 1);
            RunNpcScriptLine(event_type, (flags & W8_EVENT_NPC_SCRIPT) != 0);
            if ((flags & W8_EVENT_NPC_SCRIPT) != 0) {
                SetNpcScriptEventActive(0);
                ReleaseNpcScriptFile0055A0A0(npc->script_file);
                ReloadNpcScriptResources(npc);
            }
            slot->active_character_event = this;
            if (event_type > 1 && (event_type < 4 || event_type == 0x1c)) {
                gXStatus.character_event_queue->SetEventCharacterMask(event_type, party_slot, 1);
            }
            if (event_type != 10) {
                gXStatus.character_event_queue->active_event_type = event_type;
                gXStatus.character_event_queue->active_party_slot = party_slot;
            }
            gXStatus.character_event_queue->recent_event_clock = SetCountdownClock(5000);
            return 1;
        }
        has_quote = FormatCharacterQuoteText(character, event_type, &metadata);
        if (PlayEventSound() != 0) {
            if (event_type > 1 && (event_type < 4 || event_type == 0x1c)) {
                gXStatus.character_event_queue->SetEventCharacterMask(event_type, party_slot, 1);
            }
            if (event_type != 10) {
                gXStatus.character_event_queue->active_event_type = event_type;
                gXStatus.character_event_queue->active_party_slot = party_slot;
            }
            gXStatus.character_event_queue->recent_event_clock = SetCountdownClock(5000);
            if (event_type < 0x92) {
                SetPartyPortraitEventState(
                    party_slot, 1, event_type, g_character_text_0068c580,
                    1 - ((flags & g_character_event_flags_mask_005ed8e4) != 0));
                slot->active_character_event = this;
                row->pending_event_type_ff = event_type;
                slot->pending_event_type_114 = event_type;
                return 1;
            }
            SetPartyPortraitEventState(party_slot, 1, event_type, 0, 1);
            slot->pending_event_type_114 = event_type;
            return 1;
        }
        Complete();
        if (has_quote == 0) {
            return 0;
        }
        if (event_type > 1 && (event_type < 4 || event_type == 0x1c)) {
            gXStatus.character_event_queue->SetEventCharacterMask(event_type, party_slot, 1);
        }
        if (event_type != 10) {
            gXStatus.character_event_queue->active_event_type = event_type;
            gXStatus.character_event_queue->active_party_slot = party_slot;
        }
        gXStatus.character_event_queue->recent_event_clock = SetCountdownClock(5000);
        if ((gXStatus.character_event_queue->follow_up_flags & 1) == 0) {
            return 0;
        }
        if (event_type < 14) {
            return 0;
        }
        if (event_type >= 16) {
            return 0;
        }
        if ((gXStatus.character_event_queue->follow_up_flags & 2) != 0) {
            gXStatus.character_event_queue->follow_up_clock =
                SetCountdownClock(Random(6000) + 2000);
            return 0;
        }
        gXStatus.character_event_queue->follow_up_clock = SetCountdownClock(Random(60000) + 300000);
        return 0;
    }
finish_without_dispatch:
    Complete();
    return 0;
}

/* 0x0052F890: turn a party-slot portrait/voice event on or off, optionally
   laying out the quote bubble and posting subtitle notices when one ends. */
// FUNCTION: WIZ8 0x0052F890
void SetPartyPortraitEventState(unsigned int party_slot, unsigned char active,
                                unsigned int event_type, const wchar_t* quote_text, int show_quote)
{
    W8MonsterManagerEntry* record = &gXStatus.monster_manager_entries[party_slot];
    W8PortraitQuoteState* quote = &record->quote;

    if (record->portrait_event_active == active) {
        return;
    }
    if (active == 0) {
        record->portrait_event_active = 0;
        if (gfCapturingVideo != 0) {
            return;
        }
        record->previous_portrait_frame = record->portrait_frame;
        record->portrait_frame = 6;
        record->portrait_frame_dirty = 1;
        int pc_slot = RPCPtrToPCSlot(record);
        record->portrait_pose_animation_active = 0;
        unsigned int highest_condition =
            g_status_685170.buffers.characters[pc_slot].highest_condition;
        if (highest_condition < 0xf && gXStatus.fSurprisePossible == 0) {
            if (record->target_portrait_pose != 1) {
                record->target_portrait_pose = 1;
            }
        } else {
            record->target_portrait_pose = 2;
        }
        if (quote->quote_handle == -1) {
            record->portrait_event_active = active;
            return;
        }
        unsigned int stored_event = record->pending_event_type_114;
        unsigned int mapped_event = stored_event;
        if (static_cast<int>(g_normal_event_count_005ee70c) < static_cast<int>(mapped_event)) {
        show_deactivate_quote:
            wchar_t formatted[100];
            const wchar_t* character_name = g_status_685170.buffers.characters[party_slot].name;
            swprintf(formatted, L"%s", character_name);
            int scroll_range = GetTextBoxScrollRange();
            ShowNotice(1, formatted, 3, scroll_range, 0);
            const wchar_t* suffix = GetPortraitQuoteText(quote->quote_handle);
            ShowNotice(0xf, suffix);
        } else {
            if (g_first_remapped_event_005ee718 <= mapped_event) {
                mapped_event =
                    g_normal_event_count_005ee70c - g_first_remapped_event_005ee718 + mapped_event;
            }
            if (g_character_event_descriptors_005ee000[mapped_event].log_quote_on_finish != 0) {
                goto show_deactivate_quote;
            }
        }
        ReleasePortraitQuoteBubble(quote->quote_handle);
        if (g_current_screen_state.id == W8_SCREEN_CAMP ||
            (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
             g_level_block->review_transition_active == 0)) {
            int left = quote->x;
            int top = quote->y;
            ClearSurfaceRect(left, top, quote->width + left, quote->height + top);
            InvalidateRegion(left, top, quote->width + left, quote->height + top, 0);
        }
        RegionSetDisable(party_slot + 0x1d);
        DisableRegionSetInput(party_slot + 0x1d);
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
            RequestRedraw(0x8000);
            RequestRedrawParty();
            record->portrait_event_active = 0;
            return;
        }
        if (g_current_screen_state.id == W8_SCREEN_CAMP) {
            g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
        }
        record->portrait_event_active = active;
        return;
    }

    record->portrait_event_active = 1;
    if (gfCapturingVideo != 0) {
        return;
    }
    record->previous_portrait_frame = record->portrait_frame;
    record->portrait_frame = 7;
    record->portrait_frame_dirty = 1;
    record->portrait_frame_clock = SetCountdownClock(0x78);
    int pose_category;
    if (static_cast<int>(g_normal_event_count_005ee70c) < static_cast<int>(event_type)) {
        pose_category = 1;
    } else {
        unsigned int mapped_event = event_type;
        if (g_first_remapped_event_005ee718 <= event_type) {
            mapped_event =
                g_normal_event_count_005ee70c - g_first_remapped_event_005ee718 + event_type;
        }
        pose_category = g_character_event_descriptors_005ee000[mapped_event].portrait_pose_category;
    }
    int pc_slot = RPCPtrToPCSlot(record);
    record->portrait_pose_animation_active = 0;
    unsigned int highest_condition = g_status_685170.buffers.characters[pc_slot].highest_condition;
    if (highest_condition < 0xf && gXStatus.fSurprisePossible == 0) {
        if (record->target_portrait_pose != pose_category) {
            record->target_portrait_pose = pose_category;
        }
    } else {
        record->target_portrait_pose = 2;
    }
    if (quote_text == 0 || show_quote == 0) {
        quote->quote_handle = -1;
    } else {
        unsigned char layout_quote = 0;
        unsigned char use_modal_gate = 0;
        if (static_cast<int>(g_normal_event_count_005ee70c) < static_cast<int>(event_type)) {
            use_modal_gate = 1;
        } else {
            unsigned int mapped_event = event_type;
            if (g_first_remapped_event_005ee718 <= event_type) {
                mapped_event =
                    g_normal_event_count_005ee70c - g_first_remapped_event_005ee718 + event_type;
            }
            if (g_character_event_descriptors_005ee000[mapped_event].defer_outside_main_game != 0) {
                use_modal_gate = 1;
            } else if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                use_modal_gate = 1;
            } else {
                layout_quote = 1;
            }
        }
        if (use_modal_gate != 0 && g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
            IsModalOpen() == 0) {
            layout_quote = 1;
        }
        if (layout_quote == 0 || g_settings_6850c8.pc_subtitles == 0) {
            quote->quote_handle = -1;
        } else {
            unsigned short width;
            unsigned short height;
            quote->quote_handle = LayoutPortraitQuoteBubble(-1, 0, 0, quote_text, 200, 0, 0, 0,
                                                            &width, &height, 0xffffffff);
            quote->width = width;
            quote->height = height;
            if (g_current_screen_state.id == W8_SCREEN_CAMP) {
                if (static_cast<int>(party_slot) == giReviewCharSlot) {
                    quote->x = 10;
                    quote->y = 8;
                } else {
                    quote->x = static_cast<unsigned short>(((party_slot & 1) * 0x30) + 0x36);
                    quote->y = static_cast<unsigned short>((party_slot >> 1) * 0x27 + 5);
                }
                g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
            } else {
                unsigned short base_x = g_portrait_tables_0061cb3c.quote_x[party_slot];
                quote->x = base_x;
                quote->y = g_portrait_tables_0061cb3c.quote_y[party_slot];
                if ((party_slot & 1) == 1) {
                    quote->x = static_cast<unsigned short>(base_x - quote->width + 200);
                }
                if (quote->y + quote->height > 0x165) {
                    quote->y = static_cast<unsigned short>(0x165 - quote->height);
                }
            }
            unsigned short x = quote->x;
            unsigned short y = quote->y;
            SetRegionBounds(party_slot + 0x12e, x, y, x + quote->width, quote->height + y);
            RegionSetEnable(party_slot + 0x1d);
            EnableRegionSetInput(party_slot + 0x1d);
        }
    }
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
        g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS &&
        g_level_block->portrait_refresh_pending[party_slot] == 0 &&
        event_type != static_cast<unsigned int>(g_special_event_0068c568)) {
        RefreshSelectedPartyPortrait(party_slot);
        record->field_0cf = 1;
        record->portrait_event_active = active;
        return;
    }
    record->portrait_event_active = active;
}

/* Restarts the follow-up clock for entries of the middle event band while the
   state flag selects it. */
// FUNCTION: WIZ8 0x0052E160
void W8CharacterEventQueue::RestartFollowUpClock(W8CharacterEvent* entry)
{
    int flags = follow_up_flags;
    unsigned int type = entry->event_type;
    int duration;

    if ((flags & 1) == 0 || type < 14 || type >= 16) {
        return;
    }
    if ((flags & 2) == 0) {
        duration = Random(60000) + 300000;
    } else {
        duration = Random(6000) + 2000;
    }
    follow_up_clock = SetCountdownClock(duration);
}

/* Advances the ambient follow-up exchange once the environment allows it.
   Bit 0 of follow_up_flags marks the sequence armed; bit 1 marks that a
   speaker was already picked and a response event is pending. Each queued
   middle-band event re-arms follow_up_clock when it dispatches; the -1 here
   disarms the clock until that happens. */
// FUNCTION: WIZ8 0x0052E1C0
void W8CharacterEventQueue::ProcessFollowUpEvents()
{
    if (GetFlag68F105() != 0 || GetEnvironmentFlag0060A394() == 0) {
        return;
    }
    if ((follow_up_flags & 1) == 0) {
        follow_up_flags |= 1;
        follow_up_speaker_slot = GetRandomCharacter(0, 0, -1, -1);
        if (follow_up_speaker_slot != -1) {
            follow_up_flags |= 2;
            QueueCharacterEvent(&g_status_685170.buffers.characters[follow_up_speaker_slot],
                                Random(2) + 0xe, 1, 0, 0x7f);
            follow_up_clock = -1;
        }
        return;
    }
    if ((follow_up_flags & 2) == 0) {
        if (ClockIsTicking(follow_up_clock) != 0) {
            return;
        }
        follow_up_flags |= 1;
        follow_up_speaker_slot = GetRandomCharacter(0, 0, -1, -1);
        if (follow_up_speaker_slot != -1) {
            follow_up_flags |= 2;
            QueueCharacterEvent(&g_status_685170.buffers.characters[follow_up_speaker_slot],
                                Random(2) + 0xe, 1, 0, 0x7f);
            follow_up_clock = -1;
        }
        return;
    }
    if (ClockIsTicking(follow_up_clock) != 0) {
        return;
    }
    if (follow_up_speaker_slot == -1) {
        follow_up_flags &= ~2;
        return;
    }
    int next_slot = GetRandomCharacter(0, 0, follow_up_speaker_slot, -1);
    if (next_slot != -1) {
        QueueCharacterEvent(&g_status_685170.buffers.characters[next_slot], Random(2) + 0xe, 1, 0,
                            0x7f);
        follow_up_clock = -1;
        follow_up_flags &= ~2;
    }
}

// FUNCTION: WIZ8 0x0052D610
int W8CharacterEventQueue::QueueEntry(W8CharacterEvent* entry)
{
    unsigned int party_slot;

    party_slot = CharacterPointerToPartySlot(entry->character);
    if (HasEventCharacter(entry->event_type, party_slot)) {
        delete entry;
        return 0;
    }
    if (g_status_685170.flag_2497 != 0) {
        delete entry;
        return 0;
    }
    if (entry->event_type > 0x91 && (entry->flags & W8_EVENT_NO_PREEMPT) == 0) {
        W8MonsterManagerEntry* slot = &gXStatus.monster_manager_entries[party_slot];
        if (slot->active_character_event != 0) {
            active_events.Remove(slot->active_character_event);
            RestartFollowUpClock(slot->active_character_event);
            slot->active_character_event->Complete();
            delete slot->active_character_event;
        }
        entry->Dispatch();
        return 1;
    }
    if (entry->event_type == 0x21) {
        int index;
        if (active_events.count > 0 && active_events.data[0]->event_type == 0x21) {
            delete entry;
            return 0;
        }
        for (index = 0; index < pending_events.count; ++index) {
            if (pending_events.data[index]->event_type == 0x21) {
                delete entry;
                return 0;
            }
        }
    }
    if ((ShouldDeferCharacterEventForNpcScript(0) != 0 || IsNpcScriptSessionActive() != 0) &&
        (entry->flags & W8_EVENT_NO_NPC_DEFER) == 0) {
        npc_deferred_events.Add(entry);
        return 1;
    }
    pending_events.Add(entry);
    return 1;
}

// FUNCTION: WIZ8 0x0052DD20
void W8CharacterEventQueue::SetEventCharacterMask(unsigned int event_type, unsigned int party_slot,
                                                  bool enabled)
{
    unsigned char mask = (unsigned char)(1 << (party_slot & 31));

    unsigned int mask_index;
    if (!MapEventTypeToDescriptorIndex(event_type, &mask_index)) {
        return;
    }
    if (!enabled) {
        event_character_masks[mask_index] &= (unsigned char)~mask;
    } else {
        event_character_masks[mask_index] |= mask;
    }
}

// FUNCTION: WIZ8 0x0052DD90
bool W8CharacterEventQueue::HasEventCharacter(unsigned int event_type, unsigned int party_slot)
{
    unsigned int mask_index;
    unsigned char mask = (unsigned char)(1 << (party_slot & 31));

    if (!MapEventTypeToDescriptorIndex(event_type, &mask_index)) {
        return false;
    }
    return (event_character_masks[mask_index] & mask) != 0;
}

// FUNCTION: WIZ8 0x0052DC80
unsigned char W8CharacterEventQueue::TryAdjustQueuedEvent(W8CharacterEvent* entry)
{
    if (entry == 0 || active_event_type == -1 ||
        entry->event_type != (unsigned int)active_event_type) {
        return 1;
    }

    unsigned int party_slot = CharacterPointerToPartySlot(entry->character);
    if (party_slot == (unsigned int)active_party_slot) {
        return 1;
    }

    if (ClockIsTicking(recent_event_clock) == 0) {
        active_event_type = -1;
        active_party_slot = -1;
        return 1;
    }

    unsigned int event_type = entry->event_type;
    unsigned int descriptor_index;
    if (!MapEventTypeToDescriptorIndex(event_type, &descriptor_index)) {
        return 1;
    }
    if (g_character_event_descriptors_005ee000[descriptor_index].coalesce_duplicates == 0) {
        return 1;
    }

    entry->original_event_type = event_type;
    if (event_type == 4) {
        return 0;
    }
    entry->event_type = 10;
    return 1;
}

// FUNCTION: WIZ8 0x0052E460
unsigned char W8CharacterEventQueue::HasActiveEvents()
{
    return active_events.count > 0;
}

// FUNCTION: WIZ8 0x0052E470
unsigned char W8CharacterEventQueue::IsMainQueueEmpty() const
{
    return pending_events.count < 1;
}

// FUNCTION: WIZ8 0x0052DDD0
void W8CharacterEventQueue::ProcessDeferredCharacterEvents()
{
    W8CharacterEvent* entry;
    W8CharacterEvent* baseline;
    int index;
    int scan;
    int conflict_count;
    int* conflict_indices;
    int remaining_conflicts;
    unsigned int event_type;
    unsigned int party_slot;

    if (gXStatus.fSurprisePossible != 0) {
        return;
    }

    if (npc_deferred_events.count > 0 && ShouldDeferCharacterEventForNpcScript(0) == 0 &&
        IsNpcScriptSessionActive() == 0) {
        for (index = 0; index < npc_deferred_events.count; ++index) {
            QueueEntry(npc_deferred_events.data[index]);
        }
        npc_deferred_events.Clear();
    }

    if (pending_events.count != 0) {
        conflict_count = 1;
        baseline = pending_events.data[0];
        conflict_indices = new int[pending_events.count];
        conflict_indices[0] = 0;
        for (index = 1; index < pending_events.count; ++index) {
            entry = *pending_events.GetAt(index);
            event_type = entry->event_type;
            if (event_type == baseline->event_type && entry->character != baseline->character &&
                event_type != 0x2a && event_type != 0x24) {
                conflict_indices[conflict_count] = index;
                ++conflict_count;
            }
        }

        if (conflict_count > 3) {
            remaining_conflicts = conflict_count;
            while (remaining_conflicts > 3) {
                int pick = Random(conflict_count);
                if (conflict_indices[pick] != -1) {
                    conflict_indices[pick] = -1;
                    --remaining_conflicts;
                }
            }
            for (index = conflict_count - 1; index >= 0; --index) {
                if (conflict_indices[index] != -1) {
                    delete pending_events.RemoveAt(conflict_indices[index]);
                }
            }
        }
        delete[] conflict_indices;
    }

    if (pending_events.count == 0) {
        UpdateNpcDialogueVoiceAndCursor();
        if (PartyPortraitEventsIdle() != 0) {
            ProcessNpcScriptingFrame();
        }
        return;
    }

    index = 0;
    while (index < pending_events.count) {
        entry = *pending_events.GetAt(index);
        event_type = entry->event_type;
        if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME) {
            unsigned int descriptor_index;
            if (MapEventTypeToDescriptorIndex(event_type, &descriptor_index) &&
                g_character_event_descriptors_005ee000[descriptor_index].defer_outside_main_game !=
                    0) {
                ++index;
                continue;
            }
        }

        if (entry->character->in_party != 0) {
            party_slot = CharacterPointerToPartySlot(entry->character);
            if (!HasEventCharacter(event_type, party_slot)) {
                if (PartyPortraitEventsIdle() == 0) {
                    return;
                }
                if (entry->dispatch_delay_ms != 0 && GetTickCount() - entry->dispatch_delay_start <=
                                                         (unsigned int)entry->dispatch_delay_ms) {
                    return;
                }
                pending_events.RemoveAt(index);
                if (TryAdjustQueuedEvent(entry) == 0) {
                    delete entry;
                    return;
                }
                if (entry->Dispatch() == 0) {
                    return;
                }
                active_events.Add(entry);
                return;
            }
        }

        for (scan = 0; scan < pending_events.count; ++scan) {
            if (pending_events.data[scan] == entry) {
                pending_events.RemoveAt(scan);
                return;
            }
        }
        return;
    }
}

/* Queue `effect` on a random eligible party member. When the first pick cannot
   receive the event the draw retries up to fifty times; exhaustion returns
   zero without queueing anything. */
// FUNCTION: WIZ8 0x0052E5C0
W8CharacterEvent* ApplyItemEffectToRandomCharacter(unsigned int event_type, int excluded_slot,
                                                   int argument, unsigned int flags)
{
    int character_slot;
    int attempts;

    if (event_type >= g_normal_event_count_005ee70c &&
        (event_type < g_first_remapped_event_005ee718 ||
         event_type >= g_remapped_event_count_005ee710 + g_first_remapped_event_005ee718)) {
        return 0;
    }
    character_slot = GetRandomCharacter(0, 0, excluded_slot, -1);
    if (character_slot == -1) {
        return 0;
    }
    attempts = 0;
    while (CanDispatchCharacterEvent(character_slot, event_type, 0) == 0) {
        if (attempts >= 0x32) {
            return 0;
        }
        character_slot = GetRandomCharacter(0, 0, excluded_slot, -1);
        ++attempts;
    }
    return QueueCharacterEvent(&g_status_685170.buffers.characters[character_slot], event_type,
                               argument, flags, g_effect_argument_005ed914);
}

// FUNCTION: WIZ8 0x0052E690
W8CharacterEvent* QueueCharacterEvent(W8Character* character, int event_type, int argument,
                                      unsigned int flags, unsigned int volume)
{
    W8CharacterEvent* entry;

    if (g_settings_6850c8.pc_confirmations == 0 &&
        (event_type == g_special_event_0068c50c || event_type == g_special_event_0068c568)) {
        return 0;
    }
    if (event_type != g_special_event_0068c504 && event_type != g_special_event_0068c550 &&
        event_type != g_special_event_0068c51c && event_type != g_special_event_0068c538 &&
        event_type != g_special_event_0068c540 && event_type != g_special_event_0068c564) {
        volume = volume * 70 / 100;
    }
    entry = new W8CharacterEvent(character, event_type, argument, flags, volume);
    if (entry != 0 && gXStatus.character_event_queue->QueueEntry(entry) == 0) {
        return 0;
    }
    return entry;
}

/* Remove one queued character event from the owned vector before dispatching
   and deleting it. Event types 14 and 15 also restart the runtime state's
   follow-up clock; bit 1 selects the short interval. */
// FUNCTION: WIZ8 0x0052D8D0
void W8CharacterEventQueue::CompleteActiveEvent(W8CharacterEvent* entry)
{
    int index = active_events.IndexOf(entry);

    if (index >= 0) {
        active_events.RemoveAt(index);
    }
    if ((follow_up_flags & 1) != 0 && entry->event_type >= 14 && entry->event_type < 16) {
        if ((follow_up_flags & 2) == 0) {
            follow_up_clock = SetCountdownClock(Random(60000) + 300000);
        } else {
            follow_up_clock = SetCountdownClock(Random(6000) + 2000);
        }
    }
    entry->Complete();
    delete entry;
}

/* Set the pose a party-slot portrait is animating toward. The request drops
   any pose animation already in progress; a slot whose character is dead or
   in a severe condition - or a party caught surprised - is forced to the
   incapacitated pose instead. */
// FUNCTION: WIZ8 0x0052F000
void SetPortraitTargetPose(W8MonsterManagerEntry* slot, int pose)
{
    int party_slot = RPCPtrToPCSlot(slot);

    slot->portrait_pose_animation_active = 0;
    if (g_status_685170.buffers.characters[party_slot].highest_condition < 0xf &&
        gXStatus.fSurprisePossible == 0) {
        if (slot->target_portrait_pose != pose) {
            slot->target_portrait_pose = pose;
        }
        return;
    }
    slot->target_portrait_pose = 2;
}

/* A character at zero percent hit points may start one of the three recovered
   incapacitation events. Which pair is available is selected by the two data
   flags; successfully queueing the event clears the matching held effect. */
// FUNCTION: WIZ8 0x0052F060
void MaybeStartIncapacitationEvent(unsigned int party_slot)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    int effect;

    if ((character->hp_current * 100) / (unsigned int)character->hp_max != 0) {
        return;
    }
    effect = g_effect_005ee594;
    if (g_value_005ed8fc == 0) {
        if (g_flee_hp_fraction_005ed8f8 == 0) {
            return;
        }
        effect = Random(2) == 0 ? g_effect_005ee590 : g_effect_005ee5f8;
    }
    if (effect != -1 && QueueCharacterEvent(character, effect, 0, g_effect_argument_005ed8c8,
                                            g_effect_argument_005ed914) != 0) {
        gXStatus.character_event_queue->SetEventCharacterMask(effect, party_slot, 1);
    }
}

/* After a death in `party_slot`, pick one other eligible party member and queue
   their reaction event - the slot-two pair share one effect, later slots pick
   by gender - with a three-second clock on the queued entry. */
// FUNCTION: WIZ8 0x0052F110
void QueuePartyDeathReaction(unsigned int party_slot)
{
    W8CharacterEvent* entry;
    unsigned int selected[1];
    unsigned int remaining;
    unsigned int index;
    unsigned char skip_first_two = 0;
    int effect;

    if (party_slot < 2) {
        skip_first_two = 1;
        effect = g_effect_005ee5d8;
    } else {
        effect = g_effect_005ee5d0;
        if (g_status_685170.buffers.characters[party_slot].gender != 0) {
            effect = g_effect_005ee5d4;
        }
    }
    remaining = GetRandomPartySlots(0, 0, party_slot, selected, 1, skip_first_two);
    for (index = 0; index < remaining; ++index) {
        entry = QueueCharacterEvent(&g_status_685170.buffers.characters[selected[index]], effect, 0,
                                    g_effect_argument_005ed8cc, g_effect_argument_005ed914);
        if (entry != 0) {
            entry->dispatch_delay_ms = 3000;
            entry->dispatch_delay_start = GetTickCount();
        }
    }
}

/* Queue a low-HP flee or incapacitation event when the character is still
   standing, then always roll one of three ambient follow-up events. */
// FUNCTION: WIZ8 0x0052F2C0
void QueueDamageReactionEvents(W8Character* character)
{
    unsigned int hp_percent;
    unsigned int party_slot;
    bool has_incapacitation_event;
    bool has_flee_event;
    W8CharacterEvent* entry;
    int effect;
    int follow_up_events[3];

    hp_percent = (character->hp_current * 100) / (unsigned int)character->hp_max;
    party_slot = CharacterPointerToPartySlot(character);
    has_incapacitation_event =
        gXStatus.character_event_queue->HasEventCharacter(g_effect_005ee594, party_slot);
    has_flee_event =
        gXStatus.character_event_queue->HasEventCharacter(g_effect_005ee590, party_slot);
    gXStatus.character_event_queue->HasEventCharacter(g_effect_005ee5f8, party_slot);
    if (character->highest_condition != 0xf && character->highest_condition != 0x11) {
        if (g_value_005ed8fc <= hp_percent || has_incapacitation_event) {
            if (g_flee_hp_fraction_005ed8f8 <= hp_percent) {
                goto queue_follow_up_event;
            }
            if (Random(2) != 0 || has_flee_event) {
                effect = g_effect_005ee5f8;
            } else {
                effect = g_effect_005ee590;
            }
            entry = QueueCharacterEvent(character, effect, g_effect_argument_005ed8d4,
                                        g_effect_argument_005ed8d0, g_effect_argument_005ed914);
        } else {
            entry = QueueCharacterEvent(character, g_effect_005ee594, g_effect_argument_005ed8d4,
                                        g_effect_argument_005ed8d0, g_effect_argument_005ed914);
        }
        if (entry != 0) {
            entry->dispatch_delay_ms = 0x5dc;
            entry->dispatch_delay_start = GetTickCount();
        }
    }
queue_follow_up_event:
    follow_up_events[0] = g_special_event_0068c544;
    follow_up_events[1] = g_special_event_0068c550;
    follow_up_events[2] = g_special_event_0068c51c;
    QueueCharacterEvent(character, follow_up_events[Random(3)], g_effect_argument_005ed8d4,
                        g_effect_argument_005ed8cc, g_effect_argument_005ed914);
}

/* Turn-begin path for several surviving party members: queue event 0x15 on
   one random eligible character. */
// FUNCTION: WIZ8 0x0052F1D0
void QueueTurnReactionEvent(void)
{
    unsigned int selected[1];
    unsigned int index;
    unsigned int remaining;

    remaining = GetRandomPartySlots(0, 0, -1, selected, 1, 0);
    for (index = 0; index < remaining; ++index) {
        QueueCharacterEvent(&g_status_685170.buffers.characters[selected[index]], g_effect_005ee5dc,
                            0, g_effect_argument_005ed8cc, g_effect_argument_005ed914);
    }
}

/* Turn-begin path when only one occupied party member is still standing:
   that character says event 0x16. */
// FUNCTION: WIZ8 0x0052F240
void QueueLastSurvivorEvent(void)
{
    int alive_count = 0;
    int last_alive = 0;

    for (int slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.party_rows[slot].occupied != 0 &&
            g_status_685170.buffers.characters[slot].condition_turns[W8_CONDITION_DEAD] == 0) {
            ++alive_count;
            last_alive = slot;
        }
    }
    if (alive_count != 0) {
        QueueCharacterEvent(&g_status_685170.buffers.characters[last_alive], g_effect_005ee5e0, 0,
                            g_effect_argument_005ed8cc, g_effect_argument_005ed914);
    }
}

/* Reacts to a freshly recomputed highest_condition: the armed one-shot flag
   swallows one change, otherwise the character queues the reaction event for
   the new condition. A dying character first loses every queued event, and a
   charmed character makes a different party member say the line instead. */
// FUNCTION: WIZ8 0x0052F430
void QueueConditionChangeReaction(W8Character* character)
{
    int events[3];
    int excluded_slot;
    int slot;
    int attempts;
    unsigned int reaction;

    if (IsSedexusCaptureActive() != 0) {
        return;
    }
    if (g_status_685170.skip_next_condition_reaction != 0) {
        g_status_685170.skip_next_condition_reaction = 0;
        return;
    }
    switch (character->highest_condition) {
    case 2:
    case 7:
    case 9:
    case 10:
        reaction = g_value_005ee59c;
        if (Random(2) == 0) {
            reaction = g_value_005ee5a0;
        }
        QueueCharacterEvent(character, reaction, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 3:
    case 4:
        QueueCharacterEvent(character, g_special_event_0068c52c, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 5:
        QueueCharacterEvent(character, g_special_event_0068c558, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 6:
        QueueCharacterEvent(character, g_special_event_0068c514, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 8:
        QueueCharacterEvent(character, g_special_event_0068c508, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 0xb:
        QueueCharacterEvent(character, g_special_event_0068c578, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        break;
    case 0xc:
        QueueCharacterEvent(character, g_effect_005ee5a4, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 0xe:
    case 0x10:
        QueueCharacterEvent(character, g_effect_005ee5ac, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 0x11:
        events[0] = g_special_event_0068c538;
        events[1] = g_special_event_0068c540;
        events[2] = g_special_event_0068c564;
        QueueCharacterEvent(character, events[Random(3)], 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
        return;
    case 0x12:
        gXStatus.character_event_queue->RemoveCharacterEvents(character);
        events[0] = g_special_event_0068c538;
        events[1] = g_special_event_0068c540;
        events[2] = g_special_event_0068c564;
        QueueCharacterEvent(character, events[Random(3)], g_effect_argument_005ed8d4,
                            g_effect_argument_005ed8cc, g_effect_argument_005ed914);
        return;
    case 0x13:
        excluded_slot = CharacterPointerToPartySlot(character);
        reaction = g_effect_005ee628;
        if ((reaction < g_normal_event_count_005ee70c ||
             (g_first_remapped_event_005ee718 <= reaction &&
              reaction < g_remapped_event_count_005ee710 + g_first_remapped_event_005ee718)) &&
            (slot = GetRandomCharacter(0, 0, excluded_slot, -1)) != -1) {
            attempts = 0;
            while (CanDispatchCharacterEvent(slot, reaction, 0) == 0) {
                if (attempts >= 0x32) {
                    return;
                }
                slot = GetRandomCharacter(0, 0, excluded_slot, -1);
                ++attempts;
            }
            QueueCharacterEvent(&g_status_685170.buffers.characters[slot], reaction, 0,
                                g_effect_argument_005ed8cc, g_effect_argument_005ed914);
            return;
        }
        break;
    }
}

/* Reacts to a condition being lifted: when nothing remains as the highest
   condition the character announces full recovery (0x55), otherwise the
   surviving-condition line (0x54); selected conditions map to fixed events. */
// FUNCTION: WIZ8 0x0052F790
void QueueConditionClearedReaction(W8Character* character, int condition)
{
    if (IsSedexusCaptureActive() != 0) {
        return;
    }
    if (g_status_685170.skip_next_condition_reaction != 0) {
        g_status_685170.skip_next_condition_reaction = 0;
        return;
    }
    switch (condition) {
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 0xb:
    case 0xc:
    case 0xe:
    case 0x10:
    case 0x11:
        if (character->highest_condition == 0) {
            QueueCharacterEvent(character, g_effect_005ee6dc, 0, g_effect_argument_005ed8cc,
                                g_effect_argument_005ed914);
            return;
        }
        QueueCharacterEvent(character, g_effect_005ee6d8, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        break;
    case 10:
    case 0x13:
        QueueCharacterEvent(character, g_effect_005ee5b4, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    case 0x12:
        QueueCharacterEvent(character, g_effect_005ee5b8, 0, g_effect_argument_005ed8cc,
                            g_effect_argument_005ed914);
        return;
    }
}

/* Requeue the stored portrait event of the selected character with a forced
   argument of 4; the selection and the stored type are cleared elsewhere. */
// FUNCTION: WIZ8 0x0052E480
void RequeueSelectedPortraitEvent(void)
{
    int slot = g_status_685170.selected_character;

    if (slot != -1) {
        unsigned int event_type = g_status_685170.buffers.party_rows[slot].pending_event_type_ff;
        if (event_type != 0) {
            QueueCharacterEvent(&g_status_685170.buffers.characters[slot], event_type, 4, 0, 0x7f);
        }
    }
}

/* Occupied slots with portrait_event_active set still have a portrait/voice record in
   flight; trap-trigger follow-up waits until none of those are active. */
// FUNCTION: WIZ8 0x0052E590
unsigned char PartyPortraitEventsIdle(void)
{
    W8PartySlotRow* row = g_status_685170.buffers.party_rows;
    const W8MonsterManagerEntry* current;

    for (current = gXStatus.monster_manager_entries; current < &gXStatus.monster_manager_entries[8];
         ++current, ++row) {
        if (row->occupied != 0 && current->portrait_event_active != 0) {
            return 0;
        }
    }
    return 1;
}

/* Advance the eight character portrait/voice records. This is the complete
   per-frame state machine: it drains finished owned events, starts the
   incapacitation path when no record is active, advances facing and pose
   clocks, and asks the current screen to redraw a changed slot. */
// FUNCTION: WIZ8 0x0052E750
int UpdateCharacterEventState(void)
{
    unsigned int party_slot;
    int any_active = 0;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        W8MonsterManagerEntry* record = &gXStatus.monster_manager_entries[party_slot];
        unsigned char sound_active = 0;

        if (g_status_685170.buffers.party_rows[party_slot].occupied == 0) {
            continue;
        }
        if (record->portrait_event_active != 0) {
            if (record->voice_sound_handle == -1) {
                if (record->voice_time_remaining_ms == 0) {
                    if (record->active_character_event == 0) {
                        SetPartyPortraitEventState(party_slot, 0, -1, 0, 1);
                    } else {
                        gXStatus.character_event_queue->CompleteActiveEvent(
                            record->active_character_event);
                    }
                }
            } else {
                UpdateMouthGapTrack(record->voice_sound_handle, &record->mouth_gap);
                sound_active = record->mouth_gap.mouth_open;
            }
        }

        if (record->portrait_event_active == 0) {
            unsigned int scan;
            for (scan = 0; scan < 8; ++scan) {
                if (g_status_685170.buffers.party_rows[scan].occupied != 0 &&
                    gXStatus.monster_manager_entries[scan].portrait_event_active != 0) {
                    break;
                }
            }
            if (scan == 8) {
                MaybeStartIncapacitationEvent(party_slot);
            }
        } else {
            W8Character* character = &g_status_685170.buffers.characters[party_slot];
            if ((character->highest_condition > 14 || character->hp_current == 0) &&
                record->active_character_event != 0) {
                gXStatus.character_event_queue->CompleteActiveEvent(record->active_character_event);
            }
            if (record->portrait_event_active == 0) {
                unsigned int scan;
                for (scan = 0; scan < 8; ++scan) {
                    if (g_status_685170.buffers.party_rows[scan].occupied != 0 &&
                        gXStatus.monster_manager_entries[scan].portrait_event_active != 0) {
                        break;
                    }
                }
                if (scan == 8) {
                    MaybeStartIncapacitationEvent(party_slot);
                }
            } else {
                any_active = 1;
                if (sound_active == 0) {
                    if (ClockIsTicking(record->portrait_frame_clock) == 0) {
                        if (record->voice_time_remaining_ms < 120) {
                            record->previous_portrait_frame = record->portrait_frame;
                            record->portrait_frame = 6;
                            record->portrait_pose_dirty = 1;
                            record->voice_time_remaining_ms = 0;
                        } else {
                            int direction = ChooseDifferentMonsterDirection004C2E00(
                                                (short)record->portrait_frame - 6) +
                                            6;
                            if (g_value_0068c57c <= record->field_113 &&
                                record->field_113 <= g_value_0068c554) {
                                direction = 8;
                            }
                            record->previous_portrait_frame = record->portrait_frame;
                            record->portrait_frame = direction;
                            record->portrait_pose_dirty = 1;
                            record->portrait_frame_clock = SetCountdownClock(120);
                            record->voice_time_remaining_ms -= 120;
                        }
                    }
                } else {
                    record->previous_portrait_frame = record->portrait_frame;
                    record->portrait_frame = 6;
                    record->portrait_pose_dirty = 1;
                }
            }
        }

        if (gXStatus.fNpcDialogueMode != 0 && (party_slot & 1) != 0 &&
            IsPortraitObscuredByNpcDialogue(party_slot) != 0) {
            continue;
        }
        if (record->portrait_frame_dirty == 0 && record->field_0bd == 0 &&
            (g_current_screen_state.id != W8_SCREEN_CHARACTER ||
             record->portrait_event_active != 0)) {
            if (record->portrait_pose_animation_active == 0) {
                if (record->portrait_pose == record->target_portrait_pose) {
                    if (record->portrait_pose == 1 &&
                        ClockIsTicking(record->portrait_idle_clock) == 0) {
                        record->portrait_pose_animation_active = 1;
                        record->portrait_idle_clock = SetCountdownClock(Random(5000) + 5000);
                    }
                } else if (ClockIsTicking(record->portrait_pose_clock) == 0) {
                    int pose = record->portrait_pose;
                    record->previous_portrait_pose = pose;
                    record->portrait_pose =
                        g_portrait_tables_0061cb3c
                            .pose_transition[pose * 5 + record->target_portrait_pose];
                    record->portrait_pose_animation_active = 1;
                    record->portrait_pose_clock = SetCountdownClock(Random(50) + 50);
                }
            } else if (ClockIsTicking(record->portrait_pose_clock) == 0) {
                int pose = record->portrait_pose;
                if (pose != 2) {
                    record->previous_portrait_pose = pose;
                    record->portrait_pose =
                        g_portrait_tables_0061cb3c.pose_transition[pose * 5 + 2];
                    record->portrait_pose_animation_active = 1;
                    record->portrait_pose_clock = SetCountdownClock(Random(50) + 50);
                }
                if (record->portrait_pose == 2) {
                    record->portrait_pose_animation_active = 0;
                }
            }
            if ((record->portrait_pose_animation_active != 0 || record->portrait_pose_dirty != 0) &&
                record->field_0cf == 0) {
                RefreshPartySlotDisplay(party_slot);
            }
        }
    }
    return any_active;
}

/* Draw one party member's portrait at a screen position. When the caller asks
   for the animated form the frame blitter runs first, and the death,
   in-combat or exhausted state adds the darkened overlay. */
// FUNCTION: WIZ8 0x0052eb00
void RenderPartyPortrait0052EB00(int portrait, int left, int top, int flags, int value,
                                 int party_slot)
{
    DrawCatalogImage(-0xe, 0x12, portrait, 0, left, top, flags | 0x200, 0);
    if (party_slot == -1) {
        return;
    }
    if (value != 0 && g_portrait_frame_flags_0061cbc0[portrait] != 0) {
        char drawn = BlitPartyPortraitAnimation(portrait, left, top, flags, party_slot, 1);
        value = drawn == 0;
    }
    if ((((gXStatus.fCombatMode != 0 && g_combat_state->characters[party_slot].flag_34 != 0) ||
          gXStatus.fSurprisePossible != 0) ||
         g_status_685170.buffers.characters[party_slot].highest_condition == 0x13) &&
        value != 0) {
        ShadowVideoSurfaceRect(-0xe, left, top, left + 0x59, top + 0x47);
    }
}

/* Blit the two animated frame tracks for one party slot and mark the union
   of each track's old and new frames dirty. The B track draws the current
   frame and its blend predecessor; the A track draws the frame the animation
   is moving to, and a dead character stops after the B track. */
// FUNCTION: WIZ8 0x0052ebe0
char BlitPartyPortraitAnimation(int portrait, int left, int top, int flags, int party_slot,
                                char animate)
{
    W8MonsterManagerEntry* state = &gXStatus.monster_manager_entries[party_slot];
    W8ScreenRect rect;
    W8ScreenRect other;
    short width;
    short height;
    short image_x;
    short image_y;
    char drawn = 0;

    if (g_portrait_frame_flags_0061cbc0[portrait] == 0) {
        return drawn;
    }
    if (state->portrait_pose_dirty != 0 || state->portrait_pose != state->previous_portrait_pose ||
        (animate != 0 && state->portrait_pose != 1)) {
        GetCatalogImageSize(0x12, portrait, state->portrait_pose, &width, &height);
        GetCatalogImagePosition00549700(0x12, portrait, state->portrait_pose, &image_x, &image_y);
        rect.left = image_x + left;
        rect.top = image_y + top;
        rect.right = width + rect.left;
        rect.bottom = height + rect.top;
        if (animate == 0 &&
            ((gXStatus.fCombatMode != 0 && g_combat_state->characters[party_slot].flag_34 != 0) ||
             gXStatus.fSurprisePossible != 0)) {
            RenderPartyPortrait0052EB00(portrait, left, top, flags, 0, party_slot);
        }
        if (g_status_685170.buffers.characters[party_slot].hp_current == 0) {
            return 1;
        }
        DrawCatalogImage(-0xe, 0x12, portrait, state->portrait_pose, left, top, flags | 0x200, 0);
        drawn = 1;
        if (state->previous_portrait_pose != -1) {
            GetCatalogImageSize(0x12, portrait, state->previous_portrait_pose, &width, &height);
            GetCatalogImagePosition00549700(0x12, portrait, state->previous_portrait_pose, &image_x,
                                            &image_y);
            other.left = image_x + left;
            other.top = image_y + top;
            other.right = width + other.left;
            other.bottom = height + other.top;
            UnionScreenRects(&other, &rect, &rect);
        }
        InvalidateScreenRects(&rect, 1, 0);
        state->previous_portrait_pose = state->portrait_pose;
        state->portrait_pose_dirty = 0;
    }
    if (state->portrait_frame_dirty == 0 &&
        state->portrait_frame == state->previous_portrait_frame &&
        (animate == 0 || state->portrait_frame == 6)) {
        if (drawn == 0) {
            return 0;
        }
    } else {
        GetCatalogImageSize(0x12, portrait, state->portrait_frame, &width, &height);
        GetCatalogImagePosition00549700(0x12, portrait, state->portrait_frame, &image_x, &image_y);
        if (animate == 0 && drawn == 0 && gXStatus.fCombatMode != 0 &&
            g_combat_state->characters[party_slot].flag_34 != 0) {
            RenderPartyPortrait0052EB00(portrait, left, top, flags, 0, party_slot);
        }
        DrawCatalogImage(-0xe, 0x12, portrait, state->portrait_frame, left, top, flags, 0);
        rect.left = image_x + left;
        rect.top = image_y + top;
        rect.right = width + rect.left;
        rect.bottom = height + rect.top;
        drawn = 1;
        if (state->previous_portrait_frame != -1) {
            GetCatalogImageSize(0x12, portrait, state->previous_portrait_frame, &width, &height);
            GetCatalogImagePosition00549700(0x12, portrait, state->previous_portrait_frame,
                                            &image_x, &image_y);
            other.left = image_x + left;
            other.top = image_y + top;
            other.right = width + other.left;
            other.bottom = height + other.top;
            UnionScreenRects(&other, &rect, &rect);
        }
        InvalidateScreenRects(&rect, 1, 0);
        state->previous_portrait_frame = state->previous_portrait_frame;
        state->portrait_frame_dirty = 0;
    }
    if (((gXStatus.fCombatMode != 0 && g_combat_state->characters[party_slot].flag_34 != 0) ||
         gXStatus.fSurprisePossible != 0) ||
        g_status_685170.buffers.characters[party_slot].highest_condition == 0x13) {
        ShadowVideoSurfaceRect(-0xe, left, top, left + 0x59, top + 0x47);
    }
    return drawn;
}

// FUNCTION: WIZ8 0x0052FD80
unsigned char PartyPortraitEventRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned short callback_id = region->callback_id;
    W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[callback_id];
    W8CharacterEvent* active;

    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        break;
    case LEFT_BUTTON_UP:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            if (callback_id != 0 && callback_id != 1) {
                active = entry->active_character_event;
                if (active != 0) {
                    gXStatus.character_event_queue->CompleteActiveEvent(active);
                    return 1;
                }
            }
            TryFinishNpcVoicePlayback(1);
            return 1;
        }
        break;
    default:
        return 0;
    }
    return 1;
}
