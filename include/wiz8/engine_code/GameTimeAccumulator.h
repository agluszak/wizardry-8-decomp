#pragma once

#include "wiz8/engine_code/game_timer.h"

/* Address-qualified W8GameTimer specialization constructed beside the
   canonical GDCamera owner in GameData.cpp. */
class W8GameTimeAccumulator : public W8GameTimer {
public:
    W8GameTimeAccumulator(); /* 0x0043A910 */
    void SetDurationScale(float scale);
    void ResetDurationScale();
    float Update();
    float GetFrameDelta() const
    {
        return m_frame_delta;
    }
    float GetElapsed() const
    {
        return m_elapsed;
    }

private:
    float m_max_frame_delta;
    /* 0x28: elapsed ticks scaled into duration units, clamped to m_max_frame_delta. */
    float m_frame_delta;
    unsigned int m_elapsed_ticks;
    /* 0x30: running total of the scaled deltas. */
    float m_elapsed;
};

static_assert(sizeof(W8GameTimeAccumulator) == 0x34, "W8GameTimeAccumulator_must_be_0x34");

extern W8GameTimeAccumulator* g_game_time_accumulator;

extern float g_rate;
