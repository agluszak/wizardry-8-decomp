#ifndef WIZ8_INTEGER_CONSTANTS_H
#define WIZ8_INTEGER_CONSTANTS_H

/* Integer globals the image keeps in read-only data rather than as immediate
   operands. A body reads one of these from its address: with the initializer
   in view, C++ would fold the value into the instruction, and would initialize
   a global computed from it statically where retail computes it at startup.
   So the definitions live in their own translation unit, which reads none of
   them. */

extern const int g_character_event_no_flags;
extern const int g_effect_argument0;
extern const int g_effect_argument1;
extern const int g_effect_argument2;
extern const int g_effect_argument3;
extern const unsigned int g_character_event_no_npc_defer;
extern const unsigned char g_character_event_flags_mask; /* Gates the portrait quote flag. */
extern const unsigned int g_character_event_no_preempt;
extern const unsigned int g_character_event_npc_script;
extern const unsigned int g_flee_hp_fraction;
extern const unsigned int g_value_005ed8fc;
extern const unsigned int g_effect_threshold0;
extern const unsigned int g_effect_threshold1;
extern const unsigned int g_flee_chance;
extern const int g_character_event_full_volume;
extern const int g_effect0;
extern const int g_effect1;
extern const int g_effect2;
extern const int g_effect3;
extern const int g_effect4;
extern const int g_condition_reaction;
extern const int g_condition_reaction_alt;
extern const int g_effect5;
extern const int g_camp_overload_event_id;
extern const int g_effect6;
extern const int g_effect7;
extern const int g_effect8;
extern const int g_effect9;
extern const int g_item_message0;
extern const int g_item_message1;
extern const int g_effect10;
extern const int g_effect11;
extern const int g_effect12;
extern const int g_effect13;
extern const int g_effect14;
extern const int g_search_found_item_event;
extern const int g_search_found_item_event_alt;
extern const int g_effect15;
extern const int g_effect16;
extern const int g_effect17;
extern const int g_effect18;
extern const int g_effect19;
extern const int g_effect20;
extern const int g_effect21;
extern const int g_effect22;
extern const int g_effect23;
extern const int g_effect24; /* Queued for the allied NPC's death. */
extern const int g_effect25;
extern const int g_sight_effect0;
extern const int g_effect26;
extern const int g_effect27;
extern const int g_effect28; /* Queued for eligible party members in that pass. */
extern const int g_effect29;
extern const int g_effect30;
extern const int g_item_message2;
extern const int g_item_message3;
extern const int g_item_message4;
extern const int g_item_message5;
extern const int g_effect31;
extern const int g_effect32; /* Rest benefit after the surprise sequence. */
extern const int g_character_event_kind2;
extern const int g_item_message6;
extern const int g_item_message7;
extern const int g_sight_effect1;
extern const int g_item_message8;
extern const int g_item_message9;
extern const int g_sight_effect2;
extern const int g_sight_effect3;
extern const int g_effect33;
extern const int g_world_action_quote_min;
extern const int g_world_action_quote_max;
extern const int g_effect34;
extern const int g_effect35;
extern const int g_effect36;
extern const int g_effect37;
extern const int g_item_message10;
extern const unsigned int g_remapped_event_count;
extern const unsigned int g_first_remapped_event;
extern const int g_info_dialog_x;
extern const int g_info_dialog_y;
extern const int g_split_dialog_origin_x;
extern const int g_split_dialog_origin_y;
extern const int g_split_dialog_x;
extern const int g_split_dialog_y;

#endif
