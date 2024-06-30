#pragma once

#include <string>
#include <functional>
#include "CONFIG.h"
#include "connectivity/mqtthandler.h"
#include <SerialInterface.h>

class PlatformForwarder
{
    public:
        PlatformForwarder(SerialInterface &device);
        bool begin();
        bool deviceHandling();

    private:
        MQTTHandler _mqtt{CONFIG_MAIN_WIFI_DEFAULT_SSID, CONFIG_MAIN_WIFI_DEFAULT_PASS, CONFIG_MAIN_SERVER, 1883};
        SerialInterface &_device;

        bool receiveCommand = false;
        std::string msgCommand;
        static PlatformForwarder* instance;
        static void sendToPlatform(std::string msg);
};