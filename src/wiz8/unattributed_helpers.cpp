#include "wiz8/wiz8_windows.h"

#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/render_state.h"
#include "surrender/srCore.h"
#include "surrender/srNode.h"
#include "surrender/srTexture.h"

#include <string.h>

/*
 * Recovered bodies whose original translation unit is not established yet.
 *
 * They are here rather than in bringup_gates.cpp because that file is named for
 * the bring-up path and none of these are on it; putting them there would
 * assert a grouping the evidence does not support. Each carries its own owner
 * in the boundary map, and any of them can move once its unit is identified.
 */

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
// FUNCTION: WIZ8 0x004C5860
void Function4C5860(W8Releasable* object)
{
    if (object != NULL) {
        delete object;
    }
}

/* The original holds the argument in ecx where this holds it in eax. Declaring
   it as a pointer does not change that - VC6 picks eax either way - so the
   argument stays an int and the register is left as the wobble it is. */
// FUNCTION: WIZ8 0x004C5ED0
void Function4C5ED0(int enabled)
{
    if (enabled != 0) {
        Function4C4EF0();
    }
}

/* Records a value and stamps it with the tick it was recorded at. */
// FUNCTION: WIZ8 0x00482720
void Function482720(int value)
{
    g_dword_6874f7 = value;
    g_tick_65b9a8 = GetTickCount();
}

/* Takes an argument the decompiler does not show and hands it on in ecx, so the
   callee is a method and this is a forwarder, not the nullary call it looks
   like. Nothing follows the call, so it leaves as a jump. */
// FUNCTION: WIZ8 0x004C5810
void Function4C5810(W8Forwarded* target)
{
    target->Method4C5290();
}

/* Acts only when both arguments are set, and passes the second on. */
// FUNCTION: WIZ8 0x004C59C0
void Function4C59C0(int enabled, int value)
{
    if (enabled != 0 && value != 0) {
        Function4A7A70(value);
    }
}

/* The first argument is dead: the list comes from the global, not the caller.
   Both are reproduced because the original takes and ignores it. */
// FUNCTION: WIZ8 0x0046E5A0
void Function46E5A0(int unused, void* item)
{
    PListRemove(g_world_00659ab4->plsList00, item);
}


/* Builds the version banner into the caller's buffer. The version itself is
   unconditional; the title, the build number and the timestamp are each gated by
   their own flag, which is how the main menu asks for the bare "v1.2.4" while
   0x004E2F40 asks for all four parts. The constants are inline here exactly as
   they are there - nothing reads them from a resource. */
// FUNCTION: WIZ8 0x004E3620
void Function4E3620(char* out, char with_title, char with_build, char with_date)
{
    out[0] = '\0';
    if (with_title) {
        strcat(out, "Wizardry 8 ");
    }
    strcat(out, FormatString("v%d.%d.%d", 1, 2, 4));
    if (with_build) {
        strcat(out, FormatString(" (build %d)", 0xdb));
    }
    if (with_date) {
        strcat(out, FormatString(" %s", "2001/12/24 15:36"));
    }
}


/* Reports whether the current drive has at least 256MB free. The byte counts
   are signed: the original divides them through __alldiv, where an unsigned
   quantity divided by 0x100000 would have been shifted instead. The verdict
   goes through a byte local as well: returning the comparison directly widens
   it to a 32-bit 0/1 and the original computes it in al.
   GetDiskFreeSpaceExA is resolved by name rather than linked, because it is
   absent on the earliest shells this shipped for; when it is missing the older
   call is multiplied out by hand instead, which is why the fallback carries the
   64-bit helpers. */
