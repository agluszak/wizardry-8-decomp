#pragma once

// Constructs a SurRender type's client implementation; the original spelling is unknown.
#define SR_NEW(Type) new Type::ClientType

#include <iosfwd>
#include <windows.h>

#include "srCore.h"
#include "srCriticalSection.h"
#include "srHeap.h"

class srRuntimeClass;
class srNode;
class srColorSurfaceIFace;

class SR_DLL_EXPORT srRegistry {
public:
    class ClassNode {
        friend class srRegistry;

        struct ChildLink {
            ClassNode* node;
            ChildLink* next;
            ChildLink* previous;
        };

        struct ChildList {
            w8_ulong count;
            ChildLink* first;
            ChildLink* last;

            ChildList() : first(new ChildLink), last(first)
            {
                first->next = 0;
                first->previous = 0;
                count = 0;
            }

            /* The existing child-list owner retains non-owning ClassNode*
               values; destruction removes only the allocated links. */
            void pushFront(ClassNode* node)
            {
                ChildLink* link = new ChildLink;
                link->next = first;
                link->node = node;
                link->previous = first->previous;
                if (link->previous == 0) {
                    first = link;
                } else {
                    link->previous->next = link;
                }
                if (link->next != 0) {
                    link->next->previous = link;
                }
                ++count;
            }

            void removeFirst()
            {
                ChildLink* link = first;
                first = link->next;
                if (link->previous != 0) {
                    link->previous->next = link->next;
                }
                if (link->next != 0) {
                    link->next->previous = link->previous;
                }
                delete link;
                --count;
            }

            // FUNCTION: SURRENDER 0x10010780
            ~ChildList()
            {
                while (first != last) {
                    removeFirst();
                }
                delete first;
            }
        };

    public:
        struct NameIndex;
        struct IDIndex;

    private:
        int isSame(ClassNode* other) const;
        int isDerivedOrSame(ClassNode* derived) const;
        w8_long getNumberOfInstances(int exact) const;
        w8_ulong getClassID() const;
        ClassNode* getParent() const;
        NameIndex* getNameIndex() const;
        IDIndex* getIDIndex() const;
        void enableInstanceLookup();
        void registerInstance(srRuntimeClass* instance);
        void refreshInstance(srRuntimeClass* instance);
        void unregisterInstance(srRuntimeClass* instance);
        srRuntimeClass* findByName(ClassNode* requested_class, const char* name, int exact,
                                   const srRuntimeClass* relative_to);
        srRuntimeClass* findRelative(ClassNode* requested_class, int exact,
                                     const srRuntimeClass* relative_to);
        srRuntimeClass* findByID(ClassNode* requested_class, w8_ulong id, int exact);
        void dump(std::ostream& stream, int indent);

        ClassNode(ClassNode* parent, const char* class_name, w8_ulong class_id);
        ~ClassNode();
        void initialize(ClassNode* parent, const char* class_name, w8_ulong class_id);
        void* operator new(size_t size)
        {
            return srHeap.allocate(size);
        }
        void operator delete(void* node)
        {
            srHeap.free(node);
        }

        ChildList children;
        ClassNode* parent;
        w8_ulong class_id;
        const char* class_name;
        NameIndex* named_instances;
        NameIndex* inherited_named_instances;
        IDIndex* instances_by_id;
        IDIndex* inherited_instances_by_id;
        w8_long instance_count;
    };

    SR_DLL_IMPORT srRegistry();
    SR_DLL_IMPORT ~srRegistry();

