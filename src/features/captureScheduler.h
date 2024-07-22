#pragma once

#include <string>
#include <ctime>
#include <cstdint>
#include <Arduino.h>
#include <interface/TimeInterface.h>

class CaptureScheduleHandler
{
public:
    CaptureScheduleHandler(TimeInterface &time);
    bool begin();
    bool trigCapture();

private:
    uint32_t convertHourToEpoch(uint32_t unixTime, int hour);

    TimeInterface &_time;

    bool trigstat = false;
    uint32_t _trigEpoch;

    int startHour = 9;
    int stopHour = 10;
    uint32_t interval;
    uint32_t numCapture = 240;
};