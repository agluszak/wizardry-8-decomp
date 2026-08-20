#include "wiz8/unattributed/quarantine_common.h"
#include "surrender/srClipPlane.h"
#include "wiz8/engine_code/Level.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/stScript.h"
// SYNTHETIC: WIZ8 0x004A2200
// stLightDefinition005ECDBC::`scalar deleting destructor'

/* Remaining class methods whose original translation-unit ownership is not proved. */

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



// TEMPLATE: WIZ8 0x0049DB10
// srClassSupport<srIlluminator,srNode,0,4608>::getClassID

// TEMPLATE: WIZ8 0x0049DB30
// srClassSupport<srIlluminator,srNode,0,4608>::getClassNode
// VTABLE: WIZ8 0x005ED180
// class srClassSupport<srClipPlane,srClipPlane,0,5376>

// TEMPLATE: WIZ8 0x004BDF00
// srClassSupport<srClipPlane,srClipPlane,0,5376>::getClassID

// TEMPLATE: WIZ8 0x004BDF10
// srClassSupport<srClipPlane,srClipPlane,0,5376>::getClassName

// TEMPLATE: WIZ8 0x004BDF20
// srClassSupport<srClipPlane,srClipPlane,0,5376>::getClassNode

// TEMPLATE: WIZ8 0x004BDF90
// srClassSupport<srClipPlane,srClipPlane,0,5376>::clone

// SYNTHETIC: WIZ8 0x004BDFB0
// srClassSupport<srClipPlane,srClipPlane,0,5376>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004519D0
// srClassSupport<srNode,srNode,0,4096>::getClassID

// TEMPLATE: WIZ8 0x00445EF0
// srClassSupport<srNode,srNode,0,4096>::getClassNode

// TEMPLATE: WIZ8 0x004519F0
// srClassSupport<srNode,srNode,0,4096>::clone

// TEMPLATE: WIZ8 0x004BA1B0
// srClassSupport<stLevel,srNode,0,65543>::getClassID

// TEMPLATE: WIZ8 0x004BA1C0
// srClassSupport<stLevel,srNode,0,65543>::getClassName

// TEMPLATE: WIZ8 0x004BA1D0
// srClassSupport<stLevel,srNode,0,65543>::getClassNode

/* stLight's three registry slots are this base's generic bodies, not owned
   overrides: the name and id both come from the template arguments and the node
   walk is the generic parent chain with srLight's header-visible name inlined at
   its level. Recovering the lifecycle at 0x0049C2C0 and 0x0049C430 is what
   instantiates the specialization, so these are emitted and compared again. */
// TEMPLATE: WIZ8 0x0049DC60
// srClassSupport<stLight,srLight,0,65542>::getClassID

// TEMPLATE: WIZ8 0x0049DC70
// srClassSupport<stLight,srLight,0,65542>::getClassName

// TEMPLATE: WIZ8 0x0049DC80
// srClassSupport<stLight,srLight,0,65542>::getClassNode
