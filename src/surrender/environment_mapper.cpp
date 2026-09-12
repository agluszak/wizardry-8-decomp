#include "surrender/srEnvironmentMapper.h"

// FUNCTION: SURRENDER 0x10035430
srEnvironmentMapper::srEnvironmentMapper() {}

// FUNCTION: SURRENDER 0x10035440
srEnvironmentMapper::srEnvironmentMapper(const srEnvironmentMapper&) {}

// FUNCTION: SURRENDER 0x10035450
srEnvironmentMapper& srEnvironmentMapper::operator=(const srEnvironmentMapper&)
{
    return *this;
}

// FUNCTION: SURRENDER 0x10035460
srEnvironmentMapper::~srEnvironmentMapper() {}

// FUNCTION: SURRENDER 0x100352e0
int srEnvironmentMapper::isActive(srVertexPipe&)
{
    return 1;
}

// GLOBAL: SURRENDER 0x100A48CC
srEnvironmentMapper srEnvironmentMapper;
