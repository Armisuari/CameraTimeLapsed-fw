#pragma once

#include <string>
#include <functional>

#include "CONFIG.h"
#include "connectivity/mqtthandler.h"

#include <SerialInterface.h>
#include <interface/TimeInterface.h>
#include <interface/StorageInterface.h>
#include <interface/SwitchPowerInterface.h>
#include <interface/PowerSensorInterface.h>

#include <features/captureScheduler.h>

static const unsigned int EVT_DEVICE_OFF       = BIT0;
static const unsigned int EVT_DEVICE_ON        = BIT1;
static const unsigned int EVT_DEVICE_READY     = BIT2;
static const unsigned int EVT_COMMAND_REC      = BIT3;
static const unsigned int EVT_LIVE_STREAM      = BIT4;
static const unsigned int EVT_CAPTURE_SCHED    = BIT5;
static const unsigned int EVT_DEVICE_REBOOT    = BIT6;
static const unsigned int EVT_CAMERA_CAPTURED   = BIT7;

class PlatformForwarder
{
public:
    PlatformForwarder(SerialInterface &device, TimeInterface &time, StorageInterface &storage, SwitchPowerInterface &camPow, SwitchPowerInterface &devPow, PowerSensorInterface &senPow);
    bool begin();
    bool deviceHandler();

private:
    MQTTHandler _mqtt{CONFIG_MAIN_WIFI_DEFAULT_SSID, CONFIG_MAIN_WIFI_DEFAULT_PASS, CONFIG_MAIN_SERVER, 1883};

    SerialInterface &_device;
    TimeInterface &_time;
    StorageInterface &_storage;
    SwitchPowerInterface &_camPow;
    SwitchPowerInterface &_devPow;
    PowerSensorInterface &_senPow;

    // CaptureScheduleHandler capScheduler{_time};
    CaptureScheduleHandler *capScheduler;

    bool receiveCommand = false;
    std::string msgCommand;
    std::string lastCommand;
    static PlatformForwarder *instance;

    bool wifiDisconnected = false;

    void initPowerModules();
    bool initEventGroup();
    bool initMsgQueue();
    bool initPeripherals();
    void handleLastCommand();
    void createMainTasks();

    static uint8_t countBle;
    static void handleDeviceState(bool on);
    static void handleStreamState(bool start);
    static void handleBleConnection(bool connected);
    static void processCameraConfig(const std::string &msg);
    static void callback(std::string msg);
    void cbFunction();

    static EventGroupHandle_t _eventGroup;
    static QueueHandle_t _msgQueue;

    static TimerHandle_t _checkDeviceTimer;
    static TimerHandle_t _captureTimer;
    static TimerHandle_t _watchDogTimer;

    static void sendCommCallback(TimerHandle_t xTimer);
    static void sendCaptureCallback(TimerHandle_t xTimer);
    static void watchDogTimerCallback(TimerHandle_t xTimer);

    static int countHardReset;

    bool enqueueMessage(const std::string &msgCommand);
    bool processJsonCommand(const std::string &msgCommand);
    void handleDevicePower();
    bool startCheckDeviceTimer();
    void handleCapture();
    bool startWatchdogTimer();

    void resumeCaptureSchedule();
    void suspendCaptureSchedule();

    static TaskHandle_t captureSchedulerTaskHandle;
    static TaskHandle_t deviceHandlerTaskHandle;
    static TaskHandle_t heartbeatTaskHandle;
    static TaskHandle_t systemResetTaskHandle;
    static TaskHandle_t mqttListenerTaskHandle;

    static void captureSchedulerTask(void *pvParameter);
    static void deviceHandlerTask(void *pvParameter);

    void pubSyslog(std::string logMessage);
};
