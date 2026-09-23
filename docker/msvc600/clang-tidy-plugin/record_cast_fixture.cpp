#include "record_cast_fixture_types.h"

extern "C" void* malloc(unsigned long size);
void opaque_sink(void* value);

#line 1 "/home/build/wizardry/include/wiz8/clang_tidy_record_fixture_macros.h"
#define CAST_FIXTURE_RECORD(value) reinterpret_cast<cast_fixture::B*>(static_cast<void*>(value))

#line 1 "/repo/src/wiz8/clang_tidy_record_cast_fixture.cpp"
using cast_fixture::A;
using cast_fixture::B;
using cast_fixture::Base;
using cast_fixture::Controls;
using cast_fixture::Derived;

B* direct_reinterpret(A* value)
{
    return reinterpret_cast<B*>(value);
}

B* erased_pointer(A* value)
{
    return reinterpret_cast<B*>(static_cast<void*>(value));
}

const B* qualified_erasure(A* value)
{
    return static_cast<const B*>(static_cast<const void*>(static_cast<const A*>(value)));
}

B* parenthesized_erasure(A* value)
{
    return reinterpret_cast<B*>((static_cast<void*>(value)));
}

B* single_definition_local(A* value)
{
    void* erased = static_cast<void*>(value);
    return static_cast<B*>(erased);
}

B* ambiguous_local_definition(A* first, B* second, bool choose_second)
{
    void* erased = static_cast<void*>(first);
    if (choose_second) {
        erased = static_cast<void*>(second);
    }
    return static_cast<B*>(erased);
}

B* escaped_local_definition(A* value)
{
    void* erased = static_cast<void*>(value);
    opaque_sink(erased);
    return static_cast<B*>(erased);
}

B* byte_pointer_detour(A* value)
{
    return reinterpret_cast<B*>(reinterpret_cast<unsigned char*>(value));
}

B** pointer_depth_erasure(A** value)
{
    return static_cast<B**>(static_cast<void*>(value));
}

B* c_style_erasure(A* value)
{
    return (B*)((void*)value);
}

B& unrelated_reference(A& value)
{
    return reinterpret_cast<B&>(value);
}

Controls* controls_name(A* value)
{
    return static_cast<Controls*>(static_cast<void*>(value));
}

cast_fixture::Renderer::Pick* nested_record(A* value)
{
    return reinterpret_cast<cast_fixture::Renderer::Pick*>(static_cast<void*>(value));
}

template <class T> B* authored_template_body(A* value)
{
    return reinterpret_cast<B*>(static_cast<void*>(value));
}

template <class Source, class Destination> Destination* dependent_template_body(Source* value)
{
    return reinterpret_cast<Destination*>(static_cast<void*>(value));
}

void template_instantiations(A* value)
{
    (void)authored_template_body<int>(value);
    (void)authored_template_body<double>(value);
    (void)dependent_template_body<A, B>(value);
    (void)dependent_template_body<A, B>(value);
    (void)dependent_template_body<A, Controls>(value);
}

B* project_macro_cast(A* value)
{
    return CAST_FIXTURE_RECORD(value);
}

Base* ordinary_implicit_upcast(Derived* value)
{
    return value;
}

Base* ordinary_explicit_upcast(Derived* value)
{
    return static_cast<Base*>(value);
}

B* allocator_return()
{
    return static_cast<B*>(malloc(sizeof(B)));
}

B* opaque_callback_context(void* context)
{
    return static_cast<B*>(context);
}

unsigned char object_representation(A* value)
{
    const auto* bytes = reinterpret_cast<const unsigned char*>(value);
    return bytes[0];
}

A* same_type_round_trip(A* value)
{
    return static_cast<A*>(static_cast<void*>(value));
}

B* vendor_prefix_collision(cast_vendor::W8Collision* value)
{
    return reinterpret_cast<B*>(static_cast<void*>(value));
}

B* shared_header_use(A* value)
{
    return cast_fixture::shared_header_erasure(value);
}
