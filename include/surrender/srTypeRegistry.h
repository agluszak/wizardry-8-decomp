#pragma once

#include <iostream>
#include <windows.h>

#include "srCore.h"
#include "srHeap.h"

class srRuntimeClass;

class srRegistry {
public:
    class ClassNode {
        friend class srRegistry;

        struct ChildLink;
        struct NameIndex;
        struct IDIndex;

        unsigned long child_count_00;
        ChildLink* first_child_04;
        ChildLink* child_end_08;
        ClassNode* parent_0c;
        unsigned long class_id_10;
        const char* class_name_14;
        NameIndex* named_instances_18;
        NameIndex* inherited_named_instances_1c;
        IDIndex* instances_by_id_20;
        IDIndex* inherited_instances_by_id_24;
        long instance_count_28;
    };

    SR_DLL_IMPORT srRegistry();
    SR_DLL_IMPORT ~srRegistry();
    SR_DLL_IMPORT srRegistry& operator=(const srRegistry& other);

    SR_DLL_IMPORT unsigned long allocateID();
    SR_DLL_IMPORT int checkValidity();
    SR_DLL_IMPORT void dumpClassHierarchy(std::ostream& stream);
    SR_DLL_IMPORT void dumpInstanceNames(
        ClassNode* node, std::ostream& stream, int indent);
    SR_DLL_IMPORT ClassNode* getClassNode(unsigned long class_id);
    SR_DLL_IMPORT unsigned long getClassID(ClassNode* node);
    SR_DLL_IMPORT const char* getClassName(ClassNode* node);
    SR_DLL_IMPORT ClassNode* getChildClass(ClassNode* parent, ClassNode* child);
    SR_DLL_IMPORT long getNumberOfInstances(ClassNode* node, int recursive);
    SR_DLL_IMPORT ClassNode* getRootClass();
    SR_DLL_IMPORT ClassNode* getRootNode();
    SR_DLL_IMPORT int isDerivedOrSame(ClassNode* derived, ClassNode* base);
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
    SR_DLL_IMPORT srRuntimeClass* find(
        ClassNode* node, const srRuntimeClass* relative_to);
    SR_DLL_IMPORT srRuntimeClass* find(ClassNode* node, unsigned long id);
    SR_DLL_IMPORT srRuntimeClass* findExact(
        ClassNode* node,
        const char* name,
        const srRuntimeClass* relative_to);
    SR_DLL_IMPORT srRuntimeClass* findExact(
        ClassNode* node, const srRuntimeClass* relative_to);
    SR_DLL_IMPORT srRuntimeClass* findExact(ClassNode* node, unsigned long id);
    SR_DLL_IMPORT void refreshInstance(
        ClassNode* node, srRuntimeClass* instance);

private:
    SR_DLL_IMPORT ClassNode* addToTree(
        ClassNode* parent, const char* class_name, unsigned long class_id);

    ClassNode* root_00;
    void* class_index_04;
    unsigned long next_class_id_08;
    CRITICAL_SECTION* critical_section_0c;
};

static_assert(sizeof(srRegistry::ClassNode) == 0x2c,
              "srRegistry_ClassNode_must_be_0x2c");
static_assert(sizeof(srRegistry) == 0x10, "srRegistry_must_be_0x10");

class srRuntimeClass {
public:
    enum e_verify {
        VERIFY_DEFAULT = 0
    };

    virtual SR_DLL_IMPORT const char* getClassName() const;
    virtual SR_DLL_IMPORT unsigned long getClassID() const;
    virtual SR_DLL_IMPORT srRegistry::ClassNode* getClassNode() const;
    virtual SR_DLL_IMPORT void dump(std::ostream& stream);
    virtual SR_DLL_IMPORT void verify(e_verify mode);

    static SR_DLL_IMPORT srRegistry::ClassNode* sGetClassNode();
    static SR_DLL_IMPORT long getTotalInstances(int recursive);
    static SR_DLL_IMPORT void dumpNames(std::ostream& stream, int indent);

    SR_DLL_IMPORT srRuntimeClass(const srRuntimeClass& other);
    SR_DLL_IMPORT srRuntimeClass& operator=(const srRuntimeClass& other);
    SR_DLL_IMPORT void setName(const char* name);
    SR_DLL_IMPORT const char* getName() const;
    SR_DLL_IMPORT unsigned long getID() const;
    SR_DLL_IMPORT void getUniqueName(std::ostream& stream) const;
    SR_DLL_IMPORT int isNamed() const;
    SR_DLL_IMPORT int matchClassID(unsigned long class_id) const;

protected:
    SR_DLL_IMPORT srRuntimeClass();
    virtual SR_DLL_IMPORT ~srRuntimeClass();

private:
    static SR_DLL_IMPORT unsigned long sGetClassID();

