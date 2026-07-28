#pragma once

#include "srMath.h"
#include "srTypeRegistry.h"

/* Reconstructed from SR.DLL's export table and the reviewed 13-slot srNode
   vtable. First-party scene traversal establishes the two child-list links;
   the remaining bytes stay opaque. */
class srNode : public srClass {
public:
    class TraverseInfo;
    struct ProcessInfo {
        class srGERD* renderer;
    };
    struct BoundInfo;

    enum e_processType {
        PROCESS_TYPE_POSITIONAL_0 = 0
    };

    enum e_flag {
        FLAG_POSITIONAL_0 = 0
    };

    SR_DLL_IMPORT srNode(srNode* parent);
    SR_DLL_IMPORT srNode(const srNode& other);
    SR_DLL_IMPORT srNode& operator=(const srNode& other);

    static SR_DLL_IMPORT const char* sGetClassName();

    virtual SR_DLL_IMPORT const char* getClassName() const;
    virtual SR_DLL_IMPORT unsigned long getClassID() const;
    virtual SR_DLL_IMPORT srRegistry::ClassNode* getClassNode() const;
    virtual SR_DLL_IMPORT void dump(std::ostream& stream);

protected:
    virtual SR_DLL_IMPORT ~srNode();

public:
    virtual SR_DLL_IMPORT srClass* vInstance();
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
    SR_DLL_IMPORT void setFlag(e_flag flag);
    SR_DLL_IMPORT void clearFlag(e_flag flag);
    SR_DLL_IMPORT int testFlag(e_flag flag) const;
    srNode* nextSibling() const { return next_sibling_; }
    srNode* firstChild() const { return first_child_; }

private:
    unsigned char unknown_04_[0x124];
    srNode* next_sibling_;                  /* 0x128 */
    unsigned char unknown_12c_[0x08];
    srNode* first_child_;                   /* 0x134 */
};

typedef char srNode_must_be_0x138[(sizeof(srNode) == 0x138) ? 1 : -1];
