#pragma once

#include "srHeap.h"

// VTABLE: SURRENDER 0x10075310 srFilter
// class srFilter
class SR_DLL_IMPORT srFilter {
public:
    /* The trivial base members are defined inline. Retail emitted each as a
       real export while still folding it into the derived constructors/
       destructors, which is why those bodies are a single vtable store with
       no base call. The member dllexport marks keep the header bodies for
       that folding while still emitting the exported standalone copies. */
    // FUNCTION: SURRENDER 0x10003300
    // ??0srFilter@@QAE@XZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srFilter()
    {
    }

    // FUNCTION: SURRENDER 0x10003310
    // ??0srFilter@@QAE@ABV0@@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srFilter(const srFilter& other)
    {
    }

    // FUNCTION: SURRENDER 0x100032B0
    // ??1srFilter@@UAE@XZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    virtual ~srFilter()
    {
    }

    // FUNCTION: SURRENDER 0x10003330
    // ??4srFilter@@QAEAAV0@ABV0@@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srFilter& operator=(const srFilter& other)
    {
        return *this;
    }

    virtual const char* getName() const = 0;
    virtual double getWeight(double value) const = 0;
    virtual double getSupport() const = 0;
};

// VTABLE: SURRENDER 0x10075350 srBoxFilter
// class srBoxFilter
class SR_DLL_IMPORT srBoxFilter : public srFilter {
public:
    srBoxFilter();
    srBoxFilter(const srBoxFilter& other);
    virtual ~srBoxFilter() override;
    srBoxFilter& operator=(const srBoxFilter& other);

    virtual const char* getName() const override;
    virtual double getWeight(double value) const override;
    virtual double getSupport() const override;
};

// VTABLE: SURRENDER 0x10075378 srBellFilter
// class srBellFilter
class SR_DLL_IMPORT srBellFilter : public srFilter {
public:
    srBellFilter();
    srBellFilter(const srBellFilter& other);
    virtual ~srBellFilter() override;
    srBellFilter& operator=(const srBellFilter& other);

    virtual const char* getName() const override;
    virtual double getWeight(double value) const override;
    virtual double getSupport() const override;
};

// VTABLE: SURRENDER 0x10075398 srBSplineFilter
// class srBSplineFilter
class SR_DLL_IMPORT srBSplineFilter : public srFilter {
public:
    srBSplineFilter();
    srBSplineFilter(const srBSplineFilter& other);
    virtual ~srBSplineFilter() override;
    srBSplineFilter& operator=(const srBSplineFilter& other);

    virtual const char* getName() const override;
    virtual double getWeight(double value) const override;
    virtual double getSupport() const override;
};

// VTABLE: SURRENDER 0x10075388 srTriangleFilter
// class srTriangleFilter
class SR_DLL_IMPORT srTriangleFilter : public srFilter {
public:
    srTriangleFilter();
    srTriangleFilter(const srTriangleFilter& other);
    virtual ~srTriangleFilter() override;
    srTriangleFilter& operator=(const srTriangleFilter& other);

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