// FUNCTION: WIZ8 0x004298F0
unsigned char Function4298F0(void)
{
    FARPROC extended;
    LARGE_INTEGER available;
    LARGE_INTEGER capacity;
    LARGE_INTEGER free_bytes;
    DWORD sectors_per_cluster;
    DWORD bytes_per_sector;
    DWORD free_clusters;
    DWORD total_clusters;
    unsigned int megabytes;
    unsigned char enough;

    /* One nested expression, not two statements: the original pushes the
       procedure name before it calls GetModuleHandleA, which is what
       right-to-left argument evaluation of a single call gives. */
    extended = GetProcAddress(GetModuleHandleA("kernel32.dll"),
                              "GetDiskFreeSpaceExA");
    if (extended != NULL) {
        GetDiskFreeSpaceExA(NULL, (PULARGE_INTEGER)&available,
                            (PULARGE_INTEGER)&capacity, (PULARGE_INTEGER)&free_bytes);
        megabytes = (unsigned int)(free_bytes.QuadPart / 0x100000);
        enough = megabytes >= 0x100;
        return enough;
    }
    GetDiskFreeSpaceA(NULL, &sectors_per_cluster, &bytes_per_sector,
                      &free_clusters, &total_clusters);
    megabytes = (unsigned int)((__int64)sectors_per_cluster * bytes_per_sector
                               * free_clusters / 0x400 / 0x400);
    enough = megabytes >= 0x100;
    return enough;
}


/* Hands back the current clip rectangle. The four globals are written as one
   block by the callers that set it, and read as one here. */
// FUNCTION: WIZ8 0x00411820
void GetClipRect(int* rect)
{
    rect[0] = g_clip_left_600078;
    rect[1] = g_clip_top_60007c;
    rect[2] = g_clip_right_600080;
    rect[3] = g_clip_bottom_600084;
}

}

/*
 * Class registry slots for five host-registered classes.
 *
 * Each pair is a getClassName returning the class's own original name and a
 * getClassID returning a constant in the 0x10000 range SurRender reserves for
 * classes the host registers rather than its own - which is what makes the
 * literal the class's name and not a base it is presenting as. Nothing yet
 * places these bodies in a named translation unit, so they sit here.
 */

// FUNCTION: WIZ8 0x00485800
const char* stTextureAnim::getClassName() const
{
    return "stTextureAnim";
}

// FUNCTION: WIZ8 0x004857F0
unsigned long stTextureAnim::getClassID() const
{
    return 0x10000;
}

// FUNCTION: WIZ8 0x0047D6E0
const char* stTextureFile::getClassName() const
{
    return "stTextureFile";
}

// FUNCTION: WIZ8 0x0047D6D0
unsigned long stTextureFile::getClassID() const
{
    return 0x10001;
}

// FUNCTION: WIZ8 0x0049DC70
const char* stLight::getClassName() const
{
    return "stLight";
}

// FUNCTION: WIZ8 0x0049DC60
unsigned long stLight::getClassID() const
{
    return 0x10006;
}

// FUNCTION: WIZ8 0x004BA1C0
const char* stLevel::getClassName() const
{
    return "stLevel";
}

// FUNCTION: WIZ8 0x004BA1B0
unsigned long stLevel::getClassID() const
{
    return 0x10007;
}

// FUNCTION: WIZ8 0x004AF3E0
const char* stSound3D::getClassName() const
{
    return "stSound3D";
}

// FUNCTION: WIZ8 0x004AF3D0
unsigned long stSound3D::getClassID() const
{
    return 0x1000b;
}

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

// FUNCTION: WIZ8 0x0049DC30
const char* MonsterLight::getClassName() const
{
    return "srLight";
}

// FUNCTION: WIZ8 0x0049DC20
unsigned long MonsterLight::getClassID() const
{
    return 0x1220;
}

// FUNCTION: WIZ8 0x0042A020
const char* W8Camera005EBE14::getClassName() const
{
    return "srCamera";
}

// FUNCTION: WIZ8 0x0042A010
unsigned long W8Camera005EBE14::getClassID() const
{
    return 0x1400;
}

// FUNCTION: WIZ8 0x0042A0D0
const char* W8Scene005EBE48::getClassName() const
{
    return "srScene";
}

