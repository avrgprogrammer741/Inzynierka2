#pragma once
#include "RCRTypes.hpp"
#include <string>

class Timer {
private:
    int index;
    int timeMicroSeconds;

public:
    Timer(TimerStruct timerStruct) {
        index = std::stoi(timerStruct.index);
        timeMicroSeconds = std::stoi(timerStruct.timeMicroSeconds);
    }
    int getIndex() const { return index; }
    long getTimeMicroSeconds() const { return timeMicroSeconds; }

    void setIndex(int i) { index = i; }
    void setTimeMicroSeconds(long t) { timeMicroSeconds = t; }
};
