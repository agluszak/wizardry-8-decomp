#ifndef WIZ8_UTILITY_H
#define WIZ8_UTILITY_H

#include <wchar.h>

#include "wiz8/dice.h"

/* Local Code\UtilityFunctions.cpp. These signed screen-space shapes are
   distinct from W8ControlsRect even though the current layouts coincide. */
typedef struct W8ScreenRect {
    int left;
    int top;
    int right;
    int bottom;
} W8ScreenRect;

typedef struct W8ScreenPoint {
    int x;
    int y;
} W8ScreenPoint;

void SetDice(W8Dice* dice, unsigned char count, unsigned char sides, short base);
int RollDice(const W8Dice* dice);
int IntegerPower(int base, unsigned int exponent);
void ClampInteger(int* value, int minimum, int maximum);
void ClampUnsignedInteger(
    unsigned int* value, unsigned int minimum, unsigned int maximum);
int CompareUnsignedDescending(
    const unsigned int* first, const unsigned int* second);
int CompareSignedAscending(const int* first, const int* second);
int CompareSignedDescending(const int* first, const int* second);
char* FormatString(const char* format, ...);
wchar_t* FormatWideString(const wchar_t* format, ...);
wchar_t* ConvertStringToWide(const char* string);
char* ConvertWideStringToString(const wchar_t* string);
wchar_t* FormatUnsignedIntegerWithCommas(
    wchar_t* output, unsigned int value);
char* TitleCaseString(char* string);
float ShortestAngleDistance(float first, float second);
void UnionScreenRects(
    const W8ScreenRect* first,
    const W8ScreenRect* second,
    W8ScreenRect* result);
unsigned char ScreenPointInRect(
    const W8ScreenRect* rect, const W8ScreenPoint* point);
void WriteGameLog(int channel, const wchar_t* format, ...);
void AdjustByteByPercent(unsigned char* value, unsigned int percent);
void AdjustIntegerByPercent(unsigned int* value, unsigned int percent);
float NormalizeAngle(float angle);
void FormatDebugMessage(int channel, const char* format, ...);
/* The plain message reporter. ReadLevel.cpp uses it for load failures and
   GrCycle.cpp for a cycle with no usable LOD, so its name stays neutral. */
void ReportError00401920(const char* message);
int GetRandomCharacter(
    int require_primary, int require_secondary, int excluded_slot,
    signed char excluded_faction);

#endif