// FUNCTION: WIZ8 0x0042A0C0
unsigned long W8Scene005EBE48::getClassID() const
{
    return 0x1010;
}

// FUNCTION: WIZ8 0x00429B40
const char* W8MeshModel005EBE98::getClassName() const
{
    return "srMeshModel";
}

// FUNCTION: WIZ8 0x00429B30
unsigned long W8MeshModel005EBE98::getClassID() const
{
    return 0x2010;
}

// FUNCTION: WIZ8 0x00429BF0
const char* W8TextureMap005EBEEC::getClassName() const
{
    return "srTextureMap";
}

// FUNCTION: WIZ8 0x00429BE0
unsigned long W8TextureMap005EBEEC::getClassID() const
{
    return 0x2111;
}

// FUNCTION: WIZ8 0x00484710
const char* W8Fog005EC94C::getClassName() const
{
    return "srFog";
}

// FUNCTION: WIZ8 0x00484700
unsigned long W8Fog005EC94C::getClassID() const
{
    return 0x1210;
}

// FUNCTION: WIZ8 0x004BDF10
const char* W8ClipPlane005ED180::getClassName() const
{
    return "srClipPlane";
}

// FUNCTION: WIZ8 0x004BDF00
unsigned long W8ClipPlane005ED180::getClassID() const
{
    return 0x1500;
}

/* Six id slots whose paired name slot is an import thunk into SR.DLL rather
   than an owned body, so only this half of the registry pair exists to
   recover. 0x00429CC0 is shared by two vtables: VC6 folded one emission. */

// FUNCTION: WIZ8 0x004519D0
unsigned long W8Node005EC208::getClassID() const
{
    return 0x1000;
}

// FUNCTION: WIZ8 0x0049DB10
unsigned long W8Illuminator005ECCD8::getClassID() const
{
    return 0x1200;
}

// FUNCTION: WIZ8 0x00429A40
unsigned long W8Registered005EBD10::getClassID() const
{
    return 0x3110;
}

// FUNCTION: WIZ8 0x00429CC0
unsigned long W8Registered005EBDE0::getClassID() const
{
    return 0x2210;
}

// FUNCTION: WIZ8 0x00429E80
unsigned long W8Registered005EBF94::getClassID() const
{
    return 0x2200;
}

// FUNCTION: WIZ8 0x0047D650
unsigned long W8Registered005EC5D8::getClassID() const
{
    return 0x2900;
}

/* Three readers over globals other recovered units already own. Each is the
   whole body: one load and a return, with no guard, which is what separates
   them from the null-checked forwarders elsewhere in this file. The globals
   are reached through include/wiz8/render_state.h rather than re-declared
   here, so there is one declaration of each object in the tree. */

// FUNCTION: WIZ8 0x00421F30
IDirectDraw2* GetDirectDraw2(void)
{
    return g_direct_draw2_6596a0;
}

// FUNCTION: WIZ8 0x00427810
int GetValue65962C(void)
{
    return g_dword_65962c;
}

/* Narrower than the global it reads: the load is a byte, so only the low byte
   of the renderer mode reaches the caller. */
// FUNCTION: WIZ8 0x004291C0
unsigned char GetRendererModeByte(void)
{
    return (unsigned char)g_renderer_mode_603d74;
}

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

// FUNCTION: WIZ8 0x0048ED00
int GetValue65BA5C(void)
{
    return g_value_65ba5c;
}

// FUNCTION: WIZ8 0x004AFE90
int GetValue65BE60(void)
{
    return g_value_65be60;
}

// FUNCTION: WIZ8 0x00593320
int GetValue64C1C8(void)
{
    return g_value_64c1c8;
}

// FUNCTION: WIZ8 0x0047AE70
unsigned char GetFlag6850F6(void)
{
    return g_flag_6850f6;
}

