#pragma once

#include "surrender/srLight.h"
#include "surrender/srScene.h"

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
class stParticle : public srNode {
public:
    const char* getClassName() const override;     /* 0x0049B550 */
    unsigned long getClassID() const override;     /* 0x0049B540 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x0049B560 */
};

/* Engine Code\Trigger.cpp. The registry name has no st prefix, which is what
   separates the gameplay trigger from the renderer's own class family. */
class Trigger : public srNode {
public:
    const char* getClassName() const override;     /* 0x00445AE0 */
    unsigned long getClassID() const override;     /* 0x00445AD0 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x00445F30 */
};

/*
 * The same slots for further host-registered classes whose owning translation
 * unit is not established. Their ids are in the 0x10000 range, so each names a
 * class the program registers itself rather than a SurRender base it is
 * presenting as. They remain together as the explicit closed registry-method
 * family in engine_code/registry_classes.cpp until original TU ownership is proved.
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

// VTABLE: WIZ8 0x005ecc64
class stLight : public srLight {
public:
    stLight(srNode* parent);                         /* 0x0049C2C0 */
    stLight& operator=(const stLight& other);         /* 0x0049C690 */

    virtual const char* getClassName() const override; /* 0x0049DC70 */
    virtual srRegistry::ClassNode* getClassNode() const override; /* 0x0049DC80 */
    virtual unsigned long getClassID() const override; /* 0x0049DC60 */

protected:
    virtual ~stLight() override;                    /* 0x0049C430 */

public:
    virtual srClass* vInstance() override;          /* 0x0049E3A0 */
    virtual srNode* vslot7() override;              /* 0x0049DD60 */
    virtual void traverse(srNode::TraverseInfo& info) override; /* 0x0049C7A0 */
    virtual void process(
        const srNode::ProcessInfo& info,
        srNode::e_processType type) override;       /* 0x0049C8D0 */

private:
    unsigned long m_positional_228;                 /* 0x228 */
    unsigned long m_positional_22c;                 /* 0x22c */
    unsigned long m_positional_230;                 /* 0x230 */
    void* m_owned_234;                              /* 0x234 */
    unsigned char m_positional_238;                 /* 0x238 */
    unsigned char m_positional_239;                 /* 0x239 */
    unsigned char m_positional_23a;                 /* 0x23a */
    unsigned char m_padding_23b;
    float m_positional_23c;                         /* 0x23c */
    unsigned long m_positional_240;                 /* 0x240 */
    void* m_owned_244;                              /* 0x244 */
    unsigned long m_positional_248;                 /* 0x248 */
    float m_positional_24c;                         /* 0x24c */
    unsigned long m_positional_250;                 /* 0x250 */
    unsigned long m_positional_254;                 /* 0x254 */
};

static_assert(sizeof(stLight) == 0x258, "stLight_must_be_0x258");

class stSound3D {
public:
    const char* getClassName() const;     /* 0x004AF3E0 */
    unsigned long getClassID() const;     /* 0x004AF3D0 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x004AF3F0 */
};

/* The only one of this group whose registry builder is also recovered. It takes
   the one-level form, hanging straight off srClass with no intermediate base. */
class stScript {
public:
    unsigned long getClassID() const;     /* 0x004CF7C0 */
    const char* getClassName() const;     /* 0x004CF7D0 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x004CF7E0 */
};

/* Only the name slot is an owned body here; nothing in range supplies an id
   half, so none is declared. */
class stBinIStream {
public:
    const char* getClassName() const;     /* 0x0047CBA0 */
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

/* Monster's copy constructor establishes ownership of this complete object. */
// VTABLE: WIZ8 0x005ecd18
class MonsterLight : public srLight {
public:
    MonsterLight(const MonsterLight& other);          /* 0x0049D660 */

    virtual const char* getClassName() const override; /* 0x0049DC30 */
    virtual unsigned long getClassID() const override; /* 0x0049DC20 */
    virtual srRegistry::ClassNode* getClassNode() const override; /* 0x0049E300 */

public:
    virtual ~MonsterLight() override;                 /* 0x0049E0D0 */

public:
    virtual srNode* vslot7() override;                /* 0x0049DC40 */

private:
    unsigned long m_positional_228;                   /* 0x228 */
    unsigned long m_positional_22c;                   /* 0x22c */
    unsigned long m_positional_230;                   /* 0x230 */
    unsigned long m_positional_234;                   /* 0x234 */
    unsigned long m_positional_238;                   /* 0x238 */
    unsigned long m_positional_23c;                   /* 0x23c */
    unsigned long m_positional_240;                   /* 0x240 */
    unsigned long m_positional_244;                   /* 0x244 */
    unsigned char m_positional_248;                   /* 0x248 */
    unsigned char m_positional_249;                   /* 0x249 */
    unsigned char m_padding_24a[6];
};

static_assert(sizeof(MonsterLight) == 0x250, "MonsterLight_must_be_0x250");

// VTABLE: WIZ8 0x005ebe14
class W8Camera005EBE14 : public srCamera {
public:
    explicit W8Camera005EBE14(srNode* parent) : srCamera(parent) {}

    const char* getClassName() const override;     /* 0x0042A020 */
    unsigned long getClassID() const override;     /* 0x0042A010 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x0042A030 */
    srNode* vslot7() override;                     /* 0x0042A0A0 */
};

static_assert(sizeof(W8Camera005EBE14) == 0x188,
              "W8Camera005EBE14_must_be_0x188");

class W8Scene005EBE48 : public srScene {
public:
    explicit W8Scene005EBE48(srNode* parent) : srScene(parent) {}

    const char* getClassName() const override;     /* 0x0042A0D0 */
    unsigned long getClassID() const override;     /* 0x0042A0C0 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x0042A0E0 */
    srNode* vslot7() override;                     /* 0x0042A150 */

    void ClearOverlayState()
    {
        int index;
        for (index = 0; index != 6; ++index) {
            overlay_state_[index] = 0;
        }
    }
};

static_assert(sizeof(W8Scene005EBE48) == 0x190,
              "W8Scene005EBE48_must_be_0x190");

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

class W8Node005EC208 : public srNode {
public:
    explicit W8Node005EC208(srNode* parent) : srNode(parent) {}
    virtual ~W8Node005EC208() override;

    const char* getClassName() const override
    {
        return srNode::sGetClassName();
    }
    unsigned long getClassID() const override;     /* 0x004519D0, base id 0x1000 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x00445EF0 */
    srNode* vslot7() override;                     /* 0x004519F0 */
};

static_assert(sizeof(W8Node005EC208) == 0x138,
              "W8Node005EC208_must_be_0x138");

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

class W8Object005ECDB0 {
public:
    void InstallVtable();                 /* 0x004A2220 */
};
