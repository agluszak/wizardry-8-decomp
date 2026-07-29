#pragma once

#include "srMath.h"
#include "srTypeRegistry.h"

/* Reconstructed from SR.DLL's export table and the reviewed 13-slot srNode
   vtable. First-party scene traversal establishes the two child-list links;
   the remaining bytes stay opaque. */
class srNode : public srClass {
public:
    class TraverseInfo {
    public:
        struct Entry {
            srNode* node;
            unsigned long value;
        };

        unsigned char unknown_00_[8];
        Entry* entries;                    /* 0x08 */
        unsigned int capacity;             /* 0x0c */
        unsigned int count;                /* 0x10 */
    };
    struct ProcessInfo {
        class srGERD* renderer;
    };
    struct BoundInfo;

    enum e_processType {
        PROCESS_TYPE_POSITIONAL_0 = 0
    };

    enum e_flag {
        FLAG_POSITIONAL_0 = 0,
        FLAG_POSITIONAL_1 = 1
    };

    SR_DLL_IMPORT srNode(srNode* parent);
    SR_DLL_IMPORT srNode(const srNode& other);
    SR_DLL_IMPORT srNode& operator=(const srNode& other);

    static SR_DLL_IMPORT const char* sGetClassName();

    virtual SR_DLL_IMPORT const char* getClassName() const override;
    virtual SR_DLL_IMPORT unsigned long getClassID() const override;
    virtual SR_DLL_IMPORT srRegistry::ClassNode* getClassNode() const override;
    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;

protected:
    virtual SR_DLL_IMPORT ~srNode() override;

public:
    virtual SR_DLL_IMPORT srClass* vInstance() override;
    virtual SR_DLL_IMPORT srNode* vslot7();
    virtual SR_DLL_IMPORT void traverse(TraverseInfo& info);
    virtual SR_DLL_IMPORT void process(const ProcessInfo& info, e_processType type);
    virtual SR_DLL_IMPORT void getLocalBounds(BoundInfo& bounds);
    virtual SR_DLL_IMPORT void updateBounds();

protected:
    virtual SR_DLL_IMPORT int processSignal(unsigned long signal, void* value);

public:
    SR_DLL_IMPORT int setParent(srNode* parent, int preserve_world_transform);
    SR_DLL_IMPORT void setLocation(double x, double y, double z);
    SR_DLL_IMPORT void setLocation(const srVector3T<double>& location);
    SR_DLL_IMPORT void setRotation(double x, double y, double z);
    /* The reader for the rotation basis, taken through an out-parameter rather
       than returned. 3dapi.cpp's camera-rotation accessor is what establishes
       the float element type. */
    SR_DLL_IMPORT void getRotation(srMatrix3T<float>* rotation);
    SR_DLL_IMPORT void setFlag(e_flag flag);
    SR_DLL_IMPORT void clearFlag(e_flag flag);
    SR_DLL_IMPORT int testFlag(e_flag flag) const;
    SR_DLL_IMPORT srNode* getParent() const;
    /* Wiz8.exe emits its own deleting destructor for this base at 0x0044F3D0
       and releases through the renderer's heap rather than the CRT's. It is a
       member because the destructor it runs is protected, so nothing outside
       the class can spell the teardown at all. */
    srNode* scalar_deleting_destructor(unsigned char flags);
    srNode* nextSibling() const { return next_sibling_; }
    srNode* parentNode() const { return parent_; }
    srNode* firstChild() const { return first_child_; }

private:
    unsigned char unknown_04_[0x124];
    srNode* next_sibling_;                  /* 0x128 */
    unsigned char unknown_12c_[0x04];
    srNode* parent_;                        /* 0x130 */
    srNode* first_child_;                   /* 0x134 */
};

/* Declared for its static registry getter alone, which Wiz8.exe imports by
   decorated name - `original-export` evidence, so the name and the ABI are the
   original's. The reviewed MonsterLight row places it between srNode and
   srLight, spelling the relation out as
   srClassSupport<srIlluminator,srNode,0,0x1200>; nothing else about the class
   is recovered, so nothing else is declared. */
class srIlluminator : public srNode {
public:
    static SR_DLL_IMPORT const char* sGetClassName();
};

typedef char srNode_must_be_0x138[(sizeof(srNode) == 0x138) ? 1 : -1];
typedef char srNode_TraverseInfo_must_be_0x14[
    (sizeof(srNode::TraverseInfo) == 0x14) ? 1 : -1];
