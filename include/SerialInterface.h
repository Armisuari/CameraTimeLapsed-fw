#pragma once

#include <string>
#include <Arduino.h>
#include <HardwareSerial.h>

// Define the callback function type
typedef void (*SerialCallback)(std::string message);

class SerialInterface
{
    public:
        virtual bool begin() = 0;
        virtual bool sendComm(std::string _msg) = 0;
        virtual void setCallback(SerialCallback callback) = 0;
        // virtual void loop() = 0;
};