#include "PlatformForwarder.h"
#include <ArduinoJson.h>
#include "esp_task_wdt.h"

#define CAPTURE_SCHEDULER
#define WDT_TIMEOUT 180

PlatformForwarder *PlatformForwarder::instance = nullptr;

/* STATIC */ EventGroupHandle_t PlatformForwarder::_eventGroup = NULL;
/* STATIC */ TimerHandle_t PlatformForwarder::_checkDeviceTimer = NULL;
/* STATIC */ TimerHandle_t PlatformForwarder::_captureTimer = NULL;
/* STATIC */ int PlatformForwarder::countHardReset = 0;
/* STATIC */ QueueHandle_t PlatformForwarder::_msgQueue = NULL;
/* STATIC */ TaskHandle_t PlatformForwarder::captureSchedulerTaskHandle = NULL;
/* STATIC */ TaskHandle_t PlatformForwarder::deviceHandlerTaskHandle = NULL;
/* STATIC */ TaskHandle_t PlatformForwarder::heartbeatTaskHandle = NULL;
/* STATIC */ TaskHandle_t PlatformForwarder::systemResetTaskHandle = NULL;
/* STATIC */ TaskHandle_t PlatformForwarder::mqttListenerTaskHandle = NULL;

PlatformForwarder::PlatformForwarder(SerialInterface &device, TimeInterface &time, StorageInterface &storage, SwitchPowerInterface &camPow, SwitchPowerInterface &devPow)
    : _device(device), _time(time), _storage(storage), _camPow(camPow), _devPow(devPow)
{
    instance = this;
    capScheduler = new CaptureScheduleHandler(_time);
}

bool PlatformForwarder::begin()
{
    delay(4000);
    _camPow.init();
    _devPow.init();

    log_i("turn on camera");
    _camPow.on();
    _devPow.off();
    _devPow.off();

    _eventGroup = xEventGroupCreate();
    xEventGroupSetBits(_eventGroup, EVT_DEVICE_OFF);
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

    capScheduler->begin();

    xTaskCreate(&PlatformForwarder::captureSchedulerTask, " capture scheduler task", 1024 * 4, this, 3, &captureSchedulerTaskHandle);
    xTaskCreate(&PlatformForwarder::deviceHandlerTask, " device handler task", 1024 * 4, this, 10, &deviceHandlerTaskHandle);
    // xTaskCreate(&PlatformForwarder::heartbeatTask, " heartbeat task", 1024 * 8, this, 1, &heartbeatTaskHandle);
    // xTaskCreate(&PlatformForwarder::systemResetTask, " system reset task", 1024 * 8, this, 15, &systemResetTaskHandle);
    // xTaskCreate(&PlatformForwarder::mqttListenerTask, " mqtt listener task", 1024 * 4, this, 2, &mqttListenerTaskHandle);

    delay(2000);
    // capScheduler->captureCount = std::move(std::stoi(_storage.readNumCapture()));
    capScheduler->captureCount = std::move(atoi(_storage.readNumCapture().c_str()));
    log_d("load number capture data: %d", capScheduler->captureCount);

    lastCommand = _storage.readLastCommand();
    msgCommand = std::move(lastCommand);

    if (!msgCommand.empty())
    {
        log_w("there is a last command has not received to device: %s", msgCommand.c_str());
        xEventGroupClearBits(_eventGroup, EVT_COMMAND_REC);
    }

    // Initialize Watchdog Timer
    // log_i("Initialize Watchdog Timer");
    // esp_task_wdt_init(WDT_TIMEOUT, true); // Enable panic so ESP32 restarts
    // esp_task_wdt_add(captureSchedulerTaskHandle);             // Add current thread to WDT
    // esp_task_wdt_add(deviceHandlerTaskHandle);               // Add current thread to WDT

    return true;
}

