#pragma once

#include "surrender/srLight.h"
#include "surrender/srModelInstance.h"
#include "surrender/srScene.h"

class W8MonsterShakeCallback;
struct W8Position;

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
 * Classes below remain positional until their constructor or vtable supplies
 * the complete inheritance evidence. Closed classes move into their owned
 * headers instead of accumulating here.
 */

/* Engine Code\stModelInstance.cpp. The destructor writes both tables before
   unregistering the instance and delegating to srModelInstance. The second
   table is the ordinary srModel::Client base at srModelInstance +0x138. */
// VTABLE: WIZ8 0x005ec814 stModelInstance
// VTABLE: WIZ8 0x005ec804 srModel::Client
class stModelInstance : public srModelInstance {
public:
    const char* getClassName() const override;     /* 0x00481870 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x00481880 */
    unsigned long getClassID() const override;     /* 0x00481860 */
    class stTextureAnim* FindMouthTexture00481080(); /* 0x00481080 */

protected:
    virtual ~stModelInstance() override;           /* 0x00481940 */

public:
    unsigned int frame_index_180;
    int damage_stage_184;
    void** damage_stage_tables_188;
    int damage_stage_count_18c;
};

static_assert(sizeof(stModelInstance) == 0x190, "stModelInstance_size_must_be_0x190");

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
    void SetActive0049ACD0(int active);

    unsigned char unknown_138[0x4c];
    int state_184;
    int value_188;
    void* node_18c;
    unsigned char active_190;
    unsigned char unknown_191[0xd3];
    int start_frame_264;
    int end_frame_268;
    W8MonsterShakeCallback* callback_26c;
};

static_assert(sizeof(stParticle) == 0x270, "stParticle_size_must_be_0x270");

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

class stTextureFile {
public:
    const char* getClassName() const;     /* 0x0047D6E0 */
    srRegistry::ClassNode* getClassNode() const;        /* 0x0047D6F0 */
    unsigned long getClassID() const;     /* 0x0047D6D0 */
};

// VTABLE: WIZ8 0x005ecc64
class stLight : public srLight {
public:
    stLight();                                         /* 0x004CA8B0 */
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

    float positionalX() const { return m_positional_228; }
    float positionalY() const { return m_positional_22c; }
    float positionalZ() const { return m_positional_230; }
    void* worldLink() const { return m_owned_234; }
    void ConfigureMonsterCopy()
    {
        m_positional_18 = 2;
        m_positional_flags_5c |= 0x10;
        m_positional_flags_5c |= 4;
    }

private:
    float m_positional_228;                         /* 0x228 */
    float m_positional_22c;                         /* 0x22c */
    float m_positional_230;                         /* 0x230 */
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
    void SetVisible0049D970(char visible);
    void Update0049D990(const W8Position* position);

    virtual const char* getClassName() const override; /* 0x0049DC30 */
    virtual unsigned long getClassID() const override; /* 0x0049DC20 */
    virtual srRegistry::ClassNode* getClassNode() const override; /* 0x0049E300 */

public:
    virtual ~MonsterLight() override;                 /* 0x0049E0D0 */

public:
    virtual srNode* vslot7() override;                /* 0x0049DC40 */

private:
    float m_vertical_offset_228;                      /* 0x228 */
    srVector3T<float> m_color_first_22c;              /* 0x22c */
    srVector3T<float> m_color_second_238;             /* 0x238 */
    float m_start_time_244;                           /* 0x244 */
    unsigned char m_cycle_color_248;                  /* 0x248 */
    unsigned char m_fade_out_249;                     /* 0x249 */
    unsigned char m_padding_24a[6];
};

static_assert(sizeof(MonsterLight) == 0x250, "MonsterLight_must_be_0x250");

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
