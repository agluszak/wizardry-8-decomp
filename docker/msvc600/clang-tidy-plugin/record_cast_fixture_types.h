#pragma once

#line 1 "/home/build/wizardry/include/wiz8/clang_tidy_record_fixture_types.h"
namespace cast_fixture {
struct A {};
struct B {};
struct C {};
struct Base {};
struct Derived : Base {};
struct Controls {};
struct Renderer {
    struct Pick {};
};

inline B* shared_header_erasure(A* value)
{
    return reinterpret_cast<B*>(static_cast<void*>(value));
}
} // namespace cast_fixture

#line 1 "/opt/msvc6/include/w8vendor_fixture.h"
#pragma clang system_header
namespace cast_vendor {
struct W8Collision {};
} // namespace cast_vendor