bool PlatformForwarder::deviceHandler()
{
    receiveCommand = _mqtt.processMessage(msgCommand);

    if (!msgCommand.empty())
    {
        receiveCommand = true;
    }
    else if (!receiveCommand)
    {
        return false;
    }

    log_d("Receiving command: %s", msgCommand.c_str());

    if (!processJsonCommand(msgCommand))
    {
        log_e("failed to process Json Command");
        msgCommand.clear();
        return false;
    }

    log_i("waiting for device ready after receive command...");
    handleDevicePower();
    log_d("send command to device");
    _device.sendComm(msgCommand);
    if (!startCheckDeviceTimer())
    {
        return false;
    }

    xEventGroupWaitBits(_eventGroup, EVT_COMMAND_REC, pdTRUE, pdFALSE, portMAX_DELAY);
    xTimerDelete(_checkDeviceTimer, 0);

    receiveCommand = false;
    msgCommand.clear();
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
    else if (msgCommand.find("shu") != std::string::npos)
    {
        // log_e("suspend capture scheduler");
        // vTaskSuspend(captureSchedulerTaskHandle);
        suspendCaptureSchedule();
    }
    else if (doc["stream"] == 1)
    {
        // log_e("suspend capture scheduler");
        // vTaskSuspend(captureSchedulerTaskHandle);
        suspendCaptureSchedule();
        if (uxBits & EVT_LIVE_STREAM)
        {
            log_d("block command, already live stream");
            return false;
        }
    }
    else if (doc["stream"] == 0)
    {
        if (!(uxBits & EVT_LIVE_STREAM))
        {
            log_d("block command, already not live stream");
            return false;
        }
    }
    else if (msgCommand.find("restart") != std::string::npos)
    {
        log_i("System reset, triggered by command");
        esp_restart();
        return false;
    }

    log_d("writing last command to fs");
    try
    {
        instance->_storage.writeLastCommand(msgCommand);
    }
    catch (const std::exception &e)
    {
        log_e("error : %s", e.what());
    }

    log_i("executing command !");
    return true;
}

void PlatformForwarder::handleDevicePower()
{
    EventBits_t uxBits = xEventGroupGetBits(_eventGroup);

    if (uxBits & EVT_DEVICE_OFF)
    {
        log_w("Turning on device");
        _devPow.oneCycleOn();
        // xEventGroupClearBits(_eventGroup, EVT_DEVICE_OFF);
        // xEventGroupSetBits(_eventGroup, EVT_DEVICE_ON);
    }

    if (uxBits & EVT_DEVICE_READY)
    {
        return;
    }

    xEventGroupWaitBits(_eventGroup, EVT_DEVICE_READY, pdTRUE, pdFALSE, portMAX_DELAY);
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
    EventBits_t uxBits = xEventGroupGetBits(_eventGroup);

    if (capScheduler->trigCapture(scheduleEnabled))
    {
        if (uxBits & EVT_DEVICE_OFF)
        {
            _devPow.oneCycleOn();
        }

        log_w("capture schedule is waiting for device ready...");
        _captureTimer = xTimerCreate("captureTimer", pdMS_TO_TICKS(30000), pdTRUE, this, sendCaptureCallback);

        if (_captureTimer == NULL)
        {
            log_e("Failed to create the timer!");
            return;
        }

        if (xTimerStart(_captureTimer, 0) != pdPASS)
        {
            log_e("Failed to start the timer!");
            return;
        }

        xEventGroupWaitBits(_eventGroup, EVT_DEVICE_READY, pdTRUE, pdFALSE, portMAX_DELAY);
        xTimerDelete(_captureTimer, 0);

        log_w("Triggering capture due to schedule");
        _device.sendComm("{\"capture\":1}");
    }
    else if (capScheduler->skippedCapture() > 0)
    {
        std::string text = "{\"skippedCapture\":" + std::to_string(capScheduler->skippedCapture()) + "}";
        _device.sendComm(text);
        capScheduler->skippedCaptureCount = 0;
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
    }
    else if (msg.find("shutdown") != std::string::npos)
    {
        log_e("device turned off");
        xEventGroupClearBits(_eventGroup, EVT_DEVICE_ON);
        xEventGroupClearBits(_eventGroup, EVT_DEVICE_READY);
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_OFF);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        instance->_devPow.off();
    }
    else if (msg.find("captured") != std::string::npos)
    {
        instance->capScheduler->captureCount++;
        instance->_storage.writeNumCapture(std::to_string(instance->capScheduler->captureCount));
        log_i("succed to capture by schedule for : %d", instance->capScheduler->captureCount);
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY);
    }
    else if (msg == "recConfig")
    {
        log_w("new camera config was received");
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY);
        xEventGroupSetBits(_eventGroup, EVT_COMMAND_REC);
        instance->_storage.deleteFileLastCommand();
        instance->resumeCaptureSchedule();
    }
    else if (msg == "streamStart")
    {
        log_w("live stream is running");
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY);
        xEventGroupSetBits(_eventGroup, EVT_COMMAND_REC);
        xEventGroupSetBits(_eventGroup, EVT_LIVE_STREAM);
        instance->_storage.deleteFileLastCommand();
    }
    else if (msg == "streamStop")
    {
        log_w("live stream end");
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY);
        xEventGroupSetBits(_eventGroup, EVT_COMMAND_REC);
        xEventGroupClearBits(_eventGroup, EVT_LIVE_STREAM);
        instance->_storage.deleteFileLastCommand();
        instance->resumeCaptureSchedule();
    }
    else if (msg == "streamFailed")
    {
        // just to keep continue live stream
        log_e("failed to run live stream");
        countHardReset = 0;
    }
    else if (msg == "ble connected")
    {
        log_w("ble_connected at %d times", countBle);
        countBle = 0;
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY);
        log_w("Device is ready!");
        instance->_mqtt.publish("angkasa/checkDevice", "{\"deviceReady\": 1}");
    }
    else if (msg == "ble disconnected")
    {
        countBle += 1;
        log_e("ble_disconnected for %d times", countBle);
        xEventGroupClearBits(_eventGroup, EVT_DEVICE_READY);
    }
    else if (msg.find("camera_name") != std::string::npos)
    {
        sendToPlatform("angkasa/settings", msg);
    }
}

