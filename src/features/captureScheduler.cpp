#include "captureScheduler.h"

CaptureScheduleHandler::CaptureScheduleHandler(TimeInterface &time)
    : _time(time)
{
}

CaptureScheduleHandler::~CaptureScheduleHandler()
{
    delete this;
}

bool CaptureScheduleHandler::begin()
{
    int maxCapture;
    // Calculate the interval in seconds (240 intervals in total)
    if (stopHour > startHour)
    {
        // interval = ((stopHour - startHour) * 60 * 60) / numCapture;
        interval = 3600 / (stopHour - startHour + 7);
        maxCapture = (stopHour - startHour) * 13;
        // if (interval < 120)
        // {
        //     log_w("interval %d is too low, forcing to minimum 2 minutes", interval);
        //     interval = 120;
        // }
        log_i("interval: %d second, max capture: %d", interval, maxCapture);
    }
    else
    {
        // interval = ((24 + stopHour - startHour) * 60 * 60) / numCapture;
        interval = 3600 / (24 + stopHour - startHour + 7);
        maxCapture = (24 + stopHour - startHour) * 13;
        log_i("interval: %d second, max capture: %d", interval, maxCapture);
    }
    log_i("Interval time : %.2f minute or %d second stop:%d start:%d", (float)interval / 60, interval, stopHour, startHour);
    return true;
}

bool CaptureScheduleHandler::trigCapture(bool enable)
{
    if (enable)
    {
        uint32_t timeNow = _time.getCurrentTime();
        uint32_t startTime;
        uint32_t stopTime = convertHourToEpoch(timeNow, stopHour);

        if (startTime == 0)
            startTime = convertHourToEpoch(timeNow, startHour);

        // only update startTime if time is reaches stopTime
        if (timeNow > stopTime)
        {
            startTime = convertHourToEpoch(timeNow, startHour);
        }
        // stopTime into next day
        if (stopTime < startTime)
        {
            stopTime += 24 * 3600;
        }
        bool isTimeRange = timeNow >= startTime && timeNow < stopTime;

        static uint64_t lastPrint;
        if (millis() - lastPrint >= 10000U)
        {
            lastPrint = millis();
            log_d("Current time: %s", _time.getTimeStamp().c_str());
            log_d("timeNow:%i startTime:%i stopTime:%i", timeNow, startTime, stopTime);
            log_d("is the current time is within the range ? : %s", isTimeRange ? "yes" : "no");
        }

        // Check if the current time is within the start and stop time range
        if (isTimeRange)
        {
            if (!trigstat)
            {
                _trigEpoch = timeNow + interval;
                trigstat = true;
            }
            static uint64_t lastPrint;
            if (millis() - lastPrint >= 10000U)
            {
                lastPrint = millis();
                log_d("Current epoch time: %u", timeNow);
                log_d("Next trigger epoch time: %u\n", _trigEpoch);
                log_w("number of captured : %d", captureCount);
            }
            if (timeNow >= _trigEpoch)
            {
                static int count;
                count += 1;
                log_d("Capture Trigger : %d\n", count);
                // if (count >= numCapture)
                //     count = 0;
                _trigEpoch = timeNow + interval;
                return true;
            }
        }
        else
        {
            // Reset trigger status if out of bounds
            trigstat = false;

            // Count total of skipped capture
            if (captureCount != 0)
            {
                // skippedCaptureCount = numCapture - captureCount;
                captureCount = 0;
            }
        }

        return false;
    }

    return false;
}

uint8_t CaptureScheduleHandler::skippedCapture()
{
    return skippedCaptureCount;
}

uint32_t CaptureScheduleHandler::convertHourToEpoch(uint32_t unixTime, int hour)
{
    // Ensure the hour is valid
    if (hour < 0 || hour > 23)
    {
        return unixTime; // Return the original time if the hour is invalid
    }

    // Convert unixTime to tm structure
    time_t rawTime = unixTime + 25200;
    struct tm *timeInfo = gmtime(&rawTime);

    // Set the hour
    timeInfo->tm_hour = hour;
    timeInfo->tm_min = 0;
    timeInfo->tm_sec = 0;

    // Convert back to epoch time
    uint32_t newUnixTimeGMT = static_cast<uint32_t>(mktime(timeInfo));

    // Minus 7 hours (25200 seconds) to convert to GMT+7
    uint32_t newUnixTimeGMT7 = newUnixTimeGMT - 25200;

    return newUnixTimeGMT7;
}