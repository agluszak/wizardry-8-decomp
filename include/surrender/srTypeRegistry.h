#pragma once

// Reconstructed SDK construction interface; the original spelling is unknown.
// ClientType retains the evidenced self-support specialization and allocation.
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

        /* The child list is a single member object at offset 0: the
           constructor's unwind funclet destroys {count, first, last} on
           this+0 when initialize() throws, and ~ClassNode's state-0 region
           covers the whole body while the list teardown runs at the end. */
        struct ChildList {
            unsigned long count;
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
                link->node_00 = node;
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
        long getNumberOfInstances(int exact) const;
        unsigned long getClassID() const;
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
        srRuntimeClass* findByID(ClassNode* requested_class, unsigned long id, int exact);
        void dump(std::ostream& stream, int indent);

        ClassNode(ClassNode* parent, const char* class_name, unsigned long class_id);
        ~ClassNode();
        /* Retail emits the field initialization and parent linkage as a
           separate out-of-line body the constructor calls (0x1000F5F0). */
        void initialize(ClassNode* parent, const char* class_name, unsigned long class_id);
        void* operator new(unsigned int size)
        {
            return srHeap.allocate(size);
        }
        void operator delete(void* node)
        {
            srHeap.free(node);
        }

        ChildList children;
        ClassNode* parent;
        unsigned long class_id;
        const char* class_name;
        NameIndex* named_instances;
        NameIndex* inherited_named_instances;
        IDIndex* instances_by_id;
        IDIndex* inherited_instances_by_id;
        long instance_count;
    };

    SR_DLL_IMPORT srRegistry();
    SR_DLL_IMPORT ~srRegistry();

    SR_DLL_IMPORT unsigned long allocateID();
    SR_DLL_IMPORT int checkValidity();
    SR_DLL_IMPORT void dumpClassHierarchy(std::ostream& stream);
    SR_DLL_IMPORT void dumpInstanceNames(ClassNode* node, std::ostream& stream, int indent);
    SR_DLL_IMPORT ClassNode* getClassNode(unsigned long class_id);
    SR_DLL_IMPORT unsigned long getClassID(ClassNode* node);
    SR_DLL_IMPORT const char* getClassName(ClassNode* node);
    SR_DLL_IMPORT ClassNode* getChildClass(ClassNode* parent, ClassNode* child);
    SR_DLL_IMPORT long getNumberOfInstances(ClassNode* node, int exact);
    SR_DLL_IMPORT ClassNode* getRootClass();
    SR_DLL_IMPORT ClassNode* getRootNode();
    SR_DLL_IMPORT int isDerivedOrSame(ClassNode* base, ClassNode* derived);
    /* SR.DLL's registerClass at 0x1000EC60 adds the node, then calls
       0x1000F7E0 only when the last argument is non-zero. That routine
       allocates this node's own instance indices, named_instances and
       instances_by_id, when they are still null. So the argument selects
       whether the node carries its own instance lookup tables; it is not C++
       abstractness. stTexture2D and stSurface2D are constructed directly and
       still pass 0, which independently rules that reading out. */
    SR_DLL_IMPORT ClassNode* registerClass(const char* class_name, ClassNode* parent,
                                           unsigned long class_id, int register_instances);
    SR_DLL_IMPORT void registerInstance(ClassNode* node, srRuntimeClass* instance);
    SR_DLL_IMPORT void unregisterInstance(ClassNode* node, srRuntimeClass* instance);
    SR_DLL_IMPORT srRuntimeClass* find(ClassNode* node, const char* name,
                                       const srRuntimeClass* relative_to);
    SR_DLL_IMPORT srRuntimeClass* find(ClassNode* node, const srRuntimeClass* relative_to);
    SR_DLL_IMPORT srRuntimeClass* find(ClassNode* node, unsigned long id);
    SR_DLL_IMPORT srRuntimeClass* findExact(ClassNode* node, const char* name,
                                            const srRuntimeClass* relative_to);
    SR_DLL_IMPORT srRuntimeClass* findExact(ClassNode* node, const srRuntimeClass* relative_to);
    SR_DLL_IMPORT srRuntimeClass* findExact(ClassNode* node, unsigned long id);
    SR_DLL_IMPORT void refreshInstance(ClassNode* node, srRuntimeClass* instance);

private:
    struct ClassIndex;

    SR_DLL_IMPORT ClassNode* addToTree(ClassNode* parent, const char* class_name,
                                       unsigned long class_id);

    ClassNode* root;
    ClassIndex* class_index;
    int valid;
    srCriticalSection* critical_section;
};

static_assert(sizeof(srRegistry::ClassNode) == 0x2c, "srRegistry_ClassNode_must_be_0x2c");
static_assert(sizeof(srRegistry) == 0x10, "srRegistry_must_be_0x10");

