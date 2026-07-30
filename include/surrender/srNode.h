#pragma once

#include "srFlags.h"
#include "srMath.h"
#include "srTypeRegistry.h"
#include "srVertexProcessor.h"

template <class T>
class srMatrix4x3T;

class srCriticalSection;

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

    enum e_notify {};

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
    SR_DLL_IMPORT void clearNotify(e_notify notification);
    SR_DLL_IMPORT void setNotify(e_notify notification);
    SR_DLL_IMPORT int testNotify(e_notify notification) const;

public:
    SR_DLL_IMPORT srNode* cloneHierarchy(srNode* parent);
    SR_DLL_IMPORT void dumpHierarchy(std::ostream& stream, long indent) const;
    SR_DLL_IMPORT srNode* findChild(const char* name) const;
    SR_DLL_IMPORT srNode* findChildByNameAndType(
        const char* name, unsigned long class_id) const;
    SR_DLL_IMPORT srNode* findParent(const char* name) const;
    SR_DLL_IMPORT srNode* findParentByType(unsigned long class_id) const;
    SR_DLL_IMPORT srNode* getChild() const;
    SR_DLL_IMPORT long getChildCount() const;
    SR_DLL_IMPORT char* getFullPath(char* path) const;
    SR_DLL_IMPORT long getFullPathLength() const;
    SR_DLL_IMPORT long getHierarchyLevel() const;
    SR_DLL_IMPORT srNode* getNext() const;
    SR_DLL_IMPORT srNode* getParent() const;
    SR_DLL_IMPORT srNode* getPrev() const;
    SR_DLL_IMPORT int isChildOf(const srNode& node) const;
    SR_DLL_IMPORT int isParentOf(const srNode& node) const;
    static SR_DLL_IMPORT int isSceneGraphLocked();
    static SR_DLL_IMPORT void lockSceneGraph();
    static SR_DLL_IMPORT void unlockSceneGraph();

    SR_DLL_IMPORT void applyWorldSpaceMatrix(class srGERD& renderer);
    SR_DLL_IMPORT double getDistance(const srNode& node) const;
    SR_DLL_IMPORT srVector3T<double> getLocation() const;
    SR_DLL_IMPORT void getLocation(srVector3T<float>& location) const;
    SR_DLL_IMPORT void getLocation(srVector3T<double>& location) const;
    SR_DLL_IMPORT double getLocationX() const;
    SR_DLL_IMPORT double getLocationY() const;
    SR_DLL_IMPORT double getLocationZ() const;
    SR_DLL_IMPORT void getRotation(srMatrix3T<float>& rotation) const;
    SR_DLL_IMPORT void getRotation(srMatrix3T<double>& rotation) const;
    SR_DLL_IMPORT srVector3T<double> getScale() const;
    SR_DLL_IMPORT void getWorldSpaceCoordinates(
        srMatrix3T<float>& rotation,
        srVector3T<float>& location,
        srVector3T<float>& scale) const;
    SR_DLL_IMPORT void getWorldSpaceCoordinates(
        srMatrix3T<double>& rotation,
        srVector3T<double>& location,
        srVector3T<double>& scale) const;
    SR_DLL_IMPORT srVector3T<double> getWorldSpaceDOF() const;
    SR_DLL_IMPORT srVector3T<double> getWorldSpaceLocation() const;
    SR_DLL_IMPORT void getWorldSpaceMatrix(srMatrix4T<float>& matrix) const;
    SR_DLL_IMPORT void getWorldSpaceMatrix(srMatrix4T<double>& matrix) const;
    SR_DLL_IMPORT void getWorldSpaceMatrix(
        srMatrix4x3T<float>& matrix) const;
    SR_DLL_IMPORT void getWorldSpaceMatrix(
        srMatrix4x3T<double>& matrix) const;
    SR_DLL_IMPORT void getWorldSpaceRotation(
        srMatrix3T<float>& rotation) const;
    SR_DLL_IMPORT void getWorldSpaceRotation(
        srMatrix3T<double>& rotation) const;
    SR_DLL_IMPORT srVector3T<double> getWorldSpaceScale() const;
    SR_DLL_IMPORT void move(const srVector3T<double>& offset);
    SR_DLL_IMPORT void moveBackward(double distance);
    SR_DLL_IMPORT void moveDown(double distance);
    SR_DLL_IMPORT void moveForward(double distance);
    SR_DLL_IMPORT void moveLeft(double distance);
    SR_DLL_IMPORT void moveRight(double distance);
    SR_DLL_IMPORT void moveUp(double distance);
    SR_DLL_IMPORT void offsetLocation(const srVector3T<double>& offset);
    SR_DLL_IMPORT void offsetLocation(double x, double y, double z);
    SR_DLL_IMPORT void pitchAt(
        const srVector3T<double>& target, double amount);
    SR_DLL_IMPORT void pitchAt(const srNode* target, double amount);
    SR_DLL_IMPORT void rollAt(
        const srVector3T<double>& target, double amount);
    SR_DLL_IMPORT void rollAt(const srNode* target, double amount);
    SR_DLL_IMPORT void rollUp(double amount);
    SR_DLL_IMPORT void rotate(const srMatrix3T<double>& rotation);
    SR_DLL_IMPORT void rotate(
        double angle, const srVector3T<double>& axis);
    SR_DLL_IMPORT void rotateX(double angle);
    SR_DLL_IMPORT void rotateY(double angle);
    SR_DLL_IMPORT void rotateZ(double angle);
    SR_DLL_IMPORT int setParent(srNode* parent, int preserve_world_transform);
    SR_DLL_IMPORT void setLocation(const srVector3T<double>& location);
    SR_DLL_IMPORT void setLocation(double x, double y, double z);
    SR_DLL_IMPORT void setLocationX(double x);
    SR_DLL_IMPORT void setLocationY(double y);
    SR_DLL_IMPORT void setLocationZ(double z);
    SR_DLL_IMPORT void setRotation(const srMatrix3T<float>& rotation);
    SR_DLL_IMPORT void setRotation(const srMatrix3T<double>& rotation);
    SR_DLL_IMPORT void setRotation(
        const srVector3T<double>& first,
        const srVector3T<double>& second,
        double amount);
    SR_DLL_IMPORT void setRotation(
        const srVector3T<double>& direction, double amount);
    SR_DLL_IMPORT void setRotation(
        double amount, const srVector3T<double>& direction);
    SR_DLL_IMPORT void setRotation(double x, double y, double z);
    SR_DLL_IMPORT void setScale(const srVector3T<double>& scale);
    SR_DLL_IMPORT void setScale(double scale);
    SR_DLL_IMPORT void setWorldSpaceLocation(
        const srVector3T<double>& location);
    SR_DLL_IMPORT void setWorldSpaceMatrix(
        const srMatrix4T<double>& matrix);
    SR_DLL_IMPORT void setWorldSpaceRotation(
        const srMatrix3T<double>& rotation);
    SR_DLL_IMPORT void setFlag(e_flag flag);
    SR_DLL_IMPORT void clearFlag(e_flag flag);
    SR_DLL_IMPORT void notifyChildren(const srFlags<e_notify>& notifications);
    SR_DLL_IMPORT void notifyDependent();
    SR_DLL_IMPORT void notifyParents(const srFlags<e_notify>& notifications);
    SR_DLL_IMPORT void signal(unsigned long signal, void* value);
    SR_DLL_IMPORT int testFlag(e_flag flag) const;
    SR_DLL_IMPORT void yawAt(
        const srVector3T<double>& target, double amount);
    SR_DLL_IMPORT void yawAt(const srNode* target, double amount);
    srNode* nextSibling() const { return next_sibling_; }
    srNode* parentNode() const { return parent_; }
    srNode* firstChild() const { return first_child_; }

private:
    SR_DLL_IMPORT void signalInternal(unsigned long signal, void* value);

    static SR_DLL_IMPORT srCriticalSection sceneGraphCSect;
    static SR_DLL_IMPORT long sceneGraphLockCount;

    unsigned char unknown_18_[0x110];
    srNode* next_sibling_;                  /* 0x128 */
    srNode* previous_sibling_;              /* 0x12c */
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
