#include "Serial_Raspi.h"

Serial_Raspi::Serial_Raspi(int baudRate, SerialConfig sc, int rx, int tx)
    : _baudRate(baudRate), _sc(sc), _rx(rx), _tx(tx), _running(true)
{
}

bool Serial_Raspi::begin()
{
    raspiSerial.begin(_baudRate, _sc, _rx, _tx);
    _taskThread = std::thread(&Serial_Raspi::taskFunc, this);
    return true;
}

bool Serial_Raspi::sendComm(std::string _msg)
{
    raspiSerial.print(_msg.c_str());
    return true;
}

void Serial_Raspi::setCallback(SerialCallback callback)
{
    onSerialReceived = callback;
}

void Serial_Raspi::taskFunc()
{
    while (_running)
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
}