/* Copy construction and assignment at 0x10011A10/0x10011A80 contain a
   null-guarded byte copy at +0x04, overlapping the first member. Related
   copies occur in srClass and srGERD; the reconstruction models a zero-storage
   base. The reviewed evidence does not establish its original name.

   The reconstruction places heap-routing operators on this common base.
   Missing operator exports and calls to srHeap::free do not uniquely establish
   their original declaration owner. */
class srRuntimeClassEmptyBase {
public:
    /* Every class in this hierarchy is allocated from and freed through the
       SurRender heap rather than the global operators, and the routing is
       declared at the common root rather than per class: the identical
       scalar deleting destructor sits at slot 5 of first-party classes
       derived from srClass itself, from srModel/srMeshModel, from
       srTexture/srTextureIFace and from srNode. 0x0042A170 is one of them
       and 0x00492C40 is stMaterial's. */
    void* operator new(unsigned int size)
    {
        return srHeap.allocate(size);
    }
    void* operator new(unsigned int, void* at)
    {
        return at;
    }
    void operator delete(void* instance)
    {
        srHeap.free(instance);
    }
};

/* Retail exports include the vtable, protected construction/destruction and
   copy operations. The reconstruction uses class-level export; original
   special-member and annotation spelling is unresolved. */
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
    virtual SR_DLL_IMPORT unsigned long getClassID() const;
    virtual SR_DLL_IMPORT srRegistry::ClassNode* getClassNode() const;
    virtual SR_DLL_IMPORT void dump(std::ostream& stream);
    virtual SR_DLL_IMPORT void verify(e_verify mode);

    static SR_DLL_IMPORT srRegistry::ClassNode* sGetClassNode();
    static SR_DLL_IMPORT long getTotalInstances(int exact);
    static SR_DLL_IMPORT void dumpNames(std::ostream& stream, int indent);

    /* Copy bodies contain memberwise copying followed by a vptr store. */

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

    char* name;
    unsigned long id;
};

static_assert(sizeof(srRuntimeClass) == 0x0c, "srRuntimeClass_must_be_0x0c");

/* The exported constructor and copy constructor never install an srClass
   vtable; they leave the srRuntimeClass construction vtable in place until a
   concrete derived class installs its own. That is MSVC's novtable ABI, not a
   missing handwritten vtable write. */
/* Retail exports include protected construction/destruction, private statics
   and copy operations. The reconstruction uses class-level export; original
   special-member and annotation spelling is unresolved. clone remains
   declaration-only pending recovery; its body tail-dispatches through slot 7. */
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
    static SR_DLL_IMPORT srClass* find(unsigned long id);
    static SR_DLL_IMPORT srClass* find(const char* name, unsigned long class_id,
                                       const srRuntimeClass* relative_to);
    static SR_DLL_IMPORT srClass* find(const char* name, const srClass* relative_to);
    static SR_DLL_IMPORT srClass* find(const srClass* relative_to);
    static SR_DLL_IMPORT void performUpdates(double time);

    /* Assignment is user-defined and copies only the instance name through
       setName; the copy constructor is implicit (memberwise, vptr-last) and
       emitted via the class-level dllexport. novtable leaves the
       srRuntimeClass construction vtable in place. */
    SR_DLL_IMPORT srClass& operator=(const srClass& other);

    virtual SR_DLL_IMPORT srRegistry::ClassNode* getClassNode() const override;
    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT void verify(srRuntimeClass::e_verify mode) override;

protected:
    virtual SR_DLL_IMPORT ~srClass() override;

public:
    virtual srClass* vInstance() = 0;

    /* Slot 7. Its own spelling is not exported; vClone follows the vInstance
       convention of slot 6. clone below is the exported nonvirtual forwarder
       onto it, as instance is onto vInstance. */
    virtual srClass* vClone() = 0;

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

protected:
    SR_DLL_IMPORT srClass();
    SR_DLL_IMPORT unsigned long allocateTimeStamps(unsigned long count) const;

private:
    struct Update {
        double last_update_time;
        double interval;
        UpdateCallBack callback;
        srClass* instance;
        Update* previous;
        Update* next;
    };

    static_assert(sizeof(Update) == 0x20, "srClass_Update_must_be_0x20");

    static SR_DLL_IMPORT Update* _firstUpdate;
    static SR_DLL_IMPORT double _lastUpdateTime;
    static SR_DLL_IMPORT unsigned long _timestampCtr;

    mutable long reference_count;
    unsigned long timestamp;
    Update* update;
};

static_assert(sizeof(srClass) == 0x18, "srClass_must_be_0x18");

