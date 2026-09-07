#pragma once

bool CheckCdPresent(void);

extern "C" unsigned char InitializeStandardGamingPlatform(
    void* instance, int show_command);
extern "C" void ProcessCommandLine(char* command_line);
extern "C" void GetRuntimeSettings(void);
