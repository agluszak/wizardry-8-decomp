#pragma once

#include "wiz8/engine_code/game_timer.h"

/* The 0x28-byte timer-derived helper constructed by GDCamera. Its original
   name is not exposed; the vtable address remains the stable identity. */
class W8IntervalGate : public W8GameTimer {
public:
    W8IntervalGate();                                  /* 0x0043A4E0 */
    W8IntervalGate(
        float duration, unsigned char raw_time, unsigned char set_flag_2);
                                                        /* 0x0043A500 */
    virtual ~W8IntervalGate() override;               /* 0x004218D0 */
    void Arm();                              /* 0x0043A530 */
    unsigned int PollElapsedIntervals();                      /* 0x0043A5D0 */
    unsigned char IsFinished() const { return m_finished; }
    unsigned char Load(int handle);
    unsigned char Save(int handle);

private:
    unsigned char m_finished;                     /* 0x024 */
    unsigned char m_padding_025[3];
};

static_assert(sizeof(W8IntervalGate) == 0x28,
              "W8IntervalGate_must_be_0x28");
