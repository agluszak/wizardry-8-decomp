#pragma once

#include "surrender/srModelInstance.h"

class stTextureAnim;

/* Engine Code\stModelInstance.cpp. */
// VTABLE: WIZ8 0x005ec814 stModelInstance
// VTABLE: WIZ8 0x005ec804 srModel::Client
class stModelInstance : public srModelInstance {
public:
    const char* getClassName() const override;     /* 0x00481870 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x00481880 */
    unsigned long getClassID() const override;     /* 0x00481860 */
    stTextureAnim* FindMouthTexture00481080();     /* 0x00481080 */

protected:
    virtual ~stModelInstance() override;           /* 0x00481940 */

public:
    unsigned int frame_index_180;
    int damage_stage_184;
    void** damage_stage_tables_188;
    int damage_stage_count_18c;
};

static_assert(sizeof(stModelInstance) == 0x190,
              "stModelInstance_size_must_be_0x190");

/* The constructor at 0x0047EC80 first builds the 0x190-byte
   stModelInstance base, then installs vtables 0x005EC7D0/0x005EC7C0 and
   initializes the fields below through +0x1ac. Its original derived-class
   name is not yet available. */
class stModelInstance005EC7D0 : public stModelInstance {
public:
    int value_190;
    srVector3T<float> scale_194;
    unsigned char flag_1a0;
    unsigned char flag_1a1;
    unsigned char unknown_1a2[2];
    int value_1a4;
    int value_1a8;
    float value_1ac;
};

static_assert(sizeof(stModelInstance005EC7D0) == 0x1b0,
              "stModelInstance005EC7D0_size_must_be_0x1b0");

/* The destructor-phase table at 0x005EC89C is the no-storage registry layer
   between imported srModelInstance and the concrete 2D implementation. Its
   methods all present the public runtime identity stModelInstance2D. */
// VTABLE: WIZ8 0x005ec89c stModelInstance2D registry base
// VTABLE: WIZ8 0x005ec88c srModel::Client
class W8ModelInstance2DRegistry005EC89C : public srModelInstance {
public:
    explicit W8ModelInstance2DRegistry005EC89C(srNode* parent)
        : srModelInstance(parent)
    {
        srCore.getRegistry()->registerInstance(getClassNode(), this);
    }

    const char* getClassName() const override;     /* 0x00481A50 */
    unsigned long getClassID() const override;     /* 0x00481A40 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x00481A60 */

protected:
    virtual ~W8ModelInstance2DRegistry005EC89C() override; /* 0x00481B20 */
};

static_assert(sizeof(W8ModelInstance2DRegistry005EC89C) == 0x180,
              "W8ModelInstance2DRegistry005EC89C_must_be_0x180");

/* Concrete 2D model instance. Slot 5 and the secondary slot-0 adjustor are
   SYNTHETIC compiler-generated deleting destructors; no source body owns
   either address. */
// VTABLE: WIZ8 0x005ec858 stModelInstance2D
// VTABLE: WIZ8 0x005ec848 srModel::Client
class stModelInstance2D : public W8ModelInstance2DRegistry005EC89C {
public:
    explicit stModelInstance2D(srNode* parent);    /* 0x0047F0F0 */
    stModelInstance2D& operator=(const stModelInstance2D& other); /* 0x0047F290 */

    srClass* vInstance() override;                 /* 0x00481E30 */
    srNode* vslot7() override;                     /* 0x00481B00 */
    void process(const ProcessInfo& info, e_processType type) override; /* 0x00480920 */
};

static_assert(sizeof(stModelInstance2D) == 0x180,
              "stModelInstance2D_must_be_0x180");
