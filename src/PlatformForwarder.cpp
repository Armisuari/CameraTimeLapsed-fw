#include "PlatformForwarder.h"
#include <ArduinoJson.h>
#include <WiFi.h>
// #include "esp_task_wdt.h"

#define WATCHDOG_TIMEOUT (60000 * 5) // 5 mins timeout
#define WDT_TIMEOUT 180

PlatformForwarder *PlatformForwarder::instance = nullptr;

/* STATIC */ EventGroupHandle_t PlatformForwarder::_eventGroup = NULL;

/* STATIC */ TimerHandle_t PlatformForwarder::_checkDeviceTimer = NULL;
/* STATIC */ TimerHandle_t PlatformForwarder::_captureTimer = NULL;
/* STATIC */ TimerHandle_t PlatformForwarder::_watchDogTimer = NULL;

/* STATIC */ QueueHandle_t PlatformForwarder::_msgQueue = NULL;

/* STATIC */ TaskHandle_t PlatformForwarder::captureSchedulerTaskHandle = NULL;
/* STATIC */ TaskHandle_t PlatformForwarder::deviceHandlerTaskHandle = NULL;

/* STATIC */ int PlatformForwarder::countHardReset = 0;
/* STATIC */ uint8_t PlatformForwarder::countBle = 0;

PlatformForwarder::PlatformForwarder(SerialInterface &device, TimeInterface &time, StorageInterface &storage, SwitchPowerInterface &camPow, SwitchPowerInterface &devPow, PowerSensorInterface &senPow)
    : _device(device), _time(time), _storage(storage), _camPow(camPow), _devPow(devPow), _senPow(senPow)
{
    instance = this;
    capScheduler = new CaptureScheduleHandler(_time);
}

bool PlatformForwarder::begin()
{
    delay(4000); // Initial delay
    initPowerModules();

    if (!initEventGroup() || !initMsgQueue())
        return false;
    if (!initPeripherals())
        return false;

    JsonDocument jsonDoc;

    handleLastCommand();
    log_w("msgCommand = %s", msgCommand.c_str());

    log_d("Initialization complete, publishing system log");
    pubSyslog("init done");

    createMainTasks();

    if (!startWatchdogTimer())
    {
        log_e("failed to run wdt");
        pubSyslog("failed to run wdt");
        esp_restart();
    }

    log_w("msgCommand = %s", msgCommand.c_str());
    return true;
}

void PlatformForwarder::initPowerModules()
{
    _camPow.init();
    _devPow.init();
    _senPow.init();

    log_i("Turning on camera power");
    _camPow.on();
    _devPow.off(); // It seems calling off twice is redundant, so keep it once.
}

bool PlatformForwarder::initEventGroup()
{
    _eventGroup = xEventGroupCreate();
    if (_eventGroup == NULL)
    {
        log_e("Failed to create event group");
        return false;
    }
    xEventGroupSetBits(_eventGroup, EVT_DEVICE_OFF);
    return true;
}

bool PlatformForwarder::initMsgQueue()
{
    _msgQueue = xQueueCreate(40, sizeof(std::string));
    if (_msgQueue == NULL)
    {
        log_e("Failed to create msgQueue");
        return false;
    }
    return true;
}

bool PlatformForwarder::initPeripherals()
{
    _mqtt.init();
    _mqtt.publish("angkasa/checkDevice", "{\"deviceReady\" : 0}");

    bool res = _device.begin() && _time.init() && _storage.init();
    if (!res)
    {
        log_e("Peripheral initialization failed");
        return false;
    }

    _device.setCallback(callback);

    capScheduler->begin();
    delay(2000); // Let the scheduler stabilize after initialization

    capScheduler->captureCount = std::move(atoi(_storage.readNumCapture().c_str()));
    log_d("Loaded capture count data: %d", capScheduler->captureCount);

    return true;
}

void PlatformForwarder::handleLastCommand()
{
    lastCommand = _storage.readLastCommand();
    msgCommand = std::move(lastCommand);

    if (!msgCommand.empty())
    {
        log_w("Unreceived command detected: %s", msgCommand.c_str());
        xEventGroupClearBits(_eventGroup, EVT_COMMAND_REC);
    }
}

void PlatformForwarder::createMainTasks()
{
    xTaskCreate(&PlatformForwarder::captureSchedulerTask, "capture scheduler task", 1024 * 4, this, 3, &captureSchedulerTaskHandle);
    xTaskCreate(&PlatformForwarder::deviceHandlerTask, "device handler task", 1024 * 4, this, 35, &deviceHandlerTaskHandle);
}

