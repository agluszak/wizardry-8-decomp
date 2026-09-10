#pragma once

class W8Monster;

void AudioUpdateFinish004AEFD0();
int CountSpellsOfKind(int kind);                      /* 0x004AC8F0 */
void* CreateSpellEffect004AD8A0(
    const char* mls_name, int frame, W8Monster* parent, int value, int flags);
void Function4ADD30(int enabled);
void* SpawnSpellEffect004AD080(
    const char* name, int animation, int value_1, int value_2);
