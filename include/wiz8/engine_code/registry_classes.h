#pragma once

#include "surrender/srTypeRegistry.h"

/*
 * Classes whose only recovered members so far are the two SurRender class
 * registry slots.
 *
 * Each class is named by the literal its own getClassName slot returns, which
 * is `original-runtime-string` evidence, and each id sits at 0x10000 and up -
 * the range SurRender reserves for classes the host program registers rather
 * than its own. The owning translation unit is corroborated twice over: the
 * registry name matches the unit name, and the body sits inside that unit's
 * assertion-bounded interval.
 *
 * The slots are declared as plain members rather than `virtual`. Nothing about
 * these classes' layout is recovered yet, and giving them virtuals would have
 * VC6 synthesize a vptr and a vtable that no evidence places, where the two
 * bodies themselves need neither. Their real slot indices stay an open
 * question the vtable evidence answers, not this header.
 */

/* Engine Code\stModelInstance.cpp. */
class stModelInstance {
public:
    const char* getClassName() const;     /* 0x00481870 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x00481880 */
    unsigned long getClassID() const;     /* 0x00481860 */
};

/* Engine Code\stModelInstance.cpp, alongside the 3D form above. The two ids are
   adjacent, which is what pairs them. */
class stModelInstance2D {
public:
    const char* getClassName() const;     /* 0x00481A50 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x00481A60 */
    unsigned long getClassID() const;     /* 0x00481A40 */
};

/* Engine Code\stParticle.cpp. */
class stParticle {
public:
    const char* getClassName() const;     /* 0x0049B550 */
    unsigned long getClassID() const;     /* 0x0049B540 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x0049B560 */
};

/* Engine Code\Trigger.cpp. The registry name has no st prefix, which is what
   separates the gameplay trigger from the renderer's own class family. */
class Trigger {
public:
    const char* getClassName() const;     /* 0x00445AE0 */
    unsigned long getClassID() const;     /* 0x00445AD0 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x00445F30 */
};

/*
 * The same slot pair for five more host-registered classes whose owning
 * translation unit is not established. Their ids are in the 0x10000 range, so
 * each names a class the program registers itself rather than a SurRender base
 * it is presenting as, but nothing yet places the bodies in a named unit -
 * which is why they live in unattributed_helpers.cpp for now.
 */

class stTextureAnim {
public:
    const char* getClassName() const;     /* 0x00485800 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x00485810 */
    unsigned long getClassID() const;     /* 0x004857F0 */
};

class stTextureFile {
public:
    const char* getClassName() const;     /* 0x0047D6E0 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x0047D6F0 */
    unsigned long getClassID() const;     /* 0x0047D6D0 */
};

class stLight {
public:
    const char* getClassName() const;     /* 0x0049DC70 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x0049DC80 */
    unsigned long getClassID() const;     /* 0x0049DC60 */
};

class stLevel {
public:
    const char* getClassName() const;     /* 0x004BA1C0 */
    unsigned long getClassID() const;     /* 0x004BA1B0 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x004BA1D0 */
};

class stSound3D {
public:
    const char* getClassName() const;     /* 0x004AF3E0 */
    unsigned long getClassID() const;     /* 0x004AF3D0 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x004AF3F0 */
};

/*
 * Classes that present themselves to the scene graph under a SurRender base
 * name instead of one of their own.
 *
 * Their ids sit in SurRender's own 0x1000-0x3110 range rather than the host
 * range above, so the literal each returns names the *base* and not the class.
 * That is why these carry vtable-qualified names: the vtable is the only thing
 * that identifies them, and borrowing the base's name would assert an identity
 * the registry pair explicitly does not establish. The reviewed model reaches
 * the same conclusion for MonsterLight, whose row records that it "presents
 * itself to the SurRender scene graph as an srLight variant rather than under a
 * Wizardry name".
 */

/* Reviewed as MonsterLight in evidence/reviewed/wiz8/classes.csv, whose row
   already records both of these slots; Monster owns a pointer to it. */
class MonsterLight {
public:
    const char* getClassName() const;     /* 0x0049DC30, vtable 0x005ECD18 + 0 */
    unsigned long getClassID() const;     /* 0x0049DC20, vtable 0x005ECD18 + 4 */
};

class W8Camera005EBE14 {
public:
    const char* getClassName() const;     /* 0x0042A020 */
    unsigned long getClassID() const;     /* 0x0042A010 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x0042A030 */
};

class W8Scene005EBE48 {
public:
    const char* getClassName() const;     /* 0x0042A0D0 */
    unsigned long getClassID() const;     /* 0x0042A0C0 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x0042A0E0 */
};

class W8MeshModel005EBE98 {
public:
    const char* getClassName() const;     /* 0x00429B40 */
    unsigned long getClassID() const;     /* 0x00429B30 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x00429B50 */
};

class W8TextureMap005EBEEC {
public:
    const char* getClassName() const;     /* 0x00429BF0 */
    unsigned long getClassID() const;     /* 0x00429BE0 */
};

class W8Fog005EC94C {
public:
    const char* getClassName() const;     /* 0x00484710 */
    unsigned long getClassID() const;     /* 0x00484700 */
};

class W8ClipPlane005ED180 {
public:
    const char* getClassName() const;     /* 0x004BDF10 */
    unsigned long getClassID() const;     /* 0x004BDF00 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x004BDF20 */
};

/*
 * Six more classes that present under a SurRender base, but whose name slot is
 * an import thunk straight to SR.DLL rather than an owned body - so only the
 * id half of the pair exists here to recover. Two of the ids are named by the
 * reviewed model: 0x1000 is srNode and 0x1200 is srIlluminator, both spelled
 * out in MonsterLight's srClassSupport<srIlluminator,srNode,0,0x1200> base.
 * The rest stay vtable-qualified because nothing names their base.
 */

class W8Node005EC208 {
public:
    unsigned long getClassID() const;     /* 0x004519D0, base id 0x1000 */
};

class W8Illuminator005ECCD8 {
public:
    unsigned long getClassID() const;     /* 0x0049DB10, base id 0x1200 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x0049DB30 */
};

class W8Registered005EBD10 {
public:
    unsigned long getClassID() const;     /* 0x00429A40 */
};

/* The one body VC6 folded: vtables 0x005EBDE0 and 0x005EBF68 both point their
   id slot at it, so two classes share one emission. */
class W8Registered005EBDE0 {
public:
    unsigned long getClassID() const;     /* 0x00429CC0 */
};

class W8Registered005EBF94 {
public:
    unsigned long getClassID() const;     /* 0x00429E80 */
};

class W8Registered005EC5D8 {
public:
    unsigned long getClassID() const;     /* 0x0047D650 */
};

/*
 * Two classes whose only recovered member is the vtable install itself. Each
 * body is seven bytes - one store through the receiver and a return - which is
 * what makes them member functions rather than free ones: the object arrives
 * in ECX, and a free function taking it on the stack costs four bytes more.
 * Nothing else about either class is established, so nothing else is declared;
 * the vtable address is the only identity they have.
 */

class W8Object005EC138 {
public:
    void InstallVtable();                 /* 0x00445EE0 */
};

class W8Object005EBFD0 {
public:
    void InstallVtable();                 /* 0x0042A360 */
};
