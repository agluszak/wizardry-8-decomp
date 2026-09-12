#pragma once

#include "srVertexProcessor.h"

// VTABLE: SURRENDER 0x10076c68 srEnvironmentMapper
class SR_DLL_IMPORT srEnvironmentMapper : public srVertexProcessor {
public:
    srEnvironmentMapper();
    srEnvironmentMapper(const srEnvironmentMapper& other);
    srEnvironmentMapper& operator=(const srEnvironmentMapper& other);
    virtual ~srEnvironmentMapper() override;

    virtual int isActive(srVertexPipe& pipe) override;
    virtual void process(srVertexPipe& pipe) override;
};

static_assert((sizeof(srEnvironmentMapper) == 0x04), "srEnvironmentMapper_must_be_0x04");
