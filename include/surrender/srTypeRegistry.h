#pragma once

#include <iostream>

#include "srCore.h"
#include "srHeap.h"

class srRuntimeClass;

class srRegistry {
public:
    class ClassNode;

    SR_DLL_IMPORT ClassNode* getClassNode(unsigned long class_id);
    SR_DLL_IMPORT ClassNode* registerClass(
        const char* class_name,
        ClassNode* parent,
        unsigned long class_id,
        int concrete);
    SR_DLL_IMPORT void registerInstance(
        ClassNode* node, srRuntimeClass* instance);
    SR_DLL_IMPORT void unregisterInstance(
        ClassNode* node, srRuntimeClass* instance);
    SR_DLL_IMPORT srRuntimeClass* find(
        ClassNode* node,
        const char* name,
        const srRuntimeClass* relative_to);
};

class srRuntimeClass {
public:
    enum e_verify {
        VERIFY_DEFAULT = 0
    };

    virtual const char* getClassName() const = 0;
    virtual unsigned long getClassID() const = 0;
    virtual srRegistry::ClassNode* getClassNode() const = 0;

    SR_DLL_IMPORT void setName(const char* name);
    SR_DLL_IMPORT const char* getName() const;
};

class srClass : public srRuntimeClass {
public:
    static SR_DLL_IMPORT srRegistry::ClassNode* sGetClassNode();
    SR_DLL_IMPORT srClass& operator=(const srClass& other);

    virtual const char* getClassName() const override = 0;
    virtual unsigned long getClassID() const override = 0;
    virtual srRegistry::ClassNode* getClassNode() const override = 0;
    virtual SR_DLL_IMPORT void dump(std::ostream& stream);
    virtual SR_DLL_IMPORT void verify(srRuntimeClass::e_verify mode);

protected:
    virtual SR_DLL_IMPORT ~srClass();

public:
    virtual srClass* vInstance() = 0;

    SR_DLL_IMPORT int release() const;
    SR_DLL_IMPORT void addReference() const;
    SR_DLL_IMPORT void autoRelease();
    SR_DLL_IMPORT void touch();
    void* operator new(unsigned int size) { return srHeap.allocate(size); }

    /* Every class in this hierarchy is freed through the SurRender heap rather
       than the global operator delete, and the routing is declared here rather
       than per class: the identical 34-byte scalar deleting destructor sits at
       slot 5 of first-party classes derived from srClass itself, from
       srModel/srMeshModel, from srTexture/srTextureIFace and from srNode, so
       the only place it can come from is their common root. 0x0042A170 is one
       of them and 0x00492C40 is stMaterial's. */
    void operator delete(void* instance) { srHeap.free(instance); }

protected:
    SR_DLL_IMPORT srClass();

private:
    /* The imported srClass constructor owns this runtime bookkeeping. Direct
       first-party subclasses begin their own storage at +0x18. */
    unsigned char runtime_state_004_[0x14];
};

static_assert(sizeof(srClass) == 0x18, "srClass_must_be_0x18");

/* SurRender's exported decorated vtable names establish this template's
   parameter order. It contributes no storage: it supplies registry identity,
   instance registration and the virtual half of srClass::clone() for a class
   derived from an existing registry class. */
template <class Derived, class Base, int Abstract, unsigned long ClassID>
class srClassSupport : public Base {
public:
    static srRegistry::ClassNode* sGetClassNode()
    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(ClassID);

        if (node == 0) {
            node = registry->registerClass(
                Derived::sGetClassName(), Base::sGetClassNode(), ClassID,
                Abstract == 0);
        }
        return node;
    }

    virtual const char* getClassName() const override
    {
        return Derived::sGetClassName();
    }

    virtual unsigned long getClassID() const override
    {
        return ClassID;
    }

    virtual srRegistry::ClassNode* getClassNode() const override
    {
        return sGetClassNode();
    }

protected:
    srClassSupport()
    {
        srCore.getRegistry()->registerInstance(sGetClassNode(), this);
    }

    explicit srClassSupport(Base* parent)
        : Base(parent)
    {
        srCore.getRegistry()->registerInstance(sGetClassNode(), this);
    }

    virtual ~srClassSupport() override
    {
        srCore.getRegistry()->unregisterInstance(sGetClassNode(), this);
    }

    /* srClass::clone() dispatches through slot 7. The export is non-virtual;
       this is its unexported per-class implementation hook. */
    virtual srClass* vClone()
    {
        Derived* copy = static_cast<Derived*>(this->vInstance());
        *copy = *static_cast<const Derived*>(this);
        return copy;
    }
};
