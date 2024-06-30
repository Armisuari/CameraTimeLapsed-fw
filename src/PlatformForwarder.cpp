#include "PlatformForwarder.h"

PlatformForwarder *PlatformForwarder::instance = nullptr;

PlatformForwarder::PlatformForwarder(SerialInterface &device)
    : _device(device)
{
    instance = this;
}

bool PlatformForwarder::begin()
{
    delay(4000);
    _mqtt.init();
    bool res = _device.begin();

    if (!res)
    {
        return false;
    }

    _device.setCallback(sendToPlatform);

    return true;
}

bool PlatformForwarder::deviceHandling()
{
    _device.loop();
    receiveCommand = _mqtt.processMessage(msgCommand);
    // log_d("receiveCommnad = %s", receiveCommand ? "true" : "false");
    if (receiveCommand)
    {
        _device.sendComm(msgCommand); // send to raspi
    }

    return true;
}

void PlatformForwarder::sendToPlatform(std::string msg)
{
    log_i("send message to platform : %s", msg.c_str());
    instance->_mqtt.publish(msg);
}