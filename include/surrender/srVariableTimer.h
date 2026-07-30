#pragma once

#include <iostream>

#include "srTimer.h"

class SR_DLL_IMPORT srVariableTimer : public srTimer {
public:
    srVariableTimer(
        int argument_0,
        int argument_1,
        int argument_2,
        float multiplier,
        unsigned long step_size);
    srVariableTimer(const srTimer& timer);
    srVariableTimer(const srVariableTimer& timer);
    virtual ~srVariableTimer() override;

    srVariableTimer& operator=(const srTimer& timer);
    srVariableTimer& operator=(const srVariableTimer& timer);

    virtual char* getAscTime(
        char* buffer, e_timerReadControl control) override;
    virtual int pause() override;
    virtual unsigned long resume() override;
    virtual int reset(
        int argument_0, int argument_1, int argument_2) override;
    virtual unsigned long getMsTime(e_timerReadControl control) override;
    virtual double getTime(e_timerReadControl control) override;
    virtual unsigned long getUTime(e_timerReadControl control) override;
    virtual unsigned long getUTime(
        srQuadWord& out, e_timerReadControl control) override;
    virtual unsigned long getRawTime(e_timerReadControl control) override;
    virtual unsigned long getRawTime(
        srQuadWord& out, e_timerReadControl control) override;

    void addTime(float time);
    char* getBaseAscTime(char* buffer, e_timerReadControl control);
    unsigned long getBaseMsTime(e_timerReadControl control);
    double getBaseTime(e_timerReadControl control);
    unsigned long getBaseUTime(e_timerReadControl control);
    unsigned long getBaseUTime(
        srQuadWord& out, e_timerReadControl control);
    unsigned long getBaseRawTime(e_timerReadControl control);
    unsigned long getBaseRawTime(
        srQuadWord& out, e_timerReadControl control);
    float getMultiplier() const;
    int is_stepping() const;
    int reset(
        float multiplier,
        unsigned long step_size,
        int argument_2,
        int argument_3,
        int argument_4);
    void resetMultiplier();
    void setMultiplier(float multiplier);
    void setTime(float time);
    void stepBack(unsigned long time);
    int stepBegin(unsigned long step_size);
    unsigned long stepEnd();
    void stepForward(unsigned long time);

private:
    srQuadWord value_868;
    srQuadWord value_870;
    srQuadWord value_878;
    srQuadWord value_880;
    float multiplier_888;
    float value_88c;
    unsigned long value_890;
    int stepping_894;
};

SR_DLL_IMPORT std::ostream& operator<<(
    std::ostream& stream, const srVariableTimer& timer);

static_assert(sizeof(srVariableTimer) == 0x898,
              "srVariableTimer_must_be_0x898");
