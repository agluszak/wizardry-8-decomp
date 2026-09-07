#include "wiz8/engine_code/Level.h"
#include "wiz8/engine_code/stLight.h"

/* Recovered class lifecycle bodies whose exact original compilation boundary
   is still unresolved. Compiler-emitted registry/deleting glue is metadata in
   compiler_emissions/srClassSupport.h rather than source code in this unit. */

// FUNCTION: WIZ8 0x004A2220
stLightDefinition::~stLightDefinition()
{
}

// FUNCTION: WIZ8 0x004B9C00
stLevel::stLevel(srNode* parent)
    : srClassSupport<stLevel, srNode, false, 0x10007>(
          static_cast<srNode*>(0)),
      m_active(0), m_positional_13c(0)
{
    if (parent != 0) {
        setParent(parent, 1);
    }
}

// FUNCTION: WIZ8 0x004B9D10
stLevel::~stLevel()
{
}