bool PlatformForwarder::deviceHandler()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        wifiDisconnected = true;
        if (WiFi.status() == WL_CONNECTED)
        {
            if (_watchDogTimer != NULL)
            {
                // Reset watchdog timer if connected
                log_e("resetting wdt due to wifi connected");
                xTimerReset(_watchDogTimer, 0);
            }
            wifiDisconnected = false;
        }
    }

    // if (!msgCommand.empty())
    // {
    //     log_e("!msgCommand.empty()");
    //     log_w("msgCommand = %s", msgCommand.c_str());
    //     receiveCommand = true;
    // }

    receiveCommand = _mqtt.processMessage(msgCommand);

    if (!receiveCommand)
    {
        return false;
    }

    log_d("Receiving command: %s", msgCommand.c_str());
    pubSyslog("Receiving command");

    if (!processJsonCommand(msgCommand))
    {
        log_e("failed to process Json Command");
        msgCommand.clear();
        return false;
    }

    log_i("waiting for device ready after receive command...");
    pubSyslog("waiting device ready");

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

void PlatformForwarder::pubSyslog(std::string logMessage)
{
    log_d("publish syslog");
    JsonDocument doc;
    std::string docStr;
    doc["time"] = time(NULL);
    doc["msg"] = logMessage;

    serializeJson(doc, docStr);
    // serializeJson(doc, Serial);
    _mqtt.publish("angkasa/syslog", docStr);
}

bool PlatformForwarder::processJsonCommand(const std::string &msgCommand)
{
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, msgCommand.c_str());

    EventBits_t uxBits = xEventGroupGetBits(_eventGroup);

    if (error)
    {
        log_e("deserializeJson failed: %s", error.f_str());
        pubSyslog("deserializeJson failed");
        return false;
    }

    if (doc["pwr"] == 1)
    {
        log_d("Request device on");
        _devPow.oneCycleOn();
        pubSyslog("Request to turn on device");
        return false;
    }
    else if (doc["reqConfig"] == 1)
    {
        log_d("request camera config");
        std::string configJson = _storage.readFile();

        if (configJson == "")
        {
            log_e("configJson empty");
            pubSyslog("configJson empty");
            return false;
        }

        DynamicJsonDocument docConfig(4096);
        DeserializationError error = deserializeJson(docConfig, configJson.c_str());

        if (error)
        {
            log_e("deserializeJson failed: %s", error.f_str());
            pubSyslog("deserializeJson failed");
            return false;
        }

        docConfig["battery"] = _senPow.readVoltage();
        docConfig["battery_percentage"] = _senPow.batPercent();

        std::string newConfigJson;
        serializeJson(docConfig, newConfigJson);
        // serializeJsonPretty(docConfig, Serial);

        log_d("readFile(): %s", newConfigJson.c_str());
        _mqtt.publish("angkasa/settings", newConfigJson);
        pubSyslog("request camera config");
        return false;
    }
    else if (msgCommand.find("shu") != std::string::npos)
    {
        suspendCaptureSchedule();
        pubSyslog("Set Camera Config");
    }
    else if (doc["stream"] == 1)
    {
        suspendCaptureSchedule();
        if (uxBits & EVT_LIVE_STREAM)
        {
            log_d("block command, already live stream");
            pubSyslog("block command, already live stream");
            return false;
        }
        pubSyslog("request to start live stream");
    }
    else if (doc["stream"] == 0)
    {
        if (!(uxBits & EVT_LIVE_STREAM))
        {
            log_d("block command, already not live stream");
            pubSyslog("block command, already not live stream");
            return false;
        }
        pubSyslog("request stop live stream");
    }
    else if (msgCommand.find("restart") != std::string::npos)
    {
        log_i("System reset, triggered by command");
        pubSyslog("System reset, triggered by command");
        esp_restart();
        return false;
    }

    log_d("writing last command to fs");

    vTaskDelay(1000);
    if (!_storage.writeLastCommand(msgCommand.c_str()))
    {
        log_e("failed to write last command on fs");
    }

    log_d("executing command !");
    return true;
}

void PlatformForwarder::handleDevicePower()
{
    EventBits_t uxBits = xEventGroupGetBits(_eventGroup);

    if (uxBits & EVT_DEVICE_OFF)
    {
        log_w("Turning on device");
        _devPow.oneCycleOn();
        pubSyslog("turning on device");
    }

    if (uxBits & EVT_DEVICE_READY)
    {
        log_w("Device was ready");
        pubSyslog("Device was ready.");
        return;
    }

    xEventGroupWaitBits(_eventGroup, EVT_DEVICE_READY, pdTRUE, pdFALSE, portMAX_DELAY);
}