    char* name_04;
    unsigned long id_08;
};

static_assert(sizeof(srRuntimeClass) == 0x0c,
              "srRuntimeClass_must_be_0x0c");

/* The exported constructor and copy constructor never install an srClass
   vtable; they leave the srRuntimeClass construction vtable in place until a
   concrete derived class installs its own. That is MSVC's novtable ABI, not a
   missing handwritten vtable write. */
class __declspec(novtable) srClass : public srRuntimeClass {
public:
    typedef void (__cdecl *UpdateCallBack)(
        srClass* instance, double time, double elapsed);

    static SR_DLL_IMPORT const char* sGetClassName();
    static SR_DLL_IMPORT srRegistry::ClassNode* sGetClassNode();
    static SR_DLL_IMPORT srClass* find(unsigned long id);
    static SR_DLL_IMPORT srClass* find(
        const char* name,
        unsigned long class_id,
        const srRuntimeClass* relative_to);
    static SR_DLL_IMPORT srClass* find(
        const char* name, const srClass* relative_to);
    static SR_DLL_IMPORT srClass* find(const srClass* relative_to);
    static SR_DLL_IMPORT void performUpdates(double time);

    SR_DLL_IMPORT srClass(const srClass& other);
    SR_DLL_IMPORT srClass& operator=(const srClass& other);

    virtual SR_DLL_IMPORT srRegistry::ClassNode* getClassNode() const override;
    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT void verify(srRuntimeClass::e_verify mode) override;

protected:
    virtual SR_DLL_IMPORT ~srClass() override;

public:
    virtual srClass* vInstance() = 0;

    SR_DLL_IMPORT srClass* clone();
    SR_DLL_IMPORT srClass* instance();
    SR_DLL_IMPORT int release() const;
    SR_DLL_IMPORT void addReference() const;
    SR_DLL_IMPORT long getReferenceCount() const;
    SR_DLL_IMPORT void autoRelease();
    SR_DLL_IMPORT void touch();
    SR_DLL_IMPORT unsigned long getTimestamp() const;
    SR_DLL_IMPORT UpdateCallBack getUpdateCallBack();
    SR_DLL_IMPORT double getUpdateInterval();
    SR_DLL_IMPORT void setUpdate(UpdateCallBack callback, double interval);
    SR_DLL_IMPORT void setUpdatesTime(double time);
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
    SR_DLL_IMPORT unsigned long allocateTimeStamps(unsigned long count) const;

private:
    struct Update {
        double last_update_time_00;
        double interval_08;
        UpdateCallBack callback_10;
        srClass* instance_14;
        Update* previous_18;
        Update* next_1c;
    };

    static_assert(sizeof(Update) == 0x20, "srClass_Update_must_be_0x20");

    static SR_DLL_IMPORT Update* _firstUpdate;
    static SR_DLL_IMPORT double _lastUpdateTime;
    static SR_DLL_IMPORT unsigned long _timestampCtr;

    long reference_count_0c;
    unsigned long timestamp_10;
    Update* update_14;
};

static_assert(sizeof(srClass) == 0x18, "srClass_must_be_0x18");

/* SurRender's exported decorated vtable names establish this template's
   parameter order. It contributes no storage: it supplies registry identity,
   instance registration and the class hierarchy's clone slot for a class
   derived from an existing registry class. */
template <class Derived, class Base, bool IsAbstract, unsigned long ClassID>
class srClassSupport : public Base {
public:
    static srRegistry::ClassNode* sGetClassNode()
    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(ClassID);

        if (node == 0) {
            node = registry->registerClass(
                Derived::sGetClassName(), Base::sGetClassNode(), ClassID,
                IsAbstract);
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
        srRegistry* registry = srCore.getRegistry();
        registry->registerInstance(sGetClassNode(), this);
    }

    explicit srClassSupport(Base* parent)
        : Base(parent)
    {
        srRegistry* registry = srCore.getRegistry();
        registry->registerInstance(sGetClassNode(), this);
    }

    virtual ~srClassSupport() override
    {
        srRegistry* registry = srCore.getRegistry();
        registry->unregisterInstance(sGetClassNode(), this);
    }

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#pragma clang diagnostic ignored "-Wsuggest-override"
#endif
    /* This introduces clone when Base is srClass and overrides the family
       clone when Base is srNode or srTexture. It cannot truthfully carry an
       unconditional override specifier. */
    virtual Base* clone()
    {
        Derived* copy = static_cast<Derived*>(this->vInstance());
        *copy = *static_cast<const Derived*>(this);
        return copy;
    }
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
};
