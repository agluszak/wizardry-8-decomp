#pragma once

#include "srVP.h"

class SR_DLL_IMPORT srDebugVP : public srVP {
public:
    srDebugVP(srVP* processor);

protected:
    void resetInternalStatistics();
};
