#include "PlatformForwarder.h"

PlatformForwarder *PlatformForwarder::instance = nullptr;

PlatformForwarder::PlatformForwarder(SerialInterface &device, TimeInterface &time, StorageInterface &storage)
    : _device(device), _time(time), _storage(storage)
{
    instance = this;
}

bool PlatformForwarder::begin()
{
    delay(4000);
    _mqtt.init();
    bool res = _device.begin() && _time.init() && _storage.init();

    if (!res)
    {
        log_e("peripheral init failed");
        return false;
    }

    _device.setCallback(sendToPlatform);

    capScheduler.begin();

    return true;
}

bool PlatformForwarder::deviceHandler()
{
    _device.loop();
    receiveCommand = _mqtt.processMessage(msgCommand);

    if (receiveCommand && msgCommand != "{\"reqConfig\" : 1}")
    {
        _device.sendComm(msgCommand); // send to raspi
    }
    else if (capScheduler.trigCapture(_mqtt.isScheduleEnabled()))
    {
        _device.sendComm("{\"capture\":1}");
    }
    else if (msgCommand == "{\"reqConfig\" : 1}")
    {
        Serial.print("readFile() : ");
        std::string configJson = _storage.readFile();
        Serial.println(configJson.c_str());
        instance->_mqtt.publish(configJson);
        msgCommand="";
    }

    return true;
}

void PlatformForwarder::sendToPlatform(std::string msg)
{
    instance->_storage.writeFile(msg);
    log_i("send message to platform : %s", msg.c_str());
    instance->_mqtt.publish(msg);
}