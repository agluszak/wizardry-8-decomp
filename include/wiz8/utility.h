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
/* The plain message reporter. ReadLevel.cpp uses it for load failures and
   GrCycle.cpp for a cycle with no usable LOD, so its name stays neutral. */
void ReportError00401920(const char* message);
int GetRandomCharacter(
    int require_primary, int require_secondary, int excluded_slot,
    signed char excluded_faction);

#ifdef __cplusplus
}
#endif

#endif
