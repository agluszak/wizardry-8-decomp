#ifndef WIZ8_GAMEPLAY_BOUNDARIES_H
#define WIZ8_GAMEPLAY_BOUNDARIES_H

/* Quarantine only: move declarations to their owning subsystem before
   changing them. This header may shrink, but must not gain new APIs or become
   a compatibility facade for extracted declarations. */

#include <stddef.h>

#include "wiz8/item_tables.h"
#include "wiz8/item_instance.h"
#include "wiz8/character.h"
#include "wiz8/geometry.h"
#include "surrender/srMath.h"
#include "wiz8/startup_runtime_state.h"

/* Shared recovered Wizardry interfaces used by matching translation units. */

#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/vector.h"
#include "wiz8/combat_state.h"
#include "wiz8/magic.h"
#include "wiz8/screen_state.h"
#include "wiz8/game_state.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/layouts/encounter_tables.h"
#include "wiz8/regions.h"
#include "wiz8/ui_state.h"
#include "wiz8/utility.h"
#include "random.h"
#include "timer.h"

#endif
