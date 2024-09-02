#include "Serial_Raspi.h"

Serial_Raspi::Serial_Raspi(int baudRate, SerialConfig sc, int rx, int tx)
    : _baudRate(baudRate), _sc(sc), _rx(rx), _tx(tx)
{
}

bool Serial_Raspi::begin()
{
    raspiSerial.begin(_baudRate, _sc, _rx, _tx);

    delay(2000);
    xTaskCreate(&Serial_Raspi::taskFunc, "task func", 4096, this, 3, NULL);

    return true;
}

// void Serial_Raspi::loop()
// {
//     this->taskFunc();
// }

bool Serial_Raspi::sendComm(std::string _msg)
{
    raspiSerial.flush();
    log_d("send command %s", _msg.c_str());
    raspiSerial.print(_msg.c_str());
    return true;
}

void Serial_Raspi::setCallback(SerialCallback callback)
{
    onSerialReceived = callback;
}

void Serial_Raspi::taskFunc(void *pvParameter)
{
    Serial_Raspi *instance = reinterpret_cast<Serial_Raspi *>(pvParameter);
    // Check if data is available on Serial2

    while (true)
    {
        if (instance->raspiSerial.available())
        {
            // Read the incoming message
            std::string message = instance->raspiSerial.readStringUntil('\n').c_str();
            // Call the callback function if it is set
            if (instance->onSerialReceived != nullptr)
            {
                instance->onSerialReceived(message.c_str());
            }
        }

        delay(10);
    }

    vTaskDelete(NULL);
}
