#pragma once

bool IsVoiceMuted(void);
void SetVoiceMuted(unsigned char muted);

struct W8Character;
struct W8StartupStateElement005EE748;

extern int g_special_event_0068c558;

int Function52E750(void);
W8StartupStateElement005EE748* Function52E690(
    W8Character* character, int effect, int argument, int value_1,
    unsigned int value_2);
