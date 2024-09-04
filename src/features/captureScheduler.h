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
    ~CaptureScheduleHandler();
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

    int startHour = 0;
    int stopHour = 23;
    uint32_t interval;
    uint32_t numCapture = 650;
};