// FUNCTION: WIZ8 0x0052A070
unsigned char GetFlag68C4FA(void)
{
    return g_flag_68c4fa;
}

/* An adjacent pair, read by two bodies that sit next to each other as well. */
// FUNCTION: WIZ8 0x0057DBC0
unsigned char GetFlag68F104(void)
{
    return g_flag_68f104;
}

// FUNCTION: WIZ8 0x0057DBB0
unsigned char GetFlag68F105(void)
{
    return g_flag_68f105;
}

/* One class registry node builder, written out rather than routed through the
   class_node helper in startup_cursor.cpp: the parent's name comes from the
   base's static sGetClassName rather than a literal, and the registry is
   fetched a second time inside the branch rather than reused, both of which
   the helper's shape cannot express. */
// FUNCTION: WIZ8 0x0042A030
srRegistry::ClassNode* W8Camera005EBE14::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1400);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("srCamera", parent, 0x1400, 0);
    }
    return node;
}

/* The same two-level shape for two more classes: each registers under a
   SurRender base whose own parent is srNode, so the fallback branch registers
   srNode first and hangs the class off it. */
// FUNCTION: WIZ8 0x0042A0E0
srRegistry::ClassNode* W8Scene005EBE48::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1010);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("srScene", parent, 0x1010, 0);
    }
    return node;
}

// FUNCTION: WIZ8 0x004BDF20
srRegistry::ClassNode* W8ClipPlane005ED180::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1500);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("srClipPlane", parent, 0x1500, 0);
    }
    return node;
}

/* The same builder with the parent named by a literal rather than a static
   getter, which is the whole of the two-byte difference against 0x0042A030:
   a pushed immediate is five bytes where the call is six. "srModel" is fixed
   by arithmetic - it occupies exactly the eight bytes between "srMeshModel"
   and "srTextureMap" in the string block. */
// FUNCTION: WIZ8 0x00429B50
srRegistry::ClassNode* W8MeshModel005EBE98::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x2010);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x2000);

        if (!parent) {
            parent = parent_registry->registerClass(
                "srModel", srClass::sGetClassNode(), 0x2000, 1);
        }
        node = registry->registerClass("srMeshModel", parent, 0x2010, 0);
    }
    return node;
}

/* The third variant: the class's own name comes from a static getter as well
   as the parent's, which is the two bytes over 0x0042A030 - each call is six
   where a pushed literal is five. Both statics are imported by decorated name,
   so the pairing of id 0x1200 with srIlluminator is the original's own. */
// FUNCTION: WIZ8 0x0049DB30
srRegistry::ClassNode* W8Illuminator005ECCD8::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1200);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass(
            srIlluminator::sGetClassName(), parent, 0x1200, 0);
    }
    return node;
}

/* The same two-level shape for stLevel, which also hangs directly off srNode. */
// FUNCTION: WIZ8 0x004BA1D0
srRegistry::ClassNode* stLevel::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10007);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("stLevel", parent, 0x10007, 0);
    }
    return node;
}

/* The two-level shape again, for the class that hangs off srNode as stLevel
   and stParticle do. */
