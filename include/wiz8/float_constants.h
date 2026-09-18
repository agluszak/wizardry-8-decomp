#ifndef WIZ8_FLOAT_CONSTANTS_H
#define WIZ8_FLOAT_CONSTANTS_H

/* Float globals the image keeps as addressable storage rather than as
   immediate operands. A body that compares against one of these emits an FPU
   compare against its address; writing the literal instead lets VC6 fold the
   comparison into an integer test, which is how the difference shows up.

   They live here because more than one translation unit reads them, and one
   address must have one name: 0x005EBB38 had accumulated four - g_one,
   g_float, g_light_scale_identity twice - across GDCamera, Monster, PathAI,
   Navigator, Spells and the stLight unit. The names here are
   address-qualified on purpose: the same slot is read as a scale by one body,
   as a clamp bound by another and as a threshold by a third, so no role name
   is true of it. */

extern const float g_float_005ebb38;
extern const float g_float_005ebb34;
/* 0x005EE774: scales the record float into the group-engagement probe
   distance. */
extern const float g_float_005ee774;
/* 0x005EE77C: 7500.0, the floor added to the engagement range bound the
   group combat checks compare nearest-member distances against. */
extern const float g_float_005ee77c;
/* 0x005EE780: 1.15, the slack the reinforcement check gives a hostile
   monster's distance to the player before it counts as near the group. */
extern const float g_float_005ee780;
/* 0x005EBC64: the vertical offset the sight probes add before tracing. */
extern const float g_float_005ecb08;
extern const float g_float_005ecb20;
extern const double g_double_005ec8d8;
extern const double g_double_005ecb18;
extern const float g_float_005ecb10;
extern const float g_float_005ecb0c;
extern float g_float_005ebc64;
/* 10.0f - the per-tick cursor input-to-world scale. */
extern float g_float_005ebc88;
extern float g_float_005ebcdc;
extern double g_double_005ebc70;
extern float g_float_005ebc90;
extern double g_double_005ebc30;
extern const float g_float_005ec0a8;
extern const float g_environment_near_scale_005ec0b0;
extern const float g_world_scale_005ebc40;
/* 0x005EC510: 127.0, the SGP full-volume scale the positional-sound factory
   multiplies its loudness fraction by. */
extern const float g_float_005ec510;
extern const float g_float_005ec020;
extern float g_float_005ec1a8;
extern float g_float_005ec2f8;
/* 0x005EC35C: read as GetRangeConstant5EC35C's return and as OctPath's
   waypoint query vertical extent. */
extern float g_float_005ec35c;
/* 0x005EC360: 25000.0, read as a waypoint query half-extent by FindWaypoint
   and as a range bound by GetRangeConstant5EC360. */
extern float g_float_005ec360;
extern double g_double_005ec030;
/* 0x005EC038: 5000.0, the absolute vertical-snap ceiling that bounds
   FindNavigatorPosition's candidate rejection. */
extern double g_double_005ec038;
/* 0x005EC008: -pi/12, the fixed downward tilt applied to the sample camera by
   the region-link projector. */
extern double g_double_005ec008;
/* 0x005EC010: 6.282185, just under 2*pi — the circle-coverage bound the
   projector compares samples*fov against before adding one more direction. */
extern float g_float_005ec010;
/* 0x005EC044: 0.0004, the Random(1000) jitter scale used by the scatter-ring
   position search. */
extern float g_float_005ec044;
/* 0x005EC048: 15.0, the radius multiplier that sizes the monster-proximity
   query box in navigator placement. */
extern float g_float_005ec048;
/* 0x005EC050: 0.0002, the Random(1000) jitter scale for ring candidates. */
extern float g_float_005ec050;
/* 0x005EC1E8 / 0x005EC1F0: the quaternion->matrix normalization factor (2.0)
   and FLT_EPSILON closeness bound shared by the keyframe slerps. */
