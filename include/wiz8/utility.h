#ifndef WIZ8_UTILITY_H
#define WIZ8_UTILITY_H

#include "wiz8/local_code/ControlsRect.h"
#include <wchar.h>
#include "wiz8/wiz8_windows.h"
#include "sgp.h"

#include "wiz8/dice.h"

bool IsMessageBoxActive(void);
void RenderMessageBox(void);
void ProcessMessageBoxInput(void);

template <class T> class srVector3T;

/* Local Code\UtilityFunctions.cpp. Same four-int layout as W8ControlsRect and
   the same storage width as SGPRect, but field names and call boundaries stay
   separate: UnionScreenRects / InvalidateScreenRects consume this type, while SGP
   invalidation takes either scalars or SGPRect. No merge without a direct
   cross-API bridge. */
struct W8ScreenRect {
    int left;
    int top;
    int right;
    int bottom;
};

void SetDice(W8Dice* dice, unsigned char count, unsigned char sides, short base);
int RollDice(const W8Dice* dice);
int IntegerPower(int base, unsigned int exponent);
void ClampInteger(int* value, int minimum, int maximum);
void ClampUnsignedInteger(unsigned int* value, unsigned int minimum, unsigned int maximum);
int CompareUnsignedDescending(const unsigned int* first, const unsigned int* second);
int CompareSignedAscending(const void* first, const void* second);
int CompareSignedDescending(const void* first, const void* second);
char* FormatString(const char* format, ...);
wchar_t* FormatWideString(const wchar_t* format, ...);
wchar_t* ConvertStringToWide(const char* string);
char* ConvertWideStringToString(const wchar_t* string);
wchar_t* FormatUnsignedIntegerWithCommas(wchar_t* output, unsigned int value);
char* TitleCaseString(char* string);
void ShortenTextToWidth00577410(wchar_t* output, const wchar_t* text, unsigned int width, int font);
float ShortestAngleDistance(float first, float second);
void UnionScreenRects(const W8ScreenRect* first, const W8ScreenRect* second, W8ScreenRect* result);
bool ScreenPointInRect(const W8ScreenRect* rect, const POINT* point);
void WriteGameLog(int channel, const wchar_t* format, ...);
void AdjustByteByPercent(unsigned char* value, unsigned int percent);
void AdjustIntegerByPercent(unsigned int* value, unsigned int percent);
float NormalizeAngle(float angle);
float BearingBetween(const srVector3T<float>& from, const srVector3T<float>& to);
void FormatDebugMessage(int channel, const char* format, ...);
/* The plain message reporter. ReadLevel.cpp uses it for load failures and
   GrCycle.cpp for a cycle with no usable LOD, so its name stays neutral. */
int GetRandomCharacter(int require_primary, int require_secondary, int excluded_slot,
                       signed char excluded_gender);

extern const wchar_t g_format_d_0060aa20[];

#endif
