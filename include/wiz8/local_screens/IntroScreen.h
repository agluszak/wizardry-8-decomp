#pragma once

class W8BinkVideo;
struct W8Region;
struct W8RegionEvent;
extern W8BinkVideo* gpVideo;

void Function5AE9D0(void);
void Function5AEB20(void);
void ContinueAfterDarkEndingVideo005AE770(void);
unsigned char IntroScreenEnter(void);
void IntroScreenFrame(void);
unsigned char IntroScreenLeave(int leaving);
void SetValue64D8AC(unsigned long value);
/* 0x0064D8AC: which intro video the intro screen shows next; the router and
   the menu paths select it through SetValue64D8AC. */
extern unsigned long g_intro_video_index;
unsigned char IntroScreenRegionEvent(const W8RegionEvent*, W8Region*);