extern double g_double_005ec1e8;
extern double g_double_005ec1f0;
extern double g_double_005ec318;
extern double g_double_005ec368;
extern double g_double_005ec378;
/* 0x005EC428 / 0x005EC430: the pair BakeInstanceVertexLighting uses to undo
   srLight::setLinearAttenuation and recover a light's world range. */
extern double g_double_005ec428;
extern double g_double_005ec430;
extern float g_float_005ebc3c;
extern float g_float_005ebc58;
extern float g_float_005ebc60;
/* 0x005EBC78: 0.15, the along-ray distance discount the trace resolver
   applies before a sphere-hit candidate counts as closer than the world
   geometry. */
extern float g_float_005ebc78;
extern float g_float_005ebc7c;
/* Automap pan step as a fraction of the current zoom span. */
extern const float g_float_005ebcd8;
/* Search: the unit range the search score and collector scale against. */
extern float g_float_0061a364;
/* Search: the full facing cone the collector tests before line of sight. */
extern float g_float_0061a368;
extern float g_float_005ebccc;
extern float g_float_005ec028;
extern float g_float_005ec1a0;
extern float g_float_005ec38c;
extern float g_float_005ec384;
extern float g_float_005ec370;
extern float g_float_005ec390;
extern float g_float_005ec3b8;
extern float g_float_005ec3bc;
extern float g_float_005ec3c0;
extern float g_float_005ec3c8;
extern float g_float_005ec3d0;
extern float g_float_005ec410;
extern float g_float_005ec414;
extern float g_float_005ec438;
extern float g_float_005ec5c0;
extern float g_float_005ec5c4;
extern double g_double_005ec8d0;
extern float g_float_005ecc38;
extern float g_float_005ecc3c;
extern float g_float_005ecc40;
extern const float g_startup_near_limit_005ec000;
extern double g_double_005ebe80;
extern double g_double_005ebe88;
extern double g_double_005ebe90;
extern double g_double_005ebf40;
/* 0x005EC980: 0.25, the per-component weight that averages a trigger plane's
   four representation vectors into its center. */
extern const double g_double_005ec980;
extern const double g_double_005ebf60;
extern float g_float_005ec3f8;
extern float g_float_005ecbb4;
extern float g_float_005ed8b8;

/* The three quarter-turn values the world heading/elevation helpers read:
   0x005EC3FC and 0x005ED1E8 are the positive and negative half turns returned
   when the heading is exactly on the axis, and 0x005EC2A8 is the slightly
   different half turn the elevation helper subtracts. */
/* 0x005ECE50: the per-step homing decay AdvanceMissileAI multiplies into the
   record's turn budget. */
extern const double g_double_005ece50;
extern const float g_camera_half_pi_005ec3fc;
extern const float g_float_005ec2a8;
extern const float g_float_005ed1e8;

extern float g_navigator_gravity_00603acc;
/* 0x005EBCA4: Navigator's mode-3 step scale; the regeneration passes also read
   it as the pool-ceiling share. */
extern float g_navigator_mode3_scale_005ebca4;
extern float g_float_00603ab8;
extern float g_float_00603abc;
extern const float g_camera_snap_epsilon_005ebc2c;
extern const float g_float_005ebca0;
extern float g_movement_speed_step_005ed490;

extern const double g_zero_005ebb40;
extern const float g_camera_angle_period_005ec014;
extern const float g_float_005ebcf0;
extern const float g_float_005ebcf8;
extern const double g_double_005ec150;
/* 0x005EC240: 250000.0, squared camera-travel distance that triggers an
   automap cell refresh in UpdateWorldCameraAndPaths0044FC20. */
extern const double g_double_005ec240;
/* 0x005ED7B0: 1/360, the half-degree step the random wander angle is built
   from. */
extern const double g_double_005ed7b0;
/* 0x005EE768: 1500.0, the "close enough" distance for patrol points and heard
   noises. */
extern const double g_double_005ee768;
/* 0x005EECD0: 1/2500, the party-movement fatigue accumulator's
   accumulator-to-tick conversion. */
extern const float g_float_005eecd0;

#endif
