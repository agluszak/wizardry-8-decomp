// Pinned-VC6 lifecycle fixture. Its purpose is to retain compiler-generated
// constructor/destructor families for transient Ghidra recovery tests; none of
// these wrappers are source-authored.

#include <new>

volatile int lifecycle_sink;

struct Base {
    Base() : value(1) {}
    virtual ~Base() { lifecycle_sink += value; }
    virtual Base* clone() const { return new Base(*this); }
    int value;
};

struct Secondary {
    Secondary() : secondary(2) {}
    virtual ~Secondary() { lifecycle_sink += secondary; }
    virtual void touch() { ++secondary; }
    int secondary;
};

struct Derived : Base, Secondary {
    Derived() : member(), values(0) {}
    virtual ~Derived() {}
    virtual Base* clone() const { return new Derived(*this); }
    virtual void touch() { ++values; }
    Base member;
    int values;
};

struct VirtualDerived : virtual Base {
    VirtualDerived() : Base(), field(3) {}
    virtual ~VirtualDerived() { lifecycle_sink += field; }
    int field;
};

struct ClassDelete {
    virtual ~ClassDelete() {}
    static void operator delete(void* value) { ::operator delete(value); }
    static void operator delete[](void* value) { ::operator delete(value); }
};

// Round-trip family: these explicit source entities are intentionally simple
// enough that recovered bodies must be directly compilable, while virtual
// destruction still forces VC6 to emit deleting-wrapper and vtable families.
struct EmptyLifecycleA {
    EmptyLifecycleA();
    virtual ~EmptyLifecycleA();
};

struct EmptyLifecycleB {
    EmptyLifecycleB();
    virtual ~EmptyLifecycleB();
};

EmptyLifecycleA::EmptyLifecycleA() { lifecycle_sink += 10; }
EmptyLifecycleA::~EmptyLifecycleA() { lifecycle_sink -= 10; }
EmptyLifecycleB::EmptyLifecycleB() { lifecycle_sink += 20; }
EmptyLifecycleB::~EmptyLifecycleB() { lifecycle_sink -= 20; }

struct DeletesMember {
    DeletesMember() : owned(new Base) {}
    virtual ~DeletesMember() { delete owned; }
    Base* owned;
};

// Negative role fixture: this is an authored ownership helper, not a deleting
// destructor wrapper. The call graph alone (destructor plus deallocator) is
// deliberately indistinguishable from the old unsafe structural heuristic.
void destroy_and_free(Base* value) {
    value->~Base();
    ::operator delete(value);
}

template <typename T>
struct Holder {
    Holder() : value() {}
    ~Holder() {}
    T value;
};

struct GlobalObject {
    GlobalObject() { ++lifecycle_sink; }
    ~GlobalObject() { --lifecycle_sink; }
};

GlobalObject global_object;

static int inline_helper(const Base& value) {
    return value.value + 1;
}

static GlobalObject& local_static_object() {
    static GlobalObject value;
    return value;
}

static void exercise_lifetimes(int count) {
    Derived local;
    Holder<Derived> holder;
    Base* scalar = new Derived;
    Derived* vector = new Derived[count];
    ClassDelete* owned = new ClassDelete;
    ClassDelete* owned_vector = new ClassDelete[count];
    EmptyLifecycleA* empty_a = new EmptyLifecycleA;
    EmptyLifecycleB* empty_b = new EmptyLifecycleB;
    DeletesMember deletes_member;
    lifecycle_sink += inline_helper(local);
    local_static_object();
    delete scalar;
    delete[] vector;
    delete owned;
    delete[] owned_vector;
    delete empty_a;
    delete empty_b;
    if (count == -1) {
        destroy_and_free(new Base);
    }
}

int main(int argc, char**) {
    exercise_lifetimes(argc + 1);
    VirtualDerived virtual_derived;
    return lifecycle_sink + virtual_derived.field;
}
