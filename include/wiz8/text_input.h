#pragma once

#include "input.h"

void InitTextInputModeWithScheme(int mode);
void KillTextInputMode(void);
char AddTextInputField(
    int left, int top, int width, int height, int priority,
    const wchar_t* text, unsigned char capacity, short input_type,
    unsigned char enabled);
void RemoveTextInputField(int index);
unsigned char GetTextInputFieldLength(int index);
void SetActiveField(char index);
unsigned int HandleTextInput(const InputAtom* input);
void RenderActiveTextField(void);
