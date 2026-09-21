#pragma once

#include <iostream>

#include "srTimer.h"

class srVariableTimer;
SR_DLL_IMPORT std::ostream& operator<<(std::ostream& stream, const srVariableTimer& timer);

class SR_DLL_IMPORT srVariableTimer : public srTimer {
public:
    srVariableTimer(int argument_0 = 0, int argument_1 = 0, int argument_2 = 1,
                    float multiplier = 1.0f, unsigned long step_size = 0x1e);
    srVariableTimer(const srTimer& timer);
    srVariableTimer(const srVariableTimer& timer);
    virtual ~srVariableTimer() override;

    srVariableTimer& operator=(const srTimer& timer);
    srVariableTimer& operator=(const srVariableTimer& timer);

    virtual char* getAscTime(char* buffer, e_timerReadControl control) override;
    virtual int pause() override;
    virtual unsigned long resume() override;
    virtual int reset(int argument_0, int argument_1, int argument_2) override;
    virtual unsigned long getMsTime(e_timerReadControl control) override;
    virtual double getTime(e_timerReadControl control) override;
    virtual unsigned long getUTime(e_timerReadControl control) override;
    virtual unsigned long getUTime(srQuadWord& out, e_timerReadControl control) override;
    virtual unsigned long getRawTime(e_timerReadControl control) override;
    virtual unsigned long getRawTime(srQuadWord& out, e_timerReadControl control) override;

    void addTime(float time);
    char* getBaseAscTime(char* buffer, e_timerReadControl control);
    unsigned long getBaseMsTime(e_timerReadControl control);
    double getBaseTime(e_timerReadControl control);
    unsigned long getBaseUTime(e_timerReadControl control);
    unsigned long getBaseUTime(srQuadWord& out, e_timerReadControl control);
    unsigned long getBaseRawTime(e_timerReadControl control);
    unsigned long getBaseRawTime(srQuadWord& out, e_timerReadControl control);
    float getMultiplier() const;
    int is_stepping() const;
    int reset(float multiplier, unsigned long step_size, int argument_2, int argument_3,
              int argument_4);
    void resetMultiplier();
    void setMultiplier(float multiplier);
    void setTime(float time);
    void stepBack(unsigned long time);
    int stepBegin(unsigned long step_size);
    unsigned long stepEnd();
    void stepForward(unsigned long time);

private:
    srQuadWord m_scaled_base;  /* 0x868: epoch the scaled reads subtract */
    srQuadWord m_scaled_tick;  /* 0x870: accumulated multiplier-scaled ticks */
    srQuadWord m_prev_tick;    /* 0x878: raw tick snapshot from the last read */
    srQuadWord m_step_ticks;   /* 0x880: ticks covered by one step */
    float m_multiplier;        /* 0x888 */
    float m_step_scale;        /* 0x88c: 1.0f / step size */
    unsigned long m_step_size; /* 0x890 */
    int m_stepping;            /* 0x894 */

    friend std::ostream& operator<<(std::ostream& stream, const srVariableTimer& timer);
};

static_assert(sizeof(srVariableTimer) == 0x898, "srVariableTimer_must_be_0x898");
