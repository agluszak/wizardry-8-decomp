/* Layout evidence for include/wiz8/layouts/main_game_screen.h. Included after its declarations. */

static_assert(sizeof(W8MainGameResourceSlot) == 0x14, "W8MainGameResourceSlot_size");
static_assert(offsetof(W8MainGameResourceSlot, image_id) == 0x00,
              "W8MainGameResourceSlot_image_id");
static_assert(offsetof(W8MainGameResourceSlot, object) == 0x04, "W8MainGameResourceSlot_object");
static_assert(offsetof(W8MainGameResourceSlot, frame_count) == 0x08,
              "W8MainGameResourceSlot_frame_count");
static_assert(offsetof(W8MainGameResourceSlot, size_x) == 0x0c, "W8MainGameResourceSlot_size_x");
static_assert(offsetof(W8MainGameResourceSlot, hotspot_x) == 0x10,
              "W8MainGameResourceSlot_hotspot_x");
static_assert(sizeof(W8DialogueTextState) == 0x34, "W8DialogueTextState_size");
static_assert(offsetof(W8DialogueTextState, saved_scroll_line) == 0x30,
              "W8DialogueTextState_saved_scroll_line");
static_assert(offsetof(W8LevelRuntimeBlock, text_paint_scratch_000) == 0x0,
              "W8LevelRuntimeBlock_text_paint_scratch_000");
static_assert(sizeof(W8LevelRuntimeBlock) == 0x330, "W8LevelRuntimeBlock_must_be_0x330");
static_assert(offsetof(W8LevelRuntimeBlock, message_box_pending_0f0) == 0x0f0,
              "W8LevelRuntimeBlock_message_box_pending_0f0");
static_assert(offsetof(W8LevelRuntimeBlock, saved_redraw_flags) == 0x0f8,
              "W8LevelRuntimeBlock_saved_redraw_flags");
static_assert(offsetof(W8LevelRuntimeBlock, main_ui_mode) == 0x0fc,
              "W8LevelRuntimeBlock_main_ui_mode");
static_assert(offsetof(W8LevelRuntimeBlock, portrait_refresh_pending) == 0x109,
              "W8LevelRuntimeBlock_portrait_refresh_pending");
static_assert(offsetof(W8LevelRuntimeBlock, portrait_refresh_image) == 0x114,
              "W8LevelRuntimeBlock_portrait_refresh_image");
static_assert(offsetof(W8LevelRuntimeBlock, portrait_refresh_mode) == 0x134,
              "W8LevelRuntimeBlock_portrait_refresh_mode");
static_assert(offsetof(W8LevelRuntimeBlock, party_slots_170) == 0x170,
              "W8LevelRuntimeBlock_party_slots_170");
static_assert(offsetof(W8LevelRuntimeBlock, portrait_assay_hover_mode) == 0x188,
              "W8LevelRuntimeBlock_portrait_assay_hover_mode");
static_assert(offsetof(W8LevelRuntimeBlock, formation_highlight_party_slot) == 0x18c,
              "W8LevelRuntimeBlock_formation_highlight_party_slot");
static_assert(offsetof(W8LevelRuntimeBlock, portrait_right_hold_armed) == 0x324,
              "W8LevelRuntimeBlock_portrait_right_hold_armed");
static_assert(offsetof(W8LevelRuntimeBlock, radar_map_alternate) == 0x326,
              "W8LevelRuntimeBlock_radar_map_alternate");
static_assert(offsetof(W8LevelRuntimeBlock, review_transition_active) == 0x327,
              "W8LevelRuntimeBlock_review_transition_active");
static_assert(offsetof(W8LevelRuntimeBlock, value_194) == 0x194, "W8LevelRuntimeBlock_value_194");
static_assert(offsetof(W8LevelRuntimeBlock, value_198) == 0x198, "W8LevelRuntimeBlock_value_198");
static_assert(offsetof(W8LevelRuntimeBlock, portrait_overlay_party_slot) == 0x200,
              "W8LevelRuntimeBlock_portrait_overlay_party_slot");
static_assert(offsetof(W8LevelRuntimeBlock, condition_orb_party_slot) == 0x204,
              "W8LevelRuntimeBlock_condition_orb_party_slot");
static_assert(offsetof(W8LevelRuntimeBlock, enchantment_orb_party_slot) == 0x208,
              "W8LevelRuntimeBlock_enchantment_orb_party_slot");
static_assert(offsetof(W8LevelRuntimeBlock, condition_highlight_party_slot) == 0x20c,
              "W8LevelRuntimeBlock_condition_highlight_party_slot");
static_assert(offsetof(W8LevelRuntimeBlock, flag_210) == 0x210, "W8LevelRuntimeBlock_flag_210");
static_assert(offsetof(W8LevelRuntimeBlock, clock_214) == 0x214, "W8LevelRuntimeBlock_clock_214");
static_assert(offsetof(W8LevelRuntimeBlock, countdown_258) == 0x258,
              "W8LevelRuntimeBlock_countdown_258");
static_assert(offsetof(W8LevelRuntimeBlock, countdown_26c) == 0x26c,
              "W8LevelRuntimeBlock_countdown_26c");
static_assert(offsetof(W8LevelRuntimeBlock, palette_2ec) == 0x2ec,
              "W8LevelRuntimeBlock_palette_2ec");
static_assert(offsetof(W8LevelRuntimeBlock, countdown_32c) == 0x32c,
              "W8LevelRuntimeBlock_countdown_32c");
