/* Layout evidence for include/wiz8/engine_code/Navigator.h. Included after its declarations. */

static_assert(sizeof(W8NavigatorAttachment) == 0x60, "W8NavigatorAttachment_size_must_be_0x60");
static_assert(offsetof(W8NavigatorAttachment, path_cursor) == 0x04,
              "W8NavigatorAttachment_path_cursor");
static_assert(offsetof(W8NavigatorAttachment, position_cursor) == 0x06,
              "W8NavigatorAttachment_position_cursor");
static_assert(offsetof(W8NavigatorAttachment, path_position_index) == 0x08,
              "W8NavigatorAttachment_path_position_index");
static_assert(offsetof(W8NavigatorAttachment, capacity_0a) == 0x0a,
              "W8NavigatorAttachment_capacity_0a");
static_assert(offsetof(W8NavigatorAttachment, position_10) == 0x10,
              "W8NavigatorAttachment_position_10");
static_assert(offsetof(W8NavigatorAttachment, position_1c) == 0x1c,
              "W8NavigatorAttachment_position_1c");
static_assert(offsetof(W8NavigatorAttachment, start_waypoint) == 0x28,
              "W8NavigatorAttachment_position_28");
static_assert(offsetof(W8NavigatorAttachment, position_34) == 0x34,
              "W8NavigatorAttachment_position_34");
static_assert(offsetof(W8NavigatorAttachment, position_40) == 0x40,
              "W8NavigatorAttachment_position_40");
static_assert(offsetof(W8NavigatorAttachment, position_4c) == 0x4c,
              "W8NavigatorAttachment_position_4c");
static_assert(offsetof(W8NavigatorAttachment, path_values) == 0x50,
              "W8NavigatorAttachment_path_values");
static_assert(offsetof(W8NavigatorAttachment, separation_54) == 0x54,
              "W8NavigatorAttachment_separation_54");
static_assert(offsetof(W8NavigatorAttachment, path_length) == 0x58,
              "W8NavigatorAttachment_path_length");
static_assert(sizeof(W8NavigatorMovementState) == 0xcc,
              "W8NavigatorMovementState_size_must_be_0xcc");
static_assert(offsetof(W8NavigatorMovementState, location_id_004) == 0x04,
              "W8NavigatorMovementState_location_id_004");
static_assert(offsetof(W8NavigatorMovementState, yaw) == 0x14, "W8NavigatorMovementState_yaw");
static_assert(offsetof(W8NavigatorMovementState, target_yaw) == 0x18,
              "W8NavigatorMovementState_target_yaw");
static_assert(offsetof(W8NavigatorMovementState, pitch_020) == 0x20,
              "W8NavigatorMovementState_pitch_020");
static_assert(offsetof(W8NavigatorMovementState, target_pitch) == 0x24,
              "W8NavigatorMovementState_target_pitch");
static_assert(offsetof(W8NavigatorMovementState, roll_028) == 0x28,
              "W8NavigatorMovementState_roll_028");
static_assert(offsetof(W8NavigatorMovementState, target_roll) == 0x2c,
              "W8NavigatorMovementState_target_roll");
static_assert(offsetof(W8NavigatorMovementState, velocity_034) == 0x34,
              "W8NavigatorMovementState_velocity_034");
static_assert(offsetof(W8NavigatorMovementState, position_040) == 0x40,
              "W8NavigatorMovementState_position_040");
static_assert(offsetof(W8NavigatorMovementState, target_position) == 0x4c,
              "W8NavigatorMovementState_target_position");
static_assert(offsetof(W8NavigatorMovementState, callback_threshold) == 0x58,
              "W8NavigatorMovementState_callback_threshold");
static_assert(offsetof(W8NavigatorMovementState, callback_progress) == 0x5c,
              "W8NavigatorMovementState_callback_progress");
static_assert(offsetof(W8NavigatorMovementState, movement_scale) == 0x60,
              "W8NavigatorMovementState_movement_scale");
static_assert(offsetof(W8NavigatorMovementState, movement_speed) == 0x64,
              "W8NavigatorMovementState_movement_speed");
static_assert(offsetof(W8NavigatorMovementState, turn_rate_068) == 0x68,
              "W8NavigatorMovementState_turn_rate_068");
static_assert(offsetof(W8NavigatorMovementState, flags_06c) == 0x6c,
              "W8NavigatorMovementState_flags_06c");
static_assert(offsetof(W8NavigatorMovementState, pitch_enabled) == 0x74,
              "W8NavigatorMovementState_pitch_enabled");
static_assert(offsetof(W8NavigatorMovementState, roll_enabled) == 0x75,
              "W8NavigatorMovementState_roll_enabled");
static_assert(offsetof(W8NavigatorMovementState, vertical_velocity) == 0x78,
              "W8NavigatorMovementState_vertical_velocity");
static_assert(offsetof(W8NavigatorMovementState, vertical_base) == 0x7c,
              "W8NavigatorMovementState_vertical_base");
static_assert(offsetof(W8NavigatorMovementState, vertical_amplitude) == 0x80,
              "W8NavigatorMovementState_vertical_amplitude");
static_assert(offsetof(W8NavigatorMovementState, vertical_phase) == 0x84,
              "W8NavigatorMovementState_vertical_phase");
static_assert(offsetof(W8NavigatorMovementState, basis) == 0x88,
              "W8NavigatorMovementState_basis");
static_assert(offsetof(W8NavigatorMovementState, attachment_0ac) == 0xac,
              "W8NavigatorMovementState_attachment_0ac");
static_assert(offsetof(W8NavigatorMovementState, collision_radius) == 0xb0,
              "W8NavigatorMovementState_collision_radius");
static_assert(offsetof(W8NavigatorMovementState, alternate_radius) == 0xb4,
              "W8NavigatorMovementState_alternate_radius");
static_assert(offsetof(W8NavigatorMovementState, height_offset) == 0xb8,
              "W8NavigatorMovementState_height_offset");
static_assert(offsetof(W8NavigatorMovementState, secondary_height_offset) == 0xbc,
              "W8NavigatorMovementState_secondary_height_offset");
static_assert(offsetof(W8NavigatorMovementState, vertical_offset) == 0xc0,
              "W8NavigatorMovementState_vertical_offset");
static_assert(offsetof(W8NavigatorMovementState, scale_0c4) == 0xc4,
              "W8NavigatorMovementState_scale_0c4");
static_assert(offsetof(W8NavigatorMovementState, position_adjusted) == 0xc8,
              "W8NavigatorMovementState_position_adjusted");
static_assert(sizeof(W8Navigator) == 0x190, "W8Navigator_size_must_be_0x190");
static_assert(offsetof(W8Navigator, navigation_mode_008) == 0x08,
              "W8Navigator_navigation_mode_008");
static_assert(offsetof(W8Navigator, flags_00c) == 0x0c, "W8Navigator_flags_00c");
static_assert(offsetof(W8Navigator, collision_margin) == 0x10,
              "W8Navigator_collision_margin");
static_assert(offsetof(W8Navigator, movement_0c0) == 0xc0, "W8Navigator_movement_0c0");
static_assert(offsetof(W8Navigator, node) == 0x18c, "W8Navigator_node");
