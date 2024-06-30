#include "PlatformForwarder.h"

PlatformForwarder* PlatformForwarder::instance = nullptr;

PlatformForwarder::PlatformForwarder(SerialInterface &device)
    : _device(device)
{
    instance = this;
}

bool PlatformForwarder::begin()
{
    _mqtt.init();
    bool res = _device.begin();

    if (!res)
    {
        return false;
    }

    // _device.setCallback(sendToPlatform);
    _device.setCallback(sendToPlatform);

    return true;
}

bool PlatformForwarder::deviceHandling()
{
    receiveCommand = _mqtt.processMessage(msgCommand);
    if (receiveCommand)
    {
        _device.sendComm(msgCommand);
    }

    return true;
}

void PlatformForwarder::sendToPlatform(std::string msg)
{
    log_i("send message to platform : %s", msg.c_str());
    instance->_mqtt.publish(msg);
}