#pragma once

#include <iosfwd>

#include "srTimer.h"

class srVariableTimer;
SR_DLL_IMPORT std::ostream& operator<<(std::ostream& stream, const srVariableTimer& timer);

// VTABLE: SURRENDER 0x10077668 srVariableTimer
// class srVariableTimer
class SR_DLL_IMPORT SR_DLL_EXPORT srVariableTimer : public srTimer {
public:
    srVariableTimer(int force_system_timer = 0, int unused = 0, int save_calibration = 1,
                    float multiplier = 1.0f, w8_ulong step_size = 0x1e);
    srVariableTimer(const srTimer& timer);
    srVariableTimer(const srVariableTimer& timer);
    /* Base srTimer destruction is the entire derived teardown. */

    srVariableTimer& operator=(const srTimer& timer);
    srVariableTimer& operator=(const srVariableTimer& timer);

    virtual char* getAscTime(char* buffer, e_timerReadControl control) override;
    virtual int pause() override;
    virtual w8_ulong resume() override;
    virtual int reset(int force_system_timer, int unused, int save_calibration) override;
    virtual w8_ulong getMsTime(e_timerReadControl control) override;
    virtual double getTime(e_timerReadControl control) override;
    virtual w8_ulong getUTime(e_timerReadControl control) override;
    virtual w8_ulong getUTime(srQuadWord& out, e_timerReadControl control) override;
    virtual w8_ulong getRawTime(e_timerReadControl control) override;
    virtual w8_ulong getRawTime(srQuadWord& out, e_timerReadControl control) override;

    void addTime(float time);
    char* getBaseAscTime(char* buffer, e_timerReadControl control);
    w8_ulong getBaseMsTime(e_timerReadControl control);
    double getBaseTime(e_timerReadControl control);
    w8_ulong getBaseUTime(e_timerReadControl control);
    w8_ulong getBaseUTime(srQuadWord& out, e_timerReadControl control);
    w8_ulong getBaseRawTime(e_timerReadControl control);
    w8_ulong getBaseRawTime(srQuadWord& out, e_timerReadControl control);
    float getMultiplier() const;
    int is_stepping() const;
    int reset(float multiplier, w8_ulong step_size, int force_system_timer, int unused,
              int save_calibration);
    void resetMultiplier();
    void setMultiplier(float multiplier);
    void setTime(float time);
    void stepBack(w8_ulong time);
    int stepBegin(w8_ulong step_size);
    w8_ulong stepEnd();
    void stepForward(w8_ulong time);

private:
    srQuadWord m_scaled_base; /* 0x868: epoch the scaled reads subtract */
    srQuadWord m_scaled_tick; /* 0x870: accumulated multiplier-scaled ticks */
    srQuadWord m_prev_tick;   /* 0x878: raw tick snapshot from the last read */
    srQuadWord m_step_ticks;  /* 0x880: ticks covered by one step */
    float m_multiplier;       /* 0x888 */
    float m_step_scale;       /* 0x88c: 1.0f / step size */
    w8_ulong m_step_size;     /* 0x890 */
    int m_stepping;           /* 0x894 */

    friend SR_DLL_IMPORT std::ostream& operator<<(std::ostream& stream,
                                                  const srVariableTimer& timer);
};

W8_ABI_ASSERT(sizeof(srVariableTimer) == 0x898, "srVariableTimer_must_be_0x898");
