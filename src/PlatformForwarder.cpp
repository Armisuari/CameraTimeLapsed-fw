#include "PlatformForwarder.h"

PlatformForwarder *PlatformForwarder::instance = nullptr;

PlatformForwarder::PlatformForwarder(SerialInterface &device, TimeInterface &time)
    : _device(device), _time(time)
{
    instance = this;
}

bool PlatformForwarder::begin()
{
    delay(4000);
    _mqtt.init();
    bool res = _device.begin() && _time.init();

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
    log_d("receiveCommnad = %s", receiveCommand ? "true" : "false");

    if (receiveCommand)
    {
        _device.sendComm(msgCommand); // send to raspi
    }
    else if (capScheduler.trigCapture())
    {
        _device.sendComm("{\"capture\":1}");
    }

    return true;
}

void PlatformForwarder::sendToPlatform(std::string msg)
{
    log_i("send message to platform : %s", msg.c_str());
    instance->_mqtt.publish(msg);
}