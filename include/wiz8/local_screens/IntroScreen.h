#pragma once

#include "input.h"

class W8BinkVideo;
struct W8Region;
extern W8BinkVideo* gpVideo;

void ContinueAfterDarkEndingVideo(void);
unsigned char IntroScreenEnter(void);
void IntroScreenFrame(void);
unsigned char IntroScreenLeave(int leaving);
void SetIntroVideoIndex(unsigned long value);
/* Which intro video the intro screen shows next; the router and
   the menu paths select it through SetIntroVideoIndex. */
extern unsigned long g_intro_video_index;
unsigned char IntroScreenRegionEvent(const InputAtom*, W8Region*);
