#pragma once

#include <string>
#include "connectivity/mqtt/MQTTPublishSource.h"

enum class SyslogType : int
{
    // Reset log
    ResetUnknown = ESP_RST_UNKNOWN,   //!< Reset reason can not be determined
    ResetPowerOn = ESP_RST_POWERON,   //!< Reset due to power-on event
    ResetExtrnal = ESP_RST_EXT,       //!< Reset by external pin (not applicable for ESP32)
    ResetSoftwre = ESP_RST_SW,        //!< Software reset via esp_restart
    ResetExcptin = ESP_RST_PANIC,     //!< Software reset due to exception/panic
    ResetWatchdg = ESP_RST_INT_WDT,   //!< Reset (software or hardware) due to interrupt watchdog
    ResetTaskWDT = ESP_RST_TASK_WDT,  //!< Reset due to task watchdog
    ResetMiscWDT = ESP_RST_WDT,       //!< Reset due to other watchdogs
    ResetDeSleep = ESP_RST_DEEPSLEEP, //!< Reset after exiting deep sleep mode
    ResetBrwnOut = ESP_RST_BROWNOUT,  //!< Brownout reset (software or hardware)
    ResetOvrSDIO = ESP_RST_SDIO,      //!< Reset over SDIO

    // Detection log
    RunWdtFailed,
    PeripheralInitFailed,
    StorageInitFailed,

    ReqOnDevice,
    ReqCamConfig,
    ReqStartLiveStream,
    ReqStopLiveStream,
    SetCamConfig,

    BlockCommLs,
    BlockCommNls,
    WaitDeviceReady,
    TurnOnDevice,
    ReTurnOnDevice,
    CameraConnected,
    CameraDisconnected,
    CameraConnectFailed,
    CameraOff,
    DeviceReady,
    DeviceOn,
    DeviceOff,
    RebootDevice,

    TrigCapture,
    SendCommand,

    CamCaptured,
    BackupSucced,
    NewCamConfig,
    LiveStreamStart,
    LiveStreamStop,
    LiveStreamFailed,
};

typedef struct
{
    uint32_t timestamp;
    SyslogType eventFlag;
    const char *paddlCH;
} SyslogPacket;

class SyslogPubSource : public MQTTPublishSource
{
public:
    SyslogPubSource(const char *clientID, std::string topic, size_t bufferSize);
    bool writeData(SyslogPacket data);

protected:
    virtual std::string readMQTTPayload();
    virtual size_t readMQTTPayload(uint8_t *buff, size_t length);

private:
    std::string _formatSensorDataJson(SyslogPacket data);
    std::string _syslogType2String(SyslogPacket data);
    char _clientID[24];
};