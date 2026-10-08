#ifndef WIZ8_FLOAT_CONSTANTS_H
#define WIZ8_FLOAT_CONSTANTS_H

/* Shared addressable floating-point constants. Zero and one serve several
   unrelated roles; other names describe established consumers. Each retail
   storage location has one declaration and one definition. */

extern const float g_float_one;
extern const float g_float_zero;
/* 0x005EBB30: 0.8, the radian bias subtracted from the near-camera scatter
   heading in PositionMonsterGroupNearCamera. */
extern const float g_monster_scatter_heading_bias;
/* 0x005ED828: 0.0016, the Random(1000) scale on the same scatter heading. */
extern const float g_monster_scatter_heading_random_scale;
/* 0x005EE774: scales the record float into the group-engagement probe
   distance. */
extern const float g_group_engagement_probe_scale;
/* 0x005EE77C: 7500.0, the floor added to the engagement range bound the
   group combat checks compare nearest-member distances against. */
extern const float g_monster_engagement_range_floor;
/* 0x005EE780: 1.15, the slack the reinforcement check gives a hostile
   monster's distance to the player before it counts as near the group. */
extern const float g_reinforcement_distance_slack;
/* 0x005ECB08: the vertical offset the cursor sight probes add before tracing. */
extern const float g_cursor_sight_probe_height;
extern const float g_cursor_ground_height_sentinel;
extern const double g_double_fifteen_and_five_eighths;
extern const double g_cursor_probe_shrink_scale;
extern const float g_cursor_ground_probe_lift;
extern const float g_cursor_ground_step_threshold;
extern const float g_float_one_thousand;
/* 10.0f - the per-tick cursor input-to-world scale. */
extern const float g_float_ten;
extern const float g_float_two_thousand;
extern const double g_double_one_ten_thousandth;
extern const float g_float_one_ten_thousandth;
extern const double g_double_one;
extern const float g_float_ten_thousand;
extern const float g_environment_near_scale;
extern const float g_world_scale;
/* 0x005EC5C: 1.05, the headroom factor applied to a fired missile's speed. */
extern const float g_monster_motion_push;
/* 0x005EC510: 127.0, the SGP full-volume scale the positional-sound factory
   multiplies its loudness fraction by. */
extern const float g_sound_node_full_volume;
extern const double g_path_overlap_tolerance;
extern const float g_float_negative_one_third;
extern const float g_float_fifty_thousand;
/* 0x005EC29C: pi/4, the arc bound the targeting cone tests compare
   normalized heading and elevation deltas against. */
extern const float g_targeting_quarter_pi;
extern const float g_float_five_thousand;
/* 0x005EC35C: read as GetGroundTargetRange's return and as OctPath's
   waypoint query vertical extent. */
extern const float g_float_twelve_thousand_five_hundred;
/* 0x005EC360: 25000.0, read as a waypoint query half-extent by FindWaypoint
   and as a range bound by GetMonsterEngagementRange. */
extern const float g_float_twenty_five_thousand;
extern const double g_double_twenty_five_hundred;
/* 0x005EC038: 5000.0, the absolute vertical-snap ceiling that bounds
   FindNavigatorPosition's candidate rejection. */
extern const double g_navigator_vertical_snap_limit;
/* 0x005EC008: -pi/12, the fixed downward tilt applied to the sample camera by
   the region-link projector. */
extern const double g_region_link_camera_tilt;
/* 0x005EC010: 6.282185, just under 2*pi — the circle-coverage bound the
   projector compares samples*fov against before adding one more direction. */
extern const float g_region_link_circle_coverage;
/* 0x005EC044: 0.0004, the Random(1000) jitter scale used by the scatter-ring
   position search. */
extern const float g_scatter_outer_ring_jitter_scale;
/* 0x005EC048: 15.0, the radius multiplier that sizes the monster-proximity
   query box in navigator placement. */
extern const float g_monster_proximity_radius_scale;
/* 0x005EC050: 0.0002, the Random(1000) jitter scale for ring candidates. */
extern const float g_scatter_inner_ring_jitter_scale;
/* 0x005EC1E8 / 0x005EC1F0: the quaternion->matrix normalization factor (2.0)
   and FLT_EPSILON closeness bound shared by the keyframe slerps. */
extern const double g_quaternion_matrix_normalization;
extern const double g_slerp_epsilon;
extern const double g_motion_full_turn_radians;
extern const double g_waypoint_edge_offset;
extern const double g_waypoint_marker_scale;
/* 0x005EC428 / 0x005EC430: the pair BakeInstanceVertexLighting uses to undo
   srLight::setLinearAttenuation and recover a light's world range. */
extern const double g_light_range_attenuation_numerator;
extern const double g_light_range_attenuation_denominator;
extern const float g_float_one_tenth;
extern const float g_vector_length_squared_epsilon;
extern const float g_float_one_five_hundredth;
/* 0x005EBC78: 0.15, the along-ray distance discount the trace resolver
   applies before a sphere-hit candidate counts as closer than the world
   geometry. */
extern const float g_float_fifteen_hundredths;
extern const float g_float_half;
/* Collision-response constants: hit-fraction floor/ceiling, surface margin
   scales, the level-flag-8 height lift, and the plane-similarity thresholds. */
