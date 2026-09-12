#include "wiz8/engine_code/materials.h"

#include <cstring>

/* Trim leading/trailing spaces and fold A-Z to lowercase. Live query:
   0x00497940 sits in the gap between Engine Code\Cursor3d.cpp (upper
   0x004914e0) and Engine Code\stParticle.cpp (lower 0x00497af0). */

// FUNCTION: WIZ8 0x00497940
char* TrimAndLowercaseString(char* text)
{
    unsigned int length;
    unsigned int index;
    char copy[1024];

    length = strlen(text);
    if (length < 0x400) {
        while (text[length] == ' ') {
            text[length] = '\0';
            length -= 1;
        }
        index = 0;
        while (text[index] == ' ') {
            index += 1;
        }
        strcpy(copy, text + index);
        strcpy(text, copy);
        length = strlen(text);
        for (index = 0; index < length; ++index) {
            if (text[index] >= 'A' && text[index] <= 'Z') {
                text[index] = static_cast<char>(text[index] + ('a' - 'A'));
            }
        }
    }
    return text;
}
