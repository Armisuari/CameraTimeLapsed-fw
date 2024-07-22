#pragma once

#include <Arduino.h>
#include <interface/TimeInterface.h>

class Time_ntp : public TimeInterface
{
public:
    bool init();
    uint32_t getCurrentTime();
    bool setCurrentTime(uint32_t unixtime);
    std::string getTimeStamp();

private:
    const char *ntpServer = "pool.ntp.org";
    int timezone = 8;
};