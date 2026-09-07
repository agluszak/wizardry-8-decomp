#include "wiz8/utility.h"
#include "Font.h"

/* Shared by a main-game caller and dialog text entries. Original translation
   unit is unresolved; keep this out of UtilityFunctions.cpp's proven interval. */
// FUNCTION: WIZ8 0x00577410
void ShortenTextToWidth00577410(
    wchar_t* output, const wchar_t* text, unsigned int width, int font)
{
    wchar_t buffer[200];
    wcscpy(buffer, text);
    if (static_cast<unsigned int>(StringPixLength((unsigned short*)buffer, font)) < width) {
        wcscpy(output, buffer);
        return;
    }
    for (int index = 0; index < static_cast<int>(wcslen(buffer)); ++index) {
        if (width <= static_cast<unsigned int>(
                StringPixLengthArg(font, index + 1, (unsigned short*)buffer))) {
            --index;
            while (index >= 0) {
                if (buffer[index] != L' ' && buffer[index - 1] != L' ') {
                    buffer[index] = L'\0';
                    swprintf(output, L"%s...", buffer);
                    return;
                }
                --index;
            }
            return;
        }
    }
}
