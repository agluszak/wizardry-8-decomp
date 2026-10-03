#pragma once

#include "srHeap.h"

// VTABLE: SURRENDER 0x10075310 srFilter
// class srFilter
class SR_DLL_IMPORT SR_DLL_EXPORT srFilter {
public:
    /* The reconstruction leaves trivial construction and copying implicit.
       The explicit virtual destructor supplies the modeled destruction interface. */

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
    /* No derived state is modeled; lifecycle bodies are consistent with
       ordinary base-only operations and derived table setup. */

    virtual const char* getName() const override;
    virtual double getWeight(double value) const override;
    virtual double getSupport() const override;
};

// VTABLE: SURRENDER 0x10075378 srBellFilter
// class srBellFilter
class SR_DLL_IMPORT SR_DLL_EXPORT srBellFilter : public srFilter {
public:
    /* No derived state is modeled; lifecycle bodies are consistent with
       ordinary base-only operations and derived table setup. */

    virtual const char* getName() const override;
    virtual double getWeight(double value) const override;
    virtual double getSupport() const override;
};

// VTABLE: SURRENDER 0x10075398 srBSplineFilter
// class srBSplineFilter
class SR_DLL_IMPORT SR_DLL_EXPORT srBSplineFilter : public srFilter {
public:
    /* No derived state is modeled; lifecycle bodies are consistent with
       ordinary base-only operations and derived table setup. */

    virtual const char* getName() const override;
    virtual double getWeight(double value) const override;
    virtual double getSupport() const override;
};

// VTABLE: SURRENDER 0x10075388 srTriangleFilter
// class srTriangleFilter
class SR_DLL_IMPORT SR_DLL_EXPORT srTriangleFilter : public srFilter {
public:
    /* No derived state is modeled; lifecycle bodies are consistent with
       ordinary base-only operations and derived table setup. */

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
