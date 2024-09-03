#include "PlatformForwarder.h"
#include <ArduinoJson.h>

// #define CAPTURE_SCHEDULER

PlatformForwarder *PlatformForwarder::instance = nullptr;

/* STATIC */ EventGroupHandle_t PlatformForwarder::_eventGroup = NULL;
/* STATIC */ TimerHandle_t PlatformForwarder::_checkDeviceTimer = NULL;
/* STATIC */ int PlatformForwarder::countCamHardReset = 0;
/* STATIC */ QueueHandle_t PlatformForwarder::_msgQueue = NULL;

PlatformForwarder::PlatformForwarder(SerialInterface &device, TimeInterface &time, StorageInterface &storage, SwitchPowerInterface &camPow, SwitchPowerInterface &devPow)
    : _device(device), _time(time), _storage(storage), _camPow(camPow), _devPow(devPow)
{
    instance = this;
}

bool PlatformForwarder::begin()
{
    delay(4000);
    _camPow.init();
    _devPow.init();

    log_i("turn on camera");
    _camPow.on();
    log_i("turn on device");
    _devPow.on();

    _eventGroup = xEventGroupCreate();
    _msgQueue = xQueueCreate(40, sizeof(std::string));
    if (_msgQueue == NULL)
    {
        log_e("Failed to create msgQueue");
        return false;
    }

    _mqtt.init();
    _mqtt.publish("angkasa/checkDevice", "{\"deviceReady\" : 0}");
    bool res = _device.begin() && _time.init() && _storage.init();

    if (!res)
    {
        log_e("peripheral init failed");
        return false;
    }

    _device.setCallback(callback);

    capScheduler.begin();

    return true;
}

bool PlatformForwarder::deviceHandler()
{
    receiveCommand = _mqtt.processMessage(msgCommand);

    if (!receiveCommand)
    {
        return false;
    }

    log_d("Receiving command: %s", msgCommand.c_str());

    if (!processJsonCommand(msgCommand))
    {
        return false;
    }

    handleDevicePower();

    if (!startCheckDeviceTimer())
    {
        return false;
    }

    xEventGroupWaitBits(_eventGroup, EVT_COMMAND_REC, pdTRUE, pdFALSE, portMAX_DELAY);
    xTimerDelete(_checkDeviceTimer, 0);

    handleCapture();

    receiveCommand = false;
    return true;
}

bool PlatformForwarder::enqueueMessage(const std::string &msgCommand)
{
    std::string *msgToSend = new std::string(msgCommand);

    if (xQueueSend(_msgQueue, (void *)&msgToSend, portMAX_DELAY) != pdPASS)
    {
        log_e("Failed to queue: %s", msgCommand.c_str());
        delete msgToSend;
        return false;
    }

    return true;
}

bool PlatformForwarder::processJsonCommand(const std::string &msgCommand)
{
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, msgCommand.c_str());
    EventBits_t uxBits = xEventGroupGetBits(_eventGroup);

    if (error)
    {
        log_e("deserializeJson failed: %s", error.f_str());
        return false;
    }

    if (doc["reqConfig"] == 1)
    {
        std::string configJson = _storage.readFile();
        log_d("readFile(): %s", configJson.c_str());
        instance->_mqtt.publish("angkasa/settings", configJson);
        return false;
    }
    else if (doc["stream"] == 1)
    {
        if (uxBits & EVT_LIVE_STREAM)
        {
            log_d("block command, already live stream");
            return false;
        }
    }
    else if (doc["stream"] == 0)
    {
        log_d("stream = 0 coming");
        if (!(uxBits & EVT_LIVE_STREAM))
        {
            log_d("block command, already not live stream");
            return false;
        }
    }

    return true;
}

void PlatformForwarder::handleDevicePower()
{
    EventBits_t uxBits = xEventGroupGetBits(_eventGroup);

    if (uxBits & EVT_DEVICE_OFF)
    {
        log_w("Turning on device");
        _devPow.oneCycleOn();
        xEventGroupClearBits(_eventGroup, EVT_DEVICE_OFF);
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_ON);
    }

    xEventGroupWaitBits(_eventGroup, EVT_DEVICE_READY, pdTRUE, pdFALSE, portMAX_DELAY);
    log_w("Device is ready!");
    _mqtt.publish("angkasa/checkDevice", "{\"deviceReady\": 1}");
}

bool PlatformForwarder::startCheckDeviceTimer()
{
    _checkDeviceTimer = xTimerCreate("checkDeviceTimer", pdMS_TO_TICKS(10000), pdTRUE, this, sendCommCallback);

    if (_checkDeviceTimer == NULL)
    {
        log_e("Failed to create the timer!");
        return false;
    }

    if (xTimerStart(_checkDeviceTimer, 0) != pdPASS)
    {
        log_e("Failed to start the timer!");
        return false;
    }

    return true;
}

