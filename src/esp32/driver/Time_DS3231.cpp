#include "Time_DS3231.h"

bool Time_DS3231::init()
{
    bool res = _rtc.begin();
    if (_rtc.lostPower())
    {
        log_w("RTC lost power, let's set the time!");
        _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        // rtc.adjust(DateTime(2024, 7, 7, 19, 34, 0));
    }

    // _rtc.adjust(DateTime(2024, 7, 22, 9, 0, 0));

    return res;
}

bool Time_DS3231::setCurrentTime(uint32_t unixtime)
{
    _rtc.adjust(DateTime(unixtime));

    return true;
}

uint32_t Time_DS3231::getCurrentTime()
{
    return _rtc.now().unixtime();
}

std::string Time_DS3231::getTimeStamp()
{
    return _rtc.now().timestamp(DateTime::TIMESTAMP_FULL).c_str();
}