/* STATIC */ void PlatformForwarder::sendCommCallback(TimerHandle_t xTimer)
{
    // This function will be called when the timer expires
    PlatformForwarder *instance = reinterpret_cast<PlatformForwarder *>(pvTimerGetTimerID(xTimer));

    std::string _msgCommand = instance->msgCommand;
    log_i("send command callback triggered: %s", _msgCommand.c_str());

    instance->_device.sendComm(_msgCommand);
    log_d("conting try = %d", countHardReset);
}

/*STATIC*/ void PlatformForwarder::captureSchedulerTask(void *pvParam)
{
    PlatformForwarder *instance = reinterpret_cast<PlatformForwarder *>(pvParam);
    log_d("Capture Scheduler Task Start !");
    while (true)
    {
        instance->handleCapture();
        delay(1);
    }
    vTaskDelete(NULL);
}

/*STATIC*/ void PlatformForwarder::deviceHandlerTask(void *pvParameter)
{
    log_d("device handler task start !");
    while (true)
    {
        instance->deviceHandler();
        delay(1);
    }
    vTaskDelete(NULL);
}

/*STATIC*/ void PlatformForwarder::sendCaptureCallback(TimerHandle_t xTimer)
{
    static int camHardRes;
    static int devHardRes;
    camHardRes += 1;
    devHardRes += 1;
    log_d("capture timer callback start for = %d", camHardRes);
    EventBits_t uxBits = xEventGroupGetBits(_eventGroup);
    if (uxBits & EVT_DEVICE_OFF)
    {
        if (devHardRes >= 3)
        {
            log_w("re-boot device...");
            instance->_devPow.oneCycleOn();
            devHardRes = 0;
        }
    }

    instance->_device.sendComm("{\"capture\":1}");

    if (!(uxBits & EVT_DEVICE_READY))
    {
        if (camHardRes >= 10)
        {
            log_e("camera not response for 5 minutes. hard resetting...");
            instance->_camPow.oneCycleOn();
            instance->_devPow.oneCycleOn();
            camHardRes = 0;
        }
    }
}

