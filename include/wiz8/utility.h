#ifndef WIZ8_UTILITY_H
#define WIZ8_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

void WriteGameLog(int channel, const wchar_t* format, ...);
void AdjustByteByPercent(unsigned char* value, unsigned int percent);
void AdjustIntegerByPercent(unsigned int* value, unsigned int percent);
float NormalizeAngle(float angle);
void FormatDebugMessage(int channel, const char* format, ...);
int GetRandomCharacter(
    int require_primary, int require_secondary, int excluded_slot,
    signed char excluded_faction);

#ifdef __cplusplus
}
#endif

#endif