bool PlatformForwarder::startCheckDeviceTimer()
{
    _checkDeviceTimer = xTimerCreate("checkDeviceTimer", pdMS_TO_TICKS(30000), pdTRUE, this, sendCommCallback);

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
    bool scheduleEnabled = _mqtt.isScheduleEnabled();
    EventBits_t uxBits = xEventGroupGetBits(_eventGroup);

    if (capScheduler->trigCapture(scheduleEnabled))
    {
        if (uxBits & EVT_DEVICE_OFF)
        {
            log_w("Turning on device for capture");
            pubSyslog("Turning on device for capture");
            _devPow.oneCycleOn();
        }

        log_w("capture schedule is waiting for device ready...");
        pubSyslog("capture schedule is waiting for device ready...");
        _captureTimer = xTimerCreate("captureTimer", pdMS_TO_TICKS(60000), pdTRUE, this, sendCaptureCallback);

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

        if (!(uxBits & EVT_DEVICE_READY))
        {
            xEventGroupWaitBits(_eventGroup, EVT_DEVICE_READY, pdTRUE, pdFALSE, portMAX_DELAY);
        }

        xTimerDelete(_captureTimer, 0);

        log_w("Triggering capture due to schedule");
        pubSyslog("Triggering capture due to schedule");
        _device.sendComm("{\"capture\":1}");
    }
}

///// STATIC ///////

void PlatformForwarder::handleDeviceState(bool on)
{
    if (on)
    {
        log_w("Device turned on");
        xEventGroupClearBits(_eventGroup, EVT_DEVICE_OFF | EVT_DEVICE_READY);
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_ON);
        instance->pubSyslog("Device turned on");
    }
    else
    {
        log_e("Device turned off");
        xEventGroupClearBits(_eventGroup, EVT_DEVICE_ON | EVT_DEVICE_READY);
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_OFF);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        log_i("Backup to Google Drive successful");
        instance->pubSyslog("Backup completed");
        instance->_devPow.off();
        instance->pubSyslog("Device turned off");
    }
}

void PlatformForwarder::handleStreamState(bool start)
{
    if (start)
    {
        log_w("Live stream started");
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY | EVT_COMMAND_REC | EVT_LIVE_STREAM);
        instance->pubSyslog("Live stream started");
    }
    else
    {
        log_w("Live stream stopped");
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY | EVT_COMMAND_REC);
        xEventGroupClearBits(_eventGroup, EVT_LIVE_STREAM);
        instance->resumeCaptureSchedule();
        instance->pubSyslog("Live stream stopped");
    }
}

void PlatformForwarder::handleBleConnection(bool connected)
{
    if (connected)
    {
        log_w("BLE connected");
        countBle = 0;
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY);
        instance->_mqtt.publish("angkasa/checkDevice", "{\"deviceReady\": 1}");
        instance->pubSyslog("Camera BLE connected");
    }
    else
    {
        countBle++;
        log_e("BLE disconnected (%d times)", countBle);
        xEventGroupClearBits(_eventGroup, EVT_DEVICE_READY);
        instance->pubSyslog("Camera BLE disconnected, attempting to reconnect...");
    }
}

void PlatformForwarder::processCameraConfig(const std::string &msg)
{
    JsonDocument docConfig;
    DeserializationError error = deserializeJson(docConfig, msg.c_str());

    if (error)
    {
        log_e("Failed to deserialize JSON: %s", error.f_str());
        instance->pubSyslog("JSON deserialization failed");
        return;
    }

    docConfig["battery"] = instance->_senPow.readVoltage();
    docConfig["battery_percentage"] = instance->_senPow.batPercent();

    std::string newMsg;
    serializeJson(docConfig, newMsg);

    instance->_storage.writeFile(msg);
    log_i("Sending config to platform: %s", newMsg.c_str());
    instance->_mqtt.publish("angkasa/settings", newMsg);
}

