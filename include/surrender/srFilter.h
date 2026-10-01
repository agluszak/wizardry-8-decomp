#pragma once

#include "srHeap.h"

// VTABLE: SURRENDER 0x10075310 srFilter
// class srFilter
class SR_DLL_IMPORT SR_DLL_EXPORT srFilter {
public:
    /* The class-level export emits the trivial default constructor, copy
       constructor and assignment operator. They are compiler-generated:
       srFilter has no data members, and the retail bodies contain no authored
       work. The destructor remains explicit because it establishes the
       hierarchy's virtual destructor. */
    // SYNTHETIC: SURRENDER 0x10003300
    // ??0srFilter@@QAE@XZ
    // SYNTHETIC: SURRENDER 0x10003310
    // ??0srFilter@@QAE@ABV0@@Z
    // SYNTHETIC: SURRENDER 0x10003330
    // ??4srFilter@@QAEAAV0@ABV0@@Z

    // FUNCTION: SURRENDER 0x100032B0
    // ??1srFilter@@UAE@XZ
    virtual ~srFilter() {}

    virtual const char* getName() const = 0;
    virtual double getWeight(double value) const = 0;
    virtual double getSupport() const = 0;
};

// VTABLE: SURRENDER 0x10075350 srBoxFilter
// class srBoxFilter
class SR_DLL_IMPORT SR_DLL_EXPORT srBoxFilter : public srFilter {
public:
    /* No derived state: default/copy construction, assignment and destruction
       are the compiler-generated srFilter operations plus the derived vptr
       store. Class-level dllexport emits the standalone retail symbols. */
    // SYNTHETIC: SURRENDER 0x10003480
    // srBoxFilter::srBoxFilter()
    // SYNTHETIC: SURRENDER 0x100034A0
    // srBoxFilter::srBoxFilter(const srBoxFilter&)
    // SYNTHETIC: SURRENDER 0x10003520
    // srBoxFilter::operator=
    // SYNTHETIC: SURRENDER 0x10003530
    // srBoxFilter::~srBoxFilter

    virtual const char* getName() const override;
    virtual double getWeight(double value) const override;
    virtual double getSupport() const override;
};

// VTABLE: SURRENDER 0x10075378 srBellFilter
// class srBellFilter
class SR_DLL_IMPORT SR_DLL_EXPORT srBellFilter : public srFilter {
public:
    /* No derived state: default/copy construction, assignment and destruction
       are the compiler-generated srFilter operations plus the derived vptr
       store. Class-level dllexport emits the standalone retail symbols. */
    // SYNTHETIC: SURRENDER 0x100035E0
    // srBellFilter::srBellFilter()
    // SYNTHETIC: SURRENDER 0x100035F0
    // srBellFilter::srBellFilter(const srBellFilter&)
    // SYNTHETIC: SURRENDER 0x10003600
    // srBellFilter::operator=
    // SYNTHETIC: SURRENDER 0x10003610
    // srBellFilter::~srBellFilter

    virtual const char* getName() const override;
    virtual double getWeight(double value) const override;
    virtual double getSupport() const override;
};

// VTABLE: SURRENDER 0x10075398 srBSplineFilter
// class srBSplineFilter
class SR_DLL_IMPORT SR_DLL_EXPORT srBSplineFilter : public srFilter {
public:
    /* No derived state: default/copy construction, assignment and destruction
       are the compiler-generated srFilter operations plus the derived vptr
       store. Class-level dllexport emits the standalone retail symbols. */
    // SYNTHETIC: SURRENDER 0x10003780
    // srBSplineFilter::srBSplineFilter()
    // SYNTHETIC: SURRENDER 0x10003790
    // srBSplineFilter::srBSplineFilter(const srBSplineFilter&)
    // SYNTHETIC: SURRENDER 0x100037A0
    // srBSplineFilter::operator=
    // SYNTHETIC: SURRENDER 0x100037B0
    // srBSplineFilter::~srBSplineFilter

    virtual const char* getName() const override;
    virtual double getWeight(double value) const override;
    virtual double getSupport() const override;
};

// VTABLE: SURRENDER 0x10075388 srTriangleFilter
// class srTriangleFilter
class SR_DLL_IMPORT SR_DLL_EXPORT srTriangleFilter : public srFilter {
public:
    /* No derived state: default/copy construction, assignment and destruction
       are the compiler-generated srFilter operations plus the derived vptr
       store. Class-level dllexport emits the standalone retail symbols. */
    // SYNTHETIC: SURRENDER 0x100036B0
    // srTriangleFilter::srTriangleFilter()
    // SYNTHETIC: SURRENDER 0x100036C0
    // srTriangleFilter::srTriangleFilter(const srTriangleFilter&)
    // SYNTHETIC: SURRENDER 0x100036D0
    // srTriangleFilter::operator=
    // SYNTHETIC: SURRENDER 0x100036E0
    // srTriangleFilter::~srTriangleFilter

    virtual const char* getName() const override;
    virtual double getWeight(double value) const override;
    virtual double getSupport() const override;
};

static_assert(sizeof(srFilter) == 0x04, "srFilter_must_be_0x04");
static_assert(sizeof(srBoxFilter) == 0x04, "srBoxFilter_must_be_0x04");
static_assert(sizeof(srBellFilter) == 0x04, "srBellFilter_must_be_0x04");
static_assert(sizeof(srBSplineFilter) == 0x04, "srBSplineFilter_must_be_0x04");
static_assert(sizeof(srTriangleFilter) == 0x04, "srTriangleFilter_must_be_0x04");

extern SR_DLL_IMPORT class srBoxFilter srBoxFilter;
extern SR_DLL_IMPORT class srBellFilter srBellFilter;
extern SR_DLL_IMPORT class srBSplineFilter srBSplineFilter;
extern SR_DLL_IMPORT class srTriangleFilter srTriangleFilter;
