#ifndef WIZ8_MONSTER_RUNTIME_H
#define WIZ8_MONSTER_RUNTIME_H

#include "wiz8/3d_code/PList.h"
#include "wiz8/layouts/gameplay_databases.h"

extern "C" {

extern W8MonsterRecord* g_monster_record_cache[1000];
extern float g_monster_record_float_scale;    /* 0x005ED4F0 */
extern int g_monster_info_iterator_index;     /* 0x00683698 */
extern unsigned char g_alternate_name_slot;   /* 0x006875EF */
extern W8WideChar g_monster_name_buffer[];    /* 0x006875C3 */

}

#endif
