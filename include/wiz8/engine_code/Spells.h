#pragma once

class W8Monster;
class W8SpellVisual;
struct W8World;

/* The pair of pointers the spell factory hands the visual loader: the active
   world and the spell-bitmap directory. */
struct W8SpellVisualLoadContext {
    W8World* world;
    const char* directory;
};

void AudioUpdateFinish004AEFD0();
int CountSpellsOfKind(int kind); /* 0x004AC8F0 */
void* CreateSpellEffect004AD8A0(const char* mls_name, int frame, W8Monster* parent, int value,
                                int flags);
void Function4ADD30(int enabled);
void* SpawnSpellEffect004AD080(const char* name, int animation, int value_1, int value_2);
/* Load one named visual from the spell bitmap directory; the out pointer is
   set only on success. */
unsigned char LoadSpellVisualResource004AB580(const W8SpellVisualLoadContext* context,
                                              const char* name, int argument_2,
                                              W8SpellVisual** visual, int argument_4);