void PlatformForwarder::callback(std::string msg)
{
    log_d("Received message: %s", msg.c_str());

    if (msg == "Raspi turned on")
    {
        handleDeviceState(true);
    }
    else if (msg.find("shutdown") != std::string::npos)
    {
        handleDeviceState(false);
    }
    else if (msg.find("captured") != std::string::npos)
    {
        xEventGroupClearBits(_eventGroup, EVT_DEVICE_OFF);
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY | EVT_CAMERA_CAPTURED);

        // if (_captureTimer != NULL)
        // {
        //     log_e("delete timer capture");
        //     xTimerReset(_captureTimer, 0);
        // }

        instance->capScheduler->captureCount++;
        instance->_storage.writeNumCapture(std::to_string(instance->capScheduler->captureCount));
        log_i("Capture completed (%d)", instance->capScheduler->captureCount);
        instance->pubSyslog("Camera captured");
    }
    else if (msg == "recConfig")
    {
        log_w("New camera config received");
        xEventGroupSetBits(_eventGroup, EVT_DEVICE_READY | EVT_COMMAND_REC);
        instance->_storage.deleteFileLastCommand();
        instance->resumeCaptureSchedule();
        instance->pubSyslog("New camera config received");
    }
    else if (msg == "streamStart")
    {
        handleStreamState(true);
    }
    else if (msg == "streamStop")
    {
        handleStreamState(false);
    }
    else if (msg == "streamFailed")
    {
        log_e("Live stream failed");
        countHardReset = 0;
        instance->pubSyslog("Live stream failed");
    }
    else if (msg == "ble connected")
    {
        handleBleConnection(true);
    }
    else if (msg == "ble disconnected")
    {
        handleBleConnection(false);
    }
    else if (msg.find("camera_name") != std::string::npos)
    {
        processCameraConfig(msg);
    }
}

///// STATIC ///////

/* STATIC */ void PlatformForwarder::sendCommCallback(TimerHandle_t xTimer)
{
    std::string _msgCommand = instance->msgCommand;
    log_i("send command callback triggered: %s", _msgCommand.c_str());
    JsonDocument doc;
    deserializeJson(doc, _msgCommand.c_str());

    EventBits_t uxBits = xEventGroupGetBits(_eventGroup);
    if (uxBits & EVT_DEVICE_OFF)
    {
        log_w("turn on device...");
        instance->_devPow.oneCycleOn();
    }

    static int count;
    if (doc["stream"] == 0)
    {
        if (uxBits & EVT_LIVE_STREAM)
        {
            count += 1;
            if (count >= 3)
            {
                esp_restart();
            }
        }
        else
        {
            count = 0;
        }
    }

    instance->_device.sendComm(_msgCommand);
    // log_d("conting try = %d", countHardReset);
}

/*STATIC*/ void PlatformForwarder::captureSchedulerTask(void *pvParam)
{
    log_d("Capture Scheduler Task Start !");
    while (true)
    {
        instance->handleCapture();
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

/*STATIC*/ void PlatformForwarder::deviceHandlerTask(void *pvParameter)
{
    // PlatformForwarder *instance = reinterpret_cast<PlatformForwarder *>(pvParameter);
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
    static int devHardRes;
    devHardRes += 1;
    log_d("capture timer callback start for = %d", devHardRes);
    EventBits_t uxBits = xEventGroupGetBits(_eventGroup);

    if (uxBits & EVT_DEVICE_OFF)
    {
        log_w("Re-turning on Device");
        instance->pubSyslog("Re-turning on Device");
        instance->_devPow.oneCycleOn();
    }

    // if (devHardRes >= 12)
    // {
    //     log_e("device somehow couldn't turned on...");
    //     // log_w("re-booting device...");
    //     instance->pubSyslog("device somehow couldn't turned on...");
    //     devHardRes = 0;
    //     camHardRes = 0;
    // }

    // instance->_device.sendComm("{\"capture\":1}");

    // if (!(uxBits & EVT_DEVICE_READY))
    // {
    //     if (camHardRes >= 12)
    //     {
    //         log_e("camera not response for 5 minutes. hard resetting...");
    //         instance->_camPow.oneCycleOn();
    //         // instance->_devPow.oneCycleOn();
    //         camHardRes = 0;
    //     }
    // }
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

bool PlatformForwarder::startWatchdogTimer()
{
    _watchDogTimer = xTimerCreate("Watchdog Timer", pdMS_TO_TICKS(WATCHDOG_TIMEOUT), pdTRUE, this, watchDogTimerCallback);

    if (_watchDogTimer == NULL)
    {
        log_e("Failed to create the wdt!");
        return false;
    }

    if (xTimerStart(_watchDogTimer, 0) != pdPASS)
    {
        log_e("Failed to start the wdt!");
        return false;
    }

    return true;
}

/*STATIC*/ void PlatformForwarder::watchDogTimerCallback(TimerHandle_t xTimer)
{
    log_i("watchdog timer triggered...");
    // PlatformForwarder *instance = reinterpret_cast<PlatformForwarder *>(pvTimerGetTimerID(xTimer));
    static uint16_t count;
    log_w("wifiDisconnected = %d", instance->wifiDisconnected);
    if (instance->wifiDisconnected)
    {
        log_e("modem wifi not connected for 5 mins");
        instance->_camPow.oneCycleOn();
    }

    count += 1;
    if (count >= 360) // 3 hour lapsed
    {
        log_w("3 hour lapsed, rebooting system");
        instance->pubSyslog("3 hour lapsed, rebooting system");
        esp_restart();
    }
}