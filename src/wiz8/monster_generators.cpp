#include "wiz8/gameplay_boundaries.h"

#include <string.h>

// FUNCTION: WIZ8 0x0048BDC0
W8MonsterGenerator* FindMonGenByName(const char* name)
{
    W8GrowableVector<W8MonsterGenerator*>* generators = g_world->monster_generators;
    W8MonsterGenerator* generator;
    int index = 0;

    if (generators->GetCount() > 0) {
        do {
            generator = *generators->GetAt(index);

            if (strncmp(name, generator->name, sizeof(generator->name)) == 0) {
                return generator;
            }
            generators = g_world->monster_generators;
            ++index;
        } while (index < generators->GetCount());
    }
    return 0;
}
