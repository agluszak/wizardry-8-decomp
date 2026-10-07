#pragma once

#include "srArray.h"
#include "srCriticalSection.h"
#include "srFlags.h"
#include "srMath.h"
#include "srTypeRegistry.h"

#include <new>

// VTABLE: SURRENDER 0x10077204
// class srClassSupport<srNode, srClass, 1, 4096>

// VTABLE: SURRENDER 0x100771D0 srNode
class SR_DLL_EXPORT srNode : public srClassSupport<srNode, srClass, true, 0x1000> {
public:
    class TraverseInfo {
    public:
        struct Entry {
            srNode* node;
            unsigned long value;
        };

        srArray<srNode*> nodes;   /* 0x00 */
        srArray<Entry> entries;   /* 0x08 */
        unsigned int entry_count; /* 0x10 */
        unsigned int node_count;  /* 0x14 */
        /* srScene::process stores the active renderer here; srBounder::traverse
           reads it for the child volume tests. */
        class srGERD* renderer; /* 0x18 */
    };
    struct ProcessInfo {
        class srGERD* renderer;
    };
    /* Bounds record: box, sphere and a trailing state of 0 (empty), 1 (box+sphere) or 2
       (FLAG_GLOBAL). */
    struct BoundInfo {
        srVector3T<float> minimum;
        srVector3T<float> maximum;
        srVector3T<float> center;
        float radius;
        int state;
    };

    enum e_processType {
        PROCESS_RENDER = 0,
        PROCESS_PUSH = 1,
        PROCESS_POP = 2,
        PROCESS_PUSH_GLOBAL = 3,
        PROCESS_POP_GLOBAL = 4
    };

    /* traverse omits a DISABLE node and does not walk the children of a TERMINATE node. Lights,
       clip planes and bounders set GLOBAL. Setting IGNORE_TRANSFORM dirties the cached world
       transform. */
    enum e_flag {
        FLAG_DISABLE = 0,
        FLAG_TERMINATE = 1,
        FLAG_GLOBAL = 2,
        FLAG_IGNORE_TRANSFORM = 3
    };

    enum e_notify { NOTIFY_BOUNDS_DIRTY = 0 };

    SR_DLL_IMPORT srNode(srNode* parent = 0);

    SR_DLL_IMPORT srNode& operator=(const srNode& other);