    SR_DLL_IMPORT w8_ulong allocateID();
    SR_DLL_IMPORT int checkValidity();
    SR_DLL_IMPORT void dumpClassHierarchy(std::ostream& stream);
    SR_DLL_IMPORT void dumpInstanceNames(ClassNode* node, std::ostream& stream, int indent);
    SR_DLL_IMPORT ClassNode* getClassNode(w8_ulong class_id);
    SR_DLL_IMPORT w8_ulong getClassID(ClassNode* node);
    SR_DLL_IMPORT const char* getClassName(ClassNode* node);
    SR_DLL_IMPORT ClassNode* getChildClass(ClassNode* parent, ClassNode* child);
    SR_DLL_IMPORT w8_long getNumberOfInstances(ClassNode* node, int exact);
    SR_DLL_IMPORT ClassNode* getRootClass();
    SR_DLL_IMPORT ClassNode* getRootNode();
    SR_DLL_IMPORT int isDerivedOrSame(ClassNode* base, ClassNode* derived);
    /* Nonzero gives the node its own instance lookup tables. */
    SR_DLL_IMPORT ClassNode* registerClass(const char* class_name, ClassNode* parent,
                                           w8_ulong class_id, int register_instances);
    SR_DLL_IMPORT void registerInstance(ClassNode* node, srRuntimeClass* instance);
    SR_DLL_IMPORT void unregisterInstance(ClassNode* node, srRuntimeClass* instance);
    SR_DLL_IMPORT srRuntimeClass* find(ClassNode* node, const char* name,
                                       const srRuntimeClass* relative_to);
    SR_DLL_IMPORT srRuntimeClass* find(ClassNode* node, const srRuntimeClass* relative_to);
    SR_DLL_IMPORT srRuntimeClass* find(ClassNode* node, w8_ulong id);
    SR_DLL_IMPORT srRuntimeClass* findExact(ClassNode* node, const char* name,
                                            const srRuntimeClass* relative_to);
    SR_DLL_IMPORT srRuntimeClass* findExact(ClassNode* node, const srRuntimeClass* relative_to);
    SR_DLL_IMPORT srRuntimeClass* findExact(ClassNode* node, w8_ulong id);
    SR_DLL_IMPORT void refreshInstance(ClassNode* node, srRuntimeClass* instance);

private:
    struct ClassIndex;

    SR_DLL_IMPORT ClassNode* addToTree(ClassNode* parent, const char* class_name,
                                       w8_ulong class_id);

    ClassNode* root;
    ClassIndex* class_index;
    int valid;
    srCriticalSection* critical_section;
};

W8_ABI_ASSERT(sizeof(srRegistry::ClassNode) == 0x2c, "srRegistry_ClassNode_must_be_0x2c");
W8_ABI_ASSERT(sizeof(srRegistry) == 0x10, "srRegistry_must_be_0x10");

/* Empty common root; its original name is unknown. */
class srRuntimeClassEmptyBase {
public:
    /* Every class in this hierarchy is allocated from and freed through the SurRender heap. */
    void* operator new(size_t size)
    {
        return srHeap.allocate(size);
    }
    void* operator new(size_t, void* at)
    {
        return at;
    }
    void operator delete(void* instance)
    {
        srHeap.free(instance);
    }
};

// VTABLE: SURRENDER 0x100754E4 srRuntimeClass
// class srRuntimeClass
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srRuntimeClass : public srRuntimeClassEmptyBase {
public:
    enum e_verify { VERIFY_DEFAULT = 0 };

    virtual SR_DLL_IMPORT const char* getClassName() const;
    virtual SR_DLL_IMPORT w8_ulong getClassID() const;
    virtual SR_DLL_IMPORT srRegistry::ClassNode* getClassNode() const;
    virtual SR_DLL_IMPORT void dump(std::ostream& stream);
    virtual SR_DLL_IMPORT void verify(e_verify mode);

    static SR_DLL_IMPORT srRegistry::ClassNode* sGetClassNode();
    static SR_DLL_IMPORT w8_long getTotalInstances(int exact);
    static SR_DLL_IMPORT void dumpNames(std::ostream& stream, int indent);

    SR_DLL_IMPORT void setName(const char* name);
    SR_DLL_IMPORT const char* getName() const;
    SR_DLL_IMPORT w8_ulong getID() const;
    SR_DLL_IMPORT void getUniqueName(std::ostream& stream) const;
    SR_DLL_IMPORT int isNamed() const;
    SR_DLL_IMPORT int matchClassID(w8_ulong class_id) const;

protected:
    SR_DLL_IMPORT srRuntimeClass();
    virtual SR_DLL_IMPORT ~srRuntimeClass();

private:
    static SR_DLL_IMPORT w8_ulong sGetClassID();

    char* name;
    w8_ulong id;
};

W8_ABI_ASSERT(sizeof(srRuntimeClass) == 0x0c, "srRuntimeClass_must_be_0x0c");

