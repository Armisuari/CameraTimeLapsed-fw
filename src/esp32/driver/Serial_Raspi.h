#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <SerialInterface.h>
#include <HardwareSerial.h>

class Serial_Raspi : public SerialInterface
{
public:
    // Serial_Raspi(int baudRate = 115200, SerialConfig sc = SerialConfig::SERIAL_8N1, int rx = 20, int tx = 21);
    Serial_Raspi(int baudRate = 115200, SerialConfig sc = SerialConfig::SERIAL_8N1, int rx = 9, int tx = 8);
    bool begin();
    bool sendComm(std::string _msg);
    void setCallback(SerialCallback callback);
    // void loop();

private:
    HardwareSerial raspiSerial{1};

    std::string _message;
    int _baudRate, _rx, _tx;
    SerialConfig _sc;

    SerialCallback onSerialReceived;

    static void taskFunc(void *pvParameter);
};