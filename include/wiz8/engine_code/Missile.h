#pragma once

/* Engine Code\Missile.cpp. The record's layout stays inside its own unit until
   a second consumer needs the fields; PathAI.cpp's AI-record dispatcher only
   needs the name and the clone's signature. */

struct W8AIMissile;

W8AIMissile* CopyAIMissile004A53A0(const W8AIMissile* source);
