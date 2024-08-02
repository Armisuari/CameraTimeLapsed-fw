#pragma once

#include <string>
#include <functional>

#include "CONFIG.h"
#include "connectivity/mqtthandler.h"

#include <SerialInterface.h>
#include <interface/TimeInterface.h>
#include <interface/StorageInterface.h>

#include <features/captureScheduler.h>

class PlatformForwarder
{
public:
    PlatformForwarder(SerialInterface &device, TimeInterface &time, StorageInterface &storage);
    bool begin();
    bool deviceHandler();

private:
    MQTTHandler _mqtt{CONFIG_MAIN_WIFI_DEFAULT_SSID, CONFIG_MAIN_WIFI_DEFAULT_PASS, CONFIG_MAIN_SERVER, 1883};

    SerialInterface &_device;
    TimeInterface &_time;
    StorageInterface &_storage;

    CaptureScheduleHandler capScheduler{_time};

    bool receiveCommand = false;
    std::string msgCommand;
    static PlatformForwarder *instance;

    static void sendToPlatform(std::string msg);
};