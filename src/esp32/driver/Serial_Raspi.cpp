#include "Serial_Raspi.h"

Serial_Raspi::Serial_Raspi(int baudRate, SerialConfig sc, int rx, int tx)
    : _baudRate(baudRate), _sc(sc), _rx(rx), _tx(tx)
{
}

bool Serial_Raspi::begin()
{
    raspiSerial.begin(_baudRate, _sc, _rx, _tx);
    return true;
}

void Serial_Raspi::loop()
{
    this->taskFunc();
}

bool Serial_Raspi::sendComm(std::string _msg)
{
    log_d("send command");
    raspiSerial.print(_msg.c_str());
    return true;
}

void Serial_Raspi::setCallback(SerialCallback callback)
{
    onSerialReceived = callback;
}

void Serial_Raspi::taskFunc()
{
    // Check if data is available on Serial2
    if (raspiSerial.available())
    {
        // Read the incoming message
        std::string message = raspiSerial.readStringUntil('\n').c_str();
        // Call the callback function if it is set
        if (onSerialReceived != nullptr)
        {
            onSerialReceived(message.c_str());
        }
    }
}
