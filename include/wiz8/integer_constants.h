#ifndef WIZ8_INTEGER_CONSTANTS_H
#define WIZ8_INTEGER_CONSTANTS_H

/* Integer globals the image keeps in read-only data rather than as immediate
   operands. A body reads one of these from its address: with the initializer
   in view, C++ would fold the value into the instruction, and would initialize
   a global computed from it statically where retail computes it at startup.
   So the definitions live in their own translation unit, which reads none of
   them. */

extern const int g_character_event_no_flags;
extern const int g_effect_argument_005ed8cc;
extern const int g_effect_argument_005ed8d0;
extern const int g_effect_argument_005ed8d4;
extern const int g_effect_argument_005ed8d8;
extern const unsigned int g_character_event_no_npc_defer;
extern const unsigned char g_character_event_flags_mask; /* Gates the portrait quote flag. */
extern const unsigned int g_character_event_no_preempt;
extern const unsigned int g_character_event_npc_script;
extern const unsigned int g_flee_hp_fraction;
extern const unsigned int g_value_005ed8fc;
extern const unsigned int g_effect_threshold_005ed900;
extern const unsigned int g_effect_threshold_005ed904;
extern const unsigned int g_flee_chance;
extern const int g_character_event_full_volume;
extern const int g_effect_005ee588;
extern const int g_effect_005ee58c;
extern const int g_effect_005ee590;
extern const int g_effect_005ee594;
extern const int g_effect_005ee598;
extern const int g_condition_reaction;
extern const int g_condition_reaction_alt;
extern const int g_effect_005ee5a4;
extern const int g_camp_overload_event_id;
extern const int g_effect_005ee5ac;
extern const int g_effect_005ee5b4;
extern const int g_effect_005ee5b8;
extern const int g_effect_005ee5bc;
extern const int g_item_message_005ee5c8;
extern const int g_item_message_005ee5cc;
extern const int g_effect_005ee5d0;
extern const int g_effect_005ee5d4;
extern const int g_effect_005ee5d8;
extern const int g_effect_005ee5dc;
extern const int g_effect_005ee5e0;
extern const int g_search_found_item_event;
extern const int g_search_found_item_event_alt;
extern const int g_effect_005ee5ec;
extern const int g_effect_005ee5f0;
extern const int g_effect_005ee5f8;
extern const int g_effect_005ee5fc;
extern const int g_effect_005ee600;
extern const int g_effect_005ee604;
extern const int g_effect_005ee60c;
extern const int g_effect_005ee610;
extern const int g_effect_005ee614;
extern const int g_effect_005ee618; /* Queued for the allied NPC's death. */
extern const int g_effect_005ee61c;
extern const int g_sight_effect_005ee620;
extern const int g_effect_005ee624;
extern const int g_effect_005ee628;
extern const int g_effect_005ee630; /* Queued for eligible party members in that pass. */
extern const int g_effect_005ee634;
extern const int g_effect_005ee638;
extern const int g_item_message_005ee640;
extern const int g_item_message_005ee644;
extern const int g_item_message_005ee648;
extern const int g_item_message_005ee64c;
extern const int g_effect_005ee654;
extern const int g_effect_005ee658; /* Rest benefit after the surprise sequence. */
extern const int g_character_event_kind_005ee65c;
extern const int g_item_message_005ee664;
extern const int g_item_message_005ee668;
extern const int g_sight_effect_005ee66c;
extern const int g_item_message_005ee68c;
extern const int g_item_message_005ee690;
extern const int g_sight_effect_005ee694;
extern const int g_sight_effect_005ee698;
extern const int g_effect_005ee69c;
extern const int g_world_action_quote_min;
extern const int g_world_action_quote_max;
extern const int g_effect_005ee6d8;
extern const int g_effect_005ee6dc;
extern const int g_effect_005ee6ec;
extern const int g_effect_005ee6f8;
extern const int g_item_message_005ee6fc;
extern const unsigned int g_remapped_event_count;
extern const unsigned int g_first_remapped_event;
extern const int g_info_dialog_x;
extern const int g_info_dialog_y;
extern const int g_split_dialog_confirm;
extern const int g_split_dialog_origin_x;
extern const int g_split_dialog_origin_y;
extern const int g_split_result_kind;
extern const int g_split_dialog_x;
extern const int g_split_dialog_y;
extern const int g_split_dialog_kind;
extern const int g_split_dialog_sell_kind;
extern const int g_split_dialog_buy_kind;

#endif
