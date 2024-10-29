#include "SyslogPubSource.h"

SyslogPubSource::SyslogPubSource(const char* clientID, std::string topic, size_t bufferSize) : 
MQTTPublishSource(topic, bufferSize)
{
    strncpy(_clientID, clientID, 23);
    _clientID[23] = '\0';
}

bool SyslogPubSource::writeData(SyslogPacket data)
{
    return writeRawData(&data, sizeof(SyslogPacket)) == sizeof(SyslogPacket);   
}

std::string SyslogPubSource::readMQTTPayload()
{
    SyslogPacket data;
    std::string payload;
    if(readRawData(&data, sizeof(SyslogPacket)) > 0)
    {
        payload = _formatSensorDataJson(data);
    }

    return payload;
}

// size_t SyslogPubSource::readMQTTPayload(uint8_t* buff, size_t length)
// {
//     SyslogPacket data;
//     size_t result = 0;
//     std::string payload;
//     if(readRawData(&data, sizeof(SyslogPacket)) == sizeof(SyslogPacket))
//     {
//         payload = _formatSensorDataJson(data);
//         if(payload.length() < length)
//         {
//             payload.getBytes(buff, length);
//             result = payload.length();
//         }
//     }

//     return result;
// }

std::string SyslogPubSource::_formatSensorDataJson(SyslogPacket data)
{
    std::string message = _syslogType2String(data);
    char buffer[1024];
    sprintf(buffer, 
        "{"                       \
            "\"event\":%d,"       \
            "\"message\":\"%s\","     \
            "\"time\":%d,"        \
            "\"clientId\":\"%s\"" \
        "}",
        data.eventFlag, message.c_str(), 
        data.timestamp, _clientID);

    return std::string(buffer);
}

std::string SyslogPubSource::_syslogType2String(SyslogPacket data)
{
    std::string message;
    switch (data.eventFlag)
    {
        case SyslogType::ResetUnknown:
            message ="Reset reason can not be determined";
        break;
        case SyslogType::ResetPowerOn:
            message ="Reset due to power-on event";
        break;
        case SyslogType::ResetExtrnal:
            message ="Reset by external pin (not applicable for ESP32)";
        break;
        case SyslogType::ResetSoftwre:
            message ="Software reset via esp_restart";
        break;
        case SyslogType::ResetExcptin:
            message ="Software reset due to exception/panic";
        break;
        case SyslogType::ResetWatchdg:
            message ="Reset (software or hardware) due to interrupt watchdog";
        break;
        case SyslogType::ResetTaskWDT:
            message ="Reset due to task watchdog";
        break;
        case SyslogType::ResetMiscWDT:
            message ="Reset due to other watchdogs";
        break;
        case SyslogType::ResetDeSleep:
            message ="Reset after exiting deep sleep mode";
        break;
        case SyslogType::ResetBrwnOut:
            message ="Brownout reset (software or hardware)";
        break;
        case SyslogType::ResetOvrSDIO:
            message ="Reset over SDIO";
        break;
        case SyslogType::RunWdtFailed:
            message = "failed to run wdt";
        break;
        case SyslogType::PeripheralInitFailed:
            message = "Peripheral initialization failed";
        break;
        case SyslogType::StorageInitFailed:
            message = "Storage initialization failed, reset system";
        break;
    }

    return message;
}