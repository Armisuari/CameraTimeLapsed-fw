#pragma once

#include <interface/PowerSensorInterface.h>
#include <Wire.h>
#include <INA226.h>

class PowerSensor_INA226 : public PowerSensorInterface
{
    public:
        PowerSensor_INA226();
        bool init();
        // float readVoltage();
        // uint32_t batPercent();

    private:
        INA226 INA{0x40};
};