// FUNCTION: WIZ8 0x004AF3F0
srRegistry::ClassNode* stSound3D::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1000b);

    if (!node) {
        srRegistry* parent_registry = srCore.getRegistry();
        srRegistry::ClassNode* parent = parent_registry->getClassNode(0x1000);

        if (!parent) {
            parent = parent_registry->registerClass(
                srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        node = registry->registerClass("stSound3D", parent, 0x1000b, 0);
    }
    return node;
}

/* The three-level form of the registry builder, and the deepest one recovered
   so far: the class registers under srTexture, which registers under
   srTextureIFace, which registers under srClass. Each level probes the cache
   before building, so a chain already installed by a sibling costs one lookup.
   Only the innermost base is named by a literal - srTexture supplies its own
   name through its static getter - which is the same literal-versus-getter
   split the shallower variants show. */
// FUNCTION: WIZ8 0x00485810
srRegistry::ClassNode* stTextureAnim::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10000);

    if (!node) {
        srRegistry* texture_registry = srCore.getRegistry();
        srRegistry::ClassNode* texture = texture_registry->getClassNode(0x2110);

        if (!texture) {
            srRegistry* iface_registry = srCore.getRegistry();
            srRegistry::ClassNode* iface = iface_registry->getClassNode(0x2100);

            if (!iface) {
                iface = iface_registry->registerClass(
                    "srTextureIFace", srClass::sGetClassNode(), 0x2100, 1);
            }
            texture = texture_registry->registerClass(
                srTexture::sGetClassName(), iface, 0x2110, 0);
        }
        node = registry->registerClass("stTextureAnim", texture, 0x10000, 0);
    }
    return node;
}

/* The same chain for the sibling that loads a texture from a file. */
// FUNCTION: WIZ8 0x0047D6F0
srRegistry::ClassNode* stTextureFile::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10001);

    if (!node) {
        srRegistry* texture_registry = srCore.getRegistry();
        srRegistry::ClassNode* texture = texture_registry->getClassNode(0x2110);

        if (!texture) {
            srRegistry* iface_registry = srCore.getRegistry();
            srRegistry::ClassNode* iface = iface_registry->getClassNode(0x2100);

            if (!iface) {
                iface = iface_registry->registerClass(
                    "srTextureIFace", srClass::sGetClassNode(), 0x2100, 1);
            }
            texture = texture_registry->registerClass(
                srTexture::sGetClassName(), iface, 0x2110, 0);
        }
        node = registry->registerClass("stTextureFile", texture, 0x10001, 0);
    }
    return node;
}

/* The deepest chain in the program: stLight registers under srLight, which
   registers under srIlluminator, which registers under srNode, which registers
   under srClass. The two SurRender classes that export a static name getter -
   srNode and srIlluminator - supply theirs through it, while srLight and this
   class are literals; that is why the body is 211 bytes rather than 155. The
   srIlluminator level is the same one MonsterLight's reviewed row spells out
   as srClassSupport<srIlluminator,srNode,0,0x1200>. */
// FUNCTION: WIZ8 0x0049DC80
srRegistry::ClassNode* stLight::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10006);

    if (!node) {
        srRegistry* light_registry = srCore.getRegistry();
        srRegistry::ClassNode* light = light_registry->getClassNode(0x1220);

        if (!light) {
            srRegistry* illuminator_registry = srCore.getRegistry();
            srRegistry::ClassNode* illuminator =
                illuminator_registry->getClassNode(0x1200);

            if (!illuminator) {
                srRegistry* node_registry = srCore.getRegistry();
                srRegistry::ClassNode* base = node_registry->getClassNode(0x1000);

                if (!base) {
                    base = node_registry->registerClass(
                        srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
                }
                illuminator = illuminator_registry->registerClass(
                    srIlluminator::sGetClassName(), base, 0x1200, 0);
            }
            light = light_registry->registerClass("srLight", illuminator, 0x1220, 0);
        }
        node = registry->registerClass("stLight", light, 0x10006, 0);
    }
    return node;
}

/* Two vtable installs, seven bytes each. Written as members so the receiver
   arrives in ECX the way the originals take it; the free-function form the
   older MonsterInstallVtable5ED290 uses costs four bytes more because the
   object has to come off the stack first. */

extern "C" {
extern void* g_vtable_005ec138;
extern void* g_vtable_005ebfd0;
}

// FUNCTION: WIZ8 0x00445EE0
void W8Object005EC138::InstallVtable()
{
    *(void**)this = &g_vtable_005ec138;
}

// FUNCTION: WIZ8 0x0042A360
void W8Object005EBFD0::InstallVtable()
{
    *(void**)this = &g_vtable_005ebfd0;
}
