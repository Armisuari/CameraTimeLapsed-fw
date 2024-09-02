#include <Arduino.h>

#define DEBUG_ONLY_ESP

#include <PlatformForwarder.h>
#include "connectivity/mqtthandler.h"

#include <esp32/driver/Serial_Raspi.h>
#include <esp32/driver/Storage_LittleFS.h>
#include <esp32/driver/Switch_Mosfet.h>
#ifndef DEBUG_ONLY_ESP
#include <esp32/driver/Time_DS3231.h>
Time_DS3231 ds3231;
#else
#include <esp32/internet/Time_ntp.h>
Time_ntp ntp;
#endif

Serial_Raspi raspi;
Storage_LittleFS lfs;
Switch_Mosfet camPow(3);
Switch_Mosfet devPow(4);

#ifndef DEBUG_ONLY_ESP
PlatformForwarder app(raspi, ds3231);
#else
PlatformForwarder app(raspi, ntp, lfs, camPow, devPow);
#endif

void taskFunc(void *pvParam)
{
    while (true)
    {
        app.deviceHandler();
        delay(10);
    }
    vTaskDelete(NULL);
}

void setup()
{
    Serial.begin(115200);
    app.begin();

    xTaskCreate(taskFunc, "task", 1024 * 8, NULL, 10, NULL);
}

void loop()
{
    vTaskDelete(NULL);
}