void PlatformForwarder::handleCapture()
{
#ifdef CAPTURE_SCHEDULER
    bool scheduleEnabled = _mqtt.isScheduleEnabled();
#else
    bool scheduleEnabled = false;
#endif

    if (capScheduler.trigCapture(scheduleEnabled))
    {
        _device.sendComm("{\"capture\":1}");
    }
    else if (capScheduler.skippedCapture() > 0)
    {
        std::string text = "{\"skippedCapture\":" + std::to_string(capScheduler.skippedCapture()) + "}";
        _device.sendComm(text);
        capScheduler.skippedCaptureCount = 0;
    }
}

void PlatformForwarder::sendToPlatform(std::string topic, std::string msg)
{
    instance->_storage.writeFile(msg);
    log_i("send message to platform : %s", msg.c_str());
    instance->_mqtt.publish(topic, msg);
}

void PlatformForwarder::callback(std::string msg)
{
    static int countBle;
    log_d("msg = %s", msg.c_str());

    if (msg == "Raspi turned on")
    {
        log_w("device turned on");
        xEventGroupClearBits(_eventGroup, EVT_DEVICE_OFF);
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_ON);
        xEventGroupClearBits(_eventGroup, EVT_DEVICE_READY);
    }
    else if (msg == "Raspi shutdown")
    {
        log_e("device turned off");
        xEventGroupClearBits(_eventGroup, EVT_DEVICE_ON);
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_OFF);
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY);
    }
    else if (msg == "captured")
    {
        instance->capScheduler.captureCount++;
    }
    else if (msg == "streamStart")
    {
        log_w("live stream is running");
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY);
        xEventGroupSetBits(_eventGroup, EVT_COMMAND_REC);
        xEventGroupSetBits(_eventGroup, EVT_LIVE_STREAM);

        countCamHardReset = 0;
    }
    else if (msg == "streamStop")
    {
        log_w("live stream end");
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY);
        xEventGroupSetBits(_eventGroup, EVT_COMMAND_REC);
        xEventGroupClearBits(_eventGroup, EVT_LIVE_STREAM);
        countCamHardReset = 0;
    }
    else if (msg == "streamFailed")
    {
        // just to keep continue live stream
        log_e("failed to run live stream");
        countCamHardReset = 0;
    }
    else if (msg == "ble connected")
    {
        log_w("ble_connected at %d times", countBle);
        countBle = 0;
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY);
    }
    else if (msg == "ble disconnected")
    {
        countBle += 1;
        log_e("ble_disconnected for %d times", countBle);
        xEventGroupClearBits(_eventGroup, EVT_DEVICE_READY);
    }
    else if (msg.find("config") != std::string::npos)
    {
        sendToPlatform("angkasa/settings", msg);
    }
}

/* STATIC */ void PlatformForwarder::sendCommCallback(TimerHandle_t xTimer)
{
    // This function will be called when the timer expires
    PlatformForwarder *instance = reinterpret_cast<PlatformForwarder *>(pvTimerGetTimerID(xTimer));

    EventBits_t uxBits = xEventGroupGetBits(_eventGroup);
    if (uxBits & EVT_DEVICE_OFF)
    {
        instance->_devPow.oneCycleOn();
    }

    std::string _msgCommand = instance->msgCommand;
    log_i("send command callback triggered: %s", _msgCommand.c_str());

    instance->_device.sendComm(_msgCommand);

    countCamHardReset += 1;
    log_d("conting try = %d", countCamHardReset);

    // hard reset camera due to unresponsive BLE
    if (countCamHardReset >= 15) //
    {
        log_e("hard resetting camera due to unresponsive BLE");
        // instance->_camPow.oneCycleOn();
        countCamHardReset = 0;
    }

    // std::string *_msgCommand;
    // // Additional check for eventQueue initialization
    // if (_msgQueue == NULL)
    // {
    //     log_e("msg queue is NULL. Task is aborting.");
    // }

    // if (xQueueReceive(_msgQueue, &_msgCommand, portMAX_DELAY) == pdPASS)
    // {
    //     log_d("msg received at timer callback: %s", _msgCommand->c_str());
    //     if (_msgCommand->c_str() == "{\"deviceReady\" : 1}")
    //     {
    //         // xTimerStop(_checkDeviceTimer, 0);
    //         xTimerDelete(_checkDeviceTimer, 0);
    //         xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY);
    //     }

    //     delete _msgCommand; // Free memory after processing the message
    // }
}