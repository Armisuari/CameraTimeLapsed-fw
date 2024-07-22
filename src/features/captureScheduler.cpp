#include "captureScheduler.h"

CaptureScheduleHandler::CaptureScheduleHandler(TimeInterface &time)
: _time(time)
{
}

bool CaptureScheduleHandler::begin()
{
    // Calculate the interval in seconds (240 intervals in total)
    interval = ((stopHour - startHour) * 60 * 60) / numCapture;
    log_i("Interval time : %.2f minute or %d second", (float)interval / 60, interval);

    return true;
}

bool CaptureScheduleHandler::trigCapture()
{
    uint32_t timeNow = _time.getCurrentTime();
    uint32_t startTime = convertHourToEpoch(timeNow, startHour);
    uint32_t stopTime = convertHourToEpoch(timeNow, stopHour);

    log_i("Current time: %s", _time.getTimeStamp());

    bool isTimeRange = startTime >= startTime && startTime < stopTime;
    log_i("is the current time is within the range ? : %s", isTimeRange ? "yes" : "no");

    // Check if the current time is within the start and stop time range
    if (isTimeRange)
    {
        if (!trigstat)
        {
            _trigEpoch = timeNow + interval;
            trigstat = true;
        }
        log_i("Current epoch time: %u", timeNow);
        log_i("Next trigger epoch time: %u", _trigEpoch);

        if (timeNow >= _trigEpoch)
        {
            static int count;
            count += 1;
            log_i("Capture Trigger : %d\n", count);
            if (count >= numCapture)
                count = 0;
            _trigEpoch = timeNow + interval;
            return true;
        }
    }
    else
    {
        // Reset trigger status if out of bounds
        trigstat = false;
    }

    return false;
}

uint32_t CaptureScheduleHandler::convertHourToEpoch(uint32_t unixTime, int hour) {
    // Ensure the hour is valid
    if (hour < 0 || hour > 23) {
        return unixTime; // Return the original time if the hour is invalid
    }

    // Convert unixTime to tm structure
    time_t rawTime = unixTime;
    struct tm *timeInfo = gmtime(&rawTime);

    // Set the hour
    timeInfo->tm_hour = hour;
    timeInfo->tm_min = 0;
    timeInfo->tm_sec = 0;

    // Convert back to epoch time
    uint32_t newUnixTime = static_cast<uint32_t>(mktime(timeInfo));

    return newUnixTime;
}