#pragma once
#include "wiz8/wiz8_windows.h"

#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/render_state.h"
#include "surrender/srCore.h"
#include "surrender/srNode.h"
#include "surrender/srTexture.h"

#include <string.h>

/* Shared declarations for address-interval quarantine units. Definitions live
   only in the physical source selected by source-path or bounded-gap evidence. */

/* Only the virtual destructor is established: the call goes through slot 0 with
   the deleting flag set, which is what `delete` on a polymorphic object emits.
   No field is known, so none is modelled. */
struct W8Releasable {
    virtual ~W8Releasable();
};

/* Only that one method is established, and nothing here shows a field. */
struct W8Forwarded {
    void Method4C5290();
};

extern "C" {

extern W8World* g_world_00659ab4;
extern int g_save_flag_00687599;

extern void Function4C4EF0(void);
extern void Function4A7A70(int value);

extern int g_clip_left_600078;
extern int g_clip_top_60007c;
extern int g_clip_right_600080;
extern int g_clip_bottom_600084;

extern int g_dword_6874f7;
extern unsigned long g_tick_65b9a8;

/* Releases through the virtual destructor, tolerating a null. */

/* The original holds the argument in ecx where this holds it in eax. Declaring
   it as a pointer does not change that - VC6 picks eax either way - so the
   argument stays an int and the register is left as the wobble it is. */

/* Records a value and stamps it with the tick it was recorded at. */

/* Takes an argument the decompiler does not show and hands it on in ecx, so the
   callee is a method and this is a forwarder, not the nullary call it looks
   like. Nothing follows the call, so it leaves as a jump. */

/* Acts only when both arguments are set, and passes the second on. */

/* The first argument is dead: the list comes from the global, not the caller.
   Both are reproduced because the original takes and ignores it. */


/* Builds the version banner into the caller's buffer. The version itself is
   unconditional; the title, the build number and the timestamp are each gated by
   their own flag, which is how the main menu asks for the bare "v1.2.4" while
   0x004E2F40 asks for all four parts. The constants are inline here exactly as
   they are there - nothing reads them from a resource. */


/* Reports whether the current drive has at least 256MB free. The byte counts
   are signed: the original divides them through __alldiv, where an unsigned
   quantity divided by 0x100000 would have been shifted instead. The verdict
   goes through a byte local as well: returning the comparison directly widens
   it to a 32-bit 0/1 and the original computes it in al.
   GetDiskFreeSpaceExA is resolved by name rather than linked, because it is
   absent on the earliest shells this shipped for; when it is missing the older
   call is multiplied out by hand instead, which is why the fallback carries the
   64-bit helpers. */


/* Hands back the current clip rectangle. The four globals are written as one
   block by the callers that set it, and read as one here. */

}

/*
 * Class registry slots for the host-registered classes.
 *
 * Each pair is a getClassName returning the class's own original name and a
 * getClassID returning a constant in the 0x10000 range SurRender reserves for
 * classes the host registers rather than its own - which is what makes the
 * literal the class's name and not a base it is presenting as. Nothing yet
 * places these bodies in a named translation unit, so they sit here.
 */













/* The one-level builder, identical in shape to Trigger's: no parent lookup,
   just the cache probe and a single registerClass with the concrete flag set. */

/* The name half alone: no id body sits in range, so the class's id stays
   unrecovered rather than guessed. */

/*
 * The same slot pair for seven classes that register under a SurRender base
 * name rather than one of their own.
 *
 * Each id is inside SurRender's own 0x1000-0x3110 range, so the literal names
 * the base the class presents as and not the class itself; the vtable each
 * pair sits in is the only thing that identifies them, which is what the
 * vtable-qualified names preserve. MonsterLight is the one already carried by
 * the reviewed class model, and its row reaches the same reading of these two
 * slots independently.
 */















/* Six id slots whose paired name slot is an import thunk into SR.DLL rather
   than an owned body, so only this half of the registry pair exists to
   recover. 0x00429CC0 is shared by two vtables: VC6 folded one emission. */







/* Three readers over globals other recovered units already own. Each is the
   whole body: one load and a return, with no guard, which is what separates
   them from the null-checked forwarders elsewhere in this file. The globals
   are reached through include/wiz8/render_state.h rather than re-declared
   here, so there is one declaration of each object in the tree. */



/* Narrower than the global it reads: the load is a byte, so only the low byte
   of the renderer mode reaches the caller. */

