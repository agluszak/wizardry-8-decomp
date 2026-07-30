#pragma once

#include "srMath.h"
#include "srTypeRegistry.h"
#include "srVertexProcessor.h"

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

        class NodeArray {
        public:
            void setCapacity(unsigned int new_capacity); /* 0x0049E290 */

            srNode** data;                 /* 0x00 */
            unsigned int capacity;         /* 0x04 */
        };

        class EntryArray {
        public:
            void setCapacity(unsigned int new_capacity); /* 0x00481C80 */

            Entry* data;                   /* 0x00 */
            unsigned int capacity;         /* 0x04 */
        };

        NodeArray nodes;                    /* 0x00 */
        EntryArray entries;                 /* 0x08 */
        unsigned int entry_count;           /* 0x10 */
        unsigned int node_count;            /* 0x14 */
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
        FLAG_POSITIONAL_1 = 1,
        FLAG_POSITIONAL_2 = 2
    };

    SR_DLL_IMPORT srNode(srNode* parent);
    SR_DLL_IMPORT srNode(const srNode& other);
    SR_DLL_IMPORT srNode& operator=(const srNode& other);

    static SR_DLL_IMPORT const char* sGetClassName();
    static srRegistry::ClassNode* sGetClassNode()
    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x1000);
        if (node == 0) {
            node = registry->registerClass(
                sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
        }
        return node;
    }

    virtual SR_DLL_IMPORT const char* getClassName() const override;
    virtual SR_DLL_IMPORT unsigned long getClassID() const override;
    virtual SR_DLL_IMPORT srRegistry::ClassNode* getClassNode() const override;
    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;

protected:
    virtual SR_DLL_IMPORT ~srNode() override;

public:
    virtual SR_DLL_IMPORT srClass* vInstance() override;
    virtual SR_DLL_IMPORT srNode* clone();
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
    SR_DLL_IMPORT srVector3T<double> getLocation() const;
    SR_DLL_IMPORT void getLocation(srVector3T<float>& location) const;
    SR_DLL_IMPORT void setRotation(double x, double y, double z);
    SR_DLL_IMPORT void setRotation(const srMatrix3T<float>& rotation);
    SR_DLL_IMPORT srVector3T<double> getScale() const;
    SR_DLL_IMPORT void setScale(const srVector3T<double>& scale);
    /* The reader for the rotation basis, taken through an out-parameter rather
       than returned. 3dapi.cpp's camera-rotation accessor is what establishes
       the float element type. */
    SR_DLL_IMPORT void getRotation(srMatrix3T<float>* rotation);
    SR_DLL_IMPORT void getWorldSpaceMatrix(srMatrix4T<float>& matrix) const;
    SR_DLL_IMPORT void setFlag(e_flag flag);
    SR_DLL_IMPORT void clearFlag(e_flag flag);
    SR_DLL_IMPORT int testFlag(e_flag flag) const;
    SR_DLL_IMPORT srNode* getParent() const;
    srNode* nextSibling() const { return next_sibling_; }
    srNode* parentNode() const { return parent_; }
    srNode* firstChild() const { return first_child_; }

private:
    unsigned char unknown_18_[0x110];
    srNode* next_sibling_;                  /* 0x128 */
    unsigned char unknown_12c_[0x04];
    srNode* parent_;                        /* 0x130 */
    srNode* first_child_;                   /* 0x134 */
};

/* SR.DLL's exported primary and secondary vtable names establish the exact
   srNode/srVertexProcessor multiple-inheritance prefix. */
class srIlluminator : public srNode, public srVertexProcessor {
public:
    SR_DLL_IMPORT srIlluminator(srNode* parent);
    SR_DLL_IMPORT srIlluminator& operator=(const srIlluminator& other);
    static SR_DLL_IMPORT const char* sGetClassName();
    virtual SR_DLL_IMPORT void traverse(TraverseInfo& info) override;
    virtual SR_DLL_IMPORT void process(
        const ProcessInfo& info, e_processType type) override;
    virtual int isActive(srVertexPipe& pipe) override = 0;
    virtual void process(srVertexPipe& pipe) override = 0;
    SR_DLL_IMPORT unsigned long getGroupMask() const;
    SR_DLL_IMPORT void setGroupMask(unsigned long mask);

protected:
    virtual SR_DLL_IMPORT ~srIlluminator() override;
};

static_assert((sizeof(srNode) == 0x138), "srNode_must_be_0x138");
static_assert((sizeof(srIlluminator) == 0x168),
              "srIlluminator_must_be_0x168");
static_assert((sizeof(srNode::TraverseInfo) == 0x18), "srNode_TraverseInfo_must_be_0x18");
