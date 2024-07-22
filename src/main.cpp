#include <Arduino.h>

#define DEBUG_ONLY_ESP

#include <PlatformForwarder.h>
#include "connectivity/mqtthandler.h"

#include <esp32/driver/Serial_Raspi.h>
#ifndef DEBUG_ONLY_ESP
#include <esp32/driver/Time_DS3231.h>
Time_DS3231 ds3231;
#else
#include <esp32/internet/Time_ntp.h>
Time_ntp ntp;
#endif

Serial_Raspi raspi;

#ifndef DEBUG_ONLY_ESP
PlatformForwarder app(raspi, ds3231);
#else
PlatformForwarder app(raspi, ntp);
#endif


void setup()
{
    Serial.begin(115200);
    app.begin();
}

void loop()
{
    app.deviceHandler();
}