/* The concrete client class a canonical SurRender type hands out through
   ClientType. It is not a provider support layer: retail's client scalar
   deleting destructor calls the imported base destructor directly with no
   support-layer vtable store (W8ColorSurface at 0x00423F00, srNode at
   0x0044F3D0, the srEXT JPEG importer at 0x100151D0). The reconstruction
   leaves the client destructor implicit; its original declaration is unresolved.
   The provider ~srClassSupport restores the support vtable and unregisters the instance.
   The client layer supplies the same registry identity and clone surface
   without the registration lifecycle; the imported base constructor already
   registers the object under the canonical class node. */
template <class Base, unsigned long ClassID> class srClientSupport : public Base {
private:
    /* Same derived==base-style detection the provider template uses: a client
       type for the canonical class itself reuses that class's registry node,
       while a client type for a descendant such as srFog must register its
       own ClassID above the inherited ancestor node. */
    static char selfType(Base*);
    static long selfType(...);
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

    virtual unsigned long getClassID() const override
    {
        return ClassID;
    }

    virtual srRegistry::ClassNode* getClassNode() const override
    {
        return sGetClassNode();
    }

public:
    /* The client constructions forward the canonical base constructor before
       installing the instantiation's table; the imported base constructor
       already performs the registry work a provider layer would repeat. */
    srClientSupport() {}

    explicit srClientSupport(srNode* parent) : Base(parent) {}

    explicit srClientSupport(srColorSurfaceIFace* arg_surface) : Base(arg_surface) {}

    template <class A0, class A1> srClientSupport(A0 a0, A1 a1) : Base(a0, a1) {}

    template <class A0, class A1, class A2> srClientSupport(A0 a0, A1 a1, A2 a2) : Base(a0, a1, a2)
    {
    }

    template <class A0, class A1, class A2, class A3, class A4>
    srClientSupport(A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) : Base(a0, a1, a2, a3, a4)
    {
    }

public:
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#pragma clang diagnostic ignored "-Wsuggest-override"
#endif
    /* Same clone slot as the provider layer. */
    virtual srClass* vClone()
    {
        Base* copy = static_cast<Base*>(this->vInstance());
        *copy = *static_cast<const Base*>(this);
        return copy;
    }
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
};

/* SurRender's exported decorated vtable names establish this template's
   parameter order. It contributes no storage: it supplies registry identity,
   instance registration and the class hierarchy's clone slot for a class
   derived from an existing registry class. */
template <class Derived, class Base, bool RegisterInstances, unsigned long ClassID>
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

    virtual unsigned long getClassID() const override
    {
        return ClassID;
    }

    virtual srRegistry::ClassNode* getClassNode() const override
    {
        return sGetClassNode();
    }

public:
    /* Public because Wiz8 directly constructs the zero-argument self-support
       instantiation over srMaterial; this is not only a base-class hook. */
    srClassSupport()
    {
        srRegistry* registry = srCore.getRegistry();
        registry->registerInstance(sGetClassNode(), this);
    }

    /* Retail's exported copies (srNode 0x10051AA0, srMeshModel 0x10041BF0)
       default-construct Base, register this support layer and assign Derived
       before the compiler copy-constructs Derived's members. This accesses
       unconstructed derived state; retain that retail lifecycle ordering. */
    srClassSupport(const Derived& other) : Base()
    {
        srCore.getRegistry()->registerInstance(sGetClassNode(), this);
        *static_cast<Derived*>(this) = other;
    }

public:
    /* Forwarding constructors: scene-graph instantiations pass the canonical
       node parent, texture maps their color surface, and client-emitted
       self-support constructions up to five arguments. They are templates so
       that a class-level dllexport of an instantiation whose Base lacks a
       given constructor does not instantiate a forwarding body for it. */
    template <class A0> explicit srClassSupport(A0 a0) : Base(a0)
    {
        srRegistry* registry = srCore.getRegistry();
        registry->registerInstance(sGetClassNode(), this);
    }

    template <class A0, class A1> srClassSupport(A0 a0, A1 a1) : Base(a0, a1)
    {
        srRegistry* registry = srCore.getRegistry();
        registry->registerInstance(sGetClassNode(), this);
    }

    template <class A0, class A1, class A2> srClassSupport(A0 a0, A1 a1, A2 a2) : Base(a0, a1, a2)
    {
        srRegistry* registry = srCore.getRegistry();
        registry->registerInstance(sGetClassNode(), this);
    }

    template <class A0, class A1, class A2, class A3, class A4>
    srClassSupport(A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) : Base(a0, a1, a2, a3, a4)
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
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#pragma clang diagnostic ignored "-Wsuggest-override"
#endif
    /* Slot 7 of every registry class. The return type is srClass* at every
       level, which is what makes the nested chain legal under VC6: srClass's
       own nonvirtual clone forwards through this slot and returns srClass*.
       srClass declares vClone, so every specialization overrides it. */
    virtual srClass* vClone()
    {
        Derived* copy = static_cast<Derived*>(this->vInstance());
        *copy = *static_cast<const Derived*>(this);
        return copy;
    }
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
};
