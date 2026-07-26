#include "wiz8/gameplay_boundaries.h"

#include <string.h>

// FUNCTION: WIZ8 0x0048BDC0
W8MonsterGenerator* FindMonGenByName(const char* name)
{
    W8PtrVector* generators = g_world->monster_generators;
    W8MonsterGenerator* generator;
    int index = 0;

    if (generators->count > 0) {
        do {
            W8MonsterGenerator** element;

            if (index < generators->count) {
                element = (W8MonsterGenerator**)&generators->data[index];
            } else {
                element = (W8MonsterGenerator**)generators->data;
            }
            generator = *element;

            if (strncmp(name, generator->name, sizeof(generator->name)) == 0) {
                return generator;
            }
            generators = g_world->monster_generators;
            ++index;
        } while (index < generators->count);
    }
    return 0;
}
