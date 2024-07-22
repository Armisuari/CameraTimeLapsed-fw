#include "Time_ntp.h"

bool Time_ntp::init()
{
    configTime(0, 0, ntpServer);
    return true;
}

bool Time_ntp::setCurrentTime(uint32_t unixtime)
{
    return true;
}

uint32_t Time_ntp::getCurrentTime() { return time(NULL); }

std::string Time_ntp::getTimeStamp()
{
    time_t unixTimeT = static_cast<time_t>(getCurrentTime());

    // Adjust for the specified timezone offset in seconds
    unixTimeT += timezone * 3600;

    struct tm *timeinfo = gmtime(&unixTimeT);
    if (timeinfo == nullptr)
    {
        return "Error: Invalid Unix timestamp";
    }

    char buffer[20]; // Buffer to hold the formatted string
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    return std::string(buffer);
}
