#pragma once

#include "wiz8/engine_code/game_timer.h"

/* Address-qualified W8GameTimer specialization constructed beside the
   canonical GDCamera owner in GameData.cpp. */
class W8Object0043A910 : public W8GameTimer {
public:
    W8Object0043A910();                  /* 0x0043A910 */
    virtual ~W8Object0043A910() override;
    float GetValue28() const { return m_value_28; }
    float GetValue30() const { return m_value_30; }

private:
    float m_scale_24;
    float m_value_28;
    float m_value_2c;
    float m_value_30;
};

static_assert(sizeof(W8Object0043A910) == 0x34,
              "W8Object0043A910_must_be_0x34");

extern W8Object0043A910* g_object_6598bc;