extern const float g_collision_fraction_floor;
extern const float g_collision_slope_margin_scale;
extern const float g_collision_height_lift;
extern const float g_collision_ceiling_normal_threshold;
extern const double g_collision_direction_epsilon;
extern const float g_collision_parallel_normal_threshold;
extern const float g_collision_opposed_normal_threshold;
extern const float g_collision_contact_normal_threshold;
extern const double g_collision_centroid_scale;
/* Shadow-extrusion pitch scale: 1/1500 as a float. */
extern const float g_shadow_extrusion_pitch_scale;
extern const float g_camera_shake_intensity_base;
/* Automap pan step as a fraction of the current zoom span. */
extern const float g_float_thirty_five_hundredths;
/* Search: the unit range the search score and collector scale against. */
extern float g_search_radius;
/* Search: the full facing cone the collector tests before line of sight. */
extern float g_search_cone_angle;
extern const float g_float_three_quarters;
/* 0x005EC340: 1.25, the fast magic-recovery trait's spell-point regen scale. */
extern const float g_fast_magic_recovery_scale;
extern const float g_float_one_and_one_hundredth;
extern const float g_surface_flat_normal_threshold;
/* 0x005ED7A8: pi, the amplitude cursor-driven throw angles are scaled from. */
extern const double g_throw_angle_amplitude;
/* 0x005EBF4C: 71.0, the screen-z coefficient in the drop-item pitch. */
extern const float g_drop_item_pitch_scale;
/* 0x005EBC28: 5.0, a generic proximity/scale factor shared by navigation,
   monster-level math and the drop-item pitch. */
extern const float g_float_five;
/* 0x005EBF48: 85.0, the screen-y coefficient in the drop-item yaw. */
extern const float g_drop_item_yaw_scale;
/* 0x005ED7C0: -2500.0, the vertical scale of the drop-item direction. */
extern const double g_drop_item_vertical_scale;
extern const float g_path_direct_alignment_threshold;
extern const float g_waypoint_fallback_query_half_extent;
extern const float g_waypoint_marker_height;
extern const float g_float_nine_tenths;
extern const float g_path_heuristic_scale;
extern const float g_float_one_and_a_half;
extern const float g_path_gap_penalty_scale;
extern const float g_float_one_million;
extern const double g_path_blocking_alignment_upper_bound;
extern const double g_path_blocking_alignment_lower_bound;
extern const float g_float_one_third;
extern const float g_region_axis_alignment_threshold;
extern const float g_particle_random_unit_scale;
extern const float g_lod_level_zero_exit_threshold;
extern const float g_lod_level_two_exit_threshold;
extern const double g_double_one_thousandth;
extern const float g_particle_flutter_displacement_scale;
extern const float g_particle_flutter_velocity_floor;
extern const float g_particle_flutter_angle_random_scale;
extern const float g_startup_near_limit;
extern const double g_double_half;
extern const double g_inverse_screen_height;
extern const double g_inverse_screen_width;
extern const double g_double_three_quarters;
/* 0x005EC980: 0.25, the per-component weight that averages a trigger plane's
   four representation vectors into its center. */
extern const double g_double_quarter;
extern const double g_color_byte_scale;
extern const float g_float_one_hundred_twenty_five;
extern const float g_float_two_hundredths;
extern const float g_item_weight_display_scale;

/* The three quarter-turn values the world heading/elevation helpers read:
   0x005EC3FC and 0x005ED1E8 are the positive and negative half turns returned
   when the heading is exactly on the axis, and 0x005EC2A8 is the slightly
   different half turn the elevation helper subtracts. */
/* Per-step gravity subtracted from AdvanceMissileAI's vertical velocity. */
extern const double g_missile_gravity_acceleration;
/* 0x005ECE58/0x005ECE5C: the character launch height base and the per-slot
   vertical step GetCharacterProjectilePosition applies. */
extern const float g_character_projectile_height;
extern const float g_character_projectile_height_step;
extern const float g_camera_half_pi;
extern const float g_quarter_turn;
extern const float g_negative_quarter_turn;

extern float g_navigator_gravity;
/* 0x005EBCA4: Navigator's mode-3 step scale; the regeneration passes also read
   it as the pool-ceiling share. */
extern const float g_navigator_mode3_scale;
extern float g_default_momentum_scale;
extern float g_default_motion_limit;
extern const float g_camera_snap_epsilon;
/* 0x005ED2E0: 0.2, the perpendicular-alignment threshold the obstacle slide
   uses to pick a side. */
extern const double g_obstacle_slide_side_threshold;
extern const float g_float_six;
extern const float g_movement_speed_step;

extern const double g_double_zero;
/* 0x005ED7D0: -1000000, the ground-settle failure height; the only reader is
   the backfire scatter retry loop in Magic.cpp, so it owns the constant. */
extern const float g_ground_settle_fail;
extern const float g_camera_angle_period;
extern const float g_camera_radians_to_degrees;
extern const float g_float_inverse_half_turn_degrees;
extern const double g_double_five_hundred;
/* 0x005EC300: 1/180, the degrees-to-radians conversion shared by the camera
   view math and the missile aim scatter. Defined in GDCamera.cpp. */
extern const double g_camera_view_factor0;
/* 0x005EC240: 250000.0, squared camera-travel distance that triggers an
   automap cell refresh in UpdateWorldCameraAndPaths. */
extern const double g_automap_refresh_distance_squared;
/* Reciprocal degrees per full turn, used to scale randomized headings. */
extern const double g_inverse_full_turn_degrees;
/* 0x005EE768: 1500.0, the "close enough" distance for patrol points and heard
   noises. */
extern const double g_double_fifteen_hundred;
/* 0x005EECD0: 1/2500, the party-movement fatigue accumulator's
   accumulator-to-tick conversion. */
extern const float g_movement_fatigue_tick_scale;

#endif
