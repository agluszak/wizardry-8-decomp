#pragma once

extern float g_path_endpoint_scale;

struct W8GameData;

W8GameData* ReadGameData00447570(const char* path, bool secondary); /* 0x00447570 */
unsigned char InitializeGameData(W8GameData* game_data);