class __declspec(novtable)
#if defined(SURRENDER_BUILD)
__declspec(dllexport)
#endif
srClass : public srRuntimeClass {
public:
    typedef srClass RegistryClass;

    typedef void(__cdecl* UpdateCallBack)(srClass* instance, double time, double elapsed);

    static SR_DLL_IMPORT const char* sGetClassName();
    static SR_DLL_IMPORT srRegistry::ClassNode* sGetClassNode();
    static SR_DLL_IMPORT srClass* find(w8_ulong id);
    static SR_DLL_IMPORT srClass* find(const char* name, w8_ulong class_id,
                                       const srRuntimeClass* relative_to);
    static SR_DLL_IMPORT srClass* find(const char* name, const srClass* relative_to);
    static SR_DLL_IMPORT srClass* find(const srClass* relative_to);
    static SR_DLL_IMPORT void performUpdates(double time);

    /* Assignment copies only the instance name. */
    SR_DLL_IMPORT srClass& operator=(const srClass& other);

    virtual SR_DLL_IMPORT srRegistry::ClassNode* getClassNode() const override;
    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT void verify(srRuntimeClass::e_verify mode) override;

protected:
    virtual SR_DLL_IMPORT ~srClass() override;

public:
    virtual srClass* vInstance() = 0;

    /* Slot 7; clone is the nonvirtual forwarder onto it, as instance is onto vInstance. */
    virtual srClass* vClone() = 0;

    // FUNCTION: SURRENDER 0x1000E860 SYMBOL
    // RECOMP: ?clone@srClass@@QAEPAV1@XZ
    srClass* clone()
    {
        return vClone();
    }
    SR_DLL_IMPORT srClass* instance();
    SR_DLL_IMPORT int release() const;
    SR_DLL_IMPORT void addReference() const;
    SR_DLL_IMPORT w8_long getReferenceCount() const;
    SR_DLL_IMPORT void autoRelease();
    SR_DLL_IMPORT void touch();
    SR_DLL_IMPORT w8_ulong getTimestamp() const;
    SR_DLL_IMPORT UpdateCallBack getUpdateCallBack();
    SR_DLL_IMPORT double getUpdateInterval();
    SR_DLL_IMPORT void setUpdate(UpdateCallBack callback, double interval);
    SR_DLL_IMPORT void setUpdatesTime(double time);

protected:
    SR_DLL_IMPORT srClass();
    SR_DLL_IMPORT w8_ulong allocateTimeStamps(w8_ulong count) const;

private:
    struct Update {
        double last_update_time;
        double interval;
        UpdateCallBack callback;
        srClass* instance;
        Update* previous;
        Update* next;
    };

    W8_ABI_ASSERT(sizeof(Update) == 0x20, "srClass_Update_must_be_0x20");

    static SR_DLL_IMPORT Update* _firstUpdate;
    static SR_DLL_IMPORT double _lastUpdateTime;
    static SR_DLL_IMPORT w8_ulong _timestampCtr;

    mutable w8_long reference_count;
    w8_ulong timestamp;
    Update* update;
};

W8_ABI_ASSERT(sizeof(srClass) == 0x18, "srClass_must_be_0x18");

/* The concrete client class a canonical SurRender type hands out through ClientType. It supplies
   the registry identity and clone surface without the registration lifecycle; the imported base
   constructor already registers the object. */
