#pragma once

#include "wiz8/engine_code/game_timer.h"

/* Address-qualified W8GameTimer specialization constructed beside the
   canonical GDCamera owner in GameData.cpp. */
class W8GameTimeAccumulator : public W8GameTimer {
public:
    W8GameTimeAccumulator(); /* 0x0043A910 */
    virtual ~W8GameTimeAccumulator() override;
    void SetDurationScale(float scale);
    void ResetDurationScale();
    float Update();
    float GetFrameDelta() const
    {
        return m_frame_delta_28;
    }
    float GetElapsed() const
    {
        return m_elapsed_30;
    }

private:
    float m_scale_24;
    /* 0x28: elapsed ticks scaled into duration units, clamped to m_scale_24. */
    float m_frame_delta_28;
    unsigned int m_elapsed_ticks_2c;
    /* 0x30: running total of the scaled deltas. */
    float m_elapsed_30;
};

static_assert(sizeof(W8GameTimeAccumulator) == 0x34, "W8GameTimeAccumulator0043A910_must_be_0x34");

extern W8GameTimeAccumulator* g_game_time_accumulator_6598bc;

extern float g_rate_006068EC;
