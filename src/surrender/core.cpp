#include "surrender/srCore.h"
#include "surrender/srIStreamOpener.h"
#include "surrender/srMemoryPool.h"

// FUNCTION: SURRENDER 0x10015010
const char* srCore::getCopyright() const
{
    return copyright_;
}

// FUNCTION: SURRENDER 0x10015030
const char* srCore::getVersion() const
{
    return version_;
}
