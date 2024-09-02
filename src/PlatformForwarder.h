#pragma once

#include <string>
#include <functional>

#include "CONFIG.h"
#include "connectivity/mqtthandler.h"

#include <SerialInterface.h>
#include <interface/TimeInterface.h>
#include <interface/StorageInterface.h>
#include <interface/SwitchPowerInterface.h>

#include <features/captureScheduler.h>

static const unsigned int EVT_DEVICE_OFF       = BIT0;
static const unsigned int EVT_DEVICE_ON        = BIT1;
static const unsigned int EVT_DEVICE_READY     = BIT2;
static const unsigned int EVT_COMMAND_REC      = BIT3;
static const unsigned int EVT_LIVE_STREAM      = BIT4;
static const unsigned int EVT_CAPTURE_SCHED    = BIT5;

class PlatformForwarder
{
public:
    PlatformForwarder(SerialInterface &device, TimeInterface &time, StorageInterface &storage, SwitchPowerInterface &camPow, SwitchPowerInterface &devPow);
    bool begin();
    bool deviceHandler();

private:
    MQTTHandler _mqtt{CONFIG_MAIN_WIFI_DEFAULT_SSID, CONFIG_MAIN_WIFI_DEFAULT_PASS, CONFIG_MAIN_SERVER, 1883};

    SerialInterface &_device;
    TimeInterface &_time;
    StorageInterface &_storage;
    SwitchPowerInterface &_camPow;
    SwitchPowerInterface &_devPow;

    CaptureScheduleHandler capScheduler{_time};

    bool receiveCommand = false;
    std::string msgCommand;
    static PlatformForwarder *instance;

    static void sendToPlatform(std::string topic, std::string msg);
    static void callback(std::string msg);

    static EventGroupHandle_t _eventGroup;
    static QueueHandle_t _msgQueue;

    static TimerHandle_t _checkDeviceTimer;
    static void sendCommCallback(TimerHandle_t xTimer);

    static int countCamHardReset;

    bool enqueueMessage(const std::string &msgCommand);
    bool processJsonCommand(const std::string &msgCommand);
    void handleDevicePower();
    bool startCheckDeviceTimer();
    void handleCapture();
};
