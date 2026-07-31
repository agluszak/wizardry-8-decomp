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
extern float g_float_005ebc3c;
extern float g_float_005ec390;
extern float g_float_005ec5c0;
extern float g_float_005ec5c4;
}

#endif
