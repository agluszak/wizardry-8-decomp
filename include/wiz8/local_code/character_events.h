#pragma once

bool IsVoiceMuted(void);
void SetVoiceMuted(unsigned char muted);

struct W8Character;

int Function52E750(void);
int Function52E690(
    W8Character* character, int effect, int argument, int value_1,
    unsigned int value_2);
