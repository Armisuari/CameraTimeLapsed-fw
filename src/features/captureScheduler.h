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
    bool trigCapture(bool enable);
    uint8_t skippedCapture();
    uint8_t captureCount = 0;
    uint8_t skippedCaptureCount = 0;

private:
    uint32_t convertHourToEpoch(uint32_t unixTime, int hour);

    TimeInterface &_time;

    bool trigstat = false;
    uint32_t _trigEpoch;

    int startHour = 8;
    int stopHour = 17;
    uint32_t interval;
    uint32_t numCapture = 240;
};