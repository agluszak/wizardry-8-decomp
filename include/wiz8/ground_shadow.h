#pragma once

#include "surrender/srGERD.h"
#include "surrender/srHeap.h"
#include "surrender/srNode.h"

extern "C" char g_stGroundShadowClassName[];

class stGroundShadow;

/* The construction-phase vtable is a no-storage registry layer. Its ordinary
   destructor is emitted separately at 0x004D6A70; slot 5 at 0x004D6B50 is the
   compiler-generated deleting wrapper. */
// VTABLE: WIZ8 0x005ed3f8
class W8GroundShadowRegistry005ED3F8 : public srNode {
public:
    explicit W8GroundShadowRegistry005ED3F8(srNode* parent)
        : srNode(parent)
    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x10010);
        if (node == 0) {
            srRegistry* parent_registry = srCore.getRegistry();
            node = parent_registry->getClassNode(0x1000);
            if (node == 0) {
                node = parent_registry->registerClass(
                    srNode::sGetClassName(),
                    srClass::sGetClassNode(),
                    0x1000,
                    1);
            }
            node = registry->registerClass(
                g_stGroundShadowClassName, node, 0x10010, 0);
        }
        registry->registerInstance(node, this);
    }

    const char* getClassName() const override;       /* 0x004D69B0 */
    unsigned long getClassID() const override;       /* 0x004D69A0 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x004D69C0 */
    srNode* vslot7() override;                       /* 0x004D6A30 */

protected:
    // FUNCTION: WIZ8 0x004D6A70
    virtual ~W8GroundShadowRegistry005ED3F8() override
    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x10010);
        if (node == 0) {
            srRegistry* parent_registry = srCore.getRegistry();
            node = parent_registry->getClassNode(0x1000);
            if (node == 0) {
                node = parent_registry->registerClass(
                    srNode::sGetClassName(),
                    srClass::sGetClassNode(),
                    0x1000,
                    1);
            }
            node = registry->registerClass(
                g_stGroundShadowClassName, node, 0x10010, 0);
        }
        registry->unregisterInstance(node, this);
    }
};

static_assert(sizeof(W8GroundShadowRegistry005ED3F8) == 0x138,
              "W8GroundShadowRegistry005ED3F8_must_be_0x138");

/* The retained registry name, class id, concrete vtable, allocation size and
   clone/factory slots identify this first-party srNode subclass. */
// VTABLE: WIZ8 0x005ed3c4
class stGroundShadow : public W8GroundShadowRegistry005ED3F8 {
public:
    stGroundShadow(srNode* parent);                 /* 0x004D61B0 */
    stGroundShadow(const stGroundShadow& other);    /* 0x004D6430 */

protected:
    virtual ~stGroundShadow() override;                      /* 0x004D6370 */

public:
    virtual srClass* vInstance() override;                   /* 0x004D6BF0 */
    virtual void traverse(TraverseInfo& info) override;      /* 0x004D6540 */
    virtual void process(
        const ProcessInfo& info, e_processType type) override; /* 0x004D6640 */

    void* operator new(unsigned int size)
    {
        return srHeap.allocate(size);
    }

    int field_138;
    int value_13c;
    int value_140;
    unsigned char unknown_144[4];

private:
    void renderGroundShadow(srGERD* renderer);      /* 0x004D66A0 */
};

static_assert((sizeof(stGroundShadow) == 0x148), "stGroundShadow_must_be_0x148");