    static SR_DLL_IMPORT const char* sGetClassName();

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;

protected:
    virtual SR_DLL_IMPORT ~srNode() override;

public:
    virtual SR_DLL_IMPORT srClass* vInstance() override;
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
    SR_DLL_IMPORT srNode* findChildByNameAndType(const char* name, unsigned long class_id) const;
    SR_DLL_IMPORT srNode* findParent(const char* name) const;
    SR_DLL_IMPORT srNode* findParentByType(unsigned long class_id) const;
    SR_DLL_IMPORT srNode* getChild() const;
    SR_DLL_IMPORT long getChildCount() const;
    SR_DLL_IMPORT char* getFullPath(char* path) const;
    SR_DLL_IMPORT long getFullPathLength() const;
    SR_DLL_IMPORT long getHierarchyLevel() const;
    SR_DLL_IMPORT srNode* getNext() const;
    // FUNCTION: SURRENDER 0x10051A40 SYMBOL
    // RECOMP: ?getParent@srNode@@QBEPAV1@XZ
    srNode* getParent() const
    {
        return parent_;
    }
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
    // FUNCTION: SURRENDER 0x10053DD0 SYMBOL
    // RECOMP: ?getLocation@srNode@@QBEXAAV?$srVector3T@N@@@Z
    void getLocation(srVector3T<double>& location) const
    {
        location = this->location;
    }
    SR_DLL_IMPORT double getLocationX() const;
    SR_DLL_IMPORT double getLocationY() const;
    SR_DLL_IMPORT double getLocationZ() const;
    SR_DLL_IMPORT void getRotation(srMatrix3T<float>& rotation) const;
    SR_DLL_IMPORT void getRotation(srMatrix3T<double>& rotation) const;
    SR_DLL_IMPORT srVector3T<double> getScale() const;
    SR_DLL_IMPORT void getWorldSpaceCoordinates(srMatrix3T<float>& rotation,
                                                srVector3T<float>& location,
                                                srVector3T<float>& scale) const;
    SR_DLL_IMPORT void getWorldSpaceCoordinates(srMatrix3T<double>& rotation,
                                                srVector3T<double>& location,
                                                srVector3T<double>& scale) const;
    SR_DLL_IMPORT srVector3T<double> getWorldSpaceDOF() const;
    SR_DLL_IMPORT srVector3T<double> getWorldSpaceLocation() const;
    SR_DLL_IMPORT void getWorldSpaceMatrix(srMatrix4T<float>& matrix) const;
    SR_DLL_IMPORT void getWorldSpaceMatrix(srMatrix4T<double>& matrix) const;
    SR_DLL_IMPORT void getWorldSpaceMatrix(srMatrix4x3T<float>& matrix) const;
    SR_DLL_IMPORT void getWorldSpaceMatrix(srMatrix4x3T<double>& matrix) const;
    SR_DLL_IMPORT void getWorldSpaceRotation(srMatrix3T<float>& rotation) const;
    SR_DLL_IMPORT void getWorldSpaceRotation(srMatrix3T<double>& rotation) const;
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
    SR_DLL_IMPORT void pitchAt(const srVector3T<double>& target, double amount);
    SR_DLL_IMPORT void pitchAt(const srNode* target, double amount);
    SR_DLL_IMPORT void rollAt(const srVector3T<double>& target, double amount);
    SR_DLL_IMPORT void rollAt(const srNode* target, double amount);
    SR_DLL_IMPORT void rollUp(double amount);
    SR_DLL_IMPORT void rotate(const srMatrix3T<double>& rotation);
    SR_DLL_IMPORT void rotate(double angle, const srVector3T<double>& axis);
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
    SR_DLL_IMPORT void setRotation(const srVector3T<double>& first,
                                   const srVector3T<double>& second, double amount);
    SR_DLL_IMPORT void setRotation(const srVector3T<double>& direction, double amount);
    SR_DLL_IMPORT void setRotation(double amount, const srVector3T<double>& direction);
    SR_DLL_IMPORT void setRotation(double x, double y, double z);
    SR_DLL_IMPORT void setScale(const srVector3T<double>& scale);
    SR_DLL_IMPORT void setScale(double scale);
    SR_DLL_IMPORT void setWorldSpaceLocation(const srVector3T<double>& location);
    SR_DLL_IMPORT void setWorldSpaceMatrix(const srMatrix4T<double>& matrix);
    SR_DLL_IMPORT void setWorldSpaceRotation(const srMatrix3T<double>& rotation);
    SR_DLL_IMPORT void setFlag(e_flag flag);
    SR_DLL_IMPORT void clearFlag(e_flag flag);
    SR_DLL_IMPORT void notifyChildren(const srFlags<e_notify>& notifications);
    SR_DLL_IMPORT void notifyDependent();
    SR_DLL_IMPORT void notifyParents(const srFlags<e_notify>& notifications);
    SR_DLL_IMPORT void signal(unsigned long signal, void* value);
    SR_DLL_IMPORT int testFlag(e_flag flag) const;
    SR_DLL_IMPORT void yawAt(const srVector3T<double>& target, double amount);
    SR_DLL_IMPORT void yawAt(const srNode* target, double amount);

private:
    SR_DLL_IMPORT void checkTransformation() const;
    SR_DLL_IMPORT srNode* cloneHierarchyInternal(srNode* parent);
    SR_DLL_IMPORT srNode* findChildByNameAndTypeInternal(const char* name, unsigned long class_id);
    SR_DLL_IMPORT srNode* findChildInternal(const char* name);
    SR_DLL_IMPORT srNode* findParentByTypeInternal(unsigned long class_id);
    SR_DLL_IMPORT srNode* findParentInternal(const char* name);
    SR_DLL_IMPORT void getFullPathInternal(char* path) const;
    SR_DLL_IMPORT long getFullPathLengthInternal() const;
    SR_DLL_IMPORT void setWSDirty();
    SR_DLL_IMPORT void signalInternal(unsigned long signal, void* value);
    SR_DLL_IMPORT void unlink();
    SR_DLL_IMPORT void updateTransformation() const;

    static SR_DLL_IMPORT srCriticalSection sceneGraphCSect;
    static SR_DLL_IMPORT long sceneGraphLockCount;

    srMatrix3T<double> rotation; /* 0x018 */
    srVector3T<double> location; /* 0x060 */
    srVector3T<double> scale;    /* 0x078 */
    /* Cached world transforms and the notification word mutate inside the
       const updateTransformation/getter family. */
    mutable srMatrix4x3T<double> world_transform0; /* 0x090: cached affine world transform */
    mutable srMatrix4x3T<float> world_transform1;  /* 0x0f0 */
    mutable srFlags<e_notify> notifications;       /* 0x120 */
    srFlags<e_flag> flags;                         /* 0x124 */

public:
    srNode* next_sibling_;     /* 0x128 */
    srNode* previous_sibling_; /* 0x12c */
    srNode* parent_;           /* 0x130 */
    srNode* first_child_;      /* 0x134 */
};

static_assert((sizeof(srNode) == 0x138), "srNode_must_be_0x138");
static_assert((sizeof(srNode::TraverseInfo) == 0x1c), "srNode_TraverseInfo_must_be_0x1c");
static_assert((sizeof(srNode::BoundInfo) == 0x2c), "srNode_BoundInfo_must_be_0x2c");
