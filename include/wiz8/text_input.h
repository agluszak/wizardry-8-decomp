#pragma once

#include "input.h"
#include "mousesystem.h"
#include <stddef.h>

struct TEXTINPUTNODE;

void InitTextInputModeWithScheme(int mode);
void KillTextInputMode(void);
char AddTextInputField(
    int left, int top, int width, int height, int priority,
    const wchar_t* text, unsigned char capacity, short input_type,
    unsigned char enabled);
void RemoveTextInputField(int index);
unsigned char GetTextInputFieldLength(int index);
void SetActiveField(char index);
short GetActiveTextInputField(void);
void SetInputFieldStringWith16BitString(unsigned char field, wchar_t* text);
void Get16BitStringFromField(unsigned char field, wchar_t* text);
void SelectNextField(void);
unsigned int HandleTextInput(const InputAtom* input);
void RenderActiveTextField(void);
void RenderAllTextFields(void);

int Function55EF80(void);
void MouseMovedInTextRegionCallback(MOUSE_REGION* region, int reason);
void MouseClickedInTextRegionCallback(MOUSE_REGION* region, int reason);
void SetTextInputScheme(int mode);
unsigned int CalculateCursorPos(int width, int cursor, const wchar_t* text,
                           int* cursor_width, size_t* visible_count);
void RenderBackgroundField(TEXTINPUTNODE* field);
void RenderInactiveTextFieldNode(TEXTINPUTNODE* field);
void SelectAllText(void);
void HandleExclusiveInput(unsigned short character);
void AddChar(unsigned short character);
void SetTextInputCursor(unsigned char cursor);