void PlatformForwarder::resumeCaptureSchedule()
{
    log_w("resume capture scheduler");
    if (captureSchedulerTaskHandle != NULL)
    {
        vTaskResume(captureSchedulerTaskHandle);
    }

    if (_captureTimer != NULL)
    {
        if (xTimerStart(_captureTimer, 0) != pdPASS)
        {
            log_e("Failed to resume the _captureTimer!");
            return;
        }
    }
}

void PlatformForwarder::suspendCaptureSchedule()
{
    log_w("suspend capture scheduler");
    log_w("let's goo");
    if (captureSchedulerTaskHandle != NULL)
    {
        vTaskSuspend(captureSchedulerTaskHandle);
    }
    log_w("after suspend");
    if (_captureTimer != NULL)
    {
        xTimerStop(_captureTimer, 0);
    }
}

// void PlatformForwarder::heartbeatTask(void *pvParameter)
// {
//     static uint64_t lastHeartbeat = 0;
//     uint16_t interval = 30000;
//     while (true)
//     {
//         EventBits_t uxBits = xEventGroupGetBits(_eventGroup);

//         std::string currentEvent;

//         // if (uxBits & EVT_DEVICE_OFF)
//         // {
//         //     currentEvent += "Device is OFF";
//         // }
//         // if (uxBits & EVT_DEVICE_ON)
//         // {
//         //     currentEvent += "Device is ON";
//         // }
//         // if (uxBits & EVT_DEVICE_READY)
//         // {
//         //     currentEvent += ", READY";
//         // }
//         // else if (!(uxBits & EVT_DEVICE_READY))
//         // {
//         //     currentEvent += ", NOT READY";
//         // }
//         // if (uxBits & EVT_COMMAND_REC)
//         // {
//         //     currentEvent += ", all command RECEIVED";
//         // }
//         // else if (!(uxBits & EVT_COMMAND_REC))
//         // {
//         //     currentEvent += ", command NOT RECEIVED=>";
//         //     currentEvent += instance->msgCommand;
//         // }
//         // if (uxBits & EVT_LIVE_STREAM)
//         // {
//         //     currentEvent += ", STREAMING";
//         // }
//         // if (uxBits & EVT_CAPTURE_SCHED)
//         // {
//         //     currentEvent += ", capSchedule is ON";
//         // }
//         // if (uxBits & EVT_DEVICE_REBOOT)
//         // {
//         //     currentEvent += ", REBOOT";
//         // }

//         // if (millis() - lastHeartbeat >= interval)
//         // {
//         //     uint16_t totalCaptured = instance->capScheduler->captureCount;
//         //     // std::string currentEvent = instance->lastCommand;
//         //     std::string timestamp = instance->_time.getTimeStamp();
//         //     instance->_mqtt.publish("angkasa/heartbeat",
//         //                             "{\"time\":\"" + timestamp + "\", \"event\":\"" + currentEvent + "\", \"captured\":" + std::to_string(totalCaptured) + "}");
//         //     lastHeartbeat = millis();
//         // }

//         if (uxBits & EVT_DEVICE_OFF)
//         {
//             currentEvent = "raspi off";
//         }
//         else if (uxBits & EVT_DEVICE_ON)
//         {
//             currentEvent = "raspi on";
//         }
//         else if (uxBits & EVT_DEVICE_READY)
//         {
//             currentEvent = "raspi connected to camera";
//         }
//         else if (uxBits & EVT_COMMAND_REC)
//         {
//             currentEvent = "raspi receive command";
//         }
//         else if (uxBits & EVT_LIVE_STREAM)
//         {
//             currentEvent = "camera live stream";
//         }

//         log_i("current event = %s", currentEvent.c_str());
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
// }

// void PlatformForwarder::systemResetTask(void *pvParameter)
// {
//     while (true)
//     {
//         // std::string command;
//         // instance->_mqtt.processMessage(command);
//         if (instance->msgCommand.find("esp_restart") != std::string::npos)
//         {
//             log_i("System reset, triggered by command");
//             esp_restart();
//         }
//         delay(1);
//     }
// }

// void PlatformForwarder::mqttListenerTask(void *pvParameter)
// {
//     while (true)
//     {
//         instance->_mqtt.processMessage(instance->msgCommand);
//         delay(1);
//     }
// }