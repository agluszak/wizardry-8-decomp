#pragma once

/* Live sight and the remembered sight window are stored in one byte. */
typedef unsigned char W8SightState;
enum { W8_SIGHT_UNSEEN = 0, W8_SIGHT_SEEN = 1, W8_SIGHT_RECENT = 2 };