template <class Base, w8_ulong ClassID> class srClientSupport : public Base {
private:
    /* A client type for the canonical class itself reuses that class's registry node; a descendant
       such as srFog registers its own ClassID. */
    static char selfType(Base*);
    static w8_long selfType(...);
    enum {
        BaseOwnsClass =
            sizeof(selfType(static_cast<typename Base::RegistryClass*>(0))) == sizeof(char)
    };

public:
    typedef Base RegistryClass;
    typedef srClientSupport ClientType;

    static srRegistry::ClassNode* sGetClassNode()
    {
        if (BaseOwnsClass) {
            return Base::sGetClassNode();
        }
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(ClassID);
        if (node == 0) {
            node =
                registry->registerClass(Base::sGetClassName(), Base::sGetClassNode(), ClassID, 0);
        }
        return node;
    }

    virtual const char* getClassName() const override
    {
        return Base::sGetClassName();
    }

    virtual w8_ulong getClassID() const override
    {
        return ClassID;
    }

    virtual srRegistry::ClassNode* getClassNode() const override
    {
        return sGetClassNode();
    }

public:
    srClientSupport() {}

    explicit srClientSupport(srNode* parent) : Base(parent) {}

    explicit srClientSupport(srColorSurfaceIFace* arg_surface) : Base(arg_surface) {}

    template <class A0, class A1>
    srClientSupport(A0 first_argument, A1 second_argument) : Base(first_argument, second_argument)
    {
    }

    template <class A0, class A1, class A2>
    srClientSupport(A0 first_argument, A1 second_argument, A2 third_argument)
        : Base(first_argument, second_argument, third_argument)
    {
    }

    template <class A0, class A1, class A2, class A3, class A4>
    srClientSupport(A0 first_argument, A1 second_argument, A2 third_argument, A3 fourth_argument,
                    A4 fifth_argument)
        : Base(first_argument, second_argument, third_argument, fourth_argument, fifth_argument)
    {
    }

public:
    /* Same clone slot as the provider layer. */
    virtual srClass* vClone() override
    {
        Base* copy = static_cast<Base*>(this->vInstance());
        *copy = *static_cast<const Base*>(this);
        return copy;
    }
};

/* Supplies registry identity, instance registration and the clone slot for a class derived from an
   existing registry class. */
template <class Derived, class Base, bool RegisterInstances, w8_ulong ClassID>
class srClassSupport : public Base {
public:
    typedef Derived RegistryClass;
    typedef srClientSupport<Derived, ClassID> ClientType;
    enum { CLASS_ID = ClassID };

    static srRegistry::ClassNode* sGetClassNode()
    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(ClassID);

        if (node == 0) {
            node = registry->registerClass(Derived::sGetClassName(), Base::sGetClassNode(), ClassID,
                                           RegisterInstances);
        }
        return node;
    }

    virtual const char* getClassName() const override
    {
        return Derived::sGetClassName();
    }

    virtual w8_ulong getClassID() const override
    {
        return ClassID;
    }

    virtual srRegistry::ClassNode* getClassNode() const override
    {
        return sGetClassNode();
    }

public:
    srClassSupport()
    {
        srRegistry* registry = srCore.getRegistry();
        registry->registerInstance(sGetClassNode(), this);
    }

    /* Default-constructs Base, registers, then assigns Derived before Derived's members are
       copy-constructed; this touches unconstructed derived state. */
    srClassSupport(const Derived& other) : Base()
    {
        srCore.getRegistry()->registerInstance(sGetClassNode(), this);
        *static_cast<Derived*>(this) = other;
    }

public:
    /* Forwarding constructors. They are templates so that exporting an instantiation whose Base
       lacks a given constructor does not instantiate a forwarding body for it. */
    template <class A0> explicit srClassSupport(A0 first_argument) : Base(first_argument)
    {
        srRegistry* registry = srCore.getRegistry();
        registry->registerInstance(sGetClassNode(), this);
    }

    template <class A0, class A1>
    srClassSupport(A0 first_argument, A1 second_argument) : Base(first_argument, second_argument)
    {
        srRegistry* registry = srCore.getRegistry();
        registry->registerInstance(sGetClassNode(), this);
    }

    template <class A0, class A1, class A2>
    srClassSupport(A0 first_argument, A1 second_argument, A2 third_argument)
        : Base(first_argument, second_argument, third_argument)
    {
        srRegistry* registry = srCore.getRegistry();
        registry->registerInstance(sGetClassNode(), this);
    }

    template <class A0, class A1, class A2, class A3, class A4>
    srClassSupport(A0 first_argument, A1 second_argument, A2 third_argument, A3 fourth_argument,
                   A4 fifth_argument)
        : Base(first_argument, second_argument, third_argument, fourth_argument, fifth_argument)
    {
        srRegistry* registry = srCore.getRegistry();
        registry->registerInstance(sGetClassNode(), this);
    }

protected:
    virtual ~srClassSupport() override
    {
        srRegistry* registry = srCore.getRegistry();
        registry->unregisterInstance(sGetClassNode(), this);
    }

public:
    /* Slot 7 of every registry class; returns srClass* at every level. */
    virtual srClass* vClone() override
    {
        Derived* copy = static_cast<Derived*>(this->vInstance());
        *copy = *static_cast<const Derived*>(this);
        return copy;
    }
};