/*
 * Seven more whole-body reads over globals nothing else has claimed yet. Each
 * is a single load and a return. The names stay address-qualified because the
 * load is all the evidence there is: nothing here says what any of them mean,
 * only how wide they are - four of the seven load a byte, which is what types
 * those as byte-wide rather than truncated ints.
 */

extern "C" {
extern int g_value_65ba5c;
extern int g_value_65be60;
extern int g_value_64c1c8;
extern unsigned char g_flag_6850f6;
extern unsigned char g_flag_68c4fa;
extern unsigned char g_flag_68f104;
extern unsigned char g_flag_68f105;
}






/* An adjacent pair, read by two bodies that sit next to each other as well. */


/* One class registry node builder, written out rather than routed through the
   class_node helper in startup_cursor.cpp: the parent's name comes from the
   base's static sGetClassName rather than a literal, and the registry is
   fetched a second time inside the branch rather than reused, both of which
   the helper's shape cannot express. */

/* The same two-level shape for two more classes: each registers under a
   SurRender base whose own parent is srNode, so the fallback branch registers
   srNode first and hangs the class off it. */


/* The same builder with the parent named by a literal rather than a static
   getter, which is the whole of the two-byte difference against 0x0042A030:
   a pushed immediate is five bytes where the call is six. "srModel" is fixed
   by arithmetic - it occupies exactly the eight bytes between "srMeshModel"
   and "srTextureMap" in the string block. */

/* The third variant: the class's own name comes from a static getter as well
   as the parent's, which is the two bytes over 0x0042A030 - each call is six
   where a pushed literal is five. Both statics are imported by decorated name,
   so the pairing of id 0x1200 with srIlluminator is the original's own. */

/* The same two-level shape for stLevel, which also hangs directly off srNode. */

/* The two-level shape again, for the class that hangs off srNode as stLevel
   and stParticle do. */

/* The three-level form of the registry builder, and the deepest one recovered
   so far: the class registers under srTexture, which registers under
   srTextureIFace, which registers under srClass. Each level probes the cache
   before building, so a chain already installed by a sibling costs one lookup.
   Only the innermost base is named by a literal - srTexture supplies its own
   name through its static getter - which is the same literal-versus-getter
   split the shallower variants show. */

/* The same chain for the sibling that loads a texture from a file. */

/* The deepest chain in the program: stLight registers under srLight, which
   registers under srIlluminator, which registers under srNode, which registers
   under srClass. The two SurRender classes that export a static name getter -
   srNode and srIlluminator - supply theirs through it, while srLight and this
   class are literals; that is why the body is 211 bytes rather than 155. The
   srIlluminator level is the same one MonsterLight's reviewed row spells out
   as srClassSupport<srIlluminator,srNode,0,0x1200>. */

/* Two vtable installs, seven bytes each. Written as members so the receiver
   arrives in ECX the way the originals take it; the free-function form the
   older MonsterInstallVtable5ED290 uses costs four bytes more because the
   object has to come off the stack first. */

extern "C" {
extern void* g_vtable_005ec138;
extern void* g_vtable_005ebfd0;
}



extern "C" {
extern void* g_vtable_005ec1d8;
extern void* g_vtable_005ecdb0;
}



/*
 * Two more byte reads and four more panel redraws.
 *
 * The redraws are the shape RedrawRcsPanelA already proved exact: load the
 * panel pointer, reach its second vtable slot and pass the null rectangle that
 * spells "the whole area". The receiver is spelled Controls for that reason -
 * the slot index and the argument are what the shape fixes, and the proved
 * body is the only thing that names the class behind them.
 */

extern "C" {
extern unsigned char g_flag_69c808;
extern unsigned char g_flag_69da6c;
}

extern Controls* g_panel_69b940;
extern Controls* g_panel_69b998;
extern Controls* g_panel_69bf4c;
extern Controls* g_panel_69bf40;







/*
 * Whole-body writes over globals, the mirror image of the reads above: one
 * store and a return, with no guard. Each name stays address-qualified for the
 * same reason the readers' do - the store is the entire evidence, and it fixes
 * only the width. The globals that a recovered unit already owns are reached
 * through that unit's header rather than re-declared, so each object keeps one
 * declaration; only the ones nothing has claimed are declared here.
 */

