#pragma once

bool CheckCdPresent(void);

extern "C" void ProcessCommandLine(char* command_line);
extern "C" void GetRuntimeSettings(void);

unsigned char Function4229B0(void);
void Function4229D0(void);
int ReturnZero(void);
int Function443A50(void);
void Function482740(int value);

void NoOp(void);
extern bool g_sgp_shutdown_reentered;
