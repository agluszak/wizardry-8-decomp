#ifndef WIZ8_FLOAT_CONSTANTS_H
#define WIZ8_FLOAT_CONSTANTS_H

/* Float globals the image keeps as addressable storage rather than as
   immediate operands. A body that compares against one of these emits an FPU
   compare against its address; writing the literal instead lets VC6 fold the
   comparison into an integer test, which is how the difference shows up.

   They live here because more than one translation unit reads them, and one
   address must have one name: 0x005EBB38 had accumulated four - g_one,
   g_float, g_light_scale_identity twice - across GDCamera, Monster, PathAI,
   Navigator, Spells and the 0x0049BA51 quarantine unit. The names here are
   address-qualified on purpose: the same slot is read as a scale by one body,
   as a clamp bound by another and as a threshold by a third, so no role name
   is true of it. */

extern "C" {
extern float g_float_005ebb38;
extern double g_double_005ebc30;
extern const float g_float_005ec0a8;
extern float g_float_005ec020;
extern float g_float_005ec1a8;
extern float g_float_005ec2f8;
extern double g_double_005ec030;
extern double g_double_005ec318;
extern double g_double_005ec368;
extern double g_double_005ec378;
extern float g_float_005ebc3c;
extern float g_float_005ebc58;
extern float g_float_005ebc60;
extern float g_float_005ebc7c;
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
extern float g_float_005ec438;
extern float g_float_005ec5c0;
extern float g_float_005ec5c4;
extern double g_double_005ec8d0;
extern float g_float_005ecc38;
extern float g_float_005ecc3c;
extern float g_float_005ecc40;
}

extern const double g_zero_005ebb40;
extern const float g_camera_angle_period_005ec014;

#endif
