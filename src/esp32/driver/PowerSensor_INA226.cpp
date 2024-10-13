#include "PowerSensor_INA226.h"

PowerSensor_INA226::PowerSensor_INA226()
{
}

bool PowerSensor_INA226::init()
{
    log_d("INA226 lib version: %s", INA226_LIB_VERSION);

    if (!Wire.begin(6, 7))
    // if (!Wire.begin(21, 22))
    {
        log_e("failed to begin I2C");
        return false;
    }

    if (!INA.begin())
    {
        log_e("failed to begin INA226");
        return false;
    }

    INA.setMaxCurrentShunt(1, 0.002);

    return true;
}

float PowerSensor_INA226::readVoltage()
{
    log_d("read voltage");
    return INA.getBusVoltage();
}

uint32_t PowerSensor_INA226::batPercent()
{
    return map(readVoltage(), 10, 13, 0, 100);
    // return (readVoltage() - 11) / (12 - 11) * 100;
}