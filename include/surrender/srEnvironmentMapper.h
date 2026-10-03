#pragma once

#include "srVertexProcessor.h"

// VTABLE: SURRENDER 0x10076c68 srEnvironmentMapper
/* Provider-only processor. No known consumer imports srEnvironmentMapper;
   its exported SR.DLL methods are provider ABI, not consumer dllimport. */
class SR_DLL_EXPORT srEnvironmentMapper : public srVertexProcessor {
public:
    /* No state beyond the empty srVertexProcessor base. The class-level
       export emits the complete implicit lifecycle. */

    virtual int isActive(srVertexPipe& pipe) override;
    virtual void process(srVertexPipe& pipe) override;
};

static_assert((sizeof(srEnvironmentMapper) == 0x04), "srEnvironmentMapper_must_be_0x04");
