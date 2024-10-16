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
    // return map(readVoltage(), 10, 13, 0, 100);
    // return (readVoltage() - 11) / (12 - 11) * 100;

    // const float MAX_VOLTAGE = 12.6;
    // const float MIN_VOLTAGE = 10.5;
    // float voltage = readVoltage();

    // if (voltage < MIN_VOLTAGE) voltage = MIN_VOLTAGE;
    // if (voltage > MAX_VOLTAGE) voltage = MAX_VOLTAGE;

    // return (voltage - MIN_VOLTAGE) * 100 / (MAX_VOLTAGE - MIN_VOLTAGE);

    const int numPoints = 11;
    float OCV_voltage[] = {12.6, 12.4, 12.2, 12.0, 11.8, 11.6, 11.4, 11.2, 11.0, 10.8, 10.5};
    int OCV_percentage[] = {100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 0};

    float voltage = readVoltage();

    // Clamp the voltage within the known OCV range
    if (voltage > OCV_voltage[0])
    {
        voltage = OCV_voltage[0];
    }

    if (voltage < OCV_voltage[numPoints - 1])
    {
        voltage = OCV_voltage[numPoints - 1];
    }

    // Find the corresponding percentage using linear interpolation
    for (int i = 0; i < numPoints - 1; i++)
    {
        if (voltage <= OCV_voltage[i] && voltage >= OCV_voltage[i + 1])
        {
            // Linear interpolation
            float percentage = OCV_percentage[i] + (voltage - OCV_voltage[i]) * (OCV_percentage[i + 1] - OCV_percentage[i]) / (OCV_voltage[i + 1] - OCV_voltage[i]);
            return (uint32_t)percentage;
        }
    }

    return 0; // Return 0 if out of range (shouldn't happen)
}