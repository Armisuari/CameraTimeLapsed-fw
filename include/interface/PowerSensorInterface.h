#pragma once

#include <string>
#include <stdint.h>
#include <inttypes.h>


class PowerSensorInterface
{
    public:
        virtual bool init() = 0;
        virtual float readVoltage() = 0;
        virtual uint32_t batPercent() = 0;
};