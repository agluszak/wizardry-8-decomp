#pragma once

#include "Button System.h"

bool CreateMessageBox(wchar_t* text, int font, unsigned int shade, bool has_accept, bool has_cancel,
                      void (*callback)(void));
void MessageBoxAcceptMoveCallback(GUI_BUTTON* button, INT32 reason);
void MessageBoxAcceptClickCallback(GUI_BUTTON* button, INT32 reason);
void MessageBoxCancelMoveCallback(GUI_BUTTON* button, INT32 reason);
void MessageBoxCancelClickCallback(GUI_BUTTON* button, INT32 reason);
