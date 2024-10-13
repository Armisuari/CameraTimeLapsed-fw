#include <Arduino.h>

#define DEBUG_ONLY_ESP

#include <PlatformForwarder.h>
#include "connectivity/mqtthandler.h"

#include <esp32/driver/Serial_Raspi.h>
#include <esp32/driver/Storage_LittleFS.h>
#include <esp32/driver/Switch_Mosfet.h>
#include <esp32/driver/PowerSensor_INA226.h>
#ifndef DEBUG_ONLY_ESP
#include <esp32/driver/Time_DS3231.h>
Time_DS3231 ds3231;
#else
#include <esp32/internet/Time_ntp.h>
Time_ntp ntp;
#endif

#include "mdns.h"
#include "ota/OTA_Handler.h"

Serial_Raspi raspi;
Storage_LittleFS lfs;
Switch_Mosfet camPow(3);
Switch_Mosfet devPow(4);
PowerSensor_INA226 senPow;

#ifndef DEBUG_ONLY_ESP
PlatformForwarder app(raspi, ds3231);
#else
PlatformForwarder app(raspi, ntp, lfs, camPow, devPow, senPow);
#endif

void setupMDNSResponder(const char *hostname);

void setup()
{
    Serial.begin(115200);
    app.begin();

    // --- ota ---
    otaHandler.setup(nullptr, nullptr);
    xTaskCreate(otaHandler.task, "OtaHandler::task", 1024 * 6, NULL, 15, NULL);

    //mdns
    setupMDNSResponder("esp_timelapse");
}

void loop()
{
    vTaskDelete(NULL);
}

void setupMDNSResponder(const char *hostname)
{
    // Serial.printf("hosname: %s\n", hostname);
    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_init());
    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_hostname_set(hostname));
    ESP_LOGI("MAIN", "MDNS hostname: %s", hostname);

    // OTA Service decription
    mdns_txt_item_t serviceTxtData[3] = {
        {"HW version", "v1"},
        // {"FW version", version},
        {"Device", "SPMD"},
        {"path", "/"}};

    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_service_add(NULL, "_http", "_tcp", 80, serviceTxtData, 3));
}