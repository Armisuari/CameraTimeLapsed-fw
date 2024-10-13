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
    uint32_t numCapture = 1000; // 13 - 14 photo for a hour | schedule interval 1 minute -> Actual around 4 minute
    // 23 hour -> max 290 - 300 photo
    // 11 hour -> max 140 - 145 photo
    // 8 hour -> max 100 - 105 photo

    // 1 hour -> max 13 - 14 photo
    // 60 second = 240 second
    // 13 photo = 3600 second
    // 1 photo = 277 second =  4.6 minute
    // 100 photo =  27700 = 462 minute = 7.7 hour


    // 13 photo = 1 hour, interval -> 60 second | 240 second


    // if (hour <= 1 && numPhoto > 13)
    // interval = hour * 60 * 60 / numPhoto 
};