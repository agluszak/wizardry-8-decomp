#pragma once

#include "srHeap.h"

class SR_DLL_IMPORT srFilter {
public:
    srFilter();
    srFilter(const srFilter& other);
    virtual ~srFilter();
    srFilter& operator=(const srFilter& other);

    virtual const char* getName() const = 0;
    virtual double getWeight(double value) const = 0;
    virtual double getSupport() const = 0;
};

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
static_assert(sizeof(srBSplineFilter) == 0x04,
              "srBSplineFilter_must_be_0x04");
static_assert(sizeof(srTriangleFilter) == 0x04,
              "srTriangleFilter_must_be_0x04");

extern SR_DLL_IMPORT class srBoxFilter srBoxFilter;
extern SR_DLL_IMPORT class srBellFilter srBellFilter;
extern SR_DLL_IMPORT class srBSplineFilter srBSplineFilter;
extern SR_DLL_IMPORT class srTriangleFilter srTriangleFilter;