extern "C" {
extern int g_value_68f2b0;
extern int g_value_68f2c4;
extern int g_value_69b988;
extern unsigned long g_value_64d8ac;
extern unsigned int g_region_set_69c528;
/* Dword-wide despite reading like a flag: the load is `mov eax`, not a byte
   load, which is what rules out the narrower type its one use suggests. */
extern int g_value_68c4c0;
extern void* g_pointer_689b40;
extern unsigned char g_flag_6081e4;
extern int g_value_659c14;
extern unsigned char g_table_647ccc[128];
/* Indexed row-major with a stride of eight, which is what makes the second
   subscript the inner one. */
extern unsigned char g_table_650434[][8];
extern int g_value_69da68;
extern int g_value_69b9a4;
extern int g_value_62a518;
extern int g_value_69c1cc;
extern int g_screen_transition_object_count_654aac;
extern float g_float_60ab48;
extern float g_float_64b914;
/* Three bytes of one dword-aligned run, read individually one address apart.
   Each load is a byte, which is what makes them three flags rather than one
   wider object - the reader at 0x0052E360 loads CL exactly as its two
   neighbours do. */
extern unsigned char g_flag_6850fa;
extern unsigned char g_flag_6850fb;
extern unsigned char g_flag_6850fc;
extern int g_value_6e4104;
extern unsigned char g_flag_603c60;
extern unsigned char g_flag_603c4c;
extern int g_value_659668;
extern int g_value_659ab4;
extern int g_value_652db0;
extern int g_value_60dfac;
extern int g_value_6834d4;
extern int g_value_689fac;
extern unsigned char g_flag_68c4f4;
extern unsigned char g_flag_68c4f7;
extern unsigned char g_flag_68c500;
}


/* The setter half of the pair whose getter is GetValue65962C above; both reach
   the one declaration in render_state.h. */




/* Writes the second world, the object whose one declaration gameplay_boundaries.h
   settled; the viewport's reads through it are what typed it. */

/* The one setter of this group that cleans its own argument: the body ends in
   `ret 4` rather than `ret`, which is __stdcall and the whole of the two-byte
   difference against the cdecl siblings above. */


/*
 * The same shape with the stored value fixed by the body rather than passed in.
 * Each is a store of a literal and a return.
 */

/* Stores the constant through AL rather than as an immediate, which is what
   VC6 does when the same constant is also the return value; the sibling at
   0x00529560 keeps the immediate form precisely because it returns nothing. */





/* An adjacent set/clear pair over one flag, the two bodies sitting next to each
   other as well. */



/* Writes both halves of the renderer-mode pair. The order is the source's: the
   later address stores first. */

/* Clears the flag and reports success with a constant the caller ignores at
   every recovered site; the byte return is what the store's width types. */


/* Steps the same counter 0x004B6D20 initialises and hands back the new value.
   The step runs through EAX rather than as an in-place `inc [mem]`, which is
   what returning it costs and the whole of the five-byte difference. */




/* The stored bit pattern is a float constant, not an int: 0x457A0000 is
   4000.0f, and the store is what types the object. */

/* Hands back the global's address rather than its value - one lea-shaped load
   of the constant and a return. */

/* A setter/getter pair over one float. The setter copies the four bytes
   straight through rather than loading the FPU, which is what keeps it the
   same length as the integer setters above. */


/*
 * Three predicates over globals. Each returns the comparison itself, which is
 * what widens the byte-sized result to a full 0/1 register.
 */





/*
 * Forwarders. Each is a call and a return, with the guard the original wrote
 * and nothing more; the callee in every case is a function some other unit
 * already declares, which is what keeps these bodies free of a second spelling
 * of anything.
 */

/* The region index is the body's own constant rather than an argument. */


/* Discards the ordering the API returns; the caller of this wrapper only ever
   needed the call made. */


/* Two stores over unrelated globals, the second a literal zero. */

/* Null-guarded forwarders onto the SurRender bases. The guard is the original's
   own: both callees are imports that would fault on a null receiver. */

/* Takes its owner in ECX rather than on the stack, which is the four bytes
   between this and the cdecl forwarders above. */

/* Indexed by a signed char, which is what makes the loaded byte reach the
   caller alongside the index's own sign in the upper bytes. */


/* Row-major lookup with a stride of eight; the scaled index is what fixes the
   inner dimension. */

/* Four more whole-body stores, each one store and a return. */





/* The same forwarder shape as 0x005A19A0, except the region set comes from a
   global rather than being written into the body. */

/* Reports the value zero rather than non-zero, which is the inverted sense
   against the predicates above.

   Three bytes long: the canonical loads the global into EAX and lets `sete al`
   land in the same register, where every source shape tried here loads ECX and
   zeroes EAX first. The `mov eax` form is the evidence the global is dword-wide
   rather than the byte its one use suggests, so the read itself is settled and
   only the register choice